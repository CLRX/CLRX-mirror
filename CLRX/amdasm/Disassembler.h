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
/*! \file Disassembler.h
 * \brief an disassembler for Radeon GPU's
 */

#ifndef __CLRX_DISASSEMBLER_H__
#define __CLRX_DISASSEMBLER_H__

#include <CLRX/Config.h>
#include <string>
#include <istream>
#include <ostream>
#include <vector>
#include <utility>
#include <memory>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/amdasm/Commons.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/InputOutput.h>

/// main namespace
namespace CLRX
{

/// Disassembler exception class
class DisasmException: public Exception
{
public:
    /// empty constructor
    DisasmException() = default;
    /// constructor with messasge
    explicit DisasmException(const std::string& message);
    /// destructor
    virtual ~DisasmException() noexcept = default;
};

class Disassembler;

enum: Flags
{
    DISASM_DUMPCODE = 1,    ///< dump code
    DISASM_METADATA = 2,    ///< dump metadatas
    DISASM_DUMPDATA = 4,    ///< dump datas
    DISASM_CALNOTES = 8,    ///< dump ATI CAL notes
    DISASM_FLOATLITS = 16,  ///< print in comments float literals
    DISASM_HEXCODE = 32,    ///< print on left side hexadecimal code
    DISASM_SETUP = 64,
    DISASM_CONFIG = 128,    ///< print kernel configuration instead raw data
    DISASM_BUGGYFPLIT = 0x100,
    DISASM_CODEPOS = 0x200,   ///< print code position
    DISASM_HSACONFIG = 0x400,  ///< print HSA configuration
    DISASM_HSALAYOUT = 0x800,  ///< print in HSA layout (like Gallium or ROCm)
    
    ///< all disassembler flags (without config)
    DISASM_ALL = FLAGS_ALL&(~(DISASM_CONFIG|DISASM_BUGGYFPLIT|
                    DISASM_HSACONFIG|DISASM_HSALAYOUT))
};

struct GCNDisasmUtils;

/// main class for
class ISADisassembler: public NonCopyableAndNonMovable
{
private:
    friend struct GCNDisasmUtils; // INTERNAL LOGIC
public:
    typedef std::vector<size_t>::const_iterator LabelIter;  ///< label iterator
    
    /// named label iterator
    typedef std::vector<std::pair<size_t, CString> >::const_iterator NamedLabelIter;
protected:
    /// internal relocation structure
    struct Relocation
    {
        size_t symbol;   ///< symbol index
        RelocType type; ///< relocation type
        int64_t addend; ///< relocation addend
    };
    
    /// relocation iterator
    typedef std::vector<std::pair<size_t, Relocation> >::const_iterator RelocIter;
    
    Disassembler& disassembler; ///< disassembler instance
    size_t startOffset; ///< start offset
    size_t labelStartOffset; /// < start offset of labels
    size_t inputSize;   ///< size of input
    const cxbyte* input;    ///< input code
    bool dontPrintLabelsAfterCode;
    std::vector<size_t> labels; ///< list of local labels
    std::vector<std::pair<size_t, CString> > namedLabels;   ///< named labels
    std::vector<CString> relSymbols;    ///< symbols used by relocations
    std::vector<std::pair<size_t, Relocation> > relocations;    ///< relocations
    FastOutputBuffer output;    ///< output buffer
    
    /// constructor
    explicit ISADisassembler(Disassembler& disassembler, cxuint outBufSize = 600);
    
    /// write location in the code
    void writeLocation(size_t pos);
    /// write relocation to current place in instruction
    bool writeRelocation(size_t pos, RelocIter& relocIter);
    
public:
    virtual ~ISADisassembler();
    
    /// write all labels before specified position
    void writeLabelsToPosition(size_t pos, LabelIter& labelIter,
               NamedLabelIter& namedLabelIter);
    /// write all labels to end
    void writeLabelsToEnd(size_t start, LabelIter labelIter, NamedLabelIter namedLabelIter);
    
    /// set input code
    void setInput(size_t inputSize, const cxbyte* input, size_t startOffset = 0,
        size_t labelStartOffset = 0)
    {
        this->inputSize = inputSize;
        this->input = input;
        this->startOffset = startOffset;
        this->labelStartOffset = labelStartOffset;
    }
    
    void setDontPrintLabels(bool after)
    { dontPrintLabelsAfterCode = after; }
    
    /// analyze code before disassemblying
    virtual void analyzeBeforeDisassemble() = 0;
    
