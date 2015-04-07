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
#include <string>
#include <ostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Assembler.h>

using namespace CLRX;

ISAAssembler::~ISAAssembler()
{ }

/*
 * Assembler
 */

Assembler::Assembler(std::istream& input, cxuint flags)
{
}

Assembler::~Assembler()
{
}

void Assembler::addIncludeDir(const char* includeDir)
{
}
    
void Assembler::setIncludeDirs(const char** includeDirs)
{
}

void Assembler::setInitialDefSyms(const DefSymMap& defsyms)
{
}

void Assembler::addInitialDefSym(const std::string& symName, const std::string& symExpr)
{
}

AsmExpression* Assembler::parseExpression(size_t lineNo, size_t colNo, size_t stringSize,
             const char* string, uint64_t& outValue) const
{
    return nullptr;
}

void Assembler::assemble(const char* inputString, std::ostream& msgStream)
{
}

void Assembler::assemble(std::istream& inputStream, std::ostream& msgStream)
{
}
