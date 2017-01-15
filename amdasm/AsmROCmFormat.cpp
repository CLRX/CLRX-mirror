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
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* rocmPseudoOpNamesTbl[] =
{
    "arch_minor", "arch_stepping",
    "call_convention", "codeversion", "config",
    "control_directive", "debug_private_segment_buffer_sgpr",
    "debug_wavefront_private_segment_offset_sgpr",
    "debugmode", "dims", "dx10clamp",
    "exceptions", "fkernel", "floatmode", "gds_segment_size",
    "group_segment_align", "ieeemode", "kcode",
    "kcodeend", "kernarg_segment_align",
    "kernarg_segment_size", "kernel_code_entry_offset",
    "kernel_code_prefetch_offset", "kernel_code_prefetch_size",
    "localsize", "machine", "max_scratch_backing_memory",
    "pgmrsrc1", "pgmrsrc2", "priority",
    "private_elem_size", "private_segment_align",
    "privmode", "reserved_sgprs", "reserved_vgprs",
    "runtime_loader_kernel_symbol",
    "scratchbuffer", "sgprsnum", "tgsize",
    "use_debug_enabled", "use_dispatch_id",
    "use_dispatch_ptr", "use_dynamic_call_stack",
    "use_flat_scratch_init", "use_grid_workgroup_count",
    "use_kernarg_segment_ptr", "use_ordered_append_gds",
    "use_private_segment_buffer", "use_private_segment_size",
    "use_ptr64", "use_queue_ptr", "use_xnack_enabled",
    "userdatanum", "vgprsnum", "wavefront_sgpr_count",
    "wavefront_size",  "workgroup_fbarrier_count",
    "workgroup_group_segment_size", "workitem_private_segment_size",
    "workitem_vgpr_count"
};

enum
{
    ROCMOP_ARCH_MINOR, ROCMOP_ARCH_STEPPING,
    ROCMOP_CALL_CONVENTION, ROCMOP_CODEVERSION, ROCMOP_CONFIG,
    ROCMOP_CONTROL_DIRECTIVE, ROCMOP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    ROCMOP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    ROCMOP_DEBUGMODE, ROCMOP_DIMS, ROCMOP_DX10CLAMP,
    ROCMOP_EXCEPTIONS, ROCMOP_FKERNEL, ROCMOP_FLOATMODE, ROCMOP_GDS_SEGMENT_SIZE,
    ROCMOP_GROUP_SEGMENT_ALIGN, ROCMOP_IEEEMODE, ROCMOP_KCODE,
    ROCMOP_KCODEEND, ROCMOP_KERNARG_SEGMENT_ALIGN,
    ROCMOP_KERNARG_SEGMENT_SIZE, ROCMOP_KERNEL_CODE_ENTRY_OFFSET,
    ROCMOP_KERNEL_CODE_PREFETCH_OFFSET, ROCMOP_KERNEL_CODE_PREFETCH_SIZE,
    ROCMOP_LOCALSIZE, ROCMOP_MACHINE, ROCMOP_MAX_SCRATCH_BACKING_MEMORY,
    ROCMOP_PGMRSRC1, ROCMOP_PGMRSRC2, ROCMOP_PRIORITY,
    ROCMOP_PRIVATE_ELEM_SIZE, ROCMOP_PRIVATE_SEGMENT_ALIGN,
    ROCMOP_PRIVMODE, ROCMOP_RESERVED_SGPRS, ROCMOP_RESERVED_VGPRS,
    ROCMOP_RUNTIME_LOADER_KERNEL_SYMBOL,
    ROCMOP_SCRATCHBUFFER, ROCMOP_SGPRSNUM, ROCMOP_TGSIZE,
    ROCMOP_USE_DEBUG_ENABLED, ROCMOP_USE_DISPATCH_ID,
    ROCMOP_USE_DISPATCH_PTR, ROCMOP_USE_DYNAMIC_CALL_STACK,
    ROCMOP_USE_FLAT_SCRATCH_INIT, ROCMOP_USE_GRID_WORKGROUP_COUNT,
    ROCMOP_USE_KERNARG_SEGMENT_PTR, ROCMOP_USE_ORDERED_APPEND_GDS,
    ROCMOP_USE_PRIVATE_SEGMENT_BUFFER, ROCMOP_USE_PRIVATE_SEGMENT_SIZE,
    ROCMOP_USE_PTR64, ROCMOP_USE_QUEUE_PTR, ROCMOP_USE_XNACK_ENABLED,
    ROCMOP_USERDATANUM, ROCMOP_VGPRSNUM, ROCMOP_WAVEFRONT_SGPR_COUNT,
    ROCMOP_WAVEFRONT_SIZE, ROCMOP_WORKGROUP_FBARRIER_COUNT,
    ROCMOP_WORKGROUP_GROUP_SEGMENT_SIZE, ROCMOP_WORKITEM_PRIVATE_SEGMENT_SIZE,
    ROCMOP_WORKITEM_VGPR_COUNT
};

/*
 * ROCm format handler
 */

AsmROCmHandler::AsmROCmHandler(Assembler& assembler): AsmFormatHandler(assembler),
             output{}, codeSection(0), commentSection(ASMSECT_NONE),
             extraSectionCount(0)
{
    output.archMinor = output.archStepping = UINT32_MAX;
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::CODE,
                ELFSECTID_TEXT, ".text" });
    currentKcodeKernel = ASMKERN_GLOBAL;
    savedSection = 0;
}

AsmROCmHandler::~AsmROCmHandler()
{
    for (Kernel* kernel: kernelStates)
        delete kernel;
}

cxuint AsmROCmHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = output.symbols.size();
    cxuint thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG, ELFSECTID_UNDEF, nullptr });
    kernelStates.push_back(
        new Kernel{ thisSection, nullptr, false, ASMSECT_NONE, thisSection });
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    return thisKernel;
}

cxuint AsmROCmHandler::addSection(const char* sectionName, cxuint kernelId)
{
    const cxuint thisSection = sections.size();
    Section section;
    section.kernelId = ASMKERN_GLOBAL;  // we ignore input kernelId, we go to main
        
    if (::strcmp(sectionName, ".text") == 0) // code
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
        section.type = AsmSectionType::ROCM_COMMENT;
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
    return thisSection;
}

