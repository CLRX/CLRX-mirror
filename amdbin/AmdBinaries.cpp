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

#include <CLRX/Config.h>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <algorithm>
#include <climits>
#include <unordered_map>
#include <map>
#include <utility>
#include <vector>
#include <CLRX/Utilities.h>
#include <CLRX/AmdBinaries.h>

using namespace CLRX;

static const KernelArgType x86ArgTypeTable[]
{
    VOID,
    CHAR,
    SHORT,
    INT,
    LONG,
    FLOAT,
    DOUBLE,
    POINTER,
    CHAR2,
    CHAR3,
    CHAR4,
    CHAR8,
    CHAR16,
    SHORT2,
    SHORT3,
    SHORT4,
    SHORT8,
    SHORT16,
    INT2,
    INT3,
    INT4,
    INT8,
    INT16,
    LONG2,
    LONG3,
    LONG4,
    LONG8,
    LONG16,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    FLOAT8,
    FLOAT16,
    DOUBLE2,
    DOUBLE3,
    DOUBLE4,
    DOUBLE8,
    DOUBLE16,
    SAMPLER
};

static const KernelArgType gpuArgTypeTable[]
{
    UCHAR,
    UCHAR2,
    UCHAR3,
    UCHAR4,
    UCHAR8,
    UCHAR16,
    CHAR,
    CHAR2,
    CHAR3,
    CHAR4,
    CHAR8,
    CHAR16,
    USHORT,
    USHORT2,
    USHORT3,
    USHORT4,
    USHORT8,
    USHORT16,
    SHORT,
    SHORT2,
    SHORT3,
    SHORT4,
    SHORT8,
    SHORT16,
    UINT,
    UINT2,
    UINT3,
    UINT4,
    UINT8,
    UINT16,
    INT,
    INT2,
    INT3,
    INT4,
    INT8,
    INT16,
    ULONG,
    ULONG2,
    ULONG3,
    ULONG4,
    ULONG8,
    ULONG16,
    LONG,
    LONG2,
    LONG3,
    LONG4,
    LONG8,
    LONG16,
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    FLOAT8,
    FLOAT16,
    DOUBLE,
    DOUBLE2,
    DOUBLE3,
    DOUBLE4,
    DOUBLE8,
    DOUBLE16
};


static const uint32_t elfMagicValue = 0x464c457fU;

/* kernel info class */

KernelInfo::KernelInfo() : argsNum(0), argInfos(nullptr)
{
}

KernelInfo::~KernelInfo()
{
    delete[] argInfos;
}

KernelInfo::KernelInfo(const KernelInfo& cp)
{
    kernelName = cp.kernelName;
    argsNum = cp.argsNum;
    argInfos = new KernelArg[argsNum];
    std::copy(cp.argInfos, cp.argInfos + argsNum, argInfos);
}

KernelInfo::KernelInfo(KernelInfo&& cp)
{
    kernelName = std::move(cp.kernelName);
    argsNum = cp.argsNum;
    argInfos = cp.argInfos;
    cp.argsNum = 0;
    cp.argInfos = nullptr; // reset pointer
}

KernelInfo& KernelInfo::operator=(const KernelInfo& cp)
{
    delete[] argInfos;
    argInfos = nullptr;
    kernelName = cp.kernelName;
    argsNum = cp.argsNum;
    argInfos = new KernelArg[argsNum];
    std::copy(cp.argInfos, cp.argInfos + argsNum, argInfos);
    return *this;
}

KernelInfo& KernelInfo::operator=(KernelInfo&& cp)
{
    delete[] argInfos;
    argInfos = nullptr;
    kernelName = std::move(cp.kernelName);
    argsNum = cp.argsNum;
    argInfos = cp.argInfos;
    cp.argsNum = 0;
    cp.argInfos = nullptr; // reset pointer
    return *this;
}

void KernelInfo::allocateArgs(cxuint argsNum)
{
    delete[] argInfos;
    argInfos = nullptr;
    this->argsNum = argsNum;
    argInfos = new KernelArg[argsNum];
}

/* determine unfinished strings region in string table for checking further consistency */
static size_t unfinishedRegionOfStringTable(const char* table, size_t size)
{
    if (size == 0) // if zero
        return 0;
    size_t k;
    for (k = size-1; k>0 && table[k]!=0; k--);
    
    return (table[k]==0)?k+1:k;
}

/* ElfBinaryBase */

ElfBinaryBase::ElfBinaryBase() : binaryCodeSize(0), binaryCode(nullptr),
        sectionStringTable(nullptr), symbolStringTable(nullptr),
        symbolTable(nullptr), dynSymStringTable(nullptr), dynSymTable(nullptr)
{
}

/* ElfBinary32 */

ElfBinary32::ElfBinary32() : symbolsNum(0), dynSymbolsNum(0),
        symbolEntSize(0), dynSymEntSize(0)
{
}

