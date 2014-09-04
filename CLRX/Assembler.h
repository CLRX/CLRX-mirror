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
    DISASM_DUMPCODE = 1,
    DISASM_METADATA = 2,
    DISASM_DUMPDATA = 4,
    DISASM_CALNOTES = 8,
    DISASM_ALL = 0xff 
};

enum class GPUDeviceType
{
    UNDEFINED = 0,
    CAPE_VERDE, ///< Radeon HD7700
    PITCAIRN, ///< Radeon HD7800
    TAHITI, ///< Radeon HD7900
    OLAND, ///< Radeon R7 250
    BONAIRE, ///< Radeon R7 260
    SPECTRE, ///< Kaveri
    SPOOKY, ///< Kaveri
    KALINDI, ///< ???
    HAINAN, ///< ???? 
    HAWAII, ///< Radeon R9 290
    GPUDEVICE_MAX = HAWAII,
    
    RADEON_HD7700 = CAPE_VERDE,
    RADEON_HD7800 = PITCAIRN,
    RADEON_HD7900 = TAHITI,
    RADEON_R7_250 = OLAND,
    RADEON_R7_260 = BONAIRE,
    RADEON_R9_290 = HAWAII
};

struct ISAReservedRegister
{
    const char* name;
    char destPrefix;
    bool lastIndices;
    cxuint destIndex;
    cxuint destSize;
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
    
    virtual const char* getRegisterPrefixes() const = 0;
    virtual const ISAReservedRegister* getReservedRegisters() const = 0;
    virtual cxuint getRegistersNum(char prefix) const = 0;
    
    virtual void setRegisterIndices(const cxuint* registersNum,
                    const cxuint* registerIndices) = 0;
    
    size_t getOutputSize() const
    { return outputSize; }
    
    const char* getOutput() const
    { return output; }
};

class GCNAssembler: public ISAAssembler
{
public:
    explicit GCNAssembler(Assembler& assembler);
    ~GCNAssembler();
    
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
    std::vector<size_t> labels;
    bool finished;
protected:
    explicit ISADisassembler(Disassembler& disassembler);
public:
    virtual ~ISADisassembler();
    
    void setInput(size_t inputSize, const cxbyte* input);
    // before disassemble - first phase before generating output.
    // collects labels and other symbols 
    virtual void beforeDisassemble() = 0;
    virtual size_t disassemble(size_t maxSize, char* buffer) = 0;
    
    bool isFinished() const
    { return finished; }
};

class GCNDisassembler: public ISADisassembler
{
public:
    GCNDisassembler(Disassembler& disassembler);
    ~GCNDisassembler();
    
    void setInput(size_t inputSize, const cxbyte* input);
    void beforeDisassemble();
    size_t disassemble(size_t maxSize, char* buffer);
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
    std::string driverInfo;
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
    std::string kernelName;
    size_t metadataSize;
    const char* metadata;
    size_t headerSize;
    const cxbyte* header;
    std::vector<AsmCALNote> calNotes;
    size_t dataSize;
    const cxbyte* data;
    size_t codeSize;
    const cxbyte* code;
};

struct DisasmInput
{
    GPUDeviceType deviceType;
    bool is64BitMode;
    AsmGlobalMetadata metadata;
    size_t globalDataSize;
    const cxbyte* globalData;
    std::vector<DisasmKernelInput> kernelInputs;
};

class Disassembler
{
private:
    ISADisassembler* isaDisassembler;
    bool fromBinary;
    const DisasmInput* input;
    std::ostream& output;
    cxuint flags;
public:
    Disassembler(const AmdMainGPUBinary32& binary, std::ostream& output,
                 cxuint flags = 0);
    Disassembler(const AmdMainGPUBinary64& binary, std::ostream& output,
                 cxuint flags = 0);
    Disassembler(const DisasmInput* disasmInput, std::ostream& output,
                 cxuint flags = 0);
    ~Disassembler();
    
    void disassemble();
    
    cxuint getFlags() const
    { return flags; }
    void setFlags(cxuint flags)
    { this->flags = flags; }
    
    const DisasmInput* getInput() const
    { return input; }
    
    const std::ostream& getOutput() const
    { return output; }
    std::ostream& getOutput()
    { return output; }
};

};

#endif