    /// first part before disassemble - clear numbered labels
    void clearNumberedLabels();
    /// last part before disassemble - prepare labels (sorting) 
    void prepareLabelsAndRelocations();
    /// makes some things before disassemblying
    void beforeDisassemble();
    /// disassembles input code
    virtual void disassemble() = 0;

    /// add named label to list (must be called before disassembly)
    void addNamedLabel(size_t pos, const CString& name)
    { namedLabels.push_back(std::make_pair(pos, name)); }
    /// add named label to list (must be called before disassembly)
    void addNamedLabel(size_t pos, CString&& name)
    { namedLabels.push_back(std::make_pair(pos, name)); }
    
    /// add symbol to relocations
    size_t addRelSymbol(const CString& symName)
    {
        size_t index = relSymbols.size();
        relSymbols.push_back(symName);
        return index;
    }
    /// add relocation
    void addRelocation(size_t offset, RelocType type, size_t symIndex, int64_t addend)
    { relocations.push_back(std::make_pair(offset, Relocation{symIndex, type, addend})); }
    /// clear all relocations
    void clearRelocations()
    {
        relSymbols.clear();
        relocations.clear();
    }
    /// get numbered labels
    const std::vector<size_t>& getLabels() const
    { return labels; }
    /// get named labels
    const std::vector<std::pair<size_t, CString> >& getNamedLabels() const
    { return namedLabels; }
    /// flush output
    void flushOutput()
    { return output.flush(); }
};

/// GCN architectur dissassembler
class GCNDisassembler: public ISADisassembler
{
private:
    bool instrOutOfCode;
    
    friend struct GCNDisasmUtils; // INTERNAL LOGIC
public:
    /// constructor
    GCNDisassembler(Disassembler& disassembler);
    /// destructor
    ~GCNDisassembler();
    
    /// analyze code before disassemblying
    void analyzeBeforeDisassemble();
    /// disassemble code
    void disassemble();
};

/// single kernel input for disassembler
/** all pointer members holds only pointers that should be freed by your routines.
 * No management of data */
struct AmdDisasmKernelInput
{
    CString kernelName; ///< kernel name
    size_t metadataSize;    ///< metadata size
    const char* metadata;   ///< kernel's metadata
    size_t headerSize;  ///< kernel header size
    const cxbyte* header;   ///< kernel header size
    std::vector<CALNoteInput> calNotes;   ///< ATI CAL notes
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
    CString driverInfo; ///< driver info (for AMD Catalyst drivers)
    CString compileOptions; ///< compile options which used by in clBuildProgram
    size_t globalDataSize;  ///< global (constants for kernels) data size
    const cxbyte* globalData;   ///< global (constants for kernels) data
    std::vector<AmdDisasmKernelInput> kernels;    ///< kernel inputs
};

/// relocation with addend
struct AmdCL2RelaEntry
{
    size_t offset;  ///< offset
    RelocType type; ///< relocation type
    cxuint symbol;  ///< symbol
    int64_t addend; ///< addend
};

/// single kernel input for disassembler
/** all pointer members holds only pointers that should be freed by your routines.
 * No management of data */
struct AmdCL2DisasmKernelInput
{
    CString kernelName; ///< kernel name
    size_t metadataSize;    ///< metadata size
    const cxbyte* metadata;   ///< kernel's metadata
    size_t isaMetadataSize;    ///< metadata size
    const cxbyte* isaMetadata;   ///< kernel's metadata
    size_t setupSize;    ///< data (from inner binary) size
    const cxbyte* setup; ///< data from inner binary
    size_t stubSize;    ///< data (from inner binary) size
    const cxbyte* stub; ///< data from inner binary
    std::vector<AmdCL2RelaEntry> textRelocs;    ///< text relocations
    size_t codeSize;    ///< size of code of kernel
    const cxbyte* code; ///< code of kernel
};

/// whole disassembler input (for AMD Catalyst driver GPU binaries)
/** all pointer members holds only pointers that should be freed by your routines.
 * No management of data */
struct AmdCL2DisasmInput
{
    GPUDeviceType deviceType;   ///< GPU device type
    uint32_t archMinor;     ///< GPU arch minor
    uint32_t archStepping;     ///< GPU arch stepping
    bool is64BitMode;       ///< true if 64-bit mode of addressing
    cxuint driverVersion; ///< driver version
    CString compileOptions; ///< compile options which used by in clBuildProgram
    CString aclVersionString; ///< acl version string
    size_t globalDataSize;  ///< global (constants for kernels) data size
    const cxbyte* globalData;   ///< global (constants for kernels) data
    size_t rwDataSize;  ///< global rw data size
    const cxbyte* rwData;   ///< global rw data data
    size_t bssAlignment;    ///< alignment of global bss section
    size_t bssSize;         ///< size of global bss section
    size_t samplerInitSize;     ///< sampler init data size
    const cxbyte* samplerInit;  ///< sampler init data
    size_t codeSize;            ///< code size
    const cxbyte* code;         ///< code
    std::vector<AmdCL2RelaEntry> textRelocs;    ///< text relocations
    
