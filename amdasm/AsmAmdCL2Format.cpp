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
#include <cstdio>
#include <cstring>
#include <string>
#include <stack>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* amdCL2PseudoOpNamesTbl[] =
{
    "acl_version", "arch_minor", "arch_stepping",
    "arg", "bssdata", "compile_options", "config",
    "cws", "debugmode", "dims", "driver_version", "dx10clamp", "exceptions",
    "floatmode", "gdssize", "get_driver_version",
    "globaldata", "ieeemode", "inner",
    "isametadata", "localsize", "metadata", "pgmrsrc1", "pgmrsrc2",
    "priority", "privmode", "rwdata", "sampler",
    "samplerinit", "samplerreloc", "scratchbuffer", "setup",
    "setupargs", "sgprsnum", "stub", "tgsize", "useargs",
    "useenqueue", "usegeneric", "usesetup", "vgprsnum"
};

enum
{
    AMDCL2OP_ACL_VERSION = 0, AMDCL2OP_ARCH_MINOR, AMDCL2OP_ARCH_STEPPING,
    AMDCL2OP_ARG, AMDCL2OP_BSSDATA, AMDCL2OP_COMPILE_OPTIONS,
    AMDCL2OP_CONFIG, AMDCL2OP_CWS, AMDCL2OP_DEBUGMODE, AMDCL2OP_DIMS,
    AMDCL2OP_DRIVER_VERSION, AMDCL2OP_DX10CLAMP, AMDCL2OP_EXCEPTIONS,
    AMDCL2OP_FLOATMODE, AMDCL2OP_GDSSIZE, AMDCL2OP_GET_DRIVER_VERSION, AMDCL2OP_GLOBALDATA,
    AMDCL2OP_IEEEMODE, AMDCL2OP_INNER, AMDCL2OP_ISAMETADATA, AMDCL2OP_LOCALSIZE,
    AMDCL2OP_METADATA, AMDCL2OP_PGMRSRC1, AMDCL2OP_PGMRSRC2, AMDCL2OP_PRIORITY,
    AMDCL2OP_PRIVMODE, AMDCL2OP_RWDATA, AMDCL2OP_SAMPLER, AMDCL2OP_SAMPLERINIT,
    AMDCL2OP_SAMPLERRELOC, AMDCL2OP_SCRATCHBUFFER, AMDCL2OP_SETUP, AMDCL2OP_SETUPARGS,
    AMDCL2OP_SGPRSNUM, AMDCL2OP_STUB, AMDCL2OP_TGSIZE, AMDCL2OP_USEARGS,
    AMDCL2OP_USEENQUEUE, AMDCL2OP_USEGENERIC, AMDCL2OP_USESETUP, AMDCL2OP_VGPRSNUM
};

/*
 * AmdCatalyst format handler
 */

AsmAmdCL2Handler::AsmAmdCL2Handler(Assembler& assembler) : AsmFormatHandler(assembler),
        output{}, rodataSection(0), dataSection(ASMSECT_NONE), bssSection(ASMSECT_NONE), 
        samplerInitSection(ASMSECT_NONE), extraSectionCount(0),
        innerExtraSectionCount(0)
{
    output.archMinor = output.archStepping = UINT32_MAX;
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    sections.push_back({ ASMKERN_INNER, AsmSectionType::DATA, ELFSECTID_RODATA,
            ".rodata" });
    savedSection = innerSavedSection = 0;
}

AsmAmdCL2Handler::~AsmAmdCL2Handler()
{
    for (Kernel* kernel: kernelStates)
        delete kernel;
}

