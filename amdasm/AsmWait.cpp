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
#include <vector>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmRegAlloc.h"

using namespace CLRX;

ISAWaitHandler::ISAWaitHandler()
{ }

ISAWaitHandler* ISAWaitHandler::copy() const
{
    return new ISAWaitHandler(*this);
}

bool ISAWaitHandler::nextInstr(ReadPos& readPos,
                AsmDelayedOp& delOp, AsmWaitInstr& waitInstr)
{
    size_t delResOffset = SIZE_MAX;
    size_t waitInstrOffset = SIZE_MAX;
    if (readPos.delOpPos < delayedOps.size())
        delResOffset = delayedOps[readPos.delOpPos].offset;
    if (readPos.waitInstrPos < waitInstrs.size())
        waitInstrOffset = waitInstrs[readPos.waitInstrPos].offset;
    if (delResOffset < waitInstrOffset)
    {
        delOp = delayedOps[readPos.delOpPos++];
        return false;
    }
    waitInstr = waitInstrs[readPos.waitInstrPos++];
    return true;
}

ISAWaitHandler::ReadPos ISAWaitHandler::findPositionByOffset(size_t offset) const
{
    const size_t dPos = std::lower_bound(delayedOps.begin(), delayedOps.end(),
        AsmDelayedOp{ offset }, [](const AsmDelayedOp& a, const AsmDelayedOp& b)
        { return a.offset < b.offset; }) - delayedOps.begin();
    const size_t wPos = std::lower_bound(waitInstrs.begin(), waitInstrs.end(),
        AsmWaitInstr{ offset }, [](const AsmWaitInstr& a, const AsmWaitInstr& b)
        { return a.offset < b.offset; }) - waitInstrs.begin();
    return ISAWaitHandler::ReadPos{ dPos, wPos };
}

/* AsmWaitScheduler */

// QReg - queue register - contain - reg index and access type (read or write)
static inline uint16_t qregVal(uint16_t reg, bool write)
{ return reg | (write ? 0x8000 : 0); }

static inline bool qregWrite(uint16_t qreg)
{ return (qreg & 0x8000)!=0; }

static inline uint16_t qregReg(uint16_t qreg)
{ return qreg & 0x7fff; }

namespace CLRX
{

struct QueueEntry1
{
    std::unordered_set<uint16_t> regs;
    bool haveDelayedOp;
    
    QueueEntry1() : haveDelayedOp(false)
    { }
    
    bool empty() const
    { return haveDelayedOp || !regs.empty(); }
    
    void join(const QueueEntry1& b)
    {
        regs.insert(b.regs.begin(), b.regs.end());
        haveDelayedOp |= b.haveDelayedOp;
    }
    
    void joinWithRegPlaces(const QueueEntry1& b, uint16_t startPos, uint16_t opos,
            std::unordered_map<uint16_t, uint16_t>& regPlaces)
    {
        for (auto e: b.regs)
            if (regs.insert(e).second)
            {
                auto rres = regPlaces.insert(std::make_pair(e, opos));
                if (rres.second && rres.first->second-startPos < opos-startPos)
                    rres.first->second = opos;
            }
        regs.insert(b.regs.begin(), b.regs.end());
        haveDelayedOp |= b.haveDelayedOp;
    }
};

static inline void updateRegPlaces(const QueueEntry1& b, uint16_t startPos, uint16_t opos,
            std::unordered_map<uint16_t, uint16_t>& regPlaces)
{
    for (auto e: b.regs)
    {
        auto rres = regPlaces.insert(std::make_pair(e, opos));
        if (rres.second && rres.first->second-startPos < opos-startPos)
            rres.first->second = opos;
    }
}


struct CLRX_INTERNAL QueueState1
{
    cxuint maxQueueSize;
    uint16_t orderedStartPos;
    std::deque<QueueEntry1> ordered;  // ordered items
    QueueEntry1 random;   // items in random order
    // register place in queue - key - reg, value - position
    std::unordered_map<uint16_t, uint16_t> regPlaces;
    // request queue size at start of block (by enqueuing and waiting/flushing)
    // used while joining with previous block
    cxuint requestedQueueSize;
    bool firstFlush;
    bool reallyFlushed; // if really already flushed (queue size has been shrinked)
    
