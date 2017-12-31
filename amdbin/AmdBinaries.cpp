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

#include <CLRX/Config.h>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstdint>
#include <map>
#include <mutex>
#include <utility>
#include <vector>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/utils/GPUId.h>

/* INFO: in this file is used ULEV function for conversion
 * from LittleEndian and unaligned access to other memory access policy and endianness
 * Please use this function whenever you want to get or set word in ELF binary,
 * because ELF binaries can be unaligned in memory (as inner binaries).
 */

using namespace CLRX;

// X86 argument's type table
static const KernelArgType x86ArgTypeTable[] =
{
    KernelArgType::VOID,
    KernelArgType::CHAR,
    KernelArgType::SHORT,
    KernelArgType::INT,
    KernelArgType::LONG,
    KernelArgType::FLOAT,
    KernelArgType::DOUBLE,
    KernelArgType::POINTER,
    KernelArgType::CHAR2,
    KernelArgType::CHAR3,
    KernelArgType::CHAR4,
    KernelArgType::CHAR8,
    KernelArgType::CHAR16,
    KernelArgType::SHORT2,
    KernelArgType::SHORT3,
    KernelArgType::SHORT4,
    KernelArgType::SHORT8,
    KernelArgType::SHORT16,
    KernelArgType::INT2,
    KernelArgType::INT3,
    KernelArgType::INT4,
    KernelArgType::INT8,
    KernelArgType::INT16,
    KernelArgType::LONG2,
    KernelArgType::LONG3,
    KernelArgType::LONG4,
    KernelArgType::LONG8,
    KernelArgType::LONG16,
    KernelArgType::FLOAT2,
    KernelArgType::FLOAT3,
    KernelArgType::FLOAT4,
    KernelArgType::FLOAT8,
    KernelArgType::FLOAT16,
    KernelArgType::DOUBLE2,
    KernelArgType::DOUBLE3,
    KernelArgType::DOUBLE4,
    KernelArgType::DOUBLE8,
    KernelArgType::DOUBLE16,
    KernelArgType::SAMPLER
};

// GPU argument's type table
static const KernelArgType gpuArgTypeTable[] =
{
    KernelArgType::UCHAR,
    KernelArgType::UCHAR2,
    KernelArgType::UCHAR3,
    KernelArgType::UCHAR4,
    KernelArgType::UCHAR8,
    KernelArgType::UCHAR16,
    KernelArgType::CHAR,
    KernelArgType::CHAR2,
    KernelArgType::CHAR3,
    KernelArgType::CHAR4,
    KernelArgType::CHAR8,
    KernelArgType::CHAR16,
    KernelArgType::USHORT,
    KernelArgType::USHORT2,
    KernelArgType::USHORT3,
    KernelArgType::USHORT4,
    KernelArgType::USHORT8,
    KernelArgType::USHORT16,
    KernelArgType::SHORT,
    KernelArgType::SHORT2,
    KernelArgType::SHORT3,
    KernelArgType::SHORT4,
    KernelArgType::SHORT8,
    KernelArgType::SHORT16,
    KernelArgType::UINT,
    KernelArgType::UINT2,
    KernelArgType::UINT3,
    KernelArgType::UINT4,
    KernelArgType::UINT8,
    KernelArgType::UINT16,
    KernelArgType::INT,
    KernelArgType::INT2,
    KernelArgType::INT3,
    KernelArgType::INT4,
    KernelArgType::INT8,
    KernelArgType::INT16,
    KernelArgType::ULONG,
    KernelArgType::ULONG2,
    KernelArgType::ULONG3,
    KernelArgType::ULONG4,
    KernelArgType::ULONG8,
    KernelArgType::ULONG16,
    KernelArgType::LONG,
    KernelArgType::LONG2,
    KernelArgType::LONG3,
    KernelArgType::LONG4,
    KernelArgType::LONG8,
    KernelArgType::LONG16,
    KernelArgType::FLOAT,
    KernelArgType::FLOAT2,
    KernelArgType::FLOAT3,
    KernelArgType::FLOAT4,
    KernelArgType::FLOAT8,
    KernelArgType::FLOAT16,
    KernelArgType::DOUBLE,
    KernelArgType::DOUBLE2,
    KernelArgType::DOUBLE3,
    KernelArgType::DOUBLE4,
    KernelArgType::DOUBLE8,
    KernelArgType::DOUBLE16
};

static const uint32_t elfMagicValue = 0x464c457fU;

/* determine unfinished strings region in string table for checking further consistency */
static size_t unfinishedRegionOfStringTable(const cxbyte* table, size_t size)
{
    if (size == 0) // if zero
        return 0;
    size_t k;
    for (k = size-1; k>0 && table[k]!=0; k--);
    
    return (table[k]==0)?k+1:k;
}

/* AMD inner GPU binary */

AmdInnerGPUBinary32::AmdInnerGPUBinary32(const CString& _kernelName,
         size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags)
        : ElfBinary32(binaryCodeSize, binaryCode, creationFlags), kernelName(_kernelName),
          encodingEntriesNum(0), encodingEntries(nullptr)
{
    if (getProgramHeadersNum() >= 1)
    {
        const Elf32_Phdr& ephdr = getProgramHeader(0);
        const size_t encTableOffset = ULEV(ephdr.p_offset);
        const size_t encTableSize = ULEV(ephdr.p_filesz);
        if (ULEV(ephdr.p_type) != 0x70000002)
            throw BinException("Missing encodings table");
        if (encTableSize%sizeof(CALEncodingEntry) != 0)
            throw BinException("Wrong size of encodings table");
        if (usumGt(encTableOffset, encTableSize, binaryCodeSize))
            throw BinException("Offset+Size of encodings table out of range");
        
        encodingEntriesNum = encTableSize/sizeof(CALEncodingEntry);
        encodingEntries = reinterpret_cast<CALEncodingEntry*>(
                    binaryCode + encTableOffset);
        /* verify encoding entries */
        for (cxuint i = 0; i < encodingEntriesNum; i++)
        {
            const CALEncodingEntry& entry = encodingEntries[i];
            const size_t offset = ULEV(entry.offset);
            const size_t size = ULEV(entry.size);
            if (offset >= binaryCodeSize)
                throw BinException("Encoding entry offset out of range");
            if (usumGt(offset, size, binaryCodeSize))
                throw BinException("Encoding entry offset+size out of range");
        }
        
        cxuint encodingIndex = 0;
        if ((creationFlags & AMDBIN_CREATE_CALNOTES) != 0)
            calNotesTable.resize(encodingEntriesNum);
        
        for (uint32_t i = 1; i < getProgramHeadersNum(); i++)
        {
            const Elf32_Phdr& phdr = getProgramHeader(i);
            const CALEncodingEntry& encEntry = encodingEntries[encodingIndex];
            const size_t encEntryOffset = ULEV(encEntry.offset);
            const size_t encEntrySize = ULEV(encEntry.size);
            const size_t offset = ULEV(phdr.p_offset);
            const size_t size = ULEV(phdr.p_filesz);
            // check offset and ranges of program header
            if (offset < encEntryOffset)
                throw BinException("Kernel program offset out of encoding");
            if (usumGt(offset, size, encEntryOffset+encEntrySize))
                throw BinException("Kernel program offset+size out of encoding");
            
            if ((creationFlags & AMDBIN_CREATE_CALNOTES) != 0 &&
                        ULEV(phdr.p_type) == PT_NOTE)
            {
                Array<CALNote>& calNotes = calNotesTable[encodingIndex];
                uint32_t calNotesCount = 0;
                // verifying ATI CAL notes
                for (uint32_t pos = 0; pos < size; calNotesCount++)
                {
                    const CALNoteHeader& nhdr =
                        *reinterpret_cast<const CALNoteHeader*>(binaryCode+offset+pos);
                    if (ULEV(nhdr.nameSize) != 8)
                        throw BinException("Wrong name size in Note header!");
                    if (::memcmp(nhdr.name, "ATI CAL", 8) != 0)
                        throw BinException("Wrong name in Note header!");
                    if (usumGt(uint32_t(pos + sizeof(CALNoteHeader)),
                                ULEV(nhdr.descSize), size))
                        throw BinException("CAL Note desc size out of range");
                    pos += sizeof(CALNoteHeader) + ULEV(nhdr.descSize);
                }
                
                // put CAL Notes
                calNotes.resize(calNotesCount);
                uint32_t count = 0;
                for (uint32_t pos = 0; pos < size; count++)
                {
                    CALNoteHeader* nhdr =
                        reinterpret_cast<CALNoteHeader*>(binaryCode+offset+pos);
                    calNotes[count].header = nhdr;
                    calNotes[count].data = binaryCode + offset + pos +
                            sizeof(CALNoteHeader);
                    pos += sizeof(CALNoteHeader) + ULEV(nhdr->descSize);
                }
            }
            if (offset+size == encEntryOffset+encEntrySize)
            {
                /* next encoding entry */
                encodingIndex++;
                // if program headers table is not exhausted, but no encoding entries
                if (i+1 < getProgramHeadersNum() && encodingIndex >= encodingEntriesNum)
                    throw BinException("ProgramHeaders out of encodings!");
            }
        }
    }
}

