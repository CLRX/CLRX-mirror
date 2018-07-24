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

static inline void insertReplace(SSAReplacesMap& rmap, const AsmSingleVReg& vreg,
              size_t origId, size_t destId)
{
    ARDOut << "  insertreplace: " << vreg.regVar << ":" << vreg.index  << ": " <<
                origId << ", " << destId << "\n";
    auto res = rmap.insert({ vreg, {} });
    res.first->second.insertValue({ origId, destId });
}

/* caching concepts:
 * resfirstPointsCache - cache of the ways that goes to conflict which should be resolved
 *               from first code block of the code. The entries holds a stackVarMap state
 *               to first point the conflict (first visited already code block)
 * resSecondPointsCache - cache of the tree traversing, starting at the first conflict
 *               point (first visited code block). Entries holds a
 *               regvars SSAId read before write (that should resolved)
 */

static void handleSSAEntryWhileResolving(SSAReplacesMap* replacesMap,
            const LastSSAIdMap* stackVarMap, SVRegBlockMap& alreadyReadMap,
            FlowStackEntry2& entry, const SSAEntry& sentry,
            RBWSSAIdMap* cacheSecPoints)
{
    if (sentry.first.regVar==nullptr)
        return; // ignore registers (only regvars)
    
    const SSAInfo& sinfo = sentry.second;
    auto res = alreadyReadMap.insert({ sentry.first, entry.blockIndex });
    
    if (res.second && sinfo.readBeforeWrite)
    {
        if (cacheSecPoints != nullptr)
        {
            auto res = cacheSecPoints->insert({ sentry.first, { sinfo.ssaIdBefore } });
            if (!res.second)
                res.first->second.insertValue(sinfo.ssaIdBefore);
        }
        
        if (stackVarMap != nullptr)
        {
            // resolve conflict for this variable ssaId.
            // only if in previous block previous SSAID is
            // read before all writes
            auto it = stackVarMap->find(sentry.first);
            
            if (it != stackVarMap->end())
            {
                // found, resolve by set ssaIdLast
                for (size_t ssaId: it->second)
                {
                    if (ssaId > sinfo.ssaIdBefore)
                        insertReplace(*replacesMap, sentry.first, ssaId,
                                    sinfo.ssaIdBefore);
                    else if (ssaId < sinfo.ssaIdBefore)
                        insertReplace(*replacesMap, sentry.first,
                                        sinfo.ssaIdBefore, ssaId);
                }
            }
        }
    }
}

// use res second point cache entry to resolve conflict with SSAIds.
// it emits SSA replaces from these conflicts
static void useResSecPointCache(SSAReplacesMap* replacesMap,
        const LastSSAIdMap* stackVarMap, const SVRegBlockMap& alreadyReadMap,
        const RBWSSAIdMap* resSecondPoints, BlockIndex nextBlock,
        RBWSSAIdMap* destCacheSecPoints)
{
    ARDOut << "use resSecPointCache for " << nextBlock <<
            ", alreadyRMapSize: " << alreadyReadMap.size() << "\n";
    for (const auto& sentry: *resSecondPoints)
    {
        const bool alreadyRead = alreadyReadMap.find(sentry.first) != alreadyReadMap.end();
        if (destCacheSecPoints != nullptr && !alreadyRead)
        {
            auto res = destCacheSecPoints->insert(sentry);
            if (!res.second)
                for (size_t srcSSAId: sentry.second)
                    res.first->second.insertValue(srcSSAId);
        }
        
        if (stackVarMap != nullptr)
        {
            auto it = stackVarMap->find(sentry.first);
            
            if (it != stackVarMap->end() && !alreadyRead)
            {
                // found, resolve by set ssaIdLast
                for (size_t ssaId: it->second)
                {
                    for (size_t secSSAId: sentry.second)
                    {
                        if (ssaId > secSSAId)
                            insertReplace(*replacesMap, sentry.first, ssaId, secSSAId);
                        else if (ssaId < secSSAId)
                            insertReplace(*replacesMap, sentry.first, secSSAId, ssaId);
                    }
                }
            }
        }
    }
}

// add new res second cache entry with readBeforeWrite for all encountered regvars
static void addResSecCacheEntry(const RoutineMap& routineMap,
                const std::vector<CodeBlock>& codeBlocks,
                SimpleCache<size_t, RBWSSAIdMap>& resSecondPointsCache,
                size_t nextBlock)
{
    ARDOut << "addResSecCacheEntry: " << nextBlock << "\n";
    //std::stack<CallStackEntry> callStack = prevCallStack;
    // traverse by graph from next block
    std::deque<FlowStackEntry2> flowStack;
    flowStack.push_back({ nextBlock, 0 });
    std::unordered_set<size_t> visited;
    
    // already read in current path
    // key - vreg, value - source block where vreg of conflict found
    SVRegBlockMap alreadyReadMap;
    
    RBWSSAIdMap cacheSecPoints;
    
    while (!flowStack.empty())
    {
        FlowStackEntry2& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (visited.insert(entry.blockIndex).second)
            {
                ARDOut << "  resolv (cache): " << entry.blockIndex << "\n";
                
                const RBWSSAIdMap* resSecondPoints =
                            resSecondPointsCache.use(entry.blockIndex);
                if (resSecondPoints == nullptr)
                    for (auto& sentry: cblock.ssaInfoMap)
                        handleSSAEntryWhileResolving(nullptr, nullptr,
                                alreadyReadMap, entry, sentry,
                                &cacheSecPoints);
                else // to use cache
                {
                    useResSecPointCache(nullptr, nullptr, alreadyReadMap,
                            resSecondPoints, entry.blockIndex, &cacheSecPoints);
                    flowStack.pop_back();
                    continue;
                }
            }
            else
            {
                // back, already visited
                ARDOut << "resolv already (cache): " << entry.blockIndex << "\n";
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
                    const RoutineData& rdata = routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                        alreadyReadMap.insert({v.first, entry.blockIndex });
                    for (const auto& v: rdata.lastSSAIdMap)
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
                    const RoutineData& rdata = routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                    {
                        auto it = alreadyReadMap.find(v.first);
                        if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                            alreadyReadMap.erase(it);
                    }
                    for (const auto& v: rdata.lastSSAIdMap)
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
            ARDOut << "  popresolv (cache)\n";
            flowStack.pop_back();
        }
    }
    
    resSecondPointsCache.put(nextBlock, cacheSecPoints);
}

// apply calls (changes from these calls) from code blocks to stack var map
static void applyCallToStackVarMap(const CodeBlock& cblock, const RoutineMap& routineMap,
        LastSSAIdMap& stackVarMap, BlockIndex blockIndex, size_t nextIndex)
{
    for (const NextBlock& next: cblock.nexts)
        if (next.isCall)
        {
            const LastSSAIdMap& regVarMap =
                    routineMap.find(next.block)->second.lastSSAIdMap;
            for (const auto& sentry: regVarMap)
                stackVarMap[sentry.first].clear(); // clearing
        }
    
    for (const NextBlock& next: cblock.nexts)
        if (next.isCall)
        {
            ARDOut << "  applycall: " << blockIndex << ": " <<
                    nextIndex << ": " << next.block << "\n";
            const LastSSAIdMap& regVarMap =
                    routineMap.find(next.block)->second.lastSSAIdMap;
            for (const auto& sentry: regVarMap)
                for (size_t s: sentry.second)
                    stackVarMap.insertSSAId(sentry.first, s);
        }
}


