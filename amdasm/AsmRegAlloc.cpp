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

ISAUsageHandler::ISAUsageHandler(const std::vector<cxbyte>& _content) :
            content(_content), lastOffset(0), readOffset(0), instrStructPos(0),
            regUsagesPos(0), regUsages2Pos(0), regVarUsagesPos(0),
            pushedArgs(0), argPos(0), argFlags(0), isNext(false), useRegMode(false)
{ }

ISAUsageHandler::~ISAUsageHandler()
{ }

void ISAUsageHandler::rewind()
{
    readOffset = instrStructPos = 0;
    regUsagesPos = regUsages2Pos = regVarUsagesPos = 0;
    useRegMode = false;
    pushedArgs = 0;
    skipBytesInInstrStruct();
}

void ISAUsageHandler::skipBytesInInstrStruct()
{
    // do not add instruction size if usereg (usereg immediately before instr regusages)
    if ((instrStructPos != 0 || argPos != 0) && !useRegMode)
        readOffset += defaultInstrSize;
    argPos = 0;
    for (;instrStructPos < instrStruct.size() &&
        instrStruct[instrStructPos] > 0x80; instrStructPos++)
        readOffset += (instrStruct[instrStructPos] & 0x7f);
    isNext = (instrStructPos < instrStruct.size());
}

void ISAUsageHandler::putSpace(size_t offset)
{
    if (lastOffset != offset)
    {
        flush(); // flush before new instruction
        // useReg immediately before instruction regusages
        size_t defaultInstrSize = (!useRegMode ? this->defaultInstrSize : 0);
        if (lastOffset > offset)
            throw AsmException("Offset before previous instruction");
        if (!instrStruct.empty() && offset - lastOffset < defaultInstrSize)
            throw AsmException("Offset between previous instruction");
        size_t toSkip = !instrStruct.empty() ? 
                offset - lastOffset - defaultInstrSize : offset;
        while (toSkip > 0)
        {
            size_t skipped = std::min(toSkip, size_t(0x7f));
            instrStruct.push_back(skipped | 0x80);
            toSkip -= skipped;
        }
        lastOffset = offset;
        argFlags = 0;
        pushedArgs = 0;
    } 
}

void ISAUsageHandler::pushUsage(const AsmRegVarUsage& rvu)
{
    if (lastOffset == rvu.offset && useRegMode)
        flush(); // only flush if useRegMode and no change in offset
    else // otherwise
        putSpace(rvu.offset);
    useRegMode = false;
    if (rvu.regVar != nullptr)
    {
        argFlags |= (1U<<pushedArgs);
        regVarUsages.push_back({ rvu.regVar, rvu.rstart, rvu.rend, rvu.regField,
            rvu.rwFlags, rvu.align });
    }
    else // reg usages
        regUsages.push_back({ rvu.regField,cxbyte(rvu.rwFlags |
                    getRwFlags(rvu.regField, rvu.rstart, rvu.rend)) });
    pushedArgs++;
}

void ISAUsageHandler::pushUseRegUsage(const AsmRegVarUsage& rvu)
{
    if (lastOffset == rvu.offset && !useRegMode)
        flush(); // only flush if useRegMode and no change in offset
    else // otherwise
        putSpace(rvu.offset);
    useRegMode = true;
    if (pushedArgs == 0 || pushedArgs == 256)
    {
        argFlags = 0;
        pushedArgs = 0;
        instrStruct.push_back(0x80); // sign of regvarusage from usereg
        instrStruct.push_back(0);
    }
    if (rvu.regVar != nullptr)
    {
        argFlags |= (1U<<(pushedArgs & 7));
        regVarUsages.push_back({ rvu.regVar, rvu.rstart, rvu.rend, rvu.regField,
            rvu.rwFlags, rvu.align });
    }
    else // reg usages
        regUsages2.push_back({ rvu.rstart, rvu.rend, rvu.rwFlags });
    pushedArgs++;
    if ((pushedArgs & 7) == 0) // just flush per 8 bit
    {
        instrStruct.push_back(argFlags);
        instrStruct[instrStruct.size() - ((pushedArgs+7) >> 3) - 1] = pushedArgs;
        argFlags = 0;
    }
}

void ISAUsageHandler::flush()
{
    if (pushedArgs != 0)
    {
        if (!useRegMode)
        {
            // normal regvarusages
            instrStruct.push_back(argFlags);
            if ((argFlags & (1U<<(pushedArgs-1))) != 0)
                regVarUsages.back().rwFlags |= 0x80;
            else // reg usages
                regUsages.back().rwFlags |= 0x80;
        }
        else
        {
            // use reg regvarusages
            if ((pushedArgs & 7) != 0) //if only not pushed args remains
                instrStruct.push_back(argFlags);
            instrStruct[instrStruct.size() - ((pushedArgs+7) >> 3) - 1] = pushedArgs;
        }
    }
}

AsmRegVarUsage ISAUsageHandler::nextUsage()
{
    if (!isNext)
        throw AsmException("No reg usage in this code");
    AsmRegVarUsage rvu;
    // get regvarusage
    bool lastRegUsage = false;
    rvu.offset = readOffset;
    if (!useRegMode && instrStruct[instrStructPos] == 0x80)
    {
        // useRegMode (begin fetching useregs)
        useRegMode = true;
        argPos = 0;
        instrStructPos++;
        // pushedArgs - numer of useregs, 0 - 256 useregs
        pushedArgs = instrStruct[instrStructPos++];
        argFlags = instrStruct[instrStructPos];
    }
    rvu.useRegMode = useRegMode; // no ArgPos
    
    if ((instrStruct[instrStructPos] & (1U << (argPos&7))) != 0)
    {
        // regvar usage
        const AsmRegVarUsageInt& inRVU = regVarUsages[regVarUsagesPos++];
        rvu.regVar = inRVU.regVar;
        rvu.rstart = inRVU.rstart;
        rvu.rend = inRVU.rend;
        rvu.regField = inRVU.regField;
        rvu.rwFlags = inRVU.rwFlags & ASMRVU_ACCESS_MASK;
        rvu.align = inRVU.align;
        if (!useRegMode)
            lastRegUsage = ((inRVU.rwFlags&0x80) != 0);
    }
    else if (!useRegMode)
    {
        // simple reg usage
        const AsmRegUsageInt& inRU = regUsages[regUsagesPos++];
        rvu.regVar = nullptr;
        const std::pair<uint16_t, uint16_t> regPair =
                    getRegPair(inRU.regField, inRU.rwFlags);
        rvu.rstart = regPair.first;
        rvu.rend = regPair.second;
        rvu.rwFlags = (inRU.rwFlags & ASMRVU_ACCESS_MASK);
        rvu.regField = inRU.regField;
        rvu.align = 0;
        lastRegUsage = ((inRU.rwFlags&0x80) != 0);
    }
    else
    {
        // use reg (simple reg usage, second structure)
        const AsmRegUsage2Int& inRU = regUsages2[regUsages2Pos++];
        rvu.regVar = nullptr;
        rvu.rstart = inRU.rstart;
        rvu.rend = inRU.rend;
        rvu.rwFlags = inRU.rwFlags;
        rvu.regField = ASMFIELD_NONE;
        rvu.align = 0;
    }
    argPos++;
    if (useRegMode)
    {
        // if inside useregs
        if (argPos == (pushedArgs&0xff))
        {
            instrStructPos++; // end
            skipBytesInInstrStruct();
            useRegMode = false;
        }
        else if ((argPos & 7) == 0) // fetch new flag
        {
            instrStructPos++;
            argFlags = instrStruct[instrStructPos];
        }
    }
    // after instr
    if (lastRegUsage)
    {
        instrStructPos++;
        skipBytesInInstrStruct();
    }
    return rvu;
}

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

