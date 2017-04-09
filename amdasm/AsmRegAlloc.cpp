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
#include <map>
#include <set>
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
    rvu.useRegMode = useRegMode; // no ArgPos
    
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

struct ResolveEntry
{
    size_t sourceBlock;
    bool handled;
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
    std::vector<bool> visited(codeBlocks.size(), false);
    
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
        if (entry.first.regVar==nullptr)
            continue;
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
    
    cxuint regRanges[MAX_REGTYPES_NUM*2];
    cxuint realRegsCount[MAX_REGTYPES_NUM];
    std::fill(realRegsCount, realRegsCount+MAX_REGTYPES_NUM, 0);
    size_t regTypesNum;
    assembler.isaAssembler->getRegisterRanges(regTypesNum, regRanges);
    
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
            // only if regVar
            for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
            {
                auto res = cbit->ssaInfoMap.insert(
                        { AsmSingleVReg{ rvu.regVar, rindex }, SSAInfo() });
                
                SSAInfo& sinfo = res.first->second;
                if (res.second)
                    sinfo.firstPos = rvu.offset;
                if ((rvu.rwFlags & ASMRVU_READ) != 0 && sinfo.ssaIdChange == 0)
                    sinfo.readBeforeWrite = true;
                if (rvu.regVar!=nullptr && rvu.rwFlags == ASMRVU_WRITE &&
                            rvu.regField!=ASMFIELD_NONE)
                    sinfo.ssaIdChange++;
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
            routineMap[next.block].processed = false;
    
    LastSSAIdMap lastMultiSSAIdMap; // current SSA id from visited calls
    std::unordered_set<size_t> selectedRoutines;
    std::vector<bool> visited(codeBlocks.size(), false);
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
                        if (sinfo.ssaIdChange!=0 && ssaEntry.first.regVar!=nullptr)
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
                                // add to replaced ssaid to revert changes at pop
                                entry.replacedMultiSSAIds.insert(*lmsit);
                                lastMultiSSAIdMap.erase(lmsit);
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
            else // normal block (revert changes in lastMultiSSAIdMap)
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
    std::map<size_t, size_t> l;
    
    Liveness() { }
    
    void clear()
    { l.clear(); }
    
    void expand(size_t k)
    {
        if (l.empty())
            l.insert(std::make_pair(k, k+1));
        else
        {
            auto it = l.end();
            --it;
            it->second = k+1;
        }
    }
    void newRegion(size_t k)
    {
        if (l.empty())
            l.insert(std::make_pair(k, k));
        else
        {
            auto it = l.end();
            --it;
            if (it->first != k && it->second != k)
                l.insert(std::make_pair(k, k));
        }
    }
    
    void insert(size_t k, size_t k2)
    {
        auto it1 = l.lower_bound(k);
        if (it1!=l.begin() && (it1==l.end() || it1->first>k))
            --it1;
        if (it1->second < k)
            ++it1;
        auto it2 = l.lower_bound(k2);
        if (it1!=it2)
        {
            k = std::min(k, it1->first);
            k2 = std::max(k2, (--it2)->second);
            l.erase(it1, it2);
        }
        l.insert(std::make_pair(k, k2));
    }
    
    bool contain(size_t t) const
    {
        auto it = l.lower_bound(t);
        if (it==l.begin() && it->first>t)
            return false;
        if (it==l.end() || it->first>t)
            --it;
        return it->first<=t && t<it->second;
    }
    
    bool common(const Liveness& b) const
    {
        auto i = l.begin();
        auto j = b.l.begin();
        for (; i != l.end() && j != b.l.end();)
        {
            if (i->first==i->second)
            {
                ++i;
                continue;
            }
            if (j->first==j->second)
            {
                ++j;
                continue;
            }
            if (i->first<j->first)
            {
                if (i->second > j->first)
                    return true; // common place
                ++i;
            }
            else
            {
                if (i->first < j->second)
                    return true; // common place
                ++j;
            }
        }
        return false;
    }
};

typedef AsmRegAllocator::VarIndexMap VarIndexMap;

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
    if (ssaIdIdx==0)
        ssaId = ssaInfo.ssaIdBefore;
    else if (ssaIdIdx==1)
        ssaId = ssaInfo.ssaIdFirst;
    else if (ssaIdIdx<ssaInfo.ssaIdChange)
        ssaId = ssaInfo.ssaId + ssaIdIdx-1;
    else // last
        ssaId = ssaInfo.ssaIdLast;
    
