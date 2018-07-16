/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <assert.h>
#include <iostream>
#include <cstddef>
#include <stack>
#include <deque>
#include <vector>
#include <utility>
#include <unordered_set>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"
#include "AsmRegAlloc.h"

using namespace CLRX;

#if ASMREGALLOC_DEBUGDUMP
std::ostream& operator<<(std::ostream& os, const CLRX::BlockIndex& v)
{
    if (v.pass==0)
        return os << v.index;
    else
        return os << v.index << "#" << v.pass;
}
#endif

ISAUsageHandler::ISAUsageHandler()
{ }

ISAUsageHandler::~ISAUsageHandler()
{ }

void ISAUsageHandler::pushUsage(const AsmRegVarUsage& rvu)
{
    if (!chunks.empty())
    {
        const Chunk& last = chunks.back();
        
        if ((last.offsetFirst & ~size_t(0xffff)) != (rvu.offset & ~size_t(0xffff)))
            // add new chunk
            chunks.push_back(Chunk{ rvu.offset });
    }
    else
        chunks.push_back(Chunk{ rvu.offset });
    
    const cxbyte align = rvu.align!=0 ? 32-CLZ32(rvu.align) : 0;
    chunks.back().items.push_back(RegVarUsageInt{ rvu.regVar, rvu.rstart, rvu.rend, 
                rvu.regField, rvu.rwFlags, align, rvu.useRegMode,
                uint16_t(rvu.offset & 0xffffU) });
}

AsmRegVarUsage ISAUsageHandler::nextUsage(ReadPos& readPos)
{
    const Chunk& chunk = chunks[readPos.chunkPos];
    const RegVarUsageInt& item = chunk.items[readPos.itemPos];
    readPos.itemPos++;
    // fix itemPos to zero
    if (readPos.itemPos >= chunk.items.size())
    {
        readPos.itemPos = 0;
        readPos.chunkPos++;
    }
    const cxbyte outAlign = item.align!=0 ? 1U<<(item.align-1) : 0;
    return AsmRegVarUsage{ (chunk.offsetFirst & ~size_t(0xffffU)) | item.offsetLo,
            item.regVar, item.rstart, item.rend, item.regField, item.rwFlags,
            cxbyte(outAlign), item.useRegMode!=0 };
}

ISAUsageHandler::ReadPos ISAUsageHandler::findPositionByOffset(size_t offset) const
{
    if (chunks.empty())
        return ReadPos{ 0, 0 };
    size_t chunkPos = std::lower_bound(chunks.begin(), chunks.end(), Chunk{ offset },
            [](const Chunk& a, const Chunk& b)
            { return a.offsetFirst < b.offsetFirst; }) - chunks.begin();
    // fix - move back if found offset is greater or if end
    if (chunkPos == chunks.size() ||
        (chunkPos != 0 && chunks[chunkPos].offsetFirst != offset))
        chunkPos--;
    
    size_t itemPos = 0;
    if (chunks[chunkPos].offsetFirst != offset)
    {
        const std::vector<RegVarUsageInt>& items = chunks[chunkPos].items;
        RegVarUsageInt rvu;
        rvu.offsetLo = uint16_t(offset & 0xffff);
        size_t itemPos = std::lower_bound(items.begin(), items.end(), rvu,
                [](const RegVarUsageInt& a, const RegVarUsageInt& b)
                { return a.offsetLo < b.offsetLo; }) - items.begin();
        // fix itemPos to zero
        if (itemPos >= items.size())
        {
            chunkPos++;
            itemPos = 0;
        }
    }
    return ReadPos{ chunkPos, itemPos };
}


ISALinearDepHandler::ISALinearDepHandler()
{ }

size_t ISALinearDepHandler::findPositionByOffset(size_t offset) const
{
    return std::lower_bound(regVarLinDeps.begin(), regVarLinDeps.end(),
            AsmRegVarLinearDep{offset},
            [](const AsmRegVarLinearDep& a, const AsmRegVarLinearDep& b)
            { return a.offset < b.offset; }) - regVarLinDeps.begin();
}