    /// sampler relocations
    std::vector<std::pair<size_t, size_t> > samplerRelocs;
    std::vector<AmdCL2DisasmKernelInput> kernels;    ///< kernel inputs
};

/// disasm ROCm kernel input
struct ROCmDisasmKernelInput
{
    CString kernelName; ///< kernel name
    const cxbyte* setup;    ///< setup
    size_t codeSize;    ///< code size
    size_t offset;      ///< kernel offset
};

/// disasm ROCm region
struct ROCmDisasmRegionInput
{
    CString regionName; ///< region name
    size_t size;    ///< region size
    size_t offset;  ///< region offset in code
    ROCmRegionType type;  ///< type
};

/// disasm ROCm input
struct ROCmDisasmInput
{
    GPUDeviceType deviceType;   ///< GPU device type
    uint32_t archMinor;     ///< GPU arch minor
    uint32_t archStepping;     ///< GPU arch stepping
    uint32_t eflags;   ///< ELF header e_flags field
    bool newBinFormat;  ///< new binary format
    std::vector<ROCmDisasmRegionInput> regions;  ///< regions
    size_t codeSize;    ///< code size
    const cxbyte* code; ///< code
    size_t globalDataSize;    ///< global data size
    const cxbyte* globalData; ///< global data
    CString target;     ///< LLVM target triple
    size_t metadataSize;    ///< metadata size
    const char* metadata;   ///< metadata
    Array<std::pair<CString, size_t> > gotSymbols; ///< GOT symbols names
};

/// disasm kernel info structure (Gallium binaries)
struct GalliumDisasmKernelInput
{
    CString kernelName;   ///< kernel's name
    GalliumProgInfoEntry progInfo[5];   ///< program info for kernel
    uint32_t offset;    ///< offset of kernel code
    std::vector<GalliumArgInfo> argInfos;   ///< arguments
};

/// whole disassembler input (for Gallium driver GPU binaries)
struct GalliumDisasmInput
{
    GPUDeviceType deviceType;   ///< GPU device type
    bool is64BitMode;       ///< true if 64-bit mode of addressing
    bool isLLVM390;     ///< if >=LLVM3.9
    bool isMesa170;     ///< if >=Mesa3D 17.0
    bool isAMDHSA;       ///< if AMDHSA (LLVM 4.0)
    size_t globalDataSize;  ///< global (constants for kernels) data size
    const cxbyte* globalData;   ///< global (constants for kernels) data
    std::vector<GalliumDisasmKernelInput> kernels;    ///< list of input kernels
    size_t codeSize;    ///< code size
    const cxbyte* code; ///< code
    std::vector<GalliumScratchReloc> scratchRelocs; ///< scratch buffer text relocations
};

/// disassembler input for raw code
struct RawCodeInput
{
    GPUDeviceType deviceType;   ///< GPU device type
    size_t codeSize;            ///< code size
    const cxbyte* code;         ///< code
};

/// disassembler class
class Disassembler: public NonCopyableAndNonMovable
{
private:
    friend class ISADisassembler;
    std::unique_ptr<ISADisassembler> isaDisassembler;
    bool fromBinary;
    BinaryFormat binaryFormat;
    union {
        const AmdDisasmInput* amdInput;
        const AmdCL2DisasmInput* amdCL2Input;
        const GalliumDisasmInput* galliumInput;
        const ROCmDisasmInput* rocmInput;
        const RawCodeInput* rawInput;
    };
    std::ostream& output;
    Flags flags;
    size_t sectionCount;
public:
    /// constructor for 32-bit GPU binary
    /**
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const AmdMainGPUBinary32& binary, std::ostream& output,
                 Flags flags = 0);
    /// constructor for 64-bit GPU binary
    /**
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const AmdMainGPUBinary64& binary, std::ostream& output,
                 Flags flags = 0);
    /// constructor for AMD OpenCL 2.0 GPU binary 32-bit
    /**
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     * \param driverVersion driverVersion (0 - detected by disassembler)
     */
    Disassembler(const AmdCL2MainGPUBinary32& binary, std::ostream& output,
                 Flags flags = 0, cxuint driverVersion = 0);
    /// constructor for AMD OpenCL 2.0 GPU binary 64-bit
    /**
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     * \param driverVersion driverVersion (0 - detected by disassembler)
     */
    Disassembler(const AmdCL2MainGPUBinary64& binary, std::ostream& output,
                 Flags flags = 0, cxuint driverVersion = 0);
    /// constructor for ROCm GPU binary
    /**
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const ROCmBinary& binary, std::ostream& output, Flags flags = 0);
    /// constructor for AMD disassembler input
    /**
     * \param disasmInput disassembler input object
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const AmdDisasmInput* disasmInput, std::ostream& output,
                 Flags flags = 0);
    /// constructor for AMD OpenCL 2.0 disassembler input
    /**
     * \param disasmInput disassembler input object
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const AmdCL2DisasmInput* disasmInput, std::ostream& output,
                 Flags flags = 0);
    /// constructor for ROCMm disassembler input
    /**
     * \param disasmInput disassembler input object
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const ROCmDisasmInput* disasmInput, std::ostream& output,
                 Flags flags = 0);
    
    /// constructor for bit GPU binary from Gallium
    /**
     * \param deviceType GPU device type
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     * \param llvmVersion LLVM version
     */
    Disassembler(GPUDeviceType deviceType, const GalliumBinary& binary,
                 std::ostream& output, Flags flags = 0, cxuint llvmVersion = 0);
    
