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
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <ostream>
#include <memory>
#include <vector>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/GPUId.h>
#include "DisasmInternals.h"

using namespace CLRX;

// template for getting gallium input from gallium binary
template<typename GalliumElfBinary>
static void getGalliumDisasmInputFromBinaryBase(const GalliumBinary& binary,
            const GalliumElfBinary& elfBin, GalliumDisasmInput* input)
{
    uint16_t rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = elfBin.getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { }
    const uint16_t textIndex = elfBin.getSectionIndex(".text");
    
    // set up global data (is '.rodata' section)
    if (rodataIndex != SHN_UNDEF)
    {
        input->globalData = elfBin.getSectionContent(rodataIndex);
        input->globalDataSize = ULEV(elfBin.getSectionHeader(rodataIndex).sh_size);
    }
    else
    {
        input->globalDataSize = 0;
        input->globalData = nullptr;
    }
    input->isMesa170 = binary.isMesa170();
    input->isLLVM390 = elfBin.isLLVM390();
    const cxuint progInfoEntriesNum = input->isLLVM390 ? 5 : 3;
    // kernels
    input->kernels.resize(binary.getKernelsNum());
    for (cxuint i = 0; i < binary.getKernelsNum(); i++)
    {
        const GalliumKernel& kernel = binary.getKernel(i);
        const GalliumProgInfoEntry* progInfo = elfBin.getProgramInfo(i);
        GalliumProgInfoEntry outProgInfo[5];
        // get program info entries
        for (cxuint k = 0; k < progInfoEntriesNum; k++)
        {
            outProgInfo[k].address = ULEV(progInfo[k].address);
            outProgInfo[k].value = ULEV(progInfo[k].value);
        }
        input->kernels[i] = { kernel.kernelName, {}, kernel.offset,
            std::vector<GalliumArgInfo>(kernel.argInfos.begin(), kernel.argInfos.end()) };
        std::copy(outProgInfo, outProgInfo+progInfoEntriesNum, input->kernels[i].progInfo);
    }
    // set code
    input->code = elfBin.getSectionContent(textIndex);
    input->codeSize = ULEV(elfBin.getSectionHeader(textIndex).sh_size);
    // scratch relocations
    const Array<GalliumScratchReloc>& scratchRelocs = elfBin.getScratchRelocs();
    input->scratchRelocs.assign(scratchRelocs.begin(), scratchRelocs.end());
    std::sort(input->scratchRelocs.begin(), input->scratchRelocs.end(),
              [](const GalliumScratchReloc& r1, const GalliumScratchReloc& r2)
              { return r1.offset < r2.offset; });
}

GalliumDisasmInput* CLRX::getGalliumDisasmInputFromBinary(GPUDeviceType deviceType,
           const GalliumBinary& binary, cxuint llvmVersion)
{
    std::unique_ptr<GalliumDisasmInput> input(new GalliumDisasmInput);
    input->deviceType = deviceType;
    input->isAMDHSA = (llvmVersion >= 40000);
    if (!binary.is64BitElfBinary())
    {
        input->is64BitMode = false;
        getGalliumDisasmInputFromBinaryBase(binary, binary.getElfBinary32(), input.get());
    }
    else // 64-bit
    {
        input->is64BitMode = true;
        getGalliumDisasmInputFromBinaryBase(binary, binary.getElfBinary64(), input.get());
    }
    return input.release();
}

// kernel argument types names
static const char* galliumArgTypeNamesTbl[] =
{
    "scalar", "constant", "global", "local", "image2d_rd", "image2d_wr", "image3d_rd",
    "image3d_wr", "sampler"
};

// semantics names
static const char* galliumArgSemTypeNamesTbl[] =
{
    "general", "griddim", "gridoffset", "imgsize", "imgformat"
};