ElfBinary32::ElfBinary32(size_t binaryCodeSize, char* binaryCode, cxuint creationFlags) :
        symbolsNum(0), dynSymbolsNum(0), symbolEntSize(0), dynSymEntSize(0)
{
    this->creationFlags = creationFlags;
    this->binaryCode = binaryCode;
    this->binaryCodeSize = binaryCodeSize;
    
    if (binaryCodeSize < sizeof(Elf32_Ehdr))
        throw Exception("Binary is too small!!!");
    
    const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(binaryCode);
    if (*reinterpret_cast<const uint32_t*>(binaryCode) != elfMagicValue)
        throw Exception("This is not ELF binary");
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
        throw Exception("This is not 32-bit ELF binary");
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        throw Exception("Other than little-endian binaries are not supported!");
    
    if (ehdr->e_phoff != 0)
    {   /* reading and checking program headers */
        if (ehdr->e_phoff > binaryCodeSize)
            throw Exception("ProgramHeaders offset out of range!");
        if (ehdr->e_phoff + size_t(ehdr->e_phentsize)*ehdr->e_phnum > binaryCodeSize)
            throw Exception("ProgramHeaders offset+size out of range!");
        
        for (cxuint i = 0; i < ehdr->e_phnum; i++)
        {
            const Elf32_Phdr& phdr = getProgramHeader(i);
            if (phdr.p_offset > binaryCodeSize)
                throw Exception("Segment offset out of range!");
            if (phdr.p_offset+phdr.p_filesz > binaryCodeSize)
                throw Exception("Segment offset+size out of range!");
        }
    }
    
    if (ehdr->e_shoff != 0 && ehdr->e_shstrndx != SHN_UNDEF)
    {   /* indexing of sections */
        if (ehdr->e_shoff > binaryCodeSize)
            throw Exception("SectionHeaders offset out of range!");
        if (ehdr->e_shoff + size_t(ehdr->e_shentsize)*ehdr->e_shnum > binaryCodeSize)
            throw Exception("SectionHeaders offset+size out of range!");
        if (ehdr->e_shstrndx >= ehdr->e_shnum)
            throw Exception("Shstrndx out of range!");
        
        Elf32_Shdr& shstrShdr = getSectionHeader(ehdr->e_shstrndx);
        sectionStringTable = binaryCode + shstrShdr.sh_offset;
        const size_t unfinishedShstrPos = unfinishedRegionOfStringTable(
                    sectionStringTable, shstrShdr.sh_size);
        
        const Elf32_Shdr* symTableHdr = nullptr;
        const Elf32_Shdr* dynSymTableHdr = nullptr;
        
        for (cxuint i = 0; i < ehdr->e_shnum; i++)
        {
            const Elf32_Shdr& shdr = getSectionHeader(i);
            if (shdr.sh_offset > binaryCodeSize)
                throw Exception("Section offset out of range!");
            if (shdr.sh_type != SHT_NOBITS)
                if (shdr.sh_offset+shdr.sh_size > binaryCodeSize)
                    throw Exception("Section offset+size out of range!");
            if (shdr.sh_link >= ehdr->e_shnum)
                throw Exception("Section link out of range!");
            
            if (shdr.sh_name >= shstrShdr.sh_size)
                throw Exception("Section name index out of range!");
            
            if (shdr.sh_name >= unfinishedShstrPos)
                throw Exception("Unfinished section name!");
            
            const char* shname = sectionStringTable + shdr.sh_name;
            
            if (*shname != 0 && (creationFlags & ELF_CREATE_SECTIONMAP) != 0)
                sectionIndexMap.insert(std::make_pair(shname, i));
            if (shdr.sh_type == SHT_SYMTAB)
                symTableHdr = &shdr;
            if (shdr.sh_type == SHT_DYNSYM)
                dynSymTableHdr = &shdr;
        }
        
        if (symTableHdr != nullptr)
        {   // indexing symbols
            if (symTableHdr->sh_entsize < sizeof(Elf32_Sym))
                throw Exception("SymTable entry size is too small!");
            
            symbolEntSize = symTableHdr->sh_entsize;
            symbolTable = binaryCode + symTableHdr->sh_offset;
            if (symTableHdr->sh_link == SHN_UNDEF)
                throw Exception("Symbol table doesnt have string table");
            
            Elf32_Shdr& symstrShdr = getSectionHeader(symTableHdr->sh_link);
            symbolStringTable = binaryCode + symstrShdr.sh_offset;
            
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    symbolStringTable, symstrShdr.sh_size);
            symbolsNum = symTableHdr->sh_size/symTableHdr->sh_entsize;
            
            for (uint32_t i = 0; i < symbolsNum; i++)
            {   /* verify symbol names */
                const Elf32_Sym& sym = getSymbol(i);
                if (sym.st_name >= symstrShdr.sh_size)
                    throw Exception("Symbol name index out of range!");
                if (sym.st_name >= unfinishedSymstrPos)
                    throw Exception("Unfinished symbol name!");
                
                const char* symname = symbolStringTable + sym.st_name;
                // add to symbol map
                if (*symname != 0 && (creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                    symbolIndexMap.insert(std::make_pair(symname, i));
            }
        }
        if (dynSymTableHdr != nullptr)
        {   // indexing dynamic symbols
            if (dynSymTableHdr->sh_entsize < sizeof(Elf32_Sym))
                throw Exception("DynSymTable entry size is too small!");
            
            dynSymEntSize = dynSymTableHdr->sh_entsize;
            dynSymTable = binaryCode + dynSymTableHdr->sh_offset;
            if (dynSymTableHdr->sh_link == SHN_UNDEF)
                throw Exception("DynSymbol table doesnt have string table");
            
            Elf32_Shdr& dynSymstrShdr = getSectionHeader(dynSymTableHdr->sh_link);
            dynSymbolsNum = dynSymTableHdr->sh_size/dynSymTableHdr->sh_entsize;
            
            dynSymStringTable = binaryCode + dynSymstrShdr.sh_offset;
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    dynSymStringTable, dynSymstrShdr.sh_size);
            
            for (uint32_t i = 0; i < dynSymbolsNum; i++)
            {   /* verify symbol names */
                const Elf32_Sym& sym = getDynSymbol(i);
                if (sym.st_name >= dynSymstrShdr.sh_size)
                    throw Exception("DynSymbol name index out of range!");
                if (sym.st_name >= unfinishedSymstrPos)
                    throw Exception("Unfinished dynsymbol name!");
                
                const char* symname = dynSymStringTable + sym.st_name;
                // add to symbol map
                if (*symname != 0 && (creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                    dynSymIndexMap.insert(std::make_pair(symname, i));
            }
        }
    }
}

uint16_t ElfBinary32::getSectionIndex(const char* name) const
{
    if (hasSectionMap())
    {
        SectionIndexMap::const_iterator it = sectionIndexMap.find(name);
        if (it == sectionIndexMap.end())
            throw Exception("Cant find Elf32 Section");
        return it->second;
    }
    else
    {
        for (cxuint i = 0; i < getSectionHeadersNum(); i++)
        {
            if (::strcmp(getSectionName(i), name) == 0)
                return i;
        }
        throw Exception("Cant find Elf32 Section");
    }
}

uint32_t ElfBinary32::getSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = symbolIndexMap.find(name);
    if (it == symbolIndexMap.end())
        throw Exception("Cant find Elf32 Symbol");
    return it->second;
}

uint32_t ElfBinary32::getDynSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = dynSymIndexMap.find(name);
    if (it == dynSymIndexMap.end())
        throw Exception("Cant find Elf32 DynSymbol");
    return it->second;
}

/* ElfBinary64 */

ElfBinary64::ElfBinary64() : symbolsNum(0), dynSymbolsNum(0),
        symbolEntSize(0), dynSymEntSize(0)
{
}

