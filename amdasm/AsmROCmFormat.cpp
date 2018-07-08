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
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include "AsmAmdInternals.h"
#include "AsmROCmInternals.h"

using namespace CLRX;

// all ROCm pseudo-op names (sorted)
static const char* rocmPseudoOpNamesTbl[] =
{
    "arch_minor", "arch_stepping", "arg",
    "call_convention", "codeversion", "config",
    "control_directive", "cws", "debug_private_segment_buffer_sgpr",
    "debug_wavefront_private_segment_offset_sgpr",
    "debugmode", "default_hsa_features", "dims", "dx10clamp",
    "eflags", "exceptions", "fixed_work_group_size",
    "fkernel", "floatmode", "gds_segment_size",
    "globaldata", "gotsym", "group_segment_align", "ieeemode", "kcode",
    "kcodeend", "kernarg_segment_align",
    "kernarg_segment_size", "kernel_code_entry_offset",
    "kernel_code_prefetch_offset", "kernel_code_prefetch_size",
    "localsize", "machine",
    "max_flat_work_group_size", "max_scratch_backing_memory",
    "md_group_segment_fixed_size", "md_kernarg_segment_align",
    "md_kernarg_segment_size", "md_language","md_private_segment_fixed_size",
    "md_sgprsnum", "md_symname", "md_version",
    "md_vgprsnum", "md_wavefront_size",
    "metadata", "newbinfmt", "nosectdiffs",
    "pgmrsrc1", "pgmrsrc2", "printf", "priority",
    "private_elem_size", "private_segment_align",
    "privmode", "reqd_work_group_size",
    "reserved_sgprs", "reserved_vgprs",
    "runtime_handle", "runtime_loader_kernel_symbol",
    "scratchbuffer", "sgprsnum",
    "spilledsgprs", "spilledvgprs", "target", "tgsize", "tripple",
    "use_debug_enabled", "use_dispatch_id",
    "use_dispatch_ptr", "use_dynamic_call_stack",
    "use_flat_scratch_init", "use_grid_workgroup_count",
    "use_kernarg_segment_ptr", "use_ordered_append_gds",
    "use_private_segment_buffer", "use_private_segment_size",
    "use_ptr64", "use_queue_ptr", "use_xnack_enabled",
    "userdatanum", "vectypehint", "vgprsnum", "wavefront_sgpr_count",
    "wavefront_size", "work_group_size_hint", "workgroup_fbarrier_count",
    "workgroup_group_segment_size", "workitem_private_segment_size",
    "workitem_vgpr_count"
};

// all enums for ROCm pseudo-ops
enum
{
    ROCMOP_ARCH_MINOR, ROCMOP_ARCH_STEPPING, ROCMOP_ARG,
    ROCMOP_CALL_CONVENTION, ROCMOP_CODEVERSION, ROCMOP_CONFIG,
    ROCMOP_CONTROL_DIRECTIVE, ROCMOP_CWS, ROCMOP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    ROCMOP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    ROCMOP_DEBUGMODE, ROCMOP_DEFAULT_HSA_FEATURES, ROCMOP_DIMS, ROCMOP_DX10CLAMP,
    ROCMOP_EFLAGS, ROCMOP_EXCEPTIONS, ROCMOP_FIXED_WORK_GROUP_SIZE, ROCMOP_FKERNEL,
    ROCMOP_FLOATMODE, ROCMOP_GDS_SEGMENT_SIZE, ROCMOP_GLOBALDATA, ROCMOP_GOTSYM,
    ROCMOP_GROUP_SEGMENT_ALIGN, ROCMOP_IEEEMODE, ROCMOP_KCODE,
    ROCMOP_KCODEEND, ROCMOP_KERNARG_SEGMENT_ALIGN,
    ROCMOP_KERNARG_SEGMENT_SIZE, ROCMOP_KERNEL_CODE_ENTRY_OFFSET,
    ROCMOP_KERNEL_CODE_PREFETCH_OFFSET, ROCMOP_KERNEL_CODE_PREFETCH_SIZE,
    ROCMOP_LOCALSIZE, ROCMOP_MACHINE,
    ROCMOP_MAX_FLAT_WORK_GROUP_SIZE, ROCMOP_MAX_SCRATCH_BACKING_MEMORY,
    ROCMOP_MD_GROUP_SEGMENT_FIXED_SIZE, ROCMOP_MD_KERNARG_SEGMENT_ALIGN,
    ROCMOP_MD_KERNARG_SEGMENT_SIZE, ROCMOP_MD_LANGUAGE,
    ROCMOP_MD_PRIVATE_SEGMENT_FIXED_SIZE, ROCMOP_MD_SGPRSNUM,
    ROCMOP_MD_SYMNAME, ROCMOP_MD_VERSION, ROCMOP_MD_VGPRSNUM, ROCMOP_MD_WAVEFRONT_SIZE,
    ROCMOP_METADATA, ROCMOP_NEWBINFMT, ROCMOP_NOSECTDIFFS,
    ROCMOP_PGMRSRC1, ROCMOP_PGMRSRC2, ROCMOP_PRINTF,
    ROCMOP_PRIORITY, ROCMOP_PRIVATE_ELEM_SIZE, ROCMOP_PRIVATE_SEGMENT_ALIGN,
    ROCMOP_PRIVMODE, ROCMOP_REQD_WORK_GROUP_SIZE,
    ROCMOP_RESERVED_SGPRS, ROCMOP_RESERVED_VGPRS,
    ROCMOP_RUNTIME_HANDLE, ROCMOP_RUNTIME_LOADER_KERNEL_SYMBOL,
    ROCMOP_SCRATCHBUFFER, ROCMOP_SGPRSNUM, ROCMOP_SPILLEDSGPRS, ROCMOP_SPILLEDVGPRS,
    ROCMOP_TARGET, ROCMOP_TGSIZE, ROCMOP_TRIPPLE,
    ROCMOP_USE_DEBUG_ENABLED, ROCMOP_USE_DISPATCH_ID,
    ROCMOP_USE_DISPATCH_PTR, ROCMOP_USE_DYNAMIC_CALL_STACK,
    ROCMOP_USE_FLAT_SCRATCH_INIT, ROCMOP_USE_GRID_WORKGROUP_COUNT,
    ROCMOP_USE_KERNARG_SEGMENT_PTR, ROCMOP_USE_ORDERED_APPEND_GDS,
    ROCMOP_USE_PRIVATE_SEGMENT_BUFFER, ROCMOP_USE_PRIVATE_SEGMENT_SIZE,
    ROCMOP_USE_PTR64, ROCMOP_USE_QUEUE_PTR, ROCMOP_USE_XNACK_ENABLED,
    ROCMOP_USERDATANUM, ROCMOP_VECTYPEHINT, ROCMOP_VGPRSNUM, ROCMOP_WAVEFRONT_SGPR_COUNT,
    ROCMOP_WAVEFRONT_SIZE, ROCM_WORK_GROUP_SIZE_HINT, ROCMOP_WORKGROUP_FBARRIER_COUNT,
    ROCMOP_WORKGROUP_GROUP_SEGMENT_SIZE, ROCMOP_WORKITEM_PRIVATE_SEGMENT_SIZE,
    ROCMOP_WORKITEM_VGPR_COUNT
};

/*
 * ROCm format handler
 */

AsmROCmHandler::AsmROCmHandler(Assembler& assembler):
             AsmKcodeHandler(assembler), output{}, commentSection(ASMSECT_NONE),
             metadataSection(ASMSECT_NONE), dataSection(ASMSECT_NONE),
             gotSection(ASMSECT_NONE), extraSectionCount(0),
             prevSymbolsCount(0), unresolvedGlobals(false), good(true)
{
    codeSection = 0;
    sectionDiffsResolvable = true;
    output.newBinFormat = assembler.isNewROCmBinFormat();
    output.metadataInfo.initialize();
    output.archMinor = output.archStepping = UINT32_MAX;
    output.eflags = BINGEN_DEFAULT;
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    // add text section as first
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::CODE,
                ELFSECTID_TEXT, ".text" });
    savedSection = 0;
}

AsmROCmHandler::~AsmROCmHandler()
{
    for (Kernel* kernel: kernelStates)
        delete kernel;
}

AsmKernelId AsmROCmHandler::addKernel(const char* kernelName)
{
    AsmKernelId thisKernel = output.symbols.size();
    AsmSectionId thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG, ELFSECTID_UNDEF, nullptr });
    kernelStates.push_back(new Kernel(thisSection));
    output.metadataInfo.kernels.push_back(ROCmKernelMetadata());
    output.metadataInfo.kernels.back().initialize();
    output.metadataInfo.kernels.back().name = kernelName;
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    return thisKernel;
}