// main routine to resilve SSA conflicts in code
// it emits SSA replaces from these conflicts
static void resolveSSAConflicts(const std::deque<FlowStackEntry2>& prevFlowStack,
        const RoutineMap& routineMap, const std::vector<CodeBlock>& codeBlocks,
        const PrevWaysIndexMap& prevWaysIndexMap,
        const CBlockBitPool& waysToCache, ResSecondPointsToCache& cblocksToCache,
        SimpleCache<size_t, LastSSAIdMap>& resFirstPointsCache,
        SimpleCache<size_t, RBWSSAIdMap>& resSecondPointsCache,
        SSAReplacesMap& replacesMap)
{
    size_t nextBlock = prevFlowStack.back().blockIndex;
    auto pfEnd = prevFlowStack.end();
    --pfEnd;
    ARDOut << "startResolv: " << (pfEnd-1)->blockIndex << "," << nextBlock << "\n";
    LastSSAIdMap stackVarMap;
    
    size_t pfStartIndex = 0;
    {
        auto pfPrev = pfEnd;
        --pfPrev;
        auto it = prevWaysIndexMap.find(pfPrev->blockIndex);
        if (it != prevWaysIndexMap.end())
        {
            const LastSSAIdMap* cached = resFirstPointsCache.use(it->second.first);
            if (cached!=nullptr)
            {
                ARDOut << "use pfcached: " << it->second.first << ", " <<
                        it->second.second << "\n";
                stackVarMap = *cached;
                pfStartIndex = it->second.second+1;
                
                // apply missing calls at end of the cached
                const CodeBlock& cblock = codeBlocks[it->second.first];
                
                const FlowStackEntry2& entry = *(prevFlowStack.begin()+pfStartIndex-1);
                if (entry.nextIndex > cblock.nexts.size())
                    applyCallToStackVarMap(cblock, routineMap, stackVarMap, -1, -1);
            }
        }
    }
    
    // collect previous svreg from current path
    for (auto pfit = prevFlowStack.begin()+pfStartIndex; pfit != pfEnd; ++pfit)
    {
        const FlowStackEntry2& entry = *pfit;
        ARDOut << "  apply: " << entry.blockIndex << "\n";
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        for (const auto& sentry: cblock.ssaInfoMap)
        {
            if (sentry.first.regVar==nullptr)
                // ignore regular registers (only regvars)
                continue;
            const SSAInfo& sinfo = sentry.second;
            if (sinfo.ssaIdChange != 0)
                stackVarMap[sentry.first] = { sinfo.ssaId + sinfo.ssaIdChange - 1 };
        }
        if (entry.nextIndex > cblock.nexts.size())
            applyCallToStackVarMap(cblock, routineMap, stackVarMap,
                        entry.blockIndex, entry.nextIndex);
        
        // put to first point cache
        if (waysToCache[pfit->blockIndex] &&
            !resFirstPointsCache.hasKey(pfit->blockIndex))
        {
            ARDOut << "put pfcache " << pfit->blockIndex << "\n";
            resFirstPointsCache.put(pfit->blockIndex, stackVarMap);
        }
    }
    
    RBWSSAIdMap cacheSecPoints;
    const bool toCache = (!resSecondPointsCache.hasKey(nextBlock)) &&
                cblocksToCache.count(nextBlock)>=2;
    
    //std::stack<CallStackEntry> callStack = prevCallStack;
    // traverse by graph from next block
    std::deque<FlowStackEntry2> flowStack;
    flowStack.push_back({ nextBlock, 0 });
    std::unordered_set<size_t> visited;
    
    // already read in current path
    // key - vreg, value - source block where vreg of conflict found
    SVRegBlockMap alreadyReadMap;
    
    while (!flowStack.empty())
    {
        FlowStackEntry2& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (visited.insert(entry.blockIndex).second)
            {
                ARDOut << "  resolv: " << entry.blockIndex << "\n";
                
                const RBWSSAIdMap* resSecondPoints =
                        resSecondPointsCache.use(entry.blockIndex);
                if (resSecondPoints == nullptr)
                    for (auto& sentry: cblock.ssaInfoMap)
                        handleSSAEntryWhileResolving(&replacesMap, &stackVarMap,
                                alreadyReadMap, entry, sentry,
                                toCache ? &cacheSecPoints : nullptr);
                else // to use cache
                {
                    useResSecPointCache(&replacesMap, &stackVarMap, alreadyReadMap,
                            resSecondPoints, entry.blockIndex,
                            toCache ? &cacheSecPoints : nullptr);
                    flowStack.pop_back();
                    continue;
                }
            }
            else
            {
                cblocksToCache.increase(entry.blockIndex);
                ARDOut << "cblockToCache: " << entry.blockIndex << "=" <<
                            cblocksToCache.count(entry.blockIndex) << "\n";
                // back, already visited
                ARDOut << "resolv already: " << entry.blockIndex << "\n";
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
                    const RoutineData& rdata = routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                        alreadyReadMap.insert({v.first, entry.blockIndex });
                    for (const auto& v: rdata.lastSSAIdMap)
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
                    const RoutineData& rdata = routineMap.find(next.block)->second;
                    for (const auto& v: rdata.rbwSSAIdMap)
                    {
                        auto it = alreadyReadMap.find(v.first);
                        if (it != alreadyReadMap.end() && it->second == entry.blockIndex)
                            alreadyReadMap.erase(it);
                    }
                    for (const auto& v: rdata.lastSSAIdMap)
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
            ARDOut << "  popresolv\n";
            
            if (cblocksToCache.count(entry.blockIndex)==2 &&
                !resSecondPointsCache.hasKey(entry.blockIndex))
                // add to cache
                addResSecCacheEntry(routineMap, codeBlocks, resSecondPointsCache,
                            entry.blockIndex);
            
            flowStack.pop_back();
        }
    }
    
    if (toCache)
        resSecondPointsCache.put(nextBlock, cacheSecPoints);
}

// join ret SSAId Map - src - last SSAIdMap from called routine
static void joinRetSSAIdMap(RetSSAIdMap& dest, const LastSSAIdMap& src,
                BlockIndex routineBlock)
{
    for (const auto& entry: src)
    {
        ARDOut << "  joinRetSSAIdMap: " << entry.first.regVar << ":" <<
                cxuint(entry.first.index) << ":";
        for (size_t v: entry.second)
            ARDOut << " " << v;
        ARDOut << "\n";
        // insert if not inserted
        auto res = dest.insert({entry.first, { { routineBlock }, entry.second } });
        if (res.second)
            continue; // added new
        VectorSet<size_t>& destEntry = res.first->second.ssaIds;
        res.first->second.routines.push_back(routineBlock);
        // add new ways
        for (size_t ssaId: entry.second)
            destEntry.insertValue(ssaId);
        ARDOut << "    :";
        for (size_t v: destEntry)
            ARDOut << " " << v;
        ARDOut << "\n";
    }
}

// simple join last ssaid map
static void joinLastSSAIdMap(LastSSAIdMap& dest, const LastSSAIdMap& src)
{
    for (const auto& entry: src)
    {
        ARDOut << "  joinLastSSAIdMap: " << entry.first.regVar << ":" <<
                cxuint(entry.first.index) << ":";
        for (size_t v: entry.second)
            ARDOut << " " << v;
        ARDOut << "\n";
        auto res = dest.insert(entry); // find
        if (res.second)
            continue; // added new
        VectorSet<size_t>& destEntry = res.first->second;
        // add new ways
        for (size_t ssaId: entry.second)
            destEntry.insertValue(ssaId);
        ARDOut << "    :";
        for (size_t v: destEntry)
            ARDOut << " " << v;
        ARDOut << "\n";
    }
}

// join last SSAIdMap of the routine including later routine call
// dest - dest last SSAId map, src - source lastSSAIdMap
// laterRdatas - data of subroutine/routine exeuted after src lastSSAIdMap state
static void joinLastSSAIdMapInt(LastSSAIdMap& dest, const LastSSAIdMap& src,
                    const LastSSAIdMap& laterRdataCurSSAIdMap,
                    const LastSSAIdMap& laterRdataLastSSAIdMap, bool loop)
{
    for (const auto& entry: src)
    {
        auto lsit = laterRdataLastSSAIdMap.find(entry.first);
        if (lsit != laterRdataLastSSAIdMap.end())
        {
            auto csit = laterRdataCurSSAIdMap.find(entry.first);
            if (csit != laterRdataCurSSAIdMap.end() && !csit->second.empty())
            {
                // if found in last ssa ID map,
                // but has first value (some way do not change SSAId)
                // then pass to add new ssaIds before this point
                if (!lsit->second.hasValue(csit->second[0]))
                    continue; // otherwise, skip
            }
        }
        ARDOut << "  joinLastSSAIdMapInt: " << entry.first.regVar << ":" <<
                cxuint(entry.first.index) << ":";
        for (size_t v: entry.second)
            ARDOut << " " << v;
        ARDOut << "\n";
        auto res = dest.insert(entry); // find
        if (res.second)
            continue; // added new
        VectorSet<size_t>& destEntry = res.first->second;
        // add new ways
        for (size_t ssaId: entry.second)
            destEntry.insertValue(ssaId);
        ARDOut << "    :";
        for (size_t v: destEntry)
            ARDOut << " " << v;
        ARDOut << "\n";
    }
    if (!loop) // do not if loop
        joinLastSSAIdMap(dest, laterRdataLastSSAIdMap);
}