    cxuint regType = getRegType(regTypesNum, regRanges, svreg); // regtype
    const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
    const std::vector<size_t>& ssaIdIndices =
                vregIndexMap.find(svreg)->second;
    return livenesses[regType][ssaIdIndices[ssaId]];
}

typedef std::deque<FlowStackEntry>::const_iterator FlowStackCIter;

struct VRegLastPos
{
    size_t ssaId; // last SSA id
    std::vector<FlowStackCIter> blockChain; // subsequent blocks that changes SSAId
};

/* TODO: add handling calls
 * handle many start points in this code (for example many kernel's in same code)
 * replace sets by vector, and sort and remove same values on demand
 */

typedef std::unordered_map<AsmSingleVReg, VRegLastPos> LastVRegMap;

static void putCrossBlockLivenesses(const std::deque<FlowStackEntry>& flowStack,
        const std::vector<CodeBlock>& codeBlocks,
        const std::vector<size_t>& codeBlockLiveTimes, const LastVRegMap& lastVRegMap,
        std::vector<Liveness>* livenesses, const VarIndexMap* vregIndexMaps,
        size_t regTypesNum, const cxuint* regRanges)
{
    const CodeBlock& cblock = codeBlocks[flowStack.back().blockIndex];
    for (const auto& entry: cblock.ssaInfoMap)
        if (entry.second.readBeforeWrite)
        { // find last 
            auto lvrit = lastVRegMap.find(entry.first);
            if (lvrit == lastVRegMap.end())
                continue; // not found
            const VRegLastPos& lastPos = lvrit->second;
            FlowStackCIter flit = lastPos.blockChain.back();
            cxuint regType = getRegType(regTypesNum, regRanges, entry.first);
            const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
            const std::vector<size_t>& ssaIdIndices =
                        vregIndexMap.find(entry.first)->second;
            Liveness& lv = livenesses[regType][ssaIdIndices[entry.second.ssaIdBefore]];
            FlowStackCIter flitEnd = flowStack.end();
            --flitEnd; // before last element
            // insert live time to last seen position
            const CodeBlock& lastBlk = codeBlocks[flit->blockIndex];
            size_t toLiveCvt = codeBlockLiveTimes[flit->blockIndex] - lastBlk.start;
            lv.insert(lastBlk.ssaInfoMap.find(entry.first)->second.lastPos + toLiveCvt,
                    toLiveCvt + lastBlk.end);
            for (++flit; flit != flitEnd; ++flit)
            {
                const CodeBlock& cblock = codeBlocks[flit->blockIndex];
                size_t blockLiveTime = codeBlockLiveTimes[flit->blockIndex];
                lv.insert(blockLiveTime, cblock.end-cblock.start + blockLiveTime);
            }
        }
}