ElfBinary64::ElfBinary64(size_t binaryCodeSize, char* binaryCode, cxuint creationFlags) :
        symbolsNum(0), dynSymbolsNum(0), symbolEntSize(0), dynSymEntSize(0)
{
    this->binaryCode = binaryCode;
    this->binaryCodeSize = binaryCodeSize;
    this->creationFlags = creationFlags;
    
    if (binaryCodeSize < sizeof(Elf64_Ehdr))
        throw Exception("Binary is too small!!!");
    
    const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binaryCode);
    if (*reinterpret_cast<const uint32_t*>(binaryCode) != elfMagicValue)
        throw Exception("This is not ELF binary");
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
        throw Exception("This is not 64-bit ELF binary");
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        throw Exception("Other than little-endian binaries are not supported!");
    
    if (ehdr->e_phoff != 0)
    {   /* reading and checking program headers */
        if (ehdr->e_phoff > binaryCodeSize)
            throw Exception("ProgramHeaders offset out of range!");
        if (ehdr->e_phoff + size_t(ehdr->e_phentsize)*ehdr->e_phnum > binaryCodeSize)
            throw Exception("ProgramHeaders offset+size out of range!");
        
        for (cxuint i = 0; i < ehdr->e_phnum; i++)
        {
            const Elf64_Phdr& phdr = getProgramHeader(i);
            if (phdr.p_offset > binaryCodeSize)
                throw Exception("Segment offset out of range!");
            if (phdr.p_offset+phdr.p_filesz > binaryCodeSize)
                throw Exception("Segment offset+size out of range!");
        }
    }
    
    if (ehdr->e_shoff != 0 && ehdr->e_shstrndx != SHN_UNDEF)
    {   /* indexing of sections */
        if (ehdr->e_shoff > binaryCodeSize)
            throw Exception("SectionHeaders offset out of range!");
        if (ehdr->e_shoff + size_t(ehdr->e_shentsize)*ehdr->e_shnum > binaryCodeSize)
            throw Exception("SectionHeaders offset+size out of range!");
        if (ehdr->e_shstrndx >= ehdr->e_shnum)
            throw Exception("Shstrndx out of range!");
        
        Elf64_Shdr& shstrShdr = getSectionHeader(ehdr->e_shstrndx);
        sectionStringTable = binaryCode + shstrShdr.sh_offset;
        const size_t unfinishedShstrPos = unfinishedRegionOfStringTable(
                    sectionStringTable, shstrShdr.sh_size);
        
        const Elf64_Shdr* symTableHdr = nullptr;
        const Elf64_Shdr* dynSymTableHdr = nullptr;
        
        for (cxuint i = 0; i < ehdr->e_shnum; i++)
        {
            const Elf64_Shdr& shdr = getSectionHeader(i);
            if (shdr.sh_offset > binaryCodeSize)
                throw Exception("Section offset out of range!");
            if (shdr.sh_type != SHT_NOBITS)
                if (shdr.sh_offset+shdr.sh_size > binaryCodeSize)
                    throw Exception("Section offset+size out of range!");
            if (shdr.sh_link >= ehdr->e_shnum)
                throw Exception("Section link out of range!");
            
            if (shdr.sh_name >= shstrShdr.sh_size)
                throw Exception("Section name index out of range!");
            
            if (shdr.sh_name >= unfinishedShstrPos)
                throw Exception("Unfinished section name!");
            
            const char* shname = sectionStringTable + shdr.sh_name;
            if (*shname != 0 && (creationFlags & ELF_CREATE_SECTIONMAP) != 0)
                sectionIndexMap.insert(std::make_pair(shname, i));
            if (shdr.sh_type == SHT_SYMTAB)
                symTableHdr = &shdr;
            if (shdr.sh_type == SHT_DYNSYM)
                dynSymTableHdr = &shdr;
        }
        
        if (symTableHdr != nullptr)
        {   // indexing symbols
            if (symTableHdr->sh_entsize < sizeof(Elf64_Sym))
                throw Exception("SymTable entry size is too small!");
            
            symbolEntSize = symTableHdr->sh_entsize;
            symbolTable = binaryCode + symTableHdr->sh_offset;
            if (symTableHdr->sh_link == SHN_UNDEF)
                throw Exception("Symbol table doesnt have string table");
            
            Elf64_Shdr& symstrShdr = getSectionHeader(symTableHdr->sh_link);
            symbolStringTable = binaryCode + symstrShdr.sh_offset;
            
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    symbolStringTable, symstrShdr.sh_size);
            symbolsNum = symTableHdr->sh_size/symTableHdr->sh_entsize;
            
            for (size_t i = 0; i < symbolsNum; i++)
            {   /* verify symbol names */
                const Elf64_Sym& sym = getSymbol(i);
                if (sym.st_name >= symstrShdr.sh_size)
                    throw Exception("Symbol name index out of range!");
                if (sym.st_name >= unfinishedSymstrPos)
                    throw Exception("Unfinished symbol name!");
                
                const char* symname = symbolStringTable + sym.st_name;
                // add to symbol map
                if (*symname != 0 && (creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                    symbolIndexMap.insert(std::make_pair(symname, i));
            }
        }
        if (dynSymTableHdr != nullptr)
        {   // indexing dynamic symbols
            if (dynSymTableHdr->sh_entsize < sizeof(Elf64_Sym))
                throw Exception("DynSymTable entry size is too small!");
            
            dynSymEntSize = dynSymTableHdr->sh_entsize;
            dynSymTable = binaryCode + dynSymTableHdr->sh_offset;
            if (dynSymTableHdr->sh_link == SHN_UNDEF)
                throw Exception("DynSymbol table doesnt have string table");
            
            Elf64_Shdr& dynSymstrShdr = getSectionHeader(dynSymTableHdr->sh_link);
            dynSymbolsNum = dynSymTableHdr->sh_size/dynSymTableHdr->sh_entsize;
            
            dynSymStringTable = binaryCode + dynSymstrShdr.sh_offset;
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    dynSymStringTable, dynSymstrShdr.sh_size);
            
            for (size_t i = 0; i < dynSymbolsNum; i++)
            {   /* verify symbol names */
                const Elf64_Sym& sym = getDynSymbol(i);
                if (sym.st_name >= dynSymstrShdr.sh_size)
                    throw Exception("DynSymbol name index out of range!");
                if (sym.st_name >= unfinishedSymstrPos)
                    throw Exception("Unfinished dynsymbol name!");
                
                const char* symname = dynSymStringTable + sym.st_name;
                // add to symbol map
                if (*symname != 0 && (creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                    dynSymIndexMap.insert(std::make_pair(symname, i));
            }
        }
    }
}

uint16_t ElfBinary64::getSectionIndex(const char* name) const
{
    if (hasSectionMap())
    {
        SectionIndexMap::const_iterator it = sectionIndexMap.find(name);
        if (it == sectionIndexMap.end())
            throw Exception("Cant find Elf64 Section");
        return it->second;
    }
    else
    {
        for (cxuint i = 0; i < getSectionHeadersNum(); i++)
        {
            if (::strcmp(getSectionName(i), name) == 0)
                return i;
        }
        throw Exception("Cant find Elf64 Section");
    }
}

size_t ElfBinary64::getSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = symbolIndexMap.find(name);
    if (it == symbolIndexMap.end())
        throw Exception("Cant find Elf64 Symbol");
    return it->second;
}