static cxuint getRegType(size_t regTypesNum, const cxuint* regRanges,
            const AsmSingleVReg& svreg)
{
    cxuint regType; // regtype
    if (svreg.regVar!=nullptr)
        regType = svreg.regVar->type;
    else
        for (regType = 0; regType < regTypesNum; regType++)
            if (svreg.index >= regRanges[regType<<1] &&
                svreg.index < regRanges[(regType<<1)+1])
                break;
    return regType;
}

static Liveness& getLiveness(const AsmSingleVReg& svreg, size_t ssaIdIdx,
        const AsmRegAllocator::SSAInfo& ssaInfo, std::vector<Liveness>* livenesses,
        const VarIndexMap* vregIndexMaps, size_t regTypesNum, const cxuint* regRanges)
{
    size_t ssaId;
    if (svreg.regVar==nullptr)
        ssaId = 0;
    else if (ssaIdIdx==0)
        ssaId = ssaInfo.ssaIdBefore;
    else if (ssaIdIdx==1)
        ssaId = ssaInfo.ssaIdFirst;
    else if (ssaIdIdx<ssaInfo.ssaIdChange)
        ssaId = ssaInfo.ssaId + ssaIdIdx-1;
    else // last
        ssaId = ssaInfo.ssaIdLast;
    
    cxuint regType = getRegType(regTypesNum, regRanges, svreg); // regtype
    const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
    const std::vector<size_t>& ssaIdIndices = vregIndexMap.find(svreg)->second;
    ARDOut << "lvn[" << regType << "][" << ssaIdIndices[ssaId] << "]. ssaIdIdx: " <<
            ssaIdIdx << ". ssaId: " << ssaId << ". svreg: " << svreg.regVar << ":" <<
            svreg.index << "\n";
    return livenesses[regType][ssaIdIndices[ssaId]];
}

static Liveness& getLiveness2(const AsmSingleVReg& svreg,
        size_t ssaId, std::vector<Liveness>* livenesses,
        const VarIndexMap* vregIndexMaps, size_t regTypesNum, const cxuint* regRanges)
{
    cxuint regType = getRegType(regTypesNum, regRanges, svreg); // regtype
    const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
    const std::vector<size_t>& ssaIdIndices = vregIndexMap.find(svreg)->second;
    ARDOut << "lvn[" << regType << "][" << ssaIdIndices[ssaId] << "]. ssaId: " <<
            ssaId << ". svreg: " << svreg.regVar << ":" << svreg.index << "\n";
    return livenesses[regType][ssaIdIndices[ssaId]];
}


static void applyToLiveCallTime(size_t block, Liveness& lv,
            const std::vector<CodeBlock>& codeBlocks, const RoutineLvMap& routineMap,
            const std::unordered_map<size_t, size_t>& callLiveTimesMap)
{
    auto cit = callLiveTimesMap.find(block);
    if (cit != callLiveTimesMap.end())
    {
        size_t callLiveTime = cit->second;
        const CodeBlock& cblock = codeBlocks[block];
        for (const NextBlock& next: cblock.nexts)
            if (next.isCall)
            {
                const auto& allLvs= routineMap.find(next.block)->second.allLivenesses;
                if (allLvs.find(&lv) == allLvs.end())
                    // add callLiveTime only if vreg not present in routine
                    lv.insert(callLiveTime, callLiveTime+1);
                callLiveTime--;
            }
    }
}

static void fillUpInsideRoutine(std::vector<bool>& visited,
            const std::vector<CodeBlock>& codeBlocks, const RoutineLvMap& routineMap,
            const std::unordered_map<size_t, size_t>& callLiveTimesMap,
            size_t startBlock, const AsmSingleVReg& svreg, Liveness& lv)
{
    std::deque<FlowStackEntry3> flowStack;
    flowStack.push_back({ startBlock, 0 });
    
    if (lv.contain(codeBlocks[startBlock].end-1))
        // already filled, then do nothing
        return;
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                size_t cbStart = cblock.start;
                if (flowStack.size() == 1)
                {
                    // if first block, then get last occurrence in this path
                    auto sinfoIt = cblock.ssaInfoMap.find(svreg);
                    if (sinfoIt != cblock.ssaInfoMap.end())
                        cbStart = sinfoIt->second.lastPos+1;
                }
                // just insert
                lv.insert(cbStart, cblock.end);
                
                applyToLiveCallTime(entry.blockIndex, lv, codeBlocks, routineMap,
                        callLiveTimesMap);
            }
            else
            {
                flowStack.pop_back();
                continue;
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
            flowStack.pop_back();
    }
}

