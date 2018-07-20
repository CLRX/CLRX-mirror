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
static inline uint16_t qreg(uint16_t reg, bool write)
{ return reg | (write ? 0x8000 : 0); }

static inline bool qregWrite(uint16_t qreg)
{ return (qreg & 0x8000)!=0; }

static inline uint16_t qregReg(uint16_t qreg)
{ return qreg & 0x7ffff; }

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
    
    QueueState1(cxuint _maxQueueSize) : maxQueueSize(_maxQueueSize), orderedStartPos(0),
                requestedQueueSize(0), firstFlush(true), reallyFlushed(false)
    { }
    
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
        reallyFlushed = true;
        firstFlush = false;
        if (size == 0)
            random.regs.clear(); // clear randomly ordered if must be empty
        while (size > ordered.size())
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
    
    cxuint findMinQueueSizeForReg(uint16_t reg) const
    {
        auto it = regPlaces.find(reg);
        if (it == regPlaces.end())
            // if found, then 0 otherwize not found (UINT_MAX)
            return random.regs.find(reg) != random.regs.end() ? 0 : UINT_MAX;
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
            for (auto oitx = ordered.begin(); oitx != oit1; ++oitx)
                for (auto e: oitx->regs)
                    if (oitx->regs.insert(e).second)
                    {
                        auto rres = regPlaces.insert(std::make_pair(e, qpos));
                        if (rres.second && rres.first->second-orderedStartPos <
                                            qpos-orderedStartPos)
                            rres.first->second = qpos;
                    }
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
    
    void joinNext(const QueueState1& next)
    {
        if (next.reallyFlushed)
            *this = next; // just copy
        else
        {
            if (next.requestedQueueSize != ordered.size())
                flushTo(next.requestedQueueSize - ordered.size());
            size_t orderedSize = ordered.size();
            ordered.insert(ordered.end(), next.ordered.begin(), next.ordered.end());
            // update regPlaces after pushing to front
            uint16_t qpos = orderedStartPos + orderedSize;
            for (auto oitx = ordered.begin() + orderedSize; oitx != ordered.end(); ++oitx)
                for (auto e: oitx->regs)
                    if (oitx->regs.insert(e).second)
                    {
                        auto rres = regPlaces.insert(std::make_pair(e, qpos));
                        if (rres.second && rres.first->second-orderedStartPos <
                                            qpos-orderedStartPos)
                            rres.first->second = qpos;
                    }
            // join with previous
            random.join(next.random);
        }
    }
};

struct CLRX_INTERNAL WaitFlowStackEntry0
{
    size_t blockIndex;
    size_t nextIndex;
    bool isCall;
    bool haveReturn;
    
