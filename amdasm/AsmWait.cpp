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
    size_t rwaitInstrIndex; // waitInstr index which removes queueEntry
};
// key - register number, value - previous register queue position
struct QueueEntry2
{
    std::unordered_map<uint16_t, uint16_t> regs;
    size_t rwaitInstrIndex; // waitInstr index which removes queueEntry
};

struct CLRX_INTERNAL QueueState
{
    std::vector<QueueEntry1> ordered;  // ordered items
    QueueEntry1 random;   // items in random order
    // request queue size at end of block (by enqueuing and waiting/flushing)
    cxuint requestedQueueSize;
};

struct CLRX_INTERNAL QueueState2
{
    cxuint maxQueueSize;
    uint16_t orderedStartPos;
    QueueEntry1 firstOrdered; // on full
    std::deque<QueueEntry2> ordered;  // ordered items
    QueueEntry1 random;   // items in random order
    // register place in queue - key - reg, value - position
    std::unordered_map<uint16_t, uint16_t> regPlaces;
    // request queue size at start of block (by enqueuing and waiting/flushing)
    // used while joining with previous block
    cxuint requestedQueueSize;
    bool firstFlush;
    
    QueueState2(cxuint _maxQueueSize) : maxQueueSize(_maxQueueSize),
                orderedStartPos(0), requestedQueueSize(0), firstFlush(true)
    { }
    
    void pushOrdered(uint16_t reg)
    {
        uint16_t prevPlace = orderedStartPos-2; // no place
        const uint16_t lastPlace = orderedStartPos + ordered.size()-1;
        auto rpres = regPlaces.insert(std::make_pair(reg, lastPlace));
        if (!rpres.second)
        {
            // if already exists
            prevPlace = rpres.first->second; // if some place
            rpres.first->second = lastPlace;
        }
        ordered.back().regs.insert(std::make_pair(reg, prevPlace));
    }
    
    void pushRandom(uint16_t reg)
    { random.regs.insert(reg); }
    
    void nextOrder()
    {
        if (ordered.empty() || !ordered.back().regs.empty())
            // push only if empty or previous is filled
            ordered.push_back(QueueEntry2());
        
        if (ordered.size() == maxQueueSize)
        {
            // move first entry in ordered queue to first ordered
            for (const auto& e: ordered.front().regs)
                firstOrdered.regs.insert(e.first);
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
        firstFlush = false;
        if (size == 0)
            random.regs.clear(); // clear randomly ordered if must be empty
        if (size < maxQueueSize)
            firstOrdered.regs.clear(); // clear first in full
        while (size > ordered.size())
        {
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
        if (pos == 0xffff) // before start (is first)
            return firstOrdered.regs.find(reg)!= firstOrdered.regs.end() ?
                    ordered.size() : UINT_MAX;
        return ordered.size()-1 - cxuint(pos);
    }
    
    /*QueueState toQueueState() const
    {
        QueueState
    }*/
};

struct CLRX_INTERNAL WaitFlowStackEntry0
{
    size_t blockIndex;
    size_t nextIndex;
    bool isCall;
    bool haveReturn;
    
    QueueState queues[ASM_WAIT_MAX_TYPES_NUM];
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
