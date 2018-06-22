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

#ifndef __CLRX_ASMREGALLOC_H__
#define __CLRX_ASMREGALLOC_H__

#include <CLRX/Config.h>
#include <iostream>
#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/DTree.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

#define ASMREGALLOC_DEBUGDUMP 0

/* svreg - single regvar register
 * ssaId - SSA id for svreg
 * vidx - virtual variable index, refer to some ssaId for some svreg
 */

namespace CLRX
{
    
/* (rvu.rwFlags != ASMRVU_WRITE || rvu.regField==ASMFIELD_NONE) -
 *    treat read-write as read (because is not possible to change assign different
 *    new SSA in single field),
 *    and treat write as read for userusage because field to assign new SSA is not
 *    specified.
 *  additionally use (rvu.rwFlags == ASMRVU_WRITE && rvu.regField!=ASMFIELD_NONE)
 *   to check whether is write with possible new SSA assignment
 * */
static inline bool checkWriteWithSSA(const AsmRegVarUsage& rvu)
{
    return (rvu.rwFlags == ASMRVU_WRITE && rvu.regField!=ASMFIELD_NONE);
}

static inline bool checkNoWriteWithSSA(const AsmRegVarUsage& rvu)
{
    return (rvu.rwFlags != ASMRVU_WRITE || rvu.regField==ASMFIELD_NONE);
}

typedef AsmRegAllocator::CodeBlock CodeBlock;
typedef AsmRegAllocator::NextBlock NextBlock;
typedef AsmRegAllocator::SSAInfo SSAInfo;
typedef std::pair<AsmSingleVReg, SSAInfo> SSAEntry;
typedef AsmRegAllocator::VIdxSetEntry VIdxSetEntry;

//  BlockIndex

struct CLRX_INTERNAL BlockIndex
{
    size_t index;
    size_t pass;
    
    BlockIndex(size_t _index = 0, size_t _pass = 0)
            : index(_index), pass(_pass)
    { }
    
    bool operator==(const BlockIndex& v) const
    { return index==v.index && pass==v.pass; }
    bool operator!=(const BlockIndex& v) const
    { return index!=v.index || pass!=v.pass; }
    
    BlockIndex operator+(size_t p) const
    { return BlockIndex(index+p, pass); }
};

};

#if ASMREGALLOC_DEBUGDUMP
extern CLRX_INTERNAL std::ostream& operator<<(std::ostream& os, const CLRX::BlockIndex& v);
#endif

namespace std
{

template<>
struct hash<BlockIndex>
{
    typedef BlockIndex argument_type;    ///< argument type
    typedef std::size_t result_type;    ///< result type
    
    /// a calling operator
    size_t operator()(const BlockIndex& r1) const
    {
        std::hash<size_t> h1;
        return h1(r1.index) ^ h1(r1.pass);
    }
};

}

namespace CLRX
{

class CLRX_INTERNAL CBlockBitPool: public std::vector<bool>
{
public:
    CBlockBitPool(size_t n = 0, bool v = false) : std::vector<bool>(n<<1, v)
    { }
    
    reference operator[](BlockIndex i)
    { return std::vector<bool>::operator[](i.index + (i.pass ? (size()>>1) : 0)); }
    const_reference operator[](BlockIndex i) const
    { return std::vector<bool>::operator[](i.index + (i.pass ? (size()>>1) : 0)); }
};

/** Simple cache **/

// map of last SSAId for routine, key - varid, value - last SSA ids
class CLRX_INTERNAL LastSSAIdMap: public
            std::unordered_map<AsmSingleVReg, VectorSet<size_t> >
{
public:
    LastSSAIdMap()
    { }
    
    iterator insertSSAId(const AsmSingleVReg& vreg, size_t ssaId)
    {
        auto res = insert({ vreg, { ssaId } });
        if (!res.second)
            res.first->second.insertValue(ssaId);
        return res.first;
    }
    
    void eraseSSAId(const AsmSingleVReg& vreg, size_t ssaId)
    {
        auto it = find(vreg);
        if (it != end())
             it->second.eraseValue(ssaId);
    }
    
    size_t weight() const
    { return size(); }
};

typedef std::unordered_map<AsmSingleVReg, BlockIndex> SVRegBlockMap;

class CLRX_INTERNAL SVRegMap: public std::unordered_map<AsmSingleVReg, size_t>
{
public:
    SVRegMap()
    { }
    