size_t ElfBinary64::getDynSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = dynSymIndexMap.find(name);
    if (it == dynSymIndexMap.end())
        throw Exception("Cant find Elf64 DynSymbol");
    return it->second;
}

/* AMD inner GPU binary */

AmdInnerGPUBinary32::AmdInnerGPUBinary32(const std::string& _kernelName,
         size_t binaryCodeSize, char* binaryCode, cxuint creationFlags)
        : ElfBinary32(binaryCodeSize, binaryCode, creationFlags), kernelName(_kernelName)
{
}

/* AMD inner X86 binary */

AmdInnerX86Binary32::AmdInnerX86Binary32(size_t binaryCodeSize, char* binaryCode,
         cxuint creationFlags) : ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
{
}

size_t AmdInnerX86Binary32::getKernelInfos(KernelInfo*& kernelInfos) const
{
    delete[] kernelInfos;
    kernelInfos = nullptr;
    
    if (binaryCode == nullptr)
        return 0;
    
    cxuint rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { return 0; /* no section */ }
    
    const Elf32_Shdr& rodataHdr = getSectionHeader(rodataIndex);
    
    /* get kernel metadata symbols */
    std::vector<uint32_t> choosenSyms;
    for (uint32_t i = 0; i < dynSymbolsNum; i++)
    {
        const char* symName = getDynSymbolName(i);
        const size_t len = ::strlen(symName);
        if (len < 18 || (::strncmp(symName, "__OpencCL_", 9) != 0 &&
            ::strcmp(symName+len-9, "_metadata") != 0)) // not metdata then skip
            continue;
        choosenSyms.push_back(i);
    }
    
    try
    {
    kernelInfos = new KernelInfo[choosenSyms.size()];
    
    const size_t unfinishedRegion = unfinishedRegionOfStringTable(
        binaryCode + rodataHdr.sh_offset, rodataHdr.sh_size);
    
    size_t ki = 0;
    for (auto i: choosenSyms)
    {
        const Elf32_Sym& sym = getDynSymbol(i);
        if (sym.st_shndx >= getSectionHeadersNum())
            throw Exception("Metadata section index out of range");
        
        const Elf32_Shdr& dataHdr = getSectionHeader(sym.st_shndx); // from symbol
        const size_t fileOffset = sym.st_value - dataHdr.sh_addr + dataHdr.sh_offset;
        if (fileOffset < dataHdr.sh_offset ||
            fileOffset >= dataHdr.sh_offset + dataHdr.sh_size)
            throw Exception("File offset of kernelMetadata out of range!");
        const char* data = binaryCode + fileOffset;
        
        /* parse number of args */
        uint32_t argsNum = (*reinterpret_cast<const uint32_t*>(data) - 44)/24;
        KernelInfo& kernelInfo = kernelInfos[ki++];
        
        const char* symName = getDynSymbolName(i);
        const size_t len = ::strlen(symName);
        kernelInfo.kernelName.assign(symName+9, len-18);
        kernelInfo.allocateArgs(argsNum);
        
        /* get argument info */
        for (uint32_t ai = 0; ai < argsNum; ai++)
        {
            const X86KernelArgSym& argNameSym =
                *reinterpret_cast<const X86KernelArgSym*>(
                    data + 32 + 16*2*size_t(ai));
            
            const X86KernelArgSym& argTypeSym =
                *reinterpret_cast<const X86KernelArgSym*>(
                    data + 32 + 16*(2*size_t(ai)+1));
            
            KernelArg& karg = kernelInfo.argInfos[ai];
            if (argNameSym.nameOffset < rodataHdr.sh_offset ||
                argNameSym.nameOffset >= rodataHdr.sh_offset+rodataHdr.sh_size)
                throw Exception("kernel arg name offset out of range!");
            
            if (argNameSym.nameOffset-rodataHdr.sh_offset >= unfinishedRegion)
                throw Exception("Arg name is unfinished!");
            
            if (argTypeSym.nameOffset < rodataHdr.sh_offset ||
                argTypeSym.nameOffset >= rodataHdr.sh_offset+rodataHdr.sh_size)
                throw Exception("kernel arg type offset out of range!");
            
            if (argTypeSym.nameOffset-rodataHdr.sh_offset >= unfinishedRegion)
                throw Exception("Type name is unfinished!");
            
            karg.argName = binaryCode + argNameSym.nameOffset;
            if (argNameSym.argType > 0x26)
                throw Exception("Unknown kernel arg type");
            karg.argType = x86ArgTypeTable[argNameSym.argType];
            karg.ptrSpace = static_cast<KernelPtrSpace>(argNameSym.ptrType);
            karg.ptrAccess = argNameSym.ptrAccess;
            karg.typeName = binaryCode + argTypeSym.nameOffset;
        }
    }
    }
    catch(...) // if exception happens
    {
        delete[] kernelInfos;
        kernelInfos = nullptr;
        throw;
    }
    return choosenSyms.size();
}

/* AMD inner X86-64 binary */

AmdInnerX86Binary64::AmdInnerX86Binary64(size_t binaryCodeSize, char* binaryCode,
         cxuint creationFlags) : ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{
}