void AsmAmdCL2Handler::saveCurrentSection()
{   /// save previous section
    if (assembler.currentKernel==ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else if (assembler.currentKernel==ASMKERN_INNER)
        innerSavedSection = assembler.currentSection;
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

cxuint AsmAmdCL2Handler::getDriverVersion() const
{
    cxuint driverVersion = 0;
    if (output.driverVersion==0)
    {
        if (assembler.driverVersion==0) // just detect driver version
            driverVersion = detectAmdDriverVersion();
        else // from assembler setup
            driverVersion = assembler.driverVersion;
    }
    else
        driverVersion = output.driverVersion;
    return driverVersion;
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
    
    if (::strcmp(sectionName, ".rodata")==0 && (kernelId == ASMKERN_GLOBAL ||
            kernelId == ASMKERN_INNER))
    {
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global Data allowed only for new binary format");
        rodataSection = sections.size();
        sections.push_back({ ASMKERN_INNER,  AsmSectionType::DATA,
                ELFSECTID_RODATA, ".rodata" });
    }
    else if (::strcmp(sectionName, ".data")==0 && (kernelId == ASMKERN_GLOBAL ||
            kernelId == ASMKERN_INNER))
    {
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global RWData allowed only for new binary format");
        dataSection = sections.size();
        sections.push_back({ ASMKERN_INNER,  AsmSectionType::AMDCL2_RWDATA,
                ELFSECTID_DATA, ".data" });
    }
    else if (::strcmp(sectionName, ".bss")==0 && (kernelId == ASMKERN_GLOBAL ||
            kernelId == ASMKERN_INNER))
    {
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global BSS allowed only for new binary format");
        bssSection = sections.size();
        sections.push_back({ ASMKERN_INNER,  AsmSectionType::AMDCL2_BSS,
                ELFSECTID_BSS, ".bss" });
    }
    else if (kernelId == ASMKERN_GLOBAL)
    {
        Section section;
        section.kernelId = kernelId;
        auto out = extraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
        sections.push_back(section);
    }
    else // add inner section (even if we inside kernel)
    {
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Inner section are allowed "
                        "only for new binary format");
        
        Section section;
        section.kernelId = ASMKERN_INNER;
        auto out = innerExtraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = innerExtraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
        sections.push_back(section);
    }
    
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
        if (::strcmp(sectionName, ".rodata")==0)
            return rodataSection;
        else if (::strcmp(sectionName, ".data")==0)
            return dataSection;
        else if (::strcmp(sectionName, ".bss")==0)
            return bssSection;
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
    
    if (sections[sectionId].type == AsmSectionType::DATA)
    {
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global Data allowed only for new binary format");
    }
    if (sections[sectionId].type == AsmSectionType::AMDCL2_RWDATA)
    {
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global RWData allowed only for new binary format");
    }
    if (sections[sectionId].type == AsmSectionType::AMDCL2_BSS)
    {
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global BSS allowed only for new binary format");
    }
    
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
    {   // global data, rwdata and bss are relocatable sections (we set unresolvable flag)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_UNRESOLVABLE;
        if (info.type != AsmSectionType::AMDCL2_BSS)
            info.flags |= ASMSECT_WRITEABLE;
    }
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

void AsmAmdCL2PseudoOps::setArchMinor(AsmAmdCL2Handler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    const char* valuePlace = linePtr;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    asmr.printWarningForRange(sizeof(cxuint)<<3, value,
                 asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.archMinor = value;
}

void AsmAmdCL2PseudoOps::setArchStepping(AsmAmdCL2Handler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    const char* valuePlace = linePtr;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    asmr.printWarningForRange(sizeof(cxuint)<<3, value,
                 asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.archStepping = value;
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
    
    cxuint driverVersion = handler.getDriverVersion();
    std::pair<AsmSymbolMap::iterator, bool> res = asmr.globalScope.symbolMap.insert(
                std::make_pair(symName, AsmSymbol(&asmr.globalScope,
                                      ASMSECT_ABS, driverVersion)));
    if (!res.second)
    {   // found
        if (res.first->second.onceDefined && res.first->second.isDefined()) // if label
            asmr.printError(symNamePlace, (std::string("Symbol '")+symName.c_str()+
                        "' is already defined").c_str());
        else
            asmr.setSymbol(*res.first, driverVersion, ASMSECT_ABS);
    }
}

void AsmAmdCL2PseudoOps::doInner(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    try
    { handler.setCurrentKernel(ASMKERN_INNER); }
    catch(const AsmFormatException& ex) // if error
    {
        asmr.printError(pseudoOpPlace, ex.what());
        return;
    }
    
    asmr.currentOutPos = asmr.sections[asmr.currentSection].getSize();
}

void AsmAmdCL2PseudoOps::doGlobalData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (handler.getDriverVersion() < 191205)
    {
        asmr.printError(pseudoOpPlace, "Global Data allowed only for new binary format");
        return;
    }
    
    if (handler.rodataSection==ASMSECT_NONE)
    {   /* add this section */
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_INNER,  AsmSectionType::DATA,
            ELFSECTID_RODATA, ".rodata" });
        handler.rodataSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, handler.rodataSection);
}

void AsmAmdCL2PseudoOps::doRwData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.getDriverVersion() < 191205)
    {
        asmr.printError(pseudoOpPlace, "Global RWData allowed only for new binary format");
        return;
    }
    
    if (handler.dataSection==ASMSECT_NONE)
    {   /* add this section */
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_INNER,  AsmSectionType::AMDCL2_RWDATA,
            ELFSECTID_DATA, ".data" });
        handler.dataSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, handler.dataSection);
}

