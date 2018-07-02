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


void ISAWaitHandler::pushDelayedResult(size_t offset, const AsmDelayedResult& delResult)
{
    delayedResults.push_back(std::make_pair(offset, delResult));
}

void ISAWaitHandler::pushWaitInstr(size_t offset, const AsmWaitInstr& waitInstr)
{
    waitInstrs.push_back(std::make_pair(offset, waitInstr));
}

bool ISAWaitHandler::nextInstr(std::pair<size_t, AsmDelayedResult>* delRes,
                    std::pair<size_t, AsmWaitInstr>* waitInstr)
{
    size_t delResOffset = SIZE_MAX;
    size_t waitInstrOffset = SIZE_MAX;
    if (readPos.delResPos < delayedResults.size())
        delResOffset = delayedResults[readPos.delResPos].first;
    if (readPos.waitInstrPos < waitInstrs.size())
        waitInstrOffset = waitInstrs[readPos.waitInstrPos].first;
    if (delResOffset < waitInstrOffset)
    {
        *delRes = delayedResults[readPos.delResPos++];
        return false;
    }
    *waitInstr = waitInstrs[readPos.waitInstrPos++];
    return true;
}

/* AsmWaitScheduler */

AsmWaitScheduler::AsmWaitScheduler(const AsmWaitConfig& _asmWaitConfig,
            Assembler& _assembler,
            const std::vector<AsmRegAllocator::CodeBlock>& _codeBlocks,
            bool _onlyWarnings)
        : waitConfig(_asmWaitConfig), assembler(_assembler), codeBlocks(_codeBlocks),
          onlyWarnings(_onlyWarnings)
{ }

void AsmWaitScheduler::schedule()
{
}