    /// constructor for Gallium disassembler input
    /**
     * \param disasmInput disassembler input object
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const GalliumDisasmInput* disasmInput, std::ostream& output,
                 Flags flags = 0);
    
    /// constructor for raw code
    Disassembler(GPUDeviceType deviceType, size_t rawCodeSize, const cxbyte* rawCode,
                 std::ostream& output, Flags flags = 0);
    
    ~Disassembler();
    
    /// disassembles input
    void disassemble();
    
    /// get disassemblers flags
    Flags getFlags() const
    { return flags; }
    /// get disassemblers flags
    void setFlags(Flags flags)
    { this->flags = flags; }
    
    /// get deviceType
    GPUDeviceType getDeviceType() const;
    
    /// get disassembler input
    const AmdDisasmInput* getAmdInput() const
    { return amdInput; }
    
    /// get disassembler input
    const AmdCL2DisasmInput* getAmdCL2Input() const
    { return amdCL2Input; }
    
    /// get disassembler input
    const GalliumDisasmInput* getGalliumInput() const
    { return galliumInput; }
    
    /// get output stream
    const std::ostream& getOutput() const
    { return output; }
    /// get output stream
    std::ostream& getOutput()
    { return output; }
};

// routines to get binary config inputs

/// prepare AMD OpenCL input from AMD 32-bit binary
extern AmdDisasmInput* getAmdDisasmInputFromBinary32(
            const AmdMainGPUBinary32& binary, Flags flags);
/// prepare AMD OpenCL input from AMD 64-bit binary
extern AmdDisasmInput* getAmdDisasmInputFromBinary64(
            const AmdMainGPUBinary64& binary, Flags flags);
/// prepare AMD OpenCL 2.0 input from AMD 32-bit binary
extern AmdCL2DisasmInput* getAmdCL2DisasmInputFromBinary32(
            const AmdCL2MainGPUBinary32& binary, cxuint driverVersion,
            bool hsaLayout = false);
/// prepare AMD OpenCL 2.0 input from AMD 64-bit binary
extern AmdCL2DisasmInput* getAmdCL2DisasmInputFromBinary64(
            const AmdCL2MainGPUBinary64& binary, cxuint driverVersion,
            bool hsaLayout = false);
/// prepare ROCM input from ROCM binary
extern ROCmDisasmInput* getROCmDisasmInputFromBinary(
            const ROCmBinary& binary);
/// prepare Gallium input from Gallium binary
extern GalliumDisasmInput* getGalliumDisasmInputFromBinary(
            GPUDeviceType deviceType, const GalliumBinary& binary, cxuint llvmVersion);

};

#endif