static void joinVRegRecur(const std::deque<FlowStackEntry3>& flowStack,
            const std::vector<CodeBlock>& codeBlocks, const RoutineLvMap& routineMap,
            const std::unordered_map<size_t, size_t>& callLiveTimesMap,
            LastVRegStackPos flowStkStart, const AsmSingleVReg& svreg, size_t ssaId,
            const VarIndexMap* vregIndexMaps, std::vector<Liveness>* livenesses,
            size_t regTypesNum, const cxuint* regRanges, bool skipLastBlock = false)
{
    struct JoinEntry
    {
        size_t blockIndex; // block index where is call
        size_t nextIndex; // next index of routine call
        size_t lastAccessIndex; // last access pos index
        bool inSubroutines;
    };
    
    FlowStackCIter flitEnd = flowStack.end();
    if (skipLastBlock)
        --flitEnd; // before last element
    Liveness& lv = getLiveness2(svreg, ssaId, livenesses, vregIndexMaps,
                    regTypesNum, regRanges);
    
    if (flitEnd != flowStack.begin())
    {
        const CodeBlock& cbLast = codeBlocks[(flitEnd-1)->blockIndex];
        if (lv.contain(cbLast.end-1))
            // if already filled up
            return;
    }
    
    std::vector<bool> visited(codeBlocks.size(), false);
    
    std::stack<JoinEntry> rjStack; // routine join stack
    if (flowStkStart.inSubroutines)
        rjStack.push({ flowStack[flowStkStart.stackPos].blockIndex, 0, 0, true });
    
    while (!rjStack.empty())
    {
        JoinEntry& entry = rjStack.top();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.inSubroutines && entry.nextIndex < cblock.nexts.size())
        {
            bool doNextIndex = false; // true if to next call
            if (cblock.nexts[entry.nextIndex].isCall)
            {
                const RoutineDataLv& rdata =
                        routineMap.find(cblock.nexts[entry.nextIndex].block)->second;
                auto lastAccessIt = rdata.lastAccessMap.find(svreg);
                if (lastAccessIt != rdata.lastAccessMap.end())
                {
                    if (entry.lastAccessIndex < lastAccessIt->second.size())
                    {
                        const auto& lastAccess =
                                lastAccessIt->second[entry.lastAccessIndex];
                        rjStack.push({ lastAccess.blockIndex, 0,
                                    0, lastAccess.inSubroutines });
                        entry.lastAccessIndex++;
                    }
                    else
                        doNextIndex = true;
                }
                else
                    doNextIndex = true;
            }
            else
                doNextIndex = true;
            
            if (doNextIndex)
            {
                // to next call
                entry.nextIndex++;
                entry.lastAccessIndex = 0;
            }
        }
        else
        {
            size_t blockStart = entry.blockIndex;
            if (!entry.inSubroutines)
                fillUpInsideRoutine(visited, codeBlocks, routineMap, callLiveTimesMap,
                            blockStart, svreg, lv);
            else
                // fill up next block in path (do not fill start block)
                fillUpInsideRoutine(visited, codeBlocks, routineMap, callLiveTimesMap,
                        blockStart+1, svreg, lv);
            rjStack.pop();
        }
    }
    
    auto flit = flowStack.begin() + flowStkStart.stackPos + (flowStkStart.inSubroutines);
    const CodeBlock& lastBlk = codeBlocks[flit->blockIndex];
    
    applyToLiveCallTime(flit->blockIndex, lv, codeBlocks, routineMap, callLiveTimesMap);
    
    auto sinfoIt = lastBlk.ssaInfoMap.find(svreg);
    size_t lastPos = lastBlk.start;
    if (sinfoIt != lastBlk.ssaInfoMap.end())
    {
        // if begin at some point at last block
        lastPos = sinfoIt->second.lastPos;
        lv.insert(lastPos + 1, lastBlk.end);
        ++flit; // skip last block in stack
    }
    // fill up to this
    for (; flit != flitEnd; ++flit)
    {
        const CodeBlock& cblock = codeBlocks[flit->blockIndex];
        applyToLiveCallTime(flit->blockIndex, lv, codeBlocks,
                        routineMap, callLiveTimesMap);
        lv.insert(cblock.start, cblock.end);
    }
}

/* handle many start points in this code (for example many kernel's in same code)
 * replace sets by vector, and sort and remove same values on demand
 */

/* join livenesses between consecutive code blocks */
static void putCrossBlockLivenesses(const std::deque<FlowStackEntry3>& flowStack,
        const std::vector<CodeBlock>& codeBlocks, const RoutineLvMap& routineMap,
        const std::unordered_map<size_t, size_t>& callLiveTimesMap,
        const LastVRegMap& lastVRegMap, std::vector<Liveness>* livenesses,
        const VarIndexMap* vregIndexMaps, size_t regTypesNum, const cxuint* regRanges)
{
    ARDOut << "putCrossBlockLv block: " << flowStack.back().blockIndex << "\n";
    const CodeBlock& cblock = codeBlocks[flowStack.back().blockIndex];
    for (const auto& entry: cblock.ssaInfoMap)
        if (entry.second.readBeforeWrite)
        {
            // find last
            auto lvrit = lastVRegMap.find(entry.first);
            LastVRegStackPos flowStackStart = (lvrit != lastVRegMap.end()) ?
                lvrit->second.back() : LastVRegStackPos{ 0, false };
            
            joinVRegRecur(flowStack, codeBlocks, routineMap, callLiveTimesMap,
                    flowStackStart, entry.first, entry.second.ssaIdBefore,
                    vregIndexMaps, livenesses, regTypesNum, regRanges, true);
        }
}

// add new join second cache entry with readBeforeWrite for all encountered regvars
static void addJoinSecCacheEntry(const RoutineLvMap& routineMap,
                const std::vector<CodeBlock>& codeBlocks,
                SimpleCache<size_t, SVRegMap>& joinSecondPointsCache,
                size_t nextBlock)
{
    ARDOut << "addJoinSecCacheEntry: " << nextBlock << "\n";
    //std::stack<CallStackEntry> callStack = prevCallStack;
    // traverse by graph from next block
    std::deque<FlowStackEntry3> flowStack;
    flowStack.push_back({ nextBlock, 0 });
    CBlockBitPool visited(codeBlocks.size(), false);
    
    SVRegBlockMap alreadyReadMap;
    SVRegMap cacheSecPoints;
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                ARDOut << "  resolv (cache): " << entry.blockIndex << "\n";
                
                const SVRegMap* resSecondPoints =
                            joinSecondPointsCache.use(entry.blockIndex);
                if (resSecondPoints == nullptr)
                {
                    // if joinSecondPointCache not found
                    for (auto& sentry: cblock.ssaInfoMap)
                    {
                        const SSAInfo& sinfo = sentry.second;
                        auto res = alreadyReadMap.insert(
                                    { sentry.first, entry.blockIndex });
                        
                        if (res.second && sinfo.readBeforeWrite)
                            cacheSecPoints[sentry.first] = sinfo.ssaIdBefore;
                    }
                }
                else // to use cache
                {
                    // add to current cache sec points
                    for (const auto& rsentry: *resSecondPoints)
                    {
                        const bool alreadyRead =
                            alreadyReadMap.find(rsentry.first) != alreadyReadMap.end();
                        if (!alreadyRead)
                            cacheSecPoints[rsentry.first] = rsentry.second;
                    }
                    flowStack.pop_back();
                    continue;
                }
            }
            else
            {
                // back, already visited
                ARDOut << "join already (cache): " << entry.blockIndex << "\n";
                flowStack.pop_back();
                continue;
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            // add toResolveMap ssaIds inside called routines
            for (const auto& next: cblock.nexts)
                if (next.isCall)
                {
                    const RoutineDataLv& rdata = routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                        alreadyReadMap.insert({v.first, entry.blockIndex });
                    for (const auto& v: rdata.lastAccessMap)
                        alreadyReadMap.insert({v.first, entry.blockIndex });
                }
            
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
            // remove old to resolve in leaved way to allow collecting next ssaId
            // before write (can be different due to earlier visit)
            for (const auto& next: cblock.nexts)
                if (next.isCall)
                {
                    const RoutineDataLv& rdata = routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                    {
                        auto it = alreadyReadMap.find(v.first);
                        if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                            alreadyReadMap.erase(it);
                    }
                    for (const auto& v: rdata.lastAccessMap)
                    {
                        auto it = alreadyReadMap.find(v.first);
                        if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                            alreadyReadMap.erase(it);
                    }
                }
            
            for (const auto& sentry: cblock.ssaInfoMap)
            {
                auto it = alreadyReadMap.find(sentry.first);
                if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                    // remove old to resolve in leaved way to allow collecting next ssaId
                    // before write (can be different due to earlier visit)
                    alreadyReadMap.erase(it);
            }
            ARDOut << "  popjoin (cache)\n";
            flowStack.pop_back();
        }
    }
    
    joinSecondPointsCache.put(nextBlock, cacheSecPoints);
}

