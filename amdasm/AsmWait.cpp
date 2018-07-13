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

ISAWaitHandler::ISAWaitHandler() : readPos{ size_t(0), size_t(0) }
{ }

void ISAWaitHandler::rewind()
{
    readPos = { size_t(0), size_t(0) };
}

void ISAWaitHandler::setReadPos(const ReadPos& _readPos)
{
    readPos = _readPos;
}

ISAWaitHandler* ISAWaitHandler::copy() const
{
    return new ISAWaitHandler(*this);
}


void ISAWaitHandler::pushDelayedOp(const AsmDelayedOp& delOp)
{
    delayedOps.push_back(delOp);
}

void ISAWaitHandler::pushWaitInstr(const AsmWaitInstr& waitInstr)
{
    waitInstrs.push_back(waitInstr);
}

bool ISAWaitHandler::nextInstr(AsmDelayedOp& delOp, AsmWaitInstr& waitInstr)
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

/* AsmWaitScheduler */

namespace CLRX
{

typedef std::unordered_set<uint16_t> QueueEntry1;
// key - register number, value - previous register queue position
typedef std::unordered_map<uint16_t, uint16_t> QueueEntry2;

struct CLRX_INTERNAL QueueState
{
    std::vector<QueueEntry1> ordered;  // ordered items
    std::vector<QueueEntry1> random;   // items in random order
};

enum {
    REGPLACE_NONE = UINT16_MAX,
    REGPLACE_RANDOM = UINT16_MAX-1
};

struct CLRX_INTERNAL QueueState2
{
    std::deque<QueueEntry2> ordered;  // ordered items
    std::deque<QueueEntry2> random;   // items in random order
    // register place in queue - key - reg, value - position
    std::unordered_map<uint16_t, uint16_t> regPlace;
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

typedef AsmRegAllocator::CodeBlock CodeBlock;
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
    
    for (size_t i = 0; i < codeBlocks.size(); i++)
    {
        WCodeBlock& wblock = waitCodeBlocks[i];
        const CodeBlock& cblock = codeBlocks[i];
        // fill usage of registers (real access) to wCblock
        usageHandler.setReadPos(cblock.usagePos);
        
        SVRegMap ssaIdIdxMap;
        SVRegMap svregWriteOffsets;
        std::vector<AsmRegVarUsage> instrRVUs;
        
        std::unordered_map<size_t, size_t> readRegs;
        std::unordered_map<size_t, size_t> writeRegs;
        while (usageHandler.hasNext())
        {
            const AsmRegVarUsage rvu = usageHandler.nextUsage();
            if (rvu.offset >= cblock.end)
                break;
            
            // 
            for (uint16_t rindex = rvu.rstart; rindex < rvu.rend; rindex++)
            {
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
                const cxuint rreg = graphColorMaps[regType][vidx];
                
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
    
    // fill up waitInstrs
    waitHandler.rewind();
    if (!waitHandler.hasNext())
        return;
    
    ISAWaitHandler::ReadPos oldReadPos = waitHandler.getReadPos();
    auto cbit = codeBlocks.begin();
    auto wcbit = waitCodeBlocks.begin();
    
    bool isWaitInstr;
    AsmDelayedOp delayedOp;
    AsmWaitInstr waitInstr;
    isWaitInstr =  waitHandler.nextInstr(delayedOp, waitInstr);
    
    while (true)
    {
        size_t offset = isWaitInstr ? waitInstr.offset : delayedOp.offset;
        while (cbit != codeBlocks.end() && cbit->end <= offset)
        {
            wcbit->waitPos = oldReadPos;
            ++cbit;
            ++wcbit;
        }
        if (cbit == codeBlocks.end())
            break;
        
        // skip rvu's before codeblock
        while (offset < cbit->start && waitHandler.hasNext())
        {
            oldReadPos = waitHandler.getReadPos();
            isWaitInstr =  waitHandler.nextInstr(delayedOp, waitInstr);
            offset = isWaitInstr ? waitInstr.offset : delayedOp.offset;
        }
        if (offset < cbit->start)
            break;
        
        wcbit->waitPos = oldReadPos;
        while (offset < cbit->end)
        {
            if (isWaitInstr)
                wcbit->waitInstrs.push_back(waitInstr);
            // next position
            oldReadPos = waitHandler.getReadPos();
            isWaitInstr =  waitHandler.nextInstr(delayedOp, waitInstr);
            offset = isWaitInstr ? waitInstr.offset : delayedOp.offset;
        }
        // next code block
        ++cbit;
        ++wcbit;
    }
    
    std::vector<bool> visited(codeBlocks.size());
    std::deque<WaitFlowStackEntry0> flowStack;
    flowStack.push_back({ 0, 0 });
    /*
     * main loop - processing wait instrs and delayed ops
     */
    while (!flowStack.empty())
    {
        WaitFlowStackEntry0& entry = flowStack.back();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        WCodeBlock& wblock = waitCodeBlocks[entry.blockIndex];
        
        if (entry.nextIndex == 0)
        {
            // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
            }
            else
            {
            }
        }
        
        /*if (!callStack.empty() &&
            entry.blockIndex == callStack.back().callBlock &&
            entry.nextIndex-1 == callStack.back().callNextIndex)
        {
        }*/
        
        if (entry.nextIndex < cblock.nexts.size())
        {
        }
        else if (((entry.nextIndex==0 && cblock.nexts.empty()) ||
                // if have any call then go to next block
                (cblock.haveCalls && entry.nextIndex==cblock.nexts.size())) &&
                 !cblock.haveReturn && !cblock.haveEnd)
        {
        }
        else
        {
        }
    }
    
    // after processing. we clean up wait instructions list
}