void AsmAmdCL2PseudoOps::doBssData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    if (handler.getDriverVersion() < 191205)
    {
        asmr.printError(pseudoOpPlace, "Global BSS allowed only for new binary format");
        return;
    }
    
    uint64_t sectionAlign = 0;
    bool good = true;
    // parse alignment
    skipSpacesToEnd(linePtr, end);
    if (linePtr+6<end && ::strncasecmp(linePtr, "align", 5)==0 && !isAlpha(linePtr[5]))
    {   // if alignment
        linePtr+=5;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr=='=')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            const char* valuePtr = linePtr;
            if (getAbsoluteValueArg(asmr, sectionAlign, linePtr, true))
            {
                if (sectionAlign!=0 && (1ULL<<(63-CLZ64(sectionAlign))) != sectionAlign)
                {
                    asmr.printError(valuePtr, "Alignment must be power of two or zero");
                    good = false;
                }
            }
            else
                good = false;
        }
        else
        {
            asmr.printError(linePtr, "Expected '=' after 'align'");
            good = false;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.bssSection==ASMSECT_NONE)
    {   /* add this section */
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_INNER,  AsmSectionType::AMDCL2_BSS,
            ELFSECTID_BSS, ".bss" });
        handler.bssSection = thisSection;
    }
    
    asmr.goToSection(pseudoOpPlace, handler.bssSection, sectionAlign);
}

void AsmAmdCL2PseudoOps::doSamplerInit(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.getDriverVersion() < 191205)
    {
        asmr.printError(pseudoOpPlace, "SamplerInit allowed only for new binary format");
        return;
    }
    if (handler.output.samplerConfig)
    {   // error
        asmr.printError(pseudoOpPlace,
                "SamplerInit is illegal if sampler definitions are present");
        return;
    }
    
    if (handler.samplerInitSection==ASMSECT_NONE)
    {   /* add this section */
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_INNER,  AsmSectionType::AMDCL2_SAMPLERINIT,
            AMDCL2SECTID_SAMPLERINIT, nullptr });
        handler.samplerInitSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, handler.samplerInitSection);
}

