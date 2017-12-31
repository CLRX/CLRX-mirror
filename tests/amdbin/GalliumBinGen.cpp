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
#include <iostream>
#include <sstream>
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/GalliumBinaries.h>

using namespace CLRX;

static const char* origBinaryFiles[5] =
{
    CLRX_SOURCE_DIR "/tests/amdbin/galliumbins/BlackScholes.0.reconf.orig",
    CLRX_SOURCE_DIR "/tests/amdbin/galliumbins/DCT.0.reconf.orig",
    CLRX_SOURCE_DIR "/tests/amdbin/galliumbins/MatrixMultiplication.0.reconf.orig",
    CLRX_SOURCE_DIR "/tests/amdbin/galliumbins/vectoradd-64bit.clo.reconf",
    CLRX_SOURCE_DIR "/tests/amdbin/galliumbins/vectoradd-llvm4.clrx.clo"
};

template<typename GalliumElfBinary>
static GalliumInput getGalliumInputBase(bool disassembly, const GalliumBinary* galliumBin,
        const GalliumElfBinary& elfBin)
{
    const auto& commentHdr = elfBin.getSectionHeader(".comment");
    uint16_t rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = elfBin.getSectionIndex(".rodata"); }
    catch(Exception& ex)
    { }
    GalliumInput input;
    input.is64BitElf = galliumBin->is64BitElfBinary();
    
    input.deviceType = GPUDeviceType::CAPE_VERDE;
    if (rodataIndex != SHN_UNDEF)
    {
        // add rodata (global data)
        const auto& rodataHdr = elfBin.getSectionHeader(rodataIndex);
        input.globalDataSize = ULEV(rodataHdr.sh_size);
        input.globalData = elfBin.getSectionContent(rodataIndex);
    }
    else
    {
        input.globalDataSize = 0;
        input.globalData = nullptr;
    }
    
    // global settings
    input.commentSize = ULEV(commentHdr.sh_size);
    input.comment = (const char*)elfBin.getSectionContent(".comment");
    input.isMesa170 = galliumBin->isMesa170();
    input.isLLVM390 = elfBin.isLLVM390();
    
    const cxuint progInfoEntriesNum = input.isLLVM390 ? 5 : 3;
    // kernel input
    const cxuint kernelsNum = galliumBin->getKernelsNum();
    for (cxuint i = 0; i < kernelsNum; i++)
    {
        cxuint ii = (((i&~3)+3) < kernelsNum) ? (i^3) : i;
        const GalliumKernel& kernel = galliumBin->getKernel(ii);
        const GalliumProgInfoEntry* progInfo = elfBin.getProgramInfo(ii);
        GalliumProgInfoEntry outProgInfo[5];
        // get program info entries
        for (cxuint k = 0; k < progInfoEntriesNum; k++)
        {
            outProgInfo[k].address = ULEV(progInfo[k].address);
            outProgInfo[k].value = ULEV(progInfo[k].value);
        }
        GalliumKernelInput kinput = { kernel.kernelName, {}, false, {}, kernel.offset,
            std::vector<GalliumArgInfo>(kernel.argInfos.begin(), kernel.argInfos.end()) };
        std::copy(outProgInfo, outProgInfo + progInfoEntriesNum, kinput.progInfo);
        input.kernels.push_back(kinput);
    }
    // set code data
    input.codeSize = ULEV(elfBin.getSectionHeader(".text").sh_size);
    input.code = elfBin.getSectionContent(".text");
    return input;
}

static GalliumInput getGalliumInput(bool disassembly, const GalliumBinary* galliumBin)
{
    if (!galliumBin->is64BitElfBinary())
        return getGalliumInputBase(disassembly, galliumBin, galliumBin->getElfBinary32());
    else // 64-bit elf binary
        return getGalliumInputBase(disassembly, galliumBin, galliumBin->getElfBinary64());
}

static void testOrigBinary(cxuint testCase, const char* origBinaryFilename)
{
    Array<cxbyte> inputData;
    std::unique_ptr<GalliumBinary> galliumBin;
    Array<cxbyte> output;
    std::string origBinFilenameStr(origBinaryFilename);
    filesystemPath(origBinFilenameStr); // convert to system path (native separators)
    
    // loading data gallium binary
    inputData = loadDataFromFile(origBinFilenameStr.c_str());
    galliumBin.reset(new GalliumBinary(inputData.size(), inputData.data(),
            GALLIUM_INNER_CREATE_SECTIONMAP |
            GALLIUM_INNER_CREATE_SYMBOLMAP | GALLIUM_INNER_CREATE_PROGINFOMAP));
    
    // get GalliumInput
    GalliumInput galliumInput = getGalliumInput(true, galliumBin.get());
    GalliumBinGenerator binGen(&galliumInput);
    binGen.generate(output);
    
    // compare (result) generated binary
    if (output.size() != inputData.size())
    {
        std::ostringstream oss;
            oss << "Failed for #" << testCase << " file=" << origBinaryFilename <<
                    ": expectedSize=" << inputData.size() <<
                    ", resultSize=" << output.size();
        throw Exception(oss.str());
    }
    for (size_t i = 0; i < inputData.size(); i++)
        if (output[i] != inputData[i])
        {
            std::ostringstream oss;
            oss << "Failed for #" << testCase << " file=" << origBinaryFilename <<
                    ": byte=" << i;
            throw Exception(oss.str());
        }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(origBinaryFiles)/sizeof(const char*); i++)
        try
        { testOrigBinary(i, origBinaryFiles[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
