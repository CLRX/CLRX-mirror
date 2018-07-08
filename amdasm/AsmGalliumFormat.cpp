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

#include <CLRX/Config.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include "AsmROCmInternals.h"
#include "AsmGalliumInternals.h"

using namespace CLRX;

// all Gallium pseudo-op names (sorted)
static const char* galliumPseudoOpNamesTbl[] =
{
    "arch_minor", "arch_stepping",
    "arg", "args", "call_convention", "codeversion",
    "config", "control_directive",
    "debug_private_segment_buffer_sgpr",
    "debug_wavefront_private_segment_offset_sgpr",
    "debugmode", "default_hsa_features",
    "dims", "driver_version", "dx10clamp",
    "entry", "exceptions", "floatmode", "gds_segment_size",
    "get_driver_version", "get_llvm_version", "globaldata",
    "group_segment_align",
    "hsa_debugmode", "hsa_dims", "hsa_dx10clamp", "hsa_exceptions",
    "hsa_floatmode", "hsa_ieeemode",
    "hsa_localsize", "hsa_pgmrsrc1",
    "hsa_pgmrsrc2", "hsa_priority", "hsa_privmode",
    "hsa_scratchbuffer", "hsa_sgprsnum", "hsa_tgsize",
    "hsa_userdatanum", "hsa_vgprsnum",
    "ieeemode", "kcode", "kcodeend",
    "kernarg_segment_align", "kernarg_segment_size",
    "kernel_code_entry_offset", "kernel_code_prefetch_offset",
    "kernel_code_prefetch_size",
    "llvm_version", "localsize",
    "machine", "max_scratch_backing_memory",
    "pgmrsrc1", "pgmrsrc2", "priority",
    "private_elem_size", "private_segment_align",
    "privmode", "proginfo", "reserved_sgprs", "reserved_vgprs",
    "runtime_loader_kernel_symbol",
    "scratchbuffer", "scratchsym", "sgprsnum",
    "spilledsgprs", "spilledvgprs", "tgsize",
    "use_debug_enabled", "use_dispatch_id",
    "use_dispatch_ptr", "use_dynamic_call_stack",
    "use_flat_scratch_init", "use_grid_workgroup_count",
    "use_kernarg_segment_ptr",
    "use_ordered_append_gds", "use_private_segment_buffer",
    "use_private_segment_size", "use_ptr64",
    "use_queue_ptr", "use_xnack_enabled",
    "userdatanum", "vgprsnum",
    "wavefront_sgpr_count", "wavefront_size",
    "workgroup_fbarrier_count", "workgroup_group_segment_size",
    "workitem_private_segment_size", "workitem_vgpr_count"
};

// all enums for Gallium pseudo-ops
enum
{
    GALLIUMOP_ARCH_MINOR = 0, GALLIUMOP_ARCH_STEPPING,
    GALLIUMOP_ARG, GALLIUMOP_ARGS, GALLIUMOP_CALL_CONVENTION, GALLIUMOP_CODEVERSION,
    GALLIUMOP_CONFIG, GALLIUMOP_CONTROL_DIRECTIVE,
    GALLIUMOP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    GALLIUMOP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    GALLIUMOP_DEBUGMODE, GALLIUMOP_DEFAULT_HSA_FEATURES,
    GALLIUMOP_DIMS, GALLIUMOP_DRIVER_VERSION, GALLIUMOP_DX10CLAMP,
    GALLIUMOP_ENTRY, GALLIUMOP_EXCEPTIONS, GALLIUMOP_FLOATMODE, GALLIUMOP_GDS_SEGMENT_SIZE,
    GALLIUMOP_GET_DRIVER_VERSION, GALLIUMOP_GET_LLVM_VERSION, GALLIUMOP_GLOBALDATA,
    GALLIUMOP_GROUP_SEGMENT_ALIGN,
    GALLIUMOP_HSA_DEBUGMODE, GALLIUMOP_HSA_DIMS,
    GALLIUMOP_HSA_DX10CLAMP, GALLIUMOP_HSA_EXCEPTIONS,
    GALLIUMOP_HSA_FLOATMODE, GALLIUMOP_HSA_IEEEMODE,
    GALLIUMOP_HSA_LOCALSIZE, GALLIUMOP_HSA_PGMRSRC1,
    GALLIUMOP_HSA_PGMRSRC2, GALLIUMOP_HSA_PRIORITY, GALLIUMOP_HSA_PRIVMODE,
    GALLIUMOP_HSA_SCRATCHBUFFER, GALLIUMOP_HSA_SGPRSNUM, GALLIUMOP_HSA_TGSIZE,
    GALLIUMOP_HSA_USERDATANUM, GALLIUMOP_HSA_VGPRSNUM,
    GALLIUMOP_IEEEMODE, GALLIUMOP_KCODE, GALLIUMOP_KCODEEND,
    GALLIUMOP_KERNARG_SEGMENT_ALIGN, GALLIUMOP_KERNARG_SEGMENT_SIZE,
    GALLIUMOP_KERNEL_CODE_ENTRY_OFFSET, GALLIUMOP_KERNEL_CODE_PREFETCH_OFFSET,
    GALLIUMOP_KERNEL_CODE_PREFETCH_SIZE,
    GALLIUMOP_LLVM_VERSION, GALLIUMOP_LOCALSIZE,
    GALLIUMOP_MACHINE, GALLIUMOP_MAX_SCRATCH_BACKING_MEMORY,
    GALLIUMOP_PGMRSRC1, GALLIUMOP_PGMRSRC2, GALLIUMOP_PRIORITY,
    GALLIUMOP_PRIVATE_ELEM_SIZE, GALLIUMOP_PRIVATE_SEGMENT_ALIGN,
    GALLIUMOP_PRIVMODE, GALLIUMOP_PROGINFO,
    GALLIUMOP_RESERVED_SGPRS, GALLIUMOP_RESERVED_VGPRS,
    GALLIUMOP_RUNTIME_LOADER_KERNEL_SYMBOL,
    GALLIUMOP_SCRATCHBUFFER, GALLIUMOP_SCRATCHSYM, GALLIUMOP_SGPRSNUM,
    GALLIUMOP_SPILLEDSGPRS, GALLIUMOP_SPILLEDVGPRS, GALLIUMOP_TGSIZE,
    GALLIUMOP_USE_DEBUG_ENABLED, GALLIUMOP_USE_DISPATCH_ID,
    GALLIUMOP_USE_DISPATCH_PTR, GALLIUMOP_USE_DYNAMIC_CALL_STACK,
    GALLIUMOP_USE_FLAT_SCRATCH_INIT, GALLIUMOP_USE_GRID_WORKGROUP_COUNT,
    GALLIUMOP_USE_KERNARG_SEGMENT_PTR,
    GALLIUMOP_USE_ORDERED_APPEND_GDS, GALLIUMOP_USE_PRIVATE_SEGMENT_BUFFER,
    GALLIUMOP_USE_PRIVATE_SEGMENT_SIZE, GALLIUMOP_USE_PTR64,
    GALLIUMOP_USE_QUEUE_PTR, GALLIUMOP_USE_XNACK_ENABLED,
    GALLIUMOP_USERDATANUM, GALLIUMOP_VGPRSNUM,
    GALLIUMOP_WAVEFRONT_SGPR_COUNT, GALLIUMOP_WAVEFRONT_SIZE,
    GALLIUMOP_WORKGROUP_FBARRIER_COUNT, GALLIUMOP_WORKGROUP_GROUP_SEGMENT_SIZE,
    GALLIUMOP_WORKITEM_PRIVATE_SEGMENT_SIZE, GALLIUMOP_WORKITEM_VGPR_COUNT
};