cxuint AsmROCmHandler::getSectionId(const char* sectionName) const
{
    if (::strcmp(sectionName, ".text") == 0) // code
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

void AsmROCmHandler::setCurrentKernel(cxuint kernel)
{
    if (kernel != ASMKERN_GLOBAL && kernel >= kernelStates.size())
        throw AsmFormatException("KernelId out of range");
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else // if kernel
        kernelStates[assembler.currentKernel]->savedSection = assembler.currentSection;
    
    assembler.currentKernel = kernel;
    if (kernel != ASMKERN_GLOBAL)
        assembler.currentSection = kernelStates[kernel]->savedSection;
    else // default main section
        assembler.currentSection = savedSection;
}

void AsmROCmHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else // if kernel
        kernelStates[assembler.currentKernel]->savedSection = assembler.currentSection;
    
    assembler.currentSection = sectionId;
    assembler.currentKernel = sections[sectionId].kernelId;
}


AsmFormatHandler::SectionInfo AsmROCmHandler::getSectionInfo(cxuint sectionId) const
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

void AsmROCmHandler::restoreKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {
        Kernel& newKernel = *kernelStates[currentKcodeKernel];
        assembler.isaAssembler->setAllocatedRegisters(newKernel.allocRegs,
                            newKernel.allocRegFlags);
    }
}

void AsmROCmHandler::saveKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {   // save other state
        size_t regTypesNum;
        Kernel& oldKernel = *kernelStates[currentKcodeKernel];
        const cxuint* regs = assembler.isaAssembler->getAllocatedRegisters(
                            regTypesNum, oldKernel.allocRegFlags);
        std::copy(regs, regs+2, oldKernel.allocRegs);
    }
}


void AsmROCmHandler::handleLabel(const CString& label)
{
    if (assembler.sections[assembler.currentSection].type != AsmSectionType::CODE)
        return;
    auto kit = assembler.kernelMap.find(label);
    if (kit == assembler.kernelMap.end())
        return;
    if (!kcodeSelection.empty())
        return; // do not change if inside kcode
    // save other state
    saveKcodeCurrentAllocRegs();
    // restore this state
    currentKcodeKernel = kit->second;
    restoreKcodeCurrentAllocRegs();
}

void AsmROCmHandler::Kernel::initializeKernelConfig()
{
    if (!config)
    {
        config.reset(new AsmROCmKernelConfig{});
        // set default values to kernel config
        ::memset(config.get(), 0xff, 128);
        ::memset(config->controlDirective, 0, 128);
        config->computePgmRsrc1 = config->computePgmRsrc2 = 0;
        config->enableSpgrRegisterFlags = 0;
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

namespace CLRX
{

bool AsmROCmPseudoOps::checkPseudoOpName(const CString& string)
{
    if (string.empty() || string[0] != '.')
        return false;
    const size_t pseudoOp = binaryFind(rocmPseudoOpNamesTbl, rocmPseudoOpNamesTbl +
                sizeof(rocmPseudoOpNamesTbl)/sizeof(char*), string.c_str()+1,
               CStringLess()) - rocmPseudoOpNamesTbl;
    return pseudoOp < sizeof(rocmPseudoOpNamesTbl)/sizeof(char*);
}

void AsmROCmPseudoOps::setArchMinor(AsmROCmHandler& handler, const char* linePtr)
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

void AsmROCmPseudoOps::setArchStepping(AsmROCmHandler& handler, const char* linePtr)
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

    
void AsmROCmPseudoOps::doConfig(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "Kernel config can be defined only inside kernel");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    AsmROCmHandler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    asmr.goToSection(pseudoOpPlace, kernel.configSection);
    kernel.initializeKernelConfig();
}

void AsmROCmPseudoOps::doControlDirective(AsmROCmHandler& handler,
              const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "Kernel control directive can be defined "
                    "only inside kernel");
        return;
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmROCmHandler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.ctrlDirSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel,
            AsmSectionType::ROCM_CONFIG_CTRL_DIRECTIVE,
            ELFSECTID_UNDEF, nullptr });
        kernel.ctrlDirSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, kernel.ctrlDirSection);
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
}

void AsmROCmPseudoOps::doFKernel(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, ".fkernel can be only inside kernel");
        return;
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.kernelStates[asmr.currentKernel]->isFKernel = true;
}