static void putCrossBlockForLoop(const std::deque<FlowStackEntry>& flowStack,
        const std::vector<CodeBlock>& codeBlocks,
        const std::vector<size_t>& codeBlockLiveTimes,
        std::vector<Liveness>* livenesses, const VarIndexMap* vregIndexMaps,
        size_t regTypesNum, const cxuint* regRanges)
{
    auto flitStart = flowStack.end();
    --flitStart;
    size_t curBlock = flitStart->blockIndex;
    // find step in way
    while (flitStart->blockIndex != curBlock) --flitStart;
    auto flitEnd = flowStack.end();
    --flitEnd;
    std::unordered_map<AsmSingleVReg, std::pair<size_t, size_t> > varMap;
    
    // collect var to check
    size_t flowPos = 0;
    for (auto flit = flitStart; flit != flitEnd; ++flit, flowPos++)
    {
        const CodeBlock& cblock = codeBlocks[flit->blockIndex];
        for (const auto& entry: cblock.ssaInfoMap)
        {
            const SSAInfo& sinfo = entry.second;
            size_t lastSSAId = (sinfo.ssaIdChange != 0) ? sinfo.ssaIdLast :
                    (sinfo.readBeforeWrite) ? sinfo.ssaIdBefore : 0;
            varMap[entry.first] = { lastSSAId, flowPos };
        }
    }
    // find connections
    flowPos = 0;
    for (auto flit = flitStart; flit != flitEnd; ++flit, flowPos++)
    {
        const CodeBlock& cblock = codeBlocks[flit->blockIndex];
        for (const auto& entry: cblock.ssaInfoMap)
        {
            const SSAInfo& sinfo = entry.second;
            auto varMapIt = varMap.find(entry.first);
            if (!sinfo.readBeforeWrite || varMapIt == varMap.end() ||
                flowPos > varMapIt->second.second ||
                sinfo.ssaIdBefore != varMapIt->second.first)
                continue;
            // just connect
            
            cxuint regType = getRegType(regTypesNum, regRanges, entry.first);
            const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
            const std::vector<size_t>& ssaIdIndices =
                        vregIndexMap.find(entry.first)->second;
            Liveness& lv = livenesses[regType][ssaIdIndices[entry.second.ssaIdBefore]];
            
            if (flowPos == varMapIt->second.second)
            {   // fill whole loop
                for (auto flit2 = flitStart; flit != flitEnd; ++flit)
                {
                    const CodeBlock& cblock = codeBlocks[flit2->blockIndex];
                    size_t blockLiveTime = codeBlockLiveTimes[flit2->blockIndex];
                    lv.insert(blockLiveTime, cblock.end-cblock.start + blockLiveTime);
                }
                continue;
            }
            
            size_t flowPos2 = 0;
            for (auto flit2 = flitStart; flowPos2 < flowPos; ++flit2, flowPos++)
            {
                const CodeBlock& cblock = codeBlocks[flit2->blockIndex];
                size_t blockLiveTime = codeBlockLiveTimes[flit2->blockIndex];
                lv.insert(blockLiveTime, cblock.end-cblock.start + blockLiveTime);
            }
            // insert liveness for last block in loop of last SSAId (prev round)
            auto flit2 = flitStart + flowPos;
            const CodeBlock& firstBlk = codeBlocks[flit2->blockIndex];
            size_t toLiveCvt = codeBlockLiveTimes[flit2->blockIndex] - firstBlk.start;
            lv.insert(codeBlockLiveTimes[flit2->blockIndex],
                    firstBlk.ssaInfoMap.find(entry.first)->second.firstPos + toLiveCvt);
            // insert liveness for first block in loop of last SSAId
            flit2 = flitStart + (varMapIt->second.second+1);
            const CodeBlock& lastBlk = codeBlocks[flit2->blockIndex];
            toLiveCvt = codeBlockLiveTimes[flit2->blockIndex] - lastBlk.start;
            lv.insert(lastBlk.ssaInfoMap.find(entry.first)->second.lastPos + toLiveCvt,
                    toLiveCvt + lastBlk.end);
            // fill up loop end
            for (++flit2; flit2 != flitEnd; ++flit2)
            {
                const CodeBlock& cblock = codeBlocks[flit2->blockIndex];
                size_t blockLiveTime = codeBlockLiveTimes[flit2->blockIndex];
                lv.insert(blockLiveTime, cblock.end-cblock.start + blockLiveTime);
            }
        }
    }
}

struct LiveBlock
{
    size_t start;
    size_t end;
    size_t vidx;
    
    bool operator==(const LiveBlock& b) const
    { return start==b.start && end==b.end && vidx==b.vidx; }
    
    bool operator<(const LiveBlock& b) const
    { return start<b.start || (start==b.start &&
            (end<b.end || (end==b.end && vidx<b.vidx))); }
};

typedef AsmRegAllocator::LinearDep LinearDep;
typedef AsmRegAllocator::EqualToDep EqualToDep;
typedef std::unordered_map<size_t, LinearDep> LinearDepMap;
typedef std::unordered_map<size_t, EqualToDep> EqualToDepMap;

