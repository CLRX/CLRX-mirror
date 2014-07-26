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



class ISAAssembler
{
public:
    typedef std::unordered_map<std::string, uint64_t> SymbolMap;
private:
    SymbolMap& symbolMap;
    
    ISAAssembler(SymbolMap& symbolMap);
public:
    virtual ~ISAAssembler();
    
    virtual size_t getMaxOutputSize() const = 0;
    virtual size_t assemble(size_t lineSize, const char* line, char* output) = 0;
};

class ISADisassembler
{
private:
    ISADisassembler();
public:
    virtual ~ISADisassembler();
    
    virtual size_t getMaxISASize() const = 0;
    virtual size_t assemble(size_t lineSize, const char* line, char* outBinary) = 0;
};

class Assembler
{
protected:
    explicit Assembler(const char* string);
    explicit Assembler(const std::istream& is);
public:
    virtual ~Assembler();
    
    void assemble();
};

class DisassemblerBase
{
protected:
    explicit DisassemblerBase(const std::ostream& os);
public
};

};

#endif

