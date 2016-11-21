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
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* rocmPseudoOpNamesTbl[] =
{
    "call_convention", "codeversion", "config",
    "control_directive", "debug_private_segment_buffer_sgpr",
    "debug_wavefront_private_segment_offset_sgpr",
    "debugmode", "dims", "dx10clamp",
    "exceptions", "floatmode", "gds_segment_size",
    "group_segment_align", "ieeemode", "kcode",
    "kcodeend", "kernarg_segment_align",
    "kernarg_segment_size", "kernel_code_entry_offset",
    "kernel_code_prefetch_offset", "kernel_code_prefetch_size",
    "localsize", "machine", "max_scratch_backing_memory",
    "pgmrsrc1", "pgmrsrc2", "priority", "private_segment_align",
    "privmode", "reserved_sgpr_count", "reserved_sgpr_first",
    "reserved_vgpr_count", "reserved_vgpr_first",
    "runtime_loader_kernel_symbol",
    "scratchbuffer", "sgprsnum", "tgsize",
    "use_debug_enabled", "use_dispatch_id",
    "use_dispatch_ptr", "use_dynamic_call_stack",
    "use_flat_scratch_init", "use_grid_workgroup_count",
    "use_kernarg_segment_ptr", "use_ordered_append_gds",
    "use_private_segment_size", "use_ptr64", "use_queue_ptr",
    "use_private_segment_buffer", "use_xnack_enabled",
    "userdatanum", "vgprsnum", "wavefront_sgpr_count",
    "wavefront_size", "workitem_vgpr_count",
    "workgroup_fbarrier_count", "workgroup_group_segment_size",
    "workitem_private_segment_size"
};

enum
{
    ROCMOP_CALL_CONVENTION, ROCMOP_CODEVERSION, ROCMOP_CONFIG,
    ROCMOP_CONTROL_DIRECTIVE, ROCMOP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    ROCMOP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    ROCMOP_DEBUGMODE, ROCMOP_DIMS, ROCMOP_DX10CLAMP,
    ROCMOP_EXCEPTIONS, ROCMOP_FLOATMODE, ROCMOP_GDS_SEGMENT_SIZE,
    ROCMOP_GROUP_SEGMENT_ALIGN, ROCMOP_IEEEMODE, ROCMOP_KCODE,
    ROCMOP_KCODEEND, ROCMOP_KERNARG_SEGMENT_ALIGN,
    ROCMOP_KERNARG_SEGMENT_SIZE, ROCMOP_KERNEL_CODE_ENTRY_OFFSET,
    ROCMOP_KERNEL_CODE_PREFETCH_OFFSET, ROCMOP_KERNEL_CODE_PREFETCH_SIZE,
    ROCMOP_LOCALSIZE, ROCMOP_MACHINE, ROCMOP_MAX_SCRATCH_BACKING_MEMORY,
    ROCMOP_PGMRSRC1, ROCMOP_PGMRSRC2, ROCMOP_PRIORITY, ROCMOP_PRIVATE_SEGMENT_ALIGN,
    ROCMOP_PRIVMODE, ROCMOP_RESERVED_SGPR_COUNT, ROCMOP_RESERVED_SGPR_FIRST,
    ROCMOP_RESERVED_VGPR_COUNT, ROCMOP_RESERVED_VGPR_FIRST,
    ROCMOP_RUNTIME_LOADER_KERNEL_SYMBOL,
    ROCMOP_SCRATCHBUFFER, ROCMOP_SGPRSNUM, ROCMOP_TGSIZE,
    ROCMOP_USE_DEBUG_ENABLED, ROCMOP_USE_DISPATCH_ID,
    ROCMOP_USE_DISPATCH_PTR, ROCMOP_USE_DYNAMIC_CALL_STACK,
    ROCMOP_USE_FLAT_SCRATCH_INIT, ROCMOP_USE_GRID_WORKGROUP_COUNT,
    ROCMOP_USE_KERNARG_SEGMENT_PTR, ROCMOP_USE_ORDERED_APPEND_GDS,
    ROCMOP_USE_PRIVATE_SEGMENT_SIZE, ROCMOP_USE_PTR64, ROCMOP_USE_QUEUE_PTR,
    ROCMOP_USE_PRIVATE_SEGMENT_BUFFER, ROCMOP_USE_XNACK_ENABLED,
    ROCMOP_USERDATANUM, ROCMOP_VGPRSNUM, ROCMOP_WAVEFRONT_SGPR_COUNT,
    ROCMOP_WAVEFRONT_SIZE, ROCMOP_WORKITEM_VGPR_COUNT,
    ROCMOP_WORKGROUP_FBARRIER_COUNT, ROCMOP_WORKGROUP_GROUP_SEGMENT_SIZE,
    ROCMOP_WORKITEM_PRIVATE_SEGMENT_SIZE
};