ISALinearDepHandler* ISALinearDepHandler::copy() const
{
    return new ISALinearDepHandler(*this);
}

/*
 * Asm register allocator stuff
 */

AsmRegAllocator::AsmRegAllocator(Assembler& _assembler) : assembler(_assembler)
{ }

AsmRegAllocator::AsmRegAllocator(Assembler& _assembler,
        const std::vector<CodeBlock>& _codeBlocks, const SSAReplacesMap& _ssaReplacesMap)
        : assembler(_assembler), codeBlocks(_codeBlocks), ssaReplacesMap(_ssaReplacesMap)
{ }

static inline bool codeBlockStartLess(const AsmRegAllocator::CodeBlock& c1,
                  const AsmRegAllocator::CodeBlock& c2)
{ return c1.start < c2.start; }

static inline bool codeBlockEndLess(const AsmRegAllocator::CodeBlock& c1,
                  const AsmRegAllocator::CodeBlock& c2)
{ return c1.end < c2.end; }

void AsmRegAllocator::createCodeStructure(const std::vector<AsmCodeFlowEntry>& codeFlow,
             size_t codeSize, const cxbyte* code)
{
    ISAAssembler* isaAsm = assembler.isaAssembler;
    if (codeSize == 0)
        return;
    std::vector<size_t> splits;
    std::vector<size_t> codeStarts;
    std::vector<size_t> codeEnds;
    codeStarts.push_back(0);
    codeEnds.push_back(codeSize);
    for (const AsmCodeFlowEntry& entry: codeFlow)
    {
        size_t instrAfter = 0;
        if (entry.type == AsmCodeFlowType::JUMP || entry.type == AsmCodeFlowType::CJUMP ||
            entry.type == AsmCodeFlowType::CALL || entry.type == AsmCodeFlowType::RETURN)
            instrAfter = entry.offset + isaAsm->getInstructionSize(
                        codeSize - entry.offset, code + entry.offset);
        
        switch(entry.type)
        {
            case AsmCodeFlowType::START:
                codeStarts.push_back(entry.offset);
                break;
            case AsmCodeFlowType::END:
                codeEnds.push_back(entry.offset);
                break;
            case AsmCodeFlowType::JUMP:
                splits.push_back(entry.target);
                codeEnds.push_back(instrAfter);
                break;
            case AsmCodeFlowType::CJUMP:
                splits.push_back(entry.target);
                splits.push_back(instrAfter);
                break;
            case AsmCodeFlowType::CALL:
                splits.push_back(entry.target);
                splits.push_back(instrAfter);
                break;
            case AsmCodeFlowType::RETURN:
                codeEnds.push_back(instrAfter);
                break;
            default:
                break;
        }
    }
    std::sort(splits.begin(), splits.end());
    splits.resize(std::unique(splits.begin(), splits.end()) - splits.begin());
    std::sort(codeEnds.begin(), codeEnds.end());
    codeEnds.resize(std::unique(codeEnds.begin(), codeEnds.end()) - codeEnds.begin());
    // remove codeStarts between codeStart and codeEnd
    size_t i = 0;
    size_t ii = 0;
    size_t ei = 0; // codeEnd i
    while (i < codeStarts.size())
    {
        size_t end = (ei < codeEnds.size() ? codeEnds[ei] : SIZE_MAX);
        if (ei < codeEnds.size())
            ei++;
        codeStarts[ii++] = codeStarts[i];
        // skip codeStart to end
        for (i++ ;i < codeStarts.size() && codeStarts[i] < end; i++);
    }
    codeStarts.resize(ii);
    // add next codeStarts
    auto splitIt = splits.begin();
    for (size_t codeEnd: codeEnds)
    {
        auto it = std::lower_bound(splitIt, splits.end(), codeEnd);
        if (it != splits.end())
        {
            codeStarts.push_back(*it);
            splitIt = it;
        }
        else // if end
            break;
    }
    
    std::sort(codeStarts.begin(), codeStarts.end());
    codeStarts.resize(std::unique(codeStarts.begin(), codeStarts.end()) -
                codeStarts.begin());
    // divide to blocks
    splitIt = splits.begin();
    for (size_t codeStart: codeStarts)
    {
        size_t codeEnd = *std::upper_bound(codeEnds.begin(), codeEnds.end(), codeStart);
        splitIt = std::lower_bound(splitIt, splits.end(), codeStart);
        
        if (splitIt != splits.end() && *splitIt==codeStart)
            ++splitIt; // skip split in codeStart
        
        for (size_t start = codeStart; start < codeEnd; )
        {
            size_t end = codeEnd;
            if (splitIt != splits.end())
            {
                end = std::min(end, *splitIt);
                ++splitIt;
            }
            codeBlocks.push_back({ start, end, { }, false, false, false });
            start = end;
        }
    }
    // force empty block at end if some jumps goes to its
    if (!codeEnds.empty() && !codeStarts.empty() && !splits.empty() &&
        codeStarts.back()==codeEnds.back() && codeStarts.back() == splits.back())
        codeBlocks.push_back({ codeStarts.back(), codeStarts.back(), { },
                             false, false, false });
    
    // construct flow-graph
    for (const AsmCodeFlowEntry& entry: codeFlow)
        if (entry.type == AsmCodeFlowType::CALL || entry.type == AsmCodeFlowType::JUMP ||
            entry.type == AsmCodeFlowType::CJUMP || entry.type == AsmCodeFlowType::RETURN)
        {
            std::vector<CodeBlock>::iterator it;
            size_t instrAfter = entry.offset + isaAsm->getInstructionSize(
                        codeSize - entry.offset, code + entry.offset);
            
            if (entry.type != AsmCodeFlowType::RETURN)
                it = binaryFind(codeBlocks.begin(), codeBlocks.end(),
                        CodeBlock{ entry.target }, codeBlockStartLess);
            else // return
            {
                it = binaryFind(codeBlocks.begin(), codeBlocks.end(),
                        CodeBlock{ 0, instrAfter }, codeBlockEndLess);
                // if block have return
                if (it != codeBlocks.end())
                    it->haveEnd = it->haveReturn = true;
                continue;
            }
            
            if (it == codeBlocks.end())
                continue; // error!
            auto it2 = std::lower_bound(codeBlocks.begin(), codeBlocks.end(),
                    CodeBlock{ instrAfter }, codeBlockStartLess);
            auto curIt = it2;
            --curIt;
            
            curIt->nexts.push_back({ size_t(it - codeBlocks.begin()),
                        entry.type == AsmCodeFlowType::CALL });
            curIt->haveCalls |= entry.type == AsmCodeFlowType::CALL;
            if (entry.type == AsmCodeFlowType::CJUMP ||
                 entry.type == AsmCodeFlowType::CALL)
            {
                curIt->haveEnd = false; // revert haveEnd if block have cond jump or call
                if (it2 != codeBlocks.end() && entry.type == AsmCodeFlowType::CJUMP)
                    // add next next block (only for cond jump)
                    curIt->nexts.push_back({ size_t(it2 - codeBlocks.begin()), false });
            }
            else if (entry.type == AsmCodeFlowType::JUMP)
                curIt->haveEnd = true; // set end
        }
    // force haveEnd for block with cf_end
    for (const AsmCodeFlowEntry& entry: codeFlow)
        if (entry.type == AsmCodeFlowType::END)
        {
            auto it = binaryFind(codeBlocks.begin(), codeBlocks.end(),
                    CodeBlock{ 0, entry.offset }, codeBlockEndLess);
            if (it != codeBlocks.end())
                it->haveEnd = true;
        }
    
    if (!codeBlocks.empty()) // always set haveEnd to last block
        codeBlocks.back().haveEnd = true;
    
    // reduce nexts
    for (CodeBlock& block: codeBlocks)
    {
        // first non-call nexts, for correct resolving SSA conflicts
        std::sort(block.nexts.begin(), block.nexts.end(),
                  [](const NextBlock& n1, const NextBlock& n2)
                  { return int(n1.isCall)<int(n2.isCall) ||
                      (n1.isCall == n2.isCall && n1.block < n2.block); });
        auto it = std::unique(block.nexts.begin(), block.nexts.end(),
                  [](const NextBlock& n1, const NextBlock& n2)
                  { return n1.block == n2.block && n1.isCall == n2.isCall; });
        block.nexts.resize(it - block.nexts.begin());
    }
}