struct CLRX_INTERNAL GPUDeviceInnerCodeEntry
{
    uint32_t dMachine;
    GPUDeviceType deviceType;
};

// GPU device code table - code in CALEncodingEntry (machine field)
static const GPUDeviceInnerCodeEntry gpuDeviceInnerCodeTable[19] =
{
    { 0x1a, GPUDeviceType::TAHITI },
    { 0x1b, GPUDeviceType::PITCAIRN },
    { 0x1c, GPUDeviceType::CAPE_VERDE },
    { 0x20, GPUDeviceType::OLAND },
    { 0x21, GPUDeviceType::BONAIRE },
    { 0x22, GPUDeviceType::SPECTRE },
    { 0x23, GPUDeviceType::SPOOKY },
    { 0x24, GPUDeviceType::KALINDI },
    { 0x25, GPUDeviceType::HAINAN },
    { 0x27, GPUDeviceType::HAWAII },
    { 0x29, GPUDeviceType::ICELAND },
    { 0x2a, GPUDeviceType::TONGA },
    { 0x2b, GPUDeviceType::MULLINS },
    { 0x2c, GPUDeviceType::BAFFIN },
    { 0x2d, GPUDeviceType::FIJI },
    { 0x2e, GPUDeviceType::CARRIZO },
    { 0x2f, GPUDeviceType::ELLESMERE },
    { 0x31, GPUDeviceType::DUMMY },
    { 0x32, GPUDeviceType::GFX804 }
};

static const cxuint gpuDeviceInnerCodeTableSize = sizeof(gpuDeviceInnerCodeTable)/
            sizeof(GPUDeviceInnerCodeEntry);

cxuint AmdInnerGPUBinary32::findCALEncodingEntryIndex(GPUDeviceType deviceType) const
{
    cxuint encEntryIndex = 0;
    for (encEntryIndex = 0; encEntryIndex < encodingEntriesNum; encEntryIndex++)
    {
        const CALEncodingEntry& encEntry = encodingEntries[encEntryIndex];
        /* check gpuDeviceType */
        const uint32_t dMachine = ULEV(encEntry.machine);
        // detect GPU device from machine field from CAL encoding entry
        cxuint index = binaryFind(gpuDeviceInnerCodeTable,
                  gpuDeviceInnerCodeTable + gpuDeviceInnerCodeTableSize, { dMachine }, 
                  [] (const GPUDeviceInnerCodeEntry& l, const GPUDeviceInnerCodeEntry& r)
                  { return l.dMachine < r.dMachine; }) - gpuDeviceInnerCodeTable;
        if (gpuDeviceInnerCodeTableSize != index &&
            gpuDeviceInnerCodeTable[index].deviceType == deviceType)
            break; // if found
    }
    if (encEntryIndex == encodingEntriesNum)
        throw BinException("Can't find suitable CALEncodingEntry!");
    return encEntryIndex;
}

template<typename ArgSym>
static size_t skipStructureArgX86(const ArgSym* argDescTable,
            size_t argDescsNum, size_t startPos)
{
    size_t nestedLevel = 0;
    size_t pos = startPos;
    do {
        const ArgSym& argDesc = argDescTable[pos++];
        if (ULEV(argDesc.argType) == 0x28)
            nestedLevel++;
        else if (ULEV(argDesc.argType) == 0)
            nestedLevel--;
    } while (nestedLevel != 0 && pos < argDescsNum);
    if (nestedLevel != 0)
        throw BinException("Unfinished kernel argument structure");
    return pos;
}

/* AMD inner X86 binary */

struct CLRX_INTERNAL AmdInnerX86Types : Elf32Types
{
    typedef X86KernelArgSym KernelArgSym;
    typedef ElfBinary32 ElfBinary;
    static const size_t argDescsNumOffset;
    static const size_t argDescsNumESize;
    static const size_t argDescTableOffset;
};

struct CLRX_INTERNAL AmdInnerX86_64Types : Elf64Types
{
    typedef X86_64KernelArgSym KernelArgSym;
    typedef ElfBinary64 ElfBinary;
    static const size_t argDescsNumOffset;
    static const size_t argDescsNumESize;
    static const size_t argDescTableOffset;
};

/* different argDescsNumOffset and argDescsNumESize used for compute number of
 * entries in kernel argument table in binary */

const size_t AmdInnerX86Types::argDescsNumOffset = 44;
const size_t AmdInnerX86Types::argDescsNumESize = 12;
const size_t AmdInnerX86Types::argDescTableOffset = 32;

const size_t AmdInnerX86_64Types::argDescsNumOffset = 80;
const size_t AmdInnerX86_64Types::argDescsNumESize = 16;
const size_t AmdInnerX86_64Types::argDescTableOffset = 64;