AsmSectionId AsmROCmHandler::addSection(const char* sectionName, AsmKernelId kernelId)
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
        section.type = AsmSectionType::ROCM_COMMENT;
        section.elfBinSectId = ELFSECTID_COMMENT;
        section.name = ".comment"; // set static name (available by whole lifecycle)
    }
    else
    {
        // extra (user) section
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

AsmSectionId AsmROCmHandler::getSectionId(const char* sectionName) const
{
    if (::strcmp(sectionName, ".rodata") == 0) // data
        return dataSection;
    else if (::strcmp(sectionName, ".text") == 0) // code
        return codeSection;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        return commentSection;
    else
    {
        // if extra section, then find it
        SectionMap::const_iterator it = extraSectionMap.find(sectionName);
        if (it != extraSectionMap.end())
            return it->second;
    }
    return ASMSECT_NONE;
}

void AsmROCmHandler::setCurrentKernel(AsmKernelId kernel)
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

void AsmROCmHandler::setCurrentSection(AsmSectionId sectionId)
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


AsmFormatHandler::SectionInfo AsmROCmHandler::getSectionInfo(AsmSectionId sectionId) const
{
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    
    AsmFormatHandler::SectionInfo info;
    info.type = sections[sectionId].type;
    info.flags = 0;
    // code is addressable and writeable
    if (info.type == AsmSectionType::CODE || info.type == AsmSectionType::DATA)
    {
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
        if (info.type == AsmSectionType::DATA && !sectionDiffsResolvable)
            info.flags |= ASMSECT_ABS_ADDRESSABLE;
    }
    else if (info.type == AsmSectionType::ROCM_GOT)
    {
        info.flags = ASMSECT_ADDRESSABLE;
        if (!sectionDiffsResolvable)
            info.flags |= ASMSECT_ABS_ADDRESSABLE;
    }
    // any other section (except config) are absolute addressable and writeable
    else if (info.type != AsmSectionType::CONFIG)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    
    if (sectionDiffsResolvable)
        if (info.type == AsmSectionType::CODE || info.type == AsmSectionType::DATA ||
            info.type == AsmSectionType::ROCM_GOT)
            info.relSpace = 0;  // first rel space
    
    info.name = sections[sectionId].name;
    return info;
}

bool AsmROCmHandler::isCodeSection() const
{
    return sections[assembler.currentSection].type == AsmSectionType::CODE;
}

AsmKcodeHandler::KernelBase& AsmROCmHandler::getKernelBase(AsmKernelId index)
{ return *kernelStates[index]; }

size_t AsmROCmHandler::getKernelsNum() const
{ return kernelStates.size(); }

void AsmROCmHandler::Kernel::initializeKernelConfig()
{
    if (!config)
    {
        config.reset(new AsmROCmKernelConfig{});
        config->initialize();
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

void AsmROCmPseudoOps::setEFlags(AsmROCmHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    const char* valuePlace = linePtr;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    asmr.printWarningForRange(sizeof(uint32_t)<<3, value,
                 asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.eflags = value;
}

void AsmROCmPseudoOps::setTarget(AsmROCmHandler& handler, const char* linePtr,
                        bool tripple)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (!asmr.parseString(out, linePtr))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (tripple)
        handler.output.targetTripple = out;
    else
        handler.output.target = out;
}

void AsmROCmPseudoOps::doConfig(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
        PSEUDOOP_RETURN_BY_ERROR("Kernel config can be defined only inside kernel")
    
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
        PSEUDOOP_RETURN_BY_ERROR("Kernel control directive can be defined "
                    "only inside kernel")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmROCmHandler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.ctrlDirSection == ASMSECT_NONE)
    {
        // define control directive section (if not exists)
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel,
            AsmSectionType::ROCM_CONFIG_CTRL_DIRECTIVE,
            ELFSECTID_UNDEF, nullptr });
        kernel.ctrlDirSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, kernel.ctrlDirSection);
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
}

void AsmROCmPseudoOps::doGlobalData(AsmROCmHandler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    // go to global data (named in ELF as '.rodata')
    asmr.goToSection(pseudoOpPlace, ".rodata");
}

void AsmROCmPseudoOps::setNewBinFormat(AsmROCmHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.newBinFormat = true;
}

void AsmROCmPseudoOps::addMetadata(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    
    if (handler.output.useMetadataInfo)
        PSEUDOOP_RETURN_BY_ERROR("Metadata can't be defined if metadata config "
                    "is already defined")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmSectionId& metadataSection = handler.metadataSection;
    if (metadataSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::ROCM_METADATA,
            ELFSECTID_UNDEF, nullptr });
        metadataSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, metadataSection);
}

void AsmROCmPseudoOps::doFKernel(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
        PSEUDOOP_RETURN_BY_ERROR(".fkernel can be only inside kernel")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    // set fkernel flag for kernel
    handler.kernelStates[asmr.currentKernel]->isFKernel = true;
}

