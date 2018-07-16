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

namespace CLRX
{

struct CLRX_INTERNAL LivenessState
{
    const std::deque<FlowStackEntry3>& flowStack;
    const std::vector<CodeBlock>& codeBlocks;
    std::vector<bool>& waysToCache;
    ResSecondPointsToCache& cblocksToCache;
    PrevWaysIndexMap& prevWaysIndexMap;
    std::vector<Liveness>* livenesses;
    const VarIndexMap* vregIndexMaps;
    std::unordered_map<size_t, VIdxSetEntry>& vidxCallMap;
    std::unordered_map<size_t, VIdxSetEntry>& vidxRoutineMap;
    RoutineLvMap& routineMap;
    size_t regTypesNum;
    const cxuint* regRanges;
};

};

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
        const AsmRegAllocator::SSAInfo& ssaInfo, LivenessState& ls,
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
    
    regType = getRegType(ls.regTypesNum, ls.regRanges, svreg); // regtype
    const VarIndexMap& vregIndexMap = ls.vregIndexMaps[regType];
    const std::vector<size_t>& vidxes = vregIndexMap.find(svreg)->second;
    /*ARDOut << "lvn[" << regType << "][" << vidxes[ssaId] << "]. ssaIdIdx: " <<
            ssaIdIdx << ". ssaId: " << ssaId << ". svreg: " << svreg.regVar << ":" <<
            svreg.index << "\n";*/
    //return livenesses[regType][ssaIdIndices[ssaId]];
    vidx = vidxes[ssaId];
}

static inline Liveness& getLiveness(const AsmSingleVReg& svreg, size_t ssaIdIdx,
        const AsmRegAllocator::SSAInfo& ssaInfo, LivenessState& ls)
{
    cxuint regType;
    size_t vidx;
    getVIdx(svreg, ssaIdIdx, ssaInfo, ls, regType, vidx);
    return ls.livenesses[regType][vidx];
}

static void getVIdx2(const AsmSingleVReg& svreg, size_t ssaId, LivenessState& ls,
        cxuint& regType, size_t& vidx)
{
    regType = getRegType(ls.regTypesNum, ls.regRanges, svreg); // regtype
    const VarIndexMap& vregIndexMap = ls.vregIndexMaps[regType];
    const std::vector<size_t>& vidxes = vregIndexMap.find(svreg)->second;
    /*ARDOut << "lvn[" << regType << "][" << vidxes[ssaId] << "]. ssaId: " <<
            ssaId << ". svreg: " << svreg.regVar << ":" << svreg.index << "\n";*/
    vidx = vidxes[ssaId];
}


static void addVIdxToCallEntry(size_t blockIndex, cxuint regType, size_t vidx,
                LivenessState& ls)
{
    const CodeBlock& cblock = ls.codeBlocks[blockIndex];
    if (cblock.haveCalls)
    {
        VIdxSetEntry& varCallEntry = ls.vidxCallMap[blockIndex];
        for (const NextBlock& next: cblock.nexts)
            if (next.isCall)
            {
                auto vidxRIt = ls.vidxRoutineMap.find(next.block);
                if (vidxRIt == ls.vidxRoutineMap.end())
                    continue;
                const auto& allLvs = vidxRIt->second;
                if (allLvs.vs[regType].find(vidx) == allLvs.vs[regType].end())
                    // add callLiveTime only if vreg not present in routine
                    varCallEntry.vs[regType].insert(vidx);
            }
    }
}

