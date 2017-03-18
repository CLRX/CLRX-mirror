/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
#include <deque>
#include <vector>
#include <utility>
#include <unordered_map>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

typedef AsmRegAllocator::CodeBlock CodeBlock;
typedef AsmRegAllocator::SSAInfo SSAInfo;

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
            throw Exception("Offset before previous instruction");
        if (!instrStruct.empty() && offset - lastOffset < defaultInstrSize)
            throw Exception("Offset between previous instruction");
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
        {   // normal regvarusages
            instrStruct.push_back(argFlags);
            if ((argFlags & (1U<<(pushedArgs-1))) != 0)
                regVarUsages.back().rwFlags |= 0x80;
            else // reg usages
                regUsages.back().rwFlags |= 0x80;
        }
        else // use reg regvarusages
        {
            if ((pushedArgs & 7) != 0) //if only not pushed args remains
                instrStruct.push_back(argFlags);
            instrStruct[instrStruct.size() - ((pushedArgs+7) >> 3) - 1] = pushedArgs;
        }
    }
}

AsmRegVarUsage ISAUsageHandler::nextUsage()
{
    if (!isNext)
        throw Exception("No reg usage in this code");
    AsmRegVarUsage rvu;
    // get regvarusage
    bool lastRegUsage = false;
    rvu.offset = readOffset;
    if (!useRegMode && instrStruct[instrStructPos] == 0x80)
    {   // useRegMode (begin fetching useregs)
        useRegMode = true;
        argPos = 0;
        instrStructPos++;
        // pushedArgs - numer of useregs, 0 - 256 useregs
        pushedArgs = instrStruct[instrStructPos++];
        argFlags = instrStruct[instrStructPos];
    }
    
    if ((instrStruct[instrStructPos] & (1U << (argPos&7))) != 0)
    {   // regvar usage
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
    {   // simple reg usage
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
    {   // use reg (simple reg usage, second structure)
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
    {   // if inside useregs
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

static inline bool codeBlockStartLess(const AsmRegAllocator::CodeBlock& c1,
                  const AsmRegAllocator::CodeBlock& c2)
{ return c1.start < c2.start; }

void AsmRegAllocator::createCodeStructure(const std::vector<AsmCodeFlowEntry>& codeFlow,
             size_t codeSize, const cxbyte* code)
{
    ISAAssembler* isaAsm = assembler.isaAssembler;
    std::vector<size_t> splits;
    std::vector<size_t> codeStarts;
    std::vector<size_t> codeEnds;
    codeStarts.push_back(0);
    codeEnds.push_back(codeSize);
    for (const AsmCodeFlowEntry& entry: codeFlow)
    {
        size_t instrAfter = 0;
        if (entry.type == AsmCodeFlowType::JUMP || entry.type == AsmCodeFlowType::CJUMP)
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
    codeEnds.resize(std::unique(codeEnds.begin(), codeEnds.end())
                - codeEnds.begin());
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
    codeStarts.resize(std::unique(codeStarts.begin(), codeStarts.end())
                - codeStarts.begin());
    // divide to blocks
    for (size_t codeStart: codeStarts)
    {
        size_t codeEnd = *std::lower_bound(codeEnds.begin(), codeEnds.end(), codeStart);
        splitIt = std::lower_bound(splitIt, splits.end(), codeStart);
        if (splitIt == splits.end())
            break; // end of code blocks
        for (size_t start = *splitIt; start < codeEnd;)
        {
            start = *splitIt;
            ++splitIt;
            size_t end = codeEnd;
            if (splitIt != splits.end())
                end = std::min(end, *splitIt);
            codeBlocks.push_back({ start, end, { }, false, false });
        }
        if (splitIt == splits.end())
            break; // end of code blocks
    }
    
    // construct flow-graph
    for (const AsmCodeFlowEntry& entry: codeFlow)
        if (entry.type == AsmCodeFlowType::CALL || entry.type == AsmCodeFlowType::JUMP ||
            entry.type == AsmCodeFlowType::CJUMP || entry.type == AsmCodeFlowType::RETURN)
        {
            auto it = binaryFind(codeBlocks.begin(), codeBlocks.end(),
                    CodeBlock{ entry.target }, codeBlockStartLess);
            
            if (entry.type == AsmCodeFlowType::RETURN)
            {   // if block have return
                if (it != codeBlocks.end())
                    it->haveReturn = true;
                continue;
            }
            
            if (entry.type == AsmCodeFlowType::END && it != codeBlocks.end())
                it->haveEnd = true;
            
            size_t instrAfter = entry.offset + isaAsm->getInstructionSize(
                        codeSize - entry.offset, code + entry.offset);
            auto it2 = binaryFind(codeBlocks.begin(), codeBlocks.end(),
                    CodeBlock{ instrAfter }, codeBlockStartLess);
            if (it == codeBlocks.end() || it2 == codeBlocks.end())
                continue; // error!
            
            it->nexts.push_back({ size_t(it2 - codeBlocks.begin()),
                        entry.type == AsmCodeFlowType::CALL });
            if (entry.type != AsmCodeFlowType::JUMP) // add next next block
                it->nexts.push_back({ size_t(it - codeBlocks.begin() + 1), false });
        }
    
    // reduce nexts
    for (CodeBlock& block: codeBlocks)
    {
        std::sort(block.nexts.begin(), block.nexts.end(),
                  [](const NextBlock& n1, const NextBlock& n2)
                  { return n1.block < n2.block ||
                      (n1.block==n2.block && int(n1.isCall)<int(n2.isCall)); });
        auto it = std::unique(block.nexts.begin(), block.nexts.end(),
                  [](const NextBlock& n1, const NextBlock& n2)
                  { return n1.block == n2.block && n1.isCall == n2.isCall; });
        block.nexts.resize(it - block.nexts.begin());
    }
}

struct FlowStackEntry
{
    size_t blockIndex;
    size_t nextIndex;
};

struct SSAId
{
    size_t ssaId;
    size_t blockIndex;
};

static void resolveSSAConflicts(std::deque<FlowStackEntry>& prevFlowStack,
        std::vector<CodeBlock>& codeBlocks, size_t nextBlock)
{
    std::unordered_map<AsmSingleVReg, SSAId> stackVarMap;
    for (const FlowStackEntry& entry: prevFlowStack)
    {
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        for (const auto& sentry: cblock.ssaInfoMap)
        {
            const SSAInfo& sinfo = sentry.second;
            stackVarMap[sentry.first] =
                    { sinfo.ssaId + sinfo.ssaIdChange - 1, entry.nextIndex };
        }
    }
    // traverse by graph from next block
    std::deque<FlowStackEntry> flowStack;
    std::vector<bool> visited(codeBlocks.size());
    
    struct ResolveEntry
    {
        size_t sourceBlock; // current source block index
        size_t firstSSAId; // from first occurrence
        bool firstOccur; // true if first occurence
        bool handled; // handled in current way
        size_t readBeforeBlock;
        size_t ssaIdBefore;
    };
    std::unordered_map<AsmSingleVReg, ResolveEntry> toResolveMap;
    
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        { // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                for (auto& sentry: cblock.ssaInfoMap)
                {
                    SSAInfo& sinfo = sentry.second;
                    size_t blkReadBefore = sinfo.ssaIdBefore!=SIZE_MAX ?
                            entry.blockIndex : SIZE_MAX;
                    auto res = toResolveMap.insert({ sentry.first,
                            { entry.blockIndex, sinfo.ssaId, true, false,
                                blkReadBefore } });
                    if (!res.second && res.first->second.readBeforeBlock == SIZE_MAX)
                    {
                        res.first->second.readBeforeBlock = blkReadBefore;
                        res.first->second.ssaIdBefore = sinfo.ssaIdBefore;
                    }
                    if (!res.second && res.first->second.handled)
                        continue; // handled
                    
                    if (!res.second && res.first->second.firstOccur &&
                        res.first->second.readBeforeBlock!=SIZE_MAX &&
                        sinfo.ssaIdChange != 0)
                    {   // resolve conflict for this variable ssaId>
                        auto it = stackVarMap.find(sentry.first);
                        if (it != stackVarMap.end() && it->second.ssaId > sinfo.ssaId)
                        {   // found, resolve by set ssaIdLast
                            CodeBlock& newBlock = codeBlocks[it->second.blockIndex];
                            newBlock.ssaInfoMap.find(sentry.first)->second.ssaIdLast =
                                    res.first->second.ssaIdBefore;
                        }
                        res.first->second.firstOccur = false;
                        res.first->second.handled = true;
                    }
                    else if (!res.second && !res.first->second.firstOccur &&
                        res.first->second.readBeforeBlock!=SIZE_MAX &&
                        sinfo.ssaIdFirst > res.first->second.firstSSAId)
                    {   // resolved but encounter more than once
                        sinfo.ssaIdFirst = res.first->second.firstSSAId;
                        res.first->second.sourceBlock = entry.blockIndex;
                        res.first->second.handled = true;
                    }
                }
            }
            else
            {   // back, already visited
                flowStack.pop_back();
                continue;
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (entry.nextIndex==0 && cblock.nexts.empty() &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
            for (const auto& sentry: cblock.ssaInfoMap)
            {   // mark resolved variables as not handled for further processing
                auto it = toResolveMap.find(sentry.first);
                if (it != toResolveMap.end())
                {
                    if (it->second.handled &&
                        it->second.sourceBlock == entry.blockIndex)
                        it->second.handled = false;
                    if (it->second.readBeforeBlock == entry.blockIndex)
                        it->second.readBeforeBlock = SIZE_MAX;
                }
            }
            flowStack.pop_back();
        }
    }
}

void AsmRegAllocator::createSSAData(ISAUsageHandler& usageHandler)
{
    usageHandler.rewind();
    auto cbit = codeBlocks.begin();
    AsmRegVarUsage rvu;
    if (!usageHandler.hasNext())
        return; // do nothing if no regusages
    rvu = usageHandler.nextUsage();
    
    while (true)
    {
        while (cbit != codeBlocks.end() && cbit->end <= rvu.offset)
            ++cbit;
        if (cbit == codeBlocks.end())
            break;
        // skip rvu's before codeblock
        while (rvu.offset < cbit->start && usageHandler.hasNext())
            rvu = usageHandler.nextUsage();
        if (rvu.offset < cbit->start)
            break;
        
        while(true)
        {   // process rvu
            if (rvu.regVar != nullptr)
            { // only if regVar
                for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
                {
                    SSAInfo& sinfo = cbit->ssaInfoMap[
                            AsmSingleVReg{ rvu.regVar, rindex }];
                    if ((rvu.rwFlags & ASMRVU_READ) != 0 && sinfo.ssaIdChange == 0)
                        sinfo.readBeforeWrite = true;
                    if (rvu.rwFlags == ASMRVU_WRITE && rvu.regField!=ASMFIELD_NONE)
                        sinfo.ssaIdChange++;
                }
            }
            // get next rvusage
            if (!usageHandler.hasNext())
                break;
            rvu = usageHandler.nextUsage();
            if (rvu.offset >= cbit->end)
                break; // if end of codeblock
        }
    }
    
    // resolve conflicts between SSA across paths (like loops)
    struct CallStackEntry
    {
        size_t callBlock; // index
        size_t callNextIndex; // index of call next
        std::vector<size_t> returns; // returns
    };
    std::deque<CallStackEntry> callStack;
    struct RoutineData
    {
        Array<size_t> returns; // block with returns
        // last SSA id for all ways in this routine
        std::unordered_map<AsmSingleVReg, Array<size_t> > ssaIdMap;
        // first - caller block, second - routine block
        Array<std::pair<size_t, size_t> > calls;
    };
    std::deque<FlowStackEntry> flowStack;
    std::unordered_map<size_t, RoutineData> routineMap;
    // total SSA count
    std::unordered_map<AsmSingleVReg, size_t> totalSSACountMap;
    // last SSA ids in current way in code flow
    std::unordered_map<AsmSingleVReg, size_t> curSSAIdMap;
    
    std::vector<bool> visited(codeBlocks.size());
    /* resolve SSA conflict:
     * when not visited block goes to visited block (win lower SSAid)
     *    fix many different (different ways) SSAid in subgraph of visited block
     * when return blocks of routine have different SSAids
     *    (in different ways) (join all together)
     */
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        { // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                for (auto& ssaEntry: cblock.ssaInfoMap)
                {
                    size_t& ssaId = curSSAIdMap[ssaEntry.first];
                    size_t& totalSSACount = totalSSACountMap[ssaEntry.first];
                    ssaEntry.second.ssaId = ssaEntry.second.ssaIdFirst = ssaId;
                    ssaEntry.second.ssaIdBefore = ssaId-1;
                    ssaId += ssaEntry.second.ssaIdChange;
                    ssaEntry.second.ssaIdLast = ssaId-1;
                    totalSSACount = std::max(totalSSACount, ssaId);
                }
                /*if (!callStack.empty() &&
                    entry.blockIndex == callStack.top().callBlock &&
                    entry.nextIndex-1 == callStack.top().callNextIndex)
                {   // insert new routine
                    CallStackEntry& centry = callStack.top();
                    auto res = routineMap.insert(std::make_pair(
                        codeBlocks[centry.callBlock].nexts[centry.callNextIndex].block,
                        RoutineData{ }));
                    res.first->second.returns =
                        Array<size_t>(centry.returns.begin(), centry.returns.end());
                    callStack.pop(); // just return from call
                }*/
            }
            else
            {   // back, already visited
                size_t nextIndex = entry.blockIndex;
                flowStack.pop_back();
                resolveSSAConflicts(flowStack, codeBlocks, nextIndex);
                continue;
            }
        }
        if (entry.nextIndex < cblock.nexts.size())
        {
            /*if (cblock.nexts[entry.nextIndex].isCall)
            {
                auto it = routineMap.find(cblock.nexts[entry.nextIndex].block);
                if (it == routineMap.end())
                    // add return block to callstack
                    callStack.push({ entry.blockIndex, entry.nextIndex });
                else
                {   // if routine already visited
                    auto it = routineMap.find(cblock.nexts[entry.nextIndex].block);
                    entry.nextIndex++;
                    continue;
                }
            }*/
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (entry.nextIndex==0 && cblock.nexts.empty() &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
            /*if (!callStack.empty() && cblock.haveReturn)
            { // add return to stack
                CallStackEntry& centry = callStack.back();
                centry.returns.push_back(entry.blockIndex);
            }*/
            for (const auto& ssaEntry: cblock.ssaInfoMap)
                curSSAIdMap[ssaEntry.first] -= ssaEntry.second.ssaIdChange;
            flowStack.pop_back();
        }
    }
}

void AsmRegAllocator::allocateRegisters(cxuint sectionId)
{   // before any operation, clear all
    codeBlocks.clear();
    // set up
    const AsmSection& section = assembler.sections[sectionId];
    createCodeStructure(section.codeFlow, section.content.size(), section.content.data());
}
