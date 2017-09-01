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
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* galliumPseudoOpNamesTbl[] =
{
    "arg", "args", "config",
    "debugmode", "dims", "driver_version", "dx10clamp",
    "entry", "exceptions", "floatmode",
    "get_driver_version", "get_llvm_version",
    "globaldata", "ieeemode",
    "kcode", "kcodeend",
    "llvm_version", "localsize",
    "pgmrsrc1", "pgmrsrc2", "priority",
    "privmode", "proginfo",
    "scratchbuffer", "sgprsnum", "spilledsgprs", "spilledvgprs",
    "tgsize", "userdatanum", "vgprsnum"
};

enum
{
    GALLIUMOP_ARG = 0, GALLIUMOP_ARGS, GALLIUMOP_CONFIG,
    GALLIUMOP_DEBUGMODE, GALLIUMOP_DIMS, GALLIUMOP_DRIVER_VERSION, GALLIUMOP_DX10CLAMP,
    GALLIUMOP_ENTRY, GALLIUMOP_EXCEPTIONS, GALLIUMOP_FLOATMODE,
    GALLIUMOP_GET_DRIVER_VERSION, GALLIUMOP_GET_LLVM_VERSION,
    GALLIUMOP_GLOBALDATA, GALLIUMOP_IEEEMODE,
    GALLIUMOP_KCODE, GALLIUMOP_KCODEEND,
    GALLIUMOP_LLVM_VERSION, GALLIUMOP_LOCALSIZE,
    GALLIUMOP_PGMRSRC1, GALLIUMOP_PGMRSRC2, GALLIUMOP_PRIORITY,
    GALLIUMOP_PRIVMODE, GALLIUMOP_PROGINFO,
    GALLIUMOP_SCRATCHBUFFER, GALLIUMOP_SGPRSNUM,
    GALLIUMOP_SPILLEDSGPRS, GALLIUMOP_SPILLEDVGPRS,
    GALLIUMOP_TGSIZE, GALLIUMOP_USERDATANUM, GALLIUMOP_VGPRSNUM
};

void AsmGalliumHandler::Kernel::initializeAmdHsaKernelConfig()
{
    if (!config)
    {
        config.reset(new AsmROCmKernelConfig{});
        // set default values to kernel config
        ::memset(config.get(), 0xff, 128);
        ::memset(config->controlDirective, 0, 128);
        config->computePgmRsrc1 = config->computePgmRsrc2 = 0;
        config->enableSgprRegisterFlags = 0;
        config->enableFeatureFlags = 0;
        config->reserved1[0] = config->reserved1[1] = config->reserved1[2] = 0;
        config->dimMask = 0;
        config->usedVGPRsNum = BINGEN_DEFAULT;
        config->usedSGPRsNum = BINGEN_DEFAULT;
        config->userDataNum = BINGEN8_DEFAULT;
        config->ieeeMode = false;
        config->floatMode = 0xc0;
        config->priority = 0;
        config->exceptions = 0;
        config->tgSize = false;
        config->debugMode = false;
        config->privilegedMode = false;
        config->dx10Clamp = false;
    }
}

/*
 * GalliumCompute format handler
 */

AsmGalliumHandler::AsmGalliumHandler(Assembler& assembler): AsmFormatHandler(assembler),
             output{}, codeSection(0), dataSection(ASMSECT_NONE),
             commentSection(ASMSECT_NONE), extraSectionCount(0)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::CODE,
                ELFSECTID_TEXT, ".text" });
    inside = Inside::MAINLAYOUT;
    currentKcodeKernel = ASMKERN_GLOBAL;
    savedSection = 0;
    defaultLLVMVersion = detectLLVMCompilerVersion();
    defaultDriverVersion = detectMesaDriverVersion();
}

AsmGalliumHandler::~AsmGalliumHandler()
{
    for (Kernel* kernel: kernelStates)
        delete kernel;
}

cxuint AsmGalliumHandler::determineLLVMVersion() const
{
    if (assembler.getLLVMVersion() == 0)
        return defaultLLVMVersion;
    else
        return assembler.getLLVMVersion();
}

cxuint AsmGalliumHandler::determineDriverVersion() const
{
    if (assembler.getDriverVersion() == 0)
        return defaultDriverVersion;
    else
        return assembler.getDriverVersion();
}

cxuint AsmGalliumHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = output.kernels.size();
    cxuint thisSection = sections.size();
    output.addEmptyKernel(kernelName, determineLLVMVersion());
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG, ELFSECTID_UNDEF, nullptr });
    kernelStates.push_back(new Kernel{ thisSection, nullptr, false, 0 });
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    inside = Inside::MAINLAYOUT;
    return thisKernel;
}