    size_t weight() const
    { return size(); }
};

typedef LastSSAIdMap RBWSSAIdMap;
typedef std::unordered_map<BlockIndex, VectorSet<BlockIndex> > SubrLoopsMap;

struct CLRX_INTERNAL RetSSAEntry
{
    std::vector<BlockIndex> routines;
    VectorSet<size_t> ssaIds;
    size_t prevSSAId; // for curSSAId
};

typedef std::unordered_map<AsmSingleVReg, RetSSAEntry> RetSSAIdMap;

struct CLRX_INTERNAL LoopSSAIdMap
{
    LastSSAIdMap ssaIdMap;
    bool passed;
};

struct CLRX_INTERNAL RoutineData
{
    // rbwSSAIdMap - read before write SSAId's map
    SVRegMap rbwSSAIdMap;
    SVRegMap origRbwSSAIdMap;
    LastSSAIdMap curSSAIdMap;
    LastSSAIdMap lastSSAIdMap;
    // key - loop block, value - last ssaId map for loop end
    std::unordered_map<BlockIndex, LoopSSAIdMap> loopEnds;
    bool notFirstReturn;
    bool generated;
    size_t weight_;
    
    RoutineData() : notFirstReturn(false), generated(false), weight_(0)
    { }
    
    void calculateWeight()
    {
        weight_ = rbwSSAIdMap.size() + lastSSAIdMap.weight();
        for (const auto& entry: loopEnds)
            weight_ += entry.second.ssaIdMap.weight();
    }
    
    size_t weight() const
    { return weight_; }
};

struct CLRX_INTERNAL LastAccessBlockPos
{
    size_t blockIndex;
    bool inSubroutines; // true if last access in some called subroutine
    
    bool operator==(const LastAccessBlockPos& v) const
    { return blockIndex==v.blockIndex; }
    bool operator!=(const LastAccessBlockPos& v) const
    { return blockIndex!=v.blockIndex; }
};

struct CLRX_INTERNAL LastVRegStackPos
{
    size_t stackPos;
    bool inSubroutines; // true if last access in some called subroutine
};

typedef std::unordered_map<AsmSingleVReg, VectorSet<LastAccessBlockPos> > LastAccessMap;
typedef std::unordered_map<AsmSingleVReg, LastAccessBlockPos> RoutineCurAccessMap;
class CLRX_INTERNAL LastStackPosMap
        : public std::unordered_map<AsmSingleVReg, LastVRegStackPos>
{
public:
    LastStackPosMap()
    { }
    
    size_t weight() const
    { return size(); }
};

// Routine data for createLivenesses - holds svreg read before writes and
// last access of the svregs
struct CLRX_INTERNAL RoutineDataLv
{
    SVRegMap rbwSSAIdMap;
    // holds all vreg SSA's used in routine (used while creating call point)
    // includes subroutines called in this routines
    std::unordered_set<size_t> allSSAs[MAX_REGTYPES_NUM];
    // key - svreg, value - list of the last codeblocks where is svreg
    LastAccessMap lastAccessMap;
    std::unordered_set<size_t> haveReturnBlocks;
    bool fromSecondPass;
};

// used by createSSAData
struct CLRX_INTERNAL FlowStackEntry
{
    BlockIndex blockIndex;
    size_t nextIndex;
    bool isCall;
    bool haveReturn;
    RetSSAIdMap prevRetSSAIdSets;
};

// used by resolveSSAConflicts
struct CLRX_INTERNAL FlowStackEntry2
{
    size_t blockIndex;
    size_t nextIndex;
};


// used by createLivenesses
struct CLRX_INTERNAL FlowStackEntry3
{
    BlockIndex blockIndex;
    size_t nextIndex;
    bool isCall;
    bool havePath;
};

struct CLRX_INTERNAL FlowStackEntry4
{
    size_t blockIndex;
    size_t nextIndex;
    bool isCall;
    bool haveReturn;
    RoutineCurAccessMap prevCurSVRegMap;
};

struct CLRX_INTERNAL CallStackEntry
{
    BlockIndex callBlock; // index
    size_t callNextIndex; // index of call next
    BlockIndex routineBlock;    // routine block
};

typedef std::unordered_map<BlockIndex, RoutineData> RoutineMap;
typedef std::unordered_map<BlockIndex, RoutineDataLv> RoutineLvMap;

typedef std::unordered_map<size_t, std::pair<size_t, size_t> > PrevWaysIndexMap;

class CLRX_INTERNAL ResSecondPointsToCache: public CBlockBitPool
{
public:
    explicit ResSecondPointsToCache(size_t n) : CBlockBitPool(n<<1, false)
    { }
    