static void joinLastSSAIdMap(LastSSAIdMap& dest, const LastSSAIdMap& src,
                    const RoutineData& laterRdata, bool loop = false)
{
    joinLastSSAIdMapInt(dest, src, laterRdata.curSSAIdMap, laterRdata.lastSSAIdMap, loop);
}


// join routine data from child call with data from parent routine
// (just join child call from parent)
static void joinRoutineData(RoutineData& dest, const RoutineData& src,
            const SVRegMap& curSSAIdMap, bool notFirstReturn)
{
    // insert readBeforeWrite only if doesnt exists in destination
    dest.rbwSSAIdMap.insert(src.rbwSSAIdMap.begin(), src.rbwSSAIdMap.end());
    dest.origRbwSSAIdMap.insert(src.origRbwSSAIdMap.begin(), src.origRbwSSAIdMap.end());
    
    //joinLastSSAIdMap(dest.curSSAIdMap, src.lastSSAIdMap);
    
    for (const auto& entry: src.lastSSAIdMap)
    {
        ARDOut << "  joinRoutineData: " << entry.first.regVar << ":" <<
                cxuint(entry.first.index) << ":";
        for (size_t v: entry.second)
            ARDOut << " " << v;
        ARDOut << "\n";
        auto res = dest.curSSAIdMap.insert(entry); // find
        VectorSet<size_t>& destEntry = res.first->second;
        
        if (!res.second)
        {
            // add new ways
            for (size_t ssaId: entry.second)
                destEntry.insertValue(ssaId);
        }
        else if (notFirstReturn)
        {
            auto csit = curSSAIdMap.find(entry.first);
            // insert to lastSSAIdMap if no ssaIds for regvar in lastSSAIdMap
            dest.lastSSAIdMap.insert({ entry.first,
                        { (csit!=curSSAIdMap.end() ? csit->second : 1)-1 } });
        }
        
        ARDOut << "    :";
        for (size_t v: destEntry)
            ARDOut << " " << v;
        ARDOut << "\n";
    }
}

// reduce retSSAIds for calls (for all read before write SSAIds for current code block)
static void reduceSSAIdsForCalls(FlowStackEntry& entry,
            const std::vector<CodeBlock>& codeBlocks,
            RetSSAIdMap& retSSAIdMap, RoutineMap& routineMap,
            SSAReplacesMap& ssaReplacesMap)
{
    if (retSSAIdMap.empty())
        return;
    LastSSAIdMap rbwSSAIdMap;
    std::unordered_set<AsmSingleVReg> reduced;
    std::unordered_set<AsmSingleVReg> changed;
    const CodeBlock& cblock = codeBlocks[entry.blockIndex.index];
    // collect rbw SSAIds
    for (const NextBlock next: cblock.nexts)
        if (next.isCall)
        {
            auto it = routineMap.find(next.block); // must find
            for (const auto& rentry: it->second.rbwSSAIdMap)
                rbwSSAIdMap.insertSSAId(rentry.first,rentry.second);
            
        }
    for (const NextBlock next: cblock.nexts)
        if (next.isCall)
        {
            auto it = routineMap.find(next.block); // must find
            // add changed
            for (const auto& lentry: it->second.lastSSAIdMap)
                if (rbwSSAIdMap.find(lentry.first) == rbwSSAIdMap.end())
                    changed.insert(lentry.first);
        }
    
    // reduce SSAIds
    for (const auto& rentry: retSSAIdMap)
    {
        auto ssaIdsIt = rbwSSAIdMap.find(rentry.first);
        if (ssaIdsIt != rbwSSAIdMap.end())
        {
            const VectorSet<size_t>& ssaIds = ssaIdsIt->second;
            const VectorSet<size_t>& rssaIds = rentry.second.ssaIds;
            Array<size_t> outSSAIds(ssaIds.size() + rssaIds.size());
            std::copy(rssaIds.begin(), rssaIds.end(), outSSAIds.begin());
            std::copy(ssaIds.begin(), ssaIds.end(), outSSAIds.begin()+rssaIds.size());
            
            std::sort(outSSAIds.begin(), outSSAIds.end());
            outSSAIds.resize(std::unique(outSSAIds.begin(), outSSAIds.end()) -
                        outSSAIds.begin());
            size_t minSSAId = outSSAIds.front();
            if (outSSAIds.size() >= 2)
            {
                for (auto sit = outSSAIds.begin()+1; sit != outSSAIds.end(); ++sit)
                    insertReplace(ssaReplacesMap, rentry.first, *sit, minSSAId);
                
                ARDOut << "calls retssa ssaid: " << rentry.first.regVar << ":" <<
                        rentry.first.index << "\n";
            }
            
            for (BlockIndex rblock: rentry.second.routines)
                routineMap.find(rblock)->second.lastSSAIdMap[rentry.first] =
                            VectorSet<size_t>({ minSSAId });
            reduced.insert(rentry.first);
        }
    }
    for (const AsmSingleVReg& vreg: reduced)
        retSSAIdMap.erase(vreg);
    reduced.clear();
        
    for (const AsmSingleVReg& vreg: changed)
    {
        auto rit = retSSAIdMap.find(vreg);
        if (rit != retSSAIdMap.end())
        {
            // if modified
            // put before removing to revert for other ways after calls
            entry.prevRetSSAIdSets.insert(*rit);
            // just remove, if some change without read before
            retSSAIdMap.erase(rit);
        }
    }
}

static void reduceSSAIds2(SVRegMap& curSSAIdMap, RetSSAIdMap& retSSAIdMap,
                FlowStackEntry& entry, const SSAEntry& ssaEntry)
{
    const SSAInfo& sinfo = ssaEntry.second;
    size_t& ssaId = curSSAIdMap[ssaEntry.first];
    auto ssaIdsIt = retSSAIdMap.find(ssaEntry.first);
    if (ssaIdsIt != retSSAIdMap.end() && sinfo.readBeforeWrite)
    {
        auto& ssaIds = ssaIdsIt->second.ssaIds;
        ssaId = ssaIds.front()+1; // plus one
        retSSAIdMap.erase(ssaIdsIt);
    }
    else if (ssaIdsIt != retSSAIdMap.end() && sinfo.ssaIdChange!=0)
    {
        // put before removing to revert for other ways after calls
        entry.prevRetSSAIdSets.insert(*ssaIdsIt);
        // just remove, if some change without read before
        retSSAIdMap.erase(ssaIdsIt);
    }
}