void AsmGalliumHandler::Kernel::initializeAmdHsaKernelConfig()
{
    if (!hsaConfig)
    {
        hsaConfig.reset(new AsmAmdHsaKernelConfig{});
        hsaConfig->initialize();
    }
}

/*
 * GalliumCompute format handler
 */

AsmGalliumHandler::AsmGalliumHandler(Assembler& assembler):
             AsmKcodeHandler(assembler), output{}, dataSection(ASMSECT_NONE),
             commentSection(ASMSECT_NONE), scratchSection(ASMSECT_NONE),
             extraSectionCount(0), archMinor(BINGEN_DEFAULT), archStepping(BINGEN_DEFAULT)
{
    codeSection = 0;
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    // define .text section (will be first section)
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::CODE,
                ELFSECTID_TEXT, ".text" });
    inside = Inside::MAINLAYOUT;
    savedSection = 0;
    // detect driver and LLVM version once for using many times
    detectedLLVMVersion = detectLLVMCompilerVersion();
    detectedDriverVersion = detectMesaDriverVersion();
}

AsmGalliumHandler::~AsmGalliumHandler()
{
    for (Kernel* kernel: kernelStates)
        delete kernel;
}

// determine LLVM version from assembler settings or CLRX settings
cxuint AsmGalliumHandler::determineLLVMVersion() const
{
    if (assembler.getLLVMVersion() == 0)
        return detectedLLVMVersion;
    else
        return assembler.getLLVMVersion();
}

// determine Mesa3D driver version from assembler settings or CLRX settings
cxuint AsmGalliumHandler::determineDriverVersion() const
{
    if (assembler.getDriverVersion() == 0)
        return detectedDriverVersion;
    else
        return assembler.getDriverVersion();
}

AsmKernelId AsmGalliumHandler::addKernel(const char* kernelName)
{
    AsmKernelId thisKernel = output.kernels.size();
    AsmSectionId thisSection = sections.size();
    output.addEmptyKernel(kernelName, determineLLVMVersion());
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG, ELFSECTID_UNDEF, nullptr });
    kernelStates.push_back(new Kernel(thisSection));
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    inside = Inside::MAINLAYOUT;
    return thisKernel;
}

AsmSectionId AsmGalliumHandler::addSection(const char* sectionName, AsmKernelId kernelId)
{
    const AsmSectionId thisSection = sections.size();
    Section section;
    section.kernelId = ASMKERN_GLOBAL;  // we ignore input kernelId, we go to main
    
    if (::strcmp(sectionName, ".rodata") == 0)
    {
         // data (global data/ rodata) section
        if (dataSection!=ASMSECT_NONE)
            throw AsmFormatException("Only section '.rodata' can be in binary");
        dataSection = thisSection;
        section.type = AsmSectionType::DATA;
        section.elfBinSectId = ELFSECTID_RODATA;
        section.name = ".rodata"; // set static name (available by whole lifecycle)
    }
    else if (::strcmp(sectionName, ".text") == 0)
    {
         // code section
        if (codeSection!=ASMSECT_NONE)
            throw AsmFormatException("Only one section '.text' can be in binary");
        codeSection = thisSection;
        section.type = AsmSectionType::CODE;
        section.elfBinSectId = ELFSECTID_TEXT;
        section.name = ".text"; // set static name (available by whole lifecycle)
    }
    else if (::strcmp(sectionName, ".comment") == 0)
    {
         // comment section
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
    
    // sectiona are global, hence no kernel specified
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = thisSection;
    inside = Inside::MAINLAYOUT;
    return thisSection;
}

AsmSectionId AsmGalliumHandler::getSectionId(const char* sectionName) const
{
    if (::strcmp(sectionName, ".rodata") == 0) // data
        return dataSection;
    else if (::strcmp(sectionName, ".text") == 0) // code
        return codeSection;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        return commentSection;
    else
    {
        // if user extra section defined, just find
        SectionMap::const_iterator it = extraSectionMap.find(sectionName);
        if (it != extraSectionMap.end())
            return it->second;
    }
    return ASMSECT_NONE;
}

void AsmGalliumHandler::setCurrentKernel(AsmKernelId kernel)
{
    // set kernel and their default section
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

void AsmGalliumHandler::setCurrentSection(AsmSectionId sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentSection = sectionId;
    assembler.currentKernel = sections[sectionId].kernelId;
    inside = Inside::MAINLAYOUT;
}

AsmFormatHandler::SectionInfo AsmGalliumHandler::getSectionInfo(
                            AsmSectionId sectionId) const
{
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    
    AsmFormatHandler::SectionInfo info;
    info.type = sections[sectionId].type;
    info.flags = 0;
    // code is addressable and writeable
    if (info.type == AsmSectionType::CODE)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
    else if (info.type == AsmSectionType::GALLIUM_SCRATCH)
        // scratch is unresolvable (for relocation)
        info.flags = ASMSECT_UNRESOLVABLE;
    // any other section (except config) are absolute addressable and writeable
    else if (info.type != AsmSectionType::CONFIG)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    
    info.name = sections[sectionId].name;
    return info;
}

bool AsmGalliumHandler::isCodeSection() const
{
    return sections[assembler.currentSection].type == AsmSectionType::CODE;
}

AsmKcodeHandler::KernelBase& AsmGalliumHandler::getKernelBase(AsmKernelId index)
{ return *kernelStates[index]; }

size_t AsmGalliumHandler::getKernelsNum() const
{ return kernelStates.size(); }

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

void AsmGalliumPseudoOps::setArchMinor(AsmGalliumHandler& handler, const char* linePtr)
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
    handler.archMinor = value;
}

void AsmGalliumPseudoOps::setArchStepping(AsmGalliumHandler& handler, const char* linePtr)
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
    handler.archStepping = value;
}