void AsmRegAllocator::applySSAReplaces()
{
    if (ssaReplacesMap.empty())
        return; // do nothing
    
    /* prepare SSA id replaces */
    struct MinSSAGraphNode
    {
        size_t minSSAId;
        bool visited;
        std::unordered_set<size_t> nexts;
        MinSSAGraphNode() : minSSAId(SIZE_MAX), visited(false)
        { }
    };
    
    typedef std::map<size_t, MinSSAGraphNode, std::greater<size_t> > SSAGraphNodesMap;
    
    struct MinSSAGraphStackEntry
    {
        SSAGraphNodesMap::iterator nodeIt;
        std::unordered_set<size_t>::const_iterator nextIt;
        size_t minSSAId;
        
        MinSSAGraphStackEntry(
                SSAGraphNodesMap::iterator _nodeIt,
                std::unordered_set<size_t>::const_iterator _nextIt,
                size_t _minSSAId = SIZE_MAX)
                : nodeIt(_nodeIt), nextIt(_nextIt), minSSAId(_minSSAId)
        { }
    };
    
    for (auto& entry: ssaReplacesMap)
    {
        ARDOut << "SSAReplace: " << entry.first.regVar << "." << entry.first.index << "\n";
        VectorSet<SSAReplace>& replaces = entry.second;
        std::sort(replaces.begin(), replaces.end(), std::greater<SSAReplace>());
        replaces.resize(std::unique(replaces.begin(), replaces.end()) - replaces.begin());
        VectorSet<SSAReplace> newReplaces;
        
        SSAGraphNodesMap ssaGraphNodes;
        
        auto it = replaces.begin();
        while (it != replaces.end())
        {
            auto itEnd = std::upper_bound(it, replaces.end(),
                    std::make_pair(it->first, size_t(0)), std::greater<SSAReplace>());
            {
                auto itLast = itEnd;
                --itLast;
                MinSSAGraphNode& node = ssaGraphNodes[it->first];
                node.minSSAId = std::min(node.minSSAId, itLast->second);
                for (auto it2 = it; it2 != itEnd; ++it2)
                {
                    node.nexts.insert(it2->second);
                    ssaGraphNodes.insert({ it2->second, MinSSAGraphNode() });
                }
            }
            it = itEnd;
        }
        /*for (const auto& v: ssaGraphNodes)
            ARDOut << "  SSANode: " << v.first << ":" << &v.second << " minSSAID: " <<
                            v.second.minSSAId << std::endl;*/
        // propagate min value
        std::stack<MinSSAGraphStackEntry> minSSAStack;
        
        // initialize parents and new nexts
        for (auto ssaGraphNodeIt = ssaGraphNodes.begin();
                 ssaGraphNodeIt!=ssaGraphNodes.end(); )
        {
            ARDOut << "  Start in " << ssaGraphNodeIt->first << "." << "\n";
            minSSAStack.push({ ssaGraphNodeIt, ssaGraphNodeIt->second.nexts.begin() });
            // traverse with minimalize SSA id
            while (!minSSAStack.empty())
            {
                MinSSAGraphStackEntry& entry = minSSAStack.top();
                MinSSAGraphNode& node = entry.nodeIt->second;
                bool toPop = false;
                if (entry.nextIt == node.nexts.begin())
                {
                    toPop = node.visited;
                    node.visited = true;
                }
                if (!toPop && entry.nextIt != node.nexts.end())
                {
                    auto nodeIt = ssaGraphNodes.find(*entry.nextIt);
                    if (nodeIt != ssaGraphNodes.end())
                        minSSAStack.push({ nodeIt, nodeIt->second.nexts.begin(),
                                    size_t(0) });
                    ++entry.nextIt;
                }
                else
                {
                    minSSAStack.pop();
                    if (!minSSAStack.empty())
                        node.nexts.insert(minSSAStack.top().nodeIt->first);
                }
            }
            
            // skip visited nodes
            for(; ssaGraphNodeIt != ssaGraphNodes.end(); ++ssaGraphNodeIt)
                if (!ssaGraphNodeIt->second.visited)
                    break;
        }
        
        /*for (const auto& v: ssaGraphNodes)
        {
            ARDOut << "  Nexts: " << v.first << ":" << &v.second << " nexts:";
            for (size_t p: v.second.nexts)
                ARDOut << " " << p;
            ARDOut << "\n";
        }*/
        
        for (auto& entry: ssaGraphNodes)
            entry.second.visited = false;
        
        std::vector<MinSSAGraphNode*> toClear; // nodes to clear
        
        for (auto ssaGraphNodeIt = ssaGraphNodes.begin();
                 ssaGraphNodeIt!=ssaGraphNodes.end(); )
        {
            ARDOut << "  Start in " << ssaGraphNodeIt->first << "." << "\n";
            minSSAStack.push({ ssaGraphNodeIt, ssaGraphNodeIt->second.nexts.begin() });
            // traverse with minimalize SSA id
            while (!minSSAStack.empty())
            {
                MinSSAGraphStackEntry& entry = minSSAStack.top();
                MinSSAGraphNode& node = entry.nodeIt->second;
                bool toPop = false;
                if (entry.nextIt == node.nexts.begin())
                {
                    toPop = node.visited;
                    if (!node.visited)
                        // this flag visited for this node will be clear after this pass
                        toClear.push_back(&node);
                    node.visited = true;
                }
                
                // try to children only all parents are visited and if parent has children
                if (!toPop && entry.nextIt != node.nexts.end())
                {
                    auto nodeIt = ssaGraphNodes.find(*entry.nextIt);
                    if (nodeIt != ssaGraphNodes.end())
                    {
                        ARDOut << "  Node: " <<
                                entry.nodeIt->first << ":" << &node << " minSSAId: " <<
                                node.minSSAId << " to " <<
                                nodeIt->first << ":" << &(nodeIt->second) <<
                                " minSSAId: " << nodeIt->second.minSSAId << "\n";
                        nodeIt->second.minSSAId =
                                std::min(nodeIt->second.minSSAId, node.minSSAId);
                        minSSAStack.push({ nodeIt, nodeIt->second.nexts.begin(),
                                nodeIt->second.minSSAId });
                    }
                    ++entry.nextIt;
                }
                else
                {
                    node.minSSAId = std::min(node.minSSAId, entry.minSSAId);
                    ARDOut << "    Node: " <<
                                entry.nodeIt->first << ":" << &node << " minSSAId: " <<
                                node.minSSAId << "\n";
                    minSSAStack.pop();
                    if (!minSSAStack.empty())
                    {
                        MinSSAGraphStackEntry& pentry = minSSAStack.top();
                        pentry.minSSAId = std::min(pentry.minSSAId, node.minSSAId);
                    }
                }
            }
            
            const size_t minSSAId = ssaGraphNodeIt->second.minSSAId;
            
            // skip visited nodes
            for(; ssaGraphNodeIt != ssaGraphNodes.end(); ++ssaGraphNodeIt)
                if (!ssaGraphNodeIt->second.visited)
                    break;
            // zeroing visited
            for (MinSSAGraphNode* node: toClear)
            {
                node->minSSAId = minSSAId; // fill up by minSSAId
                node->visited = false;
            }
            toClear.clear();
        }
        
        for (const auto& entry: ssaGraphNodes)
            newReplaces.push_back({ entry.first, entry.second.minSSAId });
        
        std::sort(newReplaces.begin(), newReplaces.end());
        entry.second = newReplaces;
    }
    
    /* apply SSA id replaces */
    for (CodeBlock& cblock: codeBlocks)
        for (auto& ssaEntry: cblock.ssaInfoMap)
        {
            auto it = ssaReplacesMap.find(ssaEntry.first);
            if (it == ssaReplacesMap.end())
                continue;
            SSAInfo& sinfo = ssaEntry.second;
            VectorSet<SSAReplace>& replaces = it->second;
            if (sinfo.readBeforeWrite)
            {
                auto rit = binaryMapFind(replaces.begin(), replaces.end(),
                                 ssaEntry.second.ssaIdBefore);
                if (rit != replaces.end())
                    sinfo.ssaIdBefore = rit->second; // replace
            }
            if (sinfo.ssaIdFirst != SIZE_MAX)
            {
                auto rit = binaryMapFind(replaces.begin(), replaces.end(),
                                 ssaEntry.second.ssaIdFirst);
                if (rit != replaces.end())
                    sinfo.ssaIdFirst = rit->second; // replace
            }
            if (sinfo.ssaIdLast != SIZE_MAX)
            {
                auto rit = binaryMapFind(replaces.begin(), replaces.end(),
                                 ssaEntry.second.ssaIdLast);
                if (rit != replaces.end())
                    sinfo.ssaIdLast = rit->second; // replace
            }
        }
    
    // clear ssa replaces
    ssaReplacesMap.clear();
}