cxuint AsmGalliumHandler::addSection(const char* sectionName, cxuint kernelId)
{
    const cxuint thisSection = sections.size();
    Section section;
    section.kernelId = ASMKERN_GLOBAL;  // we ignore input kernelId, we go to main
    
    if (::strcmp(sectionName, ".rodata") == 0) // data
    {
        if (dataSection!=ASMSECT_NONE)
            throw AsmFormatException("Only section '.rodata' can be in binary");
        dataSection = thisSection;
        section.type = AsmSectionType::DATA;
        section.elfBinSectId = ELFSECTID_RODATA;
        section.name = ".rodata"; // set static name (available by whole lifecycle)
    }
    else if (::strcmp(sectionName, ".text") == 0) // code
    {
        if (codeSection!=ASMSECT_NONE)
            throw AsmFormatException("Only one section '.text' can be in binary");
        codeSection = thisSection;
        section.type = AsmSectionType::CODE;
        section.elfBinSectId = ELFSECTID_TEXT;
        section.name = ".text"; // set static name (available by whole lifecycle)
    }
    else if (::strcmp(sectionName, ".comment") == 0) // comment
    {
        if (commentSection!=ASMSECT_NONE)
            throw AsmFormatException("Only one section '.comment' can be in binary");
        commentSection = thisSection;
        section.type = AsmSectionType::GALLIUM_COMMENT;
        section.elfBinSectId = ELFSECTID_COMMENT;
        section.name = ".comment"; // set static name (available by whole lifecycle)
    }
    else
    {
        auto out = extraSectionMap.insert(std::make_pair(CString(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = extraSectionCount++;
        /// reference entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    sections.push_back(section);
    
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = thisSection;
    inside = Inside::MAINLAYOUT;
    return thisSection;
}

cxuint AsmGalliumHandler::getSectionId(const char* sectionName) const
{
    if (::strcmp(sectionName, ".rodata") == 0) // data
        return dataSection;
    else if (::strcmp(sectionName, ".text") == 0) // code
        return codeSection;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        return commentSection;
    else
    {
        SectionMap::const_iterator it = extraSectionMap.find(sectionName);
        if (it != extraSectionMap.end())
            return it->second;
    }
    return ASMSECT_NONE;
}

void AsmGalliumHandler::setCurrentKernel(cxuint kernel)
{   // set kernel and their default section
    if (kernel != ASMKERN_GLOBAL && kernel >= kernelStates.size())
        throw AsmFormatException("KernelId out of range");
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentKernel = kernel;
    if (kernel != ASMKERN_GLOBAL)
        assembler.currentSection = kernelStates[kernel]->defaultSection;
    else // default main section
        assembler.currentSection = savedSection;
    inside = Inside::MAINLAYOUT;
}

void AsmGalliumHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentSection = sectionId;
    assembler.currentKernel = sections[sectionId].kernelId;
    inside = Inside::MAINLAYOUT;
}

AsmFormatHandler::SectionInfo AsmGalliumHandler::getSectionInfo(cxuint sectionId) const
{
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    
    AsmFormatHandler::SectionInfo info;
    info.type = sections[sectionId].type;
    info.flags = 0;
    if (info.type == AsmSectionType::CODE)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
    else if (info.type != AsmSectionType::CONFIG)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    
    info.name = sections[sectionId].name;
    return info;
}

// check whether label is kernel label and restore register allocation to this kernel
void AsmGalliumHandler::handleLabel(const CString& label)
{
    if (assembler.sections[assembler.currentSection].type != AsmSectionType::CODE)
        return;
    auto kit = assembler.kernelMap.find(label);
    if (kit == assembler.kernelMap.end())
        return;
    if (!kcodeSelection.empty())
        return; // do not change if inside kcode
    // add code start
    assembler.sections[assembler.currentSection].addCodeFlowEntry({
                    size_t(assembler.currentOutPos), 0, AsmCodeFlowType::START });
    // save other state
    saveKcodeCurrentAllocRegs();
    if (currentKcodeKernel != ASMKERN_GLOBAL)
        assembler.kernels[currentKcodeKernel].closeCodeRegion(
                        assembler.sections[codeSection].content.size());
    // restore this state
    currentKcodeKernel = kit->second;
    restoreKcodeCurrentAllocRegs();
    if (currentKcodeKernel != ASMKERN_GLOBAL)
        assembler.kernels[currentKcodeKernel].openCodeRegion(
                        assembler.sections[codeSection].content.size());
}

void AsmGalliumHandler::restoreKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {
        Kernel& newKernel = *kernelStates[currentKcodeKernel];
        assembler.isaAssembler->setAllocatedRegisters(newKernel.allocRegs,
                            newKernel.allocRegFlags);
    }
}

void AsmGalliumHandler::saveKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {   // save other state
        size_t regTypesNum;
        Kernel& oldKernel = *kernelStates[currentKcodeKernel];
        const cxuint* regs = assembler.isaAssembler->getAllocatedRegisters(
                            regTypesNum, oldKernel.allocRegFlags);
        std::copy(regs, regs+regTypesNum, oldKernel.allocRegs);
    }
}

namespace CLRX
{

bool AsmGalliumPseudoOps::checkPseudoOpName(const CString& string)
{
    if (string.empty() || string[0] != '.')
        return false;
    const size_t pseudoOp = binaryFind(galliumPseudoOpNamesTbl, galliumPseudoOpNamesTbl +
                sizeof(galliumPseudoOpNamesTbl)/sizeof(char*), string.c_str()+1,
               CStringLess()) - galliumPseudoOpNamesTbl;
    return pseudoOp < sizeof(galliumPseudoOpNamesTbl)/sizeof(char*);
}

void AsmGalliumPseudoOps::setDriverVersion(AsmGalliumHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    asmr.driverVersion = value;
}

void AsmGalliumPseudoOps::setLLVMVersion(AsmGalliumHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    asmr.llvmVersion = value;
}

void AsmGalliumPseudoOps::getXXXVersion(AsmGalliumHandler& handler, const char* linePtr,
            bool getLLVMVersion)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    const char* symNamePlace = linePtr;
    const CString symName = extractScopedSymName(linePtr, end, false);
    if (symName.empty())
    {
        asmr.printError(symNamePlace, "Illegal symbol name");
        return;
    }
    size_t symNameLength = symName.size();
    if (symNameLength >= 3 && symName.compare(symNameLength-3, 3, "::.")==0)
    {
        asmr.printError(symNamePlace, "Symbol '.' can be only in global scope");
        return;
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint driverVersion = 0;
    if (getLLVMVersion)
        driverVersion = handler.determineLLVMVersion();
    else
        driverVersion = handler.determineDriverVersion();
    
    std::pair<AsmSymbolEntry*, bool> res = asmr.insertSymbolInScope(symName,
                AsmSymbol(ASMSECT_ABS, driverVersion));
    if (!res.second)
    {   // found
        if (res.first->second.onceDefined && res.first->second.isDefined()) // if label
            asmr.printError(symNamePlace, (std::string("Symbol '")+symName.c_str()+
                        "' is already defined").c_str());
        else
            asmr.setSymbol(*res.first, driverVersion, ASMSECT_ABS);
    }
}

void AsmGalliumPseudoOps::doConfig(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Configuration outside kernel definition");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel]->hasProgInfo)
    {
        asmr.printError(pseudoOpPlace,
                "Configuration can't be defined if progInfo was defined");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.inside = AsmGalliumHandler::Inside::CONFIG;
    handler.output.kernels[asmr.currentKernel].useConfig = true;
    if (handler.determineLLVMVersion() >= 40000U) // HSA since LLVM 4.0
        handler.kernelStates[asmr.currentKernel]->initializeAmdHsaKernelConfig();
}

void AsmGalliumPseudoOps::doGlobalData(AsmGalliumHandler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    asmr.goToSection(pseudoOpPlace, ".rodata");
}

void AsmGalliumPseudoOps::setDimensions(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
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

void AsmGalliumPseudoOps::setConfigValue(AsmGalliumHandler& handler,
         const char* pseudoOpPlace, const char* linePtr, GalliumConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    if (target >= GALLIUMCVAL_HSA_FIRST_PARAM && handler.determineLLVMVersion() < 40000U)
    {
        asmr.printError(pseudoOpPlace, "HSA configuration pseudo-op only for LLVM<=4.0.0");
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
            case GALLIUMCVAL_SGPRSNUM:
            case GALLIUMCVAL_HSA_SGPRSNUM:
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
            case GALLIUMCVAL_VGPRSNUM:
            case GALLIUMCVAL_HSA_VGPRSNUM:
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
            case GALLIUMCVAL_SPILLEDSGPRS:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
                if (value > maxSGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Spilled SGPRs number out of range (0-%u)",
                             maxSGPRsNum);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case GALLIUMCVAL_SPILLEDVGPRS:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxVGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_VGPR, 0);
                if (value > maxVGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Spilled VGPRs number out of range (0-%u)",
                             maxVGPRsNum);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case GALLIUMCVAL_EXCEPTIONS:
            case GALLIUMCVAL_HSA_EXCEPTIONS:
                asmr.printWarningForRange(7, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0x7f;
                break;
            case GALLIUMCVAL_FLOATMODE:
            case GALLIUMCVAL_HSA_FLOATMODE:
                asmr.printWarningForRange(8, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0xff;
                break;
            case GALLIUMCVAL_PRIORITY:
            case GALLIUMCVAL_HSA_PRIORITY:
                asmr.printWarningForRange(2, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 3;
                break;
            case GALLIUMCVAL_LOCALSIZE:
            case GALLIUMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE:
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
            case GALLIUMCVAL_USERDATANUM:
            case GALLIUMCVAL_HSA_USERDATANUM:
                if (value > 16)
                {
                    asmr.printError(valuePlace, "UserDataNum out of range (0-16)");
                    good = false;
                }
                break;
            case GALLIUMCVAL_PGMRSRC1:
            case GALLIUMCVAL_PGMRSRC2:
            case GALLIUMCVAL_HSA_PGMRSRC1:
            case GALLIUMCVAL_HSA_PGMRSRC2:
                asmr.printWarningForRange(32, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                break;
            
            case GALLIUMCVAL_PRIVATE_ELEM_SIZE:
                if (value==0 || 1ULL<<(63-CLZ64(value)) != value)
                {
                    asmr.printError(valuePlace,
                                    "Private element size must be power of two");
                    good = false;
                }
                else if (value < 2 || value > 16)
                {
                    asmr.printError(valuePlace, "Private element size out of range");
                    good = false;
                }
                break;
            case GALLIUMCVAL_KERNARG_SEGMENT_ALIGN:
            case GALLIUMCVAL_GROUP_SEGMENT_ALIGN:
            case GALLIUMCVAL_PRIVATE_SEGMENT_ALIGN:
                if (value==0 || 1ULL<<(63-CLZ64(value)) != value)
                {
                    asmr.printError(valuePlace, "Alignment must be power of two");
                    good = false;
                }
                else if (value < 16)
                {
                    asmr.printError(valuePlace, "Alignment must be not smaller than 16");
                    good = false;
                }
                break;
            case GALLIUMCVAL_WAVEFRONT_SIZE:
                if (value==0 || 1ULL<<(63-CLZ64(value)) != value)
                {
                    asmr.printError(valuePlace, "Wavefront size must be power of two");
                    good = false;
                }
                else if (value > 256)
                {
                    asmr.printError(valuePlace,
                                "Wavefront size must be not greater than 256");
                    good = false;
                }
                break;
            case GALLIUMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE:
            case GALLIUMCVAL_GDS_SEGMENT_SIZE:
            case GALLIUMCVAL_WORKGROUP_FBARRIER_COUNT:
            case GALLIUMCVAL_CALL_CONVENTION:
                asmr.printWarningForRange(32, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                break;
            case GALLIUMCVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR:
            case GALLIUMCVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
                if (value >= maxSGPRsNum)
                {
                    asmr.printError(valuePlace, "SGPR register out of range");
                    good = false;
                }
                break;
            }
            default:
                break;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    GalliumKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    // set value
    switch(target)
    {
        case GALLIUMCVAL_SGPRSNUM:
            config.usedSGPRsNum = value;
            break;
        case GALLIUMCVAL_VGPRSNUM:
            config.usedVGPRsNum = value;
            break;
        case GALLIUMCVAL_SPILLEDSGPRS:
            config.spilledSGPRs = value;
            break;
        case GALLIUMCVAL_SPILLEDVGPRS:
            config.spilledVGPRs = value;
            break;
        case GALLIUMCVAL_PGMRSRC1:
            config.pgmRSRC1 = value;
            break;
        case GALLIUMCVAL_PGMRSRC2:
            config.pgmRSRC2 = value;
            break;
        case GALLIUMCVAL_FLOATMODE:
            config.floatMode = value;
            break;
        case GALLIUMCVAL_LOCALSIZE:
            config.localSize = value;
            break;
        case GALLIUMCVAL_SCRATCHBUFFER:
            config.scratchBufferSize = value;
            break;
        case GALLIUMCVAL_PRIORITY:
            config.priority = value;
            break;
        case GALLIUMCVAL_USERDATANUM:
            config.userDataNum = value;
            break;
        case GALLIUMCVAL_EXCEPTIONS:
            config.exceptions = value;
            break;
        default:
            break;
    }
}

void AsmGalliumPseudoOps::setConfigBoolValue(AsmGalliumHandler& handler,
         const char* pseudoOpPlace, const char* linePtr, GalliumConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    GalliumKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    switch(target)
    {
        case GALLIUMCVAL_DEBUGMODE:
            config.debugMode = true;
            break;
        case GALLIUMCVAL_DX10CLAMP:
            config.dx10Clamp = true;
            break;
        case GALLIUMCVAL_IEEEMODE:
            config.ieeeMode = true;
            break;
        case GALLIUMCVAL_PRIVMODE:
            config.privilegedMode = true;
            break;
        case GALLIUMCVAL_TGSIZE:
            config.tgSize = true;
            break;
        default:
            break;
    }
}

void AsmGalliumPseudoOps::doArgs(AsmGalliumHandler& handler,
               const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Arguments outside kernel definition");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.inside = AsmGalliumHandler::Inside::ARGS;
}

static const std::pair<const char*, GalliumArgType> galliumArgTypesMap[9] =
{
    { "constant", GalliumArgType::CONSTANT },
    { "global", GalliumArgType::GLOBAL },
    { "image2d_rd", GalliumArgType::IMAGE2D_RDONLY },
    { "image2d_wr", GalliumArgType::IMAGE2D_WRONLY },
    { "image3d_rd", GalliumArgType::IMAGE3D_RDONLY },
    { "image3d_wr", GalliumArgType::IMAGE3D_WRONLY },
    { "local", GalliumArgType::LOCAL },
    { "sampler", GalliumArgType::SAMPLER },
    { "scalar", GalliumArgType::SCALAR }
};

static const std::pair<const char*, cxuint> galliumArgSemanticsMap[5] =
{
    { "general", cxuint(GalliumArgSemantic::GENERAL) },
    { "griddim", cxuint(GalliumArgSemantic::GRID_DIMENSION) },
    { "gridoffset", cxuint(GalliumArgSemantic::GRID_OFFSET) },
    { "imgformat", cxuint(GalliumArgSemantic::IMAGE_FORMAT) },
    { "imgsize", cxuint(GalliumArgSemantic::IMAGE_SIZE) },
};

void AsmGalliumPseudoOps::doArg(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Argument definition outside kernel configuration");
        return;
    }
    if (handler.inside != AsmGalliumHandler::Inside::ARGS)
    {
        asmr.printError(pseudoOpPlace, "Argument definition outside arguments list");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    char name[20];
    bool good = true;
    const char* nameStringPlace = linePtr;
    GalliumArgType argType = GalliumArgType::GLOBAL;
    
    GalliumArgSemantic argSemantic = GalliumArgSemantic::GENERAL;
    bool semanticDefined = false;
    if (getNameArg(asmr, 20, name, linePtr, "argument type"))
    {
        toLowerString(name);
        if (::strcmp(name, "griddim")==0)
        {   // shortcur for grid dimension
            argSemantic = GalliumArgSemantic::GRID_DIMENSION;
            argType = GalliumArgType::SCALAR;
            semanticDefined = true;
        }
        else if (::strcmp(name, "gridoffset")==0)
        {   // shortcut for grid dimension
            argSemantic = GalliumArgSemantic::GRID_OFFSET;
            argType = GalliumArgType::SCALAR;
            semanticDefined = true;
        }
        else
        {
            cxuint index = binaryMapFind(galliumArgTypesMap, galliumArgTypesMap + 9,
                         name, CStringLess()) - galliumArgTypesMap;
            if (index != 9) // end of this map
                argType = galliumArgTypesMap[index].second;
            else
            {
                asmr.printError(nameStringPlace, "Unknown argument type");
                good = false;
            }
        }
    }
    else
        good = false;
    
    // parse rest of arguments
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    const char* sizeStrPlace = linePtr;
    uint64_t size = 4;
    good &= getAbsoluteValueArg(asmr, size, linePtr, true);
    
    if (size > UINT32_MAX || size == 0)
        asmr.printWarning(sizeStrPlace, "Size of argument out of range");
    
    uint64_t targetSize = (size+3ULL)&~3ULL;
    uint64_t targetAlign = (size!=0) ? 1ULL<<(63-CLZ64(targetSize)) : 1;
    if (targetSize > targetAlign)
        targetAlign <<= 1;
    bool sext = false;
    
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        const char* targetSizePlace = linePtr;
        if (getAbsoluteValueArg(asmr, targetSize, linePtr, false))
        {
            if (targetSize > UINT32_MAX || targetSize == 0)
                asmr.printWarning(targetSizePlace, "Target size of argument out of range");
        }
        else
            good = false;
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            const char* targetAlignPlace = linePtr;
            if (getAbsoluteValueArg(asmr, targetAlign, linePtr, false))
            {
                if (targetAlign > UINT32_MAX || targetAlign == 0)
                    asmr.printWarning(targetAlignPlace,
                                      "Target alignment of argument out of range");
                if (targetAlign==0 || targetAlign != (1ULL<<(63-CLZ64(targetAlign))))
                {
                    asmr.printError(targetAlignPlace, "Target alignment is not power of 2");
                    good = false;
                }
            }
            else
                good = false;
            
            if (!skipComma(asmr, haveComma, linePtr))
                return;
            if (haveComma)
            {
                skipSpacesToEnd(linePtr, end);
                const char* numExtPlace = linePtr;
                if (getNameArg(asmr, 5, name, linePtr, "numeric extension", false))
                {
                    toLowerString(name);
                    if (::strcmp(name, "sext")==0)
                        sext = true;
                    else if (::strcmp(name, "zext")!=0 && *name!=0)
                    {
                        asmr.printError(numExtPlace, "Unknown numeric extension");
                        good = false;
                    }
                }
                else
                    good = false;
                
                if (!semanticDefined)
                {   /// if semantic has not been defined in the first argument
                    if (!skipComma(asmr, haveComma, linePtr))
                        return;
                    if (haveComma)
                    {
                        cxuint semantic;
                        if (getEnumeration(asmr, linePtr, "argument semantic", 5,
                                    galliumArgSemanticsMap, semantic))
                            argSemantic = GalliumArgSemantic(semantic);
                        else
                            good = false;
                    }
                }
            }
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // put this definition to argument list
    handler.output.kernels[asmr.currentKernel].argInfos.push_back(
        { argType, sext, argSemantic, uint32_t(size),
            uint32_t(targetSize), uint32_t(targetAlign) });
}

void AsmGalliumPseudoOps::doProgInfo(AsmGalliumHandler& handler,
                 const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo outside kernel definition");
        return;
    }
    if (handler.output.kernels[asmr.currentKernel].useConfig)
    {
        asmr.printError(pseudoOpPlace,
                "ProgInfo can't be defined if configuration was exists");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.inside = AsmGalliumHandler::Inside::PROGINFO;
    handler.kernelStates[asmr.currentKernel]->hasProgInfo = true;
}

void AsmGalliumPseudoOps::doEntry(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo entry outside kernel configuration");
        return;
    }
    if (handler.inside != AsmGalliumHandler::Inside::PROGINFO)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo entry definition outside ProgInfo");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* addrPlace = linePtr;
    uint64_t entryAddr;
    bool good = true;
    if (getAbsoluteValueArg(asmr, entryAddr, linePtr, true))
        asmr.printWarningForRange(32, entryAddr, asmr.getSourcePos(addrPlace),
                              WS_UNSIGNED);
    else
        good = false;
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    skipSpacesToEnd(linePtr, end);
    const char* entryValPlace = linePtr;
    uint64_t entryVal;
    if (getAbsoluteValueArg(asmr, entryVal, linePtr, true))
        asmr.printWarningForRange(32, entryVal, asmr.getSourcePos(entryValPlace));
    else
        good = false;
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // do operation
    AsmGalliumHandler::Kernel& kstate = *handler.kernelStates[asmr.currentKernel];
    kstate.hasProgInfo = true;
    const cxuint llvmVersion = handler.determineLLVMVersion();
    if (llvmVersion<30900U && kstate.progInfoEntries == 3)
    {
        asmr.printError(pseudoOpPlace, "Maximum 3 entries can be in ProgInfo");
        return;
    }
    if (llvmVersion>=30900U && kstate.progInfoEntries == 5)
    {
        asmr.printError(pseudoOpPlace, "Maximum 5 entries can be in ProgInfo");
        return;
    }
    GalliumProgInfoEntry& pentry = handler.output.kernels[asmr.currentKernel]
            .progInfo[kstate.progInfoEntries++];
    pentry.address = entryAddr;
    pentry.value = entryVal;
}

/* update kernel code selection, join all regallocation and store to
 * current kernel regalloc */
void AsmGalliumPseudoOps::updateKCodeSel(AsmGalliumHandler& handler,
          const std::vector<cxuint>& oldset)
{
    Assembler& asmr = handler.assembler;
    // old elements - join current regstate with all them
    size_t regTypesNum;
    for (auto it = oldset.begin(); it != oldset.end(); ++it)
    {
        Flags curAllocRegFlags;
        const cxuint* curAllocRegs = asmr.isaAssembler->getAllocatedRegisters(regTypesNum,
                               curAllocRegFlags);
        cxuint newAllocRegs[MAX_REGTYPES_NUM];
        AsmGalliumHandler::Kernel& kernel = *handler.kernelStates[*it];
        for (size_t i = 0; i < regTypesNum; i++)
            newAllocRegs[i] = std::max(curAllocRegs[i], kernel.allocRegs[i]);
        kernel.allocRegFlags |= curAllocRegFlags;
        std::copy(newAllocRegs, newAllocRegs+regTypesNum, kernel.allocRegs);
    }
    asmr.isaAssembler->setAllocatedRegisters();
}

void AsmGalliumPseudoOps::doKCode(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    bool good = true;
    skipSpacesToEnd(linePtr, end);
    if (linePtr==end)
        return;
    std::unordered_set<cxuint> newSel(handler.kcodeSelection.begin(),
                          handler.kcodeSelection.end());
    do {
        CString kname;
        const char* knamePlace = linePtr;
        skipSpacesToEnd(linePtr, end);
        bool removeKernel = false;
        if (linePtr!=end && *linePtr=='-')
        {   // '-' - remove this kernel from current kernel selection
            removeKernel = true;
            linePtr++;
        }
        else if (linePtr!=end && *linePtr=='+')
        {
            linePtr++;
            skipSpacesToEnd(linePtr, end);
            if (linePtr==end)
            {   // add all kernels
                for (cxuint k = 0; k < handler.kernelStates.size(); k++)
                    newSel.insert(k);
                break;
            }
        }
        
        if (!getNameArg(asmr, kname, linePtr, "kernel"))
        { good = false; continue; }
        auto kit = asmr.kernelMap.find(kname);
        if (kit == asmr.kernelMap.end())
        {
            asmr.printError(knamePlace, "Kernel not found");
            continue;
        }
        if (!removeKernel)
            newSel.insert(kit->second);
        else // remove kernel
            newSel.erase(kit->second);
    } while (skipCommaForMultipleArgs(asmr, linePtr));
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CODE)
    {
        asmr.printError(pseudoOpPlace, "KCode outside code");
        return;
    }
    if (handler.kcodeSelStack.empty())
        handler.saveKcodeCurrentAllocRegs();
    // push to stack
    handler.kcodeSelStack.push(handler.kcodeSelection);
    // set current sel
    handler.kcodeSelection.assign(newSel.begin(), newSel.end());
    std::sort(handler.kcodeSelection.begin(), handler.kcodeSelection.end());
    
    const std::vector<cxuint>& oldKCodeSel = handler.kcodeSelStack.top();
    if (!oldKCodeSel.empty())
        asmr.handleRegionsOnKernels(handler.kcodeSelection, oldKCodeSel,
                            handler.codeSection);
    else if (handler.currentKcodeKernel != ASMKERN_GLOBAL)
    {
        std::vector<cxuint> tempKCodeSel;
        tempKCodeSel.push_back(handler.currentKcodeKernel);
        asmr.handleRegionsOnKernels(handler.kcodeSelection, tempKCodeSel,
                            handler.codeSection);
    }
    
    updateKCodeSel(handler, handler.kcodeSelStack.top());
}

void AsmGalliumPseudoOps::doKCodeEnd(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CODE)
    {
        asmr.printError(pseudoOpPlace, "KCodeEnd outside code");
        return;
    }
    if (handler.kcodeSelStack.empty())
    {
        asmr.printError(pseudoOpPlace, "'.kcodeend' without '.kcode'");
        return;
    }
    updateKCodeSel(handler, handler.kcodeSelection);
    std::vector<cxuint> oldKCodeSel = handler.kcodeSelection;
    handler.kcodeSelection = handler.kcodeSelStack.top();
    
    if (!handler.kcodeSelection.empty())
        asmr.handleRegionsOnKernels(handler.kcodeSelection, oldKCodeSel,
                        handler.codeSection);
    else if (handler.currentKcodeKernel != ASMKERN_GLOBAL)
    {   // if choosen current kernel
        std::vector<cxuint> curKernelSel;
        curKernelSel.push_back(handler.currentKcodeKernel);
        asmr.handleRegionsOnKernels(curKernelSel, oldKCodeSel, handler.codeSection);
    }
    
    handler.kcodeSelStack.pop();
    if (handler.kcodeSelStack.empty())
        handler.restoreKcodeCurrentAllocRegs();
}

}

bool AsmGalliumHandler::parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr)
{
    const size_t pseudoOp = binaryFind(galliumPseudoOpNamesTbl, galliumPseudoOpNamesTbl +
                    sizeof(galliumPseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - galliumPseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case GALLIUMOP_ARG:
            AsmGalliumPseudoOps::doArg(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_ARGS:
            AsmGalliumPseudoOps::doArgs(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_CONFIG:
            AsmGalliumPseudoOps::doConfig(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_DEBUGMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_DEBUGMODE);
            break;
        case GALLIUMOP_DIMS:
            AsmGalliumPseudoOps::setDimensions(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_DRIVER_VERSION:
            AsmGalliumPseudoOps::setDriverVersion(*this, linePtr);
            break;
        case GALLIUMOP_DX10CLAMP:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_DX10CLAMP);
            break;
        case GALLIUMOP_ENTRY:
            AsmGalliumPseudoOps::doEntry(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_EXCEPTIONS:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_EXCEPTIONS);
            break;
        case GALLIUMOP_FLOATMODE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_FLOATMODE);
            break;
        case GALLIUMOP_GET_DRIVER_VERSION:
            AsmGalliumPseudoOps::getXXXVersion(*this, linePtr, false);
            break;
        case GALLIUMOP_GET_LLVM_VERSION:
            AsmGalliumPseudoOps::getXXXVersion(*this, linePtr, true);
            break;
        case GALLIUMOP_GLOBALDATA:
            AsmGalliumPseudoOps::doGlobalData(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_IEEEMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_IEEEMODE);
            break;
        case GALLIUMOP_KCODE:
            AsmGalliumPseudoOps::doKCode(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_KCODEEND:
            AsmGalliumPseudoOps::doKCodeEnd(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_LLVM_VERSION:
            AsmGalliumPseudoOps::setLLVMVersion(*this, linePtr);
            break;
        case GALLIUMOP_LOCALSIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_LOCALSIZE);
            break;
        case GALLIUMOP_PRIORITY:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_PRIORITY);
            break;
        case GALLIUMOP_PRIVMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_PRIVMODE);
            break;
        case GALLIUMOP_PGMRSRC1:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_PGMRSRC1);
            break;
        case GALLIUMOP_PGMRSRC2:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_PGMRSRC2);
            break;
        case GALLIUMOP_PROGINFO:
            AsmGalliumPseudoOps::doProgInfo(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_SCRATCHBUFFER:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_SCRATCHBUFFER);
            break;
        case GALLIUMOP_SGPRSNUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_SGPRSNUM);
            break;
        case GALLIUMOP_SPILLEDSGPRS:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_SPILLEDSGPRS);
            break;
        case GALLIUMOP_SPILLEDVGPRS:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_SPILLEDVGPRS);
            break;
        case GALLIUMOP_TGSIZE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_TGSIZE);
            break;
        case GALLIUMOP_USERDATANUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_USERDATANUM);
            break;
        case GALLIUMOP_VGPRSNUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_VGPRSNUM);
            break;
        default:
            return false;
    }
    return true;
}