// apply calls (changes from these calls) from code blocks to stack var map
static void applyCallToStackVarMap(const CodeBlock& cblock, const RoutineLvMap& routineMap,
        LastStackPosMap& stackVarMap, size_t pfPos, size_t nextIndex)
{
    for (const NextBlock& next: cblock.nexts)
        if (next.isCall)
        {
            ARDOut << "  japplycall: " << pfPos << ": " <<
                    nextIndex << ": " << next.block << "\n";
            const LastAccessMap& regVarMap =
                    routineMap.find(next.block)->second.lastAccessMap;
            for (const auto& sentry: regVarMap)
                stackVarMap[sentry.first] = LastVRegStackPos{ pfPos, true };
        }
}

static void joinRegVarLivenesses(const std::deque<FlowStackEntry3>& prevFlowStack,
        const std::vector<CodeBlock>& codeBlocks, const RoutineLvMap& routineMap,
        const std::unordered_map<size_t, size_t>& callLiveTimesMap,
        const PrevWaysIndexMap& prevWaysIndexMap,
        const std::vector<bool>& waysToCache, ResSecondPointsToCache& cblocksToCache,
        SimpleCache<size_t, LastStackPosMap>& joinFirstPointsCache,
        SimpleCache<size_t, SVRegMap>& joinSecondPointsCache,
        const VarIndexMap* vregIndexMaps,
        std::vector<Liveness>* livenesses, size_t regTypesNum, const cxuint* regRanges)
{
    size_t nextBlock = prevFlowStack.back().blockIndex;
    auto pfEnd = prevFlowStack.end();
    --pfEnd;
    ARDOut << "startJoinLv: " << (pfEnd-1)->blockIndex << "," << nextBlock << "\n";
    // key - varreg, value - last position in previous flowStack
    LastStackPosMap stackVarMap;
    
    size_t pfStartIndex = 0;
    {
        auto pfPrev = pfEnd;
        --pfPrev;
        auto it = prevWaysIndexMap.find(pfPrev->blockIndex);
        if (it != prevWaysIndexMap.end())
        {
            const LastStackPosMap* cached = joinFirstPointsCache.use(it->second.first);
            if (cached!=nullptr)
            {
                ARDOut << "use pfcached: " << it->second.first << ", " <<
                        it->second.second << "\n";
                stackVarMap = *cached;
                pfStartIndex = it->second.second+1;
                
                // apply missing calls at end of the cached
                const CodeBlock& cblock = codeBlocks[it->second.first];
                
                const FlowStackEntry3& entry = *(prevFlowStack.begin()+pfStartIndex-1);
                if (entry.nextIndex > cblock.nexts.size())
                    applyCallToStackVarMap(cblock, routineMap, stackVarMap,
                                    pfStartIndex-1, -1);
            }
        }
    }
    
    for (auto pfit = prevFlowStack.begin()+pfStartIndex; pfit != pfEnd; ++pfit)
    {
        const FlowStackEntry3& entry = *pfit;
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        for (const auto& sentry: cblock.ssaInfoMap)
            stackVarMap[sentry.first] = { size_t(pfit - prevFlowStack.begin()), false };
        
        if (entry.nextIndex > cblock.nexts.size())
            applyCallToStackVarMap(cblock, routineMap, stackVarMap,
                        pfit - prevFlowStack.begin(), entry.nextIndex);
        
        // put to first point cache
        if (waysToCache[pfit->blockIndex] &&
            !joinFirstPointsCache.hasKey(pfit->blockIndex))
        {
            ARDOut << "put pfcache " << pfit->blockIndex << "\n";
            joinFirstPointsCache.put(pfit->blockIndex, stackVarMap);
        }
    }
    
    SVRegMap cacheSecPoints;
    const bool toCache = (!joinSecondPointsCache.hasKey(nextBlock)) &&
                cblocksToCache.count(nextBlock)>=2;
    
    // traverse by graph from next block
    std::deque<FlowStackEntry3> flowStack;
    flowStack.push_back({ nextBlock, 0 });
    std::vector<bool> visited(codeBlocks.size(), false);
    
    // already read in current path
    // key - vreg, value - source block where vreg of conflict found
    SVRegMap alreadyReadMap;
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                ARDOut << "  lvjoin: " << entry.blockIndex << "\n";
                
                const SVRegMap* joinSecondPoints =
                        joinSecondPointsCache.use(entry.blockIndex);
                
                if (joinSecondPoints == nullptr)
                    for (const auto& sentry: cblock.ssaInfoMap)
                    {
                        const SSAInfo& sinfo = sentry.second;
                        auto res = alreadyReadMap.insert(
                                    { sentry.first, entry.blockIndex });
                        
                        if (toCache)
                            cacheSecPoints[sentry.first] = sinfo.ssaIdBefore;
                        
                        if (res.second && sinfo.readBeforeWrite)
                        {
                            auto it = stackVarMap.find(sentry.first);
                            LastVRegStackPos stackPos = (it != stackVarMap.end() ?
                                        it->second : LastVRegStackPos{ 0, false });
                            
                            joinVRegRecur(prevFlowStack, codeBlocks, routineMap,
                                callLiveTimesMap, stackPos,
                                sentry.first, sentry.second.ssaIdBefore, vregIndexMaps,
                                livenesses, regTypesNum, regRanges, true);
                        }
                    }
                else
                {
                    ARDOut << "use join secPointCache: " << entry.blockIndex << "\n";
                    // add to current cache sec points
                    for (const auto& rsentry: *joinSecondPoints)
                    {
                        const bool alreadyRead =
                            alreadyReadMap.find(rsentry.first) != alreadyReadMap.end();
                        
                        if (!alreadyRead)
                        {
                            if (toCache)
                                cacheSecPoints[rsentry.first] = rsentry.second;
                            
                            auto it = stackVarMap.find(rsentry.first);
                            LastVRegStackPos stackPos = (it != stackVarMap.end() ?
                                        it->second : LastVRegStackPos{ 0, false });
                            
                            joinVRegRecur(prevFlowStack, codeBlocks, routineMap,
                                callLiveTimesMap, stackPos,
                                rsentry.first, rsentry.second, vregIndexMaps,
                                livenesses, regTypesNum, regRanges, true);
                        }
                    }
                    flowStack.pop_back();
                    continue;
                }
            }
            else
            {
                cblocksToCache.increase(entry.blockIndex);
                ARDOut << "jcblockToCache: " << entry.blockIndex << "=" <<
                            cblocksToCache.count(entry.blockIndex) << "\n";
                // back, already visited
                ARDOut << "join already: " << entry.blockIndex << "\n";
                flowStack.pop_back();
                continue;
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            // add toResolveMap ssaIds inside called routines
            for (const auto& next: cblock.nexts)
                if (next.isCall)
                {
                    const RoutineDataLv& rdata = routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                        alreadyReadMap.insert({v.first, entry.blockIndex });
                    for (const auto& v: rdata.lastAccessMap)
                        alreadyReadMap.insert({v.first, entry.blockIndex });
                }
            
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
            // remove old to resolve in leaved way to allow collecting next ssaId
            // before write (can be different due to earlier visit)
            for (const auto& next: cblock.nexts)
                if (next.isCall)
                {
                    const RoutineDataLv& rdata = routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                    {
                        auto it = alreadyReadMap.find(v.first);
                        if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                            alreadyReadMap.erase(it);
                    }
                    for (const auto& v: rdata.lastAccessMap)
                    {
                        auto it = alreadyReadMap.find(v.first);
                        if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                            alreadyReadMap.erase(it);
                    }
                }
            
            for (const auto& sentry: cblock.ssaInfoMap)
            {
                auto it = alreadyReadMap.find(sentry.first);
                if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                    // remove old to resolve in leaved way to allow collecting next ssaId
                    // before write (can be different due to earlier visit)
                    alreadyReadMap.erase(it);
            }
            ARDOut << "  popjoin\n";
            
            if (cblocksToCache.count(entry.blockIndex)==2 &&
                !joinSecondPointsCache.hasKey(entry.blockIndex))
                // add to cache
                addJoinSecCacheEntry(routineMap, codeBlocks, joinSecondPointsCache,
                            entry.blockIndex);
            
            flowStack.pop_back();
        }
    }
    
    if (toCache)
        joinSecondPointsCache.put(nextBlock, cacheSecPoints);
}

