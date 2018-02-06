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
#include <CLRX/amdbin/ROCmBinaries.h>

using namespace CLRX;

static const char* origBinaryFiles[3] =
{
    CLRX_SOURCE_DIR "/tests/amdbin/rocmbins/consttest1-kaveri.hsaco.regen",
    CLRX_SOURCE_DIR "/tests/amdbin/rocmbins/rijndael.hsaco.regen",
    CLRX_SOURCE_DIR "/tests/amdbin/rocmbins/vectoradd-rocm.clo.regen"
};

static ROCmInput genROCmInput(const ROCmBinary& binary)
{
    uint32_t archMajor = 0;
    ROCmInput rocmInput;
    rocmInput.eflags = ULEV(binary.getHeader().e_flags);
    // try to get comment
    try
    {
        const auto& commentHdr = binary.getSectionHeader(".comment");
        rocmInput.commentSize = ULEV(commentHdr.sh_size);
        rocmInput.comment = (const char*)binary.getSectionContent(".comment");
    }
    catch(...)
    {
        rocmInput.commentSize = 0;
        rocmInput.comment = nullptr;
    }
    rocmInput.archMinor = 0;
    rocmInput.archStepping = 0;
    rocmInput.target = binary.getTarget();
    rocmInput.metadataSize = binary.getMetadataSize();
    rocmInput.metadata = binary.getMetadata();
    rocmInput.newBinFormat = binary.isNewBinaryFormat();
    rocmInput.globalDataSize = binary.getGlobalDataSize();
    rocmInput.globalData = binary.getGlobalData();
    rocmInput.useMetadataInfo = false;
    
    {
        // load .note to determine architecture (major, minor and stepping)
        const cxbyte* noteContent = (const cxbyte*)binary.getNotes();
        if (noteContent==nullptr)
            throw Exception("Missing notes in inner binary!");
        size_t notesSize = binary.getNotesSize();
        // find note about AMDGPU
        for (size_t offset = 0; offset < notesSize; )
        {
            const Elf64_Nhdr* nhdr = (const Elf64_Nhdr*)(noteContent + offset);
            size_t namesz = ULEV(nhdr->n_namesz);
            size_t descsz = ULEV(nhdr->n_descsz);
            if (usumGt(offset, namesz+descsz, notesSize))
                throw Exception("Note offset+size out of range");
            if (ULEV(nhdr->n_type) == 0x3 && namesz==4 && descsz>=0x1a &&
                ::strcmp((const char*)noteContent+offset+sizeof(Elf64_Nhdr), "AMD")==0)
            {    // AMDGPU type
                const uint32_t* content = (const uint32_t*)
                        (noteContent+offset+sizeof(Elf64_Nhdr) + 4);
                archMajor = ULEV(content[1]);
                rocmInput.archMinor = ULEV(content[2]);
                rocmInput.archStepping = ULEV(content[3]);
            }
            size_t align = (((namesz+descsz)&3)!=0) ? 4-((namesz+descsz)&3) : 0;
            offset += sizeof(Elf64_Nhdr) + namesz + descsz + align;
        }
    }
    // determine device type
    rocmInput.deviceType = GPUDeviceType::CAPE_VERDE;
    if (archMajor==0)
        rocmInput.deviceType = GPUDeviceType::CAPE_VERDE;
    else if (archMajor==7)
        rocmInput.deviceType = GPUDeviceType::BONAIRE;
    else if (archMajor==8)
        rocmInput.deviceType = GPUDeviceType::ICELAND;
    else if (archMajor==9)
        rocmInput.deviceType = GPUDeviceType::GFX900;
    // set main code to input
    rocmInput.codeSize = binary.getCodeSize();
    rocmInput.code = binary.getCode();
    
    // get ROCm symbol
    const size_t codeOffset = binary.getCode()-binary.getBinaryCode();
    for (size_t i = 0; i < binary.getRegionsNum(); i++)
    {
        const ROCmRegion& region = binary.getRegion(i);
        rocmInput.symbols.push_back({region.regionName, region.offset-codeOffset,
                    region.size, region.type});
    }
    return rocmInput;
}

static void testOrigBinary(cxuint testCase, const char* origBinaryFilename)
{
    Array<cxbyte> inputData;
    Array<cxbyte> output;
    std::string origBinFilenameStr(origBinaryFilename);
    filesystemPath(origBinFilenameStr); // convert to system path (native separators)
    
    // load input binary
    inputData = loadDataFromFile(origBinFilenameStr.c_str());
    ROCmBinary rocmBin(inputData.size(), inputData.data(), 0);
    // generate input from binary
    ROCmInput rocmInput = genROCmInput(rocmBin);
    ROCmBinGenerator binGen(&rocmInput);
    binGen.generate(output);
    
    // compare generated output with input
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