static const AMDGPUArchValues galliumAmdGpuArchValuesTbl[] =
{
    { 0, 0, 0 }, // GPUDeviceType::CAPE_VERDE
    { 0, 0, 0 }, // GPUDeviceType::PITCAIRN
    { 0, 0, 0 }, // GPUDeviceType::TAHITI
    { 0, 0, 0 }, // GPUDeviceType::OLAND
    { 7, 0, 0 }, // GPUDeviceType::BONAIRE
    { 7, 0, 0 }, // GPUDeviceType::SPECTRE
    { 7, 0, 0 }, // GPUDeviceType::SPOOKY
    { 7, 0, 0 }, // GPUDeviceType::KALINDI
    { 0, 0, 0 }, // GPUDeviceType::HAINAN
    { 7, 0, 1 }, // GPUDeviceType::HAWAII
    { 8, 0, 0 }, // GPUDeviceType::ICELAND
    { 8, 0, 0 }, // GPUDeviceType::TONGA
    { 7, 0, 0 }, // GPUDeviceType::MULLINS
    { 8, 0, 3 }, // GPUDeviceType::FIJI
    { 8, 0, 1 }, // GPUDeviceType::CARRIZO
    { 0, 0, 0 }, // GPUDeviceType::DUMMY
    { 0, 0, 0 }, // GPUDeviceType::GOOSE
    { 0, 0, 0 }, // GPUDeviceType::HORSE
    { 8, 0, 1 }, // GPUDeviceType::STONEY
    { 8, 0, 4 }, // GPUDeviceType::ELLESMERE
    { 8, 0, 4 }, // GPUDeviceType::BAFFIN
    { 8, 0, 4 }, // GPUDeviceType::GFX804
    { 9, 0, 0 } // GPUDeviceType::GFX900
};

