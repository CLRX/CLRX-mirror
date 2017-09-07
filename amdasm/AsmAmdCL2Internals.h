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

#ifndef __CLRX_ASMAMDCL2INTERNALS_H__
#define __CLRX_ASMAMDCL2INTERNALS_H__

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

enum AmdCL2ConfigValueTarget
{
    AMDCL2CVAL_LOCALSIZE,
    AMDCL2CVAL_GDSSIZE,
    AMDCL2CVAL_SCRATCHBUFFER,
    AMDCL2CVAL_USESETUP,
    AMDCL2CVAL_USEARGS,
    AMDCL2CVAL_USEENQUEUE,
    AMDCL2CVAL_USEGENERIC,
    AMDCL2CVAL_HSA_SGPRSNUM,
    AMDCL2CVAL_HSA_VGPRSNUM,
    AMDCL2CVAL_HSA_PRIVMODE,
    AMDCL2CVAL_HSA_DEBUGMODE,
    AMDCL2CVAL_HSA_DX10CLAMP,
    AMDCL2CVAL_HSA_IEEEMODE,
    AMDCL2CVAL_HSA_TGSIZE,
    AMDCL2CVAL_HSA_FLOATMODE,
    AMDCL2CVAL_HSA_PRIORITY,
    AMDCL2CVAL_HSA_EXCEPTIONS,
    AMDCL2CVAL_HSA_USERDATANUM,
    AMDCL2CVAL_HSA_PGMRSRC1,
    AMDCL2CVAL_HSA_PGMRSRC2,
    AMDCL2CVAL_KERNEL_CODE_ENTRY_OFFSET,
    AMDCL2CVAL_KERNEL_CODE_PREFETCH_OFFSET,
    AMDCL2CVAL_KERNEL_CODE_PREFETCH_SIZE,
    AMDCL2CVAL_MAX_SCRATCH_BACKING_MEMORY,
    AMDCL2CVAL_USE_PRIVATE_SEGMENT_BUFFER,
    AMDCL2CVAL_USE_DISPATCH_PTR,
    AMDCL2CVAL_USE_QUEUE_PTR,
    AMDCL2CVAL_USE_KERNARG_SEGMENT_PTR,
    AMDCL2CVAL_USE_DISPATCH_ID,
    AMDCL2CVAL_USE_FLAT_SCRATCH_INIT,
    AMDCL2CVAL_USE_PRIVATE_SEGMENT_SIZE,
    AMDCL2CVAL_USE_ORDERED_APPEND_GDS,
    AMDCL2CVAL_PRIVATE_ELEM_SIZE,
    AMDCL2CVAL_USE_PTR64,
    AMDCL2CVAL_USE_DYNAMIC_CALL_STACK,
    AMDCL2CVAL_USE_DEBUG_ENABLED,
    AMDCL2CVAL_USE_XNACK_ENABLED,
    AMDCL2CVAL_WORKITEM_PRIVATE_SEGMENT_SIZE,
    AMDCL2CVAL_WORKGROUP_GROUP_SEGMENT_SIZE,
    AMDCL2CVAL_GDS_SEGMENT_SIZE,
    AMDCL2CVAL_KERNARG_SEGMENT_SIZE,
    AMDCL2CVAL_WORKGROUP_FBARRIER_COUNT,
    AMDCL2CVAL_WAVEFRONT_SGPR_COUNT,
    AMDCL2CVAL_WORKITEM_VGPR_COUNT,
    AMDCL2CVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    AMDCL2CVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    AMDCL2CVAL_KERNARG_SEGMENT_ALIGN,
    AMDCL2CVAL_GROUP_SEGMENT_ALIGN,
    AMDCL2CVAL_PRIVATE_SEGMENT_ALIGN,
    AMDCL2CVAL_WAVEFRONT_SIZE,
    AMDCL2CVAL_CALL_CONVENTION,
    AMDCL2CVAL_RUNTIME_LOADER_KERNEL_SYMBOL,
    AMDCL2CVAL_HSA_FIRST_PARAM = AMDCL2CVAL_HSA_SGPRSNUM,
    AMDCL2CVAL_ONLY_HSA_FIRST_PARAM = AMDCL2CVAL_KERNEL_CODE_ENTRY_OFFSET,
    
    AMDCL2CVAL_SGPRSNUM = AMDCL2CVAL_HSA_SGPRSNUM,
    AMDCL2CVAL_VGPRSNUM = AMDCL2CVAL_HSA_VGPRSNUM,
    AMDCL2CVAL_PRIVMODE = AMDCL2CVAL_HSA_PRIVMODE,
    AMDCL2CVAL_DEBUGMODE = AMDCL2CVAL_HSA_DEBUGMODE,
    AMDCL2CVAL_DX10CLAMP = AMDCL2CVAL_HSA_DX10CLAMP,
    AMDCL2CVAL_IEEEMODE = AMDCL2CVAL_HSA_IEEEMODE,
    AMDCL2CVAL_TGSIZE = AMDCL2CVAL_HSA_TGSIZE,
    AMDCL2CVAL_FLOATMODE = AMDCL2CVAL_HSA_FLOATMODE,
    AMDCL2CVAL_PRIORITY = AMDCL2CVAL_HSA_PRIORITY,
    AMDCL2CVAL_EXCEPTIONS = AMDCL2CVAL_HSA_EXCEPTIONS,
    AMDCL2CVAL_USERDATANUM = AMDCL2CVAL_HSA_USERDATANUM,
    AMDCL2CVAL_PGMRSRC1 = AMDCL2CVAL_HSA_PGMRSRC1,
    AMDCL2CVAL_PGMRSRC2 = AMDCL2CVAL_HSA_PGMRSRC2
};

struct CLRX_INTERNAL AsmAmdCL2PseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    static void setArchMinor(AsmAmdCL2Handler& handler, const char* linePtr);
    static void setArchStepping(AsmAmdCL2Handler& handler, const char* linePtr);
    
    static void setAclVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    static void setCompileOptions(AsmAmdCL2Handler& handler, const char* linePtr);
    
    static void setDriverVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    
    static void getDriverVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    
    static void doInner(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doControlDirective(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setDefaultHSAFeatures(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doGlobalData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doRwData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doBssData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doSamplerInit(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doSamplerReloc(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doSampler(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setConfigValue(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
           const char* linePtr, AmdCL2ConfigValueTarget target);
    static void setConfigBoolValue(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
           const char* linePtr, AmdCL2ConfigValueTarget target);
    
    static void setDimensions(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setMachine(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setCodeVersion(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setReservedXgprs(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool inVgpr);
    
    static void setUseGridWorkGroupCount(AsmAmdCL2Handler& handler,
                      const char* pseudoOpPlace, const char* linePtr);
    
    static void setCWS(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doArg(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doSetupArgs(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                       const char* linePtr);
    
    static void addMetadata(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void addISAMetadata(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void addKernelSetup(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void addKernelStub(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doConfig(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool hsaConfig);
};

};

#endif
