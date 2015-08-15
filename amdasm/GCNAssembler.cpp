/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
#include <cstring>
#include <algorithm>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"
#include "GCNInternals.h"

using namespace CLRX;

static std::once_flag clrxGCNAssemblerOnceFlag;
static cxuint gcnInstrsTableSize = 0;
static std::unique_ptr<GCNInstruction[]> gcnInstrSortedTable = nullptr;

static void initializeGCNAssembler()
{
    while (gcnInstrsTable[gcnInstrsTableSize].mnemonic!=nullptr)
        gcnInstrsTableSize++;
    gcnInstrSortedTable.reset(new GCNInstruction[gcnInstrsTableSize]);
    std::copy(gcnInstrsTable, gcnInstrsTable + gcnInstrsTableSize,
              gcnInstrSortedTable.get());
    /// sort this table
    std::sort(gcnInstrSortedTable.get(), gcnInstrSortedTable.get()+gcnInstrsTableSize,
            [](const GCNInstruction& instr1, const GCNInstruction& instr2)
            {   // compare mnemonic and if mnemonic is equal then architectur mask
                int r = ::strcmp(instr1.mnemonic, instr2.mnemonic);
                return (r < 0) || (r==0 && instr1.archMask < instr2.archMask);
            });
}

GCNAssembler::GCNAssembler(Assembler& assembler): ISAAssembler(assembler),
        sgprsNum(0), vgprsNum(0)
{
    std::call_once(clrxGCNAssemblerOnceFlag, initializeGCNAssembler);
}

GCNAssembler::~GCNAssembler()
{ }

void GCNAssembler::assemble(uint64_t lineNo, const char* line,
                      std::vector<cxbyte>& output)
{
}

bool GCNAssembler::resolveCode(cxbyte* location, cxbyte targetType, uint64_t value)
{
    return false;
}

bool GCNAssembler::checkMnemonic(const CString& mnemonic) const
{
    return std::binary_search(gcnInstrSortedTable.get(),
              gcnInstrSortedTable.get()+gcnInstrsTableSize,
               GCNInstruction{mnemonic.c_str()},
               [](const GCNInstruction& instr1, const GCNInstruction& instr2)
               { return ::strcmp(instr1.mnemonic, instr2.mnemonic)<0; });
}

const cxuint* GCNAssembler::getAllocatedRegisters(size_t& regTypesNum) const
{
    regTypesNum = 2;
    return regTable;
}