size_t AmdInnerX86Binary64::getKernelInfos(KernelInfo*& kernelInfos) const
{
    delete[] kernelInfos;
    kernelInfos = nullptr;
    
    if (binaryCode == nullptr)
        return 0;
    
    cxuint rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { return 0; /* no section */ }
    
    const Elf64_Shdr& rodataHdr = getSectionHeader(rodataIndex);
    
    /* get kernel metadata symbols */
    std::vector<size_t> choosenSyms;
    for (size_t i = 0; i < dynSymbolsNum; i++)
    {
        const char* symName = getDynSymbolName(i);
        const size_t len = ::strlen(symName);
        if (len < 18 || (::strncmp(symName, "__OpencCL_", 9) != 0 &&
            ::strcmp(symName+len-9, "_metadata") != 0)) // not metdata then skip
            continue;
        choosenSyms.push_back(i);
    }
    
    try
    {
    kernelInfos = new KernelInfo[choosenSyms.size()];
    
    const size_t unfinishedRegion = unfinishedRegionOfStringTable(
        binaryCode + rodataHdr.sh_offset, rodataHdr.sh_size);
    
    size_t ki = 0;
    for (size_t i: choosenSyms)
    {
        const Elf64_Sym& sym = getDynSymbol(i);
        if (sym.st_shndx >= getSectionHeadersNum())
            throw Exception("Metadata section index out of range");
        
        const Elf64_Shdr& dataHdr = getSectionHeader(sym.st_shndx); // from symbol
        const size_t fileOffset = sym.st_value - dataHdr.sh_addr + dataHdr.sh_offset;
        if (fileOffset < dataHdr.sh_offset ||
            fileOffset >= dataHdr.sh_offset + dataHdr.sh_size)
            throw Exception("File offset of kernelMetadata out of range!");
        const char* data = binaryCode + fileOffset;
        
        /* parse number of args */
        size_t argsNum = (*reinterpret_cast<const uint64_t*>(data) - 80)/32;
        KernelInfo& kernelInfo = kernelInfos[ki++];
        
        const char* symName = getDynSymbolName(i);
        const size_t len = strlen(symName);
        kernelInfo.kernelName.assign(symName+9, len-18);
        kernelInfo.allocateArgs(argsNum);
        
        /* get argument info */
        for (size_t ai = 0; ai < argsNum; ai++)
        {
            const X86_64KernelArgSym& argNameSym =
                *reinterpret_cast<const X86_64KernelArgSym*>(
                    data + 64 + 20*2*size_t(ai));
                
            const X86_64KernelArgSym& argTypeSym =
                *reinterpret_cast<const X86_64KernelArgSym*>(
                    data + 64 + 20*(2*size_t(ai)+1));
            
            KernelArg& karg = kernelInfo.argInfos[ai];
            const size_t argNameOffset = 
                (uint64_t(argNameSym.nameOffsetHi)<<32) + argNameSym.nameOffsetLo;
            const size_t argTypeOffset = 
                (uint64_t(argTypeSym.nameOffsetHi)<<32) + argTypeSym.nameOffsetLo;
                
            if (argNameOffset < rodataHdr.sh_offset ||
                argNameOffset >= rodataHdr.sh_offset+rodataHdr.sh_size)
                throw Exception("kernel arg name offset out of range!");
            
            if (argNameOffset-rodataHdr.sh_offset >= unfinishedRegion)
                throw Exception("Arg name is unfinished!");
                
            if (argTypeOffset < rodataHdr.sh_offset ||
                argTypeOffset >= rodataHdr.sh_offset+rodataHdr.sh_size)
                throw Exception("kernel arg type offset out of range!");
            
            if (argTypeOffset-rodataHdr.sh_offset >= unfinishedRegion)
                throw Exception("Arg type name is unfinished!");
            
            karg.argName = binaryCode + argNameOffset;
            if (argNameSym.argType > 0x26)
                throw Exception("Unknown kernel arg type");
            karg.argType = x86ArgTypeTable[argNameSym.argType];
            karg.ptrSpace = static_cast<KernelPtrSpace>(argNameSym.ptrType);
            karg.ptrAccess = argNameSym.ptrAccess;
            karg.typeName = binaryCode + argTypeOffset;
        }
    }
    
    }
    catch(...) // if exception happens
    {
        delete[] kernelInfos;
        kernelInfos = nullptr;
        throw;
    }
    
    return choosenSyms.size();
}

/* AmdMaiBinaryBase */

AmdMainBinaryBase::AmdMainBinaryBase(AmdMainType _type) : type(_type),
        kernelInfosNum(0), kernelInfos(nullptr)
{
}

AmdMainBinaryBase::~AmdMainBinaryBase()
{
    delete[] kernelInfos;
}

const KernelInfo& AmdMainBinaryBase::getKernelInfo(const char* name) const
{
    KernelInfoMap::const_iterator it = kernelInfosMap.find(name);
    if (it == kernelInfosMap.end())
        throw Exception("Cant find kernel name");
    return kernelInfos[it->second];
}

/* AmdMainGPUBinary32 */

struct InitKernelArgMapEntry {
    uint32_t index;
    KernelArgType argType;
    KernelArgType origArgType;
    KernelPtrSpace ptrSpace;
    uint32_t ptrAccess;
    size_t namePos;
    size_t typePos;
    
    InitKernelArgMapEntry() : index(0), argType(KernelArgType::VOID),
        origArgType(KernelArgType::VOID),
        ptrSpace(KernelPtrSpace::NONE), ptrAccess(0), namePos(0), typePos(0)
    { }
};

static const cxuint vectorIdTable[17] =
{ UINT_MAX, 0, 1, 2, 3, UINT_MAX, UINT_MAX, UINT_MAX, 4,
  UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, 5 };

static const KernelArgType determineKernelArgType(const char* typeString,
           cxuint vectorSize, size_t lineNo)
{
    KernelArgType outType;
    
    if (vectorSize > 16)
        throw ParseException(lineNo, "Wrong vector size");
    const cxuint vectorId = vectorIdTable[vectorSize];
    if (vectorId == UINT_MAX)
        throw ParseException(lineNo, "Wrong vector size");
    
    if (::strncmp(typeString, "float:", 6) == 0)
        outType = gpuArgTypeTable[8*6+vectorId];
    else if (::strncmp(typeString, "double:", 7) == 0)
        outType = gpuArgTypeTable[9*6+vectorId];
    else if ((typeString[0] == 'i' || typeString[0] == 'u'))
    {
        /* indexBase - choose between unsigned/signed */
        const cxuint indexBase = (typeString[0] == 'i')?6:0;
        if (typeString[1] == '8')
        {
            if (typeString[2] != ':')
                throw ParseException(lineNo, "Cant parse type");
            outType = gpuArgTypeTable[indexBase+vectorId];
        }
        else
        {
            if (typeString[1] == '1' && typeString[2] == '6')
                outType = gpuArgTypeTable[indexBase+2*6+vectorId];
            else if (typeString[1] == '3' && typeString[2] == '2')
                outType = gpuArgTypeTable[indexBase+4*6+vectorId];
            else if (typeString[1] == '6' && typeString[2] == '4')
                outType = gpuArgTypeTable[indexBase+6*6+vectorId];
            else // if not determined
                throw ParseException(lineNo, "Cant parse type");
            if (typeString[3] != ':')
                throw ParseException(lineNo, "Cant parse type");
        }
    }
    else
        throw ParseException(lineNo, "Cant parse type");
    
    return outType;
}