void AsmAmdCL2PseudoOps::doSampler(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel!=ASMKERN_GLOBAL && asmr.currentKernel!=ASMKERN_INNER &&
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    if (handler.getDriverVersion() < 191205)
    {
        asmr.printError(pseudoOpPlace, "Sampler allowed only for new binary format");
        return;
    }
    
    bool inMain = asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    if (!inMain)
    {
        if (linePtr == end)
            return; /* if no samplers */
        AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
        do {
            uint64_t value = 0;
            const char* valuePlace = linePtr;
            if (getAbsoluteValueArg(asmr, value, linePtr, true))
            {
                asmr.printWarningForRange(sizeof(cxuint)<<3, value,
                                 asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                config.samplers.push_back(value);
            }
        } while(skipCommaForMultipleArgs(asmr, linePtr));
    }
    else
    {   // global sampler definitions
        if (handler.samplerInitSection!=ASMSECT_NONE)
        {   // error
            asmr.printError(pseudoOpPlace,
                    "Illegal sampler definition if samplerinit was defined");
            return;
        }
        handler.output.samplerConfig = true;
        if (linePtr == end)
            return; /* if no samplers */
        do {
            uint64_t value = 0;
            const char* valuePlace = linePtr;
            if (getAbsoluteValueArg(asmr, value, linePtr, true))
            {
                asmr.printWarningForRange(sizeof(cxuint)<<3, value,
                                 asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                handler.output.samplers.push_back(value);
            }
        } while(skipCommaForMultipleArgs(asmr, linePtr));
    }
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmAmdCL2PseudoOps::doSamplerReloc(AsmAmdCL2Handler& handler,
                 const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel!=ASMKERN_GLOBAL && asmr.currentKernel!=ASMKERN_INNER)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of samplerreloc pseudo-op");
        return;
    }
    if (handler.getDriverVersion() < 191205)
    {
        asmr.printError(pseudoOpPlace, "SamplerReloc allowed only for new binary format");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* offsetPlace = linePtr;
    uint64_t samplerId = 0;
    uint64_t offset = 0;
    cxuint sectionId = 0;
    bool good = getAnyValueArg(asmr, offset, sectionId, linePtr);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    good |= getAbsoluteValueArg(asmr, samplerId, linePtr, true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (sectionId != ASMSECT_ABS && sectionId != handler.rodataSection)
    {
        asmr.printError(offsetPlace, "Offset can be an absolute value "
                    "or globaldata place");
        return;
    }
    // put to sampler offsets
    if (handler.output.samplerOffsets.size() <= samplerId)
        handler.output.samplerOffsets.resize(samplerId+1);
    handler.output.samplerOffsets[samplerId] = offset;
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
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                const cxuint maxLocalSize = getGPUMaxLocalSize(arch);
                if (value > maxLocalSize)
                {
                    char buf[64];
                    snprintf(buf, 64, "LocalSize out of range (0-%u)", maxLocalSize);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case AMDCL2CVAL_GDSSIZE:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                const cxuint maxGDSSize = getGPUMaxGDSSize(arch);
                if (value > maxGDSSize)
                {
                    char buf[64];
                    snprintf(buf, 64, "GDSSize out of range (0-%u)", maxGDSSize);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case AMDCL2CVAL_PGMRSRC1:
            case AMDCL2CVAL_PGMRSRC2:
                asmr.printWarningForRange(32, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
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
        case AMDCL2CVAL_GDSSIZE:
            config.gdsSize = value;
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
        case AMDCL2CVAL_USEARGS:
            config.useArgs = true;
            break;
        case AMDCL2CVAL_USESETUP:
            config.useSetup = true;
            break;
        case AMDCL2CVAL_USEENQUEUE:
            config.useEnqueue = true;
            break;
        case AMDCL2CVAL_USEGENERIC:
            config.useGeneric = true;
            break;
        default:
            break;
    }
}

void AsmAmdCL2PseudoOps::setDimensions(AsmAmdCL2Handler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    cxuint dimMask = 0;
    if (!parseDimensions(asmr, linePtr, dimMask))
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

/// AMD OpenCL kernel argument description
struct CLRX_INTERNAL IntAmdCL2KernelArg
{
    const char* argName;
    const char* typeName;
    KernelArgType argType;
    KernelArgType pointerType;
    KernelPtrSpace ptrSpace;
    cxbyte ptrAccess;
    cxbyte used;
};

static const IntAmdCL2KernelArg setupArgsTable64[] =
{
    { "_.global_offset_0", "size_t", KernelArgType::LONG, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 },
    { "_.global_offset_1", "size_t", KernelArgType::LONG, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 },
    { "_.global_offset_2", "size_t", KernelArgType::LONG, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 },
    { "_.printf_buffer", "size_t", KernelArgType::POINTER, KernelArgType::VOID,
        KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, AMDCL2_ARGUSED_READ_WRITE },
    { "_.vqueue_pointer", "size_t", KernelArgType::LONG, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 },
    { "_.aqlwrap_pointer", "size_t", KernelArgType::LONG, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 }
};

static const IntAmdCL2KernelArg setupArgsTable32[] =
{
    { "_.global_offset_0", "size_t", KernelArgType::INT, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 },
    { "_.global_offset_1", "size_t", KernelArgType::INT, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 },
    { "_.global_offset_2", "size_t", KernelArgType::INT, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 },
    { "_.printf_buffer", "size_t", KernelArgType::POINTER, KernelArgType::VOID,
        KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, AMDCL2_ARGUSED_READ_WRITE },
    { "_.vqueue_pointer", "size_t", KernelArgType::INT, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 },
    { "_.aqlwrap_pointer", "size_t", KernelArgType::INT, KernelArgType::VOID,
        KernelPtrSpace::NONE, KARG_PTR_NORMAL, 0 }
};


void AsmAmdCL2PseudoOps::doSetupArgs(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
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
    if (!kernelState.argNamesSet.empty())
    {
        asmr.printError(pseudoOpPlace, "SetupArgs must be as first in argument list");
        return;
    }
    
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    const IntAmdCL2KernelArg* argTable = asmr._64bit ? setupArgsTable64 : setupArgsTable32;
    for (size_t i = 0; i < 6; i++)
    {
        const IntAmdCL2KernelArg& arg = argTable[i];
        kernelState.argNamesSet.insert(arg.argName);
        config.args.push_back({arg.argName, arg.typeName, arg.argType, arg.pointerType,
                arg.ptrSpace, arg.ptrAccess, arg.used });
    }
}

void AsmAmdCL2PseudoOps::addMetadata(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
    {
        asmr.printError(pseudoOpPlace, "Metadata can be defined only inside kernel");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace,
                    "Metadata can't be defined if configuration was defined");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint& metadataSection = handler.kernelStates[asmr.currentKernel]->metadataSection;
    if (metadataSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMDCL2_METADATA,
            ELFSECTID_UNDEF, nullptr });
        metadataSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, metadataSection);
}

void AsmAmdCL2PseudoOps::addISAMetadata(AsmAmdCL2Handler& handler,
                const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
    {
        asmr.printError(pseudoOpPlace, "ISAMetadata can be defined only inside kernel");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace,
                    "ISAMetadata can't be defined if configuration was defined");
        return;
    }
    if (handler.getDriverVersion() >= 191205)
    {
        asmr.printError(pseudoOpPlace, "ISA Metadata allowed only for old binary format");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint& isaMDSection = handler.kernelStates[asmr.currentKernel]->isaMetadataSection;
    if (isaMDSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel,
                AsmSectionType::AMDCL2_ISAMETADATA, ELFSECTID_UNDEF, nullptr });
        isaMDSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, isaMDSection);
}

void AsmAmdCL2PseudoOps::addKernelSetup(AsmAmdCL2Handler& handler,
                const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
    {
        asmr.printError(pseudoOpPlace, "Setup can be defined only inside kernel");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace,
                    "Setup can't be defined if configuration was defined");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint& setupSection = handler.kernelStates[asmr.currentKernel]->setupSection;
    if (setupSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMDCL2_SETUP,
            ELFSECTID_UNDEF, nullptr });
        setupSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, setupSection);
}

