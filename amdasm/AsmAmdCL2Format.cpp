/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2016 Mateusz Szpakowski
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
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* amdPseudoOpNamesTbl[] =
{
    "acl_version", "arg", "bssdata", "compile_options", "config",
    "cws", "debugmode", "dims", "driver_version", "dx10clamp", "exceptions",
    "floatmode", "get_driver_version", "globaldata", "ieeemode", "inner",
    "isametadata", "localsize" "metadata", "privmode",
    "pgmrsrc1", "pgmrsrc2", "priority", "rwdata", "sampler",
    "samplerinit", "samplerreloc", "scratchbuffer",
    "setupargs", "sgprsnum", "stub", "tgsize",
    "useenqueue", "usesetup", "usesizes", "vgprsnum"
};

/*
 * AmdCatalyst format handler
 */

AsmAmdCL2Handler::AsmAmdCL2Handler(Assembler& assembler) : AsmFormatHandler(assembler)
{
}

AsmAmdCL2Handler::~AsmAmdCL2Handler()
{
}

void AsmAmdCL2Handler::saveCurrentSection()
{
}


void AsmAmdCL2Handler::restoreCurrentAllocRegs()
{
}

void AsmAmdCL2Handler::saveCurrentAllocRegs()
{
}

cxuint AsmAmdCL2Handler::addKernel(const char* kernelName)
{
    return 0;
}

cxuint AsmAmdCL2Handler::addSection(const char* sectionName, cxuint kernelId)
{
    return 0;
}

cxuint AsmAmdCL2Handler::getSectionId(const char* sectionName) const
{
    return 0;
}

void AsmAmdCL2Handler::setCurrentKernel(cxuint kernel)
{
}

void AsmAmdCL2Handler::setCurrentSection(cxuint sectionId)
{
}

AsmFormatHandler::SectionInfo AsmAmdCL2Handler::getSectionInfo(cxuint sectionId) const
{
    return { };
}

bool AsmAmdCL2Handler::parsePseudoOp(const CString& firstName,
       const char* stmtPlace, const char* linePtr)
{
    return false;
}

bool AsmAmdCL2Handler::resolveRelocation(const AsmExpression* expr, AsmRelocation* reloc,
               bool& withReloc)
{
    return false;
}

bool AsmAmdCL2Handler::prepareBinary()
{
    return false;
}

void AsmAmdCL2Handler::writeBinary(std::ostream& os) const
{
}

void AsmAmdCL2Handler::writeBinary(Array<cxbyte>& array) const
{
}