static inline std::string stringFromCStringDelim(const char* c1,
                 size_t maxSize, char delim)
{
    size_t i = 0;
    for (i = 0; i < maxSize && c1[i] != delim; i++);
    return std::string(c1, i);
}

typedef std::map<std::string, InitKernelArgMapEntry> InitKernelArgMap;

void AmdMainGPUBinary32::initKernelInfos(cxuint creationFlags,
         const std::vector<uint32_t>& metadataSyms)
{
    delete[] kernelInfos;
    kernelInfos = nullptr;
    
    try
    {
    kernelInfos = new KernelInfo[metadataSyms.size()];
    
    uint32_t ki = 0;
    for (uint32_t it: metadataSyms)
    {   // read symbol _OpenCL..._metadata
        const Elf32_Sym& sym = getSymbol(it);
        const char* symName = getSymbolName(it);
        if (sym.st_shndx >= getSectionHeadersNum())
            throw Exception("Metadata section index out of range");
        
        const Elf32_Shdr& rodataHdr = getSectionHeader(sym.st_shndx);
        const char* rodataContent = binaryCode + rodataHdr.sh_offset;
        
        if (sym.st_value > rodataHdr.sh_size)
            throw Exception("Metadata offset out of range");
        if (sym.st_value+sym.st_size > rodataHdr.sh_size)
            throw Exception("Metadata offset+size out of range");
        
        const char* kernelDesc = rodataContent + sym.st_value;
        /* parse kernel description */
        size_t lineNo = 1;
        size_t pos = 0;
        uint32_t argIndex = 0;
        
        InitKernelArgMap initKernelArgs;
        
        // first phase (value/pointer/image/sampler)
        while (pos < sym.st_size)
        {
            if (kernelDesc[pos] != ';')
                throw ParseException(lineNo, "This is not KernelDesc line");
            pos++;
            if (pos >= sym.st_size)
                throw ParseException(lineNo, "This is not KernelDesc line");
            
            size_t tokPos = pos;
            while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= sym.st_size)
                throw ParseException(lineNo, "Is not KernelDesc line");
            
            if (::strncmp(kernelDesc + pos, "value", tokPos-pos) == 0)
            { // value
                if (kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "This is not value line");
                
                pos = ++tokPos;
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size || kernelDesc[tokPos] =='\n')
                    throw ParseException(lineNo, "No separator after name");
                
                // extract arg name
                InitKernelArgMap::iterator argIt;
                {
                    InitKernelArgMapEntry entry;
                    entry.index = argIndex++;
                    entry.namePos = pos;
                    std::pair<InitKernelArgMap::iterator, bool> result = 
                        initKernelArgs.insert(std::make_pair(
                            std::string(kernelDesc+pos, tokPos-pos), entry));
                    if (!result.second)
                        throw ParseException(lineNo, "Argument has been duplicated");
                    argIt = result.first;
                }
                ///
                pos = ++tokPos;
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size || kernelDesc[tokPos] =='\n')
                    throw ParseException(lineNo, "No separator after type");
                // get arg type
                const char* argType = kernelDesc + pos;
                /// 
                pos = ++tokPos;
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size || kernelDesc[tokPos] =='\n')
                    throw ParseException(lineNo, "No separator after vector size");
                // get vector size
                {
                    const char* outEnd;
                    cxuint vectorSize = cstrtouiParse(kernelDesc + pos, kernelDesc + tokPos,
                        outEnd, ':', lineNo);
                    if (outEnd != kernelDesc + tokPos)
                        throw ParseException(lineNo, "Garbages after integer");
                    argIt->second.argType = determineKernelArgType(argType,
                           vectorSize, lineNo);
                }
                pos = tokPos;
            }
            else if (::strncmp(kernelDesc + pos, "pointer", tokPos-pos) == 0)
            { // pointer
                if (kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "This is not pointer line");
                
                pos = ++tokPos;
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size || kernelDesc[tokPos] =='\n')
                    throw ParseException(lineNo, "No separator after name");
                
                // extract arg name
                InitKernelArgMap::iterator argIt;
                {
                    InitKernelArgMapEntry entry;
                    entry.index = argIndex++;
                    entry.namePos = pos;
                    entry.argType = KernelArgType::POINTER;
                    std::pair<InitKernelArgMap::iterator, bool> result = 
                        initKernelArgs.insert(std::make_pair(
                            std::string(kernelDesc+pos, tokPos-pos), entry));
                    if (!result.second)
                        throw ParseException(lineNo, "Argument has been duplicated");
                    argIt = result.first;
                }
                ///
                ++tokPos;
                for (cxuint k = 0; k < 4; k++) // // skip four fields
                {
                    while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                        kernelDesc[tokPos] != '\n') tokPos++;
                    if (tokPos >= sym.st_size || kernelDesc[tokPos] =='\n')
                        throw ParseException(lineNo, "No separator after field");
                    tokPos++;
                }
                pos = tokPos;
                // get pointer type (global/local/constant)
                if (pos+4 <= sym.st_size && kernelDesc[pos] == 'u' && 
                    kernelDesc[pos+1] == 'a' && kernelDesc[pos+2] == 'v' &&
                    kernelDesc[pos+3] == ':')
                {
                    argIt->second.ptrSpace = KernelPtrSpace::GLOBAL;
                    pos += 4;
                }
                else if (pos+3 <= sym.st_size && kernelDesc[pos] == 'h' &&
                    kernelDesc[pos+1] == 'c' && kernelDesc[pos+2] == ':')
                {
                    argIt->second.ptrSpace = KernelPtrSpace::CONSTANT;
                    pos += 3;
                }
                else if (pos+3 <= sym.st_size && kernelDesc[pos] == 'h' &&
                    kernelDesc[pos+1] == 'l' && kernelDesc[pos+2] == ':')
                {
                    argIt->second.ptrSpace = KernelPtrSpace::LOCAL;
                    pos += 3;
                }
                else if (pos+2 <= sym.st_size && kernelDesc[pos] == 'c' &&
                      kernelDesc[pos+1] == ':')
                {
                    argIt->second.ptrSpace = KernelPtrSpace::CONSTANT;
                    pos += 2;
                }
                else //if not match
                    throw ParseException(lineNo, "Unknown pointer type");
            }
            else if (::strncmp(kernelDesc + pos, "image", tokPos-pos) == 0)
            { // image
                if (kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "This is not image line");
                
                pos = ++tokPos;
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size || kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "No separator after name");
                
                // extract arg name
                InitKernelArgMap::iterator argIt;
                {
                    InitKernelArgMapEntry entry;
                    entry.index = argIndex++;
                    entry.namePos = pos;
                    entry.ptrSpace = KernelPtrSpace::GLOBAL;
                    entry.argType = KernelArgType::IMAGE; // set as image
                    std::pair<InitKernelArgMap::iterator, bool> result = 
                        initKernelArgs.insert(std::make_pair(
                            std::string(kernelDesc+pos, tokPos-pos), entry));
                    if (!result.second)
                        throw ParseException(lineNo, "Argument has been duplicated");
                    argIt = result.first;
                }
                
                pos = ++tokPos; // skip next field
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size || kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "No separator after field");
                
                pos = ++tokPos;
                if (pos+3 > sym.st_size || kernelDesc[pos+2] != ':')
                    throw ParseException(lineNo, "Cant parse image access qualifier");
                
                if (kernelDesc[pos] == 'R' && kernelDesc[pos+1] == 'O')
                    argIt->second.ptrAccess |= KARG_PTR_READ_ONLY;
                else if (kernelDesc[pos] == 'W' && kernelDesc[pos+1] == 'O')
                    argIt->second.ptrAccess |= KARG_PTR_WRITE_ONLY;
                else
                    throw ParseException(lineNo, "Cant parse image access qualifier");
                pos += 3;
            }
            else if (::strncmp(kernelDesc + pos, "sampler", tokPos-pos) == 0)
            { // sampler (set up some argument as sampler
                if (kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "This is not sampler line");
                
                pos = ++tokPos;
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size)
                    throw ParseException(lineNo, "No separator after name");
                
                std::string argName(kernelDesc+pos, tokPos-pos);
                InitKernelArgMap::iterator argIt = initKernelArgs.find(argName);
                if (argIt != initKernelArgs.end())
                {
                    argIt->second.origArgType = argIt->second.argType;
                    argIt->second.argType = KernelArgType::SAMPLER;
                }
                
                pos = tokPos;
            }
            else if (::strncmp(kernelDesc + pos, "constarg", tokPos-pos) == 0)
            {
                if (kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "This is not constarg line");
                
                pos = ++tokPos;
                // skip number
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size || kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "No separator after field");
                
                pos = ++tokPos;
                while (tokPos < sym.st_size && kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size)
                    throw ParseException(lineNo, "End of data");
                
                /// put constant
                const std::string thisName(kernelDesc+pos, tokPos-pos);
                InitKernelArgMap::iterator argIt = initKernelArgs.find(thisName);
                if (argIt == initKernelArgs.end())
                    throw ParseException(lineNo, "Cant find constant argument");
                // set up const access type
                argIt->second.ptrAccess |= KARG_PTR_CONST;
                pos = tokPos;
            }
            else if (::strncmp(kernelDesc + pos, "reflection", tokPos-pos) == 0)
            {
                if (kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "This is not reflection line");
                break;
            }
            
            while (pos < sym.st_size && kernelDesc[pos] != '\n') pos++;
            lineNo++;
            pos++; // skip newline
        }
        
        KernelInfo& kernelInfo = kernelInfos[ki++];
        kernelInfo.kernelName.assign(symName+9, ::strlen(symName)-18);
        kernelInfo.allocateArgs(argIndex);
        
        for (const auto& e: initKernelArgs)
        {   /* initialize kernel arguments before set argument type from reflections */
            KernelArg& karg = kernelInfo.argInfos[e.second.index];
            karg.argType = e.second.argType;
            karg.ptrSpace = e.second.ptrSpace;
            karg.ptrAccess = e.second.ptrAccess;
            karg.argName = stringFromCStringDelim(kernelDesc + e.second.namePos,
                      sym.st_size-e.second.namePos, ':');
        }
        
        if (argIndex != 0)
        {   /* check whether not end */
            if (pos >= sym.st_size)
                throw ParseException(lineNo, "Unexpected end of data");
            
            // reflections
            pos--; // skip ';'
            while (pos < sym.st_size)
            {
                if (kernelDesc[pos] != ';')
                    throw ParseException(lineNo, "Is not KernelDesc line");
                pos++;
                if (pos >= sym.st_size)
                    throw ParseException(lineNo, "Is not KernelDesc line");
                
                if (pos+11 > sym.st_size ||
                    ::strncmp(kernelDesc+pos, "reflection:", 11) != 0)
                    break; // end!!!
                pos += 11;
                
                size_t tokPos = pos;
                while (tokPos < sym.st_size && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= sym.st_size || kernelDesc[tokPos] == '\n')
                    throw ParseException(lineNo, "Is not KernelDesc line");
                
                const char* outEnd;
                cxuint argIndex = cstrtouiParse(kernelDesc+pos, kernelDesc+tokPos,
                                outEnd, ':', lineNo);
                
                if (outEnd != kernelDesc + tokPos)
                    throw ParseException(lineNo, "Garbages after integer");
                if (argIndex >= kernelInfo.argsNum)
                    throw ParseException(lineNo, "Argument index out of range");
                pos = tokPos+1;
                kernelInfo.argInfos[argIndex].typeName = stringFromCStringDelim(
                    kernelDesc+pos, sym.st_size-pos, '\n');
                
                KernelArg& argInfo = kernelInfo.argInfos[argIndex];
                
                if (argInfo.argName.compare(0, 8, "unknown_") == 0 &&
                    argInfo.typeName != "sampler_t" &&
                    argInfo.argType == KernelArgType::SAMPLER)
                {   InitKernelArgMap::const_iterator argIt =
                                initKernelArgs.find(argInfo.argName);
                    if (argIt != initKernelArgs.end() &&
                        argIt->second.origArgType != KernelArgType::VOID)
                    {   /* revert sampler type and restore original arg type */
                        argInfo.argType = argIt->second.origArgType;
                    }
                }    
                
                while (pos < sym.st_size && kernelDesc[pos] != '\n') pos++;
                lineNo++;
                pos++;
            }
        }
        
        kernelInfosNum = metadataSyms.size();
        /* maps kernel info */
        if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
            for (size_t i = 0; i < kernelInfosNum; i++)
                kernelInfosMap.insert(
                    std::make_pair(kernelInfos[i].kernelName, i));
    }
    }
    catch(...)
    {
        delete[] kernelInfos;
        kernelInfos = nullptr;
        throw;
    }
}