static void fillUpInsideRoutine(LivenessState& ls,
            size_t startBlock, size_t routineBlock, const AsmSingleVReg& svreg,
            Liveness& lv, cxuint lvRType /* lv register type */, size_t vidx,
            size_t ssaId, const RoutineDataLv& rdata,
            std::unordered_set<size_t>& havePathBlocks)
{
    const std::unordered_set<size_t>& haveReturnBlocks = rdata.haveReturnBlocks;
    std::deque<FlowStackEntry3> flowStack;
    std::unordered_set<size_t> visited;
    
    flowStack.push_back({ startBlock, 0 });
    
    size_t startSSAId = ssaId;
    bool fromStartPos = false;
    // determine first SSAId and whether filling should begin from start of the routine
    if (routineBlock == startBlock)
    {
        const CodeBlock& cblock = ls.codeBlocks[startBlock];
        auto sinfoIt = binaryMapFind(cblock.ssaInfoMap.begin(),
                cblock.ssaInfoMap.end(), svreg);
        
        auto rbwIt = rdata.rbwSSAIdMap.find(svreg);
        if (sinfoIt != cblock.ssaInfoMap.end())
        {
            const SSAInfo& sinfo = sinfoIt->second;
            if (sinfo.readBeforeWrite && sinfo.ssaIdBefore==ssaId)
                // if ssaId is first read, then we assume start at routine start pos
                fromStartPos = true;
            else if (sinfo.ssaIdChange == 0 || sinfo.ssaIdLast != ssaId)
                // do nothing (last SSAId doesnt match or no change
                return;
        }
        else if (rbwIt != rdata.rbwSSAIdMap.end())
        {
            fromStartPos = true;
            startSSAId = rbwIt->second;
        }
        else
            return; // do nothing
    }
    if (startSSAId != ssaId)
        return; // also, do nothing
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        const CodeBlock& cblock = ls.codeBlocks[entry.blockIndex.index];
        
        bool endOfPath = false;
        if (entry.nextIndex == 0)
        {
            if (visited.insert(entry.blockIndex.index).second &&
                haveReturnBlocks.find(entry.blockIndex.index) != haveReturnBlocks.end())
            {
                auto sinfoIt = binaryMapFind(cblock.ssaInfoMap.begin(),
                            cblock.ssaInfoMap.end(), svreg);
                if (flowStack.size() > 1 && sinfoIt != cblock.ssaInfoMap.end())
                {
                    if (!sinfoIt->second.readBeforeWrite)
                    {
                        // no read before write skip this path
                        flowStack.pop_back();
                        continue;
                    }
                    else
                        // we end path at first read
                        endOfPath = true;
                }
            }
            else
            {
                const bool prevPath = havePathBlocks.find(entry.blockIndex.index) !=
                        havePathBlocks.end();
                flowStack.pop_back();
                // propagate haveReturn to previous block
                if (prevPath)
                {
                    flowStack.back().havePath = true;
                    havePathBlocks.insert(flowStack.back().blockIndex.index);
                }
                continue;
            }
        }
        
        // skip calls
        if (!endOfPath)
            for (; entry.nextIndex < cblock.nexts.size() &&
                    cblock.nexts[entry.nextIndex].isCall; entry.nextIndex++);
        
        if (!endOfPath && entry.nextIndex < cblock.nexts.size())
        {
            flowStack.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (!endOfPath &&
            ((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            flowStack.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else
        {
            if (endOfPath || cblock.haveReturn)
            {
                havePathBlocks.insert(entry.blockIndex.index);
                // we have path only if have return and if have path
                entry.havePath = true;
            }
            
            const bool curHavePath = entry.havePath;
            if (curHavePath)
            {
                // fill up block when in path
                auto sinfoIt = binaryMapFind(cblock.ssaInfoMap.begin(),
                            cblock.ssaInfoMap.end(), svreg);
                size_t cbStart = cblock.start;
                size_t cbEnd = cblock.end;
                if (flowStack.size() == 1 && !fromStartPos)
                {
                    // if first block, then get last occurrence in this path
                    if (sinfoIt != cblock.ssaInfoMap.end())
                        cbStart = sinfoIt->second.lastPos+1;
                }
                if (endOfPath && sinfoIt != cblock.ssaInfoMap.end())
                    cbEnd = sinfoIt->second.firstPos+1;
                // fill up block
                lv.insert(cbStart, cbEnd);
                if (cblock.end == cbEnd)
                    addVIdxToCallEntry(entry.blockIndex.index, lvRType, vidx, ls);
            }
            
            // back
            flowStack.pop_back();
            // propagate havePath
            if (!flowStack.empty())
            {
                if (curHavePath)
                    havePathBlocks.insert(flowStack.back().blockIndex.index);
                flowStack.back().havePath |= curHavePath;
            }
        }
    }
}

static void joinVRegRecur(LivenessState& ls, LastVRegStackPos flowStkStart,
            const AsmSingleVReg& svreg, size_t ssaId, bool skipLastBlock = false)
{
    struct JoinEntry
    {
        size_t blockIndex; // block index where is call
        size_t nextIndex; // next index of routine call
        size_t lastAccessIndex; // last access pos index
        bool inSubroutines;
        size_t routineBlock;
        const RoutineDataLv* rdata;
    };
    
    std::unordered_set<LastAccessBlockPos> visited;
    std::unordered_set<size_t> havePathBlocks;
    
    FlowStackCIter flitEnd = ls.flowStack.end();
    if (skipLastBlock)
        --flitEnd; // before last element
    cxuint lvRegType;
    size_t vidx;
    getVIdx2(svreg, ssaId, ls, lvRegType, vidx);
    Liveness& lv = ls.livenesses[lvRegType][vidx];
    
    std::stack<JoinEntry> rjStack; // routine join stack
    if (flowStkStart.inSubroutines)
        rjStack.push({ ls.flowStack[flowStkStart.stackPos].blockIndex.index,
                            0, 0, true, SIZE_MAX, nullptr });
    
    while (!rjStack.empty())
    {
        JoinEntry& entry = rjStack.top();
        const CodeBlock& cblock = ls.codeBlocks[entry.blockIndex];
        
        if (entry.inSubroutines && entry.nextIndex < cblock.nexts.size())
        {
            bool doNextIndex = false; // true if to next call
            if (cblock.nexts[entry.nextIndex].isCall)
            {
                const size_t routineBlock = cblock.nexts[entry.nextIndex].block;
                const RoutineDataLv& rdata =
                        ls.routineMap.find(routineBlock)->second;
                auto lastAccessIt = rdata.lastAccessMap.find(svreg);
                if (lastAccessIt != rdata.lastAccessMap.end())
                {
                    if (entry.lastAccessIndex < lastAccessIt->second.size())
                    {
                        // we have new path in subroutine to fill
                        const auto& lastAccess =
                                lastAccessIt->second[entry.lastAccessIndex];
                        
                        if (visited.insert(lastAccess).second)
                            rjStack.push({ lastAccess.blockIndex, 0, 0,
                                    lastAccess.inSubroutines, routineBlock, &rdata });
                        
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
                fillUpInsideRoutine(ls, entry.blockIndex + (entry.inSubroutines),
                        entry.routineBlock, svreg, lv, lvRegType, vidx, ssaId,
                        *entry.rdata, havePathBlocks);
            rjStack.pop();
        }
    }
    
    auto flit = ls.flowStack.begin() + flowStkStart.stackPos;
    // applyVIdx to call entry (if needed, and if some join inside subroutines)
    // resolve this vidx before these call point and if nexts>1
    if (flowStkStart.inSubroutines &&
        lv.contain(ls.codeBlocks[flit->blockIndex.index].end-1) &&
        ls.codeBlocks[flit->blockIndex.index].nexts.size() > 1)
        // just apply, only if join before call
        addVIdxToCallEntry(flit->blockIndex.index, lvRegType, vidx, ls);
    
    if (flowStkStart.inSubroutines)
        ++flit; // skip this codeblock before call
    
    const CodeBlock& lastBlk = ls.codeBlocks[flit->blockIndex.index];
    if (flit != flitEnd)
    {
        auto sinfoIt = binaryMapFind(lastBlk.ssaInfoMap.begin(),
                    lastBlk.ssaInfoMap.end(), svreg);
        size_t lastPos = lastBlk.start;
        if (sinfoIt != lastBlk.ssaInfoMap.end())
        {
            if (flit->nextIndex > lastBlk.nexts.size())
                // only if pass this routine
                addVIdxToCallEntry(flit->blockIndex.index, lvRegType, vidx, ls);
            // if begin at some point at last block
            lastPos = sinfoIt->second.lastPos;
            lv.insert(lastPos + 1, lastBlk.end);
            ++flit; // skip last block in stack
        }
    }
    // fill up to this
    for (; flit != flitEnd; ++flit)
    {
        const CodeBlock& cblock = ls.codeBlocks[flit->blockIndex.index];
        if (flit->nextIndex > cblock.nexts.size())
            // only if pass this routine
            addVIdxToCallEntry(flit->blockIndex.index, lvRegType, vidx, ls);
        lv.insert(cblock.start, cblock.end);
    }
}

/* handle many start points in this code (for example many kernel's in same code)
 * replace sets by vector, and sort and remove same values on demand
 */

/* join livenesses between consecutive code blocks */
static void putCrossBlockLivenesses(LivenessState& ls, const LastVRegMap& lastVRegMap)
{
    ARDOut << "putCrossBlockLv block: " << ls.flowStack.back().blockIndex << "\n";
    const CodeBlock& cblock = ls.codeBlocks[ls.flowStack.back().blockIndex.index];
    for (const auto& entry: cblock.ssaInfoMap)
        if (entry.second.readBeforeWrite)
        {
            // find last
            auto lvrit = lastVRegMap.find(entry.first);
            LastVRegStackPos flowStackStart = (lvrit != lastVRegMap.end()) ?
                lvrit->second.back() : LastVRegStackPos{ 0, false };
            
            joinVRegRecur(ls, flowStackStart, entry.first, entry.second.ssaIdBefore, true);
        }
}

// add new join second cache entry with readBeforeWrite for all encountered regvars
static void addJoinSecCacheEntry(LivenessState& ls,
                SimpleCache<size_t, SVRegMap>& joinSecondPointsCache,
                size_t nextBlock)
{
    ARDOut << "addJoinSecCacheEntry: " << nextBlock << "\n";
    //std::stack<CallStackEntry> callStack = prevCallStack;
    // traverse by graph from next block
    std::deque<FlowStackEntry3> flowStack;
    flowStack.push_back({ nextBlock, 0 });
    std::unordered_set<size_t> visited;
    
    // already read in current path
    // key - vreg, value - source block where vreg of conflict found
    SVRegBlockMap alreadyReadMap;
    SVRegMap cacheSecPoints;
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        const CodeBlock& cblock = ls.codeBlocks[entry.blockIndex.index];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (visited.insert(entry.blockIndex.index).second)
            {
                ARDOut << "  resolv (cache): " << entry.blockIndex << "\n";
                
                const SVRegMap* resSecondPoints =
                            joinSecondPointsCache.use(entry.blockIndex.index);
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
            // add alreadyReadMap ssaIds inside called routines
            for (const auto& next: cblock.nexts)
                if (next.isCall)
                {
                    const RoutineDataLv& rdata = ls.routineMap.find(next.block)->second;
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
                    const RoutineDataLv& rdata = ls.routineMap.find(next.block)->second;
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

static void joinRegVarLivenesses(LivenessState& ls,
        SimpleCache<size_t, LastStackPosMap>& joinFirstPointsCache,
        SimpleCache<size_t, SVRegMap>& joinSecondPointsCache)
{
    size_t nextBlock = ls.flowStack.back().blockIndex.index;
    auto pfEnd = ls.flowStack.end();
    --pfEnd;
    ARDOut << "startJoinLv: " << (pfEnd-1)->blockIndex << "," << nextBlock << "\n";
    // key - varreg, value - last position in previous flowStack
    LastStackPosMap stackVarMap;
    
    size_t pfStartIndex = 0;
    {
        auto pfPrev = pfEnd;
        --pfPrev;
        auto it = ls.prevWaysIndexMap.find(pfPrev->blockIndex.index);
        if (it != ls.prevWaysIndexMap.end())
        {
            const LastStackPosMap* cached = joinFirstPointsCache.use(it->second.first);
            if (cached!=nullptr)
            {
                ARDOut << "use pfcached: " << it->second.first << ", " <<
                        it->second.second << "\n";
                stackVarMap = *cached;
                pfStartIndex = it->second.second+1;
                
                // apply missing calls at end of the cached
                const CodeBlock& cblock = ls.codeBlocks[it->second.first];
                
                const FlowStackEntry3& entry = *(ls.flowStack.begin()+pfStartIndex-1);
                if (entry.nextIndex > cblock.nexts.size())
                    applyCallToStackVarMap(cblock, ls.routineMap, stackVarMap,
                                    pfStartIndex-1, -1);
            }
        }
    }
    
    // collect previous svreg from current path
    for (auto pfit = ls.flowStack.begin()+pfStartIndex; pfit != pfEnd; ++pfit)
    {
        const FlowStackEntry3& entry = *pfit;
        const CodeBlock& cblock = ls.codeBlocks[entry.blockIndex.index];
        for (const auto& sentry: cblock.ssaInfoMap)
        {
            // MSVC error fix
            auto& v = stackVarMap[sentry.first];
            v = { size_t(pfit - ls.flowStack.begin()), false };
        }
        
        if (entry.nextIndex > cblock.nexts.size())
            applyCallToStackVarMap(cblock, ls.routineMap, stackVarMap,
                        pfit - ls.flowStack.begin(), entry.nextIndex);
        
        // put to first point cache
        if (ls.waysToCache[pfit->blockIndex.index] &&
            !joinFirstPointsCache.hasKey(pfit->blockIndex.index))
        {
            ARDOut << "put pfcache " << pfit->blockIndex << "\n";
            joinFirstPointsCache.put(pfit->blockIndex.index, stackVarMap);
        }
    }
    
    SVRegMap cacheSecPoints;
    const bool toCache = (!joinSecondPointsCache.hasKey(nextBlock)) &&
                ls.cblocksToCache.count(nextBlock)>=2;
    
    // traverse by graph from next block
    std::deque<FlowStackEntry3> flowStack;
    flowStack.push_back({ nextBlock, 0 });
    std::vector<bool> visited(ls.codeBlocks.size(), false);
    
    // already read in current path
    // key - vreg, value - source block where vreg of conflict found
    SVRegMap alreadyReadMap;
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        const CodeBlock& cblock = ls.codeBlocks[entry.blockIndex.index];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex.index])
            {
                visited[entry.blockIndex.index] = true;
                ARDOut << "  lvjoin: " << entry.blockIndex << "\n";
                
                const SVRegMap* joinSecondPoints =
                        joinSecondPointsCache.use(entry.blockIndex.index);
                
                if (joinSecondPoints == nullptr)
                    for (const auto& sentry: cblock.ssaInfoMap)
                    {
                        const SSAInfo& sinfo = sentry.second;
                        auto res = alreadyReadMap.insert(
                                    { sentry.first, entry.blockIndex.index });
                        
                        if (toCache)
                            cacheSecPoints[sentry.first] = sinfo.ssaIdBefore;
                        
                        if (res.second && sinfo.readBeforeWrite)
                        {
                            auto it = stackVarMap.find(sentry.first);
                            LastVRegStackPos stackPos = (it != stackVarMap.end() ?
                                        it->second : LastVRegStackPos{ 0, false });
                            
                            joinVRegRecur(ls, stackPos, sentry.first,
                                sentry.second.ssaIdBefore, true);
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
                            
                            joinVRegRecur(ls, stackPos, rsentry.first,
                                          rsentry.second, true);
                        }
                    }
                    flowStack.pop_back();
                    continue;
                }
            }
            else
            {
                ls.cblocksToCache.increase(entry.blockIndex);
                ARDOut << "jcblockToCache: " << entry.blockIndex << "=" <<
                            ls.cblocksToCache.count(entry.blockIndex) << "\n";
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
            // add alreadReadMap ssaIds inside called routines
            for (const auto& next: cblock.nexts)
                if (next.isCall)
                {
                    const RoutineDataLv& rdata = ls.routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                        alreadyReadMap.insert({v.first, entry.blockIndex.index });
                    for (const auto& v: rdata.lastAccessMap)
                        alreadyReadMap.insert({v.first, entry.blockIndex.index });
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
                    const RoutineDataLv& rdata = ls.routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                    {
                        auto it = alreadyReadMap.find(v.first);
                        if (it != alreadyReadMap.end() &&
                            it->second == entry.blockIndex.index)
                            alreadyReadMap.erase(it);
                    }
                    for (const auto& v: rdata.lastAccessMap)
                    {
                        auto it = alreadyReadMap.find(v.first);
                        if (it != alreadyReadMap.end() &&
                            it->second == entry.blockIndex.index)
                            alreadyReadMap.erase(it);
                    }
                }
            
            for (const auto& sentry: cblock.ssaInfoMap)
            {
                auto it = alreadyReadMap.find(sentry.first);
                if (it != alreadyReadMap.end() && it->second == entry.blockIndex.index)
                    // remove old to resolve in leaved way to allow collecting next ssaId
                    // before write (can be different due to earlier visit)
                    alreadyReadMap.erase(it);
            }
            ARDOut << "  popjoin\n";
            
            if (ls.cblocksToCache.count(entry.blockIndex)==2 &&
                !joinSecondPointsCache.hasKey(entry.blockIndex.index))
                // add to cache
                addJoinSecCacheEntry(ls, joinSecondPointsCache, entry.blockIndex.index);
            
            flowStack.pop_back();
        }
    }
    
    if (toCache)
        joinSecondPointsCache.put(nextBlock, cacheSecPoints);
}

static bool addUsageDeps(const cxbyte* ldeps, const std::vector<AsmRegVarUsage>& rvus,
            const std::vector<AsmRegVarLinearDep>& instrLinDeps, LinearDepMap* ldepsOut,
            Array<std::pair<AsmSingleVReg, SSAInfo> >& ssaInfoMap,
            const SVRegMap& ssaIdIdxMap, const std::vector<AsmSingleVReg>& readSVRegs,
            const std::vector<AsmSingleVReg>& writtenSVRegs, LivenessState& ls)
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
                auto ssaIdIdx = ssaIdIdxMap.find(svreg)->second;
                const SSAInfo& ssaInfo = binaryMapFind(ssaInfoMap.begin(),
                                ssaInfoMap.end(), svreg)->second;
                size_t outVIdx;
                
                // if read or read-write (but not same write)
                if (checkNoWriteWithSSA(rvu) &&
                    std::find(writtenSVRegs.begin(), writtenSVRegs.end(),
                              svreg) != writtenSVRegs.end())
                    ssaIdIdx--; // current ssaIdIdx is for write, decrement
                
                getVIdx(svreg, ssaIdIdx, ssaInfo, ls, regType, outVIdx);
                // push variable index
                vidxes.push_back(outVIdx);
            }
        }
        ldepsOut[regType][vidxes[0]].align =
                std::max(ldepsOut[regType][vidxes[0]].align, align);
        for (size_t k = 1; k < vidxes.size(); k++)
        {
            ldepsOut[regType][vidxes[k-1]].nextVidxes.insertValue(vidxes[k]);
            ldepsOut[regType][vidxes[k]].prevVidxes.insertValue(vidxes[k-1]);
        }
    }
    // add single arg linear dependencies
    for (cxuint i = 0; i < rvus.size(); i++)
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
                size_t ssaIdIdx = ssaIdIdxMap.find(svreg)->second;
                const SSAInfo& ssaInfo = binaryMapFind(ssaInfoMap.begin(),
                            ssaInfoMap.end(), svreg)->second;
                size_t outVIdx;
                
                // if read or read-write (but not same write)
                if (checkNoWriteWithSSA(rvu) &&
                    std::find(writtenSVRegs.begin(), writtenSVRegs.end(),
                              svreg) != writtenSVRegs.end())
                    ssaIdIdx--; // current ssaIdIdx is for write, decrement
                
                getVIdx(svreg, ssaIdIdx, ssaInfo, ls, regType, outVIdx);
                // push variable index
                vidxes.push_back(outVIdx);
            }
            ldepsOut[regType][vidxes[0]].align =
                std::max(ldepsOut[regType][vidxes[0]].align, align);
            for (size_t j = 1; j < vidxes.size(); j++)
            {
                ldepsOut[regType][vidxes[j-1]].nextVidxes.insertValue(vidxes[j]);
                ldepsOut[regType][vidxes[j]].prevVidxes.insertValue(vidxes[j-1]);
            }
        }
        
    // handle user defined linear dependencies
    for (const AsmRegVarLinearDep& ldep: instrLinDeps)
    {
        std::vector<size_t> vidxes, prevVidxes;
        cxuint regType = UINT_MAX;
        bool haveReadVidxes = false;
        for (uint16_t k = ldep.rstart; k < ldep.rend; k++)
        {
            AsmSingleVReg svreg = {ldep.regVar, k};
            auto ssaIdxIdIt = ssaIdIdxMap.find(svreg);
            auto ssaInfoIt = binaryMapFind(ssaInfoMap.begin(), ssaInfoMap.end(), svreg);
            if (ssaIdxIdIt == ssaIdIdxMap.end() || ssaInfoIt == ssaInfoMap.end())
                return false; // failed
            
            size_t ssaIdIdx = ssaIdxIdIt->second;
            const SSAInfo& ssaInfo = ssaInfoIt->second;
            size_t outVIdx;
            
            // if read or read-write (but not same write)
            if (std::find(readSVRegs.begin(), readSVRegs.end(),
                            svreg) != readSVRegs.end() &&
                std::find(writtenSVRegs.begin(), writtenSVRegs.end(),
                            svreg) != writtenSVRegs.end())
            {
                getVIdx(svreg, ssaIdIdx-1, ssaInfo, ls, regType, outVIdx);
                // push variable index
                prevVidxes.push_back(outVIdx);
                haveReadVidxes = true;
            }
            
            getVIdx(svreg, ssaIdIdx, ssaInfo, ls, regType, outVIdx);
            // push variable index
            vidxes.push_back(outVIdx);
            if(vidxes.size() != prevVidxes.size())
                // if prevVidxes not filled
                prevVidxes.push_back(outVIdx);
        }
        ldepsOut[regType][vidxes[0]].align = 
            std::max(ldepsOut[regType][vidxes[0]].align, cxbyte(0));
        for (size_t j = 1; j < vidxes.size(); j++)
        {
            ldepsOut[regType][vidxes[j-1]].nextVidxes.insertValue(vidxes[j]);
            ldepsOut[regType][vidxes[j]].prevVidxes.insertValue(vidxes[j-1]);
        }
        
        // add from linear deps for read vidxes (prevVidxes)
        if (haveReadVidxes)
        {
            ldepsOut[regType][prevVidxes[0]].align = 
                std::max(ldepsOut[regType][prevVidxes[0]].align, cxbyte(0));
            for (size_t j = 1; j < prevVidxes.size(); j++)
            {
                ldepsOut[regType][prevVidxes[j-1]].nextVidxes.insertValue(prevVidxes[j]);
                ldepsOut[regType][prevVidxes[j]].prevVidxes.insertValue(prevVidxes[j-1]);
            }
        }
    }
    return true;
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

