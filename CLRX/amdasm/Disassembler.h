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
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/InputOutput.h>

/// main namespace
namespace CLRX
{

class Disassembler;

/// binary for Disassembler
enum class BinaryFormat
{
    AMD = 0,    ///< AMD CATALYST format
    GALLIUM     ///< GalliumCompute format
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

/// main class for
class ISADisassembler
{
protected:
    typedef std::vector<size_t>::const_iterator LabelIter;  ///< label iterator
    
    /// named label iterator
    typedef std::vector<std::pair<size_t, std::string> >::const_iterator NamedLabelIter;
    
    Disassembler& disassembler; ///< disassembler instance
    size_t inputSize;   ///< size of input
    const cxbyte* input;    ///< input code
    std::vector<size_t> labels; ///< list of local labels
    std::vector<std::pair<size_t, std::string> > namedLabels;   ///< named labels
    FastOutputBuffer output;    ///< output buffer
    
    /// constructor
    explicit ISADisassembler(Disassembler& disassembler, cxuint outBufSize = 300);
    
    /// write all labels before specified position
    void writeLabelsToPosition(size_t pos, LabelIter& labelIter,
               NamedLabelIter& namedLabelIter);
    /// write all labels to end
    void writeLabelsToEnd(size_t start, LabelIter labelIter, NamedLabelIter namedLabelIter);
    /// write location in the code
    void writeLocation(size_t pos);
public:
    virtual ~ISADisassembler();
    
    /// set input code
    void setInput(size_t inputSize, const cxbyte* input)
    {
        this->inputSize = inputSize;
        this->input = input;
    }

    /// makes some things before disassemblying
    virtual void beforeDisassemble() = 0;
    /// disassembles input code
    virtual void disassemble() = 0;

    /// add named label to list (must be called before disassembly)
    void addNamedLabel(size_t pos, const std::string& name)
    { namedLabels.push_back(std::make_pair(pos, name)); }
};

/// GCN architectur dissassembler
class GCNDisassembler: public ISADisassembler
{
private:
    bool instrOutOfCode;
public:
    /// constructor
    GCNDisassembler(Disassembler& disassembler);
    /// destructor
    ~GCNDisassembler();
    
    /// routine called before main disassemblying
    void beforeDisassemble();
    /// disassemble code
    void disassemble();
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
    std::string driverInfo; ///< driver info (for AMD Catalyst drivers)
    std::string compileOptions; ///< compile options which used by in clBuildProgram
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
    std::vector<GalliumKernelInput> kernels;    ///< list of input kernels
    size_t codeSize;    ///< code size
    const cxbyte* code; ///< code
};

/// disassembler class
class Disassembler
{
private:
    std::unique_ptr<ISADisassembler> isaDisassembler;
    bool fromBinary;
    BinaryFormat binaryFormat;
    union {
        const AmdDisasmInput* amdInput;
        const GalliumDisasmInput* galliumInput;
    };
    std::ostream& output;
    cxuint flags;
    
    void disassembleAmd(); // Catalyst format
    void disassembleGallium(); // Gallium format
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
    /// constructor for AMD disassembler input
    /**
     * \param disasmInput disassembler input object
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const AmdDisasmInput* disasmInput, std::ostream& output,
                 cxuint flags = 0);
    
    /// constructor for bit GPU binary from Gallium
    /**
     * \param deviceType GPU device type
     * \param binary main GPU binary
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(GPUDeviceType deviceType, const GalliumBinary& binary,
                 std::ostream& output, cxuint flags = 0);
    
    /// constructor for Gallium disassembler input
    /**
     * \param disasmInput disassembler input object
     * \param output output stream
     * \param flags flags for disassembler
     */
    Disassembler(const GalliumDisasmInput* disasmInput, std::ostream& output,
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
    
    /// get deviceType
    GPUDeviceType getDeviceType() const;
    
    /// get disassembler input
    const AmdDisasmInput* getAmdInput() const
    { return amdInput; }
    
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

};

#endif
