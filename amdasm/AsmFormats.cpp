/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

AsmFormatException::AsmFormatException(const std::string& message) : Exception(message)
{ }

AsmFormatHandler::AsmFormatHandler(Assembler& _assembler) : assembler(_assembler)
{ }

AsmFormatHandler::~AsmFormatHandler()
{ }

void AsmFormatHandler::handleLabel(const CString& label)
{ }

bool AsmFormatHandler::resolveSymbol(const AsmSymbol& symbol, uint64_t& value,
                 cxuint& sectionId)
{
    return false;
}

bool AsmFormatHandler::resolveRelocation(const AsmExpression* expr, uint64_t& value,
                 cxuint& sectionId)
{
    return false;
}

/* raw code handler */

AsmRawCodeHandler::AsmRawCodeHandler(Assembler& assembler): AsmFormatHandler(assembler)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
}

cxuint AsmRawCodeHandler::addKernel(const char* kernelName)
{
    throw AsmFormatException("In rawcode defining kernels is not allowed");
}

cxuint AsmRawCodeHandler::addSection(const char* name, cxuint kernelId)
{
    if (::strcmp(name, ".text")!=0)
        throw AsmFormatException("Only section '.text' can be in raw code");
    else
        throw AsmFormatException("Section '.text' already exists");
}

cxuint AsmRawCodeHandler::getSectionId(const char* sectionName) const
{
    return ::strcmp(sectionName, ".text") ? ASMSECT_NONE : 0;
}

void AsmRawCodeHandler::setCurrentKernel(cxuint kernel)
{
    if (kernel != ASMKERN_GLOBAL)
        throw AsmFormatException("No kernels available");
}

void AsmRawCodeHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId!=0)
        throw AsmFormatException("Section doesn't exists");
}

AsmFormatHandler::SectionInfo AsmRawCodeHandler::getSectionInfo(cxuint sectionId) const
{
    if (sectionId!=0)
        throw AsmFormatException("Section doesn't exists");
    return { ".text", AsmSectionType::CODE, ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE };
}

bool AsmRawCodeHandler::parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr)
{   // not recognized any pseudo-op
    return false;
}

bool AsmRawCodeHandler::prepareBinary()
{ return true; }

void AsmRawCodeHandler::writeBinary(std::ostream& os) const
{
    const AsmSection& section = assembler.getSections()[0];
    if (!section.content.empty())
        os.write((char*)section.content.data(), section.content.size());
}

void AsmRawCodeHandler::writeBinary(Array<cxbyte>& array) const
{
    const AsmSection& section = assembler.getSections()[0];
    array.assign(section.content.begin(), section.content.end());
}
