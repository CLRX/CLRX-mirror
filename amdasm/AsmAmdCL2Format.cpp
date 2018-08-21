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
#include <stack>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include <CLRX/utils/GPUId.h>
#include "AsmROCmInternals.h"
#include "AsmAmdInternals.h"
#include "AsmAmdCL2Internals.h"

using namespace CLRX;

// all AmdCL2 pseudo-op names (sorted)
static const char* amdCL2PseudoOpNamesTbl[] =
{
    "acl_version", "arch_minor", "arch_stepping",
    "arg", "bssdata", "call_convention", "codeversion",
    "compile_options", "config", "control_directive",
    "cws", "debug_private_segment_buffer_sgpr",
    "debug_wavefront_private_segment_offset_sgpr",
    "debugmode", "default_hsa_features",
    "dims", "driver_version", "dx10clamp", "exceptions",
    "floatmode", "gds_segment_size", "gdssize", "get_driver_version",
    "globaldata", "group_segment_align",
    "hsaconfig", "hsalayout", "ieeemode", "inner",
    "isametadata", "kcode", "kcodeend", "kernarg_segment_align",
    "kernarg_segment_size", "kernel_code_entry_offset",
    "kernel_code_prefetch_offset", "kernel_code_prefetch_size",
    "localsize", "machine", "max_scratch_backing_memory",
    "metadata", "pgmrsrc1", "pgmrsrc2", "priority",
    "private_elem_size", "private_segment_align",
    "privmode", "reqd_work_group_size",
    "reserved_sgprs", "reserved_vgprs",
    "runtime_loader_kernel_symbol", "rwdata", "sampler",
    "samplerinit", "samplerreloc", "scratchbuffer", "setup",
    "setupargs", "sgprsnum", "stub", "tgsize",
    "use_debug_enabled", "use_dispatch_id",
    "use_dispatch_ptr", "use_dynamic_call_stack",
    "use_flat_scratch_init", "use_grid_workgroup_count",
    "use_kernarg_segment_ptr", "use_ordered_append_gds",
    "use_private_segment_buffer", "use_private_segment_size",
    "use_ptr64", "use_queue_ptr", "use_xnack_enabled",
    "useargs", "useenqueue", "usegeneric",
    "userdatanum", "usesetup", "vectypehint", "vgprsnum",
    "wavefront_sgpr_count", "wavefront_size",
    "work_group_size_hint", "workgroup_fbarrier_count",
    "workgroup_group_segment_size", "workitem_private_segment_size",
    "workitem_vgpr_count"
};

// all enums for AmdCL2 pseudo-ops
enum
{
    AMDCL2OP_ACL_VERSION = 0, AMDCL2OP_ARCH_MINOR, AMDCL2OP_ARCH_STEPPING,
    AMDCL2OP_ARG, AMDCL2OP_BSSDATA, AMDCL2OP_CALL_CONVENTION, AMDCL2OP_CODEVERSION, 
    AMDCL2OP_COMPILE_OPTIONS, AMDCL2OP_CONFIG, AMDCL2OP_CONTROL_DIRECTIVE,
    AMDCL2OP_CWS, AMDCL2OP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    AMDCL2OP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    AMDCL2OP_DEBUGMODE, AMDCL2OP_DEFAULT_HSA_FEATURES,
    AMDCL2OP_DIMS, AMDCL2OP_DRIVER_VERSION, AMDCL2OP_DX10CLAMP, AMDCL2OP_EXCEPTIONS,
    AMDCL2OP_FLOATMODE, AMDCL2OP_GDS_SEGMENT_SIZE,
    AMDCL2OP_GDSSIZE, AMDCL2OP_GET_DRIVER_VERSION,
    AMDCL2OP_GLOBALDATA, AMDCL2OP_GROUP_SEGMENT_ALIGN,
    AMDCL2OP_HSACONFIG, AMDCL2OP_HSALAYOUT, AMDCL2OP_IEEEMODE, AMDCL2OP_INNER,
    AMDCL2OP_ISAMETADATA, AMDCL2OP_KCODE, AMDCL2OP_KCODEEND,
    AMDCL2OP_KERNARG_SEGMENT_ALIGN,
    AMDCL2OP_KERNARG_SEGMENT_SIZE, AMDCL2OP_KERNEL_CODE_ENTRY_OFFSET,
    AMDCL2OP_KERNEL_CODE_PREFETCH_OFFSET,
    AMDCL2OP_KERNEL_CODE_PREFETCH_SIZE, AMDCL2OP_LOCALSIZE,
    AMDCL2OP_MACHINE, AMDCL2OP_MAX_SCRATCH_BACKING_MEMORY,
    AMDCL2OP_METADATA, AMDCL2OP_PGMRSRC1, AMDCL2OP_PGMRSRC2, AMDCL2OP_PRIORITY,
    AMDCL2OP_PRIVATE_ELEM_SIZE, AMDCL2OP_PRIVATE_SEGMENT_ALIGN,
    AMDCL2OP_PRIVMODE, AMDCL2OP_REQD_WORK_GROUP_SIZE,
    AMDCL2OP_RESERVED_SGPRS, AMDCL2OP_RESERVED_VGPRS,
    AMDCL2OP_RUNTIME_LOADER_KERNEL_SYMBOL, AMDCL2OP_RWDATA,
    AMDCL2OP_SAMPLER, AMDCL2OP_SAMPLERINIT,
    AMDCL2OP_SAMPLERRELOC, AMDCL2OP_SCRATCHBUFFER, AMDCL2OP_SETUP, AMDCL2OP_SETUPARGS,
    AMDCL2OP_SGPRSNUM, AMDCL2OP_STUB, AMDCL2OP_TGSIZE,
    AMDCL2OP_USE_DEBUG_ENABLED, AMDCL2OP_USE_DISPATCH_ID,
    AMDCL2OP_USE_DISPATCH_PTR, AMDCL2OP_USE_DYNAMIC_CALL_STACK,
    AMDCL2OP_USE_FLAT_SCRATCH_INIT, AMDCL2OP_USE_GRID_WORKGROUP_COUNT,
    AMDCL2OP_USE_KERNARG_SEGMENT_PTR, AMDCL2OP_USE_ORDERED_APPEND_GDS,
    AMDCL2OP_USE_PRIVATE_SEGMENT_BUFFER, AMDCL2OP_USE_PRIVATE_SEGMENT_SIZE,
    AMDCL2OP_USE_PTR64, AMDCL2OP_USE_QUEUE_PTR, AMDCL2OP_USE_XNACK_ENABLED,
    AMDCL2OP_USEARGS, AMDCL2OP_USEENQUEUE, AMDCL2OP_USEGENERIC,
    AMDCL2OP_USERDATANUM, AMDCL2OP_USESETUP, AMDCL2OP_VECTYPEHINT, AMDCL2OP_VGPRSNUM,
    AMDCL2OP_WAVEFRONT_SGPR_COUNT, AMDCL2OP_WAVEFRONT_SIZE,
    AMDCL2OP_WORK_GROUP_SIZE_HINT,
    AMDCL2OP_WORKGROUP_FBARRIER_COUNT, AMDCL2OP_WORKGROUP_GROUP_SEGMENT_SIZE,
    AMDCL2OP_WORKITEM_PRIVATE_SEGMENT_SIZE, AMDCL2OP_WORKITEM_VGPR_COUNT
};

