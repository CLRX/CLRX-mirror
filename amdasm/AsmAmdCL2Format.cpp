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
    else // add inner section (even if we inside kernel)
    {
        kernelId = section.kernelId = ASMKERN_INNER;
        auto out = innerExtraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = innerExtraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    
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
    else 
    {
        if (assembler.currentKernel != ASMKERN_INNER)
        {
            const Kernel& kernelState = *kernelStates[assembler.currentKernel];
            if (::strcmp(sectionName, ".text") == 0)
                return kernelState.codeSection;
        }
        
        SectionMap::const_iterator it = innerExtraSectionMap.find(sectionName);
        if (it != innerExtraSectionMap.end())
            return it->second;
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
    else if (kernel == ASMKERN_INNER)
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
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    AsmFormatHandler::SectionInfo info;
    info.type = sections[sectionId].type;
    info.flags = 0;
    if (info.type == AsmSectionType::CODE)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
    else if (info.type == AsmSectionType::AMDCL2_BSS ||
            info.type == AsmSectionType::AMDCL2_RWDATA ||
            info.type == AsmSectionType::DATA)
        // global data, rwdata and bss are relocatable sections (we set unresolvable flag)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE |
                    ASMSECT_UNRESOLVABLE;
    else if (info.type != AsmSectionType::CONFIG)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    info.name = sections[sectionId].name;
    return info;
}

namespace CLRX
{

bool AsmAmdCL2PseudoOps::checkPseudoOpName(const CString& string)
{
    if (string.empty() || string[0] != '.')
        return false;
    const size_t pseudoOp = binaryFind(amdCL2PseudoOpNamesTbl, amdCL2PseudoOpNamesTbl +
                sizeof(amdCL2PseudoOpNamesTbl)/sizeof(char*), string.c_str()+1,
               CStringLess()) - amdCL2PseudoOpNamesTbl;
    return pseudoOp < sizeof(amdCL2PseudoOpNamesTbl)/sizeof(char*);
}

void AsmAmdCL2PseudoOps::setAclVersion(AsmAmdCL2Handler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (!asmr.parseString(out, linePtr))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.aclVersion = out;
}

void AsmAmdCL2PseudoOps::setCompileOptions(AsmAmdCL2Handler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (!asmr.parseString(out, linePtr))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.compileOptions = out;
}

void AsmAmdCL2PseudoOps::setDriverVersion(AsmAmdCL2Handler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.driverVersion = value;
}

void AsmAmdCL2PseudoOps::getDriverVersion(AsmAmdCL2Handler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    const char* symNamePlace = linePtr;
    const CString symName = extractSymName(linePtr, end, false);
    if (symName.empty())
    {
        asmr.printError(symNamePlace, "Illegal symbol name");
        return;
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint driverVersion = 0;
    if (handler.output.driverVersion==0)
    {
        if (asmr.driverVersion==0) // just detect driver version
            driverVersion = detectAmdDriverVersion();
        else // from assembler setup
            driverVersion = asmr.driverVersion;
    }
    else
        driverVersion = handler.output.driverVersion;
    
    std::pair<AsmSymbolMap::iterator, bool> res = asmr.symbolMap.insert(
                std::make_pair(symName, AsmSymbol(ASMSECT_ABS, driverVersion)));
    if (!res.second)
    {   // found
        if (res.first->second.onceDefined && res.first->second.isDefined()) // if label
            asmr.printError(symNamePlace, (std::string("Symbol '")+symName.c_str()+
                        "' is already defined").c_str());
        else
            asmr.setSymbol(*res.first, driverVersion, ASMSECT_ABS);
    }
}

void AsmAmdCL2PseudoOps::setConfigValue(AsmAmdCL2Handler& handler,
         const char* pseudoOpPlace, const char* linePtr, AmdCL2ConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value = BINGEN_NOTSUPPLIED;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    /* ranges checking */
    if (good)
    {
        switch(target)
        {
            case AMDCL2CVAL_SGPRSNUM:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
                if (value > maxSGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Used SGPRs number out of range (0-%u)", maxSGPRsNum);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case AMDCL2CVAL_VGPRSNUM:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxVGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_VGPR, 0);
                if (value > maxVGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Used VGPRs number out of range (0-%u)", maxVGPRsNum);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case AMDCL2CVAL_EXCEPTIONS:
                asmr.printWarningForRange(7, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0x7f;
                break;
            case AMDCL2CVAL_FLOATMODE:
                asmr.printWarningForRange(8, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0xff;
                break;
            case AMDCL2CVAL_PRIORITY:
                asmr.printWarningForRange(2, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 3;
                break;
            case AMDCL2CVAL_LOCALSIZE:
                if (value > 32768)
                {
                    asmr.printError(valuePlace, "LocalSize out of range (0-32768)");
                    good = false;
                }
                break;
            default:
                break;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    // set value
    switch(target)
    {
        case AMDCL2CVAL_SGPRSNUM:
            config.usedSGPRsNum = value;
            break;
        case AMDCL2CVAL_VGPRSNUM:
            config.usedVGPRsNum = value;
            break;
        case AMDCL2CVAL_PGMRSRC1:
            config.pgmRSRC1 = value;
            break;
        case AMDCL2CVAL_PGMRSRC2:
            config.pgmRSRC2 = value;
            break;
        case AMDCL2CVAL_FLOATMODE:
            config.floatMode = value;
            break;
        case AMDCL2CVAL_LOCALSIZE:
            config.localSize = value;
            break;
        case AMDCL2CVAL_SCRATCHBUFFER:
            config.scratchBufferSize = value;
            break;
        case AMDCL2CVAL_PRIORITY:
            config.priority = value;
            break;
        case AMDCL2CVAL_EXCEPTIONS:
            config.exceptions = value;
            break;
        default:
            break;
    }
}

void AsmAmdCL2PseudoOps::setConfigBoolValue(AsmAmdCL2Handler& handler,
         const char* pseudoOpPlace, const char* linePtr, AmdCL2ConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    switch(target)
    {
        case AMDCL2CVAL_DEBUGMODE:
            config.debugMode = true;
            break;
        case AMDCL2CVAL_DX10CLAMP:
            config.dx10Clamp = true;
            break;
        case AMDCL2CVAL_IEEEMODE:
            config.ieeeMode = true;
            break;
        case AMDCL2CVAL_PRIVMODE:
            config.privilegedMode = true;
            break;
        case AMDCL2CVAL_TGSIZE:
            config.tgSize = true;
            break;
        case AMDCL2CVAL_USESETUP:
            config.useSetup = true;
            break;
        case AMDCL2CVAL_USESIZES:
            config.useSizes = true;
            break;
        case AMDCL2CVAL_USEENQUEUE:
            config.useEnqueue = true;
            break;
        default:
            break;
    }
}

void AsmAmdCL2PseudoOps::setDimensions(AsmAmdCL2Handler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    const char* dimPlace = linePtr;
    char buf[10];
    cxuint dimMask = 0;
    if (getNameArg(asmr, 10, buf, linePtr, "dimension set"))
    {
        toLowerString(buf);
        for (cxuint i = 0; buf[i]!=0; i++)
            if (buf[i]=='x')
                dimMask |= 1;
            else if (buf[i]=='y')
                dimMask |= 2;
            else if (buf[i]=='z')
                dimMask |= 4;
            else
            {
                asmr.printError(dimPlace, "Unknown dimension type");
                return;
            }
    }
    else // error
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.kernels[asmr.currentKernel].config.dimMask = dimMask;
}

void AsmAmdCL2PseudoOps::setCWS(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t out[3] = { 0, 0, 0 };
    if (!AsmAmdPseudoOps::parseCWS(asmr, pseudoOpPlace, linePtr, out))
        return;
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    config.reqdWorkGroupSize[0] = out[0];
    config.reqdWorkGroupSize[1] = out[1];
    config.reqdWorkGroupSize[2] = out[2];
}

void AsmAmdCL2PseudoOps::doArg(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of kernel argument");
        return;
    }
    
    auto& kernelState = *handler.kernelStates[asmr.currentKernel];
    AmdKernelArgInput argInput;
    if (!AsmAmdPseudoOps::parseArg(asmr, pseudoOpPlace, linePtr, kernelState.argNamesSet,
                    argInput, true))
        return;
    /* setup argument */
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    const CString argName = argInput.argName;
    config.args.push_back(std::move(argInput));
    /// put argName
    kernelState.argNamesSet.insert(argName);
}

};

bool AsmAmdCL2Handler::parsePseudoOp(const CString& firstName,
       const char* stmtPlace, const char* linePtr)
{
    const size_t pseudoOp = binaryFind(amdCL2PseudoOpNamesTbl, amdCL2PseudoOpNamesTbl +
                    sizeof(amdCL2PseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - amdCL2PseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case AMDCL2OP_ACL_VERSION:
            AsmAmdCL2PseudoOps::setAclVersion(*this, linePtr);
            break;
        case AMDCL2OP_ARG:
            AsmAmdCL2PseudoOps::doArg(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_BSSDATA:
            break;
        case AMDCL2OP_COMPILE_OPTIONS:
            AsmAmdCL2PseudoOps::setCompileOptions(*this, linePtr);
            break;
        case AMDCL2OP_CONFIG:
            break;
        case AMDCL2OP_CWS:
            AsmAmdCL2PseudoOps::setCWS(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_DEBUGMODE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_DEBUGMODE);
            break;
        case AMDCL2OP_DIMS:
            AsmAmdCL2PseudoOps::setDimensions(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_DRIVER_VERSION:
            AsmAmdCL2PseudoOps::setDriverVersion(*this, linePtr);
            break;
        case AMDCL2OP_DX10CLAMP:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_DX10CLAMP);
            break;
        case AMDCL2OP_EXCEPTIONS:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_EXCEPTIONS);
            break;
        case AMDCL2OP_FLOATMODE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_FLOATMODE);
            break;
        case AMDCL2OP_GET_DRIVER_VERSION:
            AsmAmdCL2PseudoOps::getDriverVersion(*this, linePtr);
            break;
        case AMDCL2OP_GLOBALDATA:
            break;
        case AMDCL2OP_IEEEMODE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_IEEEMODE);
            break;
        case AMDCL2OP_INNER:
            break;
        case AMDCL2OP_ISAMETADATA:
            break;
        case AMDCL2OP_LOCALSIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_LOCALSIZE);
            break;
        case AMDCL2OP_METADATA:
            break;
        case AMDCL2OP_PRIVMODE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_PRIVMODE);
            break;
        case AMDCL2OP_PGMRSRC1:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_PGMRSRC1);
            break;
        case AMDCL2OP_PGMRSRC2:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_PGMRSRC2);
            break;
        case AMDCL2OP_PRIORITY:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_PRIORITY);
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
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_SCRATCHBUFFER);
            break;
        case AMDCL2OP_SETUPARGS:
            break;
        case AMDCL2OP_SGPRSNUM:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_SGPRSNUM);
            break;
        case AMDCL2OP_STUB:
            break;
        case AMDCL2OP_TGSIZE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_TGSIZE);
            break;
        case AMDCL2OP_USEENQUEUE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_USEENQUEUE);
            break;
        case AMDCL2OP_USESETUP:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_USESETUP);
            break;
        case AMDCL2OP_USESIZES:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_USESIZES);
            break;
        case AMDCL2OP_VGPRSNUM:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_VGPRSNUM);
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
    AmdCL2GPUBinGenerator binGenerator(&output);
    binGenerator.generate(os);
}

void AsmAmdCL2Handler::writeBinary(Array<cxbyte>& array) const
{
    AmdCL2GPUBinGenerator binGenerator(&output);
    binGenerator.generate(array);
}
