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

/*********
 * createLivenesses stuff
 *********/

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

static void getVIdx(const AsmSingleVReg& svreg, size_t ssaIdIdx,
        const AsmRegAllocator::SSAInfo& ssaInfo,
        const VarIndexMap* vregIndexMaps, size_t regTypesNum, const cxuint* regRanges,
        cxuint& regType, size_t& vidx)
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
    
    regType = getRegType(regTypesNum, regRanges, svreg); // regtype
    const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
    const std::vector<size_t>& vidxes = vregIndexMap.find(svreg)->second;
    /*ARDOut << "lvn[" << regType << "][" << vidxes[ssaId] << "]. ssaIdIdx: " <<
            ssaIdIdx << ". ssaId: " << ssaId << ". svreg: " << svreg.regVar << ":" <<
            svreg.index << "\n";*/
    //return livenesses[regType][ssaIdIndices[ssaId]];
    vidx = vidxes[ssaId];
}

static inline Liveness& getLiveness(const AsmSingleVReg& svreg, size_t ssaIdIdx,
        const AsmRegAllocator::SSAInfo& ssaInfo, std::vector<Liveness>* livenesses,
        const VarIndexMap* vregIndexMaps, size_t regTypesNum, const cxuint* regRanges)
{
    cxuint regType;
    size_t vidx;
    getVIdx(svreg, ssaIdIdx, ssaInfo, vregIndexMaps, regTypesNum, regRanges,
            regType, vidx);
    return livenesses[regType][vidx];
}

static void getVIdx2(const AsmSingleVReg& svreg, size_t ssaId,
        const VarIndexMap* vregIndexMaps, size_t regTypesNum, const cxuint* regRanges,
        cxuint& regType, size_t& vidx)
{
    regType = getRegType(regTypesNum, regRanges, svreg); // regtype
    const VarIndexMap& vregIndexMap = vregIndexMaps[regType];
    const std::vector<size_t>& vidxes = vregIndexMap.find(svreg)->second;
    /*ARDOut << "lvn[" << regType << "][" << vidxes[ssaId] << "]. ssaId: " <<
            ssaId << ". svreg: " << svreg.regVar << ":" << svreg.index << "\n";*/
    vidx = vidxes[ssaId];
}


static void addVIdxToCallEntry(size_t blockIndex, cxuint regType, size_t vidx,
            const std::vector<CodeBlock>& codeBlocks,
            std::unordered_map<size_t, VIdxSetEntry>& vidxCallMap,
            const std::unordered_map<size_t, VIdxSetEntry>& vidxRoutineMap)
{
    const CodeBlock& cblock = codeBlocks[blockIndex];
    if (cblock.haveCalls)
    {
        VIdxSetEntry& varCallEntry = vidxCallMap[blockIndex];
        for (const NextBlock& next: cblock.nexts)
            if (next.isCall)
            {
                const auto& allLvs = vidxRoutineMap.find(next.block)->second;
                if (allLvs.vs[regType].find(vidx) == allLvs.vs[regType].end())
                    // add callLiveTime only if vreg not present in routine
                    varCallEntry.vs[regType].insert(vidx);
            }
    }
}