static void addUsageDeps(const cxbyte* ldeps, cxuint rvusNum,
            const AsmRegVarUsage* rvus, LinearDepMap* ldepsOut,
            const VarIndexMap* vregIndexMaps, const SVRegMap& ssaIdIdxMap,
            size_t regTypesNum, const cxuint* regRanges)
{
    // add linear deps
    cxuint count = ldeps[0];
    cxuint pos = 1;
    cxbyte rvuAdded = 0;
    for (cxuint i = 0; i < count; i++)
    {
        cxuint ccount = ldeps[pos++];
        std::vector<size_t> vidxes;
        cxuint regType = UINT_MAX;
        cxbyte align = rvus[ldeps[pos]].align;
        for (cxuint j = 0; j < ccount; j++)
        {
            rvuAdded |= 1U<<ldeps[pos];
            const AsmRegVarUsage& rvu = rvus[ldeps[pos++]];
            for (uint16_t k = rvu.rstart; k < rvu.rend; k++)
            {
                AsmSingleVReg svreg = {rvu.regVar, k};
                auto sit = ssaIdIdxMap.find(svreg);
                if (regType==UINT_MAX)
                    regType = getRegType(regTypesNum, regRanges, svreg);
                const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
                const std::vector<size_t>& ssaIdIndices =
                            vregIndexMap.find(svreg)->second;
                // push variable index
                vidxes.push_back(ssaIdIndices[sit->second]);
            }
        }
        ldepsOut[regType][vidxes[0]].align = align;
        for (size_t k = 1; k < vidxes.size(); k++)
        {
            ldepsOut[regType][vidxes[k-1]].nextVidxes.insertValue(vidxes[k]);
            ldepsOut[regType][vidxes[k]].prevVidxes.insertValue(vidxes[k-1]);
        }
    }
    // add single arg linear dependencies
    for (cxuint i = 0; i < rvusNum; i++)
        if ((rvuAdded & (1U<<i)) == 0 && rvus[i].rstart+1<rvus[i].rend)
        {
            const AsmRegVarUsage& rvu = rvus[i];
            std::vector<size_t> vidxes;
            cxuint regType = UINT_MAX;
            cxbyte align = rvus[i].align;
            for (uint16_t k = rvu.rstart; k < rvu.rend; k++)
            {
                AsmSingleVReg svreg = {rvu.regVar, k};
                auto sit = ssaIdIdxMap.find(svreg);
                assert(sit != ssaIdIdxMap.end());
                if (regType==UINT_MAX)
                    regType = getRegType(regTypesNum, regRanges, svreg);
                const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
                const std::vector<size_t>& ssaIdIndices =
                            vregIndexMap.find(svreg)->second;
                // push variable index
                vidxes.push_back(ssaIdIndices[sit->second]);
            }
            ldepsOut[regType][vidxes[0]].align = align;
            for (size_t j = 1; j < vidxes.size(); j++)
            {
                ldepsOut[regType][vidxes[j-1]].nextVidxes.insertValue(vidxes[j]);
                ldepsOut[regType][vidxes[j]].prevVidxes.insertValue(vidxes[j-1]);
            }
        }
}

static void joinRoutineDataLv(RoutineDataLv& dest, RoutineCurAccessMap& curSVRegMap,
            FlowStackEntry4& entry, const RoutineDataLv& src)
{
    dest.rbwSSAIdMap.insert(src.rbwSSAIdMap.begin(), src.rbwSSAIdMap.end());
    dest.allLivenesses.insert(src.allLivenesses.begin(), src.allLivenesses.end());
    
    // join source lastAccessMap with curSVRegMap
    for (const auto& sentry: src.lastAccessMap)
    {
        auto res = curSVRegMap.insert({ sentry.first, { entry.blockIndex, true } });
        if (!res.second)
        {   // not added because is present in map
            if (res.first->second.blockIndex != entry.blockIndex)
                entry.prevCurSVRegMap.insert(*res.first);
            // otherwise, it is same code block but inside routines
            // and do not change prevCurSVRegMap for revert
            // update entry
            res.first->second = { entry.blockIndex, true };
        }
        else
            entry.prevCurSVRegMap.insert({ sentry.first, { SIZE_MAX, true } });
    }
}