void AsmAmdCL2Handler::Kernel::initializeKernelConfig()
{
    if (!hsaConfig)
    {
        hsaConfig.reset(new AsmAmdHsaKernelConfig{});
        hsaConfig->initialize();
    }
}

/*
 * AmdCL2Catalyst format handler
 */

AsmAmdCL2Handler::AsmAmdCL2Handler(Assembler& assembler) : AsmKcodeHandler(assembler),
        output{}, rodataSection(0), dataSection(ASMSECT_NONE), bssSection(ASMSECT_NONE), 
        samplerInitSection(ASMSECT_NONE), extraSectionCount(0),
        innerExtraSectionCount(0), hsaLayout(false)
{
    output.archMinor = output.archStepping = UINT32_MAX;
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    // add .rodata section (will be default)
    sections.push_back({ ASMKERN_INNER, AsmSectionType::DATA, ELFSECTID_RODATA,
            ".rodata" });
    savedSection = innerSavedSection = 0;
    // detect driver version once for using many times
    detectedDriverVersion = detectAmdDriverVersion();
}

AsmAmdCL2Handler::~AsmAmdCL2Handler()
{
    for (Kernel* kernel: kernelStates)
        delete kernel;
}

void AsmAmdCL2Handler::saveCurrentSection()
{
    /// save previous section
    if (assembler.currentKernel==ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else if (assembler.currentKernel==ASMKERN_INNER)
        innerSavedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentKernel]->savedSection = assembler.currentSection;
}

cxuint AsmAmdCL2Handler::getDriverVersion() const
{
    cxuint driverVersion = 0;
    if (output.driverVersion==0)
    {
        if (assembler.driverVersion==0) // just detect driver version
            driverVersion = detectedDriverVersion;
        else // from assembler setup
            driverVersion = assembler.driverVersion;
    }
    else
        driverVersion = output.driverVersion;
    return driverVersion;
}

void AsmAmdCL2Handler::restoreCurrentAllocRegs()
{
    if (hsaLayout)
        return;
    if (assembler.currentKernel!=ASMKERN_GLOBAL &&
        assembler.currentKernel!=ASMKERN_INNER &&
        assembler.currentSection==kernelStates[assembler.currentKernel]->codeSection)
        assembler.isaAssembler->setAllocatedRegisters(
                kernelStates[assembler.currentKernel]->allocRegs,
                kernelStates[assembler.currentKernel]->allocRegFlags);
}

void AsmAmdCL2Handler::saveCurrentAllocRegs()
{
    if (hsaLayout)
        return;
    if (assembler.currentKernel!=ASMKERN_GLOBAL &&
        assembler.currentKernel!=ASMKERN_INNER &&
        assembler.currentSection==kernelStates[assembler.currentKernel]->codeSection)
    {
        size_t num;
        cxuint* destRegs = kernelStates[assembler.currentKernel]->allocRegs;
        const cxuint* regs = assembler.isaAssembler->getAllocatedRegisters(num,
                       kernelStates[assembler.currentKernel]->allocRegFlags);
        std::copy(regs, regs + num, destRegs);
    }
}

AsmKernelId AsmAmdCL2Handler::addKernel(const char* kernelName)
{
    AsmKernelId thisKernel = output.kernels.size();
    AsmSectionId thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    /* add new kernel and their section (.text) */
    kernelStates.push_back(new Kernel(thisSection));
    if (!hsaLayout)
        sections.push_back({ thisKernel, AsmSectionType::CODE,
                            ELFSECTID_TEXT, ".text" });
    else
        sections.push_back({ thisKernel, AsmSectionType::AMDCL2_DUMMY,
                            ELFSECTID_UNDEF, nullptr });
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    if (!hsaLayout)
        assembler.isaAssembler->setAllocatedRegisters();
    return thisKernel;
}