static void fillUpInsideRoutine(std::unordered_set<size_t>& visited,
            const std::vector<CodeBlock>& codeBlocks,
            std::unordered_map<size_t, VIdxSetEntry>& vidxCallMap,
            const std::unordered_map<size_t, VIdxSetEntry>& vidxRoutineMap,
            size_t startBlock, const AsmSingleVReg& svreg, Liveness& lv,
            cxuint lvRType /* lv register type */, size_t vidx)
{
    std::deque<FlowStackEntry3> flowStack;
    flowStack.push_back({ startBlock, 0 });
    
    //if (lv.contain(codeBlocks[startBlock].end-1))
        // already filled, then do nothing
        //return;
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            if (visited.insert(entry.blockIndex).second)
            {
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
                
                addVIdxToCallEntry(entry.blockIndex, lvRType, vidx,
                        codeBlocks, vidxCallMap, vidxRoutineMap);
            }
            else
            {
                flowStack.pop_back();
                continue;
            }
        }
        
        // skip calls
        for (; entry.nextIndex < cblock.nexts.size() &&
                    cblock.nexts[entry.nextIndex].isCall; entry.nextIndex++);
        
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
            std::unordered_map<size_t, VIdxSetEntry>& vidxCallMap,
            const std::unordered_map<size_t, VIdxSetEntry>& vidxRoutineMap,
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
    cxuint lvRegType;
    size_t vidx;
    getVIdx2(svreg, ssaId, vregIndexMaps, regTypesNum, regRanges, lvRegType, vidx);
    Liveness& lv = livenesses[lvRegType][vidx];
    
    std::unordered_set<size_t> visited;
    
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
                        if (entry.lastAccessIndex == lastAccessIt->second.size())
                            doNextIndex = true;
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
            // fill up next block in path (do not fill start block)
            /* if inSubroutines, then first block
             * (that with subroutines calls) will be skipped */
            if (rjStack.size() > 1)
                fillUpInsideRoutine(visited, codeBlocks, vidxCallMap, vidxRoutineMap,
                        entry.blockIndex + (entry.inSubroutines), svreg,
                        lv, lvRegType, vidx);
            rjStack.pop();
        }
    }
    
    if (flitEnd != flowStack.begin())
    {
        const CodeBlock& cbLast = codeBlocks[(flitEnd-1)->blockIndex];
        if (lv.contain(cbLast.end-1))
            // if already filled up
            return;
    }
    
    auto flit = flowStack.begin() + flowStkStart.stackPos + (flowStkStart.inSubroutines);
    const CodeBlock& lastBlk = codeBlocks[flit->blockIndex];
    
    if (flit != flitEnd)
    {
        auto sinfoIt = lastBlk.ssaInfoMap.find(svreg);
        size_t lastPos = lastBlk.start;
        if (sinfoIt != lastBlk.ssaInfoMap.end())
        {
            if (flit->nextIndex > lastBlk.nexts.size())
                // only if pass this routine
                addVIdxToCallEntry(flit->blockIndex, lvRegType, vidx,
                        codeBlocks, vidxCallMap, vidxRoutineMap);
            // if begin at some point at last block
            lastPos = sinfoIt->second.lastPos;
            lv.insert(lastPos + 1, lastBlk.end);
            ++flit; // skip last block in stack
        }
    }
    // fill up to this
    for (; flit != flitEnd; ++flit)
    {
        const CodeBlock& cblock = codeBlocks[flit->blockIndex];
        if (flit->nextIndex > cblock.nexts.size())
            // only if pass this routine
            addVIdxToCallEntry(flit->blockIndex, lvRegType, vidx, codeBlocks,
                        vidxCallMap, vidxRoutineMap);
        lv.insert(cblock.start, cblock.end);
    }
}

/* handle many start points in this code (for example many kernel's in same code)
 * replace sets by vector, and sort and remove same values on demand
 */