static void createRoutineDataLv(LivenessState& ls, RoutineDataLv& rdata,
        VIdxSetEntry& routineVIdxes, size_t routineBlock)
{
    ARDOut << "--------- createRoutineDataLv(" << routineBlock << ")\n";
    std::deque<FlowStackEntry4> flowStack;
    std::unordered_set<size_t> visited;
    
    // already read in current path
    // key - vreg, value - source block where vreg of conflict found
    SVRegMap alreadyReadMap;
    std::unordered_set<AsmSingleVReg> vregsNotInAllRets;
    std::unordered_set<AsmSingleVReg> vregsInFirstReturn;
    std::unordered_set<size_t>& haveReturnBlocks = rdata.haveReturnBlocks;
    
    bool notFirstReturn = false;
    flowStack.push_back({ routineBlock, 0 });
    RoutineCurAccessMap curSVRegMap; // key - svreg, value - block index
    
    while (!flowStack.empty())
    {
        FlowStackEntry4& entry = flowStack.back();
        const CodeBlock& cblock = ls.codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (visited.insert(entry.blockIndex).second)
            {
                ARDOut << "  cpjproc: " << entry.blockIndex << "\n";
                
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
                    cxuint regType = getRegType(ls.regTypesNum, ls.regRanges, svreg);
                    const VarIndexMap& vregIndexMap = ls.vregIndexMaps[regType];
                    const std::vector<size_t>& vidxes =
                                vregIndexMap.find(svreg)->second;
                    
                    // add SSA indices to allSSAs
                    if (sinfo.readBeforeWrite)
                        routineVIdxes.vs[regType].insert(vidxes[sinfo.ssaIdBefore]);
                    if (sinfo.ssaIdChange != 0)
                    {
                        routineVIdxes.vs[regType].insert(vidxes[sinfo.ssaIdFirst]);
                        for (size_t i = 1; i < sinfo.ssaIdChange-1; i++)
                            routineVIdxes.vs[regType].insert(vidxes[sinfo.ssaId+i]);
                        routineVIdxes.vs[regType].insert(vidxes[sinfo.ssaIdLast]);
                    }
                }
            }
            else
            {
                const bool prevReturn = haveReturnBlocks.find(entry.blockIndex) !=
                        haveReturnBlocks.end();
                flowStack.pop_back();
                // propagate haveReturn to previous block
                if (prevReturn)
                {
                    flowStack.back().haveReturn = true;
                    haveReturnBlocks.insert(flowStack.back().blockIndex);
                }
                continue;
            }
        }
        
        // join and skip calls
        {
            std::vector<size_t> calledRoutines;
            for (; entry.nextIndex < cblock.nexts.size() &&
                        cblock.nexts[entry.nextIndex].isCall; entry.nextIndex++)
            {
                BlockIndex rblock = cblock.nexts[entry.nextIndex].block;
                if (rblock != routineBlock /*&&
                    recurseBlocks.find(rblock.index) == recurseBlocks.end()*/)
                    calledRoutines.push_back(rblock.index);
            }
            
            for (size_t srcRoutBlock: calledRoutines)
            {
                auto srcRIt = ls.routineMap.find(srcRoutBlock);
                if (srcRIt == ls.routineMap.end())
                    continue; // skip not initialized recursion
                // update svregs 'not in all returns'
                const RoutineDataLv& srcRdata = srcRIt->second;
                if (notFirstReturn)
                    for (const auto& vrentry: srcRdata.rbwSSAIdMap)
                        if (rdata.rbwSSAIdMap.find(vrentry.first)==rdata.rbwSSAIdMap.end())
                            vregsNotInAllRets.insert(vrentry.first);
                
                joinRoutineDataLv(rdata, routineVIdxes, curSVRegMap, entry,
                        srcRdata, ls.vidxRoutineMap.find(srcRoutBlock)->second);
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
                }
                
                entry.haveReturn = true;
                haveReturnBlocks.insert(entry.blockIndex);
            }
            
            // handle vidxes not in all paths (both end and returns)
            if (cblock.haveReturn || cblock.haveEnd)
            {
                for (const auto& entry: curSVRegMap)
                    if (!notFirstReturn)
                        // fill up vregs for first return
                        vregsInFirstReturn.insert(entry.first);
                if (notFirstReturn && cblock.haveReturn)
                    for (const AsmSingleVReg& vreg: vregsInFirstReturn)
                        if (curSVRegMap.find(vreg) == curSVRegMap.end())
                            // not found in this path then add to 'not in all paths'
                            vregsNotInAllRets.insert(vreg);
                notFirstReturn = true;
            }
            
            const bool curHaveReturn = entry.haveReturn;
            
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
            // set up haveReturn
            if (!flowStack.empty())
            {
                flowStack.back().haveReturn |= curHaveReturn;
                if (flowStack.back().haveReturn)
                    haveReturnBlocks.insert(flowStack.back().blockIndex);
            }
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

