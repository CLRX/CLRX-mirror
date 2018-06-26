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

#ifndef __CLRX_ASMGALLIUMINTERNALS_H__
#define __CLRX_ASMGALLIUMINTERNALS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <memory>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "GCNInternals.h"
#include "AsmInternals.h"

namespace CLRX
{

enum GalliumConfigValueTarget
{
    GALLIUMCVAL_SGPRSNUM,
    GALLIUMCVAL_VGPRSNUM,
    GALLIUMCVAL_PGMRSRC1,
    GALLIUMCVAL_PGMRSRC2,
    GALLIUMCVAL_IEEEMODE,
    GALLIUMCVAL_FLOATMODE,
    GALLIUMCVAL_LOCALSIZE,
    GALLIUMCVAL_SCRATCHBUFFER,
    GALLIUMCVAL_PRIORITY,
    GALLIUMCVAL_USERDATANUM,
    GALLIUMCVAL_TGSIZE,
    GALLIUMCVAL_DEBUGMODE,
    GALLIUMCVAL_DX10CLAMP,
    GALLIUMCVAL_PRIVMODE,
    GALLIUMCVAL_EXCEPTIONS,
    GALLIUMCVAL_SPILLEDSGPRS,
    GALLIUMCVAL_SPILLEDVGPRS,
    // AMD HSA config values
    GALLIUMCVAL_HSA_SGPRSNUM,
    GALLIUMCVAL_HSA_VGPRSNUM,
    GALLIUMCVAL_HSA_PRIVMODE,
    GALLIUMCVAL_HSA_DEBUGMODE,
    GALLIUMCVAL_HSA_DX10CLAMP,
    GALLIUMCVAL_HSA_IEEEMODE,
    GALLIUMCVAL_HSA_TGSIZE,
    GALLIUMCVAL_HSA_FLOATMODE,
    GALLIUMCVAL_HSA_PRIORITY,
    GALLIUMCVAL_HSA_EXCEPTIONS,
    GALLIUMCVAL_HSA_USERDATANUM,
    GALLIUMCVAL_HSA_PGMRSRC1,
    GALLIUMCVAL_HSA_PGMRSRC2,
    // specific AMDHSA config values
    GALLIUMCVAL_KERNEL_CODE_ENTRY_OFFSET,
    GALLIUMCVAL_KERNEL_CODE_PREFETCH_OFFSET,
    GALLIUMCVAL_KERNEL_CODE_PREFETCH_SIZE,
    GALLIUMCVAL_MAX_SCRATCH_BACKING_MEMORY,
    GALLIUMCVAL_USE_PRIVATE_SEGMENT_BUFFER,
    GALLIUMCVAL_USE_DISPATCH_PTR,
    GALLIUMCVAL_USE_QUEUE_PTR,
    GALLIUMCVAL_USE_KERNARG_SEGMENT_PTR,
    GALLIUMCVAL_USE_DISPATCH_ID,
    GALLIUMCVAL_USE_FLAT_SCRATCH_INIT,
    GALLIUMCVAL_USE_PRIVATE_SEGMENT_SIZE,
    GALLIUMCVAL_USE_ORDERED_APPEND_GDS,
    GALLIUMCVAL_PRIVATE_ELEM_SIZE,
    GALLIUMCVAL_USE_PTR64,
    GALLIUMCVAL_USE_DYNAMIC_CALL_STACK,
    GALLIUMCVAL_USE_DEBUG_ENABLED,
    GALLIUMCVAL_USE_XNACK_ENABLED,
    GALLIUMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE,
    GALLIUMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE,
    GALLIUMCVAL_GDS_SEGMENT_SIZE,
    GALLIUMCVAL_KERNARG_SEGMENT_SIZE,
    GALLIUMCVAL_WORKGROUP_FBARRIER_COUNT,
    GALLIUMCVAL_WAVEFRONT_SGPR_COUNT,
    GALLIUMCVAL_WORKITEM_VGPR_COUNT,
    GALLIUMCVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    GALLIUMCVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    GALLIUMCVAL_KERNARG_SEGMENT_ALIGN,
    GALLIUMCVAL_GROUP_SEGMENT_ALIGN,
    GALLIUMCVAL_PRIVATE_SEGMENT_ALIGN,
    GALLIUMCVAL_WAVEFRONT_SIZE,
    GALLIUMCVAL_CALL_CONVENTION,
    GALLIUMCVAL_RUNTIME_LOADER_KERNEL_SYMBOL,
    GALLIUMCVAL_HSA_FIRST_PARAM = GALLIUMCVAL_HSA_SGPRSNUM
};

struct CLRX_INTERNAL AsmGalliumPseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    // .arch_minor
    static void setArchMinor(AsmGalliumHandler& handler, const char* linePtr);
    // .arch_stepping
    static void setArchStepping(AsmGalliumHandler& handler, const char* linePtr);
    
    // .control_directive
    static void doControlDirective(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setDriverVersion(AsmGalliumHandler& handler, const char* linePtr);
    static void setLLVMVersion(AsmGalliumHandler& handler, const char* linePtr);
    
    // .get_driver_version, .get_llvm_version
    static void getXXXVersion(AsmGalliumHandler& handler, const char* linePtr,
                              bool getLLVMVersion);
    
    /* user configuration pseudo-ops */
    static void doConfig(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    // set condfiguration value given as integer
    static void setConfigValue(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, GalliumConfigValueTarget target);
    // set condfiguration value given as boolean
    static void setConfigBoolValue(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, GalliumConfigValueTarget target);
    // .default_hsa_features
    static void setDefaultHSAFeatures(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .dims (dimensions)
    static void setDimensions(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool amdHsa);
    // .machine (HSA machine)
    static void setMachine(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .code_version (HSA code version)
    static void setCodeVersion(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .reserved_sgprs, .reserved_vpgrs (in HSA config)
    static void setReservedXgprs(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool inVgpr);
    
    // .use_grid_work_group_count
    static void setUseGridWorkGroupCount(AsmGalliumHandler& handler,
                      const char* pseudoOpPlace, const char* linePtr);
    // .gloabal_data
    static void doGlobalData(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // open argument list
    static void doArgs(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // add argument info
    static void doArg(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /// open progInfo
    static void doProgInfo(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /// add progInfo entry
    static void doEntry(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void scratchSymbol(AsmGalliumHandler& handler, const char* linePtr);
};

};

#endif