void AsmROCmPseudoOps::setConfigValue(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr, ROCmConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value = BINGEN64_NOTSUPPLIED;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    /* ranges checking */
    if (good)
    {
        switch(target)
        {
            case ROCMCVAL_SGPRSNUM:
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
            case ROCMCVAL_VGPRSNUM:
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
            case ROCMCVAL_EXCEPTIONS:
                asmr.printWarningForRange(7, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0x7f;
                break;
            case ROCMCVAL_FLOATMODE:
                asmr.printWarningForRange(8, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0xff;
                break;
            case ROCMCVAL_PRIORITY:
                asmr.printWarningForRange(2, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 3;
                break;
            case ROCMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE: // local size
            {
                asmr.printWarningForRange(32, value,
                          asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                
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
            case ROCMCVAL_USERDATANUM:
                if (value > 16)
                {
                    asmr.printError(valuePlace, "UserDataNum out of range (0-16)");
                    good = false;
                }
                break;
            case ROCMCVAL_PRIVATE_ELEM_SIZE:
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
            case ROCMCVAL_KERNARG_SEGMENT_ALIGN:
            case ROCMCVAL_GROUP_SEGMENT_ALIGN:
            case ROCMCVAL_PRIVATE_SEGMENT_ALIGN:
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
            case ROCMCVAL_WAVEFRONT_SIZE:
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
            case ROCMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE:
            case ROCMCVAL_GDS_SEGMENT_SIZE:
            case ROCMCVAL_WORKGROUP_FBARRIER_COUNT:
            case ROCMCVAL_CALL_CONVENTION:
            case ROCMCVAL_PGMRSRC1:
            case ROCMCVAL_PGMRSRC2:
                asmr.printWarningForRange(32, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                break;
            case ROCMCVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR:
            case ROCMCVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR:
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
    
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    AsmROCmKernelConfig& config = *(handler.kernelStates[asmr.currentKernel]->config);
    // set value
    switch(target)
    {
        case ROCMCVAL_SGPRSNUM:
            config.usedSGPRsNum = value;
            break;
        case ROCMCVAL_VGPRSNUM:
            config.usedVGPRsNum = value;
            break;
        case ROCMCVAL_PGMRSRC1:
            config.computePgmRsrc1 = value;
            break;
        case ROCMCVAL_PGMRSRC2:
            config.computePgmRsrc2 = value;
            break;
        case ROCMCVAL_FLOATMODE:
            config.floatMode = value;
            break;
        case ROCMCVAL_PRIORITY:
            config.priority = value;
            break;
        case ROCMCVAL_USERDATANUM:
            config.userDataNum = value;
            break;
        case ROCMCVAL_EXCEPTIONS:
            config.exceptions = value;
            break;
        case ROCMCVAL_KERNEL_CODE_ENTRY_OFFSET:
            config.kernelCodeEntryOffset = value;
            break;
        case ROCMCVAL_KERNEL_CODE_PREFETCH_OFFSET:
            config.kernelCodePrefetchOffset = value;
            break;
        case ROCMCVAL_KERNEL_CODE_PREFETCH_SIZE:
            config.kernelCodePrefetchSize = value;
            break;
        case ROCMCVAL_MAX_SCRATCH_BACKING_MEMORY:
            config.maxScrachBackingMemorySize = value;
            break;
        case ROCMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE:
            config.workitemPrivateSegmentSize = value;
            break;
        case ROCMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE:
            config.workgroupGroupSegmentSize = value;
            break;
        case ROCMCVAL_GDS_SEGMENT_SIZE:
            config.gdsSegmentSize = value;
            break;
        case ROCMCVAL_KERNARG_SEGMENT_SIZE:
            config.kernargSegmentSize = value;
            break;
        case ROCMCVAL_WORKGROUP_FBARRIER_COUNT:
            config.workgroupFbarrierCount = value;
            break;
        case ROCMCVAL_WAVEFRONT_SGPR_COUNT:
            config.wavefrontSgprCount = value;
            break;
        case ROCMCVAL_WORKITEM_VGPR_COUNT:
            config.workitemVgprCount = value;
            break;
        case ROCMCVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR:
            config.debugWavefrontPrivateSegmentOffsetSgpr = value;
            break;
        case ROCMCVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR:
            config.debugPrivateSegmentBufferSgpr = value;
            break;
        case ROCMCVAL_PRIVATE_ELEM_SIZE:
            config.enableFeatureFlags = (config.enableFeatureFlags & ~6) |
                    ((63-CLZ64(value)-1)<<1);
            break;
        case ROCMCVAL_KERNARG_SEGMENT_ALIGN:
            config.kernargSegmentAlignment = 63-CLZ64(value);
            break;
        case ROCMCVAL_GROUP_SEGMENT_ALIGN:
            config.groupSegmentAlignment = 63-CLZ64(value);
            break;
        case ROCMCVAL_PRIVATE_SEGMENT_ALIGN:
            config.privateSegmentAlignment = 63-CLZ64(value);
            break;
        case ROCMCVAL_WAVEFRONT_SIZE:
            config.wavefrontSize = 63-CLZ64(value);
            break;
        case ROCMCVAL_CALL_CONVENTION:
            config.callConvention = value;
            break;
        case ROCMCVAL_RUNTIME_LOADER_KERNEL_SYMBOL:
            config.runtimeLoaderKernelSymbol = value;
            break;
        default:
            break;
    }
}

void AsmROCmPseudoOps::setConfigBoolValue(AsmROCmHandler& handler,
          const char* pseudoOpPlace, const char* linePtr, ROCmConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    AsmROCmKernelConfig& config = *(handler.kernelStates[asmr.currentKernel]->config);
    
    switch(target)
    {
        case ROCMCVAL_PRIVMODE:
            config.privilegedMode = true;
            break;
        case ROCMCVAL_DEBUGMODE:
            config.debugMode = true;
            break;
        case ROCMCVAL_DX10CLAMP:
            config.dx10Clamp = true;
            break;
        case ROCMCVAL_IEEEMODE:
            config.ieeeMode = true;
            break;
        case ROCMCVAL_TGSIZE:
            config.tgSize = true;
            break;
        case ROCMCVAL_USE_PRIVATE_SEGMENT_BUFFER:
            config.enableSpgrRegisterFlags |= ROCMFLAG_USE_PRIVATE_SEGMENT_BUFFER;
            break;
        case ROCMCVAL_USE_DISPATCH_PTR:
            config.enableSpgrRegisterFlags |= ROCMFLAG_USE_DISPATCH_PTR;
            break;
        case ROCMCVAL_USE_QUEUE_PTR:
            config.enableSpgrRegisterFlags |= ROCMFLAG_USE_QUEUE_PTR;
            break;
        case ROCMCVAL_USE_KERNARG_SEGMENT_PTR:
            config.enableSpgrRegisterFlags |= ROCMFLAG_USE_KERNARG_SEGMENT_PTR;
            break;
        case ROCMCVAL_USE_DISPATCH_ID:
            config.enableSpgrRegisterFlags |= ROCMFLAG_USE_DISPATCH_ID;
            break;
        case ROCMCVAL_USE_FLAT_SCRATCH_INIT:
            config.enableSpgrRegisterFlags |= ROCMFLAG_USE_FLAT_SCRATCH_INIT;
            break;
        case ROCMCVAL_USE_PRIVATE_SEGMENT_SIZE:
            config.enableSpgrRegisterFlags |= ROCMFLAG_USE_PRIVATE_SEGMENT_SIZE;
            break;
        case ROCMCVAL_USE_ORDERED_APPEND_GDS:
            config.enableFeatureFlags |= ROCMFLAG_USE_ORDERED_APPEND_GDS;
            break;
        case ROCMCVAL_USE_PTR64:
            config.enableFeatureFlags |= ROCMFLAG_USE_PTR64;
            break;
        case ROCMCVAL_USE_DYNAMIC_CALL_STACK:
            config.enableFeatureFlags |= ROCMFLAG_USE_DYNAMIC_CALL_STACK;
            break;
        case ROCMCVAL_USE_DEBUG_ENABLED:
            config.enableFeatureFlags |= ROCMFLAG_USE_DEBUG_ENABLED;
            break;
        case ROCMCVAL_USE_XNACK_ENABLED:
            config.enableFeatureFlags |= ROCMFLAG_USE_XNACK_ENABLED;
            break;
        default:
            break;
    }
}

void AsmROCmPseudoOps::setDimensions(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
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
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    handler.kernelStates[asmr.currentKernel]->config->dimMask = dimMask;
}

void AsmROCmPseudoOps::setMachine(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t kindValue = BINGEN_NOTSUPPLIED;
    uint64_t majorValue = BINGEN_NOTSUPPLIED;
    uint64_t minorValue = BINGEN_NOTSUPPLIED;
    uint64_t steppingValue = BINGEN_NOTSUPPLIED;
    const char* valuePlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, kindValue, linePtr, true);
    asmr.printWarningForRange(16, kindValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    valuePlace = linePtr;
    good &= getAbsoluteValueArg(asmr, majorValue, linePtr, true);
    asmr.printWarningForRange(16, majorValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    valuePlace = linePtr;
    good &= getAbsoluteValueArg(asmr, minorValue, linePtr, true);
    asmr.printWarningForRange(16, minorValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    valuePlace = linePtr;
    good &= getAbsoluteValueArg(asmr, steppingValue, linePtr, true);
    asmr.printWarningForRange(16, steppingValue,
                      asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    AsmROCmKernelConfig* config = handler.kernelStates[asmr.currentKernel]->config.get();
    config->amdMachineKind = kindValue;
    config->amdMachineMajor = majorValue;
    config->amdMachineMinor = minorValue;
    config->amdMachineStepping = steppingValue;
}

void AsmROCmPseudoOps::setCodeVersion(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t majorValue = BINGEN_NOTSUPPLIED;
    uint64_t minorValue = BINGEN_NOTSUPPLIED;
    const char* valuePlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, majorValue, linePtr, true);
    asmr.printWarningForRange(32, majorValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    valuePlace = linePtr;
    good &= getAbsoluteValueArg(asmr, minorValue, linePtr, true);
    asmr.printWarningForRange(32, minorValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    AsmROCmKernelConfig* config = handler.kernelStates[asmr.currentKernel]->config.get();
    config->amdCodeVersionMajor = majorValue;
    config->amdCodeVersionMinor = minorValue;
}

void AsmROCmPseudoOps::setReservedXgprs(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool inVgpr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(asmr.deviceType);
    cxuint maxGPRsNum = getGPUMaxRegistersNum(arch,
                       inVgpr ? REGTYPE_VGPR : REGTYPE_SGPR, 0);
    
    uint64_t firstReg = BINGEN_NOTSUPPLIED;
    uint64_t lastReg = BINGEN_NOTSUPPLIED;
    const char* valuePlace = linePtr;
    bool haveFirstReg;
    bool good = getAbsoluteValueArg(asmr, firstReg, linePtr, true);
    haveFirstReg = good;
    if (haveFirstReg && firstReg > maxGPRsNum-1)
    {
        char buf[64];
        snprintf(buf, 64, "First reserved %s register out of range (0-%u)",
                 inVgpr ? "VGPR" : "SGPR",  maxGPRsNum-1);
        asmr.printError(valuePlace, buf);
        good = false;
    }
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    valuePlace = linePtr;
    bool haveLastReg = getAbsoluteValueArg(asmr, lastReg, linePtr, true);
    good &= haveLastReg;
    if (haveLastReg && lastReg > maxGPRsNum-1)
    {
        char buf[64];
        snprintf(buf, 64, "Last reserved %s register out of range (0-%u)",
                 inVgpr ? "VGPR" : "SGPR", maxGPRsNum-1);
        asmr.printError(valuePlace, buf);
        good = false;
    }
    if (haveFirstReg && haveLastReg && firstReg > lastReg)
    {
        asmr.printError(valuePlace, "Wrong regsister range");
        good = false;
    }
        
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    AsmROCmKernelConfig* config = handler.kernelStates[asmr.currentKernel]->config.get();
    if (inVgpr)
    {
        config->reservedVgprFirst = firstReg;
        config->reservedVgprCount = lastReg-firstReg+1;
    }
    else
    {
        config->reservedSgprFirst = firstReg;
        config->reservedSgprCount = lastReg-firstReg+1;
    }
}


void AsmROCmPseudoOps::setUseGridWorkGroupCount(AsmROCmHandler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
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
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    uint16_t& flags = handler.kernelStates[asmr.currentKernel]->config->
                enableSpgrRegisterFlags;
    flags = (flags & ~(7<<ROCMFLAG_USE_GRID_WORKGROUP_COUNT_BIT)) |
            (dimMask<<ROCMFLAG_USE_GRID_WORKGROUP_COUNT_BIT);
}

void AsmROCmPseudoOps::updateKCodeSel(AsmROCmHandler& handler,
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
        cxuint newAllocRegs[2];
        AsmROCmHandler::Kernel& kernel = *(handler.kernelStates[*it]);
        newAllocRegs[0] = std::max(curAllocRegs[0], kernel.allocRegs[0]);
        newAllocRegs[1] = std::max(curAllocRegs[1], kernel.allocRegs[1]);
        kernel.allocRegFlags |= curAllocRegFlags;
        std::copy(newAllocRegs, newAllocRegs+2, kernel.allocRegs);
    }
    asmr.isaAssembler->setAllocatedRegisters();
}

void AsmROCmPseudoOps::doKCode(AsmROCmHandler& handler, const char* pseudoOpPlace,
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
    updateKCodeSel(handler, handler.kcodeSelStack.top());
}

void AsmROCmPseudoOps::doKCodeEnd(AsmROCmHandler& handler, const char* pseudoOpPlace,
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
    handler.kcodeSelection = handler.kcodeSelStack.top();
    handler.kcodeSelStack.pop();
    if (handler.kcodeSelStack.empty())
        handler.restoreKcodeCurrentAllocRegs();
}

}

bool AsmROCmHandler::parsePseudoOp(const CString& firstName, const char* stmtPlace,
               const char* linePtr)
{
    const size_t pseudoOp = binaryFind(rocmPseudoOpNamesTbl, rocmPseudoOpNamesTbl +
                    sizeof(rocmPseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - rocmPseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case ROCMOP_ARCH_MINOR:
            AsmROCmPseudoOps::setArchMinor(*this, linePtr);
            break;
        case ROCMOP_ARCH_STEPPING:
            AsmROCmPseudoOps::setArchStepping(*this, linePtr);
            break;
        case ROCMOP_CALL_CONVENTION:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_CALL_CONVENTION);
            break;
        case ROCMOP_CODEVERSION:
            AsmROCmPseudoOps::setCodeVersion(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_CONFIG:
            AsmROCmPseudoOps::doConfig(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_CONTROL_DIRECTIVE:
            AsmROCmPseudoOps::doControlDirective(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR);
            break;
        case ROCMOP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                         ROCMCVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR);
            break;
        case ROCMOP_DEBUGMODE:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_DEBUGMODE);
            break;
        case ROCMOP_DIMS:
            AsmROCmPseudoOps::setDimensions(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_DX10CLAMP:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_DX10CLAMP);
            break;
        case ROCMOP_EXCEPTIONS:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_EXCEPTIONS);
            break;
        case ROCMOP_FKERNEL:
            AsmROCmPseudoOps::doFKernel(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_FLOATMODE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_FLOATMODE);
            break;
        case ROCMOP_GDS_SEGMENT_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_GDS_SEGMENT_SIZE);
            break;
        case ROCMOP_GROUP_SEGMENT_ALIGN:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_GROUP_SEGMENT_ALIGN);
            break;
        case ROCMOP_IEEEMODE:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_IEEEMODE);
            break;
        case ROCMOP_KCODE:
            AsmROCmPseudoOps::doKCode(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_KCODEEND:
            AsmROCmPseudoOps::doKCodeEnd(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_KERNARG_SEGMENT_ALIGN:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_KERNARG_SEGMENT_ALIGN);
            break;
        case ROCMOP_KERNARG_SEGMENT_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_KERNARG_SEGMENT_SIZE);
            break;
        case ROCMOP_KERNEL_CODE_ENTRY_OFFSET:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_KERNEL_CODE_ENTRY_OFFSET);
            break;
        case ROCMOP_KERNEL_CODE_PREFETCH_OFFSET:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_KERNEL_CODE_PREFETCH_OFFSET);
            break;
        case ROCMOP_KERNEL_CODE_PREFETCH_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_KERNEL_CODE_PREFETCH_SIZE);
            break;
        case ROCMOP_LOCALSIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE);
            break;
        case ROCMOP_MACHINE:
            AsmROCmPseudoOps::setMachine(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_MAX_SCRATCH_BACKING_MEMORY:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_MAX_SCRATCH_BACKING_MEMORY);
            break;
        case ROCMOP_PGMRSRC1:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr, ROCMCVAL_PGMRSRC1);
            break;
        case ROCMOP_PGMRSRC2:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr, ROCMCVAL_PGMRSRC2);
            break;
        case ROCMOP_PRIORITY:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr, ROCMCVAL_PRIORITY);
            break;
        case ROCMOP_PRIVATE_ELEM_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_PRIVATE_ELEM_SIZE);
            break;
        case ROCMOP_PRIVATE_SEGMENT_ALIGN:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_PRIVATE_SEGMENT_ALIGN);
            break;
        case ROCMOP_PRIVMODE:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_PRIVMODE);
            break;
        case ROCMOP_RESERVED_SGPRS:
            AsmROCmPseudoOps::setReservedXgprs(*this, stmtPlace, linePtr, false);
            break;
        case ROCMOP_RESERVED_VGPRS:
            AsmROCmPseudoOps::setReservedXgprs(*this, stmtPlace, linePtr, true);
            break;
        case ROCMOP_RUNTIME_LOADER_KERNEL_SYMBOL:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_RUNTIME_LOADER_KERNEL_SYMBOL);
            break;
        case ROCMOP_SCRATCHBUFFER:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE);
            break;
        case ROCMOP_SGPRSNUM:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_SGPRSNUM);
            break;
        case ROCMOP_TGSIZE:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_TGSIZE);
            break;
        case ROCMOP_USE_DEBUG_ENABLED:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_DEBUG_ENABLED);
            break;
        case ROCMOP_USE_DISPATCH_ID:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_DISPATCH_ID);
            break;
        case ROCMOP_USE_DISPATCH_PTR:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_DISPATCH_PTR);
            break;
        case ROCMOP_USE_DYNAMIC_CALL_STACK:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_DYNAMIC_CALL_STACK);
            break;
        case ROCMOP_USE_FLAT_SCRATCH_INIT:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_FLAT_SCRATCH_INIT);
            break;
        case ROCMOP_USE_GRID_WORKGROUP_COUNT:
            AsmROCmPseudoOps::setUseGridWorkGroupCount(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_USE_KERNARG_SEGMENT_PTR:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_KERNARG_SEGMENT_PTR);
            break;
        case ROCMOP_USE_ORDERED_APPEND_GDS:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_ORDERED_APPEND_GDS);
            break;
        case ROCMOP_USE_PRIVATE_SEGMENT_SIZE:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_PRIVATE_SEGMENT_SIZE);
            break;
        case ROCMOP_USE_PTR64:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_PTR64);
            break;
        case ROCMOP_USE_QUEUE_PTR:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_QUEUE_PTR);
            break;
        case ROCMOP_USE_PRIVATE_SEGMENT_BUFFER:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_PRIVATE_SEGMENT_BUFFER);
            break;
        case ROCMOP_USE_XNACK_ENABLED:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USE_XNACK_ENABLED);
            break;
        case ROCMOP_USERDATANUM:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_USERDATANUM);
            break;
        case ROCMOP_VGPRSNUM:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr, ROCMCVAL_VGPRSNUM);
            break;
        case ROCMOP_WAVEFRONT_SGPR_COUNT:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_WAVEFRONT_SGPR_COUNT);
            break;
        case ROCMOP_WAVEFRONT_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_WAVEFRONT_SIZE);
            break;
        case ROCMOP_WORKITEM_VGPR_COUNT:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_WORKITEM_VGPR_COUNT);
            break;
        case ROCMOP_WORKGROUP_FBARRIER_COUNT:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_WORKGROUP_FBARRIER_COUNT);
            break;
        case ROCMOP_WORKGROUP_GROUP_SEGMENT_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE);
            break;
        case ROCMOP_WORKITEM_PRIVATE_SEGMENT_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE);
            break;
        default:
            return false;
    }
    return true;
}