// reduce retSSAIds (last SSAIds for regvar) while passing by code block
// and emits SSA replaces for these ssaids
static void reduceSSAIds(SVRegMap& curSSAIdMap, RetSSAIdMap& retSSAIdMap,
            RoutineMap& routineMap, SSAReplacesMap& ssaReplacesMap, FlowStackEntry& entry,
            SSAEntry& ssaEntry)
{
    SSAInfo& sinfo = ssaEntry.second;
    size_t& ssaId = curSSAIdMap[ssaEntry.first];
    auto ssaIdsIt = retSSAIdMap.find(ssaEntry.first);
    if (ssaIdsIt != retSSAIdMap.end() && sinfo.readBeforeWrite)
    {
        auto& ssaIds = ssaIdsIt->second.ssaIds;
        
        if (sinfo.ssaId != SIZE_MAX)
        {
            VectorSet<size_t> outSSAIds = ssaIds;
            outSSAIds.insertValue(ssaId-1); // ???
            // already set
            if (outSSAIds.size() >= 1)
            {
                // reduce to minimal ssaId from all calls
                std::sort(outSSAIds.begin(), outSSAIds.end());
                outSSAIds.resize(std::unique(outSSAIds.begin(), outSSAIds.end()) -
                            outSSAIds.begin());
                // insert SSA replaces
                if (outSSAIds.size() >= 2)
                {
                    size_t minSSAId = outSSAIds.front();
                    for (auto sit = outSSAIds.begin()+1; sit != outSSAIds.end(); ++sit)
                        insertReplace(ssaReplacesMap, ssaEntry.first, *sit, minSSAId);
                }
            }
        }
        else if (ssaIds.size() >= 2)
        {
            // reduce to minimal ssaId from all calls
            std::sort(ssaIds.begin(), ssaIds.end());
            ssaIds.resize(std::unique(ssaIds.begin(), ssaIds.end()) - ssaIds.begin());
            // insert SSA replaces
            size_t minSSAId = ssaIds.front();
            for (auto sit = ssaIds.begin()+1; sit != ssaIds.end(); ++sit)
                insertReplace(ssaReplacesMap, ssaEntry.first, *sit, minSSAId);
            ssaId = minSSAId+1; // plus one
        }
        else if (ssaIds.size() == 1)
            ssaId = ssaIds.front()+1; // plus one
        
        ARDOut << "retssa ssaid: " << ssaEntry.first.regVar << ":" <<
                ssaEntry.first.index << ": " << ssaId << "\n";
        // replace smallest ssaId in routineMap lastSSAId entry
        // reduce SSAIds replaces
        for (BlockIndex rblock: ssaIdsIt->second.routines)
        {
            RoutineData& rdata = routineMap.find(rblock)->second;
            size_t rbwRetSSAId = SIZE_MAX;
            auto rbwIt = rdata.rbwSSAIdMap.find(ssaEntry.first);
            auto rlsit = rdata.lastSSAIdMap.find(ssaEntry.first);
            if (rbwIt != rdata.rbwSSAIdMap.end() && rlsit->second.hasValue(rbwIt->second))
                rbwRetSSAId = rbwIt->second;
            rlsit->second = VectorSet<size_t>({ ssaId-1 });
            if (rbwRetSSAId != SIZE_MAX)
            {
                ARDOut << "  keep retSSAId rbw: " << rbwRetSSAId << "\n";
                // add retSSAId without changes (in way without regvar changes)
                rlsit->second.insertValue(rbwRetSSAId);
            }
        }
        // finally remove from container (because obsolete)
        retSSAIdMap.erase(ssaIdsIt);
    }
    else if (ssaIdsIt != retSSAIdMap.end() && sinfo.ssaIdChange!=0)
    {
        // put before removing to revert for other ways after calls
        entry.prevRetSSAIdSets.insert(*ssaIdsIt);
        // just remove, if some change without read before
        retSSAIdMap.erase(ssaIdsIt);
    }
}

// update single current SSAId for routine and optionally lastSSAIdMap if returns
// has been encountered but not regvar
static void updateRoutineData(RoutineData& rdata, const SSAEntry& ssaEntry,
                size_t prevSSAId)
{
    const SSAInfo& sinfo = ssaEntry.second;
    bool beforeFirstAccess = true;
    // put first SSAId before write
    if (sinfo.readBeforeWrite)
    {
        //ARDOut << "PutCRBW: " << sinfo.ssaIdBefore << "\n";
        if (!rdata.rbwSSAIdMap.insert({ ssaEntry.first, prevSSAId }).second)
            // if already added
            beforeFirstAccess = false;
        
        rdata.origRbwSSAIdMap.insert({ ssaEntry.first, ssaEntry.second.ssaIdBefore });
    }
    
    if (sinfo.ssaIdChange != 0)
    {
        //ARDOut << "PutC: " << sinfo.ssaIdLast << "\n";
        auto res = rdata.curSSAIdMap.insert({ ssaEntry.first, { sinfo.ssaIdLast } });
        // put last SSAId
        if (!res.second)
        {
            beforeFirstAccess = false;
            // if not inserted
            VectorSet<size_t>& ssaIds = res.first->second;
            ssaIds.clear(); // clear all ssaIds in currentSSAID entry
            ssaIds.insertValue(sinfo.ssaIdLast);
        }
        // add readbefore if in previous returns if not added yet
        if (rdata.notFirstReturn && beforeFirstAccess)
        {
            rdata.lastSSAIdMap.insert({ ssaEntry.first, { prevSSAId } });
            for (auto& loopEnd: rdata.loopEnds)
                loopEnd.second.ssaIdMap.insert({ ssaEntry.first, { prevSSAId } });
        }
    }
    else
    {
        // insert read ssaid if no change
        auto res = rdata.curSSAIdMap.insert({ ssaEntry.first, { prevSSAId } });
        if (!res.second)
            res.first->second.insertValue(prevSSAId);
    }
}

static void initializePrevRetSSAIds(const SVRegMap& curSSAIdMap,
            const RetSSAIdMap& retSSAIdMap, const RoutineData& rdata,
            FlowStackEntry& entry)
{
    for (const auto& v: rdata.lastSSAIdMap)
    {
        auto res = entry.prevRetSSAIdSets.insert({v.first, {}});
        if (!res.second)
            continue; // already added, do not change
        auto rfit = retSSAIdMap.find(v.first);
        if (rfit != retSSAIdMap.end())
            res.first->second = rfit->second;
        
        auto csit = curSSAIdMap.find(v.first);
        res.first->second.prevSSAId = (csit!=curSSAIdMap.end() ? csit->second : 1);
    }
}

// revert retSSAIdMap while leaving from code block
static void revertRetSSAIdMap(SVRegMap& curSSAIdMap, RetSSAIdMap& retSSAIdMap,
            FlowStackEntry& entry, RoutineData* rdata)
{
    // revert retSSAIdMap
    for (auto v: entry.prevRetSSAIdSets)
    {
        auto rfit = retSSAIdMap.find(v.first);
        if (rdata!=nullptr && rfit != retSSAIdMap.end())
        {
            VectorSet<size_t>& ssaIds = rdata->curSSAIdMap[v.first];
            for (size_t ssaId: rfit->second.ssaIds)
                ssaIds.eraseValue(ssaId);
        }
        
        if (!v.second.ssaIds.empty())
        {
            // just add if previously present
            if (rfit != retSSAIdMap.end())
                rfit->second = v.second;
            else
                retSSAIdMap.insert(v);
        }
        else // erase if empty
            retSSAIdMap.erase(v.first);
        
        size_t oldSSAId = curSSAIdMap[v.first]-1;
        curSSAIdMap[v.first] = v.second.prevSSAId;
        if (rdata!=nullptr)
        {
            VectorSet<size_t>& ssaIds = rdata->curSSAIdMap[v.first];
            ssaIds.eraseValue(oldSSAId); // ??? need extra constraints
            for (size_t ssaId: v.second.ssaIds)
                ssaIds.insertValue(ssaId);
            if (v.second.ssaIds.empty())
            {
                auto cit = curSSAIdMap.find(v.first);
                ssaIds.insertValue((cit!=curSSAIdMap.end() ? cit->second : 1)-1);
            }
            
            ARDOut << " revertSSAIdPopEntry" << entry.blockIndex << ": " <<
                    v.first.regVar << ":" << v.first.index << ":";
            for (size_t v: ssaIds)
                ARDOut << " " << v;
            ARDOut << "\n";
        }
    }
}

// update current SSAId in curSSAIdMap for routine while leaving from code block
static void updateRoutineCurSSAIdMap(RoutineData* rdata, const SSAEntry& ssaEntry,
            const FlowStackEntry& entry, size_t curSSAId, size_t nextSSAId)
{
    VectorSet<size_t>& ssaIds = rdata->curSSAIdMap[ssaEntry.first];
    ARDOut << " updateRoutineCurSSAIdMap " << entry.blockIndex << ": " <<
                ssaEntry.first.regVar << ":" << ssaEntry.first.index << ":";
    for (size_t v: ssaIds)
        ARDOut << " " << v;
    ARDOut << "\n";
    
    // if cblock with some children
    if (nextSSAId != curSSAId)
        ssaIds.eraseValue(nextSSAId-1);
    
    // push previous SSAId to lastSSAIdMap (later will be replaced)
    ssaIds.insertValue(curSSAId-1);
    
    ARDOut << " updateRoutineCurSSAIdMap after " << entry.blockIndex << ": " <<
                ssaEntry.first.regVar << ":" << ssaEntry.first.index << ":";
    for (size_t v: ssaIds)
        ARDOut << " " << v;
    ARDOut << "\n";
}

