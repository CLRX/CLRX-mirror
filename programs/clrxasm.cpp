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
#include <fstream>
#include <cstring>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/CLIParser.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdasm/Assembler.h>

using namespace CLRX;

static const CLIOption programOptions[] =
{
    { "defsym", 'D', CLIArgType::STRING_ARRAY, false, true, "define symbol",
        "SYMBOL[=VALUE]" },
    { "includePath", 'I', CLIArgType::STRING_ARRAY, false, true,
        "add include directory path", "PATH" },
    { "output", 'o', CLIArgType::STRING, false, false, "set output file", "FILENAME" },
    { "binaryFormat", 'b', CLIArgType::TRIMMED_STRING, false, false,
        "set output binary format", "BINFORMAT" },
    { "64bit", '6', CLIArgType::NONE, false, false,
        "generate 64-bit code (for AmdCatalyst)", nullptr },
    { "gpuType", 'g', CLIArgType::TRIMMED_STRING, false, false,
        "set GPU type for Gallium/raw binaries", "DEVICE" },
    { "arch", 'A', CLIArgType::TRIMMED_STRING, false, false,
        "set GPU architecture for Gallium/raw binaries", "ARCH" },
    { "driverVersion", 't', CLIArgType::UINT, false, false,
        "set driver version (for Amd/GalliumCompute)", "VERSION" },
    { "llvmVersion", 0, CLIArgType::UINT, false, false,
        "set LLVM version (for GalliumCompute)", "VERSION" },
    { "newROCmBinFormat", 0, CLIArgType::NONE, false, false,
        "enable new ROCm binary format", nullptr },
    { "forceAddSymbols", 'S', CLIArgType::NONE, false, false,
        "force add symbols to binaries", nullptr },
    { "alternate", 'a', CLIArgType::NONE, false, false,
        "enable alternate macro mode", nullptr }, 
    { "buggyFPLit", 0, CLIArgType::NONE, false, false,
        "use old and buggy fplit rules", nullptr },
    { "oldModParam", 0, CLIArgType::NONE, false, false,
        "use old modifier parametrization", nullptr },
    { "noMacroCase", 'm', CLIArgType::NONE, false, false,
        "do not ignore letter's case in macro names", nullptr },
    { "policy", 0, CLIArgType::UINT, false, false,
        "set policy version", "VERSION" },
    { "noWarnings", 'w', CLIArgType::NONE, false, false, "disable warnings", nullptr },
    CLRX_CLI_AUTOHELP
    { nullptr, 0 }
};

// verify whether symbol name is correct
static bool verifySymbolName(const CString& symbolName)
{
    if (symbolName.empty())
        return false;
    auto c = symbolName.begin();
    if (isAlpha(*c) || *c=='.' || *c=='_' || *c=='$')
        while (isAlnum(*c) || *c=='.' || *c=='_' || *c=='$') c++;
    return *c==0;
}