bool AsmGalliumHandler::prepareBinary()
{   // before call we initialize pointers and datas
    bool good = true;
    
    output.is64BitElf = assembler.is64Bit();
    size_t sectionsNum = sections.size();
    size_t kernelsNum = kernelStates.size();
    output.deviceType = assembler.getDeviceType();
    if (assembler.isaAssembler!=nullptr)
    {   // make last kernel registers pool updates
        if (kcodeSelStack.empty())
            saveKcodeCurrentAllocRegs();
        else
            while (!kcodeSelStack.empty())
            {   // pop from kcode stack and apply changes
                AsmGalliumPseudoOps::updateKCodeSel(*this, kcodeSelection);
                kcodeSelection = kcodeSelStack.top();
                kcodeSelStack.pop();
            }
    }
    
    for (size_t i = 0; i < sectionsNum; i++)
    {
        const AsmSection& asmSection = assembler.sections[i];
        const Section& section = sections[i];
        const size_t sectionSize = asmSection.getSize();
        const cxbyte* sectionData = (!asmSection.content.empty()) ?
                asmSection.content.data() : (const cxbyte*)"";
        switch(asmSection.type)
        {
            case AsmSectionType::CODE:
                output.codeSize = sectionSize;
                output.code = sectionData;
                break;
            case AsmSectionType::DATA:
                output.globalDataSize = sectionSize;
                output.globalData = sectionData;
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
                output.extraSections.push_back({section.name, sectionSize, sectionData,
                    asmSection.alignment!=0?asmSection.alignment:1, elfSectType,
                    elfSectFlags, ELFSECTID_NULL, 0, 0 });
                break;
            }
            case AsmSectionType::GALLIUM_COMMENT:
                output.commentSize = sectionSize;
                output.comment = (const char*)sectionData;
                break;
            default: // ignore other sections
                break;
        }
    }
    
    GPUArchitecture arch = getGPUArchitectureFromDeviceType(assembler.deviceType);
    // set up number of the allocated SGPRs and VGPRs for kernel
    cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    for (size_t i = 0; i < kernelsNum; i++)
    {
        if (!output.kernels[i].useConfig)
            continue;
        GalliumKernelConfig& config = output.kernels[i].config;
        cxuint userSGPRsNum = config.userDataNum;
        /* include userData sgprs */
        cxuint dimMask = (config.dimMask!=BINGEN_DEFAULT) ? config.dimMask :
                ((config.pgmRSRC2>>7)&7);
        // extra sgprs for dimensions
        cxuint minRegsNum[2];
        getGPUSetupMinRegistersNum(arch, dimMask, userSGPRsNum,
                   ((config.tgSize) ? GPUSETUP_TGSIZE_EN : 0) |
                   ((config.scratchBufferSize!=0) ? GPUSETUP_SCRATCH_EN : 0), minRegsNum);
        
        if (config.usedSGPRsNum!=BINGEN_DEFAULT && maxSGPRsNum < config.usedSGPRsNum)
        {   // check only if sgprsnum set explicitly
            char numBuf[64];
            snprintf(numBuf, 64, "(max %u)", maxSGPRsNum);
            assembler.printError(assembler.kernels[i].sourcePos, (std::string(
                    "Number of total SGPRs for kernel '")+
                    output.kernels[i].kernelName.c_str()+"' is too high "+numBuf).c_str());
            good = false;
        }
        
        if (config.usedSGPRsNum==BINGEN_DEFAULT)
        {
            config.usedSGPRsNum = std::min(
                std::max(minRegsNum[0], kernelStates[i]->allocRegs[0]) +
                    getGPUExtraRegsNum(arch, REGTYPE_SGPR,
                        kernelStates[i]->allocRegFlags|GCN_VCC),
                    maxSGPRsNum); // include all extra sgprs
        }
        if (config.usedVGPRsNum==BINGEN_DEFAULT)
            config.usedVGPRsNum = std::max(minRegsNum[1], kernelStates[i]->allocRegs[1]);
    }
    
    // if set adds symbols to binary
    if (assembler.getFlags() & ASM_FORCE_ADD_SYMBOLS)
        for (const AsmSymbolEntry& symEntry: assembler.globalScope.symbolMap)
        {
            if (!symEntry.second.hasValue)
                continue; // unresolved
            if (ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // local
            if (ELF32_ST_BIND(symEntry.second.info) == STB_GLOBAL)
            {
                assembler.printError(AsmSourcePos(), (std::string("Added symbol '")+
                    symEntry.first.c_str()+"' must not be a global").c_str());
                good = false;
                continue; // local
            }
            if (assembler.kernelMap.find(symEntry.first.c_str())!=assembler.kernelMap.end())
                continue; // if kernel name
            cxuint binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            
            output.extraSymbols.push_back({ symEntry.first, symEntry.second.value,
                    symEntry.second.size, binSectId, false, symEntry.second.info,
                    symEntry.second.other });
        }
    
    /// checking symbols and set offset for kernels
    AMDGPUArchValues amdGpuArchValues = galliumAmdGpuArchValuesTbl[
                    cxuint(assembler.deviceType)];
    AsmSection& asmCSection = assembler.sections[codeSection];
    const AsmSymbolMap& symbolMap = assembler.getSymbolMap();
    
    cxuint llvmVersion = assembler.llvmVersion;
    if (llvmVersion == 0 && (assembler.flags&ASM_TESTRUN)==0)
        llvmVersion = defaultLLVMVersion;
    
    const cxuint ldsShift = arch<GPUArchitecture::GCN1_1 ? 8 : 9;
    const uint32_t ldsMask = (1U<<ldsShift)-1U;
    
    for (size_t ki = 0; ki < output.kernels.size(); ki++)
    {
        GalliumKernelInput& kinput = output.kernels[ki];
        auto it = symbolMap.find(kinput.kernelName);
        if (it == symbolMap.end() || !it->second.isDefined())
        {   // error, undefined
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                        "Symbol for kernel '")+kinput.kernelName.c_str()+
                        "' is undefined").c_str());
            good = false;
            continue;
        }
        const AsmSymbol& symbol = it->second;
        if (!symbol.hasValue)
        {   // error, unresolved
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '") + kinput.kernelName.c_str() +
                    "' is not resolved").c_str());
            good = false;
            continue;
        }
        if (symbol.sectionId != codeSection)
        {   /// error, wrong section
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '")+kinput.kernelName.c_str()+
                    "' is defined for section other than '.text'").c_str());
            good = false;
            continue;
        }
        kinput.offset = symbol.value;
        if (llvmVersion >= 40000U)
        {   // requires amdhsa-gcn (with HSA header)
            // hotfix
            GalliumKernelConfig& config = output.kernels[ki].config;
            AmdHsaKernelConfig outConfig;
            
            uint32_t dimValues = 0;
            if (config.dimMask != BINGEN_DEFAULT)
                dimValues = ((config.dimMask&7)<<7) |
                        (((config.dimMask&4) ? 2 : (config.dimMask&2) ? 1 : 0)<<11);
            else
                dimValues |= (config.pgmRSRC2 & 0x1b80U);
            cxuint sgprsNum = std::max(config.usedSGPRsNum, 1U);
            cxuint vgprsNum = std::max(config.usedVGPRsNum, 1U);
            uint32_t pgmRSRC1 =  (config.pgmRSRC1) | ((vgprsNum-1)>>2) |
                (((sgprsNum-1)>>3)<<6) | ((uint32_t(config.floatMode)&0xff)<<12) |
                (config.ieeeMode?1U<<23:0) | (uint32_t(config.priority&3)<<10) |
                (config.privilegedMode?1U<<20:0) | (config.dx10Clamp?1U<<21:0) |
                (config.debugMode?1U<<22:0);
            
            uint32_t pgmRSRC2 = (config.pgmRSRC2 & 0xffffe440U) |
                        (config.userDataNum<<1) | ((config.tgSize) ? 0x400 : 0) |
                        ((config.scratchBufferSize)?1:0) | dimValues |
                        (((config.localSize+ldsMask)>>ldsShift)<<15) |
                        ((uint32_t(config.exceptions)&0x7f)<<24);
            
            size_t argSegmentSize = 0;
            for (GalliumArgInfo& argInfo: output.kernels[ki].argInfos)
            {
                if (argInfo.semantic == GalliumArgSemantic::GRID_DIMENSION ||
                        argInfo.semantic == GalliumArgSemantic::GRID_OFFSET)
                    continue; // skip
                if (argInfo.targetAlign != 0)
                    argSegmentSize = (argSegmentSize + argInfo.targetAlign-1) &
                            ~size_t(argInfo.targetAlign-1);
                argSegmentSize += argInfo.targetSize;
            }
            argSegmentSize += 16; // gridOffset and gridDim
            SULEV(outConfig.amdCodeVersionMajor, 1);
            SULEV(outConfig.amdCodeVersionMinor, 0);
            SULEV(outConfig.amdMachineKind, 1);
            SULEV(outConfig.amdMachineMajor, amdGpuArchValues.major);
            SULEV(outConfig.amdMachineMinor, amdGpuArchValues.minor);
            SULEV(outConfig.amdMachineStepping, amdGpuArchValues.stepping);
            SULEV(outConfig.kernelCodeEntryOffset, 256);
            SULEV(outConfig.kernelCodePrefetchOffset, 0);
            SULEV(outConfig.kernelCodePrefetchSize, 0);
            SULEV(outConfig.maxScrachBackingMemorySize, 0);
            SULEV(outConfig.computePgmRsrc1, pgmRSRC1);
            SULEV(outConfig.computePgmRsrc2, pgmRSRC2);
            SULEV(outConfig.enableSgprRegisterFlags, 
                    uint16_t(AMDHSAFLAG_USE_PRIVATE_SEGMENT_BUFFER|
                        AMDHSAFLAG_USE_DISPATCH_PTR|AMDHSAFLAG_USE_KERNARG_SEGMENT_PTR));
            SULEV(outConfig.enableFeatureFlags, uint16_t(AMDHSAFLAG_USE_PTR64|2));
            SULEV(outConfig.workitemPrivateSegmentSize, config.scratchBufferSize);
            SULEV(outConfig.workgroupGroupSegmentSize, config.localSize);
            SULEV(outConfig.gdsSegmentSize, 0);
            SULEV(outConfig.kernargSegmentSize, argSegmentSize);
            SULEV(outConfig.workgroupFbarrierCount, 0);
            SULEV(outConfig.wavefrontSgprCount, config.usedSGPRsNum);
            SULEV(outConfig.workitemVgprCount, config.usedVGPRsNum);
            SULEV(outConfig.reservedVgprFirst, 0);
            SULEV(outConfig.reservedVgprCount, 0);
            SULEV(outConfig.reservedSgprFirst, 0);
            SULEV(outConfig.reservedSgprCount, 0);
            SULEV(outConfig.debugWavefrontPrivateSegmentOffsetSgpr, 0);
            SULEV(outConfig.debugPrivateSegmentBufferSgpr, 0);
            outConfig.kernargSegmentAlignment = 4;
            outConfig.groupSegmentAlignment = 4;
            outConfig.privateSegmentAlignment = 4;
            outConfig.wavefrontSize = 6;
            SULEV(outConfig.callConvention, 0);
            SULEV(outConfig.reserved1[0], 0);
            SULEV(outConfig.reserved1[1], 0);
            SULEV(outConfig.reserved1[2], 0);
            SULEV(outConfig.runtimeLoaderKernelSymbol, 0);
            
            ::memset(outConfig.controlDirective, 0, 128);
            ::memcpy(asmCSection.content.data() + symbol.value,
                     &outConfig, sizeof(AmdHsaKernelConfig));
        }
    }
    // set versions
    if (assembler.driverVersion == 0 && (assembler.flags&ASM_TESTRUN)==0) // auto detection
        output.isMesa170 = defaultDriverVersion >= 170000U;
    else
        output.isMesa170 = assembler.driverVersion >= 170000U;
    output.isLLVM390 = llvmVersion >= 30900U;
    return good;
}

void AsmGalliumHandler::writeBinary(std::ostream& os) const
{
    GalliumBinGenerator binGenerator(&output);
    binGenerator.generate(os);
}

void AsmGalliumHandler::writeBinary(Array<cxbyte>& array) const
{
    GalliumBinGenerator binGenerator(&output);
    binGenerator.generate(array);
}
