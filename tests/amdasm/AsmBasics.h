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

#ifndef __CLRXTEST_ASMBASICS_H__
#define __CLRXTEST_ASMBASICS_H__

#include <CLRX/Config.h>
#include <iostream>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Assembler.h>
#include "../TestUtils.h"

using namespace CLRX;

struct Section
{
    const char* name;
    AsmKernelId kernelId;
    AsmSectionType type;
    Array<cxbyte> content;
};

struct SymEntry
{
    const char* name;
    uint64_t value;
    AsmSectionId sectionId;
    uint64_t size;
    bool hasValue;
    bool onceDefined;
    bool base;
    cxbyte info;
    cxbyte other;
    bool regRange;
};

struct AsmTestCase
{
    const char* input;
    BinaryFormat format;
    GPUDeviceType deviceType;
    bool is64Bit;
    const Array<const char*> kernelNames;
    const Array<Section> sections;
    const Array<SymEntry> symbols;
    bool good;
    const char* errorMessages;
    const char* printMessages;
    Array<const char*> includeDirs;
};

extern const AsmTestCase asmTestCases1Tbl[];
extern const AsmTestCase asmTestCases2Tbl[];

#endif