static void createRoutineDataLv(const std::vector<CodeBlock>& codeBlocks,
        const RoutineLvMap& routineMap, RoutineDataLv& rdata,
        size_t routineBlock, const VarIndexMap* vregIndexMaps,
        std::vector<Liveness>* livenesses, size_t regTypesNum, const cxuint* regRanges)
{
    std::deque<FlowStackEntry4> flowStack;
    std::vector<bool> visited(codeBlocks.size(), false);
    
    // already read in current path
    // key - vreg, value - source block where vreg of conflict found
    SVRegMap alreadyReadMap;
    
    flowStack.push_back({ routineBlock, 0 });
    RoutineCurAccessMap curSVRegMap; // key - svreg, value - block index
    
    while (!flowStack.empty())
    {
        FlowStackEntry4& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                
                for (const auto& sentry: cblock.ssaInfoMap)
                {
                    if (sentry.second.readBeforeWrite)
                        if (alreadyReadMap.insert(
                                    { sentry.first, entry.blockIndex }).second)
                            rdata.rbwSSAIdMap.insert({ sentry.first,
                                        sentry.second.ssaIdBefore });
                    
                    auto res = curSVRegMap.insert({ sentry.first,
                        LastAccessBlockPos{ entry.blockIndex, false } });
                    if (!res.second)
                    {   // not added because is present in map
                        entry.prevCurSVRegMap.insert(*res.first);
                        res.first->second = { entry.blockIndex, false };
                    }
                    else
                        entry.prevCurSVRegMap.insert(
                                        { sentry.first, { SIZE_MAX, false } });
                    
                    // all SSAs
                    const SSAInfo& sinfo = sentry.second;
                    const AsmSingleVReg& svreg = sentry.first;
                    cxuint regType = getRegType(regTypesNum, regRanges, svreg);
                    const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
                    const std::vector<size_t>& ssaIdIndices =
                                vregIndexMap.find(svreg)->second;
                    
                    // add SSA indices to allSSAs
                    if (sinfo.readBeforeWrite)
                        rdata.allLivenesses.insert(livenesses[regType].data() +
                                    ssaIdIndices[sinfo.ssaIdBefore]);
                    if (sinfo.ssaIdChange != 0)
                    {
                        rdata.allLivenesses.insert(livenesses[regType].data() +
                                    ssaIdIndices[sinfo.ssaIdFirst]);
                        for (size_t i = 1; i < sinfo.ssaIdChange; i++)
                            rdata.allLivenesses.insert(livenesses[regType].data() +
                                        ssaIdIndices[sinfo.ssaId+i]);
                        rdata.allLivenesses.insert(livenesses[regType].data() + 
                                    ssaIdIndices[sinfo.ssaIdLast]);
                    }
                }
            }
            else
            {
                flowStack.pop_back();
                continue;
            }
        }
        
        // join and skip calls
        {
            std::vector<size_t> calledRoutines;
            for (; entry.nextIndex < cblock.nexts.size() &&
                        cblock.nexts[entry.nextIndex].isCall; entry.nextIndex++)
            {
                size_t rblock = cblock.nexts[entry.nextIndex].block;
                if (rblock != routineBlock)
                    calledRoutines.push_back(rblock);
            }
            
            for (size_t srcRoutBlock: calledRoutines)
                joinRoutineDataLv(rdata, curSVRegMap, entry,
                        routineMap.find(srcRoutBlock)->second);
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else
        {
            if (cblock.haveReturn)
            {
                // handle return
                // add curSVReg access positions to lastAccessMap
                for (const auto& entry: curSVRegMap)
                {
                    auto res = rdata.lastAccessMap.insert({ entry.first,
                                    { entry.second } });
                    if (!res.second)
                        res.first->second.insertValue(entry.second);
                }
            }
            // revert curSVRegMap
            for (const auto& sentry: entry.prevCurSVRegMap)
                if (sentry.second.blockIndex != SIZE_MAX)
                    curSVRegMap.find(sentry.first)->second = sentry.second;
                else // no before that
                    curSVRegMap.erase(sentry.first);
            
            for (const auto& sentry: cblock.ssaInfoMap)
            {
                auto it = alreadyReadMap.find(sentry.first);
                if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                    // remove old to resolve in leaved way to allow collecting next ssaId
                    // before write (can be different due to earlier visit)
                    alreadyReadMap.erase(it);
            }
            ARDOut << "  popjoin\n";
            flowStack.pop_back();
        }
    }
}

static inline void revertLastSVReg(LastVRegMap& lastVRegMap, const AsmSingleVReg& svreg)
{
    auto lvrit = lastVRegMap.find(svreg);
    if (lvrit != lastVRegMap.end())
    {
        std::vector<LastVRegStackPos>& lastPos = lvrit->second;
        lastPos.pop_back();
        if (lastPos.empty()) // just remove from lastVRegs
            lastVRegMap.erase(lvrit);
    }
}

/* TODO: handling livenesses between routine call:
 *   for any routine call in call point:
 *     add extra liveness point which will be added to liveness of the vars used between
 *     call point and to liveness of the vars used inside routine
 */

