/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
/*! \file Assembler.h
 * \brief an assembler for Radeon GPU's
 */

#ifndef __CLRX_ASSEMBLER_H__
#define __CLRX_ASSEMBLER_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <istream>
#include <ostream>
#include <vector>
#include <utility>
#include <unordered_map>
#include <CLRX/AmdBinaries.h>
#include <CLRX/Utilities.h>

/// main namespace
namespace CLRX
{

class Assembler;
class Disassembler;

enum: cxuint
{
    ASM_WARNINGS = 1,
    ASM_WARN_SIGNED_OVERFLOW = 2,
    ASM_64BIT_MODE = 4
};

enum: cxuint
{
    DISASM_ADDRESS = 1,
    DISASM_HEXCODE = 2,
    DISASM_CALLPARAMS = 4,
    DISASM_ASMFORM = 8,
    DISASM_DISPLAYDATA = 16
};

enum class GPUDeviceType
{
    UNDEFINED = 0,
    CEDAR, ///< Radeon HD5400
    REDWOOD, ///< Radeon HD5500
    JUNIPER, ///< Radeon HD5700
    CYPRESS, ///< Radeon HD5800
    CAICOS, ///< Radeon HD6400
    TURKS, ///< Radeon HD6500
    BARTS, ///< Radeon HD6800
    CAYMAN, ///< Radeon HD6900
    WINTER_PARK, ///< Radeon HD6300D
    BEAVER_CREEK, ///< Radeon HD6500D
    CAPE_VERDE, ///< Radeon HD7700
    PITCAIRN, ///< Radeon HD7800
    TAHITI, ///< Radeon HD7900
    DEVASTATOR, ///< Radeon HD7x00D
    SCRAPPER, ///< Radeon HD7400G
    OLAND, ///< Radeon R7 250
    BONAIRE, ///< Radeon R7 260
    CURACAO, ///< Radeon R9 270
    HAWAII, ///< Radeon R9 290
    
    RADEON_HD5400 = CEDAR,
    RADEON_HD5500 = REDWOOD,
    RADEON_HD5700 = JUNIPER,
    RADEON_HD5800 = CYPRESS,
    RADEON_HD6400 = CAICOS,
    RADEON_HD6500 = TURKS,
    RADEON_HD6800 = BARTS,
    RADEON_HD6900 = CAYMAN,
    RADEON_HD6300D = WINTER_PARK,
    RADEON_HD6500D = BEAVER_CREEK,
    RADEON_HD7700 = CAPE_VERDE,
    RADEON_HD7800 = PITCAIRN,
    RADEON_HD7900 = TAHITI,
    RADEON_HD7X00D = DEVASTATOR,
    RADEON_HD7400G = SCRAPPER,
    RADEON_R7_250 = OLAND,
    RADEON_R7_260 = BONAIRE,
    RADEON_R9_270 = CURACAO,
    RADEON_R9_290 = HAWAII
};

class ISAAssembler
{
private:
    Assembler& assembler;
    
    size_t outputSize;
    char* output;
    
    explicit ISAAssembler(Assembler& assembler);
protected:
    void reallocateOutput(size_t newSize);
public:
    virtual ~ISAAssembler();
    
    virtual size_t getMaxOutputSize() const = 0;
    virtual size_t assemble(size_t lineNo, const char* line) = 0;
    virtual void finish() = 0;
    
    size_t getOutputSize() const
    { return outputSize; }
    
    const char* getOutput() const
    { return output; }
};

class R800Assembler: public ISAAssembler
{
public:
    explicit R800Assembler(Assembler& assembler);
    ~R800Assembler();
    
    size_t getMaxOutputSize() const;
    size_t assemble(size_t lineNo, const char* line);
    void finish();
};

class R1000Assembler: public ISAAssembler
{
public:
    explicit R1000Assembler(Assembler& assembler);
    ~R1000Assembler();
    
    size_t getMaxOutputSize() const;
    size_t assemble(size_t lineNo, const char* line);
    void finish();
};

class ISADisassembler
{
private:
    Disassembler& disassembler;
    
    size_t inputSize;
    const cxbyte* input;
    
    ISADisassembler(Disassembler& disassembler);
public:
    virtual ~ISADisassembler();
    