void AsmROCmPseudoOps::setMetadataVersion(AsmROCmHandler& handler,
                const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    uint64_t mdVerMajor = 0, mdVerMinor = 0;
    skipSpacesToEnd(linePtr, end);
    // parse metadata major version
    const char* valuePlace = linePtr;
    bool good = true;
    if (getAbsoluteValueArg(asmr, mdVerMajor, linePtr, true))
        asmr.printWarningForRange(sizeof(cxuint)<<3, mdVerMajor,
                    asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    else
        good = false;
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    // parse metadata minor version
    skipSpacesToEnd(linePtr, end);
    valuePlace = linePtr;
    if (getAbsoluteValueArg(asmr, mdVerMinor, linePtr, true))
        asmr.printWarningForRange(sizeof(cxuint)<<3, mdVerMinor,
                    asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    else
        good = false;
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.output.useMetadataInfo = true;
    handler.output.metadataInfo.version[0] = mdVerMajor;
    handler.output.metadataInfo.version[1] = mdVerMinor;
}

void AsmROCmPseudoOps::setCWS(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t out[3] = { 1, 1, 1 };
    // parse CWS (1-3 values)
    if (!AsmAmdPseudoOps::parseCWS(asmr, pseudoOpPlace, linePtr, out))
        return;
    handler.output.useMetadataInfo = true;
    ROCmKernelMetadata& metadata = handler.output.metadataInfo.kernels[asmr.currentKernel];
    // reqd_work_group_size
    metadata.reqdWorkGroupSize[0] = out[0];
    metadata.reqdWorkGroupSize[1] = out[1];
    metadata.reqdWorkGroupSize[2] = out[2];
}

void AsmROCmPseudoOps::setWorkGroupSizeHint(AsmROCmHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t out[3] = { 1, 1, 1 };
    // parse CWS (1-3 values)
    if (!AsmAmdPseudoOps::parseCWS(asmr, pseudoOpPlace, linePtr, out))
        return;
    handler.output.useMetadataInfo = true;
    ROCmKernelMetadata& metadata = handler.output.metadataInfo.kernels[asmr.currentKernel];
    // work group size hint
    metadata.workGroupSizeHint[0] = out[0];
    metadata.workGroupSizeHint[1] = out[1];
    metadata.workGroupSizeHint[2] = out[2];
}

void AsmROCmPseudoOps::setFixedWorkGroupSize(AsmROCmHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t out[3] = { 1, 1, 1 };
    // parse CWS (1-3 values)
    if (!AsmAmdPseudoOps::parseCWS(asmr, pseudoOpPlace, linePtr, out))
        return;
    handler.output.useMetadataInfo = true;
    ROCmKernelMetadata& metadata = handler.output.metadataInfo.kernels[asmr.currentKernel];
    // fixed work group size
    metadata.fixedWorkGroupSize[0] = out[0];
    metadata.fixedWorkGroupSize[1] = out[1];
    metadata.fixedWorkGroupSize[2] = out[2];
}

void AsmROCmPseudoOps::setVecTypeHint(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    CString vecTypeHint;
    skipSpacesToEnd(linePtr, end);
    bool good = getNameArg(asmr, vecTypeHint, linePtr, "vectypehint", true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    ROCmKernelMetadata& metadata = handler.output.metadataInfo.kernels[asmr.currentKernel];
    handler.output.useMetadataInfo = true;
    metadata.vecTypeHint = vecTypeHint;
}

void AsmROCmPseudoOps::setKernelSymName(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    std::string symName;
    skipSpacesToEnd(linePtr, end);
    bool good = asmr.parseString(symName, linePtr);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    ROCmKernelMetadata& metadata = handler.output.metadataInfo.kernels[asmr.currentKernel];
    handler.output.useMetadataInfo = true;
    metadata.symbolName = symName;
}

void AsmROCmPseudoOps::setKernelLanguage(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    std::string langName;
    skipSpacesToEnd(linePtr, end);
    bool good = asmr.parseString(langName, linePtr);
    
    uint64_t langVerMajor = 0, langVerMinor = 0;
    skipSpacesToEnd(linePtr, end);
    if (linePtr != end)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        
        skipSpacesToEnd(linePtr, end);
        // parse language major version
        const char* valuePlace = linePtr;
        if (getAbsoluteValueArg(asmr, langVerMajor, linePtr, true))
            asmr.printWarningForRange(sizeof(cxuint)<<3, langVerMajor,
                        asmr.getSourcePos(valuePlace), WS_UNSIGNED);
        else
            good = false;
        if (!skipRequiredComma(asmr, linePtr))
            return;
        
        // parse language major version
        skipSpacesToEnd(linePtr, end);
        valuePlace = linePtr;
        if (getAbsoluteValueArg(asmr, langVerMinor, linePtr, true))
            asmr.printWarningForRange(sizeof(cxuint)<<3, langVerMinor,
                        asmr.getSourcePos(valuePlace), WS_UNSIGNED);
        else
            good = false;
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // just set language
    handler.output.useMetadataInfo = true;
    ROCmKernelMetadata& metadata = handler.output.metadataInfo.kernels[asmr.currentKernel];
    metadata.language = langName;
    metadata.langVersion[0] = langVerMajor;
    metadata.langVersion[1] = langVerMinor;
}

void AsmROCmPseudoOps::setRuntimeHandle(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    std::string runtimeHandle;
    skipSpacesToEnd(linePtr, end);
    bool good = asmr.parseString(runtimeHandle, linePtr);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    ROCmKernelMetadata& metadata = handler.output.metadataInfo.kernels[asmr.currentKernel];
    handler.output.useMetadataInfo = true;
    metadata.runtimeHandle = runtimeHandle;

}

void AsmROCmPseudoOps::addPrintf(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    ROCmPrintfInfo printfInfo{};
    uint64_t printfId = BINGEN_DEFAULT;
    skipSpacesToEnd(linePtr, end);
    // parse printf id
    const char* valuePlace = linePtr;
    bool good = true;
    if (getAbsoluteValueArg(asmr, printfId, linePtr))
        asmr.printWarningForRange(sizeof(cxuint)<<3, printfId,
                            asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    else
        good = false;
    printfInfo.id = printfId;
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    std::vector<uint32_t> argSizes;
    skipSpacesToEnd(linePtr, end);
    // parse argument sizes
    while (linePtr != end && *linePtr!='\"')
    {
        uint64_t argSize = 0;
        valuePlace = linePtr;
        if (getAbsoluteValueArg(asmr, argSize, linePtr, true))
            asmr.printWarningForRange(sizeof(cxuint)<<3, argSize,
                            asmr.getSourcePos(valuePlace), WS_UNSIGNED);
        else
            good = false;
        argSizes.push_back(uint32_t(argSize));
        
        if (!skipRequiredComma(asmr, linePtr))
            return;
        skipSpacesToEnd(linePtr, end);
    }
    printfInfo.argSizes.assign(argSizes.begin(), argSizes.end());
    
    if (linePtr == end)
        PSEUDOOP_RETURN_BY_ERROR("Missing format string")
    // parse format
    std::string formatStr;
    good &= asmr.parseString(formatStr, linePtr);
    printfInfo.format = formatStr;
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.useMetadataInfo = true;
    // just add new printf
    handler.output.metadataInfo.printfInfos.push_back(printfInfo);
}

static const std::pair<const char*, cxuint> rocmValueKindNamesTbl[] =
{
    { "complact", cxuint(ROCmValueKind::HIDDEN_COMPLETION_ACTION) },
    { "defqueue", cxuint(ROCmValueKind::HIDDEN_DEFAULT_QUEUE) },
    { "dynshptr", cxuint(ROCmValueKind::DYN_SHARED_PTR) },
    { "globalbuf", cxuint(ROCmValueKind::GLOBAL_BUFFER) },
    { "globaloffsetx", cxuint(ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X) },
    { "globaloffsety", cxuint(ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X) },
    { "globaloffsetz", cxuint(ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X) },
    { "gox", cxuint(ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X) },
    { "goy", cxuint(ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Y) },
    { "goz", cxuint(ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Z) },
    { "image", cxuint(ROCmValueKind::IMAGE) },
    { "none", cxuint(ROCmValueKind::HIDDEN_NONE) },
    { "pipe", cxuint(ROCmValueKind::PIPE) },
    { "printfbuf", cxuint(ROCmValueKind::HIDDEN_PRINTF_BUFFER) },
    { "queue", cxuint(ROCmValueKind::QUEUE) },
    { "sampler", cxuint(ROCmValueKind::SAMPLER) },
    { "value", cxuint(ROCmValueKind::BY_VALUE) }
};

static const size_t rocmValueKindNamesTblSize =
        sizeof(rocmValueKindNamesTbl) / sizeof(std::pair<const char*, cxuint>);

static const std::pair<const char*, cxuint> rocmValueTypeNamesTbl[] =
{
    { "char", cxuint(ROCmValueType::INT8) },
    { "double", cxuint(ROCmValueType::FLOAT64) },
    { "f16", cxuint(ROCmValueType::FLOAT16) },
    { "f32", cxuint(ROCmValueType::FLOAT32) },
    { "f64", cxuint(ROCmValueType::FLOAT64) },
    { "float", cxuint(ROCmValueType::FLOAT32) },
    { "half", cxuint(ROCmValueType::FLOAT16) },
    { "i16", cxuint(ROCmValueType::INT16) },
    { "i32", cxuint(ROCmValueType::INT32) },
    { "i64", cxuint(ROCmValueType::INT64) },
    { "i8", cxuint(ROCmValueType::INT8) },
    { "int", cxuint(ROCmValueType::INT32) },
    { "long", cxuint(ROCmValueType::INT64) },
    { "short", cxuint(ROCmValueType::INT16) },
    { "struct", cxuint(ROCmValueType::STRUCTURE) },
    { "u16", cxuint(ROCmValueType::UINT16) },
    { "u32", cxuint(ROCmValueType::UINT32) },
    { "u64", cxuint(ROCmValueType::UINT64) },
    { "u8", cxuint(ROCmValueType::UINT8) },
    { "uchar", cxuint(ROCmValueType::UINT8) },
    { "uint", cxuint(ROCmValueType::UINT32) },
    { "ulong", cxuint(ROCmValueType::UINT64) },
    { "ushort", cxuint(ROCmValueType::INT16) }
};

static const size_t rocmValueTypeNamesTblSize =
        sizeof(rocmValueTypeNamesTbl) / sizeof(std::pair<const char*, cxuint>);

static const std::pair<const char*, cxuint> rocmAddressSpaceNamesTbl[] =
{
    { "constant", cxuint(ROCmAddressSpace::CONSTANT) },
    { "generic", cxuint(ROCmAddressSpace::GENERIC) },
    { "global", cxuint(ROCmAddressSpace::GLOBAL) },
    { "local", cxuint(ROCmAddressSpace::LOCAL) },
    { "private", cxuint(ROCmAddressSpace::PRIVATE) },
    { "region", cxuint(ROCmAddressSpace::REGION) }
};

static const std::pair<const char*, cxuint> rocmAccessQualNamesTbl[] =
{
    { "default", cxuint(ROCmAccessQual::DEFAULT) },
    { "rdonly", cxuint(ROCmAccessQual::READ_ONLY) },
    { "rdwr", cxuint(ROCmAccessQual::READ_WRITE) },
    { "read_only", cxuint(ROCmAccessQual::READ_ONLY) },
    { "read_write", cxuint(ROCmAccessQual::READ_WRITE) },
    { "write_only", cxuint(ROCmAccessQual::WRITE_ONLY) },
    { "wronly", cxuint(ROCmAccessQual::WRITE_ONLY) }
};

static const size_t rocmAccessQualNamesTblSize =
        sizeof(rocmAccessQualNamesTbl) / sizeof(std::pair<const char*, cxuint>);

// add kernel argument (to metadata)
void AsmROCmPseudoOps::addKernelArg(AsmROCmHandler& handler, const char* pseudoOpPlace,
                    const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (handler.metadataSection != ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata config can't be defined if "
                    "metadata section exists")
    
    bool good = true;
    skipSpacesToEnd(linePtr, end);
    CString argName;
    if (linePtr!=end && *linePtr!=',')
        // parse name
        good &= getNameArg(asmr, argName, linePtr, "argument name", true);
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    
    std::string typeName;
    if (linePtr!=end && *linePtr=='"')
    {
        // parse arg type
        good &= asmr.parseString(typeName, linePtr);
        
        if (!skipRequiredComma(asmr, linePtr))
            return;
    }
    // parse argument size
    uint64_t argSize = 0;
    const char* sizePlace = linePtr;
    if (getAbsoluteValueArg(asmr, argSize, linePtr, true))
    {
        if (argSize == 0)
            ASM_NOTGOOD_BY_ERROR(sizePlace, "Argument size is zero")
    }
    else
        good = false;
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    skipSpacesToEnd(linePtr, end);
    // parse argument alignment
    uint64_t argAlign = 0;
    if (linePtr!=end && *linePtr!=',')
    {
        const char* valuePlace = linePtr;
        if (getAbsoluteValueArg(asmr, argAlign, linePtr, true))
        {
            if (argAlign==0 || argAlign != (1ULL<<(63-CLZ64(argAlign))))
                ASM_NOTGOOD_BY_ERROR(valuePlace, "Argument alignment is not power of 2")
        }
        else
            good = false;
    }
    if (argAlign == 0)
    {
        argAlign = (argSize!=0) ? 1ULL<<(63-CLZ64(argSize)) : 1;
        if (argSize > argAlign)
            argAlign <<= 1;
    }
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    // get value kind
    cxuint valueKindVal = 0;
    good &= getEnumeration(asmr, linePtr, "value kind", rocmValueKindNamesTblSize,
                rocmValueKindNamesTbl, valueKindVal, nullptr);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    // get value type
    cxuint valueTypeVal = 0;
    good &= getEnumeration(asmr, linePtr, "value type", rocmValueTypeNamesTblSize,
                rocmValueTypeNamesTbl, valueTypeVal, nullptr);
    
    uint64_t pointeeAlign = 0;
    if (valueKindVal == cxuint(ROCmValueKind::DYN_SHARED_PTR))
    {
        // parse pointeeAlign
        if (!skipRequiredComma(asmr, linePtr))
            return;
        const char* valuePlace = linePtr;
        if (getAbsoluteValueArg(asmr, pointeeAlign, linePtr, true))
        {
            if (pointeeAlign==0 || pointeeAlign != (1ULL<<(63-CLZ64(pointeeAlign))))
                ASM_NOTGOOD_BY_ERROR(valuePlace, "Argument pointee alignment "
                            "is not power of 2")
        }
        else
            good = false;
    }
    
    cxuint addressSpaceVal = 0;
    if (valueKindVal == cxuint(ROCmValueKind::DYN_SHARED_PTR) ||
        valueKindVal == cxuint(ROCmValueKind::GLOBAL_BUFFER))
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        // parse address space
        good &= getEnumeration(asmr, linePtr, "address space",
                    6, rocmAddressSpaceNamesTbl, addressSpaceVal, nullptr);
    }
    
    bool haveComma = false;
    cxuint accessQualVal = 0;
    if (valueKindVal == cxuint(ROCmValueKind::IMAGE) ||
        valueKindVal == cxuint(ROCmValueKind::PIPE))
    {
        if (!skipComma(asmr, haveComma, linePtr, false))
            return;
        // parse access qualifier
        if (haveComma)
            good &= getEnumeration(asmr, linePtr, "access qualifier",
                    rocmAccessQualNamesTblSize, rocmAccessQualNamesTbl,
                    accessQualVal, nullptr);
    }
    cxuint actualAccessQualVal = 0;
    if (valueKindVal == cxuint(ROCmValueKind::GLOBAL_BUFFER) ||
        valueKindVal == cxuint(ROCmValueKind::IMAGE) ||
        valueKindVal == cxuint(ROCmValueKind::PIPE))
    {
        if (!skipComma(asmr, haveComma, linePtr, false))
            return;
        // parse actual access qualifier
        if (haveComma)
            good &= getEnumeration(asmr, linePtr, "access qualifier",
                    rocmAccessQualNamesTblSize, rocmAccessQualNamesTbl,
                    actualAccessQualVal, nullptr);
    }
    
    bool argIsConst = false;
    bool argIsRestrict = false;
    bool argIsVolatile = false;
    bool argIsPipe = false;
    // parse list of flags
    skipSpacesToEnd(linePtr, end);
    while (linePtr != end)
    {
        char name[20];
        const char* fieldPlace = linePtr;
        if (!getNameArg(asmr, 20, name, linePtr, "argument flag", true))
        {
            good = false;
            break;
        }
        
        if (::strcmp(name, "const")==0)
            argIsConst = true;
        else if (::strcmp(name, "restrict")==0)
            argIsRestrict= true;
        else if (::strcmp(name, "volatile")==0)
            argIsVolatile = true;
        else if (::strcmp(name, "pipe")==0)
            argIsPipe = true;
        else
        {
            ASM_NOTGOOD_BY_ERROR(fieldPlace, "Unknown argument flag")
            break;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.output.useMetadataInfo = true;
    ROCmKernelMetadata& metadata = handler.output.metadataInfo.kernels[asmr.currentKernel];
    // setup kernel arg info
    ROCmKernelArgInfo argInfo{};
    argInfo.name = argName;
    argInfo.typeName = typeName;
    argInfo.size = argSize;
    argInfo.align = argAlign;
    argInfo.pointeeAlign = pointeeAlign;
    argInfo.valueKind = ROCmValueKind(valueKindVal);
    argInfo.valueType = ROCmValueType(valueTypeVal);
    argInfo.addressSpace = ROCmAddressSpace(addressSpaceVal);
    argInfo.accessQual = ROCmAccessQual(accessQualVal);
    argInfo.actualAccessQual = ROCmAccessQual(actualAccessQualVal);
    argInfo.isConst = argIsConst;
    argInfo.isRestrict = argIsRestrict;
    argInfo.isVolatile = argIsVolatile;
    argInfo.isPipe = argIsPipe;
    // just add to kernel arguments
    metadata.argInfos.push_back(argInfo);
}

bool AsmROCmPseudoOps::checkConfigValue(Assembler& asmr, const char* valuePlace,
                ROCmConfigValueTarget target, uint64_t value)
{
    bool good = true;
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
                ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
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
                ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
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
                ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
            }
            break;
        }
        case ROCMCVAL_USERDATANUM:
            if (value > 16)
                ASM_NOTGOOD_BY_ERROR(valuePlace, "UserDataNum out of range (0-16)")
            break;
        case ROCMCVAL_PRIVATE_ELEM_SIZE:
            if (value==0 || 1ULL<<(63-CLZ64(value)) != value)
                ASM_NOTGOOD_BY_ERROR(valuePlace,
                                "Private element size must be power of two")
            else if (value < 2 || value > 16)
                ASM_NOTGOOD_BY_ERROR(valuePlace, "Private element size out of range")
            break;
        case ROCMCVAL_KERNARG_SEGMENT_ALIGN:
        case ROCMCVAL_GROUP_SEGMENT_ALIGN:
        case ROCMCVAL_PRIVATE_SEGMENT_ALIGN:
            if (value==0 || 1ULL<<(63-CLZ64(value)) != value)
                ASM_NOTGOOD_BY_ERROR(valuePlace, "Alignment must be power of two")
            else if (value < 16)
                ASM_NOTGOOD_BY_ERROR(valuePlace, "Alignment must be not smaller than 16")
            break;
        case ROCMCVAL_WAVEFRONT_SIZE:
            if (value==0 || 1ULL<<(63-CLZ64(value)) != value)
                ASM_NOTGOOD_BY_ERROR(valuePlace, "Wavefront size must be power of two")
            else if (value > 256)
                ASM_NOTGOOD_BY_ERROR(valuePlace,
                            "Wavefront size must be not greater than 256")
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
                ASM_NOTGOOD_BY_ERROR(valuePlace, "SGPR register out of range")
            break;
        }
        default:
            break;
    }
    return good;
}

// check metadata config values
bool AsmROCmPseudoOps::checkMDConfigValue(Assembler& asmr, const char* valuePlace,
                ROCmConfigValueTarget target, uint64_t value)
{
    bool good = true;
    switch(target)
    {
        case ROCMCVAL_MD_WAVEFRONT_SIZE:
            if (value==0 || 1ULL<<(63-CLZ64(value)) != value)
                ASM_NOTGOOD_BY_ERROR(valuePlace, "Wavefront size must be power of two")
            else if (value > 256)
                ASM_NOTGOOD_BY_ERROR(valuePlace,
                            "Wavefront size must be not greater than 256")
            break;
        case ROCMCVAL_MD_KERNARG_SEGMENT_ALIGN:
            if (value==0 || 1ULL<<(63-CLZ64(value)) != value)
                ASM_NOTGOOD_BY_ERROR(valuePlace, "Alignment size must be power of two")
            break;
        case ROCMCVAL_MD_SGPRSNUM:
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
        case ROCMCVAL_MD_VGPRSNUM:
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
        default:
            break;
    }
    return good;
}

void AsmROCmPseudoOps::setConfigValueMain(AsmAmdHsaKernelConfig& config,
                ROCmConfigValueTarget target, uint64_t value)
{
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

/// set metadata config values
void AsmROCmPseudoOps::setMDConfigValue(ROCmKernelMetadata& metadata,
                        ROCmConfigValueTarget target, uint64_t value)
{
    switch(target)
    {
        case ROCMCVAL_MD_WAVEFRONT_SIZE:
            metadata.wavefrontSize = value;
            break;
        case ROCMCVAL_MD_KERNARG_SEGMENT_ALIGN:
            metadata.kernargSegmentAlign = value;
            break;
        case ROCMCVAL_MD_KERNARG_SEGMENT_SIZE:
            metadata.kernargSegmentSize = value;
            break;
        case ROCMCVAL_MD_GROUP_SEGMENT_FIXED_SIZE:
            metadata.groupSegmentFixedSize = value;
            break;
        case ROCMCVAL_MD_PRIVATE_SEGMENT_FIXED_SIZE:
            metadata.privateSegmentFixedSize = value;
            break;
        case ROCMCVAL_MD_SGPRSNUM:
            metadata.sgprsNum = value;
            break;
        case ROCMCVAL_MD_VGPRSNUM:
            metadata.vgprsNum = value;
            break;
        case ROCMCVAL_MD_SPILLEDSGPRS:
            metadata.spilledSgprs = value;
            break;
        case ROCMCVAL_MD_SPILLEDVGPRS:
            metadata.spilledVgprs = value;
            break;
        case ROCMCVAL_MAX_FLAT_WORK_GROUP_SIZE:
            metadata.maxFlatWorkGroupSize = value;
            break;
        default:
            break;
    }
}

void AsmROCmPseudoOps::setConfigValue(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr, ROCmConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value = BINGEN64_NOTSUPPLIED;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    /* ranges checking */
    if (good)
    {
        if (target < ROCMCVAL_METADATA_START)
            good = checkConfigValue(asmr, valuePlace, target, value);
        else // metadata values
            good = checkMDConfigValue(asmr, valuePlace, target, value);
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // set value
    if (target < ROCMCVAL_METADATA_START)
    {
        AsmROCmKernelConfig& config = *(handler.kernelStates[asmr.currentKernel]->config);
        setConfigValueMain(config, target, value);
    }
    else
    {
        // set metadata value
        handler.output.useMetadataInfo = true;
        ROCmKernelMetadata& metadata = 
                    handler.output.metadataInfo.kernels[asmr.currentKernel];
        setMDConfigValue(metadata, target, value);
    }
}

void AsmROCmPseudoOps::setConfigBoolValueMain(AsmAmdHsaKernelConfig& config,
                ROCmConfigValueTarget target)
{
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
            config.enableSgprRegisterFlags |= ROCMFLAG_USE_PRIVATE_SEGMENT_BUFFER;
            break;
        case ROCMCVAL_USE_DISPATCH_PTR:
            config.enableSgprRegisterFlags |= ROCMFLAG_USE_DISPATCH_PTR;
            break;
        case ROCMCVAL_USE_QUEUE_PTR:
            config.enableSgprRegisterFlags |= ROCMFLAG_USE_QUEUE_PTR;
            break;
        case ROCMCVAL_USE_KERNARG_SEGMENT_PTR:
            config.enableSgprRegisterFlags |= ROCMFLAG_USE_KERNARG_SEGMENT_PTR;
            break;
        case ROCMCVAL_USE_DISPATCH_ID:
            config.enableSgprRegisterFlags |= ROCMFLAG_USE_DISPATCH_ID;
            break;
        case ROCMCVAL_USE_FLAT_SCRATCH_INIT:
            config.enableSgprRegisterFlags |= ROCMFLAG_USE_FLAT_SCRATCH_INIT;
            break;
        case ROCMCVAL_USE_PRIVATE_SEGMENT_SIZE:
            config.enableSgprRegisterFlags |= ROCMFLAG_USE_PRIVATE_SEGMENT_SIZE;
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

void AsmROCmPseudoOps::setConfigBoolValue(AsmROCmHandler& handler,
          const char* pseudoOpPlace, const char* linePtr, ROCmConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmROCmKernelConfig& config = *(handler.kernelStates[asmr.currentKernel]->config);
    
    setConfigBoolValueMain(config, target);
}

void AsmROCmPseudoOps::setDefaultHSAFeatures(AsmROCmHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmROCmKernelConfig* config = handler.kernelStates[asmr.currentKernel]->config.get();
    config->enableSgprRegisterFlags = uint16_t(ROCMFLAG_USE_PRIVATE_SEGMENT_BUFFER|
                    ROCMFLAG_USE_DISPATCH_PTR|ROCMFLAG_USE_KERNARG_SEGMENT_PTR);
    config->enableFeatureFlags = uint16_t(AMDHSAFLAG_USE_PTR64|2);
}

void AsmROCmPseudoOps::setDimensions(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    cxuint dimMask = 0;
    if (!parseDimensions(asmr, linePtr, dimMask, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.kernelStates[asmr.currentKernel]->config->dimMask = dimMask;
}

// parse machine version - used also in AsmGalliumFormat
// four numbers - kind, major, minor, stepping
bool AsmROCmPseudoOps::parseMachine(Assembler& asmr, const char* linePtr,
        uint16_t& machineKind, uint16_t& machineMajor, uint16_t& machineMinor,
        uint16_t& machineStepping)
{
    const char* end = asmr.line + asmr.lineSize;
    
    skipSpacesToEnd(linePtr, end);
    uint64_t kindValue = 0;
    uint64_t majorValue = 0;
    uint64_t minorValue = 0;
    uint64_t steppingValue = 0;
    const char* valuePlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, kindValue, linePtr, true);
    // parse kind
    asmr.printWarningForRange(16, kindValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    valuePlace = linePtr;
    good &= getAbsoluteValueArg(asmr, majorValue, linePtr, true);
    // parse major
    asmr.printWarningForRange(16, majorValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    valuePlace = linePtr;
    good &= getAbsoluteValueArg(asmr, minorValue, linePtr, true);
    // parse minor
    asmr.printWarningForRange(16, minorValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    valuePlace = linePtr;
    // parse stepping
    good &= getAbsoluteValueArg(asmr, steppingValue, linePtr, true);
    asmr.printWarningForRange(16, steppingValue,
                      asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    machineKind = kindValue;
    machineMajor = majorValue;
    machineMinor = minorValue;
    machineStepping = steppingValue;
    return true;
}

// set machine - four numbers - kind, major, minor, stepping
void AsmROCmPseudoOps::setMachine(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    uint16_t kindValue = 0, majorValue = 0;
    uint16_t minorValue = 0, steppingValue = 0;
    if (!parseMachine(asmr, linePtr, kindValue, majorValue, minorValue, steppingValue))
        return;
    
    AsmROCmKernelConfig* config = handler.kernelStates[asmr.currentKernel]->config.get();
    config->amdMachineKind = kindValue;
    config->amdMachineMajor = majorValue;
    config->amdMachineMinor = minorValue;
    config->amdMachineStepping = steppingValue;
}

// parse code version (amd code version) - used also in AsmGalliumFormat
// two numbers - major and minor
bool AsmROCmPseudoOps::parseCodeVersion(Assembler& asmr, const char* linePtr,
                uint16_t& codeMajor, uint16_t& codeMinor)
{
    const char* end = asmr.line + asmr.lineSize;
    
    skipSpacesToEnd(linePtr, end);
    uint64_t majorValue = 0;
    uint64_t minorValue = 0;
    const char* valuePlace = linePtr;
    // parse version major
    bool good = getAbsoluteValueArg(asmr, majorValue, linePtr, true);
    asmr.printWarningForRange(32, majorValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    valuePlace = linePtr;
    // parse version minor 
    good &= getAbsoluteValueArg(asmr, minorValue, linePtr, true);
    asmr.printWarningForRange(32, minorValue, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    codeMajor = majorValue;
    codeMinor = minorValue;
    return true;
}

// two numbers - major and minor
void AsmROCmPseudoOps::setCodeVersion(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    uint16_t majorValue = 0, minorValue = 0;
    if (!parseCodeVersion(asmr, linePtr, majorValue, minorValue))
        return;
    
    AsmROCmKernelConfig* config = handler.kernelStates[asmr.currentKernel]->config.get();
    config->amdCodeVersionMajor = majorValue;
    config->amdCodeVersionMinor = minorValue;
}

// parse reserved gprs - used also in AsmGalliumFormat
// parsereserved S/VGRPS - first number is first register, second is last register
bool AsmROCmPseudoOps::parseReservedXgprs(Assembler& asmr, const char* linePtr,
                bool inVgpr, uint16_t& gprFirst, uint16_t& gprCount)
{
    const char* end = asmr.line + asmr.lineSize;
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
        // first register is out of range
        char buf[64];
        snprintf(buf, 64, "First reserved %s register out of range (0-%u)",
                 inVgpr ? "VGPR" : "SGPR",  maxGPRsNum-1);
        ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
    }
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    valuePlace = linePtr;
    bool haveLastReg = getAbsoluteValueArg(asmr, lastReg, linePtr, true);
    good &= haveLastReg;
    if (haveLastReg && lastReg > maxGPRsNum-1)
    {
        // if last register out of range
        char buf[64];
        snprintf(buf, 64, "Last reserved %s register out of range (0-%u)",
                 inVgpr ? "VGPR" : "SGPR", maxGPRsNum-1);
        ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
    }
    if (haveFirstReg && haveLastReg && firstReg > lastReg)
        ASM_NOTGOOD_BY_ERROR(valuePlace, "Wrong register range")
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    gprFirst = firstReg;
    gprCount = lastReg-firstReg+1;
    return true;
}

/// set reserved S/VGRPS - first number is first register, second is last register
void AsmROCmPseudoOps::setReservedXgprs(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool inVgpr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    uint16_t gprFirst = 0, gprCount = 0;
    if (!parseReservedXgprs(asmr, linePtr, inVgpr, gprFirst, gprCount))
        return;
    
    AsmROCmKernelConfig* config = handler.kernelStates[asmr.currentKernel]->config.get();
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
void AsmROCmPseudoOps::setUseGridWorkGroupCount(AsmROCmHandler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    cxuint dimMask = 0;
    if (!parseDimensions(asmr, linePtr, dimMask))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    uint16_t& flags = handler.kernelStates[asmr.currentKernel]->config->
                enableSgprRegisterFlags;
    flags = (flags & ~(7<<ROCMFLAG_USE_GRID_WORKGROUP_COUNT_BIT)) |
            (dimMask<<ROCMFLAG_USE_GRID_WORKGROUP_COUNT_BIT);
}

void AsmROCmPseudoOps::noSectionDiffs(AsmROCmHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.sectionDiffsResolvable = false;
}

void AsmROCmPseudoOps::addGotSymbol(AsmROCmHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    // parse symbol
    skipSpacesToEnd(linePtr, end);
    const char* symNamePlace = linePtr;
    AsmSymbolEntry* entry;
    bool good = true;
    const CString symName = extractScopedSymName(linePtr, end, false);
    if (symName.empty())
        // this is not symbol or a missing symbol
        ASM_NOTGOOD_BY_ERROR(symNamePlace, "Expected symbol")
    else if (symName==".")
        ASM_NOTGOOD_BY_ERROR(symNamePlace, "Illegal symbol '.'")
    
    AsmScope* outScope;
    CString sameSymName;
    if (good)
    {
        entry = asmr.findSymbolInScope(symName, outScope, sameSymName);
        if (outScope != &asmr.globalScope)
            ASM_NOTGOOD_BY_ERROR(symNamePlace, "Symbol must be in global scope")
    }
    
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    
    CString targetSymName;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        symNamePlace = linePtr;
        // parse target symbol (that refer to GOT section
        targetSymName = extractScopedSymName(linePtr, end, false);
        if (targetSymName.empty())
            ASM_RETURN_BY_ERROR(symNamePlace, "Illegal symbol name")
        size_t symNameLength = targetSymName.size();
        // special case for '.' symbol (check whether is in global scope)
        if (symNameLength >= 3 && targetSymName.compare(symNameLength-3, 3, "::.")==0)
            ASM_RETURN_BY_ERROR(symNamePlace, "Symbol '.' can be only in global scope")
        if (!checkGarbagesAtEnd(asmr, linePtr))
            return;
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (entry==nullptr)
    {
        // create unresolved symbol if not found
        std::pair<AsmSymbolMap::iterator, bool> res = outScope->symbolMap.insert(
                        std::make_pair(sameSymName, AsmSymbol()));
        entry = &*res.first;
    }
    // add GOT symbol
    size_t gotSymbolIndex = handler.gotSymbols.size();
    handler.gotSymbols.push_back(sameSymName);
    
    if (handler.gotSection == ASMSECT_NONE)
    {
        // create GOT section
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_GLOBAL,
            AsmSectionType::ROCM_GOT, ROCMSECTID_GOT, nullptr });
        handler.gotSection = thisSection;
        AsmFormatHandler::SectionInfo info = handler.getSectionInfo(thisSection);
        // add to assembler
        asmr.sections.push_back({ info.name, ASMKERN_GLOBAL, info.type, info.flags, 0,
                    0, info.relSpace });
    }
    
    // set up value of GOT symbol
    if (!targetSymName.empty())
    {
        std::pair<AsmSymbolEntry*, bool> res = asmr.insertSymbolInScope(targetSymName,
                    AsmSymbol(handler.gotSection, gotSymbolIndex*8));
        if (!res.second)
        {
            // if symbol found
            if (res.first->second.onceDefined && res.first->second.isDefined()) // if label
                asmr.printError(symNamePlace, (std::string("Symbol '")+
                            targetSymName.c_str()+ "' is already defined").c_str());
            else // set value of symbol
                asmr.setSymbol(*res.first, gotSymbolIndex*8, handler.gotSection);
        }
        else // set hasValue (by isResolvableSection
            res.first->second.hasValue = true;
    }
    // set GOT section size
    asmr.sections[handler.gotSection].size = handler.gotSymbols.size()*8;
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
        case ROCMOP_ARG:
            AsmROCmPseudoOps::addKernelArg(*this, stmtPlace, linePtr);
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
        case ROCMOP_CWS:
        case ROCMOP_REQD_WORK_GROUP_SIZE:
            AsmROCmPseudoOps::setCWS(*this, stmtPlace, linePtr);
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
        case ROCMOP_EFLAGS:
            AsmROCmPseudoOps::setEFlags(*this, linePtr);
            break;
        case ROCMOP_EXCEPTIONS:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_EXCEPTIONS);
            break;
        case ROCMOP_FIXED_WORK_GROUP_SIZE:
            AsmROCmPseudoOps::setFixedWorkGroupSize(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_FKERNEL:
            AsmROCmPseudoOps::doFKernel(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_FLOATMODE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_FLOATMODE);
            break;
        case ROCMOP_GLOBALDATA:
            AsmROCmPseudoOps::doGlobalData(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_GDS_SEGMENT_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_GDS_SEGMENT_SIZE);
            break;
        case ROCMOP_GOTSYM:
            AsmROCmPseudoOps::addGotSymbol(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_GROUP_SEGMENT_ALIGN:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_GROUP_SEGMENT_ALIGN);
            break;
        case ROCMOP_DEFAULT_HSA_FEATURES:
            AsmROCmPseudoOps::setDefaultHSAFeatures(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_IEEEMODE:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_IEEEMODE);
            break;
        case ROCMOP_KCODE:
            AsmKcodePseudoOps::doKCode(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_KCODEEND:
            AsmKcodePseudoOps::doKCodeEnd(*this, stmtPlace, linePtr);
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
        case ROCMOP_MAX_FLAT_WORK_GROUP_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MAX_FLAT_WORK_GROUP_SIZE);
            break;
        case ROCMOP_MAX_SCRATCH_BACKING_MEMORY:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_MAX_SCRATCH_BACKING_MEMORY);
            break;
        case ROCMOP_METADATA:
            AsmROCmPseudoOps::addMetadata(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_MD_GROUP_SEGMENT_FIXED_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_GROUP_SEGMENT_FIXED_SIZE);
            break;
        case ROCMOP_MD_KERNARG_SEGMENT_ALIGN:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_KERNARG_SEGMENT_ALIGN);
            break;
        case ROCMOP_MD_KERNARG_SEGMENT_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_KERNARG_SEGMENT_SIZE);
            break;
        case ROCMOP_MD_LANGUAGE:
            AsmROCmPseudoOps::setKernelLanguage(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_NOSECTDIFFS:
            AsmROCmPseudoOps::noSectionDiffs(*this, linePtr);
            break;
        case ROCMOP_MD_PRIVATE_SEGMENT_FIXED_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_PRIVATE_SEGMENT_FIXED_SIZE);
            break;
        case ROCMOP_SPILLEDSGPRS:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_SPILLEDSGPRS);
            break;
        case ROCMOP_SPILLEDVGPRS:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_SPILLEDVGPRS);
            break;
        case ROCMOP_MD_SGPRSNUM:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_SGPRSNUM);
            break;
        case ROCMOP_MD_SYMNAME:
            AsmROCmPseudoOps::setKernelSymName(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_MD_VERSION:
            AsmROCmPseudoOps::setMetadataVersion(*this, stmtPlace, linePtr);
            break;
        case ROCMOP_MD_VGPRSNUM:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_VGPRSNUM);
            break;
        case ROCMOP_MD_WAVEFRONT_SIZE:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                            ROCMCVAL_MD_WAVEFRONT_SIZE);
            break;
        case ROCMOP_NEWBINFMT:
            AsmROCmPseudoOps::setNewBinFormat(*this, linePtr);
            break;
        case ROCMOP_PGMRSRC1:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr, ROCMCVAL_PGMRSRC1);
            break;
        case ROCMOP_PGMRSRC2:
            AsmROCmPseudoOps::setConfigValue(*this, stmtPlace, linePtr, ROCMCVAL_PGMRSRC2);
            break;
        case ROCMOP_PRINTF:
            AsmROCmPseudoOps::addPrintf(*this, stmtPlace, linePtr);
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
        case ROCMOP_RUNTIME_HANDLE:
            AsmROCmPseudoOps::setRuntimeHandle(*this, stmtPlace, linePtr);
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
        case ROCMOP_TARGET:
            AsmROCmPseudoOps::setTarget(*this, linePtr, false);
            break;
        case ROCMOP_TGSIZE:
            AsmROCmPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             ROCMCVAL_TGSIZE);
            break;
        case ROCMOP_TRIPPLE:
            AsmROCmPseudoOps::setTarget(*this, linePtr, true);
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
        case ROCMOP_VECTYPEHINT:
            AsmROCmPseudoOps::setVecTypeHint(*this, stmtPlace, linePtr);
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
        case ROCM_WORK_GROUP_SIZE_HINT:
            AsmROCmPseudoOps::setWorkGroupSizeHint(*this, stmtPlace, linePtr);
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

static uint64_t calculateKernelArgSize(const std::vector<ROCmKernelArgInfo>& argInfos)
{
    uint64_t size = 0;
    for (const ROCmKernelArgInfo& argInfo: argInfos)
    {
        // alignment
        size = (size + argInfo.align-1) & ~(argInfo.align-1);
        size += argInfo.size;
    }
    return (size + 15) & ~uint64_t(15);
}

bool AsmROCmHandler::prepareSectionDiffsResolving()
{
    good = true;
    unresolvedGlobals = false;
    size_t sectionsNum = sections.size();
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
            case AsmSectionType::ROCM_CONFIG_CTRL_DIRECTIVE:
                if (sectionSize != 128)
                    // control directive accepts only 128-byte size
                    assembler.printError(AsmSourcePos(),
                         (std::string("Section '.control_directive' for kernel '")+
                          assembler.kernels[section.kernelId].name+
                          "' have wrong size").c_str());
                break;
            case AsmSectionType::ROCM_COMMENT:
                output.commentSize = sectionSize;
                output.comment = (const char*)sectionData;
                break;
            case AsmSectionType::DATA:
                output.globalDataSize = sectionSize;
                output.globalData = sectionData;
                break;
            case AsmSectionType::ROCM_METADATA:
                output.metadataSize = sectionSize;
                output.metadata = (const char*)sectionData;
                break;
            default:
                break;
        }
    }
    
    // enable metadata config for new binary format by default (if no metadata section)
    if (output.newBinFormat && output.metadata==nullptr)
        output.useMetadataInfo = true;
    
    GPUArchitecture arch = getGPUArchitectureFromDeviceType(assembler.deviceType);
    // set up number of the allocated SGPRs and VGPRs for kernel
    cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    AMDGPUArchVersion amdGpuArchValues = getGPUArchVersion(assembler.deviceType,
                                GPUArchVersionTable::OPENSOURCE);
    // replace arch minor and stepping by user defined values (if set)
    if (output.archMinor!=UINT32_MAX)
        amdGpuArchValues.minor = output.archMinor;
    if (output.archStepping!=UINT32_MAX)
        amdGpuArchValues.stepping = output.archStepping;
    
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
            config.amdCodeVersionMinor = (output.newBinFormat) ? 1 : 0;
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
        {
            if (output.useMetadataInfo)
                // calculate kernel arg size
                config.kernargSegmentSize = calculateKernelArgSize(
                        output.metadataInfo.kernels[i].argInfos);
            else
                config.kernargSegmentSize = 0;
        }
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
        {
            // calcuate userSGPRs
            const uint16_t sgprFlags = config.enableSgprRegisterFlags;
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
                getDefaultDimMask(arch, config.computePgmRsrc2);
        // extra sgprs for dimensions
        cxuint minRegsNum[2];
        getGPUSetupMinRegistersNum(arch, dimMask, userSGPRsNum,
                   ((config.tgSize) ? GPUSETUP_TGSIZE_EN : 0) |
                   ((config.workitemPrivateSegmentSize!=0) ?
                           GPUSETUP_SCRATCH_EN : 0), minRegsNum);
        
        if (config.usedSGPRsNum!=BINGEN_DEFAULT && maxSGPRsNum < config.usedSGPRsNum)
        {
            // check only if sgprsnum set explicitly
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
                ((config.enableSgprRegisterFlags&ROCMFLAG_USE_FLAT_SCRATCH_INIT)!=0?
                            GCN_FLAT : 0) |
                // enable_xnack
                ((config.enableFeatureFlags&ROCMFLAG_USE_XNACK_ENABLED)!=0 ?
                            GCN_XNACK : 0);
            config.usedSGPRsNum = std::min(
                std::max(minRegsNum[0], kernelStates[i]->allocRegs[0]) +
                    getGPUExtraRegsNum(arch, REGTYPE_SGPR, flags|GCN_VCC),
                    maxSGPRsNum); // include all extra sgprs
        }
        // set usedVGPRsNum
        if (config.usedVGPRsNum==BINGEN_DEFAULT)
            config.usedVGPRsNum = std::max(minRegsNum[1], kernelStates[i]->allocRegs[1]);
        
        cxuint sgprsNum = std::max(config.usedSGPRsNum, 1U);
        cxuint vgprsNum = std::max(config.usedVGPRsNum, 1U);
        // computePGMRSRC1
        config.computePgmRsrc1 |= calculatePgmRSrc1(arch, vgprsNum, sgprsNum,
                        config.priority, config.floatMode, config.privilegedMode,
                        config.dx10Clamp, config.debugMode, config.ieeeMode);
        // computePGMRSRC2
        config.computePgmRsrc2 = (config.computePgmRsrc2 & 0xffffe440U) |
                calculatePgmRSrc2(arch, (config.workitemPrivateSegmentSize != 0),
                            userSGPRsNum, false, config.dimMask,
                            (config.computePgmRsrc2 & 0x1b80U), config.tgSize,
                            (output.newBinFormat ? 0 : config.workgroupGroupSegmentSize),
                            config.exceptions);
        
        if (config.wavefrontSgprCount == BINGEN16_DEFAULT)
            config.wavefrontSgprCount = sgprsNum;
        if (config.workitemVgprCount == BINGEN16_DEFAULT)
            config.workitemVgprCount = vgprsNum;
        
        if (config.runtimeLoaderKernelSymbol == BINGEN64_DEFAULT)
            config.runtimeLoaderKernelSymbol = 0;
        
        config.toLE(); // to little-endian
        // put control directive section to config
        if (kernel.ctrlDirSection!=ASMSECT_NONE &&
            assembler.sections[kernel.ctrlDirSection].content.size()==128)
            ::memcpy(config.controlDirective, 
                 assembler.sections[kernel.ctrlDirSection].content.data(), 128);
    }
    
    // check kernel symbols and setup kernel configs
    AsmSection& asmCSection = assembler.sections[codeSection];
    const AsmSymbolMap& symbolMap = assembler.getSymbolMap();
    for (size_t ki = 0; ki < output.symbols.size(); ki++)
    {
        ROCmSymbolInput& kinput = output.symbols[ki];
        auto it = symbolMap.find(kinput.symbolName);
        if (it == symbolMap.end() || !it->second.isDefined())
        {
            // error, undefined
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                        "Symbol for kernel '")+kinput.symbolName.c_str()+
                        "' is undefined").c_str());
            good = false;
            continue;
        }
        const AsmSymbol& symbol = it->second;
        if (!symbol.hasValue)
        {
            // error, unresolved
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '") + kinput.symbolName.c_str() +
                    "' is not resolved").c_str());
            good = false;
            continue;
        }
        if (symbol.sectionId != codeSection)
        {
            /// error, wrong section
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
            // if kernel configuration out of section size
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                "Code for kernel '")+kinput.symbolName.c_str()+
                "' is too small for configuration").c_str());
            good = false;
            continue;
        }
        else if (kernel.config!=nullptr)
            // put config to code section
            ::memcpy(asmCSection.content.data() + symbol.value,
                     kernel.config.get(), sizeof(ROCmKernelConfig));
        // set symbol type
        kinput.type = kernel.isFKernel ? ROCmRegionType::FKERNEL : ROCmRegionType::KERNEL;
    }
    
    // add symbols before section diffs prepping
    addSymbols(false);
    output.gotSymbols.resize(gotSymbols.size());
    prevSymbolsCount = output.symbols.size() + output.extraSymbols.size();
    
    if (good)
    {
        binGen.reset(new ROCmBinGenerator(&output));
        binGen->prepareBinaryGen();
        
        // add relSpacesSections
        assembler.sections[codeSection].relAddress =
                    binGen->getSectionOffset(ELFSECTID_TEXT);
        
        Array<AsmSectionId> relSections(1 + (dataSection != ASMSECT_NONE) +
                    (gotSection != ASMSECT_NONE));
        AsmSectionId relSectionId = 0;
        if (dataSection != ASMSECT_NONE)
        {
            assembler.sections[dataSection].relAddress =
                    binGen->getSectionOffset(ELFSECTID_RODATA);
            relSections[relSectionId++] = dataSection;
        }
        relSections[relSectionId++] = codeSection;
        if (gotSection != ASMSECT_NONE)
        {
            assembler.sections[gotSection].relAddress =
                    binGen->getSectionOffset(ROCMSECTID_GOT);
            relSections[relSectionId++] = gotSection;
        }
        assembler.relSpacesSections.push_back(std::move(relSections));
    }
    return good;
}