void AsmRegAllocator::createLivenesses(ISAUsageHandler& usageHandler)
{
    ARDOut << "----- createLivenesses ------\n";
    // construct var index maps
    cxuint regRanges[MAX_REGTYPES_NUM*2];
    std::fill(graphVregsCounts, graphVregsCounts+MAX_REGTYPES_NUM, size_t(0));
    size_t regTypesNum;
    assembler.isaAssembler->getRegisterRanges(regTypesNum, regRanges);
    
    for (const CodeBlock& cblock: codeBlocks)
        for (const auto& entry: cblock.ssaInfoMap)
        {
            const SSAInfo& sinfo = entry.second;
            cxuint regType = getRegType(regTypesNum, regRanges, entry.first);
            VarIndexMap& vregIndices = vregIndexMaps[regType];
            size_t& graphVregsCount = graphVregsCounts[regType];
            std::vector<size_t>& ssaIdIndices = vregIndices[entry.first];
            size_t ssaIdCount = 0;
            if (sinfo.readBeforeWrite)
                ssaIdCount = sinfo.ssaIdBefore+1;
            if (sinfo.ssaIdChange!=0)
            {
                ssaIdCount = std::max(ssaIdCount, sinfo.ssaIdLast+1);
                ssaIdCount = std::max(ssaIdCount, sinfo.ssaId+sinfo.ssaIdChange-1);
                ssaIdCount = std::max(ssaIdCount, sinfo.ssaIdFirst+1);
            }
            // if not readBeforeWrite and neither ssaIdChanges but it is write to
            // normal register
            if (entry.first.regVar==nullptr)
                ssaIdCount = 1;
            
            if (ssaIdIndices.size() < ssaIdCount)
                ssaIdIndices.resize(ssaIdCount, SIZE_MAX);
            
            // set liveness index to ssaIdIndices
            if (sinfo.readBeforeWrite)
            {
                if (ssaIdIndices[sinfo.ssaIdBefore] == SIZE_MAX)
                    ssaIdIndices[sinfo.ssaIdBefore] = graphVregsCount++;
            }
            if (sinfo.ssaIdChange!=0)
            {
                // fill up ssaIdIndices (with graph Ids)
                if (ssaIdIndices[sinfo.ssaIdFirst] == SIZE_MAX)
                    ssaIdIndices[sinfo.ssaIdFirst] = graphVregsCount++;
                for (size_t ssaId = sinfo.ssaId+1;
                        ssaId < sinfo.ssaId+sinfo.ssaIdChange-1; ssaId++)
                    ssaIdIndices[ssaId] = graphVregsCount++;
                if (ssaIdIndices[sinfo.ssaIdLast] == SIZE_MAX)
                    ssaIdIndices[sinfo.ssaIdLast] = graphVregsCount++;
            }
            // if not readBeforeWrite and neither ssaIdChanges but it is write to
            // normal register
            if (entry.first.regVar==nullptr && ssaIdIndices[0] == SIZE_MAX)
                ssaIdIndices[0] = graphVregsCount++;
        }
    
    // construct vreg liveness
    std::deque<CallStackEntry2> callStack;
    std::deque<FlowStackEntry3> flowStack;
    std::vector<bool> visited(codeBlocks.size(), false);
    // hold last vreg ssaId and position
    LastVRegMap lastVRegMap;
    
    // key - current res first key, value - previous first key and its flowStack pos
    PrevWaysIndexMap prevWaysIndexMap;
    // to track ways last block indices pair: block index, flowStackPos)
    std::pair<size_t, size_t> lastCommonCacheWayPoint{ SIZE_MAX, SIZE_MAX };
    std::vector<bool> waysToCache(codeBlocks.size(), false);
    ResSecondPointsToCache cblocksToCache(codeBlocks.size());
    
    size_t rbwCount = 0;
    size_t wrCount = 0;
    
    RoutineLvMap routineMap;
    std::vector<Liveness> livenesses[MAX_REGTYPES_NUM];
    std::unordered_map<size_t, size_t> callLiveTimesMap;
    
    for (size_t i = 0; i < regTypesNum; i++)
        livenesses[i].resize(graphVregsCounts[i]);
    
    // callLiveTime - call live time where routine will be called
    // reverse counted, begin from SIZE_MAX, used for joining svreg from routines
    // and svreg used in this time
    size_t callLiveTime = SIZE_MAX-1;
    size_t curLiveTime = 0;
    flowStack.push_back({ 0, 0 });
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            curLiveTime = cblock.start;
            // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                ARDOut << "joinpush: " << entry.blockIndex << "\n";
                if (flowStack.size() > 1)
                    putCrossBlockLivenesses(flowStack, codeBlocks, routineMap, 
                            callLiveTimesMap, lastVRegMap,
                            livenesses, vregIndexMaps, regTypesNum, regRanges);
                // update last vreg position
                for (const auto& sentry: cblock.ssaInfoMap)
                {
                    // update
                    auto res = lastVRegMap.insert({ sentry.first,
                                { { flowStack.size()-1, false } } });
                    if (!res.second) // if not first seen, just update
                        // update last
                        res.first->second.push_back({ flowStack.size()-1, false });
                    
                    // count read before writes (for cache weight)
                    if (sentry.second.readBeforeWrite)
                        rbwCount++;
                    if (sentry.second.ssaIdChange!=0)
                        wrCount++;
                }
                
                // main routine to handle ssaInfos
                SVRegMap ssaIdIdxMap;
                AsmRegVarUsage instrRVUs[8];
                cxuint instrRVUsCount = 0;
                
                size_t oldOffset = cblock.usagePos.readOffset;
                std::vector<AsmSingleVReg> readSVRegs;
                std::vector<AsmSingleVReg> writtenSVRegs;
                
                usageHandler.setReadPos(cblock.usagePos);
                // register in liveness
                while (true)
                {
                    AsmRegVarUsage rvu = { 0U, nullptr, 0U, 0U };
                    bool hasNext = false;
                    if (usageHandler.hasNext() && oldOffset < cblock.end)
                    {
                        hasNext = true;
                        rvu = usageHandler.nextUsage();
                    }
                    const size_t liveTime = oldOffset;
                    if ((!hasNext || rvu.offset > oldOffset) && oldOffset < cblock.end)
                    {
                        ARDOut << "apply to liveness. offset: " << oldOffset << "\n";
                        // apply to liveness
                        for (AsmSingleVReg svreg: readSVRegs)
                        {
                            auto svrres = ssaIdIdxMap.insert({ svreg, 0 });
                            Liveness& lv = getLiveness(svreg, svrres.first->second,
                                    cblock.ssaInfoMap.find(svreg)->second,
                                    livenesses, vregIndexMaps, regTypesNum, regRanges);
                            if (svrres.second)
                                // begin region from this block
                                lv.newRegion(curLiveTime);
                            lv.expand(liveTime);
                        }
                        for (AsmSingleVReg svreg: writtenSVRegs)
                        {
                            size_t& ssaIdIdx = ssaIdIdxMap[svreg];
                            ssaIdIdx++;
                            SSAInfo& sinfo = cblock.ssaInfoMap.find(svreg)->second;
                            Liveness& lv = getLiveness(svreg, ssaIdIdx, sinfo,
                                    livenesses, vregIndexMaps, regTypesNum, regRanges);
                            // works only with ISA where smallest instruction have 2 bytes!
                            // after previous read, but not after instruction.
                            // if var is not used anywhere then this liveness region
                            // blocks assignment for other vars
                            lv.insert(liveTime+1, liveTime+2);
                        }
                        // get linear deps and equal to
                        cxbyte lDeps[16];
                        usageHandler.getUsageDependencies(instrRVUsCount, instrRVUs, lDeps);
                        
                        addUsageDeps(lDeps, instrRVUsCount, instrRVUs,
                                linearDepMaps, vregIndexMaps, ssaIdIdxMap,
                                regTypesNum, regRanges);
                        
                        readSVRegs.clear();
                        writtenSVRegs.clear();
                        if (!hasNext)
                            break;
                        oldOffset = rvu.offset;
                        instrRVUsCount = 0;
                    }
                    if (hasNext && oldOffset < cblock.end && !rvu.useRegMode)
                        instrRVUs[instrRVUsCount++] = rvu;
                    if (oldOffset >= cblock.end)
                        break;
                    
                    for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
                    {
                        // per register/singlvreg
                        AsmSingleVReg svreg{ rvu.regVar, rindex };
                        if (rvu.rwFlags == ASMRVU_WRITE && rvu.regField!=ASMFIELD_NONE)
                            writtenSVRegs.push_back(svreg);
                        else // read or treat as reading // expand previous region
                            readSVRegs.push_back(svreg);
                    }
                }
            }
            else
            {
                cblocksToCache.increase(entry.blockIndex);
                ARDOut << "jcblockToCache: " << entry.blockIndex << "=" <<
                            cblocksToCache.count(entry.blockIndex) << "\n";
                
                // back, already visited
                flowStack.pop_back();
                
                size_t curWayBIndex = flowStack.back().blockIndex;
                if (lastCommonCacheWayPoint.first != SIZE_MAX)
                {
                    // mark point of way to cache (res first point)
                    waysToCache[lastCommonCacheWayPoint.first] = true;
                    ARDOut << "mark to pfcache " <<
                            lastCommonCacheWayPoint.first << ", " <<
                            curWayBIndex << "\n";
                    prevWaysIndexMap[curWayBIndex] = lastCommonCacheWayPoint;
                }
                lastCommonCacheWayPoint = { curWayBIndex, flowStack.size()-1 };
                ARDOut << "lastCcwP: " << curWayBIndex << "\n";
                continue;
            }
        }
        
        if (!callStack.empty() &&
            entry.blockIndex == callStack.back().callBlock &&
            entry.nextIndex-1 == callStack.back().callNextIndex)
        {
            ARDOut << " ret: " << entry.blockIndex << "\n";
            auto res = routineMap.insert({ entry.blockIndex, { } });
            if (res.second)
                createRoutineDataLv(codeBlocks, routineMap, res.first->second,
                        entry.blockIndex, vregIndexMaps, livenesses,
                        regTypesNum, regRanges);
            else
            {
                // already added join livenesses from all readBeforeWrites
                for (const auto& entry: res.first->second.rbwSSAIdMap)
                {
                    // find last
                    auto lvrit = lastVRegMap.find(entry.first);
                    LastVRegStackPos flowStackStart = (lvrit != lastVRegMap.end()) ?
                            lvrit->second.back() : LastVRegStackPos{ 0, false };
                    
                    joinVRegRecur(flowStack, codeBlocks, routineMap,
                            callLiveTimesMap, flowStackStart,
                            entry.first, entry.second, vregIndexMaps,
                            livenesses, regTypesNum, regRanges, false);
                }
            }
            callStack.pop_back(); // just return from call
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            //bool isCall = false;
            size_t nextBlock = cblock.nexts[entry.nextIndex].block;
            if (cblock.nexts[entry.nextIndex].isCall)
            {
                callStack.push_back({ entry.blockIndex, entry.nextIndex, nextBlock });
                //isCall = true;
            }
            
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            if (entry.nextIndex!=0) // if back from calls (just return from calls)
            {
                callLiveTimesMap.insert({ entry.blockIndex, callLiveTime });
                
                std::unordered_set<AsmSingleVReg> regSVRegs;
                // just add last access of svreg from call routines to lastVRegMap
                // and join svregs from routine with svreg used at this time
                for (const NextBlock& next: cblock.nexts)
                    if (next.isCall)
                    {
                        const RoutineDataLv& rdata = routineMap.find(next.block)->second;
                        for (const auto& entry: rdata.lastAccessMap)
                            if (regSVRegs.insert(entry.first).second)
                            {
                                auto res = lastVRegMap.insert({ entry.first,
                                        { { flowStack.size()-1, true } } });
                                if (!res.second) // if not first seen, just update
                                    // update last
                                    res.first->second.push_back(
                                                { flowStack.size()-1, true });
                            }
                        
                        for(Liveness* lv: rdata.allLivenesses)
                            lv->insert(callLiveTime, callLiveTime+1);
                        callLiveTime--;
                    }
            }
            
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
            // revert lastSSAIdMap
            flowStack.pop_back();
            
            // revert lastVRegs in call
            std::unordered_set<AsmSingleVReg> revertedSVRegs;
            for (const NextBlock& next: cblock.nexts)
                if (next.isCall)
                {
                    const RoutineDataLv& rdata = routineMap.find(next.block)->second;
                    for (const auto& entry: rdata.lastAccessMap)
                        if (revertedSVRegs.insert(entry.first).second)
                            revertLastSVReg(lastVRegMap, entry.first);
                }
            
            for (const auto& sentry: cblock.ssaInfoMap)
                revertLastSVReg(lastVRegMap, sentry.first);
            
            if (!flowStack.empty() && lastCommonCacheWayPoint.first != SIZE_MAX &&
                    lastCommonCacheWayPoint.second >= flowStack.size())
            {
                lastCommonCacheWayPoint =
                        { flowStack.back().blockIndex, flowStack.size()-1 };
                ARDOut << "POPlastCcwP: " << lastCommonCacheWayPoint.first << "\n";
            }
        }
    }
    
    // after, that resolve joins (join with already visited code)
    // SVRegMap in this cache: key - vreg, value - last flowStack entry position
    SimpleCache<size_t, LastStackPosMap> joinFirstPointsCache(wrCount<<1);
    // SVRegMap in this cache: key - vreg, value - first readBefore in second part
    SimpleCache<size_t, SVRegMap> joinSecondPointsCache(rbwCount<<1);
    
    flowStack.clear();
    std::fill(visited.begin(), visited.end(), false);
    
    flowStack.push_back({ 0, 0 });
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex])
                visited[entry.blockIndex] = true;
            else
            {
                joinRegVarLivenesses(flowStack, codeBlocks, routineMap, callLiveTimesMap,
                        prevWaysIndexMap, waysToCache, cblocksToCache,
                        joinFirstPointsCache, joinSecondPointsCache,
                        vregIndexMaps, livenesses, regTypesNum, regRanges);
                // back, already visited
                flowStack.pop_back();
                continue;
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
            if (cblocksToCache.count(entry.blockIndex)==2 &&
                !joinSecondPointsCache.hasKey(entry.blockIndex))
                // add to cache
                addJoinSecCacheEntry(routineMap, codeBlocks, joinSecondPointsCache,
                            entry.blockIndex);
            flowStack.pop_back();
        }
    }
    
    // move livenesses to AsmRegAllocator outLivenesses
    for (size_t regType = 0; regType < regTypesNum; regType++)
    {
        std::vector<Liveness>& livenesses2 = livenesses[regType];
        Array<OutLiveness>& outLivenesses2 = outLivenesses[regType];
        outLivenesses2.resize(livenesses2.size());
        for (size_t li = 0; li < livenesses2.size(); li++)
        {
            outLivenesses2[li].resize(livenesses2[li].l.size());
            std::copy(livenesses2[li].l.begin(), livenesses2[li].l.end(),
                      outLivenesses2[li].begin());
            livenesses2[li].clear();
        }
        livenesses2.clear();
    }
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

void AsmRegAllocator::allocateRegisters(cxuint sectionId)
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
    createSSAData(*section.usageHandler);
    applySSAReplaces();
    createLivenesses(*section.usageHandler);
    createInterferenceGraph();
    colorInterferenceGraph();
}
