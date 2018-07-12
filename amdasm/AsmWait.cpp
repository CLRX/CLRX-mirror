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
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>

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

// key -register number, value - previous register queue position
typedef std::unordered_map<cxuint, cxuint> QueueEntry;

struct QueueState
{
    std::vector<QueueEntry> ordered;
    std::vector<QueueEntry> random;
};

struct QueueState2: QueueState
{
    // register place in queue - key - reg, value - position
    std::unordered_map<cxuint, cxuint> regPlace;
};

struct CLRX_INTERNAL WaitFlowStackEntry0
{
    std::unordered_map<size_t, size_t> readRegs;
    std::unordered_map<size_t, size_t> writeRegs;
};

};

AsmWaitScheduler::AsmWaitScheduler(const AsmWaitConfig& _asmWaitConfig,
        Assembler& _assembler, const std::vector<AsmRegAllocator::CodeBlock>& _codeBlocks,
        const AsmRegAllocator::VarIndexMap* _vregIndexMaps,
        const Array<cxuint>* _graphColorMaps, bool _onlyWarnings)
        : waitConfig(_asmWaitConfig), assembler(_assembler), codeBlocks(_codeBlocks),
          vregIndexMaps(_vregIndexMaps), graphColorMaps(_graphColorMaps),
          onlyWarnings(_onlyWarnings)
{ }

void AsmWaitScheduler::schedule()
{
}
