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
#include <string>
#include <istream>
#include <vector>
#include <unordered_map>
#include <CLRX/Utilities.h>

/// main namespace
namespace CLRX
{

class Assembler;
class Disassembler;

enum: cxuint
{
    DISASM_ADDRESS = 1,
    DISASM_HEXCODE = 2
};

enum class Disasm

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
    const char* input;
    
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

class Assembler
{
public:
    explicit Assembler(const char* string);
    explicit Assembler(const std::istream& is);
    ~Assembler();
    
    void assemble();
    
    const SymbolMap& getSymbolMap() const;
    uint64_t parseExpression(size_t stringSize, const char* string) const;
};

class Disassembler
{
protected:
public:
    Disassembler(size_t maxSize, char* output);
    explicit Disassembler(const std::ostream& os);
    ~Disassembler();
    
    void disassemble();
};

};

#endif