// internal routine to set driver version to symbol
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

// internal routine to set LLVM version to symbol
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
    cxuint driverVersion = 0;
    if (getLLVMVersion)
        driverVersion = handler.determineLLVMVersion();
    else
        driverVersion = handler.determineDriverVersion();
    
    AsmParseUtils::setSymbolValue(handler.assembler, linePtr, driverVersion, ASMSECT_ABS);
}

void AsmGalliumPseudoOps::doConfig(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Configuration outside kernel definition")
    if (handler.kernelStates[asmr.currentKernel]->hasProgInfo)
        PSEUDOOP_RETURN_BY_ERROR("Configuration can't be defined if progInfo was defined")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.inside = AsmGalliumHandler::Inside::CONFIG;
    handler.output.kernels[asmr.currentKernel].useConfig = true;
    if (handler.determineLLVMVersion() >= 40000U) // HSA since LLVM 4.0
        handler.kernelStates[asmr.currentKernel]->initializeAmdHsaKernelConfig();
}

void AsmGalliumPseudoOps::doControlDirective(AsmGalliumHandler& handler,
              const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
        PSEUDOOP_RETURN_BY_ERROR("Kernel control directive can be defined "
                    "only inside kernel")
    if (handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmGalliumHandler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.ctrlDirSection == ASMSECT_NONE)
    {
        // create control directive if not exists
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel,
            AsmSectionType::GALLIUM_CONFIG_CTRL_DIRECTIVE,
            ELFSECTID_UNDEF, nullptr });
        kernel.ctrlDirSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, kernel.ctrlDirSection);
    handler.kernelStates[asmr.currentKernel]->initializeAmdHsaKernelConfig();
}

void AsmGalliumPseudoOps::doGlobalData(AsmGalliumHandler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    // go to global data (named in ELF as '.rodata')
    asmr.goToSection(pseudoOpPlace, ".rodata");
}