/*
 * ROCm format handler
 */

AsmROCmHandler::AsmROCmHandler(Assembler& assembler): AsmFormatHandler(assembler),
             output{}, codeSection(0), commentSection(ASMSECT_NONE),
             extraSectionCount(0)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::CODE,
                ELFSECTID_TEXT, ".text" });
    currentKcodeKernel = ASMKERN_GLOBAL;
    savedSection = 0;
}

cxuint AsmROCmHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = output.symbols.size();
    cxuint thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG, ELFSECTID_UNDEF, nullptr });
    kernelStates.push_back({ thisSection, 0, nullptr, ASMSECT_NONE, thisSection });
    
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
    if (::strcmp(sectionName, ".control_directive") == 0)
    {
        if (kernelId == ASMKERN_GLOBAL)
            throw AsmFormatException(".control_directive section must be "
                        "defined inside kernel");
        if (kernelStates[kernelId].ctrlDirSection != ASMSECT_NONE)
            throw AsmFormatException("Only one section "
                        "'.control_directive' can be in kernel");
        kernelStates[kernelId].ctrlDirSection = thisSection;
        section.type = AsmSectionType::ROCM_CONFIG_CTRL_DIRECTIVE;
        section.elfBinSectId = ELFSECTID_TEXT; // . in text
        section.name = ".control_directive";
    }
    else // otherwise
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
    return thisSection;
}

cxuint AsmROCmHandler::getSectionId(const char* sectionName) const
{
    if (::strcmp(sectionName, ".text") == 0) // code
        return codeSection;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        return commentSection;
    else if (assembler.currentKernel != ASMKERN_GLOBAL &&
        ::strcmp(sectionName, ".control_directive") == 0) // code
        return kernelStates[assembler.currentKernel].ctrlDirSection;
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
        kernelStates[assembler.currentKernel].savedSection = assembler.currentSection;
    
    assembler.currentKernel = kernel;
    if (kernel != ASMKERN_GLOBAL)
        assembler.currentSection = kernelStates[kernel].savedSection;
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
        kernelStates[assembler.currentKernel].savedSection = assembler.currentSection;
    
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
        Kernel& newKernel = kernelStates[currentKcodeKernel];
        assembler.isaAssembler->setAllocatedRegisters(newKernel.allocRegs,
                            newKernel.allocRegFlags);
    }
}

void AsmROCmHandler::saveKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {   // save other state
        size_t regTypesNum;
        Kernel& oldKernel = kernelStates[currentKcodeKernel];
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
    
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Configuration outside kernel definition");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
}

void AsmROCmPseudoOps::doControlDirective(AsmROCmHandler& handler,
              const char* pseudoOpPlace, const char* linePtr)
{
}

void AsmROCmPseudoOps::setConfigValue(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr, ROCmConfigValueTarget target)
{
}

void AsmROCmPseudoOps::setConfigBoolValue(AsmROCmHandler& handler,
          const char* pseudoOpPlace, const char* linePtr, ROCmConfigValueTarget target)
{
}

void AsmROCmPseudoOps::setDimensions(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
}

void AsmROCmPseudoOps::doKCode(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
}

void AsmROCmPseudoOps::doKCodeEnd(AsmROCmHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
}

