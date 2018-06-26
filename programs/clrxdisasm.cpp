/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
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
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdasm/Disassembler.h>

using namespace CLRX;

static const CLIOption programOptions[] =
{
    { "metadata", 'm', CLIArgType::NONE, false, false, "dump object metadata", nullptr },
    { "data", 'd', CLIArgType::NONE, false, false, "dump global data", nullptr },
    { "calNotes", 'c', CLIArgType::NONE, false, false, "dump ATI CAL notes", nullptr },
    { "config", 'C', CLIArgType::NONE, false, false, "dump kernel configuration", nullptr },
    { "setup", 's', CLIArgType::NONE, false, false, "dump kernel setup", nullptr },
    { "HSAConfig", 'H', CLIArgType::NONE, false, false,
        "dump kernel HSA config (AMD OpenCL 2.0)", nullptr },
    { "HSALayout", 'L', CLIArgType::NONE, false, false,
        "dump in HSA layout form (AMD OpenCL 2.0)", nullptr },
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
    { "driverVersion", 't', CLIArgType::UINT, false, false,
        "set driver version (for AmdCL2)", "VERSION" },
    { "llvmVersion", 0, CLIArgType::UINT, false, false,
        "set LLVM version (for Gallium)", "VERSION" },
    { "buggyFPLit", 0, CLIArgType::NONE, false, false,
        "use old and buggy fplit rules", nullptr },
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
    
    Flags disasmFlags = DISASM_DUMPCODE|DISASM_CODEPOS;
    if (cli.hasShortOption('a'))
        disasmFlags = DISASM_ALL;
    else
        disasmFlags |= (cli.hasShortOption('m')?DISASM_METADATA:0) |
            (cli.hasShortOption('d')?DISASM_DUMPDATA:0) |
            (cli.hasShortOption('c')?DISASM_CALNOTES:0) |
            (cli.hasShortOption('s')?DISASM_SETUP:0) |
            (cli.hasShortOption('f')?DISASM_FLOATLITS:0) |
            (cli.hasShortOption('h')?DISASM_HEXCODE:0);
     disasmFlags |= (cli.hasShortOption('C')?DISASM_CONFIG:0) |
             (cli.hasLongOption("buggyFPLit")?DISASM_BUGGYFPLIT:0) |
             (cli.hasShortOption('H')?DISASM_HSACONFIG:0) |
             (cli.hasShortOption('L')?DISASM_HSALAYOUT:0);
    
    GPUDeviceType gpuDeviceType = GPUDeviceType::CAPE_VERDE;
    const bool fromRawCode = cli.hasShortOption('r');
    if (cli.hasShortOption('g'))
        gpuDeviceType = getGPUDeviceTypeFromName(cli.getShortOptArg<const char*>('g'));
    else if (cli.hasShortOption('A'))
        gpuDeviceType = getLowestGPUDeviceTypeFromArchitecture(
                    getGPUArchitectureFromName(cli.getShortOptArg<const char*>('A')));
    
    cxuint driverVersion = 0;
    if (cli.hasShortOption('t'))
        driverVersion = cli.getShortOptArg<cxuint>('t');
    cxuint llvmVersion = 0;
    if (cli.hasLongOption("llvmVersion"))
        llvmVersion = cli.getLongOptArg<cxuint>("llvmVersion");
    
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
                // standard flags for binary format creators,
                // needed by disassemblers to correctly getting all datas to dump
                Flags binFlags = AMDBIN_CREATE_KERNELINFO | AMDBIN_CREATE_KERNELINFOMAP |
                        AMDBIN_CREATE_INNERBINMAP | AMDBIN_CREATE_KERNELHEADERS |
                        AMDBIN_CREATE_KERNELHEADERMAP;
                // supply additional flags for CALNotes and info strings
                if ((disasmFlags & (DISASM_CALNOTES|DISASM_CONFIG)) != 0)
                    binFlags |= AMDBIN_INNER_CREATE_CALNOTES;
                if ((disasmFlags & (DISASM_METADATA|DISASM_CONFIG)) != 0)
                    binFlags |= AMDBIN_CREATE_INFOSTRINGS;
                
                if (isAmdBinary(binaryData.size(), binaryData.data()))
                {
                    // if amd binary
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
                    // extra (extra data) flags for OpenCL 2.0 disassembler
                    binFlags |= AMDCL2BIN_INNER_CREATE_KERNELDATA |
                                AMDCL2BIN_INNER_CREATE_KERNELDATAMAP |
                                AMDCL2BIN_INNER_CREATE_KERNELSTUBS;
                    base.reset(createAmdCL2BinaryFromCode(binaryData.size(),
                                           binaryData.data(), binFlags));
                    if (base->getType() == AmdMainType::GPU_CL2_BINARY)
                    {
                        AmdCL2MainGPUBinary32* amdGpuBin =
                                static_cast<AmdCL2MainGPUBinary32*>(base.get());
                        Disassembler disasm(*amdGpuBin, std::cout, disasmFlags,
                                            driverVersion);
                        disasm.disassemble();
                    }
                    else if (base->getType() == AmdMainType::GPU_CL2_64_BINARY)
                    {
                        AmdCL2MainGPUBinary64* amdGpuBin =
                                static_cast<AmdCL2MainGPUBinary64*>(base.get());
                        Disassembler disasm(*amdGpuBin, std::cout, disasmFlags,
                                            driverVersion);
                        disasm.disassemble();
                    }
                    else
                        throw Exception("This is not AMDGPU binary file!");
                }
                else if (isROCmBinary(binaryData.size(), binaryData.data()))
                {
                    // ROCm binary
                    ROCmBinary rocmBin(binaryData.size(), binaryData.data(), 0);
                    Disassembler disasm(rocmBin, std::cout, disasmFlags);
                    disasm.disassemble();
                }
                else
                {
                    // if gallium binary
                    GalliumBinary galliumBin(binaryData.size(),binaryData.data(), 0);
                    Disassembler disasm(gpuDeviceType, galliumBin, std::cout,
                            disasmFlags, llvmVersion);
                    disasm.disassemble();
                }
            }
            else
            {
                /* raw binaries */
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