void AsmGalliumPseudoOps::setDimensions(AsmGalliumHandler& handler,
            const char* pseudoOpPlace, const char* linePtr, bool amdHsa)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (amdHsa && handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
        
    cxuint dimMask = 0;
    if (!parseDimensions(asmr, linePtr, dimMask, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (!amdHsa)
        handler.output.kernels[asmr.currentKernel].config.dimMask = dimMask;
    else
    {    // AMD HSA
        AsmAmdHsaKernelConfig& config =
                *(handler.kernelStates[asmr.currentKernel]->hsaConfig);
        config.dimMask = dimMask;
    }
}

void AsmGalliumPseudoOps::setConfigValue(AsmGalliumHandler& handler,
         const char* pseudoOpPlace, const char* linePtr, GalliumConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (target >= GALLIUMCVAL_HSA_FIRST_PARAM && handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
        
    if ((target == GALLIUMCVAL_SPILLEDSGPRS ||  target == GALLIUMCVAL_SPILLEDSGPRS) &&
        handler.determineLLVMVersion() < 30900U)
        PSEUDOOP_RETURN_BY_ERROR("Spilled VGPRs and SGPRs only for LLVM>=3.9.0");
    
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
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
                if (value > maxSGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Used SGPRs number out of range (0-%u)", maxSGPRsNum);
                    ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
                }
                break;
            }
            case GALLIUMCVAL_VGPRSNUM:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxVGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_VGPR, 0);
                if (value > maxVGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Used VGPRs number out of range (0-%u)", maxVGPRsNum);
                    ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
                }
                break;
            }
            case GALLIUMCVAL_EXCEPTIONS:
                asmr.printWarningForRange(7, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0x7f;
                break;
            case GALLIUMCVAL_FLOATMODE:
                asmr.printWarningForRange(8, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0xff;
                break;
            case GALLIUMCVAL_PRIORITY:
                asmr.printWarningForRange(2, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 3;
                break;
            case GALLIUMCVAL_LOCALSIZE:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                const cxuint maxLocalSize = getGPUMaxLocalSize(arch);
                if (value > maxLocalSize)
                {
                    char buf[64];
                    snprintf(buf, 64, "LocalSize out of range (0-%u)", maxLocalSize);
                    ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
                }
                break;
            }
            case GALLIUMCVAL_USERDATANUM:
                if (value > 16)
                    ASM_NOTGOOD_BY_ERROR(valuePlace, "UserDataNum out of range (0-16)")
                break;
            case GALLIUMCVAL_PGMRSRC1:
            case GALLIUMCVAL_PGMRSRC2:
                asmr.printWarningForRange(32, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                break;
            default:
                break;
        }
        
        // if HSA config parameter, use ROCm routine to check config value
        if (good && target >= GALLIUMCVAL_HSA_FIRST_PARAM)
            good = AsmROCmPseudoOps::checkConfigValue(asmr, valuePlace,
                ROCmConfigValueTarget(target-GALLIUMCVAL_HSA_FIRST_PARAM), value);
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
    
    if (target >= GALLIUMCVAL_HSA_FIRST_PARAM)
    {
        // if this is HSA config parameter, use ROCm routine set this parameter
        AsmAmdHsaKernelConfig& config = *(
                    handler.kernelStates[asmr.currentKernel]->hsaConfig);
        
        AsmROCmPseudoOps::setConfigValueMain(config,
                ROCmConfigValueTarget(target-GALLIUMCVAL_HSA_FIRST_PARAM), value);
    }
}

void AsmGalliumPseudoOps::setConfigBoolValue(AsmGalliumHandler& handler,
         const char* pseudoOpPlace, const char* linePtr, GalliumConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (target >= GALLIUMCVAL_HSA_FIRST_PARAM && handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
    
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
    
    if (target >= GALLIUMCVAL_HSA_FIRST_PARAM)
    {
        // if this is HSA config parameter, use ROCm routine set this parameter
        AsmAmdHsaKernelConfig& config =
                *(handler.kernelStates[asmr.currentKernel]->hsaConfig);
        
        AsmROCmPseudoOps::setConfigBoolValueMain(config,
                    ROCmConfigValueTarget(target-GALLIUMCVAL_HSA_FIRST_PARAM));
    }
}

void AsmGalliumPseudoOps::setDefaultHSAFeatures(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmAmdHsaKernelConfig* config =
            handler.kernelStates[asmr.currentKernel]->hsaConfig.get();
    config->enableSgprRegisterFlags =
                    uint16_t(AMDHSAFLAG_USE_PRIVATE_SEGMENT_BUFFER|
                        AMDHSAFLAG_USE_DISPATCH_PTR|AMDHSAFLAG_USE_KERNARG_SEGMENT_PTR);
    config->enableFeatureFlags = uint16_t(AMDHSAFLAG_USE_PTR64|2);
}

// set machine - four numbers - kind, major, minor, stepping
void AsmGalliumPseudoOps::setMachine(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
    
    uint16_t kindValue = 0, majorValue = 0;
    uint16_t minorValue = 0, steppingValue = 0;
    // use ROCM routine to parse machine
    if (!AsmROCmPseudoOps::parseMachine(asmr, linePtr, kindValue,
                    majorValue, minorValue, steppingValue))
        return;
    
    AsmAmdHsaKernelConfig* config =
            handler.kernelStates[asmr.currentKernel]->hsaConfig.get();
    config->amdMachineKind = kindValue;
    config->amdMachineMajor = majorValue;
    config->amdMachineMinor = minorValue;
    config->amdMachineStepping = steppingValue;
}

// two numbers - major and minor
void AsmGalliumPseudoOps::setCodeVersion(AsmGalliumHandler& handler,
                const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
    
    uint16_t majorValue = 0, minorValue = 0;
    // use ROCM routine to parse code version
    if (!AsmROCmPseudoOps::parseCodeVersion(asmr, linePtr, majorValue, minorValue))
        return;
    
    AsmAmdHsaKernelConfig* config =
            handler.kernelStates[asmr.currentKernel]->hsaConfig.get();
    config->amdCodeVersionMajor = majorValue;
    config->amdCodeVersionMinor = minorValue;
}

/// set reserved S/VGRPS - first number is first register, second is last register
void AsmGalliumPseudoOps::setReservedXgprs(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr, bool inVgpr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
    
    uint16_t gprFirst = 0, gprCount = 0;
    // use ROCM routine to parse reserved registers
    if (!AsmROCmPseudoOps::parseReservedXgprs(asmr, linePtr, inVgpr, gprFirst, gprCount))
        return;
    
    AsmAmdHsaKernelConfig* config =
            handler.kernelStates[asmr.currentKernel]->hsaConfig.get();
    if (inVgpr)
    {
        config->reservedVgprFirst = gprFirst;
        config->reservedVgprCount = gprCount;
    }
    else
    {
        config->reservedSgprFirst = gprFirst;
        config->reservedSgprCount = gprCount;
    }
}

// set UseGridWorkGroupCount - 3 bits for dimensions
void AsmGalliumPseudoOps::setUseGridWorkGroupCount(AsmGalliumHandler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (handler.determineLLVMVersion() < 40000U)
        PSEUDOOP_RETURN_BY_ERROR("HSA configuration pseudo-op only for LLVM>=4.0.0")
    
    cxuint dimMask = 0;
    if (!parseDimensions(asmr, linePtr, dimMask))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    uint16_t& flags = handler.kernelStates[asmr.currentKernel]->hsaConfig->
                enableSgprRegisterFlags;
    flags = (flags & ~(7<<AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_BIT)) |
            (dimMask<<AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_BIT);
}

void AsmGalliumPseudoOps::doArgs(AsmGalliumHandler& handler,
               const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Arguments outside kernel definition")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    // go to args (set that is inside ARGS)
    handler.inside = AsmGalliumHandler::Inside::ARGS;
}

// Gallium argument type map
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

// Gallium argument's semantic map
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
        PSEUDOOP_RETURN_BY_ERROR("Argument definition outside kernel configuration")
    if (handler.inside != AsmGalliumHandler::Inside::ARGS)
        PSEUDOOP_RETURN_BY_ERROR("Argument definition outside arguments list")
    
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
        {
            // shortcur for grid dimension
            argSemantic = GalliumArgSemantic::GRID_DIMENSION;
            argType = GalliumArgType::SCALAR;
            semanticDefined = true;
        }
        else if (::strcmp(name, "gridoffset")==0)
        {
            // shortcut for grid dimension
            argSemantic = GalliumArgSemantic::GRID_OFFSET;
            argType = GalliumArgType::SCALAR;
            semanticDefined = true;
        }
        else
        {
            // standard argument type name (without shortcuts)
            cxuint index = binaryMapFind(galliumArgTypesMap, galliumArgTypesMap + 9,
                         name, CStringLess()) - galliumArgTypesMap;
            if (index != 9) // end of this map
                argType = galliumArgTypesMap[index].second;
            else
                ASM_NOTGOOD_BY_ERROR(nameStringPlace, "Unknown argument type")
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
    
    // accept only 32-bit size of argument
    if (size > UINT32_MAX || size == 0)
        asmr.printWarning(sizeStrPlace, "Size of argument out of range");
    
    // align target size to dword
    uint64_t targetSize = (size+3ULL)&~3ULL;
    // by default set alignment over target size
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
        // parse target size
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
            // target alignment
            if (getAbsoluteValueArg(asmr, targetAlign, linePtr, false))
            {
                if (targetAlign > UINT32_MAX || targetAlign == 0)
                    asmr.printWarning(targetAlignPlace,
                                      "Target alignment of argument out of range");
                if (targetAlign==0 || targetAlign != (1ULL<<(63-CLZ64(targetAlign))))
                    ASM_NOTGOOD_BY_ERROR(targetAlignPlace,
                                    "Target alignment is not power of 2")
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
                        ASM_NOTGOOD_BY_ERROR(numExtPlace, "Unknown numeric extension")
                }
                else
                    good = false;
                
                if (!semanticDefined)
                {
                    /// if semantic has not been defined in the first argument
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
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("ProgInfo outside kernel definition")
    if (handler.output.kernels[asmr.currentKernel].useConfig)
        PSEUDOOP_RETURN_BY_ERROR("ProgInfo can't be defined if configuration was exists")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // set that is in proginfo
    handler.inside = AsmGalliumHandler::Inside::PROGINFO;
    handler.kernelStates[asmr.currentKernel]->hasProgInfo = true;
}

void AsmGalliumPseudoOps::doEntry(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("ProgInfo entry outside kernel configuration")
    if (handler.inside != AsmGalliumHandler::Inside::PROGINFO)
        PSEUDOOP_RETURN_BY_ERROR("ProgInfo entry definition outside ProgInfo")
    
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
    // version earlier than 3.9 accept only 3 prog info entries
    if (llvmVersion<30900U && kstate.progInfoEntries == 3)
        PSEUDOOP_RETURN_BY_ERROR("Maximum 3 entries can be in ProgInfo")
        // version 3.9 or later, accepts only 3 prog info entries
    if (llvmVersion>=30900U && kstate.progInfoEntries == 5)
        PSEUDOOP_RETURN_BY_ERROR("Maximum 5 entries can be in ProgInfo")
    GalliumProgInfoEntry& pentry = handler.output.kernels[asmr.currentKernel]
            .progInfo[kstate.progInfoEntries++];
    pentry.address = entryAddr;
    pentry.value = entryVal;
}

void AsmGalliumPseudoOps::scratchSymbol(AsmGalliumHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (handler.scratchSection == ASMSECT_NONE)
    {
        // add scratch section if not added
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_GLOBAL,
                AsmSectionType::GALLIUM_SCRATCH, ELFSECTID_UNDEF, nullptr });
        handler.scratchSection = thisSection;
        asmr.sections.push_back({ "", ASMKERN_GLOBAL, AsmSectionType::GALLIUM_SCRATCH,
            ASMSECT_UNRESOLVABLE, 0 });
    }
    
    AsmParseUtils::setSymbolValue(asmr, linePtr, 0, handler.scratchSection);
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
        case GALLIUMOP_ARCH_MINOR:
            AsmGalliumPseudoOps::setArchMinor(*this, linePtr);
            break;
        case GALLIUMOP_ARCH_STEPPING:
            AsmGalliumPseudoOps::setArchStepping(*this, linePtr);
            break;
        case GALLIUMOP_ARG:
            AsmGalliumPseudoOps::doArg(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_ARGS:
            AsmGalliumPseudoOps::doArgs(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_CALL_CONVENTION:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_CALL_CONVENTION);
            break;
        case GALLIUMOP_CODEVERSION:
            AsmGalliumPseudoOps::setCodeVersion(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_CONFIG:
            AsmGalliumPseudoOps::doConfig(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_CONTROL_DIRECTIVE:
            AsmGalliumPseudoOps::doControlDirective(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR);
            break;
        case GALLIUMOP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                         GALLIUMCVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR);
            break;
        case GALLIUMOP_DEBUGMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_DEBUGMODE);
            break;
        case GALLIUMOP_DEFAULT_HSA_FEATURES:
            AsmGalliumPseudoOps::setDefaultHSAFeatures(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_DIMS:
            AsmGalliumPseudoOps::setDimensions(*this, stmtPlace, linePtr, false);
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
        case GALLIUMOP_GDS_SEGMENT_SIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_GDS_SEGMENT_SIZE);
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
        case GALLIUMOP_GROUP_SEGMENT_ALIGN:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_GROUP_SEGMENT_ALIGN);
            break;
        case GALLIUMOP_HSA_DEBUGMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_HSA_DEBUGMODE);
            break;
        case GALLIUMOP_HSA_DIMS:
            AsmGalliumPseudoOps::setDimensions(*this, stmtPlace, linePtr, true);
            break;
        case GALLIUMOP_HSA_DX10CLAMP:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_HSA_DX10CLAMP);
            break;
        case GALLIUMOP_HSA_EXCEPTIONS:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_HSA_EXCEPTIONS);
            break;
        case GALLIUMOP_HSA_FLOATMODE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_HSA_FLOATMODE);
            break;
        case GALLIUMOP_HSA_IEEEMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_HSA_IEEEMODE);
            break;
        case GALLIUMOP_HSA_LOCALSIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE);
            break;
        case GALLIUMOP_HSA_PGMRSRC1:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_HSA_PGMRSRC1);
            break;
        case GALLIUMOP_HSA_PGMRSRC2:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_HSA_PGMRSRC2);
            break;
        case GALLIUMOP_HSA_PRIORITY:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_HSA_PRIORITY);
            break;
        case GALLIUMOP_HSA_PRIVMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_HSA_PRIVMODE);
            break;
        case GALLIUMOP_HSA_SCRATCHBUFFER:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE);
            break;
        case GALLIUMOP_HSA_SGPRSNUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_HSA_SGPRSNUM);
            break;
        case GALLIUMOP_HSA_TGSIZE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_HSA_TGSIZE);
            break;
        case GALLIUMOP_HSA_USERDATANUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_HSA_USERDATANUM);
            break;
        case GALLIUMOP_HSA_VGPRSNUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_HSA_VGPRSNUM);
            break;
        case GALLIUMOP_IEEEMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_IEEEMODE);
            break;
        case GALLIUMOP_KCODE:
            AsmKcodePseudoOps::doKCode(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_KCODEEND:
            AsmKcodePseudoOps::doKCodeEnd(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_KERNARG_SEGMENT_ALIGN:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_KERNARG_SEGMENT_ALIGN);
            break;
        case GALLIUMOP_KERNARG_SEGMENT_SIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_KERNARG_SEGMENT_SIZE);
            break;
        case GALLIUMOP_KERNEL_CODE_ENTRY_OFFSET:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_KERNEL_CODE_ENTRY_OFFSET);
            break;
        case GALLIUMOP_KERNEL_CODE_PREFETCH_OFFSET:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_KERNEL_CODE_PREFETCH_OFFSET);
            break;
        case GALLIUMOP_KERNEL_CODE_PREFETCH_SIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_KERNEL_CODE_PREFETCH_SIZE);
            break;
        case GALLIUMOP_LLVM_VERSION:
            AsmGalliumPseudoOps::setLLVMVersion(*this, linePtr);
            break;
        case GALLIUMOP_LOCALSIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_LOCALSIZE);
            break;
        case GALLIUMOP_MACHINE:
            AsmGalliumPseudoOps::setMachine(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_MAX_SCRATCH_BACKING_MEMORY:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_MAX_SCRATCH_BACKING_MEMORY);
            break;
        case GALLIUMOP_PRIORITY:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_PRIORITY);
            break;
        case GALLIUMOP_PRIVATE_ELEM_SIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_PRIVATE_ELEM_SIZE);
            break;
        case GALLIUMOP_PRIVATE_SEGMENT_ALIGN:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_PRIVATE_SEGMENT_ALIGN);
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
        case GALLIUMOP_RESERVED_SGPRS:
            AsmGalliumPseudoOps::setReservedXgprs(*this, stmtPlace, linePtr, false);
            break;
        case GALLIUMOP_RESERVED_VGPRS:
            AsmGalliumPseudoOps::setReservedXgprs(*this, stmtPlace, linePtr, true);
            break;
        case GALLIUMOP_RUNTIME_LOADER_KERNEL_SYMBOL:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_RUNTIME_LOADER_KERNEL_SYMBOL);
            break;
        case GALLIUMOP_SCRATCHBUFFER:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_SCRATCHBUFFER);
            break;
        case GALLIUMOP_SCRATCHSYM:
            AsmGalliumPseudoOps::scratchSymbol(*this, linePtr);
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
        case GALLIUMOP_USE_DEBUG_ENABLED:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_DEBUG_ENABLED);
            break;
        case GALLIUMOP_USE_DISPATCH_ID:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_DISPATCH_ID);
            break;
        case GALLIUMOP_USE_DISPATCH_PTR:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_DISPATCH_PTR);
            break;
        case GALLIUMOP_USE_DYNAMIC_CALL_STACK:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_DYNAMIC_CALL_STACK);
            break;
        case GALLIUMOP_USE_FLAT_SCRATCH_INIT:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_FLAT_SCRATCH_INIT);
            break;
        case GALLIUMOP_USE_GRID_WORKGROUP_COUNT:
            AsmGalliumPseudoOps::setUseGridWorkGroupCount(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_USE_KERNARG_SEGMENT_PTR:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_KERNARG_SEGMENT_PTR);
            break;
        case GALLIUMOP_USE_ORDERED_APPEND_GDS:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_ORDERED_APPEND_GDS);
            break;
        case GALLIUMOP_USE_PRIVATE_SEGMENT_SIZE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_PRIVATE_SEGMENT_SIZE);
            break;
        case GALLIUMOP_USE_PTR64:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_PTR64);
            break;
        case GALLIUMOP_USE_QUEUE_PTR:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_QUEUE_PTR);
            break;
        case GALLIUMOP_USE_PRIVATE_SEGMENT_BUFFER:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_PRIVATE_SEGMENT_BUFFER);
            break;
        case GALLIUMOP_USE_XNACK_ENABLED:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_USE_XNACK_ENABLED);
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
        case GALLIUMOP_WAVEFRONT_SGPR_COUNT:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_WAVEFRONT_SGPR_COUNT);
            break;
        case GALLIUMOP_WAVEFRONT_SIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_WAVEFRONT_SIZE);
            break;
        case GALLIUMOP_WORKITEM_VGPR_COUNT:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_WORKITEM_VGPR_COUNT);
            break;
        case GALLIUMOP_WORKGROUP_FBARRIER_COUNT:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_WORKGROUP_FBARRIER_COUNT);
            break;
        case GALLIUMOP_WORKGROUP_GROUP_SEGMENT_SIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE);
            break;
        case GALLIUMOP_WORKITEM_PRIVATE_SEGMENT_SIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             GALLIUMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE);
            break;
        default:
            return false;
    }
    return true;
}