/* generic code for X86 binary. get from metadata, informations about kernel arguments */
template<typename Types>
static size_t getKernelInfosInternal(const typename Types::ElfBinary& elf,
             Array<KernelInfo>& kernelInfos)
{
    if (!elf) return 0;
    
    cxuint rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = elf.getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { return 0; /* no section */ }
    
    const typename Types::Shdr& rodataHdr = elf.getSectionHeader(rodataIndex);
    
    /* get kernel metadata symbols */
    std::vector<typename Types::Size> choosenSyms;
    const size_t dynSymbolsNum = elf.getDynSymbolsNum();
    for (typename Types::Size i = 0; i < dynSymbolsNum; i++)
    {
        const char* symName = elf.getDynSymbolName(i);
        const size_t len = ::strlen(symName);
        if (len < 18 || ::strncmp(symName, "__OpenCL_", 9) != 0 ||
            ::strcmp(symName+len-9, "_metadata") != 0) // not metdata then skip
            continue;
        choosenSyms.push_back(i);
    }
    
    const cxbyte* binaryCode = elf.getBinaryCode();
    const size_t unfinishedRegion = unfinishedRegionOfStringTable(
        binaryCode + ULEV(rodataHdr.sh_offset), ULEV(rodataHdr.sh_size));
    
    size_t unfinishedRegionArgNameSym = SIZE_MAX;
    
    /* foundInStaticSymbols - deployed because ELF binaries for X86/X64 kernels differs
     * between AMD Catalyst driver. Newest version just put symbols to dynamic table! */
    bool foundInStaticSymbols = false;
    std::vector<typename Types::Size> argTypeNamesSyms;
    if (choosenSyms.empty())
    {
        /* if not found in symbol: TODO: Fix for driver 1573.4 and newer */
        const size_t symbolsNum = elf.getSymbolsNum();
        for (typename Types::Size i = 0; i < symbolsNum; i++)
        {
            const char* symName = elf.getSymbolName(i);
            const size_t len = ::strlen(symName);
            if (::strncmp(symName, ".str", 4) == 0)
            {
                /* add arg/type name symbol to our table */
                size_t index = 0;
                if (symName[4] != 0)
                {
                    const char* outend;
                    index = cstrtovCStyle<size_t>(symName+4, nullptr, outend);
                    if (*outend != 0)
                        throw BinException("Garbages in .str symbol name!");
                }
                if (argTypeNamesSyms.size() <= index)
                    argTypeNamesSyms.resize(index+1);
                
                const typename Types::Sym& sym = elf.getSymbol(i);
                if (ULEV(sym.st_shndx) >= elf.getSectionHeadersNum())
                    throw BinException("ArgTypeNameSymStr section index out of range");
                const typename Types::Shdr& secHdr = 
                        elf.getSectionHeader(ULEV(sym.st_shndx)); // from symbol
                
                if (ULEV(sym.st_value) >= ULEV(secHdr.sh_size))
                    throw BinException("ArgTypeNameSymStr value out of range");
                if (usumGt(ULEV(sym.st_value), ULEV(sym.st_size), ULEV(secHdr.sh_size)))
                    throw BinException("ArgTypeNameSymStr value+size out of range");
                
                argTypeNamesSyms[index] = ULEV(secHdr.sh_offset) + ULEV(sym.st_value);
                if (ULEV(sym.st_shndx) == rodataIndex)
                {
                    if (ULEV(sym.st_value) >= unfinishedRegion)
                        throw BinException("Arg name/type is unfinished!");
                }
                else // is not roData
                {
                    if (unfinishedRegionArgNameSym == SIZE_MAX) // init unfished  strings
                        unfinishedRegionArgNameSym = unfinishedRegionOfStringTable(
                                binaryCode + ULEV(secHdr.sh_offset), ULEV(secHdr.sh_size));
                    
                    if (ULEV(sym.st_value) >= unfinishedRegionArgNameSym)
                        throw BinException("Arg name/type is unfinished!");
                }
                continue;
            }
            if (len < 18 || ::strncmp(symName, "__OpenCL_", 9) != 0 ||
                ::strcmp(symName+len-9, "_metadata") != 0) // not metdata then skip
                continue;
            choosenSyms.push_back(i);
        }
        foundInStaticSymbols = true;
    }
    
    kernelInfos.resize(choosenSyms.size());
    
    size_t argNameTypeNameIdx = 0;
    size_t ki = 0;
    for (auto i: choosenSyms)
    {
        const typename Types::Sym& sym = (!foundInStaticSymbols)?elf.getDynSymbol(i):
                elf.getSymbol(i);
        if (ULEV(sym.st_shndx) >= elf.getSectionHeadersNum())
            throw BinException("Metadata section index out of range");
        
        const typename Types::Shdr& dataHdr = 
                elf.getSectionHeader(ULEV(sym.st_shndx)); // from symbol
        const size_t fileOffset = ULEV(sym.st_value) -
                ((!foundInStaticSymbols)?ULEV(dataHdr.sh_addr):0) +
                ULEV(dataHdr.sh_offset);
        
        if (fileOffset < ULEV(dataHdr.sh_offset) ||
            fileOffset >= ULEV(dataHdr.sh_offset) + ULEV(dataHdr.sh_size))
            throw BinException("File offset of kernelMetadata out of range!");
        const cxbyte* data = binaryCode + fileOffset;
        
        /* parse number of args */
        typename Types::Size argDescsNum =
                (ULEV(*reinterpret_cast<const uint32_t*>(data)) -
                Types::argDescsNumOffset)/Types::argDescsNumESize;
        KernelInfo& kernelInfo = kernelInfos[ki++];
        
        // get kernel name
        const char* symName = (!foundInStaticSymbols)?elf.getDynSymbolName(i):
                elf.getSymbolName(i);
        const size_t len = ::strlen(symName);
        kernelInfo.kernelName.assign(symName+9, len-18);
        kernelInfo.argInfos.resize(argDescsNum>>1);
        
        /* get argument info */
        const typename Types::KernelArgSym* argDescTable =
                reinterpret_cast<const typename Types::KernelArgSym*>(
                    data + Types::argDescTableOffset);
        
        cxuint realArgsNum = 0;
        for (size_t ai = 0; ai < argDescsNum; ai++)
        {
            const typename Types::KernelArgSym& argNameSym = argDescTable[ai];
            cxuint argType = ULEV(argNameSym.argType);
            
            if (argType == 0x28) // skip structure
                ai = skipStructureArgX86<typename Types::KernelArgSym>(
                    argDescTable, argDescsNum, ai);
            else // if not structure
                ai++;
            const typename Types::KernelArgSym& argTypeSym = argDescTable[ai];
            
            if (realArgsNum >= kernelInfo.argInfos.size())
                throw BinException("Kernel ArgInfo index out of range");
            AmdKernelArg& karg = kernelInfo.argInfos[realArgsNum++];
            const size_t rodataHdrOffset = ULEV(rodataHdr.sh_offset);
            const size_t rodataHdrSize = ULEV(rodataHdr.sh_size);
            
            // getting kernel argument name
            // next stuff to handle different place of symbols
            if (!foundInStaticSymbols)
            {
                if (argNameSym.getNameOffset() < rodataHdrOffset ||
                    argNameSym.getNameOffset() >= rodataHdrOffset+rodataHdrSize)
                    throw BinException("Kernel arg name offset out of range!");
                
                if (argNameSym.getNameOffset()-rodataHdrOffset >= unfinishedRegion)
                    throw BinException("Arg name is unfinished!");
                
                karg.argName = reinterpret_cast<const char*>(
                    binaryCode + argNameSym.getNameOffset());
            }
            else /* otherwise get from our table */
            {
                if (argNameTypeNameIdx >= argTypeNamesSyms.size())
                    throw BinException("ArgName sym index out of range");
                const typename Types::Size value = argTypeNamesSyms[argNameTypeNameIdx++];
                if (value >= elf.getSize())
                    throw BinException("ArgName sym offset out of range");
                
                karg.argName = reinterpret_cast<const char*>(binaryCode + value);
            }
            
            // getting kernel argument type
            if (!foundInStaticSymbols)
            {
                if (argTypeSym.getNameOffset() < rodataHdrOffset ||
                    argTypeSym.getNameOffset() >= rodataHdrOffset+rodataHdrSize)
                    throw BinException("Kernel arg type offset out of range!");
                
                if (argTypeSym.getNameOffset()-rodataHdrOffset >= unfinishedRegion)
                    throw BinException("Type name is unfinished!");
                
                karg.typeName = reinterpret_cast<const char*>(
                    binaryCode + argTypeSym.getNameOffset());
            }
            else /* otherwise get from our table */
            {
                if (argNameTypeNameIdx >= argTypeNamesSyms.size())
                    throw BinException("ArgType sym index out of range");
                const typename Types::Size value = argTypeNamesSyms[argNameTypeNameIdx++];
                if (value >= elf.getSize())
                    throw BinException("ArgType sym offset out of range");
                
                karg.typeName = reinterpret_cast<const char*>(binaryCode + value);
            }
            
            if (argType != 0x28)
            {
                if (argType > 0x26)
                    throw BinException("Unknown kernel arg type");
                karg.argType = x86ArgTypeTable[argType];
                if (karg.argType == KernelArgType::POINTER &&
                    (ULEV(argNameSym.ptrAccess) &
                            (KARG_PTR_READ_ONLY|KARG_PTR_WRITE_ONLY)) != 0)
                    karg.argType = KernelArgType::IMAGE;
            }
            else // if structure
                karg.argType = KernelArgType::STRUCTURE;
            
            karg.ptrSpace = static_cast<KernelPtrSpace>(ULEV(argNameSym.ptrType));
            karg.ptrAccess = ULEV(argNameSym.ptrAccess);
        }
        kernelInfo.argInfos.resize(realArgsNum);
    }
    return choosenSyms.size();
}

AmdInnerX86Binary32::AmdInnerX86Binary32(
            size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags) :
            ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
{ }

void AmdInnerX86Binary32::getKernelInfos(Array<KernelInfo>& kernelInfos) const
{
    getKernelInfosInternal<AmdInnerX86Types>(*this, kernelInfos);
}

AmdInnerX86Binary64::AmdInnerX86Binary64(
            size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags) :
            ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{ }

void AmdInnerX86Binary64::getKernelInfos(Array<KernelInfo>& kernelInfos) const
{
    getKernelInfosInternal<AmdInnerX86_64Types>(*this, kernelInfos);
}

