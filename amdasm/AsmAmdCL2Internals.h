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
    // first HSA config param
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
    // first specific HSA config param
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
    
    // .arch_minor
    static void setArchMinor(AsmAmdCL2Handler& handler, const char* linePtr);
    // .arch_stepping
    static void setArchStepping(AsmAmdCL2Handler& handler, const char* linePtr);
    
    // .acl_version
    static void setAclVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    // .compile_options
    static void setCompileOptions(AsmAmdCL2Handler& handler, const char* linePtr);
    // .driver_version
    static void setDriverVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    // .get_driver_version (store driver version to symbol)
    static void getDriverVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    // go to inner binary (.inner)
    static void doInner(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .control_directive)
    static void doControlDirective(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .default_hsa_features
    static void setDefaultHSAFeatures(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .global_data (
    static void doGlobalData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .rwdata
    static void doRwData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .bssdata
    static void doBssData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .samplerinit (open samplerinit)
    static void doSamplerInit(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .samplerreloc
    static void doSamplerReloc(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .sampler
    static void doSampler(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // set config value
    static void setConfigValue(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
           const char* linePtr, AmdCL2ConfigValueTarget target);
    // set boolean config value
    static void setConfigBoolValue(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
           const char* linePtr, AmdCL2ConfigValueTarget target);
    // .dims (set dimensions)
    static void setDimensions(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .machine (set HSA machine)
    static void setMachine(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .machine (set HSA codeversion)
    static void setCodeVersion(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .reserved_sgprs or .reserved_vgprs (HSA reserved registers)
    static void setReservedXgprs(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool inVgpr);
    // .use_grid_work_group_count
    static void setUseGridWorkGroupCount(AsmAmdCL2Handler& handler,
                      const char* pseudoOpPlace, const char* linePtr);
    // .cws (set reqd_work_group_size or workgroupsizehint)
    static void setCWS(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .work_group_size_hint
    static void setWorkGroupSizeHint(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .vectypehint
    static void setVecTypeHint(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .arg (kernel argument)
    static void doArg(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .setupargs (add first kernel arguments)
    static void doSetupArgs(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                       const char* linePtr);
    // .metadata
    static void addMetadata(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .isametadata
    static void addISAMetadata(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .setup
    static void addKernelSetup(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .stub
    static void addKernelStub(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // open config (.config)
    static void doConfig(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool hsaConfig);
    
    // open config (.hsalayout)
    static void doHSALayout(AsmAmdCL2Handler& handler, const char* pseudoOOpPlace,
                    const char* linePtr);
};

};

#endif