void AsmRegAllocator::createInterferenceGraph()
{
    /// construct liveBlockMaps
    std::set<LiveBlock> liveBlockMaps[MAX_REGTYPES_NUM];
    for (size_t regType = 0; regType < regTypesNum; regType++)
    {
        std::set<LiveBlock>& liveBlockMap = liveBlockMaps[regType];
        Array<OutLiveness>& liveness = outLivenesses[regType];
        for (size_t li = 0; li < liveness.size(); li++)
        {
            OutLiveness& lv = liveness[li];
            for (const std::pair<size_t, size_t>& blk: lv)
                if (blk.first != blk.second)
                    liveBlockMap.insert({ blk.first, blk.second, li });
            lv.clear();
        }
        liveness.clear();
    }
    
    // create interference graphs
    for (size_t regType = 0; regType < regTypesNum; regType++)
    {
        InterGraph& interGraph = interGraphs[regType];
        interGraph.resize(graphVregsCounts[regType]);
        std::set<LiveBlock>& liveBlockMap = liveBlockMaps[regType];
        
        auto lit = liveBlockMap.begin();
        size_t rangeStart = 0;
        if (lit != liveBlockMap.end())
            rangeStart = lit->start;
        while (lit != liveBlockMap.end())
        {
            const size_t blkStart = lit->start;
            const size_t blkEnd = lit->end;
            size_t rangeEnd = blkEnd;
            auto liStart = liveBlockMap.lower_bound({ rangeStart, 0, 0 });
            auto liEnd = liveBlockMap.lower_bound({ rangeEnd, 0, 0 });
            // collect from this range, variable indices
            std::set<size_t> varIndices;
            for (auto lit2 = liStart; lit2 != liEnd; ++lit2)
                varIndices.insert(lit2->vidx);
            // push to intergraph as full subgGraph
            for (auto vit = varIndices.begin(); vit != varIndices.end(); ++vit)
                for (auto vit2 = varIndices.begin(); vit2 != varIndices.end(); ++vit2)
                    if (vit != vit2)
                        interGraph[*vit].insert(*vit2);
            // go to next live blocks
            rangeStart = rangeEnd;
            for (; lit != liveBlockMap.end(); ++lit)
                if (lit->start != blkStart && lit->end != blkEnd)
                    break;
            if (lit == liveBlockMap.end())
                break; // 
            rangeStart = std::max(rangeStart, lit->start);
        }
    }
}