static void addUsageDeps(const cxbyte* ldeps, const cxbyte* edeps, cxuint rvusNum,
            const AsmRegVarUsage* rvus, LinearDepMap* ldepsOut,
            EqualToDepMap* edepsOut, const VarIndexMap* vregIndexMaps,
            std::unordered_map<AsmSingleVReg, size_t> ssaIdIdxMap,
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
            ldepsOut[regType][vidxes[k-1]].nextVidxes.push_back(vidxes[k]);
            ldepsOut[regType][vidxes[k]].prevVidxes.push_back(vidxes[k-1]);
        }
    }
    // add single arg linear dependencies
    for (cxuint i = 0; i < rvusNum; i++)
        if ((rvuAdded & (1U<<i)) == 0 && rvus[i].rstart+1<rvus[i].rend)
        {
            const AsmRegVarUsage& rvu = rvus[i];
            std::vector<size_t> vidxes;
            cxuint regType = UINT_MAX;
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
            for (size_t j = 1; j < vidxes.size(); j++)
            {
                ldepsOut[regType][vidxes[j-1]].nextVidxes.push_back(vidxes[j]);
                ldepsOut[regType][vidxes[j]].prevVidxes.push_back(vidxes[j-1]);
            }
        }
        
    /* equalTo dependencies */
    count = edeps[0];
    pos = 1;
    for (cxuint i = 0; i < count; i++)
    {
        cxuint ccount = edeps[pos++];
        std::vector<size_t> vidxes;
        cxuint regType = UINT_MAX;
        for (cxuint j = 0; j < ccount; j++)
        {
            const AsmRegVarUsage& rvu = rvus[edeps[pos++]];
            // only one register should be set for equalTo depencencies
            // other registers in range will be resolved by linear dependencies
            AsmSingleVReg svreg = {rvu.regVar, rvu.rstart};
            auto sit = ssaIdIdxMap.find(svreg);
            if (regType==UINT_MAX)
                regType = getRegType(regTypesNum, regRanges, svreg);
            const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
            const std::vector<size_t>& ssaIdIndices =
                        vregIndexMap.find(svreg)->second;
            // push variable index
            vidxes.push_back(ssaIdIndices[sit->second]);
        }
        for (size_t j = 1; j < vidxes.size(); j++)
        {
            edepsOut[regType][vidxes[j-1]].nextVidxes.push_back(vidxes[j]);
            edepsOut[regType][vidxes[j]].prevVidxes.push_back(vidxes[j-1]);
        }
    }
}

