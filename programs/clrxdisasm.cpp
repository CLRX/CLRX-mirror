/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2016 Mateusz Szpakowski
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
    PROGOPT_CONFIG,
    PROGOPT_SETUP,
    PROGOPT_FLOATS,
    PROGOPT_HEXCODE,
    PROGOPT_ALL,
    PROGOPT_RAWCODE,
    PROGOPT_GPUTYPE,
    PROGOPT_ARCH
};

static const CLIOption programOptions[] =
{
    { "metadata", 'm', CLIArgType::NONE, false, false, "dump object metadata", nullptr },
    { "data", 'd', CLIArgType::NONE, false, false, "dump global data", nullptr },
    { "calNotes", 'c', CLIArgType::NONE, false, false, "dump ATI CAL notes", nullptr },
    { "config", 'C', CLIArgType::NONE, false, false, "dump kernel configuration", nullptr },
    { "setup", 's', CLIArgType::NONE, false, false, "dump kernel setup", nullptr },
    { "floats", 'f', CLIArgType::NONE, false, false, "display float literals", nullptr },
    { "hexcode", 'h', CLIArgType::NONE, false, false,
        "display hexadecimal instr. codes", nullptr },
    { "all", 'a', CLIArgType::NONE, false, false,
        "dump all (including hexcode and float literals)", nullptr },
    { "raw", 'r', CLIArgType::NONE, false, false, "treat input as raw GCN code", nullptr },
    { "gpuType", 'g', CLIArgType::TRIMMED_STRING, false, false,
        "set GPU type for Gallium/raw binaries", "DEVICE" },
    { "arch", 'A', CLIArgType::TRIMMED_STRING, false, false,
        "set GPU architecture for Gallium/raw binaries", "ARCH" },
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
    
    Flags disasmFlags = DISASM_DUMPCODE;
    if (cli.hasOption(PROGOPT_ALL))
        disasmFlags = DISASM_ALL;
    else
        disasmFlags |= (cli.hasOption(PROGOPT_METADATA)?DISASM_METADATA:0) |
            (cli.hasOption(PROGOPT_DATA)?DISASM_DUMPDATA:0) |
            (cli.hasOption(PROGOPT_CALNOTES)?DISASM_CALNOTES:0) |
            (cli.hasOption(PROGOPT_SETUP)?DISASM_SETUP:0) |
            (cli.hasOption(PROGOPT_FLOATS)?DISASM_FLOATLITS:0) |
            (cli.hasOption(PROGOPT_HEXCODE)?DISASM_HEXCODE:0) |
            (cli.hasOption(PROGOPT_CONFIG)?DISASM_CONFIG:0);
    
    GPUDeviceType gpuDeviceType = GPUDeviceType::CAPE_VERDE;
    const bool fromRawCode = cli.hasOption(PROGOPT_RAWCODE);
    if (cli.hasOption(PROGOPT_GPUTYPE))
        gpuDeviceType = getGPUDeviceTypeFromName(
                cli.getOptArg<const char*>(PROGOPT_GPUTYPE));
    else if (cli.hasOption(PROGOPT_ARCH))
        gpuDeviceType = getLowestGPUDeviceTypeFromArchitecture(
                    getGPUArchitectureFromName(
                            cli.getOptArg<const char*>(PROGOPT_ARCH)));
    
    int ret = 0;
    for (const char* const* args = cli.getArgs();*args != nullptr; args++)
    {
        std::cout << "/* Disassembling '" << *args << "\' */" << std::endl;
        Array<cxbyte> binaryData;
        std::unique_ptr<AmdMainBinaryBase> base = nullptr;
        try
        {
            binaryData = loadDataFromFile(*args);
            
            if (!fromRawCode)
            {
                Flags binFlags = AMDBIN_CREATE_KERNELINFO | AMDBIN_CREATE_KERNELINFOMAP |
                        AMDBIN_CREATE_INNERBINMAP | AMDBIN_CREATE_KERNELHEADERS |
                        AMDBIN_CREATE_KERNELHEADERMAP;
                if ((disasmFlags & (DISASM_CALNOTES|DISASM_CONFIG)) != 0)
                    binFlags |= AMDBIN_INNER_CREATE_CALNOTES;
                if ((disasmFlags & (DISASM_METADATA|DISASM_CONFIG)) != 0)
                    binFlags |= AMDBIN_CREATE_INFOSTRINGS;
                
                if (isAmdBinary(binaryData.size(), binaryData.data()))
                {   // if amd binary
                    base.reset(createAmdBinaryFromCode(binaryData.size(),
                            binaryData.data(), binFlags));
                    if (base->getType() == AmdMainType::GPU_BINARY)
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
                else if (isAmdCL2Binary(binaryData.size(), binaryData.data()))
                {   // AMD OpenCL 2.0 binary
                    binFlags |= AMDBIN_INNER_CREATE_KERNELDATA |
                                AMDBIN_INNER_CREATE_KERNELDATAMAP |
                                AMDBIN_INNER_CREATE_KERNELSTUBS;
                    AmdCL2MainGPUBinary amdBin(binaryData.size(),
                                       binaryData.data(), binFlags);
                    Disassembler disasm(amdBin, std::cout, disasmFlags);
                    disasm.disassemble();
                }
                else // if gallium binary
                {
                    GalliumBinary galliumBin(binaryData.size(),binaryData.data(), 0);
                    Disassembler disasm(gpuDeviceType, galliumBin, std::cout, disasmFlags);
                    disasm.disassemble();
                }
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