/* algorithm to allocate regranges:
 * from smallest regranges to greatest regranges:
 *   choosing free register: from smallest free regranges
 *      to greatest regranges:
 *         in this same regrange: 
 *               try to find free regs in regranges
 *               try to link free ends of two distinct regranges
 */

void AsmRegAllocator::colorInterferenceGraph()
{
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                    assembler.deviceType);
    
    for (size_t regType = 0; regType < regTypesNum; regType++)
    {
        const size_t maxColorsNum = getGPUMaxRegistersNum(arch, regType);
        InterGraph& interGraph = interGraphs[regType];
        const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
        Array<cxuint>& gcMap = graphColorMaps[regType];
        
        const size_t nodesNum = interGraph.size();
        gcMap.resize(nodesNum);
        std::fill(gcMap.begin(), gcMap.end(), cxuint(UINT_MAX));
        Array<size_t> sdoCounts(nodesNum);
        std::fill(sdoCounts.begin(), sdoCounts.end(), 0);
        
        SDOLDOCompare compare(interGraph, sdoCounts);
        std::set<size_t, SDOLDOCompare> nodeSet(compare);
        for (size_t i = 0; i < nodesNum; i++)
            nodeSet.insert(i);
        
        cxuint colorsNum = 0;
        // firstly, allocate real registers
        for (const auto& entry: vregIndexMap)
            if (entry.first.regVar == nullptr)
                gcMap[entry.second[0]] = colorsNum++;
        
        for (size_t colored = 0; colored < nodesNum; colored++)
        {
            size_t node = *nodeSet.begin();
            if (gcMap[node] != UINT_MAX)
                continue; // already colored
            size_t color = 0;
            
            for (color = 0; color <= colorsNum; color++)
            {
                // find first usable color
                bool thisSame = false;
                for (size_t nb: interGraph[node])
                    if (gcMap[nb] == color)
                    {
                        thisSame = true;
                        break;
                    }
                if (!thisSame)
                    break;
            }
            if (color==colorsNum) // add new color if needed
            {
                if (colorsNum >= maxColorsNum)
                    throw AsmException("Too many register is needed");
                colorsNum++;
            }
            
            gcMap[node] = color;
            // update SDO for node
            bool colorExists = false;
            for (size_t nb: interGraph[node])
                if (gcMap[nb] == color)
                {
                    colorExists = true;
                    break;
                }
            if (!colorExists)
                sdoCounts[node]++;
            // update SDO for neighbors
            for (size_t nb: interGraph[node])
            {
                colorExists = false;
                for (size_t nb2: interGraph[nb])
                    if (gcMap[nb2] == color)
                    {
                        colorExists = true;
                        break;
                    }
                if (!colorExists)
                {
                    if (gcMap[nb] == UINT_MAX)
                        nodeSet.erase(nb);  // before update we erase from nodeSet
                    sdoCounts[nb]++;
                    if (gcMap[nb] == UINT_MAX)
                        nodeSet.insert(nb); // after update, insert again
                }
            }
            
            gcMap[node] = color;
        }
    }
}

void AsmRegAllocator::allocateRegisters(AsmSectionId sectionId)
{
    // before any operation, clear all
    codeBlocks.clear();
    for (size_t i = 0; i < MAX_REGTYPES_NUM; i++)
    {
        graphVregsCounts[i] = 0;
        vregIndexMaps[i].clear();
        interGraphs[i].clear();
        linearDepMaps[i].clear();
        graphColorMaps[i].clear();
    }
    ssaReplacesMap.clear();
    cxuint maxRegs[MAX_REGTYPES_NUM];
    assembler.isaAssembler->getMaxRegistersNum(regTypesNum, maxRegs);
    
    // set up
    const AsmSection& section = assembler.sections[sectionId];
    createCodeStructure(section.codeFlow, section.content.size(), section.content.data());
    createSSAData(*section.usageHandler, *section.linearDepHandler);
    applySSAReplaces();
    createLivenesses(*section.usageHandler, *section.linearDepHandler);
    createInterferenceGraph();
    colorInterferenceGraph();
}