static const AMDGPUArchValues rocmAmdGpuArchValuesTbl[] =
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
    { 8, 0, 4 } // GPUDeviceType::BAFFIN
};

bool AsmROCmHandler::prepareBinary()
{
    bool good = true;
    size_t sectionsNum = sections.size();
    output.deviceType = assembler.getDeviceType();
    
    if (assembler.isaAssembler!=nullptr)
    {   // make last kernel registers pool updates
        if (kcodeSelStack.empty())
            saveKcodeCurrentAllocRegs();
        else
            while (!kcodeSelStack.empty())
            {   // pop from kcode stack and apply changes
                AsmROCmPseudoOps::updateKCodeSel(*this, kcodeSelection);
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
            case AsmSectionType::ROCM_CONFIG_CTRL_DIRECTIVE:
                if (sectionSize != 128)
                    assembler.printError(AsmSourcePos(),
                         (std::string("Section '.control_directive' for kernel '")+
                          assembler.kernels[section.kernelId].name+
                          "' have wrong size").c_str());
                break;
            case AsmSectionType::ROCM_COMMENT:
                output.commentSize = sectionSize;
                output.comment = (const char*)sectionData;
                break;
            default:
                break;
        }
    }
    
    GPUArchitecture arch = getGPUArchitectureFromDeviceType(assembler.deviceType);
    // set up number of the allocated SGPRs and VGPRs for kernel
    cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    AMDGPUArchValues amdGpuArchValues = rocmAmdGpuArchValuesTbl[
                    cxuint(assembler.deviceType)];
    if (output.archMinor!=UINT32_MAX)
        amdGpuArchValues.minor = output.archMinor;
    if (output.archStepping!=UINT32_MAX)
        amdGpuArchValues.stepping = output.archStepping;
    
    const cxuint ldsShift = arch<GPUArchitecture::GCN1_1 ? 8 : 9;
    const uint32_t ldsMask = (1U<<ldsShift)-1U;
    // prepare kernels configuration
    for (size_t i = 0; i < kernelStates.size(); i++)
    {
        const Kernel& kernel = *kernelStates[i];
        if (kernel.config.get() == nullptr)
            continue;
        const CString& kernelName = assembler.kernels[i].name;
        AsmROCmKernelConfig& config = *kernel.config.get();
        // setup config
        // fill default values
        if (config.amdCodeVersionMajor == BINGEN_DEFAULT)
            config.amdCodeVersionMajor = 1;
        if (config.amdCodeVersionMinor == BINGEN_DEFAULT)
            config.amdCodeVersionMinor = 0;
        if (config.amdMachineKind == BINGEN16_DEFAULT)
            config.amdMachineKind = 1;
        if (config.amdMachineMajor == BINGEN16_DEFAULT)
            config.amdMachineMajor = amdGpuArchValues.major;
        if (config.amdMachineMinor == BINGEN16_DEFAULT)
            config.amdMachineMinor = amdGpuArchValues.minor;
        if (config.amdMachineStepping == BINGEN16_DEFAULT)
            config.amdMachineStepping = amdGpuArchValues.stepping;
        if (config.kernelCodeEntryOffset == BINGEN64_DEFAULT)
            config.kernelCodeEntryOffset = 256;
        if (config.kernelCodePrefetchOffset == BINGEN64_DEFAULT)
            config.kernelCodePrefetchOffset = 0;
        if (config.kernelCodePrefetchSize == BINGEN64_DEFAULT)
            config.kernelCodePrefetchSize = 0;
        if (config.maxScrachBackingMemorySize == BINGEN64_DEFAULT) // ??
            config.maxScrachBackingMemorySize = 0;
        
        if (config.workitemPrivateSegmentSize == BINGEN_DEFAULT) // scratch buffer
            config.workitemPrivateSegmentSize =  0;
        if (config.workgroupGroupSegmentSize == BINGEN_DEFAULT) // local size
            config.workgroupGroupSegmentSize = 0;
        if (config.gdsSegmentSize == BINGEN_DEFAULT)
            config.gdsSegmentSize = 0;
        if (config.kernargSegmentSize == BINGEN64_DEFAULT)
            config.kernargSegmentSize = 0;
        if (config.workgroupFbarrierCount == BINGEN_DEFAULT)
            config.workgroupFbarrierCount = 0;
        if (config.reservedVgprFirst == BINGEN16_DEFAULT)
            config.reservedVgprFirst = 0;
        if (config.reservedVgprCount == BINGEN16_DEFAULT)
            config.reservedVgprCount = 0;
        if (config.reservedSgprFirst == BINGEN16_DEFAULT)
            config.reservedSgprFirst = 0;
        if (config.reservedSgprCount == BINGEN16_DEFAULT)
            config.reservedSgprCount = 0;
        if (config.debugWavefrontPrivateSegmentOffsetSgpr == BINGEN16_DEFAULT)
            config.debugWavefrontPrivateSegmentOffsetSgpr = 0;
        if (config.debugPrivateSegmentBufferSgpr == BINGEN16_DEFAULT)
            config.debugPrivateSegmentBufferSgpr = 0;
        if (config.kernargSegmentAlignment == BINGEN8_DEFAULT)
            config.kernargSegmentAlignment = 4; // 16 bytes
        if (config.groupSegmentAlignment == BINGEN8_DEFAULT)
            config.groupSegmentAlignment = 4; // 16 bytes
        if (config.privateSegmentAlignment == BINGEN8_DEFAULT)
            config.privateSegmentAlignment = 4; // 16 bytes
        if (config.wavefrontSize == BINGEN8_DEFAULT)
            config.wavefrontSize = 6; // 64 threads
        
        cxuint userSGPRsNum = 0;
        if (config.userDataNum == BINGEN8_DEFAULT)
        {   // calcuate userSGPRs
            const uint16_t sgprFlags = config.enableSpgrRegisterFlags;
            userSGPRsNum =
                ((sgprFlags&ROCMFLAG_USE_PRIVATE_SEGMENT_BUFFER)!=0 ? 4 : 0) +
                ((sgprFlags&ROCMFLAG_USE_DISPATCH_PTR)!=0 ? 2 : 0) +
                ((sgprFlags&ROCMFLAG_USE_QUEUE_PTR)!=0 ? 2 : 0) +
                ((sgprFlags&ROCMFLAG_USE_KERNARG_SEGMENT_PTR)!=0 ? 2 : 0) +
                ((sgprFlags&ROCMFLAG_USE_DISPATCH_ID)!=0 ? 2 : 0) +
                ((sgprFlags&ROCMFLAG_USE_FLAT_SCRATCH_INIT)!=0 ? 2 : 0) +
                ((sgprFlags&ROCMFLAG_USE_PRIVATE_SEGMENT_SIZE)!=0) +
                /* use_grid_workgroup_count */
                ((sgprFlags&ROCMFLAG_USE_GRID_WORKGROUP_COUNT_X)!=0) +
                ((sgprFlags&ROCMFLAG_USE_GRID_WORKGROUP_COUNT_Y)!=0) +
                ((sgprFlags&ROCMFLAG_USE_GRID_WORKGROUP_COUNT_Z)!=0);
           userSGPRsNum = std::min(16U, userSGPRsNum);
        }
        else // default
            userSGPRsNum = config.userDataNum;
        
        /* include userData sgprs */
        cxuint dimMask = (config.dimMask!=BINGEN_DEFAULT) ? config.dimMask :
                ((config.computePgmRsrc2>>7)&7);
        // extra sgprs for dimensions
        cxuint minRegsNum[2];
        getGPUSetupMinRegistersNum(arch, dimMask, userSGPRsNum,
                   ((config.tgSize) ? GPUSETUP_TGSIZE_EN : 0) |
                   ((config.workitemPrivateSegmentSize!=0) ?
                           GPUSETUP_SCRATCH_EN : 0), minRegsNum);
        
        if (config.usedSGPRsNum!=BINGEN_DEFAULT && maxSGPRsNum < config.usedSGPRsNum)
        {   // check only if sgprsnum set explicitly
            char numBuf[64];
            snprintf(numBuf, 64, "(max %u)", maxSGPRsNum);
            assembler.printError(assembler.kernels[i].sourcePos, (std::string(
                    "Number of total SGPRs for kernel '")+
                    kernelName.c_str()+"' is too high "+numBuf).c_str());
            good = false;
        }
        // set usedSGPRsNum
        if (config.usedSGPRsNum==BINGEN_DEFAULT)
        {
            cxuint flags = kernelStates[i]->allocRegFlags |
                // flat_scratch_init
                ((config.enableSpgrRegisterFlags&ROCMFLAG_USE_FLAT_SCRATCH_INIT)!=0?
                            GCN_FLAT : 0) |
                // enable_xnack
                ((config.enableFeatureFlags&ROCMFLAG_USE_XNACK_ENABLED)!=0 ?
                            GCN_XNACK : 0);
            config.usedSGPRsNum = std::min(
                std::max(minRegsNum[0], kernelStates[i]->allocRegs[0]) +
                    getGPUExtraRegsNum(arch, REGTYPE_SGPR, flags),
                    maxSGPRsNum); // include all extra sgprs
        }
        // set usedVGPRsNum
        if (config.usedVGPRsNum==BINGEN_DEFAULT)
            config.usedVGPRsNum = std::max(minRegsNum[1], kernelStates[i]->allocRegs[1]);
        
        cxuint sgprsNum = std::max(config.usedSGPRsNum, 1U);
        cxuint vgprsNum = std::max(config.usedVGPRsNum, 1U);
        // computePGMRSRC1
        config.computePgmRsrc1 |= ((vgprsNum-1)>>2) |
                (((sgprsNum-1)>>3)<<6) | ((uint32_t(config.floatMode)&0xff)<<12) |
                (config.ieeeMode?1U<<23:0) | (uint32_t(config.priority&3)<<10) |
                (config.privilegedMode?1U<<20:0) | (config.dx10Clamp?1U<<21:0) |
                (config.debugMode?1U<<22:0);
                
        uint32_t dimValues = 0;
        if (config.dimMask != BINGEN_DEFAULT)
            dimValues = ((config.dimMask&7)<<7) |
                    (((config.dimMask&4) ? 2 : (config.dimMask&2) ? 1 : 0)<<11);
        else
            dimValues |= (config.computePgmRsrc2 & 0x1b80U);
        // computePGMRSRC2
        config.computePgmRsrc2 = (config.computePgmRsrc2 & 0xffffe440U) |
                        (userSGPRsNum<<1) | ((config.tgSize) ? 0x400 : 0) |
                        ((config.workitemPrivateSegmentSize)?1:0) | dimValues |
                        (((config.workgroupGroupSegmentSize+ldsMask)>>ldsShift)<<15) |
                        ((uint32_t(config.exceptions)&0x7f)<<24);
        
        if (config.wavefrontSgprCount == BINGEN16_DEFAULT)
            config.wavefrontSgprCount = sgprsNum;
        if (config.workitemVgprCount == BINGEN16_DEFAULT)
            config.workitemVgprCount = vgprsNum;
        
        if (config.callConvention == BINGEN_DEFAULT)
            config.callConvention = 0;
        if (config.runtimeLoaderKernelSymbol == BINGEN64_DEFAULT)
            config.runtimeLoaderKernelSymbol = 0;
        
        // to little endian
        SLEV(config.amdCodeVersionMajor, config.amdCodeVersionMajor);
        SLEV(config.amdCodeVersionMinor, config.amdCodeVersionMinor);
        SLEV(config.amdMachineKind, config.amdMachineKind);
        SLEV(config.amdMachineMajor, config.amdMachineMajor);
        SLEV(config.amdMachineMinor, config.amdMachineMinor);
        SLEV(config.amdMachineStepping, config.amdMachineStepping);
        SLEV(config.kernelCodeEntryOffset, config.kernelCodeEntryOffset);
        SLEV(config.kernelCodePrefetchOffset, config.kernelCodePrefetchOffset);
        SLEV(config.kernelCodePrefetchSize, config.kernelCodePrefetchSize);
        SLEV(config.maxScrachBackingMemorySize, config.maxScrachBackingMemorySize);
        SLEV(config.computePgmRsrc1, config.computePgmRsrc1);
        SLEV(config.computePgmRsrc2, config.computePgmRsrc2);
        SLEV(config.enableSpgrRegisterFlags, config.enableSpgrRegisterFlags);
        SLEV(config.enableFeatureFlags, config.enableFeatureFlags);
        SLEV(config.workitemPrivateSegmentSize, config.workitemPrivateSegmentSize);
        SLEV(config.workgroupGroupSegmentSize, config.workgroupGroupSegmentSize);
        SLEV(config.gdsSegmentSize, config.gdsSegmentSize);
        SLEV(config.kernargSegmentSize, config.kernargSegmentSize);
        SLEV(config.workgroupFbarrierCount, config.workgroupFbarrierCount);
        SLEV(config.wavefrontSgprCount, config.wavefrontSgprCount);
        SLEV(config.workitemVgprCount, config.workitemVgprCount);
        SLEV(config.reservedVgprFirst, config.reservedVgprFirst);
        SLEV(config.reservedVgprCount, config.reservedVgprCount);
        SLEV(config.reservedSgprFirst, config.reservedSgprFirst);
        SLEV(config.reservedSgprCount, config.reservedSgprCount);
        SLEV(config.debugWavefrontPrivateSegmentOffsetSgpr,
             config.debugWavefrontPrivateSegmentOffsetSgpr);
        SLEV(config.debugPrivateSegmentBufferSgpr, config.debugPrivateSegmentBufferSgpr);
        SLEV(config.callConvention, config.callConvention);
        SLEV(config.runtimeLoaderKernelSymbol, config.runtimeLoaderKernelSymbol);
        // put control directive section to config
        if (kernel.ctrlDirSection!=ASMSECT_NONE &&
            assembler.sections[kernel.ctrlDirSection].content.size()==128)
            ::memcpy(config.controlDirective, 
                 assembler.sections[kernel.ctrlDirSection].content.data(), 128);
    }
    
    // if set adds symbols to binary
    std::vector<ROCmSymbolInput> dataSymbols;
    if (assembler.getFlags() & ASM_FORCE_ADD_SYMBOLS)
        for (const AsmSymbolEntry& symEntry: assembler.symbolMap)
        {
            if (!symEntry.second.hasValue)
                continue; // unresolved
            if (ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // local
            if (assembler.kernelMap.find(symEntry.first.c_str())!=assembler.kernelMap.end())
                continue; // if kernel name
            
            if (symEntry.second.sectionId==codeSection)
            {   // put data objects
                dataSymbols.push_back({symEntry.first, size_t(symEntry.second.value),
                    size_t(symEntry.second.size), ROCmRegionType::DATA});
                continue;
            }
            
            cxuint binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            
            output.extraSymbols.push_back({ symEntry.first, symEntry.second.value,
                    symEntry.second.size, binSectId, false, symEntry.second.info,
                    symEntry.second.other });
        }
    
    AsmSection& asmCSection = assembler.sections[codeSection];
    const AsmSymbolMap& symbolMap = assembler.getSymbolMap();
    for (size_t ki = 0; ki < output.symbols.size(); ki++)
    {
        ROCmSymbolInput& kinput = output.symbols[ki];
        auto it = symbolMap.find(kinput.symbolName);
        if (it == symbolMap.end() || !it->second.isDefined())
        {   // error, undefined
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                        "Symbol for kernel '")+kinput.symbolName.c_str()+
                        "' is undefined").c_str());
            good = false;
            continue;
        }
        const AsmSymbol& symbol = it->second;
        if (!symbol.hasValue)
        {   // error, unresolved
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '") + kinput.symbolName.c_str() +
                    "' is not resolved").c_str());
            good = false;
            continue;
        }
        if (symbol.sectionId != codeSection)
        {   /// error, wrong section
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '")+kinput.symbolName.c_str()+
                    "' is defined for section other than '.text'").c_str());
            good = false;
            continue;
        }
        const Kernel& kernel = *kernelStates[ki];
        kinput.offset = symbol.value;
        
        if (asmCSection.content.size() < symbol.value + sizeof(ROCmKernelConfig))
        {
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                "Code for kernel '")+kinput.symbolName.c_str()+
                "' is too small for configuration").c_str());
            good = false;
            continue;
        }
        else if (kernel.config!=nullptr) // put config to code section
            ::memcpy(asmCSection.content.data() + symbol.value,
                     kernel.config.get(), sizeof(ROCmKernelConfig));
        // set symbol type
        kinput.type = kernel.isFKernel ? ROCmRegionType::FKERNEL : ROCmRegionType::KERNEL;
    }
    
    // put data objects
    dataSymbols.insert(dataSymbols.end(), output.symbols.begin(), output.symbols.end());
    output.symbols = std::move(dataSymbols);
    return good;
}

void AsmROCmHandler::writeBinary(std::ostream& os) const
{
    ROCmBinGenerator binGenerator(&output);
    binGenerator.generate(os);
}

void AsmROCmHandler::writeBinary(Array<cxbyte>& array) const
{
    ROCmBinGenerator binGenerator(&output);
    binGenerator.generate(array);
}
