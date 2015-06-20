/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
#include <memory>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/CLIParser.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdasm/Disassembler.h>

using namespace CLRX;

enum {
    PROGOPT_METADATA = 0,
    PROGOPT_DATA,
    PROGOPT_CALNOTES,
    PROGOPT_FLOATS,
    PROGOPT_HEXCODE,
    PROGOPT_ALL,
    PROGOPT_RAWCODE,
    PROGOPT_GPUTYPE
};

static const CLIOption programOptions[] =
{
    { "metadata", 'm', CLIArgType::NONE, false, false, "dump object metadata", nullptr },
    { "data", 'd', CLIArgType::NONE, false, false, "dump global data", nullptr },
    { "calNotes", 'c', CLIArgType::NONE, false, false, "dump ATI CAL notes", nullptr },
    { "floats", 'f', CLIArgType::NONE, false, false, "display float literals", nullptr },
    { "hexcode", 'h', CLIArgType::NONE, false, false,
        "display hexadecimal instr. codes", nullptr },
    { "all", 'a', CLIArgType::NONE, false, false,
        "dump all (including hexcode and float literals)", nullptr },
    { "raw", 'r', CLIArgType::NONE, false, false, "treat input as raw GCN code", nullptr },
    { "gpuType", 'g', CLIArgType::TRIMMED_STRING, false, false,
        "set GPU type for raw binaries", nullptr },
    CLRX_CLI_AUTOHELP
    { nullptr, 0 }
};

int main(int argc, const char** argv)
try
{
    CLIParser cli("clrxdisasm", programOptions, argc, argv);
    cli.parse();
    if (cli.handleHelpOrUsage())
        return 0;
    
    if (cli.getArgsNum() == 0)
    {
        std::cerr << "No input files." << std::endl;
        return 1;
    }
    
    cxuint disasmFlags = DISASM_DUMPCODE;
    if (cli.hasOption(PROGOPT_ALL))
        disasmFlags = DISASM_ALL;
    else
        disasmFlags |= (cli.hasOption(PROGOPT_METADATA)?DISASM_METADATA:0) |
            (cli.hasOption(PROGOPT_DATA)?DISASM_DUMPDATA:0) |
            (cli.hasOption(PROGOPT_CALNOTES)?DISASM_CALNOTES:0) |
            (cli.hasOption(PROGOPT_FLOATS)?DISASM_FLOATLITS:0) |
            (cli.hasOption(PROGOPT_HEXCODE)?DISASM_HEXCODE:0);
    
    GPUDeviceType gpuDeviceType = GPUDeviceType::CAPE_VERDE;
    const bool fromRawCode = cli.hasOption(PROGOPT_RAWCODE);
    if (cli.hasOption(PROGOPT_GPUTYPE))
        gpuDeviceType = getGPUDeviceTypeFromName(
                cli.getOptArg<const char*>(PROGOPT_GPUTYPE));
    
    int ret = 0;
    for (const char* const* args = cli.getArgs();*args != nullptr; args++)
    {
        std::cout << "/* Disassembling '" << *args << "\' */" << std::endl;
        Array<cxbyte> binaryData;
        std::unique_ptr<AmdMainBinaryBase> base = nullptr;
        std::unique_ptr<GalliumBinary> galliumBin = nullptr;
        try
        {
            binaryData = loadDataFromFile(*args);
            
            if (!fromRawCode)
            {
                cxuint binFlags = AMDBIN_CREATE_KERNELINFO | AMDBIN_CREATE_KERNELINFOMAP |
                        AMDBIN_CREATE_INNERBINMAP | AMDBIN_CREATE_KERNELHEADERS |
                        AMDBIN_CREATE_KERNELHEADERMAP;
                if ((disasmFlags & DISASM_CALNOTES) != 0)
                    binFlags |= AMDBIN_INNER_CREATE_CALNOTES;
                if ((disasmFlags & DISASM_METADATA) != 0)
                    binFlags |= AMDBIN_CREATE_INFOSTRINGS;
                
                try
                { base.reset(createAmdBinaryFromCode(binaryData.size(),
                            binaryData.data(), binFlags)); }
                catch(const Exception& ex)
                {   // check whether is Gallium
                    galliumBin.reset(new GalliumBinary(binaryData.size(),
                           binaryData.data(), 0));
                }
                if (galliumBin != nullptr)
                {   // if Gallium
                    Disassembler disasm(gpuDeviceType, *galliumBin, std::cout, disasmFlags);
                    disasm.disassemble();
                }
                else if (base->getType() == AmdMainType::GPU_BINARY)
                {
                    AmdMainGPUBinary32* amdGpuBin =
                            static_cast<AmdMainGPUBinary32*>(base.get());
                    Disassembler disasm(*amdGpuBin, std::cout, disasmFlags);
                    disasm.disassemble();
                }
                else if (base->getType() == AmdMainType::GPU_64_BINARY)
                {
                    AmdMainGPUBinary64* amdGpuBin =
                            static_cast<AmdMainGPUBinary64*>(base.get());
                    Disassembler disasm(*amdGpuBin, std::cout, disasmFlags);
                    disasm.disassemble();
                }
                else
                    throw Exception("This is not AMDGPU binary file!");
            }
            else
            {   /* raw binaries */
                Disassembler disasm(gpuDeviceType, binaryData.size(), binaryData.data(),
                        std::cout, disasmFlags);
                disasm.disassemble();
            }
        }
        catch(const std::exception& ex)
        {
            ret = 1;
            std::cout << "/* ERROR for '" << *args << "\' */" << std::endl;
            std::cerr << "Error during disassemblying '" << *args << "': " <<
                    ex.what() << std::endl;
        }
    }
    
    return ret;
}
catch(const Exception& ex)
{
    std::cerr << ex.what() << std::endl;
    return 1;
}
catch(const std::bad_alloc& ex)
{
    std::cerr << "Out of memory" << std::endl;
    return 1;
}
catch(const std::exception& ex)
{
    std::cerr << "System exception: " << ex.what() << std::endl;
    return 1;
}
catch(...)
{
    std::cerr << "Unknown exception" << std::endl;
    return 1;
}