static bool tryAddLoopEnd(const FlowStackEntry& entry, BlockIndex routineBlock,
                RoutineData& rdata, bool isLoop, bool noMainLoop)
{
    if (isLoop && (!noMainLoop || routineBlock != entry.blockIndex))
    {
        // handle loops
        ARDOut << "  join loop ssaids: " << entry.blockIndex << "\n";
        // add to routine data loopEnds
        auto loopsit2 = rdata.loopEnds.find(entry.blockIndex);
        if (loopsit2 != rdata.loopEnds.end())
        {
            if (!loopsit2->second.passed)
                // still in loop join ssaid map
                joinLastSSAIdMap(loopsit2->second.ssaIdMap, rdata.curSSAIdMap);
        }
        else
            rdata.loopEnds.insert({ entry.blockIndex, { rdata.curSSAIdMap, false } });
        return true;
    }
    return false;
}


static void createRoutineData(const std::vector<CodeBlock>& codeBlocks,
        SVRegMap& curSSAIdMap, const std::unordered_set<BlockIndex>& loopBlocks,
        const std::unordered_set<BlockIndex>& callBlocks,
        const ResSecondPointsToCache& subroutToCache,
        SimpleCache<BlockIndex, RoutineData>& subroutinesCache,
        const RoutineMap& routineMap, RoutineData& rdata,
        BlockIndex routineBlock, CBlockBitPool& prevFlowStackBlocks,
        CBlockBitPool& flowStackBlocks, bool noMainLoop = false)
{
    bool fromSubroutine = noMainLoop;
    ARDOut << "--------- createRoutineData ----------------\n";
    std::unordered_set<BlockIndex> visited;
    std::unordered_set<BlockIndex> haveReturnBlocks;
    
    VectorSet<BlockIndex> activeLoops;
    SubrLoopsMap subrLoopsMap;
    SubrLoopsMap loopSubrsMap;
    RoutineMap subrDataForLoopMap;
    std::deque<FlowStackEntry> flowStack;
    // last SSA ids map from returns
    RetSSAIdMap retSSAIdMap;
    flowStack.push_back({ routineBlock, 0 });
    if (!fromSubroutine)
        flowStackBlocks[routineBlock] = true;
    
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex.index];
        
        auto addSubroutine = [&](
            std::unordered_map<BlockIndex, LoopSSAIdMap>::const_iterator loopsit2,
            bool applyToMainRoutine)
        {
            if (subroutinesCache.hasKey(entry.blockIndex))
            {
                // if already put, just applyToMainRoutine if needed
                if (applyToMainRoutine &&
                    loopBlocks.find(entry.blockIndex) != loopBlocks.end() &&
                    loopsit2 != rdata.loopEnds.end())
                {
                    RoutineData* subRdata = subroutinesCache.use(entry.blockIndex);
                    joinLastSSAIdMap(rdata.lastSSAIdMap, loopsit2->second.ssaIdMap,
                                        *subRdata, true);
                }
                return;
            }
            
            RoutineData subrData;
            bool oldFB = false;
            if (!fromSubroutine)
            {
                oldFB = flowStackBlocks[entry.blockIndex];
                flowStackBlocks[entry.blockIndex] = !oldFB;
            }
            createRoutineData(codeBlocks, curSSAIdMap, loopBlocks, callBlocks,
                    subroutToCache, subroutinesCache, routineMap, subrData,
                    entry.blockIndex, flowStackBlocks, flowStackBlocks, true);
            RoutineData subrDataCopy;
            if (!fromSubroutine)
                flowStackBlocks[entry.blockIndex] = oldFB;
            
            if (loopBlocks.find(entry.blockIndex) != loopBlocks.end())
            {   // leave from loop point
                ARDOut << "   loopfound " << entry.blockIndex << "\n";
                if (loopsit2 != rdata.loopEnds.end())
                {
                    subrDataCopy = subrData;
                    subrDataForLoopMap.insert({ entry.blockIndex, subrDataCopy });
                    ARDOut << "   loopssaId2Map: " <<
                            entry.blockIndex << "\n";
                    joinLastSSAIdMap(subrData.lastSSAIdMap,
                            loopsit2->second.ssaIdMap, subrData, true);
                    ARDOut << "   loopssaIdMap2End: \n";
                    if (applyToMainRoutine)
                        joinLastSSAIdMap(rdata.lastSSAIdMap, loopsit2->second.ssaIdMap,
                                        subrDataCopy, true);
                }
            }
            
            // apply loop to subroutines
            auto it = loopSubrsMap.find(entry.blockIndex);
            if (it != loopSubrsMap.end())
            {
                ARDOut << "    found loopsubrsmap: " << entry.blockIndex << ":";
                for (BlockIndex subr: it->second)
                {
                    ARDOut << " " << subr;
                    RoutineData* subrData2 = subroutinesCache.use(subr);
                    if (subrData2 == nullptr)
                        continue;
                    RoutineData subrData2Copy = *subrData2;
                    ARDOut << "*";
                    joinLastSSAIdMap(subrData2Copy.lastSSAIdMap,
                            loopsit2->second.ssaIdMap, subrDataCopy, false);
                    // reinsert subroutine into subroutine cache
                    subrData2Copy.calculateWeight();
                    subroutinesCache.put(subr, subrData2Copy);
                }
                ARDOut << "\n";
            }
            // apply loops to this subroutine
            auto it2 = subrLoopsMap.find(entry.blockIndex);
            if (it2 != subrLoopsMap.end())
            {
                ARDOut << "    found subrloopsmap: " << entry.blockIndex << ":";
                for (auto lit2 = it2->second.rbegin(); lit2 != it2->second.rend(); ++lit2)
                {
                    BlockIndex loop = *lit2;
                    auto loopsit3 = rdata.loopEnds.find(loop);
                    if (loopsit3 == rdata.loopEnds.end() ||
                        activeLoops.hasValue(loop))
                        continue;
                    ARDOut << " " << loop;
                    auto  itx = subrDataForLoopMap.find(loop);
                    if (itx != subrDataForLoopMap.end())
                        joinLastSSAIdMap(subrData.lastSSAIdMap,
                                loopsit3->second.ssaIdMap, itx->second, false);
                }
                ARDOut << "\n";
            }
            
            subrData.calculateWeight();
            subroutinesCache.put(entry.blockIndex, subrData);
        };
        
        if (entry.nextIndex == 0)
        {
            bool isLoop = loopBlocks.find(entry.blockIndex) != loopBlocks.end();
            
            if (!prevFlowStackBlocks.empty() && prevFlowStackBlocks[entry.blockIndex])
            {
                // if loop detected (from previous subroutine)
                // handling loops inside routine
                tryAddLoopEnd(entry, routineBlock, rdata, isLoop, noMainLoop);
                
                if (!fromSubroutine)
                    flowStackBlocks[entry.blockIndex] = !flowStackBlocks[entry.blockIndex];
                flowStack.pop_back();
                continue;
            }
            
            // process current block
            const RoutineData* cachedRdata = nullptr;
            
            if (routineBlock != entry.blockIndex)
            {
                cachedRdata = subroutinesCache.use(entry.blockIndex);
                if (cachedRdata == nullptr)
                {
                    // try in routine map
                    auto rit = routineMap.find(entry.blockIndex);
                    if (rit != routineMap.end())
                        cachedRdata = &rit->second;
                }
                if (!isLoop && visited.find(entry.blockIndex)!=visited.end() &&
                    cachedRdata == nullptr && subroutToCache.count(entry.blockIndex)!=0)
                {
                    auto loopsit2 = rdata.loopEnds.find(entry.blockIndex);
                    ARDOut << "-- subrcache2 for " << entry.blockIndex << "\n";
                    addSubroutine(loopsit2, false);
                    cachedRdata = subroutinesCache.use(entry.blockIndex);
                }
            }
            
            if (cachedRdata != nullptr)
            {
                ARDOut << "use cached subr " << entry.blockIndex << "\n";
                ARDOut << "procret2: " << entry.blockIndex << "\n";
                if (visited.find(entry.blockIndex)!=visited.end() &&
                    haveReturnBlocks.find(entry.blockIndex)==haveReturnBlocks.end())
                {
                    // no joining. no returns
                    ARDOut << "procretend2 nojoin\n";
                    if (!fromSubroutine)
                        flowStackBlocks[entry.blockIndex] =
                                    !flowStackBlocks[entry.blockIndex];
                    flowStack.pop_back();
                    continue;
                }
                joinLastSSAIdMap(rdata.lastSSAIdMap, rdata.curSSAIdMap, *cachedRdata);
                // get not given rdata curSSAIdMap ssaIds but present in cachedRdata
                // curSSAIdMap
                for (const auto& entry: cachedRdata->curSSAIdMap)
                    if (rdata.curSSAIdMap.find(entry.first) == rdata.curSSAIdMap.end())
                    {
                        auto cit = curSSAIdMap.find(entry.first);
                        size_t prevSSAId = (cit!=curSSAIdMap.end() ? cit->second : 1)-1;
                        rdata.curSSAIdMap.insert({ entry.first, { prevSSAId } });
                        
                        if (rdata.notFirstReturn)
                        {
                            rdata.lastSSAIdMap.insertSSAId(entry.first, prevSSAId);
                            for (auto& loopEnd: rdata.loopEnds)
                                loopEnd.second.ssaIdMap.
                                        insertSSAId(entry.first, prevSSAId);
                        }
                    }
                
                // join loopEnds
                for (const auto& loopEnd: cachedRdata->loopEnds)
                {
                    auto res = rdata.loopEnds.insert({ loopEnd.first, LoopSSAIdMap{} });
                    // true - do not add cached rdata loopend, because it was added
                    joinLastSSAIdMapInt(res.first->second.ssaIdMap, rdata.curSSAIdMap,
                                cachedRdata->curSSAIdMap, loopEnd.second.ssaIdMap, true);
                }
                ARDOut << "procretend2\n";
                if (!fromSubroutine)
                    flowStackBlocks[entry.blockIndex] = !flowStackBlocks[entry.blockIndex];
                flowStack.pop_back();
                // propagate haveReturn to previous block
                flowStack.back().haveReturn = true;
                haveReturnBlocks.insert(flowStack.back().blockIndex);
                continue;
            }
            else if (visited.insert(entry.blockIndex).second)
            {
                // set up loops for which subroutine is present
                if (subroutToCache.count(entry.blockIndex)!=0 && !activeLoops.empty())
                {
                    subrLoopsMap.insert({ entry.blockIndex, activeLoops });
                    for (BlockIndex loop: activeLoops)
                    {
                        auto res = loopSubrsMap.insert({ loop, { entry.blockIndex } });
                        if (!res.second)
                            res.first->second.insertValue(entry.blockIndex);
                    }
                }
                
                if (loopBlocks.find(entry.blockIndex) != loopBlocks.end())
                    activeLoops.insertValue(entry.blockIndex);
                ARDOut << "proc: " << entry.blockIndex << "\n";
                
                for (const auto& ssaEntry: cblock.ssaInfoMap)
                    if (ssaEntry.first.regVar != nullptr)
                    {
                        reduceSSAIds2(curSSAIdMap, retSSAIdMap, entry, ssaEntry);
                        // put data to routine data
                        updateRoutineData(rdata, ssaEntry, curSSAIdMap[ssaEntry.first]-1);
                        
                        if (ssaEntry.second.ssaIdChange!=0)
                            curSSAIdMap[ssaEntry.first] = ssaEntry.second.ssaIdLast+1;
                    }
            }
            else
            {
                tryAddLoopEnd(entry, routineBlock, rdata, isLoop, noMainLoop);
                if (!fromSubroutine)
                    flowStackBlocks[entry.blockIndex] = !flowStackBlocks[entry.blockIndex];
                flowStack.pop_back();
                continue;
            }
        }
        
        // join and skip calls
        {
            std::vector<BlockIndex> calledRoutines;
            for (; entry.nextIndex < cblock.nexts.size() &&
                        cblock.nexts[entry.nextIndex].isCall; entry.nextIndex++)
            {
                BlockIndex rblock = cblock.nexts[entry.nextIndex].block;
                if (callBlocks.find(rblock) != callBlocks.end())
                    rblock.pass = 1;
                if (rblock != routineBlock)
                    calledRoutines.push_back(rblock);
            }
            
            if (!calledRoutines.empty())
            {
                // toNotClear - regvar to no keep (because is used in called routines)
                std::unordered_set<AsmSingleVReg> toNotClear;
                // if regvar any called routine (used)
                std::unordered_set<AsmSingleVReg> allInCalls;
                for (BlockIndex rblock: calledRoutines)
                {
                    const RoutineData& srcRdata = routineMap.find(rblock)->second;
                    // for next recursion pass call - choose origRvwSSAIdMap
                    // otherwise - standard rbwSsaIdMap
                    const SVRegMap& srcRbwSSAIdMap =
                        (entry.blockIndex.pass == 0 && rblock.pass!=0) ?
                        srcRdata.origRbwSSAIdMap : srcRdata.rbwSSAIdMap;
                    if (entry.blockIndex.pass == 0 && rblock.pass!=0)
                        ARDOut << "choose origRbwSSAIdMap: " << rblock << "\n";
                        
                    for (const auto& rbw: srcRbwSSAIdMap)
                    {
                        allInCalls.insert(rbw.first);
                        auto lsit = srcRdata.lastSSAIdMap.find(rbw.first);
                        if (lsit != srcRdata.lastSSAIdMap.end() &&
                             lsit->second.hasValue(rbw.second))
                            // if returned not modified, then do not clear this regvar
                            toNotClear.insert(rbw.first);
                    }
                    for (const auto& rbw: srcRdata.lastSSAIdMap)
                        allInCalls.insert(rbw.first);
                }
                for (auto& entry: rdata.curSSAIdMap)
                    // if any called routine and if to clear
                    if (allInCalls.find(entry.first) != allInCalls.end() &&
                        toNotClear.find(entry.first) == toNotClear.end())
                        // not found
                        entry.second.clear();
            
                for (BlockIndex rblock: calledRoutines)
                    joinRoutineData(rdata, routineMap.find(rblock)->second,
                                    curSSAIdMap, rdata.notFirstReturn);
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            const BlockIndex nextBlock = { cblock.nexts[entry.nextIndex].block,
                        entry.blockIndex.pass };
            flowStack.push_back({ nextBlock, 0 });
            // negate - if true (already in flowstack, then popping keep this state)
            if (!fromSubroutine)
                flowStackBlocks[nextBlock] = !flowStackBlocks[nextBlock];
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            if (entry.nextIndex!=0) // if back from calls (just return from calls)
            {
                for (const NextBlock& next: cblock.nexts)
                    if (next.isCall)
                    {
                        //ARDOut << "joincall:"<< next.block << "\n";
                        BlockIndex rblock(next.block, entry.blockIndex.pass);
                        if (callBlocks.find(next.block) != callBlocks.end())
                            rblock.pass = 1;
                        auto it = routineMap.find(rblock); // must find
                        initializePrevRetSSAIds(curSSAIdMap, retSSAIdMap,
                                    it->second, entry);
                        
                        joinRetSSAIdMap(retSSAIdMap, it->second.lastSSAIdMap, rblock);
                    }
            }
            const BlockIndex nextBlock = entry.blockIndex+1;
            flowStack.push_back({ nextBlock, 0 });
            // negate - if true (already in flowstack, then popping keep this state)
            if (!fromSubroutine)
                flowStackBlocks[nextBlock] = !flowStackBlocks[nextBlock];
            entry.nextIndex++;
        }
        else
        {
            ARDOut << "popstart: " << entry.blockIndex << "\n";
            if (cblock.haveReturn)
            {
                ARDOut << "procret: " << entry.blockIndex << "\n";
                joinLastSSAIdMap(rdata.lastSSAIdMap, rdata.curSSAIdMap);
                ARDOut << "procretend\n";
                rdata.notFirstReturn = true;
                entry.haveReturn = true;
                haveReturnBlocks.insert(entry.blockIndex);
            }
            
            const bool curHaveReturn = entry.haveReturn;
            
            // revert retSSAIdMap
            revertRetSSAIdMap(curSSAIdMap, retSSAIdMap, entry, &rdata);
            //
            
            for (const auto& ssaEntry: cblock.ssaInfoMap)
            {
                if (ssaEntry.first.regVar == nullptr)
                    continue;
                size_t curSSAId = ssaEntry.second.ssaIdBefore+1;
                size_t nextSSAId = (ssaEntry.second.ssaIdLast != SIZE_MAX) ?
                    ssaEntry.second.ssaIdLast+1 : curSSAId;
                curSSAIdMap[ssaEntry.first] = curSSAId;
                
                ARDOut << "popcurnext: " << ssaEntry.first.regVar <<
                            ":" << ssaEntry.first.index << ": " <<
                            nextSSAId << ", " << curSSAId << "\n";
                
                updateRoutineCurSSAIdMap(&rdata, ssaEntry, entry, curSSAId, nextSSAId);
            }
            
            activeLoops.eraseValue(entry.blockIndex);
            
            auto loopsit2 = rdata.loopEnds.find(entry.blockIndex);
            if (loopBlocks.find(entry.blockIndex) != loopBlocks.end())
            {
                if (loopsit2 != rdata.loopEnds.end())
                {
                    ARDOut << "   mark loopblocks passed: " <<
                                entry.blockIndex << "\n";
                    // mark that loop has passed fully
                    loopsit2->second.passed = true;
                }
                else
                    ARDOut << "   loopblocks nopassed: " <<
                                entry.blockIndex << "\n";
            }
            
            if ((!noMainLoop || flowStack.size() > 1) &&
                subroutToCache.count(entry.blockIndex)!=0)
            { //put to cache
                ARDOut << "-- subrcache for " << entry.blockIndex << "\n";
                addSubroutine(loopsit2, true);
            }
            
            if (!fromSubroutine)
                flowStackBlocks[entry.blockIndex] = false;
            ARDOut << "pop: " << entry.blockIndex << "\n";
            
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
    
    ARDOut << "--------- createRoutineData end ------------\n";
}

void AsmRegAllocator::createSSAData(ISAUsageHandler& usageHandler,
                ISALinearDepHandler& linDepHandler)
{
    if (codeBlocks.empty())
        return;
    auto cbit = codeBlocks.begin();
    AsmRegVarUsage rvu;
    ISAUsageHandler::ReadPos usagePos{ 0, 0 };
    
    if (!usageHandler.hasNext(usagePos))
        return; // do nothing if no regusages
    ISAUsageHandler::ReadPos oldReadPos = usagePos;
    // old linear deps position
    rvu = usageHandler.nextUsage(usagePos);
    
    cxuint regRanges[MAX_REGTYPES_NUM*2];
    size_t regTypesNum;
    assembler.isaAssembler->getRegisterRanges(regTypesNum, regRanges);
    
    while (true)
    {
        while (cbit != codeBlocks.end() && cbit->end <= rvu.offset)
        {
            cbit->usagePos = oldReadPos;
            ++cbit;
        }
        if (cbit == codeBlocks.end())
            break;
        // skip rvu's before codeblock
        while (rvu.offset < cbit->start && usageHandler.hasNext(usagePos))
        {
            oldReadPos = usagePos;
            rvu = usageHandler.nextUsage(usagePos);
        }
        if (rvu.offset < cbit->start)
            break;
        
        cbit->usagePos = oldReadPos;
        std::unordered_map<AsmSingleVReg, SSAInfo> ssaInfoMap;
        while (rvu.offset < cbit->end)
        {
            // process rvu
            // only if regVar
            for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
            {
                auto res = ssaInfoMap.insert(
                        { AsmSingleVReg{ rvu.regVar, rindex }, SSAInfo() });
                
                SSAInfo& sinfo = res.first->second;
                if (res.second)
                    sinfo.firstPos = rvu.offset;
                sinfo.lastPos = rvu.offset;
                
                const bool writeWithSSA = checkWriteWithSSA(rvu);
                if (!writeWithSSA && (sinfo.ssaIdChange == 0 ||
                    // if first write RVU instead read RVU
                    (sinfo.ssaIdChange == 1 && sinfo.firstPos==rvu.offset)))
                    sinfo.readBeforeWrite = true;
                /* change SSA id only for write-only regvars -
                 *   read-write place can not have two different variables */
                if (writeWithSSA)
                    sinfo.ssaIdChange++;
                if (rvu.regVar==nullptr)
                    sinfo.ssaIdBefore = sinfo.ssaIdFirst =
                            sinfo.ssaId = sinfo.ssaIdLast = 0;
            }
            
            // get next rvusage
            if (!usageHandler.hasNext(usagePos))
                break;
            oldReadPos = usagePos;
            rvu = usageHandler.nextUsage(usagePos);
        }
        // prepping ssaInfoMap array in cblock (put and sorting)
        cbit->ssaInfoMap.resize(ssaInfoMap.size());
        std::copy(ssaInfoMap.begin(), ssaInfoMap.end(), cbit->ssaInfoMap.begin());
        mapSort(cbit->ssaInfoMap.begin(), cbit->ssaInfoMap.end());
        
        ++cbit;
    }
    oldReadPos = usagePos;
    // fill up remaining codeblocks oldReadPos
    for (; cbit != codeBlocks.end(); ++cbit)
        cbit->usagePos = oldReadPos;
    
    size_t rbwCount = 0;
    size_t wrCount = 0;
    
    SimpleCache<BlockIndex, RoutineData> subroutinesCache(codeBlocks.size()<<3);
    
    std::deque<CallStackEntry> callStack;
    std::deque<FlowStackEntry> flowStack;
    // total SSA count
    SVRegMap totalSSACountMap;
    // last SSA ids map from returns
    RetSSAIdMap retSSAIdMap;
    // last SSA ids in current way in code flow
    SVRegMap curSSAIdMap;
    // routine map - routine datas map, value - last SSA ids map
    RoutineMap routineMap;
    // key - current res first key, value - previous first key and its flowStack pos
    PrevWaysIndexMap prevWaysIndexMap;
    // to track ways last block indices pair: block index, flowStackPos)
    std::pair<size_t, size_t> lastCommonCacheWayPoint{ SIZE_MAX, SIZE_MAX };
    
    CBlockBitPool waysToCache(codeBlocks.size(), false);
    // used for detect loop code blocks (blocks where is begin loop)
    CBlockBitPool flowStackBlocks(codeBlocks.size(), false);
    
    // prevCallFlowStackBlocks - is always empty for createRoutineData
    CBlockBitPool prevCallFlowStackBlocks;
    // callFlowStackBlocks - flowStackBlocks from call
    CBlockBitPool callFlowStackBlocks(codeBlocks.size(), false);
    // subroutToCache - true if given block begin subroutine to cache
    ResSecondPointsToCache cblocksToCache(codeBlocks.size());
    CBlockBitPool visited(codeBlocks.size(), false);
    flowStack.push_back({ 0, 0 });
    flowStackBlocks[0] = true;
    std::unordered_set<BlockIndex> callBlocks;
    std::unordered_set<BlockIndex> loopBlocks;
    std::unordered_set<size_t> recurseBlocks;
    
    /** INFO if you want to get code changedRegVars between recursions you get 3984
     * this stuff has been deleted */
    
    std::unordered_map<size_t, SVRegMap > curSSAIdMapStateMap;
    
    /*
     * main loop to fill up ssaInfos
     */
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex.index];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex])
            {
                ARDOut << "proc: " << entry.blockIndex << "\n";
                visited[entry.blockIndex] = true;
                
                for (auto& ssaEntry: cblock.ssaInfoMap)
                {
                    SSAInfo& sinfo = ssaEntry.second;
                    if (ssaEntry.first.regVar==nullptr)
                    {
                        // TODO - pass registers through SSA marking and resolving
                        sinfo.ssaIdChange = 0; // zeroing SSA changes
                        continue; // no change for registers
                    }
                    
                    if (sinfo.ssaId != SIZE_MAX)
                    {
                        // already initialized
                        reduceSSAIds(curSSAIdMap, retSSAIdMap,
                                routineMap, ssaReplacesMap, entry, ssaEntry);
                        if (sinfo.ssaIdChange!=0)
                            curSSAIdMap[ssaEntry.first] = sinfo.ssaIdLast+1;
                        
                        // count read before writes (for cache weight)
                        if (sinfo.readBeforeWrite)
                            rbwCount++;
                        if (sinfo.ssaIdChange!=0)
                            wrCount++;
                        continue;
                    }
                    
                    reduceSSAIds(curSSAIdMap, retSSAIdMap, routineMap, ssaReplacesMap,
                                 entry, ssaEntry);
                    
                    size_t& ssaId = curSSAIdMap[ssaEntry.first];
                    
                    size_t& totalSSACount = totalSSACountMap[ssaEntry.first];
                    if (totalSSACount == 0)
                    {
                        // first read before write at all, need change totalcount, ssaId
                        ssaId++;
                        totalSSACount++;
                    }
                    
                    sinfo.ssaId = totalSSACount;
                    sinfo.ssaIdFirst = sinfo.ssaIdChange!=0 ? totalSSACount : SIZE_MAX;
                    sinfo.ssaIdBefore = ssaId-1;
                    
                    totalSSACount += sinfo.ssaIdChange;
                    sinfo.ssaIdLast = sinfo.ssaIdChange!=0 ? totalSSACount-1 : SIZE_MAX;
                    //totalSSACount = std::max(totalSSACount, ssaId);
                    if (sinfo.ssaIdChange!=0)
                        ssaId = totalSSACount;
                    
                    // count read before writes (for cache weight)
                    if (sinfo.readBeforeWrite)
                        rbwCount++;
                    if (sinfo.ssaIdChange!=0)
                        wrCount++;
                }
            }
            else
            {
                size_t pass = entry.blockIndex.pass;
                cblocksToCache.increase(entry.blockIndex);
                ARDOut << "cblockToCache: " << entry.blockIndex << "=" <<
                            cblocksToCache.count(entry.blockIndex) << "\n";
                // back, already visited
                flowStackBlocks[entry.blockIndex] = !flowStackBlocks[entry.blockIndex];
                flowStack.pop_back();
                
                if (pass != 0)
                    continue;
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
            RoutineData& prevRdata = routineMap.find(routineBlock)->second;
            if (!prevRdata.generated)
            {
                createRoutineData(codeBlocks, curSSAIdMap, loopBlocks, callBlocks,
                            cblocksToCache, subroutinesCache, routineMap, prevRdata,
                            routineBlock, prevCallFlowStackBlocks, callFlowStackBlocks);
                prevRdata.generated = true;
                
                auto csimsmit = curSSAIdMapStateMap.find(routineBlock.index);
                if (csimsmit != curSSAIdMapStateMap.end() && entry.blockIndex.pass==0)
                {
                    ARDOut << " get curSSAIdMap from back recur 2\n";
                    curSSAIdMap = csimsmit->second;
                    curSSAIdMapStateMap.erase(csimsmit);
                }
            }
            
            callStack.pop_back(); // just return from call
            callBlocks.erase(routineBlock);
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            bool isCall = false;
            BlockIndex nextBlock = cblock.nexts[entry.nextIndex].block;
            nextBlock.pass = entry.blockIndex.pass;
            if (cblock.nexts[entry.nextIndex].isCall)
            {
                if (!callBlocks.insert(nextBlock).second)
                {
                    // if already called (then it is recursion)
                    if (recurseBlocks.insert(nextBlock.index).second)
                    {
                        ARDOut << "   -- recursion: " << nextBlock << "\n";
                        nextBlock.pass = 1;
                        
                        curSSAIdMapStateMap.insert({ nextBlock.index,  curSSAIdMap });
                    }
                    else if (entry.blockIndex.pass==1)
                    {
                        entry.nextIndex++;
                        ARDOut << " NO call (rec): " << entry.blockIndex << "\n";
                        continue;
                    }
                }
                else if (entry.blockIndex.pass==1 &&
                    recurseBlocks.find(nextBlock.index) != recurseBlocks.end())
                {
                    entry.nextIndex++;
                    ARDOut << " NO call (rec)2: " << entry.blockIndex << "\n";
                    continue;
                }
                
                ARDOut << " call: " << entry.blockIndex << "\n";
                
                callStack.push_back({ entry.blockIndex, entry.nextIndex, nextBlock });
                routineMap.insert({ nextBlock, { } });
                isCall = true;
            }
            
            flowStack.push_back({ nextBlock, 0, isCall });
            if (flowStackBlocks[nextBlock])
            {
                if (!cblock.nexts[entry.nextIndex].isCall)
                    loopBlocks.insert(nextBlock);
                flowStackBlocks[nextBlock] = false; // keep to inserted in popping
            }
            else
                flowStackBlocks[nextBlock] = true;
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            if (entry.nextIndex!=0) // if back from calls (just return from calls)
            {
                reduceSSAIdsForCalls(entry, codeBlocks, retSSAIdMap, routineMap,
                                     ssaReplacesMap);
                //
                for (const NextBlock& next: cblock.nexts)
                    if (next.isCall)
                    {
                        //ARDOut << "joincall:"<< next.block << "\n";
                        size_t pass = 0;
                        if (callBlocks.find(next.block) != callBlocks.end())
                        {
                            ARDOut << " is secpass: " << entry.blockIndex << " : " <<
                                    next.block << "\n";
                            pass = 1; // it ways second pass
                        }
                        
                        auto it = routineMap.find({ next.block, pass }); // must find
                        initializePrevRetSSAIds(curSSAIdMap, retSSAIdMap,
                                    it->second, entry);
                        
                        BlockIndex rblock(next.block, entry.blockIndex.pass);
                        if (callBlocks.find(next.block) != callBlocks.end())
                            rblock.pass = 1;
                        
                        joinRetSSAIdMap(retSSAIdMap, it->second.lastSSAIdMap, rblock);
                    }
            }
            
            flowStack.push_back({ entry.blockIndex+1, 0, false });
            if (flowStackBlocks[entry.blockIndex+1])
            {
                loopBlocks.insert(entry.blockIndex+1);
                 // keep to inserted in popping
                flowStackBlocks[entry.blockIndex+1] = false;
            }
            else
                flowStackBlocks[entry.blockIndex+1] = true;
            entry.nextIndex++;
        }
        else // back
        {
            // revert retSSAIdMap
            revertRetSSAIdMap(curSSAIdMap, retSSAIdMap, entry, nullptr);
            //
            
            for (const auto& ssaEntry: cblock.ssaInfoMap)
            {
                if (ssaEntry.first.regVar == nullptr)
                    continue;
                
                size_t& curSSAId = curSSAIdMap[ssaEntry.first];
                const size_t nextSSAId = curSSAId;
                curSSAId = ssaEntry.second.ssaIdBefore+1;
                
                ARDOut << "popcurnext: " << ssaEntry.first.regVar <<
                            ":" << ssaEntry.first.index << ": " <<
                            nextSSAId << ", " << curSSAId << "\n";
            }
            
            ARDOut << "pop: " << entry.blockIndex << "\n";
            flowStackBlocks[entry.blockIndex] = false;
            
            if (!flowStack.empty() && flowStack.back().isCall)
            {
                auto csimsmit = curSSAIdMapStateMap.find(entry.blockIndex.index);
                if (csimsmit != curSSAIdMapStateMap.end())
                {
                    ARDOut << " get curSSAIdMap from back recur\n";
                    curSSAIdMap = csimsmit->second;
                }
            }
            
            flowStack.pop_back();
            
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
    
    /**********
     * after that, we find points to resolve conflicts
     **********/
    flowStack.clear();
    std::deque<FlowStackEntry2> flowStack2;
    
    std::fill(visited.begin(), visited.end(), false);
    flowStack2.push_back({ 0, 0 });
    
    SimpleCache<size_t, LastSSAIdMap> resFirstPointsCache(wrCount<<1);
    SimpleCache<size_t, RBWSSAIdMap> resSecondPointsCache(rbwCount<<1);
    
    while (!flowStack2.empty())
    {
        FlowStackEntry2& entry = flowStack2.back();
        CodeBlock& cblock = codeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex])
                visited[entry.blockIndex] = true;
            else
            {
                resolveSSAConflicts(flowStack2, routineMap, codeBlocks,
                            prevWaysIndexMap, waysToCache, cblocksToCache,
                            resFirstPointsCache, resSecondPointsCache, ssaReplacesMap);
                
                // back, already visited
                flowStack2.pop_back();
                continue;
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            flowStack2.push_back({ cblock.nexts[entry.nextIndex].block, 0 });
            entry.nextIndex++;
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            flowStack2.push_back({ entry.blockIndex+1, 0 });
            entry.nextIndex++;
        }
        else // back
        {
            if (cblocksToCache.count(entry.blockIndex)==2 &&
                !resSecondPointsCache.hasKey(entry.blockIndex))
                // add to cache
                addResSecCacheEntry(routineMap, codeBlocks, resSecondPointsCache,
                            entry.blockIndex);
            flowStack2.pop_back();
        }
    }
}