    QueueState1(cxuint _maxQueueSize = 0) : maxQueueSize(_maxQueueSize),
                orderedStartPos(0), requestedQueueSize(0),
                firstFlush(true), reallyFlushed(false)
    { }
    
    void setMaxQueueSize(cxuint _maxQueueSize)
    { maxQueueSize = _maxQueueSize; }
    
    void pushOrdered(uint16_t reg)
    {
        if (reg == UINT16_MAX)
        {
            // push same delayed op (no reg)
            ordered.back().haveDelayedOp = true;
            return;
        }
        regPlaces[reg] = orderedStartPos + ordered.size()-1;
        ordered.back().regs.insert(reg);
    }
    
    void pushRandom(uint16_t reg)
    {
        if (reg != UINT16_MAX)
            random.regs.insert(reg);
        else
            // push same delayed op (no reg)
            random.haveDelayedOp = true;
    }
    
    void nextEntry()
    {
        if (ordered.empty() || !ordered.back().empty())
            // push only if empty or previous is filled
            ordered.push_back(QueueEntry1());
        
        if (ordered.size() == maxQueueSize)
        {
            // move first entry in ordered queue to first ordered
            QueueEntry1& firstOrdered = ordered.front();
            for (auto e: firstOrdered.regs)
            {
                auto rpit = regPlaces.find(e);
                 // update regPlaces for firstOrdered before joining
                if (rpit->second == orderedStartPos)
                    rpit->second++;
            }
            auto ordit = ordered.begin();
            ++ordit; // second entry
            QueueEntry1& second = *ordit;
            // to second entry
            second.regs.insert(firstOrdered.regs.begin(), firstOrdered.regs.end());
            ordered.pop_front();
            orderedStartPos++; // next start pos for ordered queue for first entry
        }
        requestedQueueSize = std::min(requestedQueueSize+1, maxQueueSize);
    }
    void flushTo(cxuint size)
    {
        if (firstFlush && requestedQueueSize < size)
        {
            // if higher than queue size and higher than requested queue size
            requestedQueueSize = size;
            firstFlush = false;
            return;
        }
        reallyFlushed = (ordered.size() <= size);
        firstFlush = false;
        if (size == 0)
            random.regs.clear(); // clear randomly ordered if must be empty
        while (size < ordered.size())
        {
            for (auto e: ordered.front().regs)
            {
                auto prit = regPlaces.find(e);
                if (prit != regPlaces.end() && prit->second == orderedStartPos)
                regPlaces.erase(prit); // remove from regPlaces if in this place
            }
            ordered.pop_front();
            orderedStartPos++;
        }
        requestedQueueSize = std::min(size, requestedQueueSize);
    }
    
    uint16_t findMinQueueSizeForReg(uint16_t reg) const
    {
        auto it = regPlaces.find(reg);
        if (it == regPlaces.end())
            // if found, then 0 otherwize not found (UINT_MAX)
            return random.regs.find(reg) != random.regs.end() ? 0 : UINT16_MAX;
        const uint16_t pos = it->second - orderedStartPos;
        return ordered.size()-1 - cxuint(pos);
    }
    
    void joinWay(const QueueState1& way)
    {
        random.join(way.random);
        
        auto oit1 = ordered.begin();
        auto oit2 = way.ordered.begin();
        const int queueSizeDiff = way.ordered.size() - ordered.size();
        uint16_t qpos = orderedStartPos;
        if (queueSizeDiff > 0)
        {
            ordered.insert(ordered.begin(), way.ordered.begin(),
                    way.ordered.begin() + queueSizeDiff);
            oit2 += queueSizeDiff;
            orderedStartPos -= queueSizeDiff;
            oit1 = ordered.begin() + queueSizeDiff;
            qpos = orderedStartPos;
            // update regPlaces after pushing to front
            for (auto oitx = ordered.begin(); oitx != oit1; ++oitx, ++qpos)
                updateRegPlaces(*oitx, orderedStartPos, qpos, regPlaces);
        }
        else
        {
            oit1 -= queueSizeDiff;
            qpos -= queueSizeDiff;
        }
        // join entries
        for (; oit1 != ordered.end(); ++oit1, ++oit2, ++qpos)
            oit1->joinWithRegPlaces(*oit2, orderedStartPos, qpos, regPlaces);
        
        firstFlush |= way.firstFlush;
        requestedQueueSize = std::max(requestedQueueSize, way.requestedQueueSize);
    }
    