bool AsmGalliumHandler::resolveSymbol(const AsmSymbol& symbol,
                    uint64_t& value, AsmSectionId& sectionId)
{
    if (!assembler.isResolvableSection(symbol.sectionId))
    {
        value = symbol.value;
        sectionId = symbol.sectionId;
        return true;
    }
    return false;
}

bool AsmGalliumHandler::resolveRelocation(const AsmExpression* expr, uint64_t& outValue,
                    AsmSectionId& outSectionId)
{
    RelocType relType = RELTYPE_LOW_32BIT;
    AsmSectionId relSectionId = 0;
    uint64_t relValue = 0;
    const AsmExprTarget& target = expr->getTarget();
    const AsmExprTargetType tgtType = target.type;
    if (!resolveLoHiRelocExpression(expr, relType, relSectionId, relValue))
        return false;
    
    // relocation only for rodata, data and bss section
    if (relSectionId!=scratchSection)
    {
        assembler.printError(expr->getSourcePos(),
                    "Section of this expression must be a scratch");
        return false;
    }
    if (relValue != 0)
    {
        assembler.printError(expr->getSourcePos(),
                    "Expression must point to start of section");
        return false;
    }
    outSectionId = ASMSECT_ABS;   // for filling values in code
    outValue = 4U; // for filling values in code
    size_t extraOffset = (tgtType!=ASMXTGT_DATA32) ? 4 : 0;
    AsmRelocation reloc = { target.sectionId, target.offset+extraOffset, relType };
    // set up relocation (relSectionId, addend)
    reloc.relSectionId = relSectionId;
    reloc.addend = 0;
    assembler.relocations.push_back(reloc);
    return true;;
}