int main(int argc, const char** argv)
try
{
    CLIParser cli("clrxasm", programOptions, argc, argv);
    cli.parse();
    if (cli.handleHelpOrUsage())
        return 0;
    
    int ret = 0;
    bool is64Bit = false;
    BinaryFormat binFormat = BinaryFormat::AMD;
    GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE;
    uint32_t driverVersion = 0;
    uint32_t llvmVersion = 0;
    Flags flags = 0;
    bool newROCmBinFormat = false;
    bool havePolicy = false;
    cxuint policyVersion = 0;
    if (cli.hasShortOption('b'))
    {
        const char* binFmtName = cli.getShortOptArg<const char*>('b');
        // choosing binary format from name
        if (::strcasecmp(binFmtName, "raw")==0 || ::strcasecmp(binFmtName, "rawcode")==0)
            binFormat = BinaryFormat::RAWCODE;
        else if (::strcasecmp(binFmtName, "gallium")==0)
            binFormat = BinaryFormat::GALLIUM;
        else if (::strcasecmp(binFmtName, "amdcl2")==0)
            binFormat = BinaryFormat::AMDCL2;
        else if (::strcasecmp(binFmtName, "rocm")==0)
            binFormat = BinaryFormat::ROCM;
        else if (::strcasecmp(binFmtName, "amd")!=0 &&
                 ::strcasecmp(binFmtName, "catalyst")!=0)
            throw Exception("Unknown binary format");
    }
    if (cli.hasShortOption('6'))
        is64Bit = true;
    if (cli.hasShortOption('g'))
        deviceType = getGPUDeviceTypeFromName(cli.getShortOptArg<const char*>('g'));
    else if (cli.hasShortOption('A'))
        // in this case, we choose lowest GPU device for choosen GPU architecture
        deviceType = getLowestGPUDeviceTypeFromArchitecture(getGPUArchitectureFromName(
                    cli.getShortOptArg<const char*>('A')));
    if (cli.hasShortOption('t'))
        driverVersion = cli.getShortOptArg<cxuint>('t');
    if (cli.hasLongOption("llvmVersion"))
        llvmVersion = cli.getLongOptArg<cxuint>("llvmVersion");
    if (cli.hasShortOption('S'))
        flags |= ASM_FORCE_ADD_SYMBOLS;
    if (!cli.hasShortOption('w'))
        flags |= ASM_WARNINGS;
    if (cli.hasShortOption('a'))
        flags |= ASM_ALTMACRO;
    if (cli.hasLongOption("buggyFPLit"))
        flags |= ASM_BUGGYFPLIT;
    if (cli.hasShortOption('m'))
        flags |= ASM_MACRONOCASE;
    if (cli.hasLongOption("oldModParam"))
        flags |= ASM_OLDMODPARAM;
    if (cli.hasLongOption("newROCmBinFormat"))
        newROCmBinFormat = true;
    if (cli.hasLongOption("policy"))
    {
        policyVersion = cli.getLongOptArg<cxuint>("policy");
        havePolicy = true;
    }
    
    cxuint argsNum = cli.getArgsNum();
    Array<CString> filenames(argsNum);
    for (cxuint i = 0; i < argsNum; i++)
        filenames[i] = cli.getArgs()[i];
    
    std::unique_ptr<Assembler> assembler;
    if (!filenames.empty())
        assembler.reset(new Assembler(filenames, flags, binFormat, deviceType));
    else // if from stdin
        assembler.reset(new Assembler(nullptr, std::cin, flags, binFormat, deviceType));
    assembler->set64Bit(is64Bit);
    assembler->setDriverVersion(driverVersion);
    assembler->setLLVMVersion(llvmVersion);
    assembler->setNewROCmBinFormat(newROCmBinFormat);
    if (havePolicy)
        assembler->setPolicyVersion(policyVersion);
    
    size_t defSymsNum = 0;
    const char* const* defSyms = nullptr;
    size_t includePathsNum = 0;
    const char* const* includePaths = nullptr;
    if (cli.hasShortOption('D'))
        defSyms = cli.getShortOptArgArray<const char*>('D', defSymsNum);
    if (cli.hasShortOption('I'))
        includePaths = cli.getShortOptArgArray<const char*>('I', includePathsNum);
    
    for (size_t i = 0; i < includePathsNum; i++)
        assembler->addIncludeDir(includePaths[i]);
    for (size_t i = 0; i < defSymsNum; i++)
    {
        const char* eqPlace = ::strchr(defSyms[i], '=');
        CString symName;
        uint64_t value = 0;
        if (eqPlace!=nullptr)
        {
            // defsym with value
            const char* outEnd;
            bool parsed = true;
            symName.assign(defSyms[i], eqPlace);
            eqPlace++;
            while (isSpace(*eqPlace)) eqPlace++;
            try
            { value = cstrtovCStyle<uint64_t>(eqPlace, nullptr, outEnd); }
            catch(const ParseException& ex)
            {
                std::cerr << "For symbol '" << symName << "': " << ex.what() << std::endl;
                ret = 1;
                parsed = false;
            }
            // if correctly parsed value
            if (parsed)
            {
                // check whether no garbages after value
                while (isSpace(*outEnd)) outEnd++;
                if (*outEnd!=0)
                {
                    std::cerr << "Garbages at symbol '" << symName <<
                                    "' value" << std::endl;
                    ret = 1;
                }
            }
        }
        else
            symName = defSyms[i];
        if (verifySymbolName(symName))
            assembler->addInitialDefSym(symName, value);
        else
        {
            std::cerr << "Invalid symbol name '" << symName << "'" << std::endl;
            ret = 1;
        }
    }
    // exit if errors occurred
    if (ret!=0)
        return ret;
    /// run assembling
    if (!assembler->assemble())
        return 1;
    /// write output to file
    const char* outputName = "a.out";
    if (cli.hasShortOption('o'))
        outputName = cli.getShortOptArg<const char*>('o');
    assembler->writeBinary(outputName);
    return 0;
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