    bool joinPrev(const QueueState1& prev)
    {
        if (!reallyFlushed)
        {
            if (!prev.random.empty() && !prev.ordered.empty() &&
                prev.requestedQueueSize == 0)
                // no change
                return false;
            
            const cxuint prevOrderedSize = (requestedQueueSize!=prev.ordered.size() ?
                    requestedQueueSize-prev.ordered.size() : ordered.size());
            ordered.insert(ordered.begin(), prev.ordered.end() - prevOrderedSize,
                           prev.ordered.end());
            orderedStartPos -= prevOrderedSize;
            uint16_t qpos = orderedStartPos;
            auto oitend = ordered.begin() + prevOrderedSize;
            // update regplaces for front
            for (auto oitx = ordered.begin(); oitx != oitend; ++oitx, ++qpos)
                updateRegPlaces(*oitx, orderedStartPos, qpos, regPlaces);
            
            if (ordered.size() > maxQueueSize)
            {
                // push to first ordered
                size_t toFirst = ordered.size() - maxQueueSize;
                auto firstOrderedIt = ordered.begin() + toFirst;
                for (auto it = ordered.begin(); it!=firstOrderedIt; ++it)
                    firstOrderedIt->joinWithRegPlaces(*it, orderedStartPos,
                            orderedStartPos + toFirst, regPlaces);
                ordered.erase(ordered.begin(), firstOrderedIt);
                orderedStartPos += toFirst;
            }
            random.join(prev.random);
            requestedQueueSize = ordered.size();
            return true;
        }
        
        return false;
    }
};

struct CLRX_INTERNAL RRegInfo
{
    size_t offset;  /// offset where is usage
    uint16_t qsizes[ASM_WAIT_MAX_TYPES_NUM]; /// queue sizes
    uint16_t waits[ASM_WAIT_MAX_TYPES_NUM];  /// pending waits from block to this place
};

struct CLRX_INTERNAL WaitFlowStackEntry0
{
    size_t blockIndex;
    size_t nextIndex;
    bool isCall;
    bool haveReturn;
    
    QueueState1 queues[ASM_WAIT_MAX_TYPES_NUM];
    // key - reg, value - offset in codeblock
    std::unordered_map<uint16_t, RRegInfo> firstRegs;
    
    WaitFlowStackEntry0(size_t _blockIndex, size_t _nextIndex,
                        const AsmWaitConfig& waitConfig)
            : blockIndex(_blockIndex), nextIndex(_nextIndex), isCall(false)
    {
        setMaxQueueSizes(waitConfig);
    }
    
    WaitFlowStackEntry0(size_t _blockIndex, size_t _nextIndex, bool _isCall,
                        const AsmWaitConfig& waitConfig)
            : blockIndex(_blockIndex), nextIndex(_nextIndex), isCall(_isCall)
    {
        setMaxQueueSizes(waitConfig);
    }
    
    WaitFlowStackEntry0(size_t _blockIndex, size_t _nextIndex,
                        const WaitFlowStackEntry0& prev,
                        const AsmWaitConfig& waitConfig)
            : blockIndex(_blockIndex), nextIndex(_nextIndex)
    {
        setMaxQueueSizes(waitConfig);
        std::copy(prev.queues, prev.queues + waitConfig.waitQueuesNum, queues);
    }
    
    void setMaxQueueSizes(const AsmWaitConfig& waitConfig)
    {
        for (cxuint i = 0; i < waitConfig.waitQueuesNum; i++)
            queues[i].setMaxQueueSize(waitConfig.waitQueueSizes[i]);
    }
};

struct CLRX_INTERNAL WCodeBlock
{
    QueueState1 queues[ASM_WAIT_MAX_TYPES_NUM];
    std::vector<AsmWaitInstr> waitInstrs;
    Array<std::pair<uint16_t, RRegInfo> > firstRegs; ///< first occurence of reg
    
