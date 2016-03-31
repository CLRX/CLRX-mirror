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
    AMDCL2OP_ACL_VERSION = 0, AMDCL2OP_ARG, AMDCL2OP_BSSDATA, AMDCL2OP_COMPILE_OPTIONS,
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

AsmAmdCL2Handler::AsmAmdCL2Handler(Assembler& assembler) : AsmFormatHandler(assembler),
        output{}, dataSection(0), bssSection(ASMSECT_NONE), 
        samplerInitSection(ASMSECT_NONE), extraSectionCount(0),
        innerExtraSectionCount(0)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::DATA, ELFSECTID_UNDEF, nullptr });
    savedSection = innerSavedSection = 0;
}

AsmAmdCL2Handler::~AsmAmdCL2Handler()
{
    for (Kernel* kernel: kernelStates)
        delete kernel;
}

void AsmAmdCL2Handler::saveCurrentSection()
{   /// save previous section
    if (assembler.currentKernel==ASMKERN_GLOBAL || assembler.currentKernel==ASMKERN_INNER)
        savedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentKernel]->savedSection = assembler.currentSection;
}

void AsmAmdCL2Handler::restoreCurrentAllocRegs()
{
    if (assembler.currentKernel!=ASMKERN_GLOBAL &&
        assembler.currentKernel!=ASMKERN_INNER &&
        assembler.currentSection==kernelStates[assembler.currentKernel]->codeSection)
        assembler.isaAssembler->setAllocatedRegisters(
                kernelStates[assembler.currentKernel]->allocRegs,
                kernelStates[assembler.currentKernel]->allocRegFlags);
}

void AsmAmdCL2Handler::saveCurrentAllocRegs()
{
    if (assembler.currentKernel!=ASMKERN_GLOBAL &&
        assembler.currentKernel!=ASMKERN_INNER &&
        assembler.currentSection==kernelStates[assembler.currentKernel]->codeSection)
    {
        size_t num;
        cxuint* destRegs = kernelStates[assembler.currentKernel]->allocRegs;
        const cxuint* regs = assembler.isaAssembler->getAllocatedRegisters(num,
                       kernelStates[assembler.currentKernel]->allocRegFlags);
        destRegs[0] = regs[0];
        destRegs[1] = regs[1];
    }
}

cxuint AsmAmdCL2Handler::addKernel(const char* kernelName)
{
    cxuint thisKernel = output.kernels.size();
    cxuint thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    Kernel kernelState{ ASMSECT_NONE, ASMSECT_NONE, ASMSECT_NONE,
            ASMSECT_NONE, ASMSECT_NONE, thisSection, ASMSECT_NONE };
    /* add new kernel and their section (.text) */
    kernelStates.push_back(new Kernel(std::move(kernelState)));
    sections.push_back({ thisKernel, AsmSectionType::CODE, ELFSECTID_TEXT, ".text" });
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    assembler.isaAssembler->setAllocatedRegisters();
    return thisKernel;
}

cxuint AsmAmdCL2Handler::addSection(const char* sectionName, cxuint kernelId)
{
    const cxuint thisSection = sections.size();
    Section section;
    section.kernelId = kernelId;
    
    if (kernelId == ASMKERN_GLOBAL)
    {
        auto out = extraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    else if (kernelId == ASMKERN_INNER)
    {
        auto out = innerExtraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = innerExtraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    else // not in kernel
        throw AsmFormatException("Extra section can be added in main or inner binary");
    
    sections.push_back(section);
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    
    assembler.currentKernel = kernelId;
    assembler.currentSection = thisSection;
    
    restoreCurrentAllocRegs();
    return thisSection;
}

cxuint AsmAmdCL2Handler::getSectionId(const char* sectionName) const
{
    if (assembler.currentKernel == ASMKERN_GLOBAL)
    {
        SectionMap::const_iterator it = extraSectionMap.find(sectionName);
        if (it != extraSectionMap.end())
            return it->second;
        return ASMSECT_NONE;
    }
    if (assembler.currentKernel == ASMKERN_INNER)
    {
        SectionMap::const_iterator it = innerExtraSectionMap.find(sectionName);
        if (it != innerExtraSectionMap.end())
            return it->second;
        return ASMSECT_NONE;
    }
    else
    {
        const Kernel& kernelState = *kernelStates[assembler.currentKernel];
        if (::strcmp(sectionName, ".text") == 0)
            return kernelState.codeSection;
        return ASMSECT_NONE;
    }
    return 0;
}

void AsmAmdCL2Handler::setCurrentKernel(cxuint kernel)
{
    if (kernel!=ASMKERN_GLOBAL && kernel!=ASMKERN_INNER && kernel >= kernelStates.size())
        throw AsmFormatException("KernelId out of range");
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    assembler.currentKernel = kernel;
    if (kernel == ASMKERN_GLOBAL)
        assembler.currentSection = savedSection;
    else if (kernel == ASMKERN_GLOBAL)
        assembler.currentSection = innerSavedSection; // inner section
    else // kernel
        assembler.currentSection = kernelStates[kernel]->savedSection;
    restoreCurrentAllocRegs();
}

void AsmAmdCL2Handler::setCurrentSection(cxuint sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    assembler.currentKernel = sections[sectionId].kernelId;
    assembler.currentSection = sectionId;
    restoreCurrentAllocRegs();
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