AsmSectionId AsmAmdCL2Handler::addSection(const char* sectionName, AsmKernelId kernelId)
{
    const AsmSectionId thisSection = sections.size();
    
    if (::strcmp(sectionName, ".rodata")==0)
    {
        // .rodata section (main and in inner binary)
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global Data allowed only for new binary format");
        rodataSection = sections.size();
        sections.push_back({ ASMKERN_INNER,  AsmSectionType::DATA,
                ELFSECTID_RODATA, ".rodata" });
        kernelId = ASMKERN_INNER;
    }
    else if (::strcmp(sectionName, ".data")==0)
    {
        // .data section (main and in inner binary)
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global RWData allowed only for new binary format");
        dataSection = sections.size();
        sections.push_back({ ASMKERN_INNER,  AsmSectionType::AMDCL2_RWDATA,
                ELFSECTID_DATA, ".data" });
        kernelId = ASMKERN_INNER;
    }
    else if (::strcmp(sectionName, ".bss")==0)
    {
        // .bss section (main and in inner binary)
        if (getDriverVersion() < 191205)
            throw AsmFormatException("Global BSS allowed only for new binary format");
        bssSection = sections.size();
        sections.push_back({ ASMKERN_INNER,  AsmSectionType::AMDCL2_BSS,
                ELFSECTID_BSS, ".bss" });
        kernelId = ASMKERN_INNER;
    }
    else if (hsaLayout && ::strcmp(sectionName, ".text")==0)
    {
        codeSection = sections.size();
        sections.push_back({ ASMKERN_INNER,  AsmSectionType::CODE,
                ELFSECTID_TEXT, ".text" });
        kernelId = ASMKERN_INNER;
    }
    else if (kernelId == ASMKERN_GLOBAL)
    {
        // add extra section main binary
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
    else
    {
        // add inner section (even if we inside kernel)
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

AsmSectionId AsmAmdCL2Handler::getSectionId(const char* sectionName) const
{
    // if HSA layout always treat '.text' as main '.text'
    if (hsaLayout && ::strcmp(sectionName, ".text")==0)
        return codeSection;
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
    {
        if (::strcmp(sectionName, ".rodata")==0)
            return rodataSection;
        else if (::strcmp(sectionName, ".data")==0)
            return dataSection;
        else if (::strcmp(sectionName, ".bss")==0)
            return bssSection;
        // find extra section by name in main binary
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
        // find extra section by name in inner binary
        SectionMap::const_iterator it = innerExtraSectionMap.find(sectionName);
        if (it != innerExtraSectionMap.end())
            return it->second;
        return ASMSECT_NONE;
    }
    return 0;
}

void AsmAmdCL2Handler::setCurrentKernel(AsmKernelId kernel)
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

void AsmAmdCL2Handler::setCurrentSection(AsmSectionId sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    // check whether is allowed in current driver version
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

AsmFormatHandler::SectionInfo AsmAmdCL2Handler::getSectionInfo(AsmSectionId sectionId) const
{
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    AsmFormatHandler::SectionInfo info;
    info.type = sections[sectionId].type;
    info.flags = 0;
    if (info.type == AsmSectionType::CODE)
        // code is addressable and writeable
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
    else if (info.type == AsmSectionType::AMDCL2_BSS ||
            info.type == AsmSectionType::AMDCL2_RWDATA ||
            info.type == AsmSectionType::DATA)
    {
        // global data, rwdata and bss are relocatable sections (we set unresolvable flag)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_UNRESOLVABLE;
        if (info.type != AsmSectionType::AMDCL2_BSS)
            info.flags |= ASMSECT_WRITEABLE;
    }
    // any other section (except config) are absolute addressable and writeable
    else if (info.type != AsmSectionType::CONFIG)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    // config and kernel DUMMY section have no flags
    info.name = sections[sectionId].name;
    return info;
}

bool AsmAmdCL2Handler::isCodeSection() const
{
    return sections[assembler.currentSection].type == AsmSectionType::CODE;
}

AsmKcodeHandler::KernelBase& AsmAmdCL2Handler::getKernelBase(AsmKernelId index)
{ return *kernelStates[index]; }

size_t AsmAmdCL2Handler::getKernelsNum() const
{ return kernelStates.size(); }

void AsmAmdCL2Handler::handleLabel(const CString& label)
{
    if (hsaLayout)
        AsmKcodeHandler::handleLabel(label);
    else
        AsmFormatHandler::handleLabel(label);
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
    AsmParseUtils::setSymbolValue(handler.assembler, linePtr,
                    handler.getDriverVersion(), ASMSECT_ABS);
}

// go to inner binary
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
    // set current position for this section
    asmr.currentOutPos = asmr.sections[asmr.currentSection].getSize();
}

void AsmAmdCL2PseudoOps::doGlobalData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (handler.getDriverVersion() < 191205)
        PSEUDOOP_RETURN_BY_ERROR("Global Data allowed only for new binary format")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.rodataSection==ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
    
    if (handler.getDriverVersion() < 191205)
        PSEUDOOP_RETURN_BY_ERROR("Global RWData allowed only for new binary format")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.dataSection==ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
    
    if (handler.getDriverVersion() < 191205)
        PSEUDOOP_RETURN_BY_ERROR("Global BSS allowed only for new binary format")
    
    uint64_t sectionAlign = 0;
    bool good = true;
    // parse alignment
    skipSpacesToEnd(linePtr, end);
    if (linePtr+6<end && ::strncasecmp(linePtr, "align", 5)==0 && !isAlpha(linePtr[5]))
    {
        // if alignment (align=VALUE)
        linePtr+=5;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr=='=')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            const char* valuePtr = linePtr;
            if (getAbsoluteValueArg(asmr, sectionAlign, linePtr, true))
            {
                // check whether is power of 2
                if (sectionAlign!=0 && (1ULL<<(63-CLZ64(sectionAlign))) != sectionAlign)
                    ASM_NOTGOOD_BY_ERROR(valuePtr, "Alignment must be power of two or zero")
            }
            else
                good = false;
        }
        else
            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected '=' after 'align'")
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.bssSection==ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
    if (handler.getDriverVersion() < 191205)
        PSEUDOOP_RETURN_BY_ERROR("SamplerInit allowed only for new binary format")
    if (handler.output.samplerConfig)
        PSEUDOOP_RETURN_BY_ERROR("SamplerInit is illegal if sampler "
                    "definitions are present")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.samplerInitSection==ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (handler.getDriverVersion() < 191205)
        PSEUDOOP_RETURN_BY_ERROR("Sampler allowed only for new binary format")
    
    bool inMain = asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    if (!inMain)
    {
        if (linePtr == end)
            return; /* if no samplers */
        AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
        // parse many values (sampler ids)
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
    {
        // global sampler definitions
        if (handler.samplerInitSection!=ASMSECT_NONE)
            PSEUDOOP_RETURN_BY_ERROR("Illegal sampler definition if "
                    "samplerinit was defined")
        handler.output.samplerConfig = true;
        if (linePtr == end)
            return; /* if no samplers */
        // parse many values (sampler ids)
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
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of samplerreloc pseudo-op")
    if (handler.getDriverVersion() < 191205)
        PSEUDOOP_RETURN_BY_ERROR("SamplerReloc allowed only for new binary format")
    
    skipSpacesToEnd(linePtr, end);
    const char* offsetPlace = linePtr;
    uint64_t samplerId = 0;
    uint64_t offset = 0;
    AsmSectionId sectionId = 0;
    bool good = getAnyValueArg(asmr, offset, sectionId, linePtr);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    good |= getAbsoluteValueArg(asmr, samplerId, linePtr, true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (sectionId != ASMSECT_ABS && sectionId != handler.rodataSection)
        ASM_RETURN_BY_ERROR(offsetPlace, "Offset can be an absolute value "
                        "or globaldata place")
    // put to sampler offsets
    if (handler.output.samplerOffsets.size() <= samplerId)
        handler.output.samplerOffsets.resize(samplerId+1);
    handler.output.samplerOffsets[samplerId] = offset;
}

void AsmAmdCL2PseudoOps::doControlDirective(AsmAmdCL2Handler& handler,
              const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
        PSEUDOOP_RETURN_BY_ERROR("Kernel control directive can be defined "
                    "only inside kernel")
    AsmAmdCL2Handler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.metadataSection!=ASMSECT_NONE || kernel.isaMetadataSection!=ASMSECT_NONE ||
        kernel.setupSection!=ASMSECT_NONE || kernel.stubSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Control directive "
            "can't be defined if metadata,header,setup,stub section exists")
    if (kernel.configSection != ASMSECT_NONE && !kernel.useHsaConfig)
        // control directive only if hsa config
        PSEUDOOP_RETURN_BY_ERROR("Config and Control directive can't be mixed")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (kernel.ctrlDirSection == ASMSECT_NONE)
    {
        // create control directive if not exists
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel,
            AsmSectionType::AMDCL2_CONFIG_CTRL_DIRECTIVE,
            ELFSECTID_UNDEF, nullptr });
        kernel.ctrlDirSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, kernel.ctrlDirSection);
    handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    kernel.useHsaConfig = true;
    handler.output.kernels[asmr.currentKernel].hsaConfig = true;
}


void AsmAmdCL2PseudoOps::setConfigValue(AsmAmdCL2Handler& handler,
         const char* pseudoOpPlace, const char* linePtr, AmdCL2ConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    const bool useHsaConfig = handler.kernelStates[asmr.currentKernel]->useHsaConfig;
    if (!useHsaConfig && (target >= AMDCL2CVAL_ONLY_HSA_FIRST_PARAM ||
            target == AMDCL2CVAL_USERDATANUM))
        PSEUDOOP_RETURN_BY_ERROR("HSAConfig pseudo-op only in HSAConfig")
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value = BINGEN_NOTSUPPLIED;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    /* ranges checking */
    if (good)
    {
        if (useHsaConfig && target >= AMDCL2CVAL_HSA_FIRST_PARAM)
            // if hsa config
            good = AsmROCmPseudoOps::checkConfigValue(asmr, valuePlace, 
                    ROCmConfigValueTarget(cxuint(target) - AMDCL2CVAL_HSA_FIRST_PARAM),
                    value);
        else
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
                        snprintf(buf, 64,
                                 "Used SGPRs number out of range (0-%u)", maxSGPRsNum);
                        ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
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
                        snprintf(buf, 64,
                                 "Used VGPRs number out of range (0-%u)", maxVGPRsNum);
                        ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
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
                        ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
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
                        ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
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
    
    if (handler.kernelStates[asmr.currentKernel]->useHsaConfig &&
        target >= AMDCL2CVAL_HSA_FIRST_PARAM)
    {
        // if hsa config
        AsmAmdHsaKernelConfig& config =
                *(handler.kernelStates[asmr.currentKernel]->hsaConfig);
        
        AsmROCmPseudoOps::setConfigValueMain(config, ROCmConfigValueTarget(
                cxuint(target) - AMDCL2CVAL_HSA_FIRST_PARAM), value);
        return;
    }
    
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
            if (!useHsaConfig)
                config.localSize = value;
            else
            {
                // if HSA config choosen, set HSA config param
                AsmAmdHsaKernelConfig& hsaConfig = *(handler.
                                kernelStates[asmr.currentKernel]->hsaConfig);
                hsaConfig.workgroupGroupSegmentSize = value;
            }
            break;
        case AMDCL2CVAL_GDSSIZE:
            if (!useHsaConfig)
                config.gdsSize = value;
            else
            {
                // if HSA config choosen, set HSA config param
                AsmAmdHsaKernelConfig& hsaConfig = *(handler.
                                kernelStates[asmr.currentKernel]->hsaConfig);
                hsaConfig.gdsSegmentSize = value;
            }
            break;
        case AMDCL2CVAL_SCRATCHBUFFER:
            if (!useHsaConfig)
                config.scratchBufferSize = value;
            else
            {
                // if HSA config choosen, set HSA config param
                AsmAmdHsaKernelConfig& hsaConfig = *(handler.
                                kernelStates[asmr.currentKernel]->hsaConfig);
                hsaConfig.workitemPrivateSegmentSize = value;
            }
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
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    const bool useHsaConfig = handler.kernelStates[asmr.currentKernel]->useHsaConfig;
    if (useHsaConfig &&
        (target == AMDCL2CVAL_USESETUP || target == AMDCL2CVAL_USEARGS ||
         target == AMDCL2CVAL_USEENQUEUE || target == AMDCL2CVAL_USEGENERIC))
        PSEUDOOP_RETURN_BY_ERROR("Illegal config pseudo-op in HSAConfig")
    if (!useHsaConfig && target >= AMDCL2CVAL_ONLY_HSA_FIRST_PARAM)
        PSEUDOOP_RETURN_BY_ERROR("HSAConfig pseudo-op only in HSAConfig")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (useHsaConfig)
    {
        // if hsa config
        AsmAmdHsaKernelConfig& config =
                *(handler.kernelStates[asmr.currentKernel]->hsaConfig);
        
        AsmROCmPseudoOps::setConfigBoolValueMain(config, ROCmConfigValueTarget(
                cxuint(target) - AMDCL2CVAL_HSA_FIRST_PARAM));
        return;
    }
    
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

void AsmAmdCL2PseudoOps::setDefaultHSAFeatures(AsmAmdCL2Handler& handler,
                const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (!handler.kernelStates[asmr.currentKernel]->useHsaConfig)
        PSEUDOOP_RETURN_BY_ERROR("HSAConfig pseudo-op only in HSAConfig")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmAmdHsaKernelConfig* config =
            handler.kernelStates[asmr.currentKernel]->hsaConfig.get();
    config->enableSgprRegisterFlags = uint16_t(AMDHSAFLAG_USE_PRIVATE_SEGMENT_BUFFER|
                        AMDHSAFLAG_USE_KERNARG_SEGMENT_PTR);
    config->enableFeatureFlags = uint16_t((asmr._64bit ? AMDHSAFLAG_USE_PTR64 : 0) | 2);
}

void AsmAmdCL2PseudoOps::setDimensions(AsmAmdCL2Handler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    cxuint dimMask = 0;
    if (!parseDimensions(asmr, linePtr, dimMask, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (!handler.kernelStates[asmr.currentKernel]->useHsaConfig)
        handler.output.kernels[asmr.currentKernel].config.dimMask = dimMask;
    else // if HSA config
        handler.kernelStates[asmr.currentKernel]->hsaConfig->dimMask = dimMask;
}

void AsmAmdCL2PseudoOps::setMachine(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (!handler.kernelStates[asmr.currentKernel]->useHsaConfig)
        PSEUDOOP_RETURN_BY_ERROR("HSAConfig pseudo-op only in HSAConfig")
    
    uint16_t kindValue = 0, majorValue = 0;
    uint16_t minorValue = 0, steppingValue = 0;
    // use ROCm routine to parse HSA machine
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

void AsmAmdCL2PseudoOps::setCodeVersion(AsmAmdCL2Handler& handler,
                const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (!handler.kernelStates[asmr.currentKernel]->useHsaConfig)
        PSEUDOOP_RETURN_BY_ERROR("HSAConfig pseudo-op only in HSAConfig")
    
    uint16_t majorValue = 0, minorValue = 0;
    // use ROCm routine to parse HSA code version
    if (!AsmROCmPseudoOps::parseCodeVersion(asmr, linePtr, majorValue, minorValue))
        return;
    
    AsmAmdHsaKernelConfig* config =
            handler.kernelStates[asmr.currentKernel]->hsaConfig.get();
    config->amdCodeVersionMajor = majorValue;
    config->amdCodeVersionMinor = minorValue;
}

void AsmAmdCL2PseudoOps::setReservedXgprs(AsmAmdCL2Handler& handler,
                const char* pseudoOpPlace, const char* linePtr, bool inVgpr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (!handler.kernelStates[asmr.currentKernel]->useHsaConfig)
        PSEUDOOP_RETURN_BY_ERROR("HSAConfig pseudo-op only in HSAConfig")
    
    uint16_t gprFirst = 0, gprCount = 0;
    // use ROCm routine to parse HSA reserved GPRs
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


void AsmAmdCL2PseudoOps::setUseGridWorkGroupCount(AsmAmdCL2Handler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    if (!handler.kernelStates[asmr.currentKernel]->useHsaConfig)
        PSEUDOOP_RETURN_BY_ERROR("HSAConfig pseudo-op only in HSAConfig")
    
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

void AsmAmdCL2PseudoOps::setCWS(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t out[3] = { 0, 0, 0 };
    // parse CWS (1-3 values)
    if (!AsmAmdPseudoOps::parseCWS(asmr, pseudoOpPlace, linePtr, out))
        return;
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    // reqd_work_group_size
    config.reqdWorkGroupSize[0] = out[0];
    config.reqdWorkGroupSize[1] = out[1];
    config.reqdWorkGroupSize[2] = out[2];
}

void AsmAmdCL2PseudoOps::setWorkGroupSizeHint(AsmAmdCL2Handler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t out[3] = { 0, 0, 0 };
    // parse CWS (1-3 values)
    if (!AsmAmdPseudoOps::parseCWS(asmr, pseudoOpPlace, linePtr, out))
        return;
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    // work group size hint
    config.workGroupSizeHint[0] = out[0];
    config.workGroupSizeHint[1] = out[1];
    config.workGroupSizeHint[2] = out[2];
}

void AsmAmdCL2PseudoOps::setVecTypeHint(AsmAmdCL2Handler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    CString vecTypeHint;
    skipSpacesToEnd(linePtr, end);
    bool good = getNameArg(asmr, vecTypeHint, linePtr, "vectypehint", true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    config.vecTypeHint = vecTypeHint;
}

void AsmAmdCL2PseudoOps::doArg(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of kernel argument")
    
    auto& kernelState = *handler.kernelStates[asmr.currentKernel];
    AmdKernelArgInput argInput;
    // reuse code in AMDOCL handler (set CL2 to true, choosen OpenCL 2.0 path)
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

// first kernel arguments for 64-bit binaries
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

// first kernel arguments for 32-bit binaries
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
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of kernel argument")
    
    auto& kernelState = *handler.kernelStates[asmr.currentKernel];
    if (!kernelState.argNamesSet.empty())
        PSEUDOOP_RETURN_BY_ERROR("SetupArgs must be as first in argument list")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdCL2KernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    const IntAmdCL2KernelArg* argTable = asmr._64bit ? setupArgsTable64 : setupArgsTable32;
    // put this argument to kernel arguments
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
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
        PSEUDOOP_RETURN_BY_ERROR("Metadata can be defined only inside kernel")
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata can't be defined if configuration was defined")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmSectionId& metadataSection =
            handler.kernelStates[asmr.currentKernel]->metadataSection;
    if (metadataSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
        PSEUDOOP_RETURN_BY_ERROR("ISAMetadata can be defined only inside kernel")
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("ISAMetadata can't be defined if configuration "
                "was defined")
    if (handler.getDriverVersion() >= 191205)
        PSEUDOOP_RETURN_BY_ERROR("ISA Metadata allowed only for old binary format")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmSectionId& isaMDSection =
            handler.kernelStates[asmr.currentKernel]->isaMetadataSection;
    if (isaMDSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
        PSEUDOOP_RETURN_BY_ERROR("Setup can be defined only inside kernel")
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Setup can't be defined if configuration was defined")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmSectionId& setupSection = handler.kernelStates[asmr.currentKernel]->setupSection;
    if (setupSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
    
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
        PSEUDOOP_RETURN_BY_ERROR("Stub can be defined only inside kernel")
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Stub can't be defined if configuration was defined")
    if (handler.getDriverVersion() >= 191205)
        PSEUDOOP_RETURN_BY_ERROR("Stub allowed only for old binary format")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmSectionId& stubSection = handler.kernelStates[asmr.currentKernel]->stubSection;
    if (stubSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMDCL2_STUB,
            ELFSECTID_UNDEF, nullptr });
        stubSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, stubSection);
}

void AsmAmdCL2PseudoOps::doConfig(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool hsaConfig)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL || asmr.currentKernel==ASMKERN_INNER)
        PSEUDOOP_RETURN_BY_ERROR("Kernel config can be defined only inside kernel")
    AsmAmdCL2Handler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.metadataSection!=ASMSECT_NONE || kernel.isaMetadataSection!=ASMSECT_NONE ||
        kernel.setupSection!=ASMSECT_NONE || kernel.stubSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Config can't be defined if metadata,header,setup,"
                        "stub section exists")
    if (kernel.configSection != ASMSECT_NONE && kernel.useHsaConfig != hsaConfig)
        // if config defined and doesn't match type of config
        PSEUDOOP_RETURN_BY_ERROR("Config and HSAConfig can't be mixed")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (kernel.configSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::CONFIG,
            ELFSECTID_UNDEF, nullptr });
        kernel.configSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, kernel.configSection);
    kernel.useHsaConfig = hsaConfig;
    if (hsaConfig)
        handler.kernelStates[asmr.currentKernel]->initializeKernelConfig();
    handler.output.kernels[asmr.currentKernel].hsaConfig = hsaConfig;
    handler.output.kernels[asmr.currentKernel].useConfig = true;
}

void AsmAmdCL2PseudoOps::doHSALayout(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                        const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (!handler.kernelStates.empty())
        PSEUDOOP_RETURN_BY_ERROR("HSALayout must be enabled before any kernel")
    if (handler.getDriverVersion() < 191205)
        PSEUDOOP_RETURN_BY_ERROR("HSALayout mode is illegal in old binary format");
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.hsaLayout = true;
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
        case AMDCL2OP_CALL_CONVENTION:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_CALL_CONVENTION);
            break;
        case AMDCL2OP_CODEVERSION:
            AsmAmdCL2PseudoOps::setCodeVersion(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_COMPILE_OPTIONS:
            AsmAmdCL2PseudoOps::setCompileOptions(*this, linePtr);
            break;
        case AMDCL2OP_CONFIG:
            AsmAmdCL2PseudoOps::doConfig(*this, stmtPlace, linePtr, false);
            break;
        case AMDCL2OP_CONTROL_DIRECTIVE:
            AsmAmdCL2PseudoOps::doControlDirective(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_CWS:
        case AMDCL2OP_REQD_WORK_GROUP_SIZE:
            AsmAmdCL2PseudoOps::setCWS(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR);
            break;
        case AMDCL2OP_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                         AMDCL2CVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR);
            break;
        case AMDCL2OP_DEBUGMODE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_DEBUGMODE);
            break;
        case AMDCL2OP_DEFAULT_HSA_FEATURES:
            AsmAmdCL2PseudoOps::setDefaultHSAFeatures(*this, stmtPlace, linePtr);
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
        case AMDCL2OP_GDS_SEGMENT_SIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_GDS_SEGMENT_SIZE);
            break;
        case AMDCL2OP_GROUP_SEGMENT_ALIGN:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_GROUP_SEGMENT_ALIGN);
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
        case AMDCL2OP_HSACONFIG:
            AsmAmdCL2PseudoOps::doConfig(*this, stmtPlace, linePtr, true);
            break;
        case AMDCL2OP_HSALAYOUT:
            AsmAmdCL2PseudoOps::doHSALayout(*this, stmtPlace, linePtr);
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
        case AMDCL2OP_KCODE:
            AsmKcodePseudoOps::doKCode(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_KCODEEND:
            AsmKcodePseudoOps::doKCodeEnd(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_KERNARG_SEGMENT_ALIGN:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_KERNARG_SEGMENT_ALIGN);
            break;
        case AMDCL2OP_KERNARG_SEGMENT_SIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_KERNARG_SEGMENT_SIZE);
            break;
        case AMDCL2OP_KERNEL_CODE_ENTRY_OFFSET:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_KERNEL_CODE_ENTRY_OFFSET);
            break;
        case AMDCL2OP_KERNEL_CODE_PREFETCH_OFFSET:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_KERNEL_CODE_PREFETCH_OFFSET);
            break;
        case AMDCL2OP_KERNEL_CODE_PREFETCH_SIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_KERNEL_CODE_PREFETCH_SIZE);
            break;
        case AMDCL2OP_LOCALSIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_LOCALSIZE);
            break;
        case AMDCL2OP_MACHINE:
            AsmAmdCL2PseudoOps::setMachine(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_MAX_SCRATCH_BACKING_MEMORY:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_MAX_SCRATCH_BACKING_MEMORY);
            break;
        case AMDCL2OP_METADATA:
            AsmAmdCL2PseudoOps::addMetadata(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_PRIVATE_ELEM_SIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_PRIVATE_ELEM_SIZE);
            break;
        case AMDCL2OP_PRIVATE_SEGMENT_ALIGN:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_PRIVATE_SEGMENT_ALIGN);
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
        case AMDCL2OP_RESERVED_SGPRS:
            AsmAmdCL2PseudoOps::setReservedXgprs(*this, stmtPlace, linePtr, false);
            break;
        case AMDCL2OP_RESERVED_VGPRS:
            AsmAmdCL2PseudoOps::setReservedXgprs(*this, stmtPlace, linePtr, true);
            break;
        case AMDCL2OP_RUNTIME_LOADER_KERNEL_SYMBOL:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_RUNTIME_LOADER_KERNEL_SYMBOL);
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
        case AMDCL2OP_USE_DEBUG_ENABLED:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_DEBUG_ENABLED);
            break;
        case AMDCL2OP_USE_DISPATCH_ID:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_DISPATCH_ID);
            break;
        case AMDCL2OP_USE_DISPATCH_PTR:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_DISPATCH_PTR);
            break;
        case AMDCL2OP_USE_DYNAMIC_CALL_STACK:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_DYNAMIC_CALL_STACK);
            break;
        case AMDCL2OP_USE_FLAT_SCRATCH_INIT:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_FLAT_SCRATCH_INIT);
            break;
        case AMDCL2OP_USE_GRID_WORKGROUP_COUNT:
            AsmAmdCL2PseudoOps::setUseGridWorkGroupCount(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_USE_KERNARG_SEGMENT_PTR:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_KERNARG_SEGMENT_PTR);
            break;
        case AMDCL2OP_USE_ORDERED_APPEND_GDS:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_ORDERED_APPEND_GDS);
            break;
        case AMDCL2OP_USE_PRIVATE_SEGMENT_SIZE:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_PRIVATE_SEGMENT_SIZE);
            break;
        case AMDCL2OP_USE_PTR64:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_PTR64);
            break;
        case AMDCL2OP_USE_QUEUE_PTR:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_QUEUE_PTR);
            break;
        case AMDCL2OP_USE_PRIVATE_SEGMENT_BUFFER:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_PRIVATE_SEGMENT_BUFFER);
            break;
        case AMDCL2OP_USE_XNACK_ENABLED:
            AsmAmdCL2PseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_USE_XNACK_ENABLED);
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
        case AMDCL2OP_USERDATANUM:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_USERDATANUM);
            break;
        case AMDCL2OP_VECTYPEHINT:
            AsmAmdCL2PseudoOps::setVecTypeHint(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_VGPRSNUM:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                       AMDCL2CVAL_VGPRSNUM);
            break;
        case AMDCL2OP_WAVEFRONT_SGPR_COUNT:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_WAVEFRONT_SGPR_COUNT);
            break;
        case AMDCL2OP_WAVEFRONT_SIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_WAVEFRONT_SIZE);
            break;
        case AMDCL2OP_WORK_GROUP_SIZE_HINT:
            AsmAmdCL2PseudoOps::setWorkGroupSizeHint(*this, stmtPlace, linePtr);
            break;
        case AMDCL2OP_WORKITEM_VGPR_COUNT:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_WORKITEM_VGPR_COUNT);
            break;
        case AMDCL2OP_WORKGROUP_FBARRIER_COUNT:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_WORKGROUP_FBARRIER_COUNT);
            break;
        case AMDCL2OP_WORKGROUP_GROUP_SEGMENT_SIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_WORKGROUP_GROUP_SEGMENT_SIZE);
            break;
        case AMDCL2OP_WORKITEM_PRIVATE_SEGMENT_SIZE:
            AsmAmdCL2PseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                             AMDCL2CVAL_WORKITEM_PRIVATE_SEGMENT_SIZE);
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
    
    if (hsaLayout)
        prepareKcodeState();
    
    // set sections as outputs
    for (size_t i = 0; i < sectionsNum; i++)
    {
        const AsmSection& asmSection = assembler.sections[i];
        const Section& section = sections[i];
        const size_t sectionSize = asmSection.getSize();
        const cxbyte* sectionData = (!asmSection.content.empty()) ?
                asmSection.content.data() : (const cxbyte*)"";
        AmdCL2KernelInput* kernel = (section.kernelId!=ASMKERN_GLOBAL &&
                    section.kernelId!=ASMKERN_INNER) ?
                    &output.kernels[section.kernelId] : nullptr;
        
        switch(asmSection.type)
        {
            case AsmSectionType::CODE:
                if (!hsaLayout)
                {
                    kernel->codeSize = sectionSize;
                    kernel->code = sectionData;
                }
                else
                {
                    output.codeSize = sectionSize;
                    output.code = sectionData;
                }
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
            case AsmSectionType::AMDCL2_CONFIG_CTRL_DIRECTIVE:
                if (sectionSize != 128)
                    assembler.printError(AsmSourcePos(),
                         (std::string("Section '.control_directive' for kernel '")+
                          assembler.kernels[section.kernelId].name+
                          "' have wrong size").c_str());
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
                // put extra sections to binary
                if (section.kernelId == ASMKERN_GLOBAL)
                    output.extraSections.push_back({section.name, sectionSize, sectionData,
                            asmSection.alignment!=0?asmSection.alignment:1, elfSectType,
                            elfSectFlags, ELFSECTID_NULL, 0, 0 });
                else // to inner binary
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
    // max SGPR number for architecture
    cxuint maxTotalSgprsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    // driver version setup
    if (output.driverVersion==0 && (assembler.flags&ASM_TESTRUN)==0)
    {
        if (assembler.driverVersion==0) // just detect driver version
            output.driverVersion = detectedDriverVersion;
        else // from assembler setup
            output.driverVersion = assembler.driverVersion;
    }
    
    // set up number of the allocated SGPRs and VGPRs for kernel
    for (size_t i = 0; i < kernelsNum; i++)
    {
        if (!output.kernels[i].useConfig)
        {
            if (hsaLayout && output.kernels[i].setup == nullptr)
                output.kernels[i].setupSize = 256;
            continue;
        }
        
        if (!kernelStates[i]->useHsaConfig)
        {
            // previous form of config (non-HSA)
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
                    getDefaultDimMask(arch, config.pgmRSRC2);
            cxuint minRegsNum[2];
            getGPUSetupMinRegistersNum(arch, dimMask, userSGPRsNum,
                    ((config.tgSize) ? GPUSETUP_TGSIZE_EN : 0) |
                    ((config.scratchBufferSize!=0) ? GPUSETUP_SCRATCH_EN : 0), minRegsNum);
            
            const cxuint neededExtraSGPRsNum = arch>=GPUArchitecture::GCN1_2 ? 6 : 4;
            const cxuint extraSGPRsNum = (config.useEnqueue || config.useGeneric) ?
                        neededExtraSGPRsNum : 2;
            if (config.usedSGPRsNum!=BINGEN_DEFAULT)
            {
                if (assembler.policyVersion >= ASM_POLICY_UNIFIED_SGPR_COUNT)
                    config.usedSGPRsNum = std::max(extraSGPRsNum, config.usedSGPRsNum) -
                            extraSGPRsNum;
                // check only if sgprsnum set explicitly
                if (maxTotalSgprsNum-extraSGPRsNum < config.usedSGPRsNum)
                {
                    char numBuf[64];
                    snprintf(numBuf, 64, "(max %u)", maxTotalSgprsNum);
                    assembler.printError(assembler.kernels[i].sourcePos, (std::string(
                        "Number of total SGPRs for kernel '")+
                        output.kernels[i].kernelName.c_str()+"' is too high "
                        +numBuf).c_str());
                    good = false;
                }
            }
            // set up used registers if not set by user
            if (config.usedSGPRsNum==BINGEN_DEFAULT)
                config.usedSGPRsNum = std::min(maxTotalSgprsNum-extraSGPRsNum, 
                    std::max(minRegsNum[0], kernelStates[i]->allocRegs[0]));
            if (config.usedVGPRsNum==BINGEN_DEFAULT)
                config.usedVGPRsNum = std::max(minRegsNum[1],
                                kernelStates[i]->allocRegs[1]);
            
            output.kernels[i].setupSize = 256;
        }
        else
        {
            // setup HSA configuration
            AsmAmdHsaKernelConfig& config = *kernelStates[i]->hsaConfig.get();
            const CString& kernelName = output.kernels[i].kernelName;
            
            const Kernel& kernel = *kernelStates[i];
            
            // setup some params: pgmRSRC1 and PGMRSRC2 and others
            // setup config
            // fill default values
            if (config.amdCodeVersionMajor == BINGEN_DEFAULT)
                config.amdCodeVersionMajor = 1;
            if (config.amdCodeVersionMinor == BINGEN_DEFAULT)
                config.amdCodeVersionMinor = 0;
            if (config.amdMachineKind == BINGEN16_DEFAULT)
                config.amdMachineKind = 1;
            if (config.amdMachineMajor == BINGEN16_DEFAULT)
                config.amdMachineMajor = 0;
            if (config.amdMachineMinor == BINGEN16_DEFAULT)
                config.amdMachineMinor = 0;
            if (config.amdMachineStepping == BINGEN16_DEFAULT)
                config.amdMachineStepping = 0;
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
                // calculate kernel arg size
                const bool newBinaries = output.driverVersion >= 191205;
                config.kernargSegmentSize = 
                        output.kernels[i].config.calculateKernelArgSize(
                            assembler.is64Bit(), newBinaries);
            }
            if (config.workgroupFbarrierCount == BINGEN_DEFAULT)
                config.workgroupFbarrierCount = 0;
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
                    ((sgprFlags&AMDHSAFLAG_USE_PRIVATE_SEGMENT_BUFFER)!=0 ? 4 : 0) +
                    ((sgprFlags&AMDHSAFLAG_USE_DISPATCH_PTR)!=0 ? 2 : 0) +
                    ((sgprFlags&AMDHSAFLAG_USE_QUEUE_PTR)!=0 ? 2 : 0) +
                    ((sgprFlags&AMDHSAFLAG_USE_KERNARG_SEGMENT_PTR)!=0 ? 2 : 0) +
                    ((sgprFlags&AMDHSAFLAG_USE_DISPATCH_ID)!=0 ? 2 : 0) +
                    ((sgprFlags&AMDHSAFLAG_USE_FLAT_SCRATCH_INIT)!=0 ? 2 : 0) +
                    ((sgprFlags&AMDHSAFLAG_USE_PRIVATE_SEGMENT_SIZE)!=0) +
                    /* use_grid_workgroup_count */
                    ((sgprFlags&AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_X)!=0) +
                    ((sgprFlags&AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Y)!=0) +
                    ((sgprFlags&AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Z)!=0);
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
            
            if (config.usedSGPRsNum!=BINGEN_DEFAULT &&
                    maxTotalSgprsNum < config.usedSGPRsNum)
            {
                // check only if sgprsnum set explicitly
                char numBuf[64];
                snprintf(numBuf, 64, "(max %u)", maxTotalSgprsNum);
                assembler.printError(assembler.kernels[i].sourcePos, (std::string(
                        "Number of total SGPRs for kernel '")+
                        kernelName.c_str()+"' is too high "+numBuf).c_str());
                good = false;
            }
            // set usedSGPRsNum
            cxuint progSgprsNum = 0;
            cxuint flags = kernelStates[i]->allocRegFlags |
                // flat_scratch_init
                ((config.enableSgprRegisterFlags&AMDHSAFLAG_USE_FLAT_SCRATCH_INIT)!=0?
                            GCN_FLAT : 0) |
                // enable_xnack
                ((config.enableFeatureFlags&AMDHSAFLAG_USE_XNACK_ENABLED)!=0 ?
                            GCN_XNACK : 0);
            cxuint extraSgprsNum = getGPUExtraRegsNum(arch, REGTYPE_SGPR, flags|GCN_VCC);
            
            if (config.usedSGPRsNum==BINGEN_DEFAULT)
            {
                progSgprsNum = std::max(minRegsNum[0], kernelStates[i]->allocRegs[0]);
                config.usedSGPRsNum = std::min(progSgprsNum + extraSgprsNum,
                        maxTotalSgprsNum); // include all extra sgprs
            }
            else // calculate progs SPGRs num (without extra SGPR num)
                progSgprsNum = std::max(cxint(config.usedSGPRsNum - extraSgprsNum), 0);
            
            // set usedVGPRsNum
            if (config.usedVGPRsNum==BINGEN_DEFAULT)
                config.usedVGPRsNum = std::max(minRegsNum[1],
                                kernelStates[i]->allocRegs[1]);
            
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
                            0, config.exceptions);
            
            if (config.wavefrontSgprCount == BINGEN16_DEFAULT)
                config.wavefrontSgprCount = sgprsNum;
            if (config.workitemVgprCount == BINGEN16_DEFAULT)
                config.workitemVgprCount = vgprsNum;
            if (config.reservedVgprFirst == BINGEN16_DEFAULT)
                config.reservedVgprFirst = vgprsNum;
            if (config.reservedVgprCount == BINGEN16_DEFAULT)
                config.reservedVgprCount = 0;
            if (config.reservedSgprFirst == BINGEN16_DEFAULT)
                config.reservedSgprFirst = progSgprsNum;
            if (config.reservedSgprCount == BINGEN16_DEFAULT)
                config.reservedSgprCount = 0;
            
            if (config.runtimeLoaderKernelSymbol == BINGEN64_DEFAULT)
                config.runtimeLoaderKernelSymbol = 0;
            
            config.reserved1[0] = config.reserved1[1] = config.reserved1[2] = 0;
            
            config.toLE(); // to little-endian
            // put control directive section to config
            if (kernel.ctrlDirSection!=ASMSECT_NONE &&
                assembler.sections[kernel.ctrlDirSection].content.size()==128)
                ::memcpy(config.controlDirective, 
                    assembler.sections[kernel.ctrlDirSection].content.data(), 128);
            else // zeroing if not supplied
                ::memset(config.controlDirective, 0, 128);
            
            output.kernels[i].setupSize = 256;
            output.kernels[i].setup = reinterpret_cast<cxbyte*>(&config);
        }
    }
    
    // set kernel code size and offset
    if (hsaLayout)
    {
        const AsmSymbolMap& symbolMap = assembler.getSymbolMap();
        
        for (size_t ki = 0; ki < kernelsNum; ki++)
        {
            AmdCL2KernelInput& kinput = output.kernels[ki];
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
        }
        
        // prepare sorted kernel indices by offset order
        Array<size_t> sortedKIndices(kernelsNum);
        for (size_t ki = 0; ki < kernelsNum; ki++)
            sortedKIndices[ki] = ki;
        std::sort(sortedKIndices.begin(), sortedKIndices.end(), [this]
                (size_t a, size_t b)
                { return output.kernels[a].offset < output.kernels[b].offset; });
        // set kernel code sizes
        for (size_t ki = 0; ki < kernelsNum; ki++)
        {
            const size_t end = ki+1 < kernelsNum ?
                    output.kernels[sortedKIndices[ki+1]].offset : output.codeSize;
            AmdCL2KernelInput& kinput = output.kernels[sortedKIndices[ki]];
            if ((end - kinput.offset) < kinput.setupSize)
            {
                assembler.printError(assembler.kernels[sortedKIndices[ki]].sourcePos,
                        (std::string("Kernel '")+kinput.kernelName.c_str()+
                            "' size is too small").c_str());
                good = false;
            }
            kinput.codeSize = (end - kinput.offset) - kinput.setupSize;
        }
    }
    
    /* put kernels relocations */
    for (const AsmRelocation& reloc: assembler.relocations)
    {
        /* put only code relocations */
        AsmKernelId kernelId = sections[reloc.sectionId].kernelId;
        cxuint symbol = sections[reloc.relSectionId].type==AsmSectionType::DATA ? 0 :
            (sections[reloc.relSectionId].type==AsmSectionType::AMDCL2_RWDATA ? 1 : 2);
        if (kernelId != ASMKERN_GLOBAL && kernelId != ASMKERN_INNER)
            output.kernels[kernelId].relocations.push_back({reloc.offset, reloc.type,
                        symbol, size_t(reloc.addend) });
        else
            output.relocations.push_back({reloc.offset, reloc.type, symbol,
                        size_t(reloc.addend) });
    }
    
    for (AmdCL2KernelInput& kernel: output.kernels)
        std::sort(kernel.relocations.begin(), kernel.relocations.end(),
                [](const AmdCL2RelInput& a, const AmdCL2RelInput& b)
                { return a.offset < b.offset; });
    
    /* put extra symbols */
    if (assembler.flags & ASM_FORCE_ADD_SYMBOLS)
    {
        std::vector<size_t> codeOffsets;
        size_t codeOffset = 0;
        // make offset translation table
        if (!hsaLayout)
        {
            codeOffsets.resize(kernelsNum, size_t(0));
            for (size_t i = 0; i < kernelsNum; i++)
            {
                const AmdCL2KernelInput& kernel = output.kernels[i];
                codeOffset += (kernel.useConfig) ? 256 : kernel.setupSize;
                codeOffsets[i] = codeOffset;
                codeOffset += (kernel.codeSize+255)&~size_t(255);
            }
        }
        
        // put symbols
        for (const AsmSymbolEntry& symEntry: assembler.globalScope.symbolMap)
        {
            if (!symEntry.second.hasValue ||
                ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // unresolved or local
            AsmSectionId binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            // create binSymbol
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
                // put to inner binary
                if (!hsaLayout)
                    binSym.value +=
                            codeOffsets[sections[symEntry.second.sectionId].kernelId];
                output.innerExtraSymbols.push_back(std::move(binSym));
            }
        }
    }
    return good;
}

bool AsmAmdCL2Handler::resolveSymbol(const AsmSymbol& symbol, uint64_t& value,
                 AsmSectionId& sectionId)
{
    if (!assembler.isResolvableSection(symbol.sectionId))
    {
        value = symbol.value;
        sectionId = symbol.sectionId;
        return true;
    }
    return false;
}

// routine to resolve relocations (given as expression)
bool AsmAmdCL2Handler::resolveRelocation(const AsmExpression* expr, uint64_t& outValue,
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
    // set up relocation (relSectionId, addend)
    reloc.relSectionId = relSectionId;
    reloc.addend = relValue;
    assembler.relocations.push_back(reloc);
    return true;
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