/* AmdMainBinaryBase */

AmdMainBinaryBase::AmdMainBinaryBase(AmdMainType _type) : type(_type)
{ }

AmdMainBinaryBase::~AmdMainBinaryBase()
{ }

const KernelInfo& AmdMainBinaryBase::getKernelInfo(const char* name) const
{
    KernelInfoMap::const_iterator it = binaryMapFind(
        kernelInfosMap.begin(), kernelInfosMap.end(), name);
    if (it == kernelInfosMap.end())
        throw BinException("Can't find kernel name");
    return kernelInfos[it->second];
}

static const cxuint vectorIdTable[17] =
{ UINT_MAX, 0, 1, 2, 3, UINT_MAX, UINT_MAX, UINT_MAX, 4,
  UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, 5 };

static KernelArgType determineKernelArgType(const char* typeString,
           cxuint vectorSize, LineNo lineNo)
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
        {   // 8-bit type
            if (typeString[2] != ':')
                throw ParseException(lineNo, "Can't parse type");
            outType = gpuArgTypeTable[indexBase+vectorId];
        }
        else
        {
            // 16,32,64 bit types
            if (typeString[1] == '1' && typeString[2] == '6')
                outType = gpuArgTypeTable[indexBase+2*6+vectorId];
            else if (typeString[1] == '3' && typeString[2] == '2')
                outType = gpuArgTypeTable[indexBase+4*6+vectorId];
            else if (typeString[1] == '6' && typeString[2] == '4')
                outType = gpuArgTypeTable[indexBase+6*6+vectorId];
            else // if not determined
                throw ParseException(lineNo, "Can't parse type");
            if (typeString[3] != ':')
                throw ParseException(lineNo, "Can't parse type");
        }
    }
    else
        throw ParseException(lineNo, "Can't parse type");
    
    return outType;
}

static inline CString stringFromCStringDelim(const char* c1, size_t maxSize, char delim)
{
    size_t i = 0;
    for (i = 0; i < maxSize && c1[i] != delim; i++);
    return CString(c1, i);
}

/* metadata string that stored in rodata section in main GPU binary holds needed kernel
 * argument info (arg type and arg name). this function just retrieve that data */
