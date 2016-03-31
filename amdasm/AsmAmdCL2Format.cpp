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

static const char* amdCL2PseudoOpNamesTbl[] =
{
    "acl_version", "arg", "bssdata", "compile_options", "config",
    "cws", "debugmode", "dims", "driver_version", "dx10clamp", "exceptions",
    "floatmode", "get_driver_version", "globaldata", "ieeemode", "inner",
    "isametadata", "localsize", "metadata", "privmode",
    "pgmrsrc1", "pgmrsrc2", "priority", "rwdata", "sampler",
    "samplerinit", "samplerreloc", "scratchbuffer",
    "setupargs", "sgprsnum", "stub", "tgsize",
    "useenqueue", "usesetup", "usesizes", "vgprsnum"
};

enum
{
    AMDCL2OP_ACL_VERSION, AMDCL2OP_ARG, AMDCL2OP_BSSDATA, AMDCL2OP_COMPILE_OPTIONS,
    AMDCL2OP_CONFIG, AMDCL2OP_CWS, AMDCL2OP_DEBUGMODE, AMDCL2OP_DIMS,
    AMDCL2OP_DRIVER_VERSION, AMDCL2OP_DX10CLAMP, AMDCL2OP_EXCEPTIONS,
    AMDCL2OP_FLOATMODE, AMDCL2OP_GET_DRIVER_VERSION, AMDCL2OP_GLOBALDATA,
    AMDCL2OP_IEEEMODE, AMDCL2OP_INNER, AMDCL2OP_ISAMETADATA, AMDCL2OP_LOCALSIZE,
    AMDCL2OP_METADATA, AMDCL2OP_PRIVMODE, AMDCL2OP_PGMRSRC1, AMDCL2OP_PGMRSRC2,
    AMDCL2OP_PRIORITY, AMDCL2OP_RWDATA, AMDCL2OP_SAMPLER, AMDCL2OP_SAMPLERINIT,
    AMDCL2OP_SAMPLERRELOC, AMDCL2OP_SCRATCHBUFFER, AMDCL2OP_SETUPARGS,
    AMDCL2OP_SGPRSNUM, AMDCL2OP_STUB, AMDCL2OP_TGSIZE, AMDCL2OP_USEENQUEUE,
    AMDCL2OP_USESETUP, AMDCL2OP_USESIZES, AMDCL2OP_VGPRSNUM
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
    const size_t pseudoOp = binaryFind(amdCL2PseudoOpNamesTbl, amdCL2PseudoOpNamesTbl +
                    sizeof(amdCL2PseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - amdCL2PseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case AMDCL2OP_ACL_VERSION:
            break;
        case AMDCL2OP_ARG:
            break;
        case AMDCL2OP_BSSDATA:
            break;
        case AMDCL2OP_COMPILE_OPTIONS:
            break;
        case AMDCL2OP_CONFIG:
            break;
        case AMDCL2OP_CWS:
            break;
        case AMDCL2OP_DEBUGMODE:
            break;
        case AMDCL2OP_DIMS:
            break;
        case AMDCL2OP_DRIVER_VERSION:
            break;
        case AMDCL2OP_DX10CLAMP:
            break;
        case AMDCL2OP_EXCEPTIONS:
            break;
        case AMDCL2OP_FLOATMODE:
            break;
        case AMDCL2OP_GET_DRIVER_VERSION:
            break;
        case AMDCL2OP_GLOBALDATA:
            break;
        case AMDCL2OP_IEEEMODE:
            break;
        case AMDCL2OP_INNER:
            break;
        case AMDCL2OP_ISAMETADATA:
            break;
        case AMDCL2OP_LOCALSIZE:
            break;
        case AMDCL2OP_METADATA:
            break;
        case AMDCL2OP_PRIVMODE:
            break;
        case AMDCL2OP_PGMRSRC1:
            break;
        case AMDCL2OP_PGMRSRC2:
            break;
        case AMDCL2OP_PRIORITY:
            break;
        case AMDCL2OP_RWDATA:
            break;
        case AMDCL2OP_SAMPLER:
            break;
        case AMDCL2OP_SAMPLERINIT:
            break;
        case AMDCL2OP_SAMPLERRELOC:
            break;
        case AMDCL2OP_SCRATCHBUFFER:
            break;
        case AMDCL2OP_SETUPARGS:
            break;
        case AMDCL2OP_SGPRSNUM:
            break;
        case AMDCL2OP_STUB:
            break;
        case AMDCL2OP_TGSIZE:
            break;
        case AMDCL2OP_USEENQUEUE:
            break;
        case AMDCL2OP_USESETUP:
            break;
        case AMDCL2OP_USESIZES:
            break;
        case AMDCL2OP_VGPRSNUM:
            break;
        default:
            return false;
    }
    return true;
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
