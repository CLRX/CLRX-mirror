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

#ifndef __CLRX_DISASMINTERNALS_H__
#define __CLRX_DISASMINTERNALS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <ostream>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdasm/Disassembler.h>

namespace CLRX
{

// print data in bytes in assembler format (secondAlign add extra align)
extern CLRX_INTERNAL void printDisasmData(size_t size, const cxbyte* data,
              std::ostream& output, bool secondAlign = false);

// print data in 32-bit words in assembler format (secondAlign add extra align)
extern CLRX_INTERNAL void printDisasmDataU32(size_t size, const uint32_t* data,
             std::ostream& output, bool secondAlign = false);

// print data in string form in assembler format
extern CLRX_INTERNAL void printDisasmLongString(size_t size, const char* data,
            std::ostream& output, bool secondAlign = false);

// disassemble Amd OpenCL 1.0 binary input
extern CLRX_INTERNAL void disassembleAmd(std::ostream& output,
       const AmdDisasmInput* amdInput, ISADisassembler* isaDisassembler,
       size_t& sectionCount, Flags flags);

// disassemble Amd OpenCL 2.0 binary input
extern CLRX_INTERNAL void disassembleAmdCL2(std::ostream& output,
        const AmdCL2DisasmInput* amdCL2Input, ISADisassembler* isaDisassembler,
        size_t& sectionCount, Flags flags);

// disassemble ROCm binary input
extern CLRX_INTERNAL void disassembleROCm(std::ostream& output,
       const ROCmDisasmInput* rocmInput, ISADisassembler* isaDisassembler,
       Flags flags);

// dump AMDHSA configuration in assembler format
// amdshaPrefix - add extra prefix for gallium HSA config params
extern CLRX_INTERNAL void dumpAMDHSAConfig(std::ostream& output, cxuint maxSgprsNum,
             GPUArchitecture arch, const ROCmKernelConfig& config,
             bool amdhsaPrefix = false);
// disassemble code in AMDHSA layout (kernel config and kernel codes)
extern CLRX_INTERNAL void disassembleAMDHSACode(std::ostream& output,
            const std::vector<ROCmDisasmRegionInput>& regions,
            size_t codeSize, const cxbyte* code, ISADisassembler* isaDisassembler,
            Flags flags);

// disassemble Gallium binary input
extern CLRX_INTERNAL void disassembleGallium(std::ostream& output,
       const GalliumDisasmInput* galliumInput, ISADisassembler* isaDisassembler,
       Flags flags);

extern CLRX_INTERNAL const std::pair<const char*, KernelArgType> disasmArgTypeNameMap[74];

extern CLRX_INTERNAL const KernelArgType disasmGpuArgTypeTable[];

// dump kernel arguments for  kernel in AMD binaries (cl20 - OpenCL 2.0 binaries)
extern CLRX_INTERNAL void dumpAmdKernelArg(std::ostream& output,
           const AmdKernelArgInput& arg, bool cl20);

};

#endif