AmdMainGPUBinary32::AmdMainGPUBinary32(size_t binaryCodeSize, char* binaryCode,
       cxuint creationFlags) : AmdMainBinaryBase(AmdMainType::GPU_BINARY),
          ElfBinary32(binaryCodeSize, binaryCode, creationFlags),
          innerBinariesNum(0), innerBinaries(nullptr)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    std::vector<uint32_t> choosenSyms;
    std::vector<uint32_t> choosenSymsMetadata;
    
    const bool doKernelInfo = (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0;
    
    for (uint32_t i = 0; i < symbolsNum; i++)
    {
        const char* symName = getSymbolName(i);
        size_t len = strlen(symName);
        if (len < 16 || ::strncmp(symName, "__OpenCL_", 9) != 0)
            continue;
        
        if (::strcmp(symName+len-7, "_kernel") == 0) // if kernel
            choosenSyms.push_back(i);
        if (doKernelInfo && len >= 18 &&
            ::strcmp(symName+len-9, "_metadata") == 0) // if metadata
            choosenSymsMetadata.push_back(i);
    }
    innerBinariesNum = choosenSyms.size();
    
    try
    {
    if (textIndex != SHN_UNDEF)
    {   /* if have ".text" */
        const Elf32_Shdr& textHdr = getSectionHeader(textIndex);
        char* textContent = binaryCode + textHdr.sh_offset;
        
        /* create table of innerBinaries */
        innerBinaries = new AmdInnerGPUBinary32[innerBinariesNum];
        size_t ki = 0;
        for (auto it: choosenSyms)
        {
            const char* symName = getSymbolName(it);
            size_t len = strlen(symName);
            const Elf32_Sym& sym = getSymbol(it);
            
            if (sym.st_value > textHdr.sh_size)
                throw Exception("Inner binary offset out of range!");
            if (sym.st_value + sym.st_size > textHdr.sh_size)
                throw Exception("Inner binary offset+size out of range!");
            
            innerBinaries[ki++] = AmdInnerGPUBinary32(std::string(symName+9, len-16),
                sym.st_size, textContent+sym.st_value, 
                (creationFlags >> AMDBIN_INNER_SHIFT) & ELF_CREATE_ALL);
        }
    }
    
    if ((creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0)
        for (size_t i = 0; i < innerBinariesNum; i++)
            innerBinaryMap.insert(std::make_pair(innerBinaries[i].getKernelName(), i));
    
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        initKernelInfos(creationFlags, choosenSymsMetadata);
    }
    catch(...)
    {   /* free arrays */
        delete[] innerBinaries;
        delete[] kernelInfos;
        throw;
    }
}

AmdMainGPUBinary32::~AmdMainGPUBinary32()
{
    delete[] innerBinaries;
}

const AmdInnerGPUBinary32& AmdMainGPUBinary32::getInnerBinary(const char* name) const
{
    InnerBinaryMap::const_iterator it = innerBinaryMap.find(name);
    if (it == innerBinaryMap.end())
        throw Exception("Cant find inner binary");
    return innerBinaries[it->second];
}

/* AmdMainX86Binary32 */

void AmdMainX86Binary32::initKernelInfos(cxuint creationFlags)
{
    kernelInfosNum = innerBinary.getKernelInfos(kernelInfos);
    if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
        for (size_t i = 0; i < kernelInfosNum; i++)
            kernelInfosMap.insert(
                std::make_pair(kernelInfos[i].kernelName, i));
}

AmdMainX86Binary32::AmdMainX86Binary32(size_t binaryCodeSize, char* binaryCode,
       cxuint creationFlags) : AmdMainBinaryBase(AmdMainType::X86_BINARY),
       ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    if (textIndex != SHN_UNDEF)
    {
        const Elf32_Shdr& textHdr = getSectionHeader(textIndex);
        char* textContent = binaryCode + textHdr.sh_offset;
        
        innerBinary = AmdInnerX86Binary32(textHdr.sh_size, textContent,
                (creationFlags >> AMDBIN_INNER_SHIFT) & ELF_CREATE_ALL);
    }
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        initKernelInfos(creationFlags);
}

