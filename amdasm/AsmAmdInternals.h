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

#ifndef __CLRX_ASMAMDINTERNALS_H__
#define __CLRX_ASMAMDINTERNALS_H__

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

enum AmdConfigValueTarget
{
    AMDCVAL_SGPRSNUM,
    AMDCVAL_VGPRSNUM,
    AMDCVAL_PGMRSRC2,
    AMDCVAL_IEEEMODE,
    AMDCVAL_FLOATMODE,
    AMDCVAL_HWLOCAL,
    AMDCVAL_HWREGION,
    AMDCVAL_PRIVATEID,
    AMDCVAL_SCRATCHBUFFER,
    AMDCVAL_UAVPRIVATE,
    AMDCVAL_UAVID,
    AMDCVAL_CBID,
    AMDCVAL_PRINTFID,
    AMDCVAL_EARLYEXIT,
    AMDCVAL_CONDOUT,
    AMDCVAL_USECONSTDATA,
    AMDCVAL_USEPRINTF,
    AMDCVAL_TGSIZE,
    AMDCVAL_EXCEPTIONS
};

struct CLRX_INTERNAL AsmAmdPseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    // .global_data (go to global data)
    static void doGlobalData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // set compile options
    static void setCompileOptions(AsmAmdHandler& handler, const char* linePtr);
    // .driver_info
    static void setDriverInfo(AsmAmdHandler& handler, const char* linePtr);
    // .driver_version
    static void setDriverVersion(AsmAmdHandler& handler, const char* linePtr);
    // .get_driver_version
    static void getDriverVersion(AsmAmdHandler& handler, const char* linePtr);
    // .specified CAL note
    static void addCALNote(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, uint32_t calNoteId);
    // .calnote
    static void addCustomCALNote(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .metadata (open metadata)
    static void addMetadata(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .header
    static void addHeader(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .config (open configuration)
    static void doConfig(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    /// add any entry with two 32-bit integers
    static void doEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
              const char* linePtr, uint32_t requiredCalNoteIdMask, const char* entryName);
    /// add any entry with four 32-bit integers
    static void doUavEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /* dual pseudo-ops (for configured kernels and non-config kernels) */
    static void doCBId(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .condout
    static void doCondOut(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .early exit
    static void doEarlyExit(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .sampler
    static void doSampler(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    /* user configuration pseudo-ops */
    // .dims (dimensions)
    static void setDimensions(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    // parse argument (.arg) (used also by AMDOCL2 handler)
    static bool parseArg(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
         const std::unordered_set<CString>& argNamesSet, AmdKernelArgInput& arg, bool cl20);
    // .arg
    static void doArg(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // set config value
    static void setConfigValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target);
    
    // parse cws (reqd_work_group_size) (used also by AMDOCL2 handler)
    static bool parseCWS(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                     uint64_t* out);
    // set cws (reqd_work_group_size)
    static void setCWS(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // set boolean config value
    static void setConfigBoolValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target);
    // .userdata
    static void addUserData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
};

};

#endif