void AsmRegAllocator::createInterferenceGraph(ISAUsageHandler& usageHandler)
{
    // construct var index maps
    size_t graphVregsCounts[MAX_REGTYPES_NUM];
    std::fill(graphVregsCounts, graphVregsCounts+regTypesNum, size_t(0));
    cxuint regRanges[MAX_REGTYPES_NUM*2];
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
    
    // construct vreg liveness
    std::deque<FlowStackEntry> flowStack;
    std::vector<bool> visited(codeBlocks.size(), false);
    // hold last vreg ssaId and position
    LastVRegMap lastVRegMap;
    // hold start live time position for every code block
    std::vector<size_t> codeBlockLiveTimes(codeBlocks.size());
    std::unordered_set<size_t> blockInWay;
    
    std::vector<Liveness> livenesses[MAX_REGTYPES_NUM];
    
    for (size_t i = 0; i < regTypesNum; i++)
        livenesses[i].resize(graphVregsCounts[i]);
    
    size_t curLiveTime = 0;
    
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {   // process current block
            if (!blockInWay.insert(entry.blockIndex).second)
            {   // if loop
                putCrossBlockForLoop(flowStack, codeBlocks, codeBlockLiveTimes, 
                        livenesses, vregIndexMaps, regTypesNum, regRanges);
                flowStack.pop_back();
                continue;
            }
            
            codeBlockLiveTimes[entry.blockIndex] = curLiveTime;
            putCrossBlockLivenesses(flowStack, codeBlocks, codeBlockLiveTimes, 
                    lastVRegMap, livenesses, vregIndexMaps, regTypesNum, regRanges);
            
            for (const auto& sentry: cblock.ssaInfoMap)
            {
                const SSAInfo& sinfo = sentry.second;
                // update
                size_t lastSSAId =  (sinfo.ssaIdChange != 0) ? sinfo.ssaIdLast :
                        (sinfo.readBeforeWrite) ? sinfo.ssaIdBefore : 0;
                FlowStackCIter flit = flowStack.end();
                --flit; // to last position
                auto res = lastVRegMap.insert({ sentry.first, 
                            { lastSSAId, { flit } } });
                if (!res.second) // if not first seen, just update
                {   // update last
                    res.first->second.ssaId = lastSSAId;
                    res.first->second.blockChain.push_back(flit);
                }
            }
            
            size_t curBlockLiveEnd = cblock.end - cblock.start + curLiveTime;
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                std::unordered_map<AsmSingleVReg, size_t> ssaIdIdxMap;
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
                    size_t liveTimeNext = curBlockLiveEnd;
                    if (usageHandler.hasNext())
                    {
                        rvu = usageHandler.nextUsage();
                        if (!rvu.useRegMode)
                            instrRVUs[instrRVUsCount++] = rvu;
                        liveTimeNext = std::min(rvu.offset, cblock.end) -
                                cblock.start + curLiveTime;
                    }
                    size_t liveTime = oldOffset - cblock.start + curLiveTime;
                    if (!usageHandler.hasNext() || rvu.offset >= oldOffset)
                    {   // apply to liveness
                        for (AsmSingleVReg svreg: readSVRegs)
                        {
                            Liveness& lv = getLiveness(svreg, ssaIdIdxMap[svreg],
                                    cblock.ssaInfoMap.find(svreg)->second,
                                    livenesses, vregIndexMaps, regTypesNum, regRanges);
                            if (!lv.l.empty() && (--lv.l.end())->first < curLiveTime)
                                lv.newRegion(curLiveTime); // begin region from this block
                            lv.expand(liveTime);
                        }
                        for (AsmSingleVReg svreg: writtenSVRegs)
                        {
                            size_t& ssaIdIdx = ssaIdIdxMap[svreg];
                            ssaIdIdx++;
                            SSAInfo& sinfo = cblock.ssaInfoMap.find(svreg)->second;
                            Liveness& lv = getLiveness(svreg, ssaIdIdx, sinfo,
                                    livenesses, vregIndexMaps, regTypesNum, regRanges);
                            if (liveTimeNext != curBlockLiveEnd)
                                // because live after this instr
                                lv.newRegion(liveTimeNext);
                            sinfo.lastPos = liveTimeNext - curLiveTime + cblock.start;
                        }
                        // get linear deps and equal to
                        cxbyte lDeps[16];
                        cxbyte eDeps[16];
                        usageHandler.getUsageDependencies(instrRVUsCount, instrRVUs,
                                        lDeps, eDeps);
                        
                        addUsageDeps(lDeps, eDeps, instrRVUsCount, instrRVUs,
                                linearDeps, equalToDeps, vregIndexMaps, ssaIdIdxMap,
                                regTypesNum, regRanges);
                        
                        readSVRegs.clear();
                        writtenSVRegs.clear();
                        if (!usageHandler.hasNext())
                            break; // end
                        oldOffset = rvu.offset;
                        instrRVUsCount = 0;
                    }
                    if (rvu.offset >= cblock.end)
                        break;
                    
                    for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
                    {   // per register/singlvreg
                        AsmSingleVReg svreg{ rvu.regVar, rindex };
                        if (rvu.rwFlags == ASMRVU_WRITE && rvu.regField == ASMFIELD_NONE)
                            writtenSVRegs.push_back(svreg);
                        else // read or treat as reading // expand previous region
                            readSVRegs.push_back(svreg);
                    }
                }
                curLiveTime += cblock.end-cblock.start;
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
        {   // revert lastSSAIdMap
            blockInWay.erase(entry.blockIndex);
            flowStack.pop_back();
            if (!flowStack.empty())
            {
                for (const auto& sentry: cblock.ssaInfoMap)
                {
                    auto lvrit = lastVRegMap.find(sentry.first);
                    if (lvrit != lastVRegMap.end())
                    {
                        VRegLastPos& lastPos = lvrit->second;
                        lastPos.ssaId = sentry.second.ssaIdBefore;
                        lastPos.blockChain.pop_back();
                        if (lastPos.blockChain.empty()) // just remove from lastVRegs
                            lastVRegMap.erase(lvrit);
                    }
                }
            }
        }
    }
    
    /// construct liveBlockMaps
    std::set<LiveBlock> liveBlockMaps[MAX_REGTYPES_NUM];
    for (size_t regType = 0; regType < regTypesNum; regType++)
    {
        std::set<LiveBlock>& liveBlockMap = liveBlockMaps[regType];
        std::vector<Liveness>& liveness = livenesses[regType];
        for (size_t li = 0; li < liveness.size(); li++)
        {
            Liveness& lv = liveness[li];
            for (const std::pair<size_t, size_t>& blk: lv.l)
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
            {
                auto vit2 = vit;
                for (++vit2; vit2 != varIndices.end(); ++vit2)
                    interGraph[*vit].insert(*vit2);
            }
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

void AsmRegAllocator::colorInterferenceGraph()
{
}

void AsmRegAllocator::allocateRegisters(cxuint sectionId)
{   // before any operation, clear all
    codeBlocks.clear();
    for (size_t i = 0; i < MAX_REGTYPES_NUM; i++)
    {
        vregIndexMaps[i].clear();
        interGraphs[i].clear();
    }
    cxuint maxRegs[MAX_REGTYPES_NUM];
    assembler.isaAssembler->getMaxRegistersNum(regTypesNum, maxRegs);
    
    // set up
    const AsmSection& section = assembler.sections[sectionId];
    createCodeStructure(section.codeFlow, section.content.size(), section.content.data());
    createSSAData(*section.usageHandler);
    createInterferenceGraph(*section.usageHandler);
}