bool AsmGalliumHandler::prepareBinary()
{
    // before call we initialize pointers and datas
    bool good = true;
    
    output.is64BitElf = assembler.is64Bit();
    size_t sectionsNum = sections.size();
    size_t kernelsNum = kernelStates.size();
    output.deviceType = assembler.getDeviceType();
    prepareKcodeState();
    
    // set sections as outputs
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
                // handle extra (user) section, set section type and its flags
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
            case AsmSectionType::GALLIUM_CONFIG_CTRL_DIRECTIVE:
                if (sectionSize != 128)
                    // control directive accepts only 128-byte size
                    assembler.printError(AsmSourcePos(),
                         (std::string("Section '.control_directive' for kernel '")+
                          assembler.kernels[section.kernelId].name+
                          "' have wrong size").c_str());
                break;
            default: // ignore other sections
                break;
        }
    }
    
    // get current llvm version (if not TESTRUN)
    cxuint llvmVersion = assembler.llvmVersion;
    if (llvmVersion == 0 && (assembler.flags&ASM_TESTRUN)==0)
        llvmVersion = detectedLLVMVersion;
    
    GPUArchitecture arch = getGPUArchitectureFromDeviceType(assembler.deviceType);
    // set up number of the allocated SGPRs and VGPRs for kernel
    cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    for (size_t i = 0; i < kernelsNum; i++)
    {
        if (!output.kernels[i].useConfig)
            continue;
        GalliumKernelConfig& config = output.kernels[i].config;
        cxuint userSGPRsNum = config.userDataNum;
        
        if (llvmVersion >= 40000U && config.userDataNum == BINGEN8_DEFAULT)
        {
            // fixed userdatanum for LLVM 4.0
            const AmdHsaKernelConfig& hsaConfig  = *kernelStates[i]->hsaConfig.get();
            // calculate userSGPRs
            const uint16_t sgprFlags = hsaConfig.enableSgprRegisterFlags;
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
            config.userDataNum = userSGPRsNum;
        }
        
        /* include userData sgprs */
        cxuint dimMask = (config.dimMask!=BINGEN_DEFAULT) ? config.dimMask :
                getDefaultDimMask(arch, config.pgmRSRC2);
        // extra sgprs for dimensions
        cxuint minRegsNum[2];
        getGPUSetupMinRegistersNum(arch, dimMask, userSGPRsNum,
                   ((config.tgSize) ? GPUSETUP_TGSIZE_EN : 0) |
                   ((config.scratchBufferSize!=0) ? GPUSETUP_SCRATCH_EN : 0), minRegsNum);
        
        if (config.usedSGPRsNum!=BINGEN_DEFAULT && maxSGPRsNum < config.usedSGPRsNum)
        {
            // check only if sgprsnum set explicitly
            char numBuf[64];
            snprintf(numBuf, 64, "(max %u)", maxSGPRsNum);
            assembler.printError(assembler.kernels[i].sourcePos, (std::string(
                    "Number of total SGPRs for kernel '")+
                    output.kernels[i].kernelName.c_str()+"' is too high "+numBuf).c_str());
            good = false;
        }
        
        if (config.usedSGPRsNum==BINGEN_DEFAULT)
        {
            cxuint allocFlags = kernelStates[i]->allocRegFlags;
            if (llvmVersion >= 40000U)
            {
                // fix alloc reg flags for AMD HSA (such as ROCm)
                const AmdHsaKernelConfig& hsaConfig  = *kernelStates[i]->hsaConfig.get();
                allocFlags = kernelStates[i]->allocRegFlags |
                    // flat_scratch_init
                    ((hsaConfig.enableSgprRegisterFlags&
                            AMDHSAFLAG_USE_FLAT_SCRATCH_INIT)!=0? GCN_FLAT : 0) |
                    // enable_xnack
                    ((hsaConfig.enableFeatureFlags&AMDHSAFLAG_USE_XNACK_ENABLED)!=0 ?
                                GCN_XNACK : 0);
            }
            
            config.usedSGPRsNum = std::min(
                std::max(minRegsNum[0], kernelStates[i]->allocRegs[0]) +
                    getGPUExtraRegsNum(arch, REGTYPE_SGPR, allocFlags|GCN_VCC),
                    maxSGPRsNum); // include all extra sgprs
        }
        if (config.usedVGPRsNum==BINGEN_DEFAULT)
            config.usedVGPRsNum = std::max(minRegsNum[1], kernelStates[i]->allocRegs[1]);
    }
    
    // put scratch relocations
    for (const AsmRelocation& reloc: assembler.relocations)
        output.scratchRelocs.push_back({reloc.offset, reloc.type});
    
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
            AsmSectionId binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            
            output.extraSymbols.push_back({ symEntry.first, symEntry.second.value,
                    symEntry.second.size, binSectId, false, symEntry.second.info,
                    symEntry.second.other });
        }
    
    // setup amd GPU arch values (for LLVM 4.0 HSA config)
    AMDGPUArchVersion amdGpuArchValues = getGPUArchVersion(assembler.deviceType,
                                GPUArchVersionTable::OPENSOURCE);
    // replace arch minor and stepping by user defined values (if set)
    if (archMinor != BINGEN_DEFAULT)
        amdGpuArchValues.minor = archMinor;
    if (archStepping != BINGEN_DEFAULT)
        amdGpuArchValues.stepping = archStepping;
    /// checking symbols and set offset for kernels
    AsmSection& asmCSection = assembler.sections[codeSection];
    const AsmSymbolMap& symbolMap = assembler.getSymbolMap();
    
    for (size_t ki = 0; ki < output.kernels.size(); ki++)
    {
        GalliumKernelInput& kinput = output.kernels[ki];
        auto it = symbolMap.find(kinput.kernelName);
        if (it == symbolMap.end() || !it->second.isDefined())
        {
            // error, undefined
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                        "Symbol for kernel '")+kinput.kernelName.c_str()+
                        "' is undefined").c_str());
            good = false;
            continue;
        }
        const AsmSymbol& symbol = it->second;
        if (!symbol.hasValue)
        {
            // error, unresolved
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '") + kinput.kernelName.c_str() +
                    "' is not resolved").c_str());
            good = false;
            continue;
        }
        if (symbol.sectionId != codeSection)
        {
            /// error, wrong section
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '")+kinput.kernelName.c_str()+
                    "' is defined for section other than '.text'").c_str());
            good = false;
            continue;
        }
        kinput.offset = symbol.value;
        // set up AMD HSA configuration in kernel code
        if (llvmVersion >= 40000U && output.kernels[ki].useConfig)
        {
            // requires amdhsa-gcn (with HSA header)
            // hotfix
            GalliumKernelConfig config = output.kernels[ki].config;
            // outConfig is final HSA config
            AmdHsaKernelConfig outConfig;
            ::memset(&outConfig, 0xff, 128); // fill by defaults
            
            const Kernel& kernel = *kernelStates[ki];
            if (kernel.hsaConfig != nullptr)
            {
                // replace Gallium config by HSA config
                ::memcpy(&outConfig, kernel.hsaConfig.get(), sizeof(AmdHsaKernelConfig));
                // set config from HSA config
                const AsmAmdHsaKernelConfig& hsaConfig = *kernel.hsaConfig.get();
                if (hsaConfig.usedSGPRsNum != BINGEN_DEFAULT)
                    config.usedSGPRsNum = hsaConfig.usedSGPRsNum;
                if (hsaConfig.usedVGPRsNum != BINGEN_DEFAULT)
                    config.usedVGPRsNum = hsaConfig.usedVGPRsNum;
                if (hsaConfig.userDataNum != BINGEN8_DEFAULT)
                    config.userDataNum = hsaConfig.userDataNum;
                config.dimMask |= hsaConfig.dimMask;
                config.pgmRSRC1 |= hsaConfig.computePgmRsrc1;
                config.pgmRSRC2 |= hsaConfig.computePgmRsrc2;
                config.ieeeMode |= hsaConfig.ieeeMode;
                config.floatMode |= hsaConfig.floatMode;
                config.priority = std::max(hsaConfig.priority, config.priority);
                config.exceptions |= hsaConfig.exceptions;
                config.tgSize |= hsaConfig.tgSize;
                config.debugMode |= hsaConfig.debugMode;
                config.privilegedMode |= hsaConfig.privilegedMode;
                config.dx10Clamp |= hsaConfig.dx10Clamp;
                if (hsaConfig.workgroupGroupSegmentSize != BINGEN_DEFAULT) // local size
                    config.localSize = hsaConfig.workgroupGroupSegmentSize;
                if (hsaConfig.workitemPrivateSegmentSize != BINGEN_DEFAULT)
                    // scratch buffer
                    config.scratchBufferSize = hsaConfig.workitemPrivateSegmentSize;
            }
            // calculate pgmrsrcs
            cxuint sgprsNum = std::max(config.usedSGPRsNum, 1U);
            cxuint vgprsNum = std::max(config.usedVGPRsNum, 1U);
            uint32_t pgmRSRC1 =  (config.pgmRSRC1) |
                    calculatePgmRSrc1(arch, vgprsNum, sgprsNum,
                        config.priority, config.floatMode, config.privilegedMode,
                        config.dx10Clamp, config.debugMode, config.ieeeMode);
            
            uint32_t pgmRSRC2 = (config.pgmRSRC2 & 0xffffe440U) |
                    calculatePgmRSrc2(arch, (config.scratchBufferSize != 0),
                            config.userDataNum, false, config.dimMask,
                            (config.pgmRSRC2 & 0x1b80U), config.tgSize,
                            config.localSize, config.exceptions);
            
            // calculating kernel argument segment size
            size_t argSegmentSize = 0;
            for (const GalliumArgInfo& argInfo: output.kernels[ki].argInfos)
            {
                uint32_t targetAlign = argInfo.targetAlign;
                uint32_t targetSize = std::max(argInfo.size, argInfo.targetSize);
                if (argInfo.semantic == GalliumArgSemantic::GRID_DIMENSION ||
                        argInfo.semantic == GalliumArgSemantic::GRID_OFFSET)
                    // handle grid dimensions
                    targetSize = argInfo.semantic == GalliumArgSemantic::GRID_OFFSET ?
                            12 : targetSize;
                
                if (argInfo.targetAlign != 0)
                    argSegmentSize = (argSegmentSize + targetAlign-1) &
                            ~size_t(targetAlign-1);
                argSegmentSize += targetSize;
            }
            
            if (outConfig.amdCodeVersionMajor == BINGEN_DEFAULT)
                outConfig.amdCodeVersionMajor = 1;
            if (outConfig.amdCodeVersionMinor == BINGEN_DEFAULT)
                outConfig.amdCodeVersionMinor = 0;
            if (outConfig.amdMachineKind == BINGEN16_DEFAULT)
                outConfig.amdMachineKind = 1;
            if (outConfig.amdMachineMajor == BINGEN16_DEFAULT)
                outConfig.amdMachineMajor = amdGpuArchValues.major;
            if (outConfig.amdMachineMinor == BINGEN16_DEFAULT)
                outConfig.amdMachineMinor = amdGpuArchValues.minor;
            if (outConfig.amdMachineStepping == BINGEN16_DEFAULT)
                outConfig.amdMachineStepping = amdGpuArchValues.stepping;
            if (outConfig.kernelCodeEntryOffset == BINGEN64_DEFAULT)
                outConfig.kernelCodeEntryOffset = 256;
            if (outConfig.kernelCodePrefetchOffset == BINGEN64_DEFAULT)
                outConfig.kernelCodePrefetchOffset = 0;
            if (outConfig.kernelCodePrefetchSize == BINGEN64_DEFAULT)
                outConfig.kernelCodePrefetchSize = 0;
            if (outConfig.maxScrachBackingMemorySize == BINGEN64_DEFAULT) // ??
                outConfig.maxScrachBackingMemorySize = 0;
            outConfig.computePgmRsrc1 = pgmRSRC1;
            outConfig.computePgmRsrc2 = pgmRSRC2;
            if (outConfig.workitemPrivateSegmentSize == BINGEN_DEFAULT) // scratch buffer
                outConfig.workitemPrivateSegmentSize = config.scratchBufferSize;
            if (outConfig.workgroupGroupSegmentSize == BINGEN_DEFAULT)
                outConfig.workgroupGroupSegmentSize = config.localSize;
            if (outConfig.gdsSegmentSize == BINGEN_DEFAULT)
                outConfig.gdsSegmentSize = 0;
            if (outConfig.kernargSegmentSize == BINGEN64_DEFAULT)
                outConfig.kernargSegmentSize = argSegmentSize;
            if (outConfig.workgroupFbarrierCount == BINGEN_DEFAULT)
                outConfig.workgroupFbarrierCount = 0;
            if (outConfig.wavefrontSgprCount == BINGEN16_DEFAULT)
                outConfig.wavefrontSgprCount = config.usedSGPRsNum;
            if (outConfig.workitemVgprCount == BINGEN16_DEFAULT)
                outConfig.workitemVgprCount = config.usedVGPRsNum;
            if (outConfig.reservedVgprFirst == BINGEN16_DEFAULT)
                outConfig.reservedVgprFirst = 0;
            if (outConfig.reservedVgprCount == BINGEN16_DEFAULT)
                outConfig.reservedVgprCount = 0;
            if (outConfig.reservedSgprFirst == BINGEN16_DEFAULT)
                outConfig.reservedSgprFirst = 0;
            if (outConfig.reservedSgprCount == BINGEN16_DEFAULT)
                outConfig.reservedSgprCount = 0;
            if (outConfig.debugWavefrontPrivateSegmentOffsetSgpr == BINGEN16_DEFAULT)
                outConfig.debugWavefrontPrivateSegmentOffsetSgpr = 0;
            if (outConfig.debugPrivateSegmentBufferSgpr == BINGEN16_DEFAULT)
                outConfig.debugPrivateSegmentBufferSgpr = 0;
            if (outConfig.kernargSegmentAlignment == BINGEN8_DEFAULT)
                outConfig.kernargSegmentAlignment = 4;
            if (outConfig.groupSegmentAlignment == BINGEN8_DEFAULT)
                outConfig.groupSegmentAlignment = 4;
            if (outConfig.privateSegmentAlignment == BINGEN8_DEFAULT)
                outConfig.privateSegmentAlignment = 4;
            if (outConfig.wavefrontSize == BINGEN8_DEFAULT)
                outConfig.wavefrontSize = 6;
            outConfig.reserved1[0] = 0;
            outConfig.reserved1[1] = 0;
            outConfig.reserved1[2] = 0;
            if (outConfig.runtimeLoaderKernelSymbol == BINGEN64_DEFAULT)
                outConfig.runtimeLoaderKernelSymbol = 0;
            
            // to little endian
            outConfig.toLE(); // to little-endian
            // put control directive section to config
            if (kernel.ctrlDirSection!=ASMSECT_NONE &&
                assembler.sections[kernel.ctrlDirSection].content.size()==128)
                ::memcpy(outConfig.controlDirective, 
                    assembler.sections[kernel.ctrlDirSection].content.data(), 128);
            else
                ::memset(outConfig.controlDirective, 0, 128);
            
            if (asmCSection.content.size() >= symbol.value+256)
                // and store it to asm section in kernel place
                ::memcpy(asmCSection.content.data() + symbol.value,
                        &outConfig, sizeof(AmdHsaKernelConfig));
            else // if wrong size
                assembler.printError(AsmSourcePos(), (
                    std::string("HSA configuration for kernel '")+
                    kinput.kernelName.c_str()+"' out of content").c_str());
        }
    }
    // set versions
    if (assembler.driverVersion == 0 && (assembler.flags&ASM_TESTRUN)==0) // auto detection
        output.isMesa170 = detectedDriverVersion >= 170000U;
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
