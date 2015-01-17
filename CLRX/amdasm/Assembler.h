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
#include <utility>
#include <unordered_map>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/utils/Utilities.h>

/// main namespace
namespace CLRX
{

class Assembler;

enum: cxuint
{
    ASM_WARNINGS = 1,   ///< enable all warnings for assembler
    ASM_WARN_SIGNED_OVERFLOW = 2,   ///< warn about signed overflow
    ASM_64BIT_MODE = 4, ///< assemble to 64-bit addressing mode
    ASM_ALL = 0xff  ///< all flags
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

struct AsmKernelMetadata
{
    std::string metadata;
    cxbyte header[32];
    std::vector<CALNoteInput> calNotes;
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

};

#endif
