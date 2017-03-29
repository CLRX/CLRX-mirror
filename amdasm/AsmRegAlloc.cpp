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
#include <stack>
#include <deque>
#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

typedef AsmRegAllocator::CodeBlock CodeBlock;
typedef AsmRegAllocator::NextBlock NextBlock;
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
            codeBlocks.push_back({ start, end, { }, false, false, false });
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
            it->haveCalls = entry.type == AsmCodeFlowType::CALL;
            if (entry.type == AsmCodeFlowType::CJUMP) // add next next block
                it->nexts.push_back({ size_t(it - codeBlocks.begin() + 1), false });
        }
    
    // reduce nexts
    for (CodeBlock& block: codeBlocks)
    { // first non-call nexts, for correct resolving SSA conflicts
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

struct SSAId
{
    size_t ssaId;
    size_t blockIndex;
};

// map of last SSAId for routine, key - varid, value - last SSA ids

typedef std::unordered_map<AsmSingleVReg, std::vector<size_t> > LastSSAIdMap;
struct RoutineData
{
    bool processed;
    LastSSAIdMap regVarMap;
};

struct FlowStackEntry
{
    size_t blockIndex;
    size_t nextIndex;
    LastSSAIdMap replacedMultiSSAIds;
        // ssaIds from called routine already visited before call
};

struct CallStackEntry
{
    size_t callBlock; // index
    size_t callNextIndex; // index of call next
};

typedef std::pair<size_t, size_t> SSAReplace; // first - orig ssaid, second - dest ssaid
typedef std::unordered_map<AsmSingleVReg, std::vector<SSAReplace> > ReplacesMap;

static inline void insertReplace(ReplacesMap& rmap, const AsmSingleVReg& vreg,
              size_t origId, size_t destId)
{
    auto res = rmap.insert({ vreg, {} });
    res.first->second.push_back({ origId, destId });
}

static void resolveSSAConflicts(const std::deque<FlowStackEntry>& prevFlowStack,
        const std::stack<CallStackEntry>& prevCallStack,
        const std::vector<bool>& prevVisited,
        const std::unordered_map<size_t, RoutineData>& routineMap,
        const std::vector<CodeBlock>& codeBlocks,
        ReplacesMap& replacesMap)
{
    size_t nextBlock = prevFlowStack.back().blockIndex;
    auto pfEnd = prevFlowStack.end();
    --pfEnd;
    LastSSAIdMap stackVarMap;
    for (auto pfit = prevFlowStack.begin(); pfit != pfEnd; ++pfit)
    {
        const FlowStackEntry& entry = *pfit;
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        for (const auto& sentry: cblock.ssaInfoMap)
        {
            const SSAInfo& sinfo = sentry.second;
            if (sinfo.ssaIdChange != 0)
                stackVarMap[sentry.first] = { sinfo.ssaId + sinfo.ssaIdChange - 1 };
        }
        for (const NextBlock& next: cblock.nexts)
            if (next.isCall)
            {
                const LastSSAIdMap& regVarMap =
                        routineMap.find(next.block)->second.regVarMap;
                for (const auto& sentry: regVarMap)
                    stackVarMap[sentry.first] = sentry.second;
            }
    }
    
    std::stack<CallStackEntry> callStack = prevCallStack;
    // traverse by graph from next block
    std::deque<FlowStackEntry> flowStack;
    flowStack.push_back({ nextBlock, 0 });
    std::vector<bool> visited(codeBlocks.size());
    std::fill(visited.begin(), visited.end(), false);
    
    struct ResolveEntry
    {
        size_t sourceBlock;
        bool handled;
    };
    std::unordered_map<AsmSingleVReg, ResolveEntry> toResolveMap;
    
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        { // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                for (auto& sentry: cblock.ssaInfoMap)
                {
                    const SSAInfo& sinfo = sentry.second;
                    auto res = toResolveMap.insert({ sentry.first,
                        { entry.blockIndex, false } });
                    
                    if (res.second && sinfo.readBeforeWrite)
                    {   // resolve conflict for this variable ssaId>
                        auto it = stackVarMap.find(sentry.first);
                        
                        if (it != stackVarMap.end())
                            // found, resolve by set ssaIdLast
                            for (size_t ssaId: it->second)
                                if (ssaId > sinfo.ssaIdBefore)
                                    insertReplace(replacesMap, sentry.first, ssaId,
                                                sinfo.ssaIdBefore);
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
        
        if (!callStack.empty() &&
            entry.blockIndex == callStack.top().callBlock &&
            entry.nextIndex-1 == callStack.top().callNextIndex)
            callStack.pop(); // just return from call
        
        if (entry.nextIndex < cblock.nexts.size() &&
            prevVisited[cblock.nexts[entry.nextIndex].block])
        {
            if (cblock.nexts[entry.nextIndex].isCall)
                callStack.push({ entry.blockIndex, entry.nextIndex });
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd &&
                 prevVisited[entry.blockIndex+1])
        {
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
            for (const auto& sentry: cblock.ssaInfoMap)
            {   // mark resolved variables as not handled for further processing
                auto it = toResolveMap.find(sentry.first);
                if (it != toResolveMap.end() && !it->second.handled &&
                    it->second.sourceBlock == entry.blockIndex)
                    // remove if not handled yet
                    toResolveMap.erase(it);
            }
            flowStack.pop_back();
        }
    }
}

static void joinRoutineData(LastSSAIdMap& dest, const LastSSAIdMap& src,
                const std::unordered_map<AsmSingleVReg, SSAInfo>& prevSSAInfoMap)
{
    for (const auto& entry: src)
    {
        auto res = dest.insert(entry); // find
        if (res.second)
            continue; // added new
        auto ssaInfoIt = prevSSAInfoMap.find(entry.first);
        std::vector<size_t>& destEntry = res.first->second;
        if (ssaInfoIt->second.ssaIdChange!=0)
        {
            if (ssaInfoIt != prevSSAInfoMap.end())
            {
                auto it = std::find(destEntry.begin(), destEntry.end(),
                                    ssaInfoIt->second.ssaIdLast);
                if (it != destEntry.end())
                    destEntry.erase(it); // remove old way
            }
            // add new ways
            for (size_t ssaId: entry.second)
            {
                auto it = std::find(destEntry.begin(), destEntry.end(), ssaId);
                if (it == destEntry.end())
                    destEntry.push_back(ssaId);
            }
        }
    }
}

static void joinLastSSAIdMap(LastSSAIdMap& dest, const LastSSAIdMap& src)
{
    for (const auto& entry: src)
    {
        auto res = dest.insert(entry); // find
        if (res.second)
            continue; // added new
        std::vector<size_t>& destEntry = res.first->second;
        // add new ways
        for (size_t ssaId: entry.second)
        {
            auto it = std::find(destEntry.begin(), destEntry.end(), ssaId);
            if (it == destEntry.end())
                destEntry.push_back(ssaId);
        }
    }
}

static void removeLastSSAIdMap(LastSSAIdMap& dest, const LastSSAIdMap& src)
{
    for (const auto& entry: src)
    {
        auto destIt = dest.find(entry.first); // find
        std::vector<size_t>& destEntry = destIt->second;
        // add new ways
        for (size_t ssaId: entry.second)
        {
            auto it = std::find(destEntry.begin(), destEntry.end(), ssaId);
            if (it != destEntry.end())
                destEntry.erase(it);
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
        
        cbit->usagePos = usageHandler.getReadPos();
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
    
    std::stack<CallStackEntry> callStack;
    ReplacesMap replacesMap;
    std::deque<FlowStackEntry> flowStack;
    // total SSA count
    std::unordered_map<AsmSingleVReg, size_t> totalSSACountMap;
    // last SSA ids in current way in code flow
    std::unordered_map<AsmSingleVReg, size_t> curSSAIdMap;
    // routine map - routine datas map, value - last SSA ids map
    std::unordered_map<size_t, RoutineData> routineMap;
    // initialize routineMap
    for (const CodeBlock& cblock: codeBlocks)
        for (const NextBlock& next: cblock.nexts)
            // all forks and calls
            routineMap[next.block] = { false };
    
    LastSSAIdMap lastMultiSSAIdMap; // current SSA id from visited calls
    std::unordered_set<size_t> selectedRoutines;
    std::vector<bool> visited(codeBlocks.size());
    std::fill(visited.begin(), visited.end(), false);
    flowStack.push_back({ 0, 0 });
    
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {   // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                
                for (auto& ssaEntry: cblock.ssaInfoMap)
                {
                    size_t& ssaId = curSSAIdMap[ssaEntry.first];
                    size_t& totalSSACount = totalSSACountMap[ssaEntry.first];
                    if (totalSSACount == 0 && ssaEntry.second.readBeforeWrite)
                    {   // first read before write at all, need change totalcount, ssaId
                        ssaId++;
                        totalSSACount++;
                    }
                    ssaEntry.second.ssaId = ssaId;
                    ssaEntry.second.ssaIdFirst = ssaEntry.second.ssaIdChange!=0 ?
                            ssaId : SIZE_MAX;
                    ssaEntry.second.ssaIdBefore = ssaId-1;
                    ssaId += ssaEntry.second.ssaIdChange;
                    ssaEntry.second.ssaIdLast = ssaEntry.second.ssaIdChange!=0 ?
                            ssaId-1 : SIZE_MAX;
                    totalSSACount = std::max(totalSSACount, ssaId);
                }
                // check if routineMap
                auto rit = routineMap.find(entry.blockIndex);
                if (rit != routineMap.end() && !rit->second.processed)
                    selectedRoutines.insert(entry.blockIndex);
                // add routine regvar map
                for (size_t routine: selectedRoutines)
                {
                    LastSSAIdMap& regVarMap = routineMap.find(routine)->second.regVarMap;
                    for (const auto& ssaEntry: cblock.ssaInfoMap)
                    {
                        const SSAInfo& sinfo = ssaEntry.second;
                        if (sinfo.ssaIdChange!=0)
                        {
                            std::vector<size_t>& ssas = regVarMap[ssaEntry.first];
                            auto lmsit = lastMultiSSAIdMap.find(ssaEntry.first);
                            if (lmsit != lastMultiSSAIdMap.end())
                            {   // if many parallel ssaId from routine returns
                                const std::vector<size_t>& ssaIdsToRemove = lmsit->second;
                                for (size_t s: ssaIdsToRemove)
                                {
                                    auto ssaIt = std::find(ssas.begin(), ssas.end(), s);
                                    if (ssaIt != ssas.end())
                                        ssas.erase(ssaIt);
                                    // add new
                                    ssas.push_back(sinfo.ssaIdLast);
                                }
                                lastMultiSSAIdMap.erase(lmsit);
                                // add to replaced ssaid to revert changes at pop
                                entry.replacedMultiSSAIds.insert(*lmsit);
                            }
                            else
                            {
                                auto ssaIt = std::find(ssas.begin(), ssas.end(),
                                            sinfo.ssaId-1);
                                if (ssaIt != ssas.end()) // update this point
                                    *ssaIt = sinfo.ssaIdLast;
                                else if (std::find(ssas.begin(), ssas.end(),
                                        sinfo.ssaIdLast) == ssas.end())
                                    // otherwise add new way
                                    ssas.push_back(sinfo.ssaIdLast);
                            }
                        }
                    }
                }
            }
            else
            {   // join routine data
                auto rit = routineMap.find(entry.blockIndex);
                if (rit != routineMap.end() && rit->second.processed)
                { // just join with selected routines
                    // if this ways to return are visited before
                    auto fcit = flowStack.end();
                    --fcit;
                    --fcit; // before this codeblock
                    const std::unordered_map<AsmSingleVReg, SSAInfo>& prevSSAInfoMap =
                            codeBlocks[fcit->blockIndex].ssaInfoMap;
                    for (size_t routine: selectedRoutines)
                        joinRoutineData(routineMap.find(routine)->second.regVarMap,
                                rit->second.regVarMap, prevSSAInfoMap);
                }
                resolveSSAConflicts(flowStack, callStack, visited, routineMap, codeBlocks,
                                    replacesMap);
                // back, already visited
                flowStack.pop_back();
                continue;
            }
        }
        
        if (!callStack.empty() &&
            entry.blockIndex == callStack.top().callBlock &&
            entry.nextIndex-1 == callStack.top().callNextIndex)
            callStack.pop(); // just return from call
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            if (cblock.nexts[entry.nextIndex].isCall)
                callStack.push({ entry.blockIndex, entry.nextIndex });
            
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            if (entry.nextIndex!=0) // if back from call
            { // expand lastMultiSSAIdMap from all calls
                for (const NextBlock& next: cblock.nexts)
                if (next.isCall)
                    {
                        auto it = routineMap.find(next.block); // must find
                        joinLastSSAIdMap(lastMultiSSAIdMap, it->second.regVarMap);
                    }
            }
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {   // revert lastMultiSSAIdMap changes (add removed entries)
            if (cblock.haveCalls)
            { //remove all return parallel ssaids
                for(const NextBlock& next: cblock.nexts)
                    if (!next.isCall)
                    {
                        auto it = routineMap.find(next.block); // must find
                        removeLastSSAIdMap(lastMultiSSAIdMap, it->second.regVarMap);
                    }
            }
            else // normal block
                lastMultiSSAIdMap.insert(entry.replacedMultiSSAIds.begin(),
                                entry.replacedMultiSSAIds.end());
            // erase at pop from selectedRoutines
            selectedRoutines.erase(entry.blockIndex); 
            for (const auto& ssaEntry: cblock.ssaInfoMap)
                curSSAIdMap[ssaEntry.first] -= ssaEntry.second.ssaIdChange;
            flowStack.pop_back();
        }
    }
    
    /* prepare SSA id replaces */
    struct MinSSAGraphNode
    {
        size_t minSSAId;
        bool visited;
        std::unordered_set<size_t> nexts;
        MinSSAGraphNode() : minSSAId(SIZE_MAX), visited(false) { }
    };
    struct MinSSAGraphStackEntry
    {
        std::unordered_map<size_t, MinSSAGraphNode>::iterator nodeIt;
        std::unordered_set<size_t>::const_iterator nextIt;
        size_t minSSAId;
    };
    
    for (auto& entry: replacesMap)
    {
        std::vector<SSAReplace>& replaces = entry.second;
        std::sort(replaces.begin(), replaces.end());
        replaces.resize(std::unique(replaces.begin(), replaces.end()) - replaces.begin());
        std::vector<SSAReplace> newReplaces;
        
        std::unordered_map<size_t, MinSSAGraphNode> ssaGraphNodes;
        
        auto it = replaces.begin();
        while (it != replaces.end())
        {
            auto itEnd = std::upper_bound(it, replaces.end(),
                            std::make_pair(it->first, size_t(SIZE_MAX)));
            auto prevIt = it;
            {
                MinSSAGraphNode& node = ssaGraphNodes[it->first];
                node.minSSAId = std::min(node.minSSAId, it->second);
                for (auto it2 = ++it; it2 != itEnd; ++it2)
                        node.nexts.insert(it->second);
            }
            for (; it != itEnd; ++it)
            {
                MinSSAGraphNode& node = ssaGraphNodes[it->second];
                for (auto it2 = prevIt; it2 != it; ++it2)
                    node.nexts.insert(it->second);
            }
        }
        // propagate min value
        std::stack<MinSSAGraphStackEntry> minSSAStack;
        for (auto ssaGraphNodeIt = ssaGraphNodes.begin();
                 ssaGraphNodeIt!=ssaGraphNodes.begin(); )
        {
            minSSAStack.push({ ssaGraphNodeIt, ssaGraphNodeIt->second.nexts.begin() });
            // traverse with minimalize SSA id
            while (!minSSAStack.empty())
            {
                MinSSAGraphStackEntry& entry = minSSAStack.top();
                MinSSAGraphNode& node = entry.nodeIt->second;
                bool toPop = false;
                if (entry.nextIt == node.nexts.begin())
                {
                    if (!node.visited)
                        node.visited = true;
                    else
                        toPop = true;
                }
                if (!toPop && entry.nextIt != node.nexts.end())
                {
                    auto nodeIt = ssaGraphNodes.find(*entry.nextIt);
                    if (nodeIt != ssaGraphNodes.end())
                    {
                        minSSAStack.push({ nodeIt, nodeIt->second.nexts.begin(),
                                nodeIt->second.minSSAId });
                    }
                    ++entry.nextIt;
                }
                else
                {
                    node.minSSAId = std::min(node.minSSAId, entry.minSSAId);
                    minSSAStack.pop();
                    if (!minSSAStack.empty())
                    {
                        MinSSAGraphStackEntry& pentry = minSSAStack.top();
                        pentry.minSSAId = std::min(pentry.minSSAId, node.minSSAId);
                    }
                }
            }
            // skip visited nodes
            while (ssaGraphNodeIt != ssaGraphNodes.end())
                if (!ssaGraphNodeIt->second.visited)
                    break;
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
            auto it = replacesMap.find(ssaEntry.first);
            if (it == replacesMap.end())
                continue;
            SSAInfo& sinfo = ssaEntry.second;
            std::vector<SSAReplace>& replaces = it->second;
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
}

struct Liveness
{
    std::vector<std::pair<size_t, size_t> > l;
    
    Liveness() { }
    
    void expand(size_t k)
    {
        if (l.empty())
            l.push_back(std::make_pair(k, k+1));
        else
            l.back().second = k+1;
    }
    void newRegion(size_t k)
    { l.push_back(std::make_pair(k, k)); }
    
    bool contain(size_t t) const
    {
        auto it = std::lower_bound(l.begin(), l.end(), std::make_pair(t, size_t(0)));
        if (it==l.begin() && it->first>t)
            return false;
        if (it==l.end() || it->first>t)
            --it;
        return it->first<=t && t<it->second;
    }
    
    bool common(const Liveness& b) const
    {
        size_t i, j;
        for (i = j = 0; i < l.size() && j < b.l.size();)
        {
            if (l[i].first==l[i].second)
            {
                i++;
                continue;
            }
            if (b.l[j].first==b.l[j].second)
            {
                j++;
                continue;
            }
            if (l[i].first<b.l[j].first)
            {
                if (l[i].second > b.l[j].first)
                    return true; // common place
                i++;
            }
            else
            {
                if (l[i].first < b.l[j].second)
                    return true; // common place
                j++;
            }
        }
        return false;
    }
};

void AsmRegAllocator::createInterferenceGraph(ISAUsageHandler& usageHandler)
{
    // construct var index maps
    size_t graphVregsCounts[2] = { size_t(0), size_t(0) };
    for (const CodeBlock& cblock: codeBlocks)
        for (const auto& entry: cblock.ssaInfoMap)
        {
            VarIndexMap& vregIndices = vregIndexMaps[entry.first.regVar->type];
            size_t& graphVregsCount = graphVregsCounts[entry.first.regVar->type];
            std::vector<size_t>& ssaIdIndices = vregIndices[entry.first];
            const SSAInfo& sinfo = entry.second;
            size_t ssaIdCount = 0;
            if (sinfo.readBeforeWrite)
                ssaIdCount = sinfo.ssaIdBefore+1;
            if (sinfo.ssaIdChange!=0)
            {
                ssaIdCount = std::max(ssaIdCount, sinfo.ssaIdLast+1);
                ssaIdCount = std::max(ssaIdCount, sinfo.ssaIdFirst+1);
            }
            if (ssaIdIndices.size() < ssaIdCount)
                ssaIdIndices.resize(ssaIdCount, SIZE_MAX);
            
            if (sinfo.readBeforeWrite)
                ssaIdIndices[sinfo.ssaIdBefore] = graphVregsCount++;
            if (sinfo.ssaIdChange!=0)
            {   // fill up ssaIdIndices (with graph Ids)
                ssaIdIndices[sinfo.ssaIdFirst] = graphVregsCount++;
                for (size_t ssaId = sinfo.ssaId+1;
                        ssaId < sinfo.ssaId+sinfo.ssaIdChange-1; ssaId++)
                    ssaIdIndices[ssaId] = graphVregsCount++;
                ssaIdIndices[sinfo.ssaIdLast] = graphVregsCount++;
            }
        }
    // add real registers
    
    // construct vreg liveness
    std::deque<FlowStackEntry> flowStack;
    std::vector<bool> visited(codeBlocks.size());
    std::fill(visited.begin(), visited.end(), false);
    
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {   // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
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
        else if (entry.nextIndex==0 && cblock.nexts.empty() && !cblock.haveEnd)
        {
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
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
    createSSAData(*section.usageHandler);
}