static void dumpKernelConfig(std::ostream& output, cxuint maxSgprsNum,
             GPUArchitecture arch, const GalliumProgInfoEntry* progInfo, bool isLLVM390)
{
    output.write("    .config\n", 12);
    size_t bufSize;
    char buf[100];
    const cxuint ldsShift = arch<GPUArchitecture::GCN1_1 ? 8 : 9;
    const uint32_t pgmRsrc1 = progInfo[0].value;
    const uint32_t pgmRsrc2 = progInfo[1].value;
    const uint32_t scratchVal = progInfo[2].value;
    const uint32_t spilledSGPRs = progInfo[3].value;
    const uint32_t spilledVGPRs = progInfo[4].value;
    
    const cxuint dimMask = getDefaultDimMask(arch, pgmRsrc2);
    // print .dims xyz (dimensions)
    strcpy(buf, "        .dims ");
    bufSize = 14;
    if ((dimMask & 1) != 0)
        buf[bufSize++] = 'x';
    if ((dimMask & 2) != 0)
        buf[bufSize++] = 'y';
    if ((dimMask & 4) != 0)
        buf[bufSize++] = 'z';
    if ((dimMask & 7) != ((dimMask>>3) & 7))
    {
        buf[bufSize++] = ',';
        buf[bufSize++] = ' ';
        if ((dimMask & 8) != 0)
            buf[bufSize++] = 'x';
        if ((dimMask & 16) != 0)
            buf[bufSize++] = 'y';
        if ((dimMask & 32) != 0)
            buf[bufSize++] = 'z';
    }
    buf[bufSize++] = '\n';
    output.write(buf, bufSize);
    
    // print SGPR and VGPR number from PGMRSRC1
    bufSize = snprintf(buf, 100, "        .sgprsnum %u\n",
              std::min((((pgmRsrc1>>6) & 0xf)<<3)+8, maxSgprsNum));
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .vgprsnum %u\n", ((pgmRsrc1 & 0x3f)<<2)+4);
    output.write(buf, bufSize);
    if ((pgmRsrc1 & (1U<<20)) != 0)
        output.write("        .privmode\n", 18);
    if ((pgmRsrc1 & (1U<<22)) != 0)
        output.write("        .debugmode\n", 19);
    if ((pgmRsrc1 & (1U<<21)) != 0)
        output.write("        .dx10clamp\n", 19);
    if ((pgmRsrc1 & (1U<<23)) != 0)
        output.write("        .ieeemode\n", 18);
    if ((pgmRsrc2 & 0x400) != 0)
        output.write("        .tgsize\n", 16);
    
    bufSize = snprintf(buf, 100, "        .floatmode 0x%02x\n", (pgmRsrc1>>12) & 0xff);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .priority %u\n", (pgmRsrc1>>10) & 3);
    output.write(buf, bufSize);
    if (((pgmRsrc1>>24) & 0x7f) != 0)
    {
        bufSize = snprintf(buf, 100, "        .exceptions 0x%02x\n",
                   (pgmRsrc1>>24) & 0x7f);
        output.write(buf, bufSize);
    }
    const cxuint localSize = ((pgmRsrc2>>15) & 0x1ff) << ldsShift;
    if (localSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .localsize %u\n", localSize);
        output.write(buf, bufSize);
    }
    bufSize = snprintf(buf, 100, "        .userdatanum %u\n", (pgmRsrc2>>1) & 0x1f);
    output.write(buf, bufSize);
    const cxuint scratchSize = ((scratchVal >> 12) << 10) >> 6;
    if (scratchSize != 0) // scratch buffer
    {
        bufSize = snprintf(buf, 100, "        .scratchbuffer %u\n", scratchSize);
        output.write(buf, bufSize);
    }
    bufSize = snprintf(buf, 100, "        .pgmrsrc1 0x%08x\n", pgmRsrc1);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .pgmrsrc2 0x%08x\n", pgmRsrc2);
    output.write(buf, bufSize);
    if (isLLVM390)
    {
        // extra info (spilled GPRs)
        bufSize = snprintf(buf, 100, "        .spilledsgprs %d\n", spilledSGPRs);
        output.write(buf, bufSize);
        bufSize = snprintf(buf, 100, "        .spilledvgprs %d\n", spilledVGPRs);
        output.write(buf, bufSize);
    }
}

