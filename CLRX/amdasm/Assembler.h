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
#include <unordered_map>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/utils/Utilities.h>

/// main namespace
namespace CLRX
{

class Assembler;
class Disassembler;

enum: cxuint
{
    ASM_WARNINGS = 1,   ///< enable all warnings for assembler
    ASM_WARN_SIGNED_OVERFLOW = 2,   ///< warn about signed overflow
    ASM_64BIT_MODE = 4, ///< assemble to 64-bit addressing mode
    ASM_ALL = 0xff  ///< all flags
};

enum: cxuint
{
    DISASM_DUMPCODE = 1,    ///< dump code
    DISASM_METADATA = 2,    ///< dump metadatas
    DISASM_DUMPDATA = 4,    ///< dump datas
    DISASM_CALNOTES = 8,    ///< dump ATI CAL notes
    DISASM_FLOATLITS = 16,  ///< print in comments float literals
    DISASM_HEXCODE = 32,    ///< print on left side hexadecimal code
    DISASM_ALL = 0xff       ///< all disassembler flags
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

/// main class for
class ISADisassembler
{
protected:
    Disassembler& disassembler;
    size_t inputSize;
    const cxbyte* input;
    size_t startPos;
    std::vector<size_t> labels;
    explicit ISADisassembler(Disassembler& disassembler);
public:
    virtual ~ISADisassembler();
    
    /// set input code
    void setInput(size_t inputSize, const cxbyte* input, size_t startPos = 0);
    /// makes some things before disassemblying
    virtual void beforeDisassemble() = 0;
    /// disassembles input code
    virtual void disassemble() = 0;
};

class GCNDisassembler: public ISADisassembler
{
private:
    bool instrOutOfCode;
public:
    GCNDisassembler(Disassembler& disassembler);
    ~GCNDisassembler();
    
    void beforeDisassemble();
    void disassemble();
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

/// assembler global metadata's
struct AsmGlobalMetadata
{
    std::string driverInfo; ///< driver info (for AMD Catalyst drivers)
    std::string compileOptions; ///< compile options which used by in clBuildProgram
};

/// holds ATI CAL note
struct AsmCALNote
{
    CALNoteHeader header;   ///< header
    cxbyte* data;   ///< raw CAL note data
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
    GPUDeviceType deviceType;
    ISAAssembler* isaAssembler;
    std::vector<std::string> includeDirs;
    AsmSymbolMap symbolMap;
    MacroMap macroMap;
    cxuint flags;
    
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

/// single kernel input for disassembler
/** all pointer members holds only pointers that should be freed by your routines.
 * No management of data */
struct AmdDisasmKernelInput
{
    std::string kernelName; ///< kernel name
    size_t metadataSize;    ///< metadata size
    const char* metadata;   ///< kernel's metadata
    size_t headerSize;  ///< kernel header size
    const cxbyte* header;   ///< kernel header size
    std::vector<AsmCALNote> calNotes;   /// ATI CAL notes
    size_t dataSize;    ///< data (from inner binary) size
    const cxbyte* data; ///< data from inner binary
    size_t codeSize;    ///< size of code of kernel
    const cxbyte* code; ///< code of kernel
};

/// whole disassembler input (for AMD Catalyst driver GPU binaries)
/** all pointer members holds only pointers that should be freed by your routines.
 * No management of data */
struct AmdDisasmInput
{
    GPUDeviceType deviceType;   ///< GPU device type
    bool is64BitMode;       ///< true if 64-bit mode of addressing
    AsmGlobalMetadata metadata; ///< global metadata (driver info & compile options)
    size_t globalDataSize;  ///< global (constants for kernels) data size
    const cxbyte* globalData;   ///< global (constants for kernels) data
    std::vector<AmdDisasmKernelInput> kernels;    ///< kernel inputs
    
    /// get disassembler input from raw binary data
    static AmdDisasmInput createFromRawBinary(GPUDeviceType deviceType,
                        size_t binarySize, const cxbyte* binaryData);
};

/// whole disassembler input (for Gallium driver GPU binaries)
struct GalliumDisasmInput
{
    GPUDeviceType deviceType;   ///< GPU device type
    size_t globalDataSize;  ///< global (constants for kernels) data size
    const cxbyte* globalData;   ///< global (constants for kernels) data
    std::vector<GalliumKernelInput> kernels;
};

/// disassembler class
class Disassembler
{
private:
    ISADisassembler* isaDisassembler;
    bool fromBinary;
    const AmdDisasmInput* input;
    std::ostream& output;
    cxuint flags;
public:
    /// constructor for 32-bit GPU binary
    /**
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const AmdMainGPUBinary32& binary, std::ostream& output,
                 cxuint flags = 0);
    /// constructor for 64-bit GPU binary
    /**
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const AmdMainGPUBinary64& binary, std::ostream& output,
                 cxuint flags = 0);
    /// constructor for disassembler input
    /**
     * \param disasmInput disassembler input object
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const AmdDisasmInput* disasmInput, std::ostream& output,
                 cxuint flags = 0);
    ~Disassembler();
    
    /// disassembles input
    void disassemble();
    
    /// get disassemblers flags
    cxuint getFlags() const
    { return flags; }
    /// get disassemblers flags
    void setFlags(cxuint flags)
    { this->flags = flags; }
    
    /// get disassembler input
    const AmdDisasmInput* getInput() const
    { return input; }
    
    /// get output stream
    const std::ostream& getOutput() const
    { return output; }
    /// get output stream
    std::ostream& getOutput()
    { return output; }
};

/// get GPU device type from name
extern GPUDeviceType getGPUDeviceTypeFromName(const std::string& name);

};

#endif