/* AmdMainX86Binary64 */

void AmdMainX86Binary64::initKernelInfos(cxuint creationFlags)
{
    kernelInfosNum = innerBinary.getKernelInfos(kernelInfos);
    if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
        for (size_t i = 0; i < kernelInfosNum; i++)
            kernelInfosMap.insert(
                std::make_pair(kernelInfos[i].kernelName, i));
}

AmdMainX86Binary64::AmdMainX86Binary64(size_t binaryCodeSize, char* binaryCode,
       cxuint creationFlags) : AmdMainBinaryBase(AmdMainType::X86_64_BINARY),
       ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    if (textIndex != SHN_UNDEF)
    {
        const Elf64_Shdr& textHdr = getSectionHeader(".text");
        char* textContent = binaryCode + textHdr.sh_offset;
        
        innerBinary = AmdInnerX86Binary64(textHdr.sh_size, textContent,
                (creationFlags >> AMDBIN_INNER_SHIFT) & ELF_CREATE_ALL);
    }
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        initKernelInfos(creationFlags);
}

/* create amd binary */

AmdMainBinaryBase* CLRX::createAmdBinaryFromCode(size_t binaryCodeSize, char* binaryCode,
        cxuint creationFlags)
{
    if (*reinterpret_cast<const uint32_t*>(binaryCode) != elfMagicValue)
        throw Exception("This is not ELF binary");
    if (binaryCode[EI_DATA] != ELFDATA2LSB)
        throw Exception("Other than little-endian binaries are not supported!");
    
    if (binaryCode[EI_CLASS] == ELFCLASS32)
    {
        const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(binaryCode);
        if (ehdr->e_machine != ELF_M_X86) //if gpu
            return new AmdMainGPUBinary32(binaryCodeSize, binaryCode, creationFlags);
        return new AmdMainX86Binary32(binaryCodeSize, binaryCode, creationFlags);
    }
    else if (binaryCode[EI_CLASS] == ELFCLASS64)
    {
        const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binaryCode);
        if (ehdr->e_machine != ELF_M_X86)
            throw Exception("This is not x86-64 binary");
        return new AmdMainX86Binary64(binaryCodeSize, binaryCode, creationFlags);
    }
    else // fatal error
        throw Exception("Unsupported ELF class");
}