void AsmRegAllocator::createLivenesses(ISAUsageHandler& usageHandler,
                ISALinearDepHandler& linDepHandler)
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
    std::deque<CallStackEntry> callStack;
    std::deque<FlowStackEntry3> flowStack;
    CBlockBitPool visited(codeBlocks.size(), false);
    // hold last vreg ssaId and position
    LastVRegMap lastVRegMap;
    
    // key - current res first key, value - previous first key and its flowStack pos
    PrevWaysIndexMap prevWaysIndexMap;
    // to track ways last block indices pair: block index, flowStackPos)
    std::pair<size_t, size_t> lastCommonCacheWayPoint{ SIZE_MAX, SIZE_MAX };
    std::vector<bool> waysToCache(codeBlocks.size(), false);
    ResSecondPointsToCache cblocksToCache(codeBlocks.size());
    std::unordered_set<BlockIndex> callBlocks;
    std::unordered_set<size_t> recurseBlocks;
    
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
    
    // structure to pass many arguments in compact pack
    LivenessState ls = { flowStack, codeBlocks, waysToCache, cblocksToCache,
        prevWaysIndexMap, livenesses, vregIndexMaps, vidxCallMap, vidxRoutineMap,
        routineMap, regTypesNum, regRanges };
    
    const size_t linearDepSize = linDepHandler.size();
    
    while (!flowStack.empty())
    {
        FlowStackEntry3& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex.index];
        
        if (entry.nextIndex == 0)
        {
            curLiveTime = cblock.start;
            // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                ARDOut << "joinpush: " << entry.blockIndex << "\n";
                if (flowStack.size() > 1)
                    putCrossBlockLivenesses(ls, lastVRegMap);
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
                std::vector<AsmRegVarUsage> instrRVUs;
                
                std::vector<AsmSingleVReg> readSVRegs;
                std::vector<AsmSingleVReg> writtenSVRegs;
                
                ISAUsageHandler::ReadPos usagePos = cblock.usagePos;
                size_t oldOffset = usageHandler.hasNext(usagePos) ?
                        cblock.start : cblock.end;
                
                size_t linearDepPos = linDepHandler.findPositionByOffset(cblock.start);
                
                // register in liveness
                bool rvuFirst = true;
                while (true)
                {
                    AsmRegVarUsage rvu = { 0U, nullptr, 0U, 0U };
                    bool hasNext = false;
                    if (usageHandler.hasNext(usagePos) && oldOffset < cblock.end)
                    {
                        hasNext = true;
                        rvu = usageHandler.nextUsage(usagePos);
                        if (rvuFirst)
                        {
                            oldOffset = rvu.offset;
                            rvuFirst = false;
                        }
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
                                    binaryMapFind(cblock.ssaInfoMap.begin(),
                                        cblock.ssaInfoMap.end(), svreg)->second, ls);
                            if (svrres.second)
                                // begin region from this block
                                lv.insert(curLiveTime, liveTime+1);
                            else
                                lv.expand(liveTime+1);
                        }
                        for (AsmSingleVReg svreg: writtenSVRegs)
                        {
                            size_t& ssaIdIdx = ssaIdIdxMap[svreg];
                            if (svreg.regVar != nullptr)
                                ssaIdIdx++;
                            SSAInfo& sinfo = binaryMapFind(cblock.ssaInfoMap.begin(),
                                        cblock.ssaInfoMap.end(), svreg)->second;
                            Liveness& lv = getLiveness(svreg, ssaIdIdx, sinfo, ls);
                            // works only with ISA where smallest instruction have 2 bytes!
                            // after previous read, but not after instruction.
                            // if var is not used anywhere then this liveness region
                            // blocks assignment for other vars
                            lv.insert(liveTime+1, liveTime+2);
                        }
                        
                        // collecting linear deps for instruction
                        std::vector<AsmRegVarLinearDep> instrLinDeps;
                        AsmRegVarLinearDep linDep = { 0, nullptr, 0, 0 };
                        bool haveLdep = false;
                        if (oldOffset == 0 && linearDepPos < linearDepSize)
                        {
                            // special case: if offset is zero, force get linear dep
                            linDep = linDepHandler.getLinearDep(linearDepPos++);
                            haveLdep = true;
                        }
                        while (linDep.offset < oldOffset && linearDepPos < linearDepSize)
                        {
                            linDep = linDepHandler.getLinearDep(linearDepPos++);
                            haveLdep = true;
                        }
                        // if found
                        if (haveLdep)
                            while (linDep.offset == oldOffset)
                            {
                                // just put
                                instrLinDeps.push_back(linDep);
                                if (linearDepPos < linearDepSize)
                                    linDep = linDepHandler.getLinearDep(linearDepPos++);
                                else // no data
                                    break;
                            }
                        // get linear deps and equal to
                        cxbyte lDeps[16];
                        usageHandler.getUsageDependencies(instrRVUs.size(),
                                    instrRVUs.data(), lDeps);
                        
                        if (!addUsageDeps(lDeps, instrRVUs, instrLinDeps, linearDepMaps,
                                cblock.ssaInfoMap, ssaIdIdxMap,
                                readSVRegs, writtenSVRegs, ls))
                            assembler.printError(nullptr, "Linear deps failed");
                        
                        readSVRegs.clear();
                        writtenSVRegs.clear();
                        if (!hasNext)
                            break;
                        oldOffset = rvu.offset;
                        instrRVUs.clear();
                    }
                    if (hasNext && oldOffset < cblock.end && !rvu.useRegMode)
                        instrRVUs.push_back(rvu);
                    if (oldOffset >= cblock.end)
                        break;
                    
                    for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
                    {
                        // per register/singlvreg
                        AsmSingleVReg svreg{ rvu.regVar, rindex };
                        if (checkWriteWithSSA(rvu))
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
                
                size_t curWayBIndex = flowStack.back().blockIndex.index;
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
            const BlockIndex routineBlock = callStack.back().routineBlock;
            auto res = routineMap.insert({ routineBlock.index, { } });
            
            // while second pass in recursion: the routine's insertion was happened
            // later in first pass (after return from second pass)
            // we check whether second pass happened for this routine
            // order: create in second pass recursion
            //           (fromSecondPass && rblock.pass==0 avoids
            //            doubles creating in second pass)
            //        create in first pass recursion
            if (res.second || (res.first->second.fromSecondPass && routineBlock.pass==0))
            {
                res.first->second.fromSecondPass = routineBlock.pass==1;
                auto varRes = vidxRoutineMap.insert({ routineBlock.index, VIdxSetEntry{} });
                createRoutineDataLv(ls, res.first->second, varRes.first->second,
                        routineBlock.index);
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
                    
                    joinVRegRecur(ls, flowStackStart, entry.first, entry.second, false);
                }
            }
            callBlocks.erase(routineBlock);
            callStack.pop_back(); // just return from call
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            BlockIndex nextBlock = cblock.nexts[entry.nextIndex].block;
            nextBlock.pass = entry.blockIndex.pass;
            if (cblock.nexts[entry.nextIndex].isCall)
            {
                if (!callBlocks.insert(nextBlock).second)
                {
                    // just skip recursion (is good?)
                    if (recurseBlocks.insert(nextBlock.index).second)
                    {
                        ARDOut << "   -- recursion: " << nextBlock << "\n";
                        nextBlock.pass = 1;
                    }
                    else if (entry.blockIndex.pass==1)
                    {
                        /// mark that is routine call to skip
                        entry.nextIndex++;
                        continue;
                    }
                }
                else if (entry.blockIndex.pass==1 &&
                    recurseBlocks.find(nextBlock.index) != recurseBlocks.end())
                {
                    /// mark that is routine call to skip
                    entry.nextIndex++;
                    continue;
                }
                
                callStack.push_back({ entry.blockIndex,
                            entry.nextIndex, nextBlock });
            }
            
            flowStack.push_back({ nextBlock, 0 });
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
                        auto rit = routineMap.find(next.block);
                        if (rit == routineMap.end())
                            continue;
                        const RoutineDataLv& rdata = rit->second;
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
                    auto rit = routineMap.find(next.block);
                    if (rit == routineMap.end())
                        continue;
                    const RoutineDataLv& rdata = rit->second;
                    for (const auto& entry: rdata.lastAccessMap)
                        if (revertedSVRegs.insert(entry.first).second)
                            revertLastSVReg(lastVRegMap, entry.first);
                }
            
            for (const auto& sentry: cblock.ssaInfoMap)
                revertLastSVReg(lastVRegMap, sentry.first);
            
            if (!flowStack.empty() && lastCommonCacheWayPoint.first != SIZE_MAX &&
                    lastCommonCacheWayPoint.second >= flowStack.size() &&
                    flowStack.back().blockIndex.pass == 0)
            {
                lastCommonCacheWayPoint =
                        { flowStack.back().blockIndex.index, flowStack.size()-1 };
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
        CodeBlock& cblock = codeBlocks[entry.blockIndex.index];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex])
                visited[entry.blockIndex] = true;
            else
            {
                joinRegVarLivenesses(ls, joinFirstPointsCache, joinSecondPointsCache);
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
                !joinSecondPointsCache.hasKey(entry.blockIndex.index))
                // add to cache
                addJoinSecCacheEntry(ls, joinSecondPointsCache, entry.blockIndex.index);
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