static void parseAmdGpuKernelMetadata(const char* symName, size_t metadataSize,
          const char* kernelDesc, KernelInfo& kernelInfo)
{
    // internal structure to hold kernel arguments info in map
    struct InitKernelArgMapEntry
    {
        uint32_t index;
        KernelArgType argType;
        KernelArgType origArgType;
        KernelPtrSpace ptrSpace;
        uint32_t ptrAccess;
        const char* nameStr;
        
        InitKernelArgMapEntry() : index(0), argType(KernelArgType::VOID),
            origArgType(KernelArgType::VOID),
            ptrSpace(KernelPtrSpace::NONE), ptrAccess(0), nameStr(nullptr)
        { }
    };
    typedef std::map<CString, InitKernelArgMapEntry> InitKernelArgMap;
    
    /* parse kernel description */
    LineNo lineNo = 1;
    uint32_t argIndex = 0;
    
    InitKernelArgMap initKernelArgs;
    
    const char* kptr = kernelDesc;
    const char* kend = kernelDesc + metadataSize;
    // first phase (value/pointer/image/sampler)
    while (kptr < kend)
    {
        if (*kptr != ';')
            throw ParseException(lineNo, "This is not KernelDesc line");
        kptr++;
        if (kptr >= kend)
            throw ParseException(lineNo, "This is not KernelDesc line");
        
        const char* tokPtr = kptr;
        while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
        if (tokPtr >= kend)
            throw ParseException(lineNo, "Is not KernelDesc line");
        
        if (::strncmp(kptr, "value", tokPtr-kptr) == 0)
        {
            // value
            if (*tokPtr == '\n')
                throw ParseException(lineNo, "This is not value line");
            
            kptr = ++tokPtr;
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend || *tokPtr =='\n')
                throw ParseException(lineNo, "No separator after name");
            
            // extract arg name
            InitKernelArgMap::iterator argIt;
            {
                InitKernelArgMapEntry entry;
                entry.index = argIndex++;
                entry.nameStr = kptr;
                std::pair<InitKernelArgMap::iterator, bool> result = 
                    initKernelArgs.insert(std::make_pair(CString(kptr, tokPtr), entry));
                if (!result.second)
                    throw ParseException(lineNo, "Argument has been duplicated");
                argIt = result.first;
            }
            ///
            kptr = ++tokPtr;
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend || *tokPtr =='\n')
                throw ParseException(lineNo, "No separator after type");
            // get arg type
            const char* argType = kptr;
            ///
            if (tokPtr-kptr != 6 || ::strncmp(argType, "struct", 6)!=0)
            {
                // regular type
                kptr = ++tokPtr;
                while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
                if (tokPtr >= kend || *tokPtr =='\n')
                    throw ParseException(lineNo, "No separator after vector size");
                // get vector size
                {
                    const char* outEnd;
                    cxuint vectorSize = 0;
                    try
                    { vectorSize = cstrtoui(kptr, tokPtr, outEnd); }
                    catch(const ParseException& ex)
                    { throw ParseException(lineNo, ex.what()); }
                    if (outEnd != tokPtr)
                        throw ParseException(lineNo, "Garbages after integer");
                    argIt->second.argType = determineKernelArgType(argType,
                           vectorSize, lineNo);
                }
                kptr = tokPtr;
            }
            else // if structure
                argIt->second.argType = KernelArgType::STRUCTURE;
        }
        else if (::strncmp(kptr, "pointer", tokPtr-kptr) == 0)
        {
            // pointer
            if (*tokPtr == '\n')
                throw ParseException(lineNo, "This is not pointer line");
            
            kptr = ++tokPtr;
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend || *tokPtr =='\n')
                throw ParseException(lineNo, "No separator after name");
            
            // extract arg name
            InitKernelArgMap::iterator argIt;
            {
                InitKernelArgMapEntry entry;
                entry.index = argIndex++;
                entry.nameStr = kptr;
                entry.argType = KernelArgType::POINTER;
                std::pair<InitKernelArgMap::iterator, bool> result = 
                    initKernelArgs.insert(std::make_pair(CString(kptr, tokPtr), entry));
                if (!result.second)
                    throw ParseException(lineNo, "Argument has been duplicated");
                argIt = result.first;
            }
            ///
            ++tokPtr;
            for (cxuint k = 0; k < 4; k++) // // skip four fields
            {
                while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
                if (tokPtr >= kend || *tokPtr =='\n')
                    throw ParseException(lineNo, "No separator after field");
                tokPtr++;
            }
            kptr = tokPtr;
            // get pointer type (global/local/constant)
            if (kptr+4 <= kend && *kptr == 'u' && kptr[1] == 'a' && kptr[2] == 'v' &&
                kptr[3] == ':')
            { argIt->second.ptrSpace = KernelPtrSpace::GLOBAL; kptr += 4; }
            else if (kptr+3 <= kend && *kptr == 'h' && kptr[1] == 'c' && kptr[2] == ':')
            { argIt->second.ptrSpace = KernelPtrSpace::CONSTANT; kptr += 3; }
            else if (kptr+3 <= kend && *kptr == 'h' && kptr[1] == 'l' && kptr[2] == ':')
            { argIt->second.ptrSpace = KernelPtrSpace::LOCAL; kptr += 3; }
            else if (kptr+2 <= kend && *kptr == 'c' && kptr[1] == ':')
            { argIt->second.ptrSpace = KernelPtrSpace::CONSTANT; kptr += 2; }
            else //if not match
                throw ParseException(lineNo, "Unknown pointer type");
            
            tokPtr = kptr;
            for (cxuint k = 0; k < 2; k++) // // skip two fields
            {
                while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
                if (tokPtr >= kend || *tokPtr =='\n')
                    throw ParseException(lineNo, "No separator after field");
                tokPtr++;
            }
            // RO/RW
            kptr = tokPtr;
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend || *tokPtr =='\n')
                throw ParseException(lineNo, "No separator after access qualifier");
            if (kptr+3 <= kend && *kptr=='R' && kptr[1]=='O' && kptr[2]==':' &&
                argIt->second.ptrSpace==KernelPtrSpace::GLOBAL)
                argIt->second.ptrAccess |= KARG_PTR_CONST;
            else if (kptr+3 <= kend && *kptr=='R' && kptr[1]=='W' && kptr[2]==':')
                argIt->second.ptrAccess |= KARG_PTR_NORMAL;
            /* volatile specifier */
            kptr = ++tokPtr;
            if (kptr+2 <= kend && kptr[1] == ':' && (*kptr == '0' || *kptr == '1'))
            {
                if (*kptr == '1')
                    argIt->second.ptrAccess |= KARG_PTR_VOLATILE;
                kptr += 2;
            }
            else // error
                throw ParseException("Unknown value or end at volatile field");
            
            /* restrict specifier */
            if (kptr+2 <= kend && kptr[1] == '\n' && (*kptr == '0' || *kptr == '1'))
            {
                if (*kptr == '1')
                    argIt->second.ptrAccess |= KARG_PTR_RESTRICT;
                kptr++;
            }
            else // error
                throw ParseException("Unknown value or end at restrict field");
        }
        else if (::strncmp(kptr, "image", tokPtr-kptr) == 0)
        {
            // parse image arg
            if (*tokPtr == '\n')
                throw ParseException(lineNo, "This is not image line");
            
            kptr = ++tokPtr;
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend || *tokPtr == '\n')
                throw ParseException(lineNo, "No separator after name");
            
            // extract arg name
            InitKernelArgMap::iterator argIt;
            InitKernelArgMapEntry entry;
            entry.index = argIndex++;
            entry.nameStr = kptr;
            entry.ptrSpace = KernelPtrSpace::GLOBAL;
            CString name(kptr, tokPtr);
            
            kptr = ++tokPtr;
            // quick image type parsing (1D,1DA,1DB,2D,2DA,3D)
            if (kptr+3 < kend)
            {
                if (kptr[1] != 'D')
                    throw ParseException("Unknown image type");
                if (*kptr == '1')
                {
                    // one dimensional image
                    if (kptr[2] == 'A')
                    { entry.argType = KernelArgType::IMAGE1D_ARRAY; kptr += 3; }
                    else if (kptr[2] == 'B')
                    { entry.argType = KernelArgType::IMAGE1D_BUFFER; kptr += 3; }
                    else
                    { entry.argType = KernelArgType::IMAGE1D; kptr += 2; }
                }
                else if (*kptr == '2')
                {
                    if (kptr[2] == 'A')
                    { entry.argType = KernelArgType::IMAGE2D_ARRAY; kptr += 3; }
                    else
                    { entry.argType = KernelArgType::IMAGE2D; kptr += 2; }
                }
                else if (*kptr == '3')
                { entry.argType = KernelArgType::IMAGE3D; kptr += 2; }
                else
                    throw ParseException("Unknown image type");
                if (kptr >= kend || *kptr != ':')
                    throw ParseException("No separator after field");
            }
            else
                throw ParseException(lineNo, "No separator after field");
            
            std::pair<InitKernelArgMap::iterator, bool> result = 
                initKernelArgs.insert(std::make_pair(name, entry));
            if (!result.second)
                throw ParseException(lineNo, "Argument has been duplicated");
            argIt = result.first;
            
            ++kptr;
            if (kptr+3 > kend || kptr[2] != ':')
                throw ParseException(lineNo, "Can't parse image access qualifier");
            // handle img access qualifier: RO,WO,RW */
            if (*kptr == 'R' && kptr[1] == 'O')
                argIt->second.ptrAccess |= KARG_PTR_READ_ONLY;
            else if (*kptr == 'W' && kptr[1] == 'O')
                argIt->second.ptrAccess |= KARG_PTR_WRITE_ONLY;
            else if (*kptr == 'R' && kptr[1] == 'W') //???
                argIt->second.ptrAccess |= KARG_PTR_READ_WRITE;
            else
                throw ParseException(lineNo, "Can't parse image access qualifier");
            kptr += 3;
        }
        else if (::strncmp(kptr, "sampler", tokPtr-kptr) == 0)
        {
            // sampler (set up some argument as sampler
            if (*tokPtr == '\n')
                throw ParseException(lineNo, "This is not sampler line");
            
            kptr = ++tokPtr;
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend)
                throw ParseException(lineNo, "No separator after name");
            
            CString argName(kptr, tokPtr);
            InitKernelArgMap::iterator argIt = initKernelArgs.find(argName);
            if (argIt != initKernelArgs.end())
            {
                argIt->second.origArgType = argIt->second.argType;
                argIt->second.argType = KernelArgType::SAMPLER;
            }
            
            kptr = tokPtr;
        }
        else if (::strncmp(kptr, "constarg", tokPtr-kptr) == 0)
        {
            if (*tokPtr == '\n')
                throw ParseException(lineNo, "This is not constarg line");
            
            kptr = ++tokPtr;
            // skip number
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend || *tokPtr == '\n')
                throw ParseException(lineNo, "No separator after field");
            
            kptr = ++tokPtr;
            while (tokPtr < kend && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend)
                throw ParseException(lineNo, "End of data");
            
            /// put constant
            const CString thisName(kptr, tokPtr);
            InitKernelArgMap::iterator argIt = initKernelArgs.find(thisName);
            if (argIt == initKernelArgs.end())
                throw ParseException(lineNo, "Can't find constant argument");
            // set up const access type
            argIt->second.ptrAccess |= KARG_PTR_CONST;
            kptr = tokPtr;
        }
        else if (::strncmp(kptr, "counter", tokPtr-kptr) == 0)
        {
            if (*tokPtr == '\n')
                throw ParseException(lineNo, "This is not constarg line");
            
            kptr = ++tokPtr;
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend)
                throw ParseException(lineNo, "No separator after name");
            
            CString argName(kptr, tokPtr);
            // extract arg name
            InitKernelArgMap::iterator argIt;
            {
                InitKernelArgMapEntry entry;
                entry.index = argIndex++;
                entry.nameStr = kptr;
                entry.argType = KernelArgType::COUNTER32;
                std::pair<InitKernelArgMap::iterator, bool> result = 
                    initKernelArgs.insert(std::make_pair(CString(kptr, tokPtr), entry));
                if (!result.second)
                    throw ParseException(lineNo, "Argument has been duplicated");
                argIt = result.first;
            }
            
            kptr = tokPtr;
        }
        else if (::strncmp(kptr, "reflection", tokPtr-kptr) == 0)
        {
            if (*tokPtr == '\n')
                throw ParseException(lineNo, "This is not reflection line");
            break;
        }
        
        while (kptr < kend && *kptr != '\n') kptr++;
        lineNo++;
        kptr++; // skip newline
    }
    
    kernelInfo.kernelName.assign(symName+9, ::strlen(symName)-18);
    kernelInfo.argInfos.resize(argIndex);
    
    for (const auto& e: initKernelArgs)
    {
        /* initialize kernel arguments before set argument type from reflections */
        AmdKernelArg& karg = kernelInfo.argInfos[e.second.index];
        karg.argType = e.second.argType;
        karg.ptrSpace = e.second.ptrSpace;
        karg.ptrAccess = e.second.ptrAccess;
        karg.argName = stringFromCStringDelim(e.second.nameStr, kend-e.second.nameStr, ':');
    }
    
    /* reflections holds argument type names, we just retrieve from arg type names! */
    if (argIndex != 0)
    {
        /* check whether not end */
        if (kptr >= kend)
            throw ParseException(lineNo, "Unexpected end of data");
        
        // reflections
        kptr--; // skip ';'
        while (kptr < kend)
        {
            if (*kptr != ';')
                throw ParseException(lineNo, "Is not KernelDesc line");
            kptr++;
            if (kptr >= kend)
                throw ParseException(lineNo, "Is not KernelDesc line");
            // finish at end of string or if no ';reflection'
            if (kptr+11 > kend || ::strncmp(kptr, "reflection:", 11) != 0)
                break; // end!!!
            kptr += 11;
            
            const char* tokPtr = kptr;
            while (tokPtr < kend && *tokPtr != ':' && *tokPtr != '\n') tokPtr++;
            if (tokPtr >= kend || *tokPtr == '\n')
                throw ParseException(lineNo, "Is not KernelDesc line");
            
            const char* outEnd;
            cxuint argIndex = 0;
            try
            { argIndex = cstrtoui(kptr, tokPtr, outEnd); }
            catch(const ParseException& ex)
            { throw ParseException(lineNo, ex.what()); }
            
            if (outEnd != tokPtr)
                throw ParseException(lineNo, "Garbages after integer");
            if (argIndex >= kernelInfo.argInfos.size())
                throw ParseException(lineNo, "Argument index out of range");
            kptr = tokPtr+1;
            
            AmdKernelArg& argInfo = kernelInfo.argInfos[argIndex];
            argInfo.typeName = stringFromCStringDelim(kptr, kend-kptr, '\n');
            
            if (argInfo.argName.compare(0, 8, "unknown_") == 0 &&
                argInfo.typeName != "sampler_t" &&
                argInfo.argType == KernelArgType::SAMPLER)
            {
                InitKernelArgMap::const_iterator argIt =
                            initKernelArgs.find(argInfo.argName);
                if (argIt != initKernelArgs.end() &&
                    argIt->second.origArgType != KernelArgType::VOID)
                    /* revert sampler type and restore original arg type */
                    argInfo.argType = argIt->second.origArgType;
            }    
            
            while (kptr < kend && *kptr != '\n') kptr++;
            lineNo++;
            kptr++;
        }
    }
}