void AsmAmdCL2PseudoOps::addKernelStub(AsmAmdCL2Handler& handler,
                const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
    {
        asmr.printError(pseudoOpPlace, "Stub can be defined only inside kernel");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace,
                    "Stub can't be defined if configuration was defined");
        return;
    }
    if (handler.getDriverVersion() >= 191205)
    {
        asmr.printError(pseudoOpPlace, "Stub allowed only for old binary format");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint& stubSection = handler.kernelStates[asmr.currentKernel]->stubSection;
    if (stubSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMDCL2_STUB,
            ELFSECTID_UNDEF, nullptr });
        stubSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, stubSection);
}

void AsmAmdCL2PseudoOps::doConfig(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
    {
        asmr.printError(pseudoOpPlace, "Kernel config can be defined only inside kernel");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    AsmAmdCL2Handler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.metadataSection!=ASMSECT_NONE || kernel.isaMetadataSection!=ASMSECT_NONE ||
        kernel.setupSection!=ASMSECT_NONE || kernel.stubSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace, "Config can't be defined if metadata,header,setup,"
                        "stub section exists");
        return;
    }
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (kernel.configSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::CONFIG,
            ELFSECTID_UNDEF, nullptr });
        kernel.configSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, kernel.configSection);
    handler.output.kernels[asmr.currentKernel].useConfig = true;
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
        case AMDCL2OP_ARCH_MINOR:
            AsmAmdCL2PseudoOps::setArchMinor(*this, linePtr);
            break;
        case AMDCL2OP_ARCH_STEPPING:
            AsmAmdCL2PseudoOps::setArchStepping(*this, linePtr);
            break;
        case AMDCL2OP_ARG:
            AsmAmdCL2PseudoOps::doArg(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_BSSDATA:
            AsmAmdCL2PseudoOps::doBssData(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_COMPILE_OPTIONS:
            AsmAmdCL2PseudoOps::setCompileOptions(*this, linePtr);
            break;
        case AMDCL2OP_CONFIG:
            AsmAmdCL2PseudoOps::doConfig(*this, stmtPlace, linePtr);
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
        case AMDCL2OP_GDSSIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_GDSSIZE);
            break;
        case AMDCL2OP_GET_DRIVER_VERSION:
            AsmAmdCL2PseudoOps::getDriverVersion(*this, linePtr);
            break;
        case AMDCL2OP_GLOBALDATA:
            AsmAmdCL2PseudoOps::doGlobalData(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_IEEEMODE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_IEEEMODE);
            break;
        case AMDCL2OP_INNER:
            AsmAmdCL2PseudoOps::doInner(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_ISAMETADATA:
            AsmAmdCL2PseudoOps::addISAMetadata(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_LOCALSIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_LOCALSIZE);
            break;
        case AMDCL2OP_METADATA:
            AsmAmdCL2PseudoOps::addMetadata(*this, stmtPlace, linePtr);
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
            AsmAmdCL2PseudoOps::doRwData(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_SAMPLER:
            AsmAmdCL2PseudoOps::doSampler(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_SAMPLERINIT:
            AsmAmdCL2PseudoOps::doSamplerInit(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_SAMPLERRELOC:
            AsmAmdCL2PseudoOps::doSamplerReloc(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_SCRATCHBUFFER:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_SCRATCHBUFFER);
            break;
        case AMDCL2OP_SETUP:
            AsmAmdCL2PseudoOps::addKernelSetup(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_SETUPARGS:
            AsmAmdCL2PseudoOps::doSetupArgs(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_SGPRSNUM:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_SGPRSNUM);
            break;
        case AMDCL2OP_STUB:
            AsmAmdCL2PseudoOps::addKernelStub(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_TGSIZE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_TGSIZE);
            break;
        case AMDCL2OP_USEARGS:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_USEARGS);
            break;
        case AMDCL2OP_USEENQUEUE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_USEENQUEUE);
            break;
        case AMDCL2OP_USEGENERIC:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_USEGENERIC);
            break;
        case AMDCL2OP_USESETUP:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_USESETUP);
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

bool AsmAmdCL2Handler::prepareBinary()
{
    bool good = true;
    if (assembler.isaAssembler!=nullptr)
        saveCurrentAllocRegs(); // save last kernel allocated registers to kernel state
    
    output.is64Bit = assembler.is64Bit();
    output.deviceType = assembler.getDeviceType();
    /* initialize sections */
    const size_t sectionsNum = sections.size();
    const size_t kernelsNum = kernelStates.size();
    for (size_t i = 0; i < sectionsNum; i++)
    {
        const AsmSection& asmSection = assembler.sections[i];
        const Section& section = sections[i];
        const size_t sectionSize = asmSection.getSize();
        const cxbyte* sectionData = (!asmSection.content.empty()) ?
                asmSection.content.data() : (const cxbyte*)"";
        AmdCL2KernelInput* kernel = (section.kernelId!=ASMKERN_GLOBAL) ?
                    &output.kernels[section.kernelId] : nullptr;
        
        switch(asmSection.type)
        {
            case AsmSectionType::CODE:
                kernel->codeSize = sectionSize;
                kernel->code = sectionData;
                break;
            case AsmSectionType::AMDCL2_METADATA:
                kernel->metadataSize = sectionSize;
                kernel->metadata = sectionData;
                break;
            case AsmSectionType::AMDCL2_ISAMETADATA:
                kernel->isaMetadataSize = sectionSize;
                kernel->isaMetadata = sectionData;
                break;
            case AsmSectionType::DATA:
                output.globalDataSize = sectionSize;
                output.globalData = sectionData;
                break;
            case AsmSectionType::AMDCL2_RWDATA:
                output.rwDataSize = sectionSize;
                output.rwData = sectionData;
                break;
            case AsmSectionType::AMDCL2_BSS:
                output.bssAlignment = asmSection.alignment;
                output.bssSize = asmSection.getSize();
                break;
            case AsmSectionType::AMDCL2_SAMPLERINIT:
                output.samplerInitSize = sectionSize;
                output.samplerInit = sectionData;
                break;
            case AsmSectionType::AMDCL2_SETUP:
                kernel->setupSize = sectionSize;
                kernel->setup = sectionData;
                break;
            case AsmSectionType::AMDCL2_STUB:
                kernel->stubSize = sectionSize;
                kernel->stub = sectionData;
                break;
            case AsmSectionType::EXTRA_PROGBITS:
            case AsmSectionType::EXTRA_NOTE:
            case AsmSectionType::EXTRA_NOBITS:
            case AsmSectionType::EXTRA_SECTION:
            {
                uint32_t elfSectType =
                       (asmSection.type==AsmSectionType::EXTRA_NOTE) ? SHT_NOTE :
                       (asmSection.type==AsmSectionType::EXTRA_NOBITS) ? SHT_NOBITS :
                             SHT_PROGBITS;
                uint32_t elfSectFlags = 
                    ((asmSection.flags&ASMELFSECT_ALLOCATABLE) ? SHF_ALLOC : 0) |
                    ((asmSection.flags&ASMELFSECT_WRITEABLE) ? SHF_WRITE : 0) |
                    ((asmSection.flags&ASMELFSECT_EXECUTABLE) ? SHF_EXECINSTR : 0);
                if (section.kernelId == ASMKERN_GLOBAL)
                    output.extraSections.push_back({section.name, sectionSize, sectionData,
                            asmSection.alignment!=0?asmSection.alignment:1, elfSectType,
                            elfSectFlags, ELFSECTID_NULL, 0, 0 });
                else
                    output.innerExtraSections.push_back({section.name, sectionSize,
                            sectionData, asmSection.alignment!=0?asmSection.alignment:1,
                            elfSectType, elfSectFlags, ELFSECTID_NULL, 0, 0 });
                break;
            }
            default: // ignore other sections
                break;
        }
    }
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(assembler.deviceType);
    cxuint maxTotalSgprsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    // set up number of the allocated SGPRs and VGPRs for kernel
    for (size_t i = 0; i < kernelsNum; i++)
    {
        if (!output.kernels[i].useConfig)
            continue;
        AmdCL2KernelConfig& config = output.kernels[i].config;
        cxuint userSGPRsNum = 4;
        if (config.useGeneric)
            userSGPRsNum = 12;
        else if (config.useEnqueue)
            userSGPRsNum = 10;
        else if (config.useSetup)
            userSGPRsNum = 8;
        else if (config.useArgs)
            userSGPRsNum = 6;
        
        /* include userData sgprs */
        cxuint dimMask = (config.dimMask!=BINGEN_DEFAULT) ? config.dimMask :
                ((config.pgmRSRC2>>7)&7);
        cxuint minRegsNum[2];
        getGPUSetupMinRegistersNum(arch, dimMask, userSGPRsNum,
                   ((config.tgSize) ? GPUSETUP_TGSIZE_EN : 0) |
                   ((config.scratchBufferSize!=0) ? GPUSETUP_SCRATCH_EN : 0), minRegsNum);
        
        const cxuint neededExtraSGPRsNum = arch==GPUArchitecture::GCN1_2 ? 6 : 4;
        const cxuint extraSGPRsNum = (config.useEnqueue || config.useGeneric) ?
                    neededExtraSGPRsNum : 2;
        if (config.usedSGPRsNum!=BINGEN_DEFAULT)
        {   // check only if sgprsnum set explicitly
            if (maxTotalSgprsNum-extraSGPRsNum < config.usedSGPRsNum)
            {
                char numBuf[64];
                snprintf(numBuf, 64, "(max %u)", maxTotalSgprsNum);
                assembler.printError(assembler.kernels[i].sourcePos, (std::string(
                    "Number of total SGPRs for kernel '")+
                    output.kernels[i].kernelName.c_str()+"' is too high "+numBuf).c_str());
                good = false;
            }
        }
        
        if (config.usedSGPRsNum==BINGEN_DEFAULT)
            config.usedSGPRsNum = std::min(maxTotalSgprsNum-extraSGPRsNum, 
                std::max(minRegsNum[0], kernelStates[i]->allocRegs[0]));
        if (config.usedVGPRsNum==BINGEN_DEFAULT)
            config.usedVGPRsNum = std::max(minRegsNum[1], kernelStates[i]->allocRegs[1]);
    }
    
    /* put kernels relocations */
    for (const AsmRelocation& reloc: assembler.relocations)
    {   /* put only code relocations */
        cxuint kernelId = sections[reloc.sectionId].kernelId;
        cxuint symbol = sections[reloc.relSectionId].type==AsmSectionType::DATA ? 0 :
            (sections[reloc.relSectionId].type==AsmSectionType::AMDCL2_RWDATA ? 1 : 2);
        output.kernels[kernelId].relocations.push_back({reloc.offset, reloc.type,
                    symbol, size_t(reloc.addend) });
    }
    
    for (AmdCL2KernelInput& kernel: output.kernels)
        std::sort(kernel.relocations.begin(), kernel.relocations.end(),
                [](const AmdCL2RelInput& a, const AmdCL2RelInput& b)
                { return a.offset < b.offset; });
    
    /* put extra symbols */
    if (assembler.flags & ASM_FORCE_ADD_SYMBOLS)
    {
        std::vector<size_t> codeOffsets(kernelsNum);
        size_t codeOffset = 0;
        // make offset translation table
        for (size_t i = 0; i < kernelsNum; i++)
        {
            const AmdCL2KernelInput& kernel = output.kernels[i];
            codeOffset += (kernel.useConfig) ? 256 : kernel.setupSize;
            codeOffsets[i] = codeOffset;
            codeOffset += (kernel.codeSize+255)&~size_t(255);
        }
        
        for (const AsmSymbolEntry& symEntry: assembler.globalScope.symbolMap)
        {
            if (!symEntry.second.hasValue ||
                ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // unresolved or local
            cxuint binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            
            BinSymbol binSym = { symEntry.first, symEntry.second.value,
                        symEntry.second.size, binSectId, false, symEntry.second.info,
                        symEntry.second.other };
            
            if (symEntry.second.sectionId == ASMSECT_ABS ||
                sections[symEntry.second.sectionId].kernelId == ASMKERN_GLOBAL)
                output.extraSymbols.push_back(std::move(binSym));
            else if (sections[symEntry.second.sectionId].kernelId == ASMKERN_INNER)
                // to kernel extra symbols.
                output.innerExtraSymbols.push_back(std::move(binSym));
            else if (sections[symEntry.second.sectionId].type == AsmSectionType::CODE)
            {
                binSym.value += codeOffsets[sections[symEntry.second.sectionId].kernelId];
                output.innerExtraSymbols.push_back(std::move(binSym));
            }
        }
    }
    // driver version setup
    if (output.driverVersion==0 && (assembler.flags&ASM_TESTRUN)==0)
    {
        if (assembler.driverVersion==0) // just detect driver version
            output.driverVersion = detectAmdDriverVersion();
        else // from assembler setup
            output.driverVersion = assembler.driverVersion;
    }
    return good;
}

bool AsmAmdCL2Handler::resolveSymbol(const AsmSymbol& symbol, uint64_t& value,
                 cxuint& sectionId)
{
    if (!assembler.isResolvableSection(symbol.sectionId))
    {
        value = symbol.value;
        sectionId = symbol.sectionId;
        return true;
    }
    return false;
}

bool AsmAmdCL2Handler::resolveRelocation(const AsmExpression* expr, uint64_t& outValue,
                 cxuint& outSectionId)
{
    const AsmExprTarget& target = expr->getTarget();
    const AsmExprTargetType tgtType = target.type;
    if ((tgtType!=ASMXTGT_DATA32 &&
        !assembler.isaAssembler->relocationIsFit(32, tgtType)))
    {
        assembler.printError(expr->getSourcePos(),
                        "Can't resolve expression for non 32-bit integer");
        return false;
    }
    if (target.sectionId==ASMSECT_ABS ||
        assembler.sections[target.sectionId].type!=AsmSectionType::CODE)
    {
        assembler.printError(expr->getSourcePos(), "Can't resolve expression outside "
                "code section");
        return false;
    }
    const Array<AsmExprOp>& ops = expr->getOps();
    
    size_t relOpStart = 0;
    size_t relOpEnd = ops.size();
    RelocType relType = RELTYPE_LOW_32BIT;
    // checking what is expression
    AsmExprOp lastOp = ops.back();
    if (lastOp==AsmExprOp::BIT_AND || lastOp==AsmExprOp::MODULO ||
        lastOp==AsmExprOp::SIGNED_MODULO || lastOp==AsmExprOp::DIVISION ||
        lastOp==AsmExprOp::SIGNED_DIVISION || lastOp==AsmExprOp::SHIFT_RIGHT)
    {   // check low or high relocation
        relOpStart = 0;
        relOpEnd = expr->toTop(ops.size()-2);
        /// evaluate second argument
        cxuint tmpSectionId;
        uint64_t secondArg;
        if (!expr->evaluate(assembler, relOpEnd, ops.size()-1, secondArg, tmpSectionId))
            return false;
        if (tmpSectionId!=ASMSECT_ABS)
        {   // must be absolute
            assembler.printError(expr->getSourcePos(),
                        "Second argument for relocation operand must be absolute");
            return false;
        }
        bool good = true;
        switch (lastOp)
        {
            case AsmExprOp::BIT_AND:
                relType = RELTYPE_LOW_32BIT;
                good = ((secondArg & 0xffffffffULL) == 0xffffffffULL);
                break;
            case AsmExprOp::MODULO:
            case AsmExprOp::SIGNED_MODULO:
                relType = RELTYPE_LOW_32BIT;
                good = ((secondArg>>32)!=0 && (secondArg & 0xffffffffULL) == 0);
                break;
            case AsmExprOp::DIVISION:
            case AsmExprOp::SIGNED_DIVISION:
                relType = RELTYPE_HIGH_32BIT;
                good = (secondArg == 0x100000000ULL);
                break;
            case AsmExprOp::SHIFT_RIGHT:
                relType = RELTYPE_HIGH_32BIT;
                good = (secondArg == 32);
                break;
            default:
                break;
        }
        if (!good)
        {
            assembler.printError(expr->getSourcePos(),
                        "Can't resolve relocation for this expression");
            return false;
        }
    }
    // 
    cxuint relSectionId = 0;
    uint64_t relValue = 0;
    if (expr->evaluate(assembler, relOpStart, relOpEnd, relValue, relSectionId))
    {
        if (relSectionId!=rodataSection && relSectionId!=dataSection &&
            relSectionId!=bssSection)
        {
            assembler.printError(expr->getSourcePos(),
                     "Section of this expression must be a global data, rwdata or bss");
            return false;
        }
        outSectionId = ASMSECT_ABS;   // for filling values in code
        outValue = 0x55555555U; // for filling values in code
        size_t extraOffset = (tgtType!=ASMXTGT_DATA32) ? 4 : 0;
        AsmRelocation reloc = { target.sectionId, target.offset+extraOffset, relType };
        reloc.relSectionId = relSectionId;
        reloc.addend = relValue;
        assembler.relocations.push_back(reloc);
        return true;
    }
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