void AsmROCmHandler::addSymbols(bool sectionDiffsPrepared)
{
    output.extraSymbols.clear();
    
    // prepate got symbols set
    Array<CString> gotSymSet(gotSymbols.begin(), gotSymbols.end());
    std::sort(gotSymSet.begin(), gotSymSet.end());
    
    // if set adds symbols to binary
    std::vector<ROCmSymbolInput> dataSymbols;
    
    const bool forceAddSymbols = (assembler.getFlags() & ASM_FORCE_ADD_SYMBOLS) != 0;
    
    if (forceAddSymbols || !gotSymbols.empty())
        for (const AsmSymbolEntry& symEntry: assembler.globalScope.symbolMap)
        {
            if (!forceAddSymbols &&
                !std::binary_search(gotSymSet.begin(), gotSymSet.end(), symEntry.first))
                // if not forceAddSymbols and if not in got symbols
                continue;
                
            if (ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // local
            if (assembler.kernelMap.find(symEntry.first.c_str())!=assembler.kernelMap.end())
                continue; // if kernel name
            
            if (symEntry.second.sectionId==codeSection)
            {
                if (!symEntry.second.hasValue && !sectionDiffsPrepared)
                    // mark that we have some unresolved globals
                    unresolvedGlobals = true;
                // put data objects
                dataSymbols.push_back({symEntry.first, size_t(symEntry.second.value),
                    size_t(symEntry.second.size), ROCmRegionType::DATA});
                continue;
            }
            cxbyte info = symEntry.second.info;
            // object type for global symbol referring to global data
            if (symEntry.second.sectionId==dataSection)
                info = ELF32_ST_INFO(ELF32_ST_BIND(symEntry.second.info), STT_OBJECT);
            
            AsmSectionId binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            
            if (!symEntry.second.hasValue && !sectionDiffsPrepared)
                // mark that we have some unresolved globals
                unresolvedGlobals = true;
            
            output.extraSymbols.push_back({ symEntry.first, symEntry.second.value,
                    symEntry.second.size, binSectId, binSectId != ELFSECTID_ABS,
                    info, symEntry.second.other });
        }
    
    // put data objects
    if (sectionDiffsPrepared)
    {
        // move kernels to start
        for (size_t i = 0; i < kernelStates.size(); i++)
            output.symbols[i] = output.symbols[output.symbols.size()-kernelStates.size()+i];
        output.symbols.resize(kernelStates.size());
    }
    dataSymbols.insert(dataSymbols.end(), output.symbols.begin(), output.symbols.end());
    output.symbols = std::move(dataSymbols);
}

bool AsmROCmHandler::prepareBinary()
{
    if (!sectionDiffsResolvable)
        // if no section resolvable
        prepareSectionDiffsResolving();
    
    if (unresolvedGlobals)
    {
        // add and update symbols after section diffs prepping only
        // if we have unresolved globals
        addSymbols(true);
        if (prevSymbolsCount != output.symbols.size() + output.extraSymbols.size())
        {
            assembler.printError(nullptr, "Symbols count before section differences"
                " resolving doesn't match with current symbols count!");
            good = false;
            return false;
        }
        binGen->updateSymbols();
    }
    
    if (gotSymbols.empty())
        return good;
    
    // create map to speedup finding symbol indices
    size_t outputSymsNum = output.symbols.size();
    Array<std::pair<CString, size_t> > outputSymMap(outputSymsNum);
    for (size_t i = 0; i < outputSymsNum; i++)
        outputSymMap[i] = std::make_pair(output.symbols[i].symbolName, i);
    mapSort(outputSymMap.begin(), outputSymMap.end());
    
    size_t extraSymsNum = output.extraSymbols.size();
    Array<std::pair<CString, size_t> > extraSymMap(extraSymsNum);
    for (size_t i = 0; i < extraSymsNum; i++)
        extraSymMap[i] = std::make_pair(output.extraSymbols[i].name, i);
    mapSort(extraSymMap.begin(), extraSymMap.end());
    
    output.gotSymbols.clear();
    // prepare GOT symbols
    const AsmSymbolMap& symbolMap = assembler.getSymbolMap();
    for (const CString& symName: gotSymbols)
    {
        const AsmSymbolEntry& symEntry = *symbolMap.find(symName);
        if (!symEntry.second.hasValue)
        {
            good = false;
            assembler.printError(nullptr, (std::string(
                    "GOT symbol '")+symName.c_str()+"' is unresolved").c_str());
            continue;
        }
        if (ELF32_ST_BIND(symEntry.second.info) != STB_GLOBAL)
        {
            good = false;
            assembler.printError(nullptr, (std::string(
                    "GOT symbol '")+symName.c_str()+"' is not global symbol").c_str());
            continue;
        }
        
        // put to output got symbols table
        auto it = binaryMapFind(outputSymMap.begin(), outputSymMap.end(), symName);
        if (it != outputSymMap.end())
            output.gotSymbols.push_back(it->second);
        else
        {   // find in extra symbols
            it = binaryMapFind(extraSymMap.begin(), extraSymMap.end(), symName);
            if (it != extraSymMap.end())
                output.gotSymbols.push_back(outputSymsNum + it->second);
            else
            {
                good = false;
                assembler.printError(nullptr, (std::string(
                        "GOT symbol '")+symName.c_str()+"' not found!").c_str());
            }
        }
    }
    return good;
}

void AsmROCmHandler::writeBinary(std::ostream& os) const
{
    binGen->generate(os);
}

void AsmROCmHandler::writeBinary(Array<cxbyte>& array) const
{
    binGen->generate(array);
}