void CLRX::disassembleGallium(std::ostream& output,
          const GalliumDisasmInput* galliumInput, ISADisassembler* isaDisassembler,
          Flags flags)
{
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doMetadata = ((flags & (DISASM_METADATA|DISASM_CONFIG)) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    const bool doDumpConfig = ((flags & DISASM_CONFIG) != 0);
    
    if (galliumInput->is64BitMode)
        output.write(".64bit\n", 7);
    else
        output.write(".32bit\n", 7);
    
    if (doDumpData && galliumInput->globalData != nullptr &&
        galliumInput->globalDataSize != 0)
    {   //
        output.write(".rodata\n", 8);
        printDisasmData(galliumInput->globalDataSize, galliumInput->globalData, output);
    }
    if (galliumInput->isMesa170)
        output.write(".driver_version 170000\n", 23);
    // print correct llvm version
    if (galliumInput->isAMDHSA)
        output.write(".llvm_version 40000\n", 20);
    else if (galliumInput->isLLVM390)
        output.write(".llvm_version 30900\n", 20);
    
    if (!galliumInput->scratchRelocs.empty())
        output.write(".scratchsym .scratchaddr\n", 25);
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(galliumInput->deviceType);
    const cxuint maxSgprsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    for (cxuint i = 0; i < galliumInput->kernels.size(); i++)
    {
        const GalliumDisasmKernelInput& kinput = galliumInput->kernels[i];
        {
            output.write(".kernel ", 8);
            output.write(kinput.kernelName.c_str(), kinput.kernelName.size());
            output.put('\n');
        }
        if (doMetadata)
        {
            char lineBuf[128];
            output.write("    .args\n", 10);
            for (const GalliumArgInfo& arg: kinput.argInfos)
            {
                // print kernel argument
                ::memcpy(lineBuf, "        .arg ", 13);
                size_t pos = 13;
                // arg format: .arg TYPENAME, SIZE, TARGETSIZE, TALIGN, NUMEXT, SEMANTIC
                if (arg.type <= GalliumArgType::MAX_VALUE)
                {
                    const char* typeStr = galliumArgTypeNamesTbl[cxuint(arg.type)];
                    const size_t len = ::strlen(typeStr);
                    ::memcpy(lineBuf+pos, typeStr, len);
                    pos += len;
                }
                else
                    pos += itocstrCStyle<cxuint>(cxuint(arg.type), lineBuf+pos, 16);
                
                lineBuf[pos++] = ',';
                lineBuf[pos++] = ' ';
                // arg size
                pos += itocstrCStyle<uint32_t>(arg.size, lineBuf+pos, 16);
                lineBuf[pos++] = ',';
                lineBuf[pos++] = ' ';
                // target size
                pos += itocstrCStyle<uint32_t>(arg.targetSize, lineBuf+pos, 16);
                lineBuf[pos++] = ',';
                lineBuf[pos++] = ' ';
                // target align
                pos += itocstrCStyle<uint32_t>(arg.targetAlign, lineBuf+pos, 16);
                // integer extension
                if (arg.signExtended)
                    ::memcpy(lineBuf+pos, ", sext, ", 8);
                else
                    ::memcpy(lineBuf+pos, ", zext, ", 8);
                pos += 8;
                // print argument semantic
                if (arg.semantic <= GalliumArgSemantic::MAX_VALUE)
                {
                    const char* typeStr = galliumArgSemTypeNamesTbl[cxuint(arg.semantic)];
                    const size_t len = ::strlen(typeStr);
                    ::memcpy(lineBuf+pos, typeStr, len);
                    pos += len;
                }
                else
                    pos += itocstrCStyle<cxuint>(cxuint(arg.semantic), lineBuf+pos, 16);
                lineBuf[pos++] = '\n';
                output.write(lineBuf, pos);
            }
            if (!doDumpConfig)
            {
                /// proginfo (if no config)
                const cxuint progInfoEntriesNum = galliumInput->isLLVM390 ? 5 : 3;
                output.write("    .proginfo\n", 14);
                for (cxuint k = 0; k < progInfoEntriesNum; k++)
                {
                    // print prog info entries: .entry address, value
                    const GalliumProgInfoEntry& piEntry = kinput.progInfo[k];
                    output.write("        .entry ", 15);
                    char buf[32];
                    size_t numSize = itocstrCStyle<uint32_t>(piEntry.address,
                                 buf, 32, 16, 8);
                    output.write(buf, numSize);
                    output.write(", ", 2);
                    numSize = itocstrCStyle<uint32_t>(piEntry.value, buf, 32, 16, 8);
                    output.write(buf, numSize);
                    output.write("\n", 1);
                }
            }
            else
            {
                dumpKernelConfig(output, maxSgprsNum, arch, kinput.progInfo,
                    galliumInput->isLLVM390);
                if (galliumInput->isAMDHSA)
                    // print AMD HSA config with special prefix for some HSA values
                    dumpAMDHSAConfig(output, maxSgprsNum, arch,
                        *reinterpret_cast<const ROCmKernelConfig*>(
                             galliumInput->code + kinput.offset), true);
            }
        }
        if (!galliumInput->isAMDHSA)
            isaDisassembler->addNamedLabel(kinput.offset, kinput.kernelName);
    }
    
    if (!galliumInput->scratchRelocs.empty())
    {
        // put scratch relocations and scratch symbol
        isaDisassembler->addRelSymbol(".scratchaddr");
        for (const GalliumScratchReloc& entry: galliumInput->scratchRelocs)
            isaDisassembler->addRelocation(entry.offset, entry.type, 0, 0);
    }
    
    if (doDumpCode && galliumInput->code != nullptr && galliumInput->codeSize != 0)
    {
        // print text
        if (!galliumInput->isAMDHSA)
        {
            // just disassembly code in simple way
            output.write(".text\n", 6);
            isaDisassembler->setInput(galliumInput->codeSize, galliumInput->code);
            isaDisassembler->beforeDisassemble();
            isaDisassembler->disassemble();
        }
        else
        {
            // LLVM 4.0 - AMDHSA code
            std::vector<ROCmDisasmRegionInput> regions(galliumInput->kernels.size());
            Array<size_t> sortedIndices(galliumInput->kernels.size());
            for (size_t i = 0; i < sortedIndices.size(); i++)
                sortedIndices[i] = i;
            // sort by offset (sortedRegions is indices of sorted regions)
            std::sort(sortedIndices.begin(), sortedIndices.end(), [&galliumInput]
                    (size_t a, size_t b)
                    {
                        return galliumInput->kernels[a].offset <
                                galliumInput->kernels[b].offset;
                    });
            
            // preparing ROCMDIsasm region inputs for dissasemblying in AMDHSA form
            for (size_t i = 0; i < galliumInput->kernels.size(); i++)
            {
                const GalliumDisasmKernelInput& kernel = galliumInput->kernels[i];
                ROCmDisasmRegionInput& region = regions[i];
                region.regionName = kernel.kernelName;
                region.offset = kernel.offset;
                region.type = ROCmRegionType::KERNEL;
            }
            
            // set correct region size using sorted indices by region offsets
            for (size_t i = 0; i < sortedIndices.size(); i++)
            {
                const size_t index = sortedIndices[i];
                const size_t end = (i+1 < galliumInput->kernels.size()) ?
                        galliumInput->kernels[sortedIndices[i+1]].offset :
                        galliumInput->codeSize;
                ROCmDisasmRegionInput& region = regions[index];
                region.size = end - galliumInput->kernels[index].offset;
                if (region.size < 256)
                    throw DisasmException("Gallium kernel region is too small");
            }
            
            disassembleAMDHSACode(output, regions, galliumInput->codeSize,
                            galliumInput->code, isaDisassembler, flags);
        }
    }
}