    void increase(BlockIndex ip)
    {
        const size_t i = ip.index + (ip.pass ? (size()>>2) : 0);
        if ((*this)[i<<1])
            (*this)[(i<<1)+1] = true;
        else
            (*this)[i<<1] = true;
    }
    
    cxuint count(BlockIndex ip) const
    {
        const size_t i = ip.index + (ip.pass ? (size()>>2) : 0);
        return cxuint((*this)[i<<1]) + (*this)[(i<<1)+1];
    }
};

typedef AsmRegAllocator::SSAReplace SSAReplace; // first - orig ssaid, second - dest ssaid
typedef AsmRegAllocator::SSAReplacesMap SSAReplacesMap;

#if ASMREGALLOC_DEBUGDUMP
#define ARDOut std::cout
#else
struct NoOutput
{
    template<typename T>
    NoOutput& operator<<(const T& v)
    { return *this; }
};

#define ARDOut NoOutput()
#endif

struct CLRX_INTERNAL Liveness
{
    DTreeMap<size_t, size_t> l;
    
    Liveness() { }
    
    void clear()
    { l.clear(); }
    
    void expand(size_t k)
    {
        auto it = l.lower_bound(k);
        if (it != l.begin())
            --it;
        else // do nothing
            return;
        // try expand, lower new bound, then use older
        it->second = std::max(it->second, k);
        auto nextIt = it;
        ++nextIt;
        if (nextIt != l.end() && nextIt->first <= it->second)
        {
            // join if needed
            it->second = nextIt->second;
            l.erase(nextIt);
        }
    }
    
    void insert(size_t k, size_t k2)
    {
        auto it = l.upper_bound(k);
        bool doCheckRight = false;
        if (it != l.begin())
        {
            auto prevIt = it;
            --prevIt;
            if (prevIt->second >= k)
            {
                if (it != l.end() && it->first <= k2)
                {
                    // join
                    prevIt->second = it->second;
                    l.erase(it);
                }
                else
                    prevIt->second = std::max(k2, prevIt->second);
            }
            else
                doCheckRight = true;
        }
        else
            doCheckRight = true;
        
        if (doCheckRight)
        {
            if (it != l.end() && it->first <= k2)
            {
                //it = l.insert(it, std::make_pair(k, it->second));
                //l.erase(++it);
                l.replace(it, std::make_pair(k, it->second));
            }
            else
                //l.insert(it, std::make_pair(k, k2));
                l.insert(std::make_pair(k, k2));
        }
    }
    
    bool contain(size_t t) const
    {
        if (l.empty())
            return false;
        auto it = l.lower_bound(t);
        if (it==l.begin() && it->first>t)
            return false;
        if (it==l.end() || it->first>t)
            --it;
        return it->first<=t && t<it->second;
    }
};

typedef AsmRegAllocator::VarIndexMap VarIndexMap;

typedef std::deque<FlowStackEntry3>::const_iterator FlowStackCIter;

// key - singlevreg, value - code block chain
typedef std::unordered_map<AsmSingleVReg, std::vector<LastVRegStackPos> > LastVRegMap;

struct CLRX_INTERNAL LiveBlock
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
typedef std::unordered_map<size_t, LinearDep> LinearDepMap;

typedef AsmRegAllocator::InterGraph InterGraph;

struct CLRX_INTERNAL SDOLDOCompare
{
    const InterGraph& interGraph;
    const Array<size_t>& sdoCounts;
    
    SDOLDOCompare(const InterGraph& _interGraph, const Array<size_t>&_sdoCounts)
        : interGraph(_interGraph), sdoCounts(_sdoCounts)
    { }
    
    bool operator()(size_t a, size_t b) const
    {
        if (sdoCounts[a] > sdoCounts[b])
            return true;
        return interGraph[a].size() > interGraph[b].size();
    }
};

};

namespace std
{

template<>
struct hash<LastAccessBlockPos>
{
    typedef LastAccessBlockPos argument_type;    ///< argument type
    typedef std::size_t result_type;    ///< result type
    
    /// a calling operator
    size_t operator()(const LastAccessBlockPos& r1) const
    {
        std::hash<size_t> h1;
        std::hash<bool> h2;
        return h1(r1.blockIndex) ^ h2(r1.inSubroutines);
    }
};

}

#endif