    void setMaxQueueSizes(const AsmWaitConfig& waitConfig)
    {
        for (cxuint i = 0; i < waitConfig.waitQueuesNum; i++)
            queues[i].setMaxQueueSize(waitConfig.waitQueueSizes[i]);
    }
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
        const AsmRegAllocator::SSAInfo& ssaInfo, const VarIndexMap* vregIndexMaps,
        size_t regTypesNum, const cxuint* regRanges, cxuint& regType, size_t& vidx)
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

static cxuint getRRegFromSVReg(const AsmSingleVReg& svreg, size_t outSSAIdIdx,
        const CodeBlock& cblock, const VarIndexMap* vregIndexMaps,
        const Array<cxuint>* graphColorMaps, size_t regTypesNum, const cxuint* regRanges)
{
    cxuint rreg = svreg.index;
    
    if (svreg.regVar != nullptr)
    {
        // if regvar, get vidx and get from vidx register index
        // get real register index
        const SSAInfo& ssaInfo = binaryMapFind(cblock.ssaInfoMap.begin(),
                cblock.ssaInfoMap.end(), svreg)->second;
        cxuint regType;
        size_t vidx;
        getVIdx(svreg, outSSAIdIdx, ssaInfo, vregIndexMaps,
                regTypesNum, regRanges, regType, vidx);
        rreg = regRanges[2*regType] + graphColorMaps[regType][vidx];
    }
    
    return rreg;
}

static void processQueueBlock(const CodeBlock& cblock, WCodeBlock& wblock,
        ISAWaitHandler& waitHandler, ISAWaitHandler::ReadPos& waitPos,
        ISAUsageHandler& usageHandler, const AsmWaitConfig& waitConfig,
        const VarIndexMap* vregIndexMaps, const Array<cxuint>* graphColorMaps,
        size_t regTypesNum, const cxuint* regRanges, bool onlyWarnings)
{
    // fill usage of registers (real access) to wCblock
    ISAUsageHandler::ReadPos usagePos = cblock.usagePos;
    
    SVRegMap ssaIdIdxMap;
    SVRegMap svregWriteOffsets;
    std::vector<AsmRegVarUsage> instrRVUs;
    
    std::unordered_map<uint16_t, RRegInfo> firstRegs;
    // process waits and delayed ops
    AsmWaitInstr waitInstr;
    AsmDelayedOp delayedOp;
    size_t instrOffset = SIZE_MAX;
    bool isWaitInstr = false;
    if (waitHandler.hasNext(waitPos))
    {
        isWaitInstr = waitHandler.nextInstr(waitPos, delayedOp, waitInstr);
        instrOffset = (isWaitInstr ? waitInstr.offset : delayedOp.offset);
    }
    
    uint16_t curQueueSizes[ASM_WAIT_MAX_TYPES_NUM];
    uint16_t curWaits[ASM_WAIT_MAX_TYPES_NUM];
    std::fill(curQueueSizes, curQueueSizes + waitConfig.waitQueuesNum, uint16_t(0));
    std::fill(curWaits, curWaits + waitConfig.waitQueuesNum, uint16_t(0));
    
    cxuint flushedQueues = 0;
    while (usageHandler.hasNext(usagePos))
    {
        const AsmRegVarUsage rvu = usageHandler.nextUsage(usagePos);
        if (rvu.offset >= cblock.end && instrOffset >= cblock.end)
            break;
        
        bool genWaitCnt = false;
        // gwaitI - current wait instruction
        AsmWaitInstr gwaitI { rvu.offset, { } };
        for (cxuint q = 0; q < waitConfig.waitQueuesNum; q++)
            gwaitI.waits[q] = waitConfig.waitQueueSizes[q]-1;
        
        std::vector<std::pair<uint16_t, RRegInfo> > curRegs;
        
        if (rvu.offset < cblock.end && rvu.offset <= instrOffset)
        {
            // process RegVar usage
            for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
            {
                AsmSingleVReg svreg{ rvu.regVar, rindex };
                
                size_t outSSAIdIdx = 0;
                if (rvu.regVar != nullptr)
                {
                    // if regvar, get vidx and get from vidx register index
                    if (checkWriteWithSSA(rvu))
                    {
                        size_t& ssaIdIdx = ssaIdIdxMap[svreg];
                        if (svreg.regVar != nullptr)
                            ssaIdIdx++;
                        outSSAIdIdx = ssaIdIdx;
                        svregWriteOffsets.insert({ svreg, rvu.offset });
                    }
                    else // insert zero
                    {
                        outSSAIdIdx = 0;
                        auto svrres = ssaIdIdxMap.insert({ svreg, 0 });
                        outSSAIdIdx = svrres.first->second;
                        auto swit = svregWriteOffsets.find(svreg);
                        if (swit != svregWriteOffsets.end() && swit->second == rvu.offset)
                            outSSAIdIdx--; // before this write
                    }
                }
                const cxuint rreg = getRRegFromSVReg(svreg, outSSAIdIdx, cblock,
                            vregIndexMaps, graphColorMaps, regTypesNum, regRanges);
                
                // put to readRegs and writeRegs
                if ((rvu.rwFlags & ASMRVU_READ) != 0)
                    curRegs.push_back({ qregVal(rreg, false), { rvu.offset } });
                if ((rvu.rwFlags & ASMRVU_WRITE) != 0)
                    curRegs.push_back({ qregVal(rreg, true), { rvu.offset } });
                
                // rreg
                if ((rvu.rwFlags & ASMRVU_READ) != 0)
                    for (cxuint q = 0; q < waitConfig.waitQueuesNum; q++)
                    {
                        uint16_t waitCnt = wblock.queues[q]
                            .findMinQueueSizeForReg(qregVal(rreg, false));
                        if (waitCnt != UINT16_MAX)
                        {
                            if (!onlyWarnings)
                            {
                                gwaitI.waits[q] = std::min(gwaitI.waits[q], waitCnt);
                                genWaitCnt = true;
                            }
                        }
                    }
                if ((rvu.rwFlags & ASMRVU_WRITE) != 0)
                    for (cxuint q = 0; q < waitConfig.waitQueuesNum; q++)
                    {
                        uint16_t waitCnt = wblock.queues[q]
                            .findMinQueueSizeForReg(qregVal(rreg, true));
                        if (waitCnt != UINT16_MAX)
                        {
                            if (!onlyWarnings)
                            {
                                gwaitI.waits[q] = std::min(gwaitI.waits[q], waitCnt);
                                genWaitCnt = true;
                            }
                        }
                    }
            }
            
            if (genWaitCnt && rvu.offset != instrOffset)
            {
                // generate wait instr
                wblock.waitInstrs.push_back(gwaitI);
                for (cxuint w = 0; w < waitConfig.waitQueuesNum; w++)
                    wblock.queues[w].flushTo(gwaitI.waits[w]);
            }
        }
        
        if (instrOffset < cblock.end && rvu.offset >= instrOffset && isWaitInstr)
        {
            // process wait instr
            if (genWaitCnt)
            {
                // if user waitinstr and generate wait instr
                // then choose min queue sizes in wait instr and generate new
                // wait instr.
                for (cxuint w = 0; w < waitConfig.waitQueuesNum; w++)
                    gwaitI.waits[w] = std::min(gwaitI.waits[w], waitInstr.waits[w]);
                wblock.waitInstrs.push_back(gwaitI);
                for (cxuint w = 0; w < waitConfig.waitQueuesNum; w++)
                    wblock.queues[w].flushTo(gwaitI.waits[w]);
            }
            else
            {
                wblock.waitInstrs.push_back(waitInstr);
                for (cxuint w = 0; w < waitConfig.waitQueuesNum; w++)
                {
                    wblock.queues[w].flushTo(waitInstr.waits[w]);
                    gwaitI.waits[w] = waitInstr.waits[w];
                    genWaitCnt = true;
                }
            }
        }
        
        // update curQueue sizes from curent waitcnt
        if (genWaitCnt && flushedQueues < waitConfig.waitQueuesNum)
            for (cxuint q = 0; q < waitConfig.waitQueuesNum; q++)
            {
                if (!wblock.queues[q].reallyFlushed)
                    curQueueSizes[q] = std::min(curQueueSizes[q],
                                        uint16_t(wblock.queues[q].ordered.size()));
                else
                {
                    // queues flushes free space (entries) before this block
                    if (curQueueSizes[q] != UINT16_MAX)
                        flushedQueues++;
                    curQueueSizes[q] = UINT16_MAX;
                }
            }
        
        if (instrOffset < cblock.end && rvu.offset >= instrOffset)
        {
            if (!isWaitInstr)
            {
                // delayed op
                const AsmDelayedOpTypeEntry& delOpEntry = waitConfig.delayOpTypes[
                                delayedOp.delayedOpType];
                const cxuint queue1Idx = delOpEntry.waitType;
                const cxuint queue2Idx = delayedOp.delayedOpType!=ASMDELOP_NONE ?
                        waitConfig.delayOpTypes[delayedOp.delayedOpType].waitType :
                        UINT_MAX;
                // next entry
                wblock.queues[queue1Idx].nextEntry();
                if (queue2Idx != UINT_MAX)
                    wblock.queues[queue2Idx].nextEntry();
                cxuint rcount = 0, rcount2 = 0;
                
                for (uint16_t rindex = delayedOp.rstart;
                                    rindex < delayedOp.rend; rindex++)
                {
                    AsmSingleVReg svreg{ delayedOp.regVar, rindex };
                    const size_t ssaIdIdx = ssaIdIdxMap.find(svreg)->second;
                    const cxuint rreg = getRRegFromSVReg(svreg, ssaIdIdx, cblock,
                            vregIndexMaps, graphColorMaps, regTypesNum, regRanges);
                    
                    if ((delayedOp.rwFlags & ASMRVU_READ) != 0 &&
                                delOpEntry.finishOnRegReadOut)
                    {
                        const uint16_t qreg = qregVal(rreg, false);
                        if (delOpEntry.ordered)
                            wblock.queues[queue1Idx].pushOrdered(qreg);
                        else
                            wblock.queues[queue1Idx].pushRandom(qreg);
                    }
                    if ((delayedOp.rwFlags & ASMRVU_WRITE) != 0)
                    {
                        const uint16_t qreg = qregVal(rreg, true);
                        if (delOpEntry.ordered)
                            wblock.queues[queue1Idx].pushOrdered(qreg);
                        else
                            wblock.queues[queue1Idx].pushRandom(qreg);
                    }
                    // if queue2
                    if (queue2Idx != UINT_MAX)
                    {
                        const AsmDelayedOpTypeEntry& delOpEntry2 =
                                waitConfig.delayOpTypes[delayedOp.delayedOpType2];
                        if ((delayedOp.rwFlags2 & ASMRVU_READ) != 0 &&
                                delOpEntry2.finishOnRegReadOut)
                        {
                            const uint16_t qreg = qregVal(rreg, false);
                            if (delOpEntry2.ordered)
                                wblock.queues[queue2Idx].pushOrdered(qreg);
                            else
                                wblock.queues[queue2Idx].pushRandom(qreg);
                        }
                        if ((delayedOp.rwFlags2 & ASMRVU_WRITE) != 0)
                        {
                            const uint16_t qreg = qregVal(rreg, true);
                            if (delOpEntry2.ordered)
                                wblock.queues[queue2Idx].pushOrdered(qreg);
                            else
                                wblock.queues[queue2Idx].pushRandom(qreg);
                        }
                        // do next queue entry if registered per element
                        rcount2 += 4;
                        if (delOpEntry2.counting!=255 && delOpEntry2.counting <= rcount)
                        {
                            // new entry
                            wblock.queues[queue2Idx].nextEntry();
                            rcount2 = 0;
                        }
                    }
                    
                    // do next queue entry if registered per element
                    rcount += 4;
                    if (delOpEntry.counting!=255 && delOpEntry.counting <= rcount)
                    {
                        // new entry
                        wblock.queues[queue1Idx].nextEntry();
                        rcount = 0;
                    }
                }
                
                // update current queue sizes from last delayed ops
                if (flushedQueues < waitConfig.waitQueuesNum)
                {
                    curQueueSizes[queue2Idx] = wblock.queues[queue1Idx].ordered.size();
                    if (queue1Idx != UINT_MAX)
                        curQueueSizes[queue2Idx] = wblock.queues[queue2Idx].ordered.size();
                }
            }
            
            // only if any not flushed queue
            if (flushedQueues < waitConfig.waitQueuesNum)
            {
                // update current queue sizes
                for (auto& re: curRegs)
                    std::copy(curQueueSizes, curQueueSizes + waitConfig.waitQueuesNum,
                                re.second.qsizes);
                // put current regs to firstRegs
                firstRegs.insert(curRegs.begin(), curRegs.end());
            }
            // get next instr
            if (!waitHandler.hasNext(waitPos))
                instrOffset = SIZE_MAX;
            else
            {
                isWaitInstr = waitHandler.nextInstr(waitPos, delayedOp, waitInstr);
                instrOffset = (isWaitInstr ? waitInstr.offset : delayedOp.offset);
            }
        }
        
        // copy to wblock as array
        wblock.firstRegs.resize(firstRegs.size());
        std::copy(firstRegs.begin(), firstRegs.end(), wblock.firstRegs.begin());
        mapSort(wblock.firstRegs.begin(), wblock.firstRegs.end());
    }
}

AsmWaitScheduler::AsmWaitScheduler(const AsmWaitConfig& _asmWaitConfig,
        Assembler& _assembler, const std::vector<CodeBlock>& _codeBlocks,
        const VarIndexMap* _vregIndexMaps, const Array<cxuint>* _graphColorMaps,
        bool _onlyWarnings)
        : waitConfig(_asmWaitConfig), assembler(_assembler), codeBlocks(_codeBlocks),
          vregIndexMaps(_vregIndexMaps), graphColorMaps(_graphColorMaps),
          onlyWarnings(_onlyWarnings)
{ }

void AsmWaitScheduler::schedule(ISAUsageHandler& usageHandler, ISAWaitHandler& waitHandler)
{
    if (codeBlocks.empty())
        return;
    
    cxuint regRanges[MAX_REGTYPES_NUM*2];
    size_t regTypesNum;
    std::vector<WCodeBlock> waitCodeBlocks(codeBlocks.size());
    for(WCodeBlock& wblock: waitCodeBlocks)
        wblock.setMaxQueueSizes(waitConfig);
    
    assembler.isaAssembler->getRegisterRanges(regTypesNum, regRanges);
    
    // fill queue states
    ISAWaitHandler::ReadPos waitPos{ 0, 0 };
    for (size_t i = 0; i < codeBlocks.size(); i++)
    {
        WCodeBlock& wblock = waitCodeBlocks[i];
        const CodeBlock& cblock = codeBlocks[i];
        
        processQueueBlock(cblock, wblock, waitHandler, waitPos, usageHandler, waitConfig,
                    vregIndexMaps, graphColorMaps, regTypesNum, regRanges, onlyWarnings);
    }
    
    std::vector<bool> visited(codeBlocks.size(), false);
    std::unordered_map<size_t, size_t> visitedCount;
    std::unordered_map<size_t, VectorSet<cxuint> > loopWays;
    std::deque<WaitFlowStackEntry0> flowStack;
    std::unordered_set<size_t> flowStackBlocks;
    flowStackBlocks.insert(size_t(0));
    
    flowStack.push_back(WaitFlowStackEntry0(0, 0, waitConfig));
    /// find multiply visited points to schedule
    while (!flowStack.empty())
    {
        WaitFlowStackEntry0& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        if (entry.nextIndex == 0)
        {
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                flowStackBlocks.insert(entry.blockIndex);
            }
            else
            {
                if (flowStackBlocks.find(entry.blockIndex) != flowStackBlocks.end())
                {
                    // if loop
                    auto flit = flowStack.end();
                    flit -= 2;
                    loopWays[flit->blockIndex].insertValue(flit->nextIndex);
                }
                auto vcres = visitedCount.insert({ entry.blockIndex, size_t(2)});
                if (!vcres.second)
                    vcres.first->second++;
                flowStack.pop_back();
                continue;
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            size_t nextBlock = cblock.nexts[entry.nextIndex].block;
            bool isCall = cblock.nexts[entry.nextIndex].isCall;
            flowStack.push_back(WaitFlowStackEntry0(nextBlock, 0, isCall, waitConfig));
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            if (entry.nextIndex!=0) // if back from calls (just return from calls)
            {
            }
            flowStack.push_back(WaitFlowStackEntry0(entry.blockIndex+1, 0, waitConfig));
            entry.nextIndex++;
        }
        else // back
        {
            flowStackBlocks.erase(entry.blockIndex);
            flowStack.pop_back();
        }
    }
    
    /// join queue state together and add a missing wait instructions
    flowStack.clear();
    flowStack.push_back(WaitFlowStackEntry0(0, 0, waitConfig));
    
    struct LoopStackEntry
    {
        size_t loopBlock;
        size_t blockBeforeLoop;
        size_t wayToLoop;
        cxuint count;
    };
    
    std::deque<LoopStackEntry> loopStack;
    
    while (!flowStack.empty())
    {
        WaitFlowStackEntry0& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        WCodeBlock& wblock = waitCodeBlocks[entry.blockIndex];
        if (entry.nextIndex == 0)
        {
            auto vcIt = visitedCount.find(entry.blockIndex);
            
            // process current block
            // process only if this ways will be visited from all predecessors
            auto flit = flowStack.end();
            if (flowStack.size() > 1)
            {
                flit -= 2;
                auto loopWayIt = loopWays.find(flit->blockIndex);
                if (loopWayIt != loopWays.end() &&
                    loopWayIt->second.hasValue(flit->nextIndex))
                {
                    if (!loopStack.empty())
                    {
                        LoopStackEntry& loopEntry = loopStack.back();
                        auto lsit = loopStack.end();
                        if (loopEntry.blockBeforeLoop == flit->blockIndex &&
                            loopEntry.wayToLoop == flit->nextIndex)
                        {
                            // this way
                            // check whether previous loop is touched twice or
                            // no loop before
                            if (loopStack.size() > 1)
                            {
                                lsit -= 2;
                                if (lsit->count < 2)
                                {
                                    // if not then skip
                                    loopStack.pop_back();
                                    flowStack.pop_back();
                                    continue;
                                }
                            }
                            
                            loopEntry.count++;
                            if (loopEntry.count > 2)
                            {
                                // skip, loop has been visited twice
                                loopStack.pop_back();
                                flowStack.pop_back();
                                continue;
                            }
                        }
                    }
                    // push new loop to stack
                    loopStack.push_back({ entry.blockIndex, flit->blockIndex,
                            flit->nextIndex, cxuint(1) });
                }
                for (cxuint q = 0; q < waitConfig.waitQueuesNum; q++)
                    wblock.queues[q].joinPrev(waitCodeBlocks[flit->blockIndex].queues[q]);
            }
            if (vcIt!=visitedCount.end() && vcIt->second>1)
            {
                vcIt->second--;
                // go back (not all ways were merged)
                flowStack.pop_back();
                continue;
            }
        }
        
        if (entry.nextIndex < cblock.nexts.size())
        {
            size_t nextBlock = cblock.nexts[entry.nextIndex].block;
            bool isCall = cblock.nexts[entry.nextIndex].isCall;
            flowStack.push_back(WaitFlowStackEntry0(nextBlock, 0,  isCall, waitConfig));
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
            if (entry.nextIndex!=0) // if back from calls (just return from calls)
            {
            }
            flowStack.push_back(WaitFlowStackEntry0(entry.blockIndex+1, 0, waitConfig));
            entry.nextIndex++;
        }
        else // back
        {
            if (!loopStack.empty() && loopStack.back().loopBlock==entry.blockIndex)
                loopStack.pop_back();
            flowStack.pop_back();
        }
    }
}