    QueueState1 queues[ASM_WAIT_MAX_TYPES_NUM];
    // key - reg, value - offset in codeblock
    std::unordered_map<size_t, size_t> readRegs;
    // key - reg, value - offset in codeblock
    std::unordered_map<size_t, size_t> writeRegs;
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

AsmWaitScheduler::AsmWaitScheduler(const AsmWaitConfig& _asmWaitConfig,
        Assembler& _assembler, const std::vector<CodeBlock>& _codeBlocks,
        const VarIndexMap* _vregIndexMaps, const Array<cxuint>* _graphColorMaps,
        bool _onlyWarnings)
        : waitConfig(_asmWaitConfig), assembler(_assembler), codeBlocks(_codeBlocks),
          vregIndexMaps(_vregIndexMaps), graphColorMaps(_graphColorMaps),
          onlyWarnings(_onlyWarnings), waitCodeBlocks(_codeBlocks.size())
{ }

void AsmWaitScheduler::schedule(ISAUsageHandler& usageHandler, ISAWaitHandler& waitHandler)
{
    if (codeBlocks.empty())
        return;
    
    cxuint regRanges[MAX_REGTYPES_NUM*2];
    size_t regTypesNum;
    assembler.isaAssembler->getRegisterRanges(regTypesNum, regRanges);
    
    // fill queue states
    ISAWaitHandler::ReadPos waitPos{ 0, 0 };
    for (size_t i = 0; i < codeBlocks.size(); i++)
    {
        WCodeBlock& wblock = waitCodeBlocks[i];
        const CodeBlock& cblock = codeBlocks[i];
        // fill usage of registers (real access) to wCblock
        ISAUsageHandler::ReadPos usagePos = cblock.usagePos;
        
        SVRegMap ssaIdIdxMap;
        SVRegMap svregWriteOffsets;
        std::vector<AsmRegVarUsage> instrRVUs;
        
        std::unordered_map<size_t, size_t> readRegs;
        std::unordered_map<size_t, size_t> writeRegs;
        while (usageHandler.hasNext(usagePos))
        {
            const AsmRegVarUsage rvu = usageHandler.nextUsage(usagePos);
            if (rvu.offset >= cblock.end)
                break;
            
            for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
            {
                cxuint rreg = rindex;
                
                if (rvu.regVar != nullptr)
                {
                    // if regvar, get vidx and get from vidx register index
                    AsmSingleVReg svreg{ rvu.regVar, rindex };
                    size_t outSSAIdIdx = 0;
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
                        if (swit != svregWriteOffsets.end() &&
                            swit->second == rvu.offset)
                            outSSAIdIdx--; // before this write
                    }
                    
                    // get real register index
                    const SSAInfo& ssaInfo = binaryMapFind(cblock.ssaInfoMap.begin(),
                            cblock.ssaInfoMap.end(), svreg)->second;
                    cxuint regType;
                    size_t vidx;
                    getVIdx(svreg, outSSAIdIdx, ssaInfo, vregIndexMaps,
                            regTypesNum, regRanges, regType, vidx);
                    rreg = regRanges[2*regType] + graphColorMaps[regType][vidx];
                }
                
                // put to readRegs and writeRegs
                if ((rvu.rwFlags & ASMRVU_READ) != 0)
                    readRegs.insert({ rreg, rvu.offset });
                if ((rvu.rwFlags & ASMRVU_WRITE) != 0)
                    writeRegs.insert({ rreg, rvu.offset });
            }
            
            // copy to wblock as array
            wblock.readRegs.resize(readRegs.size());
            wblock.writeRegs.resize(writeRegs.size());
            std::copy(readRegs.begin(), readRegs.end(), wblock.readRegs.begin());
            std::copy(writeRegs.begin(), writeRegs.end(), wblock.writeRegs.begin());
            mapSort(wblock.readRegs.begin(), wblock.readRegs.end());
            mapSort(wblock.writeRegs.begin(), wblock.writeRegs.end());
        }
    }
    
    
    for (size_t i = 0; i < codeBlocks.size(); i++)
    {
        const CodeBlock& cblock = codeBlocks[i];
        WCodeBlock& wblock = waitCodeBlocks[i];
        
        AsmWaitInstr waitInstr;
        AsmDelayedOp delayedOp;
        if (!waitHandler.hasNext(waitPos))
            continue;
        bool isWaitInstr = waitHandler.nextInstr(waitPos, delayedOp, waitInstr);
        size_t instrOffset = (isWaitInstr ? waitInstr.offset : delayedOp.offset);
        // skip instrs between codeblock
        while (instrOffset < cblock.start)
        {
            if (!waitHandler.hasNext(waitPos))
                break;
            isWaitInstr = waitHandler.nextInstr(waitPos, delayedOp, waitInstr);
            instrOffset = (isWaitInstr ? waitInstr.offset : delayedOp.offset);
        }
        
        // real loop
        while (instrOffset < cblock.end)
        {
            if (!waitHandler.hasNext(waitPos))
                break;
            isWaitInstr = waitHandler.nextInstr(waitPos, delayedOp, waitInstr);
            instrOffset = (isWaitInstr ? waitInstr.offset : delayedOp.offset);
        }
    }
    
    // after processing. we clean up wait instructions list
}
