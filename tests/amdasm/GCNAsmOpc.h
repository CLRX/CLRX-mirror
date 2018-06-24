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

#ifndef __CLRXTEST_GCNASMOPC_H__
#define __CLRXTEST_GCNASMOPC_H__

#include <cstdint>

struct GCNAsmOpcodeCase
{
    const char* input;
    uint32_t expWord0, expWord1;
    bool twoWords;
    bool good;
    const char* errorMessages;
};

extern const GCNAsmOpcodeCase encGCNOpcodeCases[];
extern const GCNAsmOpcodeCase encGCN11OpcodeCases[];
extern const GCNAsmOpcodeCase encGCN12OpcodeCases[];
extern const GCNAsmOpcodeCase encGCN14OpcodeCases[];
extern const GCNAsmOpcodeCase encGCN141OpcodeCases[];

#endif
