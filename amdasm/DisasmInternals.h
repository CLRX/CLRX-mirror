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

extern CLRX_INTERNAL void printDisasmData(size_t size, const cxbyte* data,
              std::ostream& output, bool secondAlign = false);

extern CLRX_INTERNAL void printDisasmDataU32(size_t size, const uint32_t* data,
             std::ostream& output, bool secondAlign = false);

extern CLRX_INTERNAL void printDisasmLongString(size_t size, const char* data,
            std::ostream& output, bool secondAlign = false);

extern CLRX_INTERNAL void disassembleAmd(std::ostream& output,
       const AmdDisasmInput* amdInput, ISADisassembler* isaDisassembler,
       size_t& sectionCount, Flags flags);

extern CLRX_INTERNAL void disassembleAmdCL2(std::ostream& output,
        const AmdCL2DisasmInput* amdCL2Input, ISADisassembler* isaDisassembler,
        size_t& sectionCount, Flags flags);

extern CLRX_INTERNAL void disassembleROCm(std::ostream& output,
       const ROCmDisasmInput* rocmInput, ISADisassembler* isaDisassembler,
       size_t& sectionCount, Flags flags);

extern CLRX_INTERNAL void disassembleGallium(std::ostream& output,
       const GalliumDisasmInput* galliumInput, ISADisassembler* isaDisassembler,
       size_t& sectionCount, Flags flags);

extern CLRX_INTERNAL AmdDisasmInput* getAmdDisasmInputFromBinary32(
            const AmdMainGPUBinary32& binary, Flags flags);

extern CLRX_INTERNAL AmdDisasmInput* getAmdDisasmInputFromBinary64(
            const AmdMainGPUBinary64& binary, Flags flags);

extern CLRX_INTERNAL AmdCL2DisasmInput* getAmdCL2DisasmInputFromBinary32(
            const AmdCL2MainGPUBinary32& binary, cxuint driverVersion);

extern CLRX_INTERNAL AmdCL2DisasmInput* getAmdCL2DisasmInputFromBinary64(
            const AmdCL2MainGPUBinary64& binary, cxuint driverVersion);

extern CLRX_INTERNAL ROCmDisasmInput* getROCmDisasmInputFromBinary(
            const ROCmBinary& binary);

extern CLRX_INTERNAL GalliumDisasmInput* getGalliumDisasmInputFromBinary(
            GPUDeviceType deviceType, const GalliumBinary& binary);

extern CLRX_INTERNAL const std::pair<const char*, KernelArgType> disasmArgTypeNameMap[74];

extern CLRX_INTERNAL const KernelArgType disasmGpuArgTypeTable[];

extern CLRX_INTERNAL void dumpAmdKernelArg(std::ostream& output,
           const AmdKernelArgInput& arg, bool cl20);

};

#endif
