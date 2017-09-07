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

#ifndef __CLRX_ASMROCMINTERNALS_H__
#define __CLRX_ASMROCMINTERNALS_H__

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

enum ROCmConfigValueTarget
{
    ROCMCVAL_SGPRSNUM,
    ROCMCVAL_VGPRSNUM,
    ROCMCVAL_PRIVMODE,
    ROCMCVAL_DEBUGMODE,
    ROCMCVAL_DX10CLAMP,
    ROCMCVAL_IEEEMODE,
    ROCMCVAL_TGSIZE,
    ROCMCVAL_FLOATMODE,
    ROCMCVAL_PRIORITY,
    ROCMCVAL_EXCEPTIONS,
    ROCMCVAL_USERDATANUM,
    ROCMCVAL_PGMRSRC1,
    ROCMCVAL_PGMRSRC2,
    ROCMCVAL_KERNEL_CODE_ENTRY_OFFSET,
    ROCMCVAL_KERNEL_CODE_PREFETCH_OFFSET,
    ROCMCVAL_KERNEL_CODE_PREFETCH_SIZE,
    ROCMCVAL_MAX_SCRATCH_BACKING_MEMORY,
    ROCMCVAL_USE_PRIVATE_SEGMENT_BUFFER,
    ROCMCVAL_USE_DISPATCH_PTR,
    ROCMCVAL_USE_QUEUE_PTR,
    ROCMCVAL_USE_KERNARG_SEGMENT_PTR,
    ROCMCVAL_USE_DISPATCH_ID,
    ROCMCVAL_USE_FLAT_SCRATCH_INIT,
    ROCMCVAL_USE_PRIVATE_SEGMENT_SIZE,
    ROCMCVAL_USE_ORDERED_APPEND_GDS,
    ROCMCVAL_PRIVATE_ELEM_SIZE,
    ROCMCVAL_USE_PTR64,
    ROCMCVAL_USE_DYNAMIC_CALL_STACK,
    ROCMCVAL_USE_DEBUG_ENABLED,
    ROCMCVAL_USE_XNACK_ENABLED,
    ROCMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE,
    ROCMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE,
    ROCMCVAL_GDS_SEGMENT_SIZE,
    ROCMCVAL_KERNARG_SEGMENT_SIZE,
    ROCMCVAL_WORKGROUP_FBARRIER_COUNT,
    ROCMCVAL_WAVEFRONT_SGPR_COUNT,
    ROCMCVAL_WORKITEM_VGPR_COUNT,
    ROCMCVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    ROCMCVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    ROCMCVAL_KERNARG_SEGMENT_ALIGN,
    ROCMCVAL_GROUP_SEGMENT_ALIGN,
    ROCMCVAL_PRIVATE_SEGMENT_ALIGN,
    ROCMCVAL_WAVEFRONT_SIZE,
    ROCMCVAL_CALL_CONVENTION,
    ROCMCVAL_RUNTIME_LOADER_KERNEL_SYMBOL
};

struct CLRX_INTERNAL AsmROCmPseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    static void setArchMinor(AsmROCmHandler& handler, const char* linePtr);
    static void setArchStepping(AsmROCmHandler& handler, const char* linePtr);
    
    /* user configuration pseudo-ops */
    static void doConfig(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doControlDirective(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doFKernel(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static bool checkConfigValue(Assembler& asmr, const char* valuePlace,
                    ROCmConfigValueTarget target, uint64_t value);
    static void setConfigValueMain(AsmAmdHsaKernelConfig& config,
                        ROCmConfigValueTarget target, uint64_t value);
    
    static void setConfigValue(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, ROCmConfigValueTarget target);
    
    static void setConfigBoolValueMain(AsmAmdHsaKernelConfig& config,
                        ROCmConfigValueTarget target);
    
    static void setConfigBoolValue(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, ROCmConfigValueTarget target);
    
    static void setDefaultHSAFeatures(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setDimensions(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static bool parseMachine(Assembler& asmr, const char* linePtr,
                uint16_t& machineKind, uint16_t& machineMajor, uint16_t& machineMinor,
                uint16_t& machineStepping);
    
    static void setMachine(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static bool parseCodeVersion(Assembler& asmr, const char* linePtr,
                uint16_t& codeMajor, uint16_t& codeMinor);
    
    static void setCodeVersion(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static bool parseReservedXgprs(Assembler& asmr, const char* linePtr, bool inVgpr,
                uint16_t& gprFirst, uint16_t& gprCount);
    
    static void setReservedXgprs(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool inVgpr);
    
    static void setUseGridWorkGroupCount(AsmROCmHandler& handler,
                      const char* pseudoOpPlace, const char* linePtr);
    
    static void doKCode(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doKCodeEnd(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void updateKCodeSel(AsmROCmHandler& handler,
                      const std::vector<cxuint>& oldset);
};

};

#endif