struct CLRX_INTERNAL AmdGPU32Types : Elf32Types
{
    typedef ElfBinary32 ElfBinary;
};

struct CLRX_INTERNAL AmdGPU64Types: Elf64Types
{
    typedef ElfBinary64 ElfBinary;
};

AmdMainGPUBinaryBase::AmdMainGPUBinaryBase(AmdMainType type)
        : AmdMainBinaryBase(type), metadatas(nullptr), globalDataSize(0), globalData(0)
{ }

template<typename Types>
void AmdMainGPUBinaryBase::initMainGPUBinary(typename Types::ElfBinary& mainElf)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = mainElf.getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    std::vector<size_t> choosenSyms;
    std::vector<size_t> choosenSymsMetadata;
    std::vector<size_t> choosenSymsHeaders;
    
    const cxuint creationFlags = mainElf.getCreationFlags();
    
    const bool doKernelHeaders = (creationFlags & AMDBIN_CREATE_KERNELHEADERS) != 0;
    const bool doKernelInfo = (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0;
    const bool doInfoStrings = (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0;
    size_t compileOptionsEnd = 0;
    uint16_t compileOptionShIndex = SHN_UNDEF;
    
    for (size_t i = 0; i < mainElf.getSymbolsNum(); i++)
    {
        const char* symName = mainElf.getSymbolName(i);
        size_t len = ::strlen(symName);
        if (len < 16 || ::strncmp(symName, "__OpenCL_", 9) != 0)
            continue;
        
        if (::strcmp(symName+len-7, "_kernel") == 0) // if kernel
            choosenSyms.push_back(i);
        else if (doInfoStrings && ::strcmp(symName, "__OpenCL_compile_options") == 0)
        {
            // set compile options
            const typename Types::Sym& sym = mainElf.getSymbol(i);
            compileOptionShIndex = ULEV(sym.st_shndx);
            if (compileOptionShIndex >= mainElf.getSectionHeadersNum())
                throw BinException("CompileOptions section index out of range");
            const typename Types::Shdr& shdr =
                    mainElf.getSectionHeader(compileOptionShIndex);
            
            char* sectionContent = reinterpret_cast<char*>(
                        mainElf.getSectionContent(compileOptionShIndex));
            if (ULEV(sym.st_value) >= ULEV(shdr.sh_size))
                throw BinException("CompileOptions value out of range");
            // compileOptionsEnd used later for setting offset for driver info
            // compile options precedes driver info
            compileOptionsEnd = ULEV(sym.st_value) + ULEV(sym.st_size);
            if (usumGt(ULEV(sym.st_value), ULEV(sym.st_size), ULEV(shdr.sh_size)))
                throw BinException("CompileOptions value+size out of range");
            compileOptions.assign(sectionContent + ULEV(sym.st_value), ULEV(sym.st_size));
        }
        else if (::strcmp(symName, "__OpenCL_0_global") == 0 ||
                 ::strcmp(symName, "__OpenCL_2_global") == 0)
        {
            /* global data (constant data) */
            const typename Types::Sym& sym = mainElf.getSymbol(i);
            const uint16_t shindex = ULEV(sym.st_shndx);
            const typename Types::Shdr& shdr = mainElf.getSectionHeader(shindex);
            if (ULEV(sym.st_value) >= ULEV(shdr.sh_size))
                throw BinException("globalData value out of range");
            if (usumGt(ULEV(sym.st_value), ULEV(sym.st_size), ULEV(shdr.sh_size)))
                throw BinException("globalData value+size out of range");
            globalDataSize = ULEV(sym.st_size);
            globalData = mainElf.getSectionContent(shindex) + ULEV(sym.st_value);
        }
        else if (doKernelInfo && len >= 18 &&
            ::strcmp(symName+len-9, "_metadata") == 0) // if metadata
            choosenSymsMetadata.push_back(i);
        else if (doKernelHeaders && ::strcmp(symName+len-7, "_header") == 0) // if header
            choosenSymsHeaders.push_back(i);
    }
    
    if (doInfoStrings)
    {
        // put driver info
        uint16_t commentShIndex = SHN_UNDEF;
        try
        { commentShIndex = mainElf.getSectionIndex(".comment"); }
        catch(const Exception& ex)
        { }
        if (commentShIndex != SHN_UNDEF)
        {
            size_t offset = 0;
            if (compileOptionShIndex == commentShIndex)
                offset = compileOptionsEnd;
            const typename Types::Shdr& shdr = mainElf.getSectionHeader(commentShIndex);
            const char* sectionContent = reinterpret_cast<char*>(
                            mainElf.getSectionContent(commentShIndex));
            driverInfo.assign(sectionContent + offset, ULEV(shdr.sh_size)-offset);
        }
    }
    
    innerBinaries.resize(choosenSyms.size());
    
    if (textIndex != SHN_UNDEF) /* if have ".text" */
    {
        const typename Types::Shdr& textHdr = mainElf.getSectionHeader(textIndex);
        cxbyte* textContent = mainElf.getBinaryCode() + ULEV(textHdr.sh_offset);
        
        /* create table of innerBinaries */
        size_t ki = 0;
        for (auto it: choosenSyms)
        {
            const char* symName = mainElf.getSymbolName(it);
            size_t len = ::strlen(symName);
            const typename Types::Sym& sym = mainElf.getSymbol(it);
            
            const typename Types::Word symvalue = ULEV(sym.st_value);
            const typename Types::Word symsize = ULEV(sym.st_size);
            if (symvalue > ULEV(textHdr.sh_size))
                throw BinException("Inner binary offset out of range!");
            if (usumGt(symvalue, symsize, ULEV(textHdr.sh_size)))
                throw BinException("Inner binary offset+size out of range!");
            
            innerBinaries[ki++] = AmdInnerGPUBinary32(CString(symName+9, len-16),
                symsize, textContent+symvalue,
                (creationFlags >> AMDBIN_INNER_SHIFT) & AMDBIN_INNER_INT_CREATE_ALL);
        }
        if ((creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0)
        {
            innerBinaryMap.resize(innerBinaries.size());
            for (size_t i = 0; i < innerBinaries.size(); i++)
                innerBinaryMap[i] = std::make_pair(innerBinaries[i].getKernelName(), i);
            mapSort(innerBinaryMap.begin(), innerBinaryMap.end());
        }
    }
    
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
    {
        kernelInfos.resize(choosenSymsMetadata.size());
        metadatas.reset(new AmdGPUKernelMetadata[kernelInfos.size()]);
        
        typename Types::Size ki = 0;
        for (typename Types::Size it: choosenSymsMetadata)
        {
            // read symbol _OpenCL..._metadata
            const typename Types::Sym& sym = mainElf.getSymbol(it);
            const char* symName = mainElf.getSymbolName(it);
            if (ULEV(sym.st_shndx) >= mainElf.getSectionHeadersNum())
                throw BinException("Metadata section index out of range");
            
            const typename Types::Shdr& rodataHdr =
                    mainElf.getSectionHeader(ULEV(sym.st_shndx));
            cxbyte* secContent = mainElf.getBinaryCode() + ULEV(rodataHdr.sh_offset);
            
            const typename Types::Word symvalue = ULEV(sym.st_value);
            const typename Types::Word symsize = ULEV(sym.st_size);
            if (symvalue > ULEV(rodataHdr.sh_size))
                throw BinException("Metadata offset out of range");
            if (usumGt(symvalue, symsize, ULEV(rodataHdr.sh_size)))
                throw BinException("Metadata offset+size out of range");
            
            // parse AMDGPU kernel metadata
            parseAmdGpuKernelMetadata(symName, symsize,
                  reinterpret_cast<const char*>(secContent + symvalue),
                  kernelInfos[ki]);
            metadatas[ki].size = symsize;
            metadatas[ki].data = reinterpret_cast<char*>(secContent + symvalue);
            ki++;
        }
        /* maps kernel info */
        if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
        {
            kernelInfosMap.resize(kernelInfos.size());
            for (size_t i = 0; i < kernelInfos.size(); i++)
                kernelInfosMap[i] = std::make_pair(kernelInfos[i].kernelName, i);
            mapSort(kernelInfosMap.begin(), kernelInfosMap.end());
        }
    }
    if ((creationFlags & AMDBIN_CREATE_KERNELHEADERS) != 0)
    {
        kernelHeaders.resize(choosenSymsHeaders.size());
        
        typename Types::Size ki = 0;
        
        for (typename Types::Size it: choosenSymsHeaders)
        {
            const typename Types::Sym& sym = mainElf.getSymbol(it);
            const char* symName = mainElf.getSymbolName(it);
            const size_t symNameLen = ::strlen(symName);
            if (ULEV(sym.st_shndx) >= mainElf.getSectionHeadersNum())
                throw BinException("KernelHeader section index out of range");
            
            const typename Types::Shdr& rodataHdr =
                    mainElf.getSectionHeader(ULEV(sym.st_shndx));
            cxbyte* secContent = mainElf.getBinaryCode() + ULEV(rodataHdr.sh_offset);
            
            const typename Types::Word symvalue = ULEV(sym.st_value);
            const typename Types::Word symsize = ULEV(sym.st_size);
            if (symvalue > ULEV(rodataHdr.sh_size))
                throw BinException("KernelHeader offset out of range");
            if (usumGt(symvalue, symsize, ULEV(rodataHdr.sh_size)))
                throw BinException("KernelHeader offset+size out of range");
            
            // kernel name preceded by '__OpenCL_' and precedes '_kernel'
            kernelHeaders[ki].kernelName.assign(symName + 9, symName + symNameLen-7);
            kernelHeaders[ki].size = symsize;
            kernelHeaders[ki].data = secContent + symvalue;
            ki++;
        }
        /* maps kernel headers and sort it */
        if ((creationFlags & AMDBIN_CREATE_KERNELHEADERMAP) != 0)
        {
            kernelHeaderMap.resize(kernelHeaders.size());
            for (size_t i = 0; i < kernelHeaders.size(); i++)
                kernelHeaderMap[i] = std::make_pair(kernelHeaders[i].kernelName, i);
            mapSort(kernelHeaderMap.begin(), kernelHeaderMap.end());
        }
    }
}

const AmdInnerGPUBinary32& AmdMainGPUBinaryBase::getInnerBinary(const char* name) const
{
    InnerBinaryMap::const_iterator it = binaryMapFind(innerBinaryMap.begin(),
                  innerBinaryMap.end(), name);
    if (it == innerBinaryMap.end())
        throw BinException("Can't find inner binary");
    return innerBinaries[it->second];
}

const AmdGPUKernelHeader& AmdMainGPUBinaryBase::getKernelHeaderEntry(
            const char* name) const
{
    KernelHeaderMap::const_iterator it = binaryMapFind(kernelHeaderMap.begin(),
                   kernelHeaderMap.end(), name);
    if (it == kernelHeaderMap.end())
        throw BinException("Can't find kernel header");
    return kernelHeaders[it->second];
}

/* AmdMainGPUBinary32 */

AmdMainGPUBinary32::AmdMainGPUBinary32(size_t binaryCodeSize, cxbyte* binaryCode,
       Flags creationFlags) : AmdMainGPUBinaryBase(AmdMainType::GPU_BINARY),
          ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
{
    initMainGPUBinary<AmdGPU32Types>(*this);
}

struct CLRX_INTERNAL GPUDeviceCodeEntry
{
    uint16_t elfMachine;
    GPUDeviceType deviceType;
};

// GPU device code table - code in machine field in ELF header
static const GPUDeviceCodeEntry gpuDeviceCodeTable[19] =
{
    { 0x3fd, GPUDeviceType::TAHITI },
    { 0x3fe, GPUDeviceType::PITCAIRN },
    { 0x3ff, GPUDeviceType::CAPE_VERDE },
    { 0x402, GPUDeviceType::OLAND },
    { 0x403, GPUDeviceType::BONAIRE },
    { 0x404, GPUDeviceType::SPECTRE },
    { 0x405, GPUDeviceType::SPOOKY },
    { 0x406, GPUDeviceType::KALINDI },
    { 0x407, GPUDeviceType::HAINAN },
    { 0x408, GPUDeviceType::HAWAII },
    { 0x409, GPUDeviceType::ICELAND },
    { 0x40a, GPUDeviceType::TONGA },
    { 0x40b, GPUDeviceType::MULLINS },
    { 0x40c, GPUDeviceType::FIJI },
    { 0x40d, GPUDeviceType::CARRIZO },
    { 0x40e, GPUDeviceType::ELLESMERE },
    { 0x40f, GPUDeviceType::BAFFIN },
    { 0x411, GPUDeviceType::DUMMY },
    { 0x412, GPUDeviceType::GFX804 }
};

static const cxuint gpuDeviceCodeTableSize =
            sizeof(gpuDeviceCodeTable)/sizeof(GPUDeviceCodeEntry);

static GPUDeviceType findGPUDeviceType(uint16_t elfMachine)
{
    cxuint index = binaryFind(gpuDeviceCodeTable,
        gpuDeviceCodeTable+gpuDeviceCodeTableSize, { elfMachine },
        [] (const GPUDeviceCodeEntry& l, const GPUDeviceCodeEntry& r) -> bool
        { return l.elfMachine < r.elfMachine;}) - gpuDeviceCodeTable;
    if (gpuDeviceCodeTableSize == index)
        throw BinException("Can't determine GPU device type");
    return gpuDeviceCodeTable[index].deviceType;
}

GPUDeviceType AmdMainGPUBinary32::determineGPUDeviceType() const
{
    return findGPUDeviceType(ULEV(getHeader().e_machine));
}

/* AmdMainGPUBinary64 */

AmdMainGPUBinary64::AmdMainGPUBinary64(size_t binaryCodeSize, cxbyte* binaryCode,
       Flags creationFlags) : AmdMainGPUBinaryBase(AmdMainType::GPU_64_BINARY),
          ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{
    initMainGPUBinary<AmdGPU64Types>(*this);
}

GPUDeviceType AmdMainGPUBinary64::determineGPUDeviceType() const
{
    return findGPUDeviceType(ULEV(getHeader().e_machine));
}

/* AmdMainX86Binary32 */

void AmdMainX86Binary32::initKernelInfos(Flags creationFlags)
{
    innerBinary.getKernelInfos(kernelInfos);
    if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
    {
        kernelInfosMap.resize(kernelInfos.size());
        for (size_t i = 0; i < kernelInfos.size(); i++)
            kernelInfosMap[i] = std::make_pair(kernelInfos[i].kernelName, i);
        mapSort(kernelInfosMap.begin(), kernelInfosMap.end());
    }
}

AmdMainX86Binary32::AmdMainX86Binary32(size_t binaryCodeSize, cxbyte* binaryCode,
       Flags creationFlags) : AmdMainBinaryBase(AmdMainType::X86_BINARY),
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
        cxbyte* textContent = binaryCode + ULEV(textHdr.sh_offset);
        
        innerBinary = AmdInnerX86Binary32(ULEV(textHdr.sh_size), textContent,
                (creationFlags >> AMDBIN_INNER_SHIFT) & AMDBIN_INNER_INT_CREATE_ALL);
    }
    
    if ((creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0)
    {
        size_t compileOptionsEnd = 0;
        uint16_t compileOptionShIndex = SHN_UNDEF;
        
        for (size_t i = 0; i < symbolsNum; i++)
        {
            const char* symName = getSymbolName(i);
            if (::strcmp(symName, "__OpenCL_compile_options") == 0)
            {
                // set compile options
                const Elf32_Sym& sym = getSymbol(i);
                compileOptionShIndex = ULEV(sym.st_shndx);
                if (compileOptionShIndex >= getSectionHeadersNum())
                    throw BinException("CompileOptions section index out of range");
                const Elf32_Shdr& shdr = getSectionHeader(compileOptionShIndex);
                const char* sectionContent = reinterpret_cast<char*>(
                            getSectionContent(compileOptionShIndex));
                if (ULEV(sym.st_value) >= ULEV(shdr.sh_size))
                    throw BinException("CompileOptions value out of range");
                // compileOptionsEnd used later for setting offset for driver info
                // compile options precedes driver info
                compileOptionsEnd = ULEV(sym.st_value) + ULEV(sym.st_size);
                if (usumGt(ULEV(sym.st_value), ULEV(sym.st_size), ULEV(shdr.sh_size)))
                    throw BinException("CompileOptions value+size out of range");
                compileOptions.assign(sectionContent + ULEV(sym.st_value),
                                      ULEV(sym.st_size));
            }
        }
        
        // put driver info
        uint16_t commentShIndex = SHN_UNDEF;
        try
        { commentShIndex = getSectionIndex(".comment"); }
        catch(const Exception& ex)
        { }
        if (commentShIndex != SHN_UNDEF)
        {
            size_t offset = 0;
            if (compileOptionShIndex == commentShIndex)
                offset = compileOptionsEnd;
            const Elf32_Shdr& shdr = getSectionHeader(commentShIndex);
            const char* sectionContent = reinterpret_cast<char*>(
                            getSectionContent(commentShIndex));
            driverInfo.assign(sectionContent + offset, ULEV(shdr.sh_size)-offset);
        }
    }
    
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        initKernelInfos(creationFlags);
}

/* AmdMainX86Binary64 */

void AmdMainX86Binary64::initKernelInfos(Flags creationFlags)
{
    innerBinary.getKernelInfos(kernelInfos);
    if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
    {
        kernelInfosMap.resize(kernelInfos.size());
        for (size_t i = 0; i < kernelInfos.size(); i++)
            kernelInfosMap[i] = std::make_pair(kernelInfos[i].kernelName, i);
        mapSort(kernelInfosMap.begin(), kernelInfosMap.end());
    }
}

AmdMainX86Binary64::AmdMainX86Binary64(size_t binaryCodeSize, cxbyte* binaryCode,
       Flags creationFlags) : AmdMainBinaryBase(AmdMainType::X86_64_BINARY),
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
        cxbyte* textContent = binaryCode + ULEV(textHdr.sh_offset);
        
        innerBinary = AmdInnerX86Binary64(ULEV(textHdr.sh_size), textContent,
                (creationFlags >> AMDBIN_INNER_SHIFT) & AMDBIN_INNER_INT_CREATE_ALL);
    }
    
    if ((creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0)
    {
        size_t compileOptionsEnd = 0;
        uint16_t compileOptionShIndex = SHN_UNDEF;
        for (size_t i = 0; i < symbolsNum; i++)
        {
            const char* symName = getSymbolName(i);
            if (::strcmp(symName, "__OpenCL_compile_options") == 0)
            {
                // set compile options
                const Elf64_Sym& sym = getSymbol(i);
                compileOptionShIndex = ULEV(sym.st_shndx);
                if (compileOptionShIndex >= getSectionHeadersNum())
                    throw BinException("CompileOptions section index out of range");
                const Elf64_Shdr& shdr = getSectionHeader(compileOptionShIndex);
                const char* sectionContent = reinterpret_cast<char*>(
                            getSectionContent(compileOptionShIndex));
                if (ULEV(sym.st_value) >= ULEV(shdr.sh_size))
                    throw BinException("CompileOptions value out of range");
                // compileOptionsEnd used later for setting offset for driver info
                // compile options precedes driver info
                compileOptionsEnd = ULEV(sym.st_value) + ULEV(sym.st_size);
                if (usumGt(ULEV(sym.st_value), ULEV(sym.st_size), ULEV(shdr.sh_size)))
                    throw BinException("CompileOptions value+size out of range");
                compileOptions.assign(sectionContent + ULEV(sym.st_value),
                                      ULEV(sym.st_size));
            }
        }
        
        // put driver info
        uint16_t commentShIndex = SHN_UNDEF;
        try
        { commentShIndex = getSectionIndex(".comment"); }
        catch(const Exception& ex)
        { }
        if (commentShIndex != SHN_UNDEF)
        {
            size_t offset = 0;
            if (compileOptionShIndex == commentShIndex)
                offset = compileOptionsEnd;
            const Elf64_Shdr& shdr = getSectionHeader(commentShIndex);
            const char* sectionContent = reinterpret_cast<char*>(
                            getSectionContent(commentShIndex));
            driverInfo.assign(sectionContent + offset, ULEV(shdr.sh_size)-offset);
        }
    }
    
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        initKernelInfos(creationFlags);
}

