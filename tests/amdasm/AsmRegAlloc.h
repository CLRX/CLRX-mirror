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

#ifndef __CLRXTEST_ASMREGALLOC_H__
#define __CLRXTEST_ASMREGALLOC_H__

#include <CLRX/Config.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include "../TestUtils.h"

using namespace CLRX;

typedef AsmRegAllocator::CodeBlock CodeBlock;
typedef AsmRegAllocator::NextBlock NextBlock;

struct CCodeBlock
{
    size_t start, end; // place in code
    Array<NextBlock> nexts; ///< nexts blocks
    bool haveCalls;
    bool haveReturn;
    bool haveEnd;
};

struct AsmCodeStructCase
{
    const char* input;
    Array<CCodeBlock> codeBlocks;
    bool good;
    const char* errorMessages;
};

typedef AsmRegAllocator::SSAInfo SSAInfo;
typedef AsmRegAllocator::SSAReplace SSAReplace;
typedef AsmRegAllocator::SSAReplacesMap SSAReplacesMap;

struct TestSingleVReg
{
    CString name;
    uint16_t index;
    
    bool operator==(const TestSingleVReg& b) const
    { return name==b.name && index==b.index; }
    bool operator!=(const TestSingleVReg& b) const
    { return name!=b.name || index!=b.index; }
    bool operator<(const TestSingleVReg& b) const
    { return name<b.name || (name==b.name && index<b.index); }
};

struct TestSingleVReg2
{
    const char* name;
    uint16_t index;
};

struct CCodeBlock2
{
    size_t start, end; // place in code
    Array<NextBlock> nexts; ///< nexts blocks
    Array<std::pair<TestSingleVReg2, SSAInfo> > ssaInfos;
    bool haveCalls;
    bool haveReturn;
    bool haveEnd;
};

struct AsmSSADataCase
{
    const char* input;
    Array<CCodeBlock2> codeBlocks;
    Array<std::pair<TestSingleVReg2, Array<SSAReplace> > > ssaReplaces;
    bool good;
    const char* errorMessages;
};

extern const AsmCodeStructCase codeStructTestCases1Tbl[];
extern const AsmSSADataCase ssaDataTestCases1Tbl[];
extern const AsmSSADataCase ssaDataTestCases2Tbl[];
extern const AsmSSADataCase ssaDataTestCases3Tbl[];

#endif