    virtual size_t getMaxLineSize() const = 0;
    virtual size_t disassemble(char* line) = 0;
};

class R800Disassembler: public ISADisassembler
{
public:
    R800Disassembler(Disassembler& disassembler);
    ~R800Disassembler();
    
    size_t getMaxLineSize() const;
    size_t disassemble(char* line);
};

class R1000Disassembler: public ISADisassembler
{
public:
    R1000Disassembler(Disassembler& disassembler);
    ~R1000Disassembler();
    
    size_t getMaxLineSize() const;
    size_t disassemble(char* line);
};

class AsmExpression;

struct AsmSymbol
{
    cxuint sectionId;
    bool isDefined;
    uint64_t value;
    AsmExpression* resolvingExpression;
};

typedef std::unordered_map<std::string, uint64_t> AsmSymbolMap;

struct AsmGlobalMetadata
{
    std::string driverVersion;
    std::string compileOptions;
};

struct AsmCALNote
{
    CALNoteHeader header;
    cxbyte* data;
};

struct AsmKernelMetadata
{
    std::string metadata;
    cxbyte header[32];
    std::vector<AsmCALNote> calNotes;
};

struct AsmKernel
{
    AsmKernelMetadata metadata;
    std::vector<cxbyte> execData;
    std::vector<cxbyte> code;
};

class Assembler
{
public:
    typedef std::unordered_map<std::string, std::string> DefSymMap;
    typedef std::unordered_map<std::string, std::string> MacroMap;
    typedef std::unordered_map<std::string, AsmKernel> KernelMap;
private:
    cxuint flags;
    GPUDeviceType deviceType;
    ISAAssembler* isaAssembler;
    std::vector<std::string> includeDirs;
    AsmSymbolMap symbolMap;
    MacroMap macroMap;
    
    AsmGlobalMetadata globalMetadata;
    std::vector<cxbyte> globalData; // 
    KernelMap kernelMap;
public:
    explicit Assembler(std::istream& input, cxuint flags);
    ~Assembler();
    
    void assemble();
    
    GPUDeviceType getDeviceType() const
    { return deviceType; }
    
    void setDeviceType(const GPUDeviceType deviceType)
    { this->deviceType = deviceType; }
    
    cxuint getFlags() const
    { return flags; }
    void setFlags(cxuint flags)
    { this->flags = flags; }
    
    const std::vector<std::string>& getIncludeDirs() const
    { return includeDirs; }
    
    const AsmSymbolMap& getSymbolMap() const
    { return symbolMap; }
    
    void setInitialDefSyms(const DefSymMap& defsyms);
    
    void addInitialDefSym(const std::string& symName, const std::string& symExpr);
    
    uint64_t parseExpression(size_t stringSize, const char* string) const;
    
    const AsmGlobalMetadata& getGlobalMetadata() const
    { return globalMetadata; }
    const cxbyte* getGlobalData() const
    { return globalData.data(); }
    const AsmKernel& getKernel(const std::string& kernelName) const;
    
    const KernelMap& getKernelMap() const
    { return kernelMap; }
};

struct DisasmKernelInput
{
    size_t metadataSize;
    char* metadata;
    cxbyte* header;
    std::vector<AsmCALNote> calNotes;
    size_t execDataSize;
    cxbyte* execData;
    size_t codeSize;
    cxbyte* code;
};

struct DisasmInput
{
    bool is64BitMode;
    GPUDeviceType deviceType;
    AsmGlobalMetadata metadata;
    cxbyte* globalData;
    std::vector<DisasmKernelInput> kernelInputs;
};

class Disassembler
{
private:
    ISADisassembler* isaDisassembler;
    const DisasmInput& input;
    std::ostream& output;
    cxuint flags;
public:
    Disassembler(const AmdMainGPUBinary32& binary, std::ostream& os, cxuint flags);
    Disassembler(const AmdMainGPUBinary64& binary, std::ostream& os, cxuint flags);
    
    Disassembler(const DisasmInput& disasmInput, std::ostream& os, cxuint flags);
    ~Disassembler();
    
    void disassemble();
    
    cxuint getFlags() const
    { return flags; }
    void setFlags(cxuint flags)
    { this->flags = flags; }
    
    const DisasmInput& getInput() const
    { return input; }
    
    const std::ostream& getOutput() const
    { return output; }
    std::ostream& getOutput()
    { return output; }
};

};

#endif