void AsmROCmPseudoOps::updateKCodeSel(AsmROCmHandler& handler,
                  const std::vector<cxuint>& oldset)
{
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
        case ROCMOP_CALL_CONVENTION:
            break;
        case ROCMOP_CODEVERSION:
            break;
        case ROCMOP_CONFIG:
            break;
        case ROCMOP_CONTROL_DIRECTIVE:
            break;
        case ROCMOP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR:
            break;
        case ROCMOP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR:
            break;
        case ROCMOP_DEBUGMODE:
            break;
        case ROCMOP_DIMS:
            break;
        case ROCMOP_DX10CLAMP:
            break;
        case ROCMOP_EXCEPTIONS:
            break;
        case ROCMOP_FLOATMODE:
            break;
        case ROCMOP_GDS_SEGMENT_SIZE:
            break;
        case ROCMOP_GROUP_SEGMENT_ALIGN:
            break;
        case ROCMOP_IEEEMODE:
            break;
        case ROCMOP_KCODE:
            break;
        case ROCMOP_KCODEEND:
            break;
        case ROCMOP_KERNARG_SEGMENT_ALIGN:
            break;
        case ROCMOP_KERNARG_SEGMENT_SIZE:
            break;
        case ROCMOP_KERNEL_CODE_ENTRY_OFFSET:
            break;
        case ROCMOP_KERNEL_CODE_PREFETCH_OFFSET:
            break;
        case ROCMOP_KERNEL_CODE_PREFETCH_SIZE:
            break;
        case ROCMOP_LOCALSIZE:
            break;
        case ROCMOP_MACHINE:
            break;
        case ROCMOP_MAX_SCRATCH_BACKING_MEMORY:
            break;
        case ROCMOP_PGMRSRC1:
            break;
        case ROCMOP_PGMRSRC2:
            break;
        case ROCMOP_PRIORITY:
            break;
        case ROCMOP_PRIVATE_SEGMENT_ALIGN:
            break;
        case ROCMOP_PRIVMODE:
            break;
        case ROCMOP_RESERVED_SGPR_COUNT:
            break;
        case ROCMOP_RESERVED_SGPR_FIRST:
            break;
        case ROCMOP_RESERVED_VGPR_COUNT:
            break;
        case ROCMOP_RESERVED_VGPR_FIRST:
            break;
        case ROCMOP_RUNTIME_LOADER_KERNEL_SYMBOL:
            break;
        case ROCMOP_SCRATCHBUFFER:
            break;
        case ROCMOP_SGPRSNUM:
            break;
        case ROCMOP_TGSIZE:
            break;
        case ROCMOP_USE_DEBUG_ENABLED:
            break;
        case ROCMOP_USE_DISPATCH_ID:
            break;
        case ROCMOP_USE_DISPATCH_PTR:
            break;
        case ROCMOP_USE_DYNAMIC_CALL_STACK:
            break;
        case ROCMOP_USE_FLAT_SCRATCH_INIT:
            break;
        case ROCMOP_USE_GRID_WORKGROUP_COUNT:
            break;
        case ROCMOP_USE_KERNARG_SEGMENT_PTR:
            break;
        case ROCMOP_USE_ORDERED_APPEND_GDS:
            break;
        case ROCMOP_USE_PRIVATE_SEGMENT_SIZE:
            break;
        case ROCMOP_USE_PTR64:
            break;
        case ROCMOP_USE_QUEUE_PTR:
            break;
        case ROCMOP_USE_PRIVATE_SEGMENT_BUFFER:
            break;
        case ROCMOP_USE_XNACK_ENABLED:
            break;
        case ROCMOP_USERDATANUM:
            break;
        case ROCMOP_VGPRSNUM:
            break;
        case ROCMOP_WAVEFRONT_SGPR_COUNT:
            break;
        case ROCMOP_WAVEFRONT_SIZE:
            break;
        case ROCMOP_WORKITEM_VGPR_COUNT:
            break;
        case ROCMOP_WORKGROUP_FBARRIER_COUNT:
            break;
        case ROCMOP_WORKGROUP_GROUP_SEGMENT_SIZE:
            break;
        case ROCMOP_WORKITEM_PRIVATE_SEGMENT_SIZE:
            break;
        default:
            return false;
    }
    return true;
}

bool AsmROCmHandler::prepareBinary()
{
    return false;
}

void AsmROCmHandler::writeBinary(std::ostream& os) const
{
}

void AsmROCmHandler::writeBinary(Array<cxbyte>& array) const
{
}