/* join livenesses between consecutive code blocks */
static void putCrossBlockLivenesses(const std::deque<FlowStackEntry3>& flowStack,
        const std::vector<CodeBlock>& codeBlocks, const RoutineLvMap& routineMap,
        std::unordered_map<size_t, VIdxSetEntry>& vidxCallMap,
        const std::unordered_map<size_t, VIdxSetEntry>& vidxRoutineMap,
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
            
            joinVRegRecur(flowStack, codeBlocks, routineMap, vidxCallMap, vidxRoutineMap,
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
    std::unordered_set<size_t> visited;
    
    SVRegBlockMap alreadyReadMap;
    SVRegMap cacheSecPoints;
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (visited.insert(entry.blockIndex).second)
            {
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
            {
                // MSVC error fix
                auto& v = stackVarMap[sentry.first];
                v = LastVRegStackPos{ pfPos, true };
            }
        }
}

static void joinRegVarLivenesses(const std::deque<FlowStackEntry3>& prevFlowStack,
        const std::vector<CodeBlock>& codeBlocks, const RoutineLvMap& routineMap,
        std::unordered_map<size_t, VIdxSetEntry>& vidxCallMap,
        const std::unordered_map<size_t, VIdxSetEntry>& vidxRoutineMap,
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
        {
            // MSVC error fix
            auto& v = stackVarMap[sentry.first];
            v = { size_t(pfit - prevFlowStack.begin()), false };
        }
        
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
                                vidxCallMap, vidxRoutineMap,
                                stackPos, sentry.first, sentry.second.ssaIdBefore,
                                vregIndexMaps, livenesses, regTypesNum, regRanges, true);
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
                                vidxCallMap, vidxRoutineMap,
                                stackPos, rsentry.first, rsentry.second, vregIndexMaps,
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
            if (rvu.regVar == nullptr)
                continue;
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
            if (rvu.regVar == nullptr)
                continue;
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

static void joinRoutineDataLv(RoutineDataLv& dest, VIdxSetEntry& destVars,
            RoutineCurAccessMap& curSVRegMap, FlowStackEntry4& entry,
            const RoutineDataLv& src, const VIdxSetEntry& srcVars)
{
    dest.rbwSSAIdMap.insert(src.rbwSSAIdMap.begin(), src.rbwSSAIdMap.end());
    for (size_t i = 0; i < MAX_REGTYPES_NUM; i++)
        destVars.vs[i].insert(srcVars.vs[i].begin(), srcVars.vs[i].end());
    
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
        const RoutineLvMap& routineMap,
        const std::unordered_map<size_t, VIdxSetEntry>& vidxRoutineMap,
        RoutineDataLv& rdata, VIdxSetEntry& routineVIdxes,
        size_t routineBlock, const VarIndexMap* vregIndexMaps,
        size_t regTypesNum, const cxuint* regRanges)
{
    std::deque<FlowStackEntry4> flowStack;
    std::unordered_set<size_t> visited;
    
    // already read in current path
    // key - vreg, value - source block where vreg of conflict found
    SVRegMap alreadyReadMap;
    std::unordered_set<AsmSingleVReg> vregsNotInAllRets;
    std::unordered_set<AsmSingleVReg> vregsInFirstReturn;
    
    bool notFirstReturn = false;
    flowStack.push_back({ routineBlock, 0 });
    RoutineCurAccessMap curSVRegMap; // key - svreg, value - block index
    
    while (!flowStack.empty())
    {
        FlowStackEntry4& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (visited.insert(entry.blockIndex).second)
            {
                for (const auto& sentry: cblock.ssaInfoMap)
                {
                    if (sentry.second.readBeforeWrite)
                        if (alreadyReadMap.insert(
                                    { sentry.first, entry.blockIndex }).second)
                        {
                            auto res = rdata.rbwSSAIdMap.insert({ sentry.first,
                                        sentry.second.ssaIdBefore });
                            if (res.second && notFirstReturn)
                                // first readBeforeWrite and notFirstReturn
                                vregsNotInAllRets.insert(sentry.first);
                        }
                    
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
                    const std::vector<size_t>& vidxes =
                                vregIndexMap.find(svreg)->second;
                    
                    // add SSA indices to allSSAs
                    if (sinfo.readBeforeWrite)
                        routineVIdxes.vs[regType].insert(vidxes[sinfo.ssaIdBefore]);
                    if (sinfo.ssaIdChange != 0)
                    {
                        routineVIdxes.vs[regType].insert(vidxes[sinfo.ssaIdFirst]);
                        for (size_t i = 1; i < sinfo.ssaIdChange; i++)
                            routineVIdxes.vs[regType].insert(vidxes[sinfo.ssaId+i]);
                        routineVIdxes.vs[regType].insert(vidxes[sinfo.ssaIdLast]);
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
            {
                // update svregs 'not in all returns'
                const RoutineDataLv& srcRdata = routineMap.find(srcRoutBlock)->second;
                if (notFirstReturn)
                    for (const auto& vrentry: srcRdata.rbwSSAIdMap)
                        if (rdata.rbwSSAIdMap.find(vrentry.first)==rdata.rbwSSAIdMap.end())
                            vregsNotInAllRets.insert(vrentry.first);
                
                joinRoutineDataLv(rdata, routineVIdxes, curSVRegMap, entry,
                        srcRdata, vidxRoutineMap.find(srcRoutBlock)->second);
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
                    if (!notFirstReturn)
                        // fill up vregs for first return
                        vregsInFirstReturn.insert(entry.first);
                }
                if (notFirstReturn)
                    for (const AsmSingleVReg& vreg: vregsInFirstReturn)
                        if (curSVRegMap.find(vreg) == curSVRegMap.end())
                            // not found in this path then add to 'not in all paths'
                            vregsNotInAllRets.insert(vreg);
                
                notFirstReturn = true;
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
    
    // handle unused svregs in this path (from routine start) and
    // used other paths and have this same ssaId as at routine start.
    // just add to lastAccessMap svreg for start to join through all routine
    for (const AsmSingleVReg& svreg: vregsNotInAllRets)
    {
        auto res = rdata.lastAccessMap.insert({ svreg, { { routineBlock, false } } });
        if (!res.second)
        {
            VectorSet<LastAccessBlockPos>& sset = res.first->second;
            // filter before inserting (remove everything that do not point to calls)
            sset.resize(std::remove_if(sset.begin(), sset.end(),
                [](const LastAccessBlockPos& b)
                { return !b.inSubroutines; }) - sset.begin());
            sset.insertValue({ routineBlock, false });
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
            std::vector<size_t>& vidxes = vregIndices[entry.first];
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
            
            if (vidxes.size() < ssaIdCount)
                vidxes.resize(ssaIdCount, SIZE_MAX);
            
            // set liveness index to vidxes
            if (sinfo.readBeforeWrite)
            {
                if (vidxes[sinfo.ssaIdBefore] == SIZE_MAX)
                    vidxes[sinfo.ssaIdBefore] = graphVregsCount++;
            }
            if (sinfo.ssaIdChange!=0)
            {
                // fill up vidxes (with graph Ids)
                if (vidxes[sinfo.ssaIdFirst] == SIZE_MAX)
                    vidxes[sinfo.ssaIdFirst] = graphVregsCount++;
                for (size_t ssaId = sinfo.ssaId+1;
                        ssaId < sinfo.ssaId+sinfo.ssaIdChange-1; ssaId++)
                    vidxes[ssaId] = graphVregsCount++;
                if (vidxes[sinfo.ssaIdLast] == SIZE_MAX)
                    vidxes[sinfo.ssaIdLast] = graphVregsCount++;
            }
            // if not readBeforeWrite and neither ssaIdChanges but it is write to
            // normal register
            if (entry.first.regVar==nullptr && vidxes[0] == SIZE_MAX)
                vidxes[0] = graphVregsCount++;
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
    
    for (size_t i = 0; i < regTypesNum; i++)
        livenesses[i].resize(graphVregsCounts[i]);
    
    // callLiveTime - call live time where routine will be called
    // reverse counted, begin from SIZE_MAX, used for joining svreg from routines
    // and svreg used in this time
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
                            vidxCallMap, vidxRoutineMap, lastVRegMap,
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
                            if (svreg.regVar != nullptr)
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
            const size_t routineBlock = callStack.back().routineBlock;
            auto res = routineMap.insert({ routineBlock, { } });
            
            if (res.second)
            {
                auto varRes = vidxRoutineMap.insert({ routineBlock, VIdxSetEntry{} });
                createRoutineDataLv(codeBlocks, routineMap, vidxRoutineMap,
                        res.first->second, varRes.first->second,
                        routineBlock, vregIndexMaps, regTypesNum, regRanges);
            }
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
                                  vidxCallMap, vidxRoutineMap,
                            flowStackStart, entry.first, entry.second, vregIndexMaps,
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
                joinRegVarLivenesses(flowStack, codeBlocks, routineMap,
                        vidxCallMap, vidxRoutineMap,
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