bool CLRX::isAmdBinary(size_t binarySize, const cxbyte* binary)
{
    if (!isElfBinary(binarySize, binary))
        return false;
    
    if (binary[EI_CLASS] == ELFCLASS32)
    {
        const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(binary);
        uint32_t emachine = ULEV(ehdr->e_machine);
        if ((emachine != ELF_M_X86 && (emachine<=0x3f0 || emachine>=0x500)) ||
                    ULEV(ehdr->e_flags)!=0)
            return false;
    }
    else // elf 64-bit
    {
        const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binary);
        uint32_t emachine = ULEV(ehdr->e_machine);
        if ((emachine != ELF_M_X86 && (emachine<=0x3f0 || emachine>=0x500)) ||
                    ULEV(ehdr->e_flags)!=0)
            return false;
    }
    return true;
}

/* create amd binary */

AmdMainBinaryBase* CLRX::createAmdBinaryFromCode(size_t binaryCodeSize, cxbyte* binaryCode,
        Flags creationFlags)
{
    // checking whether is AMDOCL binary (little endian and ELF magic)
    if (binaryCodeSize < sizeof(Elf32_Ehdr) ||
        ULEV(*reinterpret_cast<const uint32_t*>(binaryCode)) != elfMagicValue)
        throw BinException("This is not ELF binary");
    if (binaryCode[EI_DATA] != ELFDATA2LSB)
        throw BinException("Other than little-endian binaries are not supported!");
    
    // checking binary class
    if (binaryCode[EI_CLASS] == ELFCLASS32)
    {
        const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(binaryCode);
        if (ULEV(ehdr->e_machine) != ELF_M_X86) //if gpu
            return new AmdMainGPUBinary32(binaryCodeSize, binaryCode, creationFlags);
        return new AmdMainX86Binary32(binaryCodeSize, binaryCode, creationFlags);
    }
    else if (binaryCode[EI_CLASS] == ELFCLASS64)
    {
        const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binaryCode);
        if (ULEV(ehdr->e_machine) != ELF_M_X86)
            return new AmdMainGPUBinary64(binaryCodeSize, binaryCode, creationFlags);
        return new AmdMainX86Binary64(binaryCodeSize, binaryCode, creationFlags);
    }
    else // fatal error
        throw BinException("Unsupported ELF class");
}
