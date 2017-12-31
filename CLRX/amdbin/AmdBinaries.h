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
/*! \file AmdBinaries.h
 * \brief AMD binaries handling
 */

#ifndef __CLRX_AMDBINARIES_H__
#define __CLRX_AMDBINARIES_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <memory>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/amdbin/ElfBinaries.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>

#ifdef HAVE_MINGW
#undef VOID
#endif

/// main namespace
namespace CLRX
{

enum : Flags {
    AMDBIN_INNER_INT_CREATE_ALL = 0xff,
    AMDBIN_CREATE_CALNOTES = 0x10, ///< only for internal usage
    
    AMDBIN_CREATE_KERNELINFO = 0x10,    ///< create kernel informations
    AMDBIN_CREATE_KERNELINFOMAP = 0x20, ///< create map of kernel informations
    AMDBIN_CREATE_INNERBINMAP = 0x40,   ///< create map of inner binaries
    AMDBIN_CREATE_INFOSTRINGS = 0x80, ///< create compile options and driver info
    AMDBIN_CREATE_KERNELHEADERS = 0x100,    ///< create kernel headers
    AMDBIN_CREATE_KERNELHEADERMAP = 0x200,    ///< create kernel headers map
    AMDBIN_INNER_CREATE_SECTIONMAP = 0x1000, ///< create map of sections for inner binaries
    AMDBIN_INNER_CREATE_SYMBOLMAP = 0x2000,  ///< create map of symbols for inner binaries
    /** create map of dynamic symbols for inner binaries */
    AMDBIN_INNER_CREATE_DYNSYMMAP = 0x4000,
    AMDBIN_INNER_CREATE_CALNOTES = 0x10000, ///< create CAL notes for AMD inner GPU binary
    
    AMDBIN_CREATE_ALL = ELF_CREATE_ALL | 0xffff0, ///< all AMD binaries creation flags
    AMDBIN_INNER_SHIFT = 12 ///< shift for convert inner binary flags into elf binary flags
};

enum : cxuint {
    ELF_M_X86 = 0x7d4
};

/// kernel argument type
enum class KernelArgType : cxbyte
{
    VOID = 0,
    UCHAR, CHAR, USHORT, SHORT, UINT, INT, ULONG, LONG, FLOAT, DOUBLE, POINTER, IMAGE,
    IMAGE1D, IMAGE1D_ARRAY, IMAGE1D_BUFFER, IMAGE2D, IMAGE2D_ARRAY, IMAGE3D,
    UCHAR2, UCHAR3, UCHAR4, UCHAR8, UCHAR16,
    CHAR2, CHAR3, CHAR4, CHAR8, CHAR16,
    USHORT2, USHORT3, USHORT4, USHORT8, USHORT16,
    SHORT2, SHORT3, SHORT4, SHORT8, SHORT16,
    UINT2, UINT3, UINT4, UINT8, UINT16,
    INT2, INT3, INT4, INT8, INT16,
    ULONG2, ULONG3, ULONG4, ULONG8, ULONG16,
    LONG2, LONG3, LONG4, LONG8, LONG16,
    FLOAT2, FLOAT3, FLOAT4, FLOAT8, FLOAT16,
    DOUBLE2, DOUBLE3, DOUBLE4, DOUBLE8, DOUBLE16,
    SAMPLER, STRUCTURE, COUNTER32, COUNTER64, PIPE, CMDQUEUE, CLKEVENT,
    MAX_VALUE = COUNTER64,
    MAX_VALUE_CL2 = CLKEVENT,
    MIN_IMAGE = IMAGE,
    MAX_IMAGE = IMAGE3D
};

/// return true if argType is image
inline bool isKernelArgImage(KernelArgType argType);

inline bool isKernelArgImage(KernelArgType argType)
{
    return argType>=KernelArgType::MIN_IMAGE && argType<=KernelArgType::MAX_IMAGE;
}

/// kernel pointer type of argument
enum class KernelPtrSpace : cxbyte
{
    NONE = 0,   ///< no pointer
    LOCAL,      ///< pointer to local memory
    CONSTANT,   ///< pointer to constant memory
    GLOBAL,      ///< pointer to global memory
    MAX_VALUE = GLOBAL
};

enum : cxbyte
{
    KARG_PTR_NORMAL = 0,    ///< no special flags
    KARG_PTR_READ_ONLY = 1, ///< read only image
    KARG_PTR_WRITE_ONLY = 2,    ///< write only image
    KARG_PTR_READ_WRITE = 3,    ///< read-write image (???)
    KARG_PTR_ACCESS_MASK = 3,
    KARG_PTR_CONST = 4,     ///< constant buffer
    KARG_PTR_RESTRICT = 8,  ///< buffer is restrict specifier
    KARG_PTR_VOLATILE = 16  ///< volatile buffer
};

/// kernel argument info structure
struct AmdKernelArg
{
    KernelArgType argType;  ///< argument type
    KernelPtrSpace ptrSpace;///< pointer space for argument if argument is pointer or image
    cxbyte ptrAccess;  ///< pointer access flags
    CString typeName;   ///< name of type of argument
    CString argName;    ///< argument name
};

/// X86_64 kernel argument symbol
struct X86KernelArgSym
{
    uint32_t argType;   ///< arg type
    uint32_t ptrType;   ///< pointer type
    uint32_t ptrAccess; ///< pointer access flags
    uint32_t nameOffset; ///< name offset
    
    /// get name offset
    size_t getNameOffset() const
    { return ULEV(nameOffset); }
};

/// X86_64 kernel argument symbol
struct X86_64KernelArgSym
{
    uint32_t argType;   ///< arg type
    uint32_t ptrType;   ///< pointer type
    uint32_t ptrAccess; ///< pointer access flags
    uint32_t nameOffsetLo;  ///< low half of name offset
    uint32_t nameOffsetHi;  ///< high half of name offset
    
    /// compute name offset
    size_t getNameOffset() const
    { return (uint64_t(ULEV(nameOffsetHi))<<32)+uint64_t(ULEV(nameOffsetLo)); }
};

enum : uint32_t
{
    /* this cal note types comes from MultiSim-4.2 sources */
    CALNOTE_ATI_PROGINFO = 1,
    CALNOTE_ATI_INPUTS = 2,
    CALNOTE_ATI_OUTPUTS = 3,
    CALNOTE_ATI_CONDOUT = 4,
    CALNOTE_ATI_FLOAT32CONSTS = 5,
    CALNOTE_ATI_INT32CONSTS = 6,
    CALNOTE_ATI_BOOL32CONSTS = 7,
    CALNOTE_ATI_EARLYEXIT = 8,
    CALNOTE_ATI_GLOBAL_BUFFERS = 9,
    CALNOTE_ATI_CONSTANT_BUFFERS = 10,
    CALNOTE_ATI_INPUT_SAMPLERS = 11,
    CALNOTE_ATI_PERSISTENT_BUFFERS = 12,
    CALNOTE_ATI_SCRATCH_BUFFERS = 13,
    CALNOTE_ATI_SUB_CONSTANT_BUFFERS = 14,
    CALNOTE_ATI_UAV_MAILBOX_SIZE = 15,
    CALNOTE_ATI_UAV = 16,
    CALNOTE_ATI_UAV_OP_MASK = 17,
    CALNOTE_ATI_MAXTYPE = CALNOTE_ATI_UAV_OP_MASK
};

/// CALEncodingEntry. There are not copied (ULEV must be used)
struct CALEncodingEntry
{
    uint32_t machine;   ///< machine type
    uint32_t type;      ///< type of entry
    uint32_t offset;    ///< offset in ELF
    uint32_t size;      ///< size
    uint32_t flags;     ///< flags
};

/// ATI CAL Note header. There are not copied (ULEV must be used)
struct CALNoteHeader
{
    uint32_t nameSize;  ///< name size (must be 8)
    uint32_t descSize;  ///< description size
    uint32_t type;  ///< type
    char name[8];   ///< name string
};

/// ATI CAL note. There are not copied (ULEV must be used)
struct CALNote
{
    CALNoteHeader* header;  ///< header of CAL note
    cxbyte* data;   ///< data of CAL note
};

/// ATI CAL note input
struct CALNoteInput
{
    CALNoteHeader header;  ///< header of CAL note
    const cxbyte* data;   ///< data of CAL note
};

/// CAL program info entry. There are not copied (ULEV must be used)
struct CALProgramInfoEntry
{
    uint32_t address;   ///< address of value
    uint32_t value;     ///< value to set
};

/// There are not copied (ULEV must be used)
struct CALUAVEntry
{
    uint32_t uavId; ///< uav id
    uint32_t f1;    ///< field 1
    uint32_t f2;    ///< field 2
    uint32_t type;  ///< type
};

/// There are not copied (ULEV must be used)
struct CALDataSegmentEntry
{
    uint32_t offset;    ///< offset
    uint32_t size;      ///< size
};

/// There are not copied (ULEV must be used)
struct CALConstantBufferMask
{
    uint32_t index; ///< index
    uint32_t size;  ///< size
};

/// There are not copied (ULEV must be used)
struct CALSamplerMapEntry
{
    uint32_t input; ///< input
    uint32_t sampler;   ///< sampler
};

/// kernel informations
/** all fields and all KernelArgInfo datas is in host endian and is aligned. This object
 * is created from binary data available in ELF binaries
 */
struct KernelInfo
{
    CString kernelName; ///< kernel name
    Array<AmdKernelArg> argInfos;    ///< array of argument informations
};

/// AMD inner binary for GPU binaries that represent a single kernel
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdInnerGPUBinary32: public ElfBinary32
{
private:
    /// ATTENTION: Do not put non-copyable stuff (likes pointers, arrays),
    /// because this object will copied
    CString kernelName; ///< kernel name
    uint32_t encodingEntriesNum;
    CALEncodingEntry* encodingEntries;
    Array<Array<CALNote> > calNotesTable;
public:
    AmdInnerGPUBinary32() = default;
    /** constructor
     * \param kernelName kernel name
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerGPUBinary32(const CString& kernelName, size_t binaryCodeSize,
            cxbyte* binaryCode, Flags creationFlags = ELF_CREATE_ALL);
    ~AmdInnerGPUBinary32() = default;
    
    /// return true if binary has CAL notes infos
    bool hasCALNotes() const
    { return (creationFlags & AMDBIN_CREATE_CALNOTES) != 0; }
    
    /// get kernel name
    const CString& getKernelName() const
    { return kernelName; }
    
    /// get CALEncoding entries number
    uint32_t getCALEncodingEntriesNum() const
    { return encodingEntriesNum; }
    
    /// get CALEncodingDictionaryEntries
    const CALEncodingEntry& getCALEncodingEntry(cxuint index) const
    { return encodingEntries[index]; }
    
    /// get CALEncodingDictionaryEntries
    CALEncodingEntry& getCALEncodingEntry(cxuint index)
    { return encodingEntries[index]; }
    
    /// get CAL Notes number
    uint32_t getCALNotesNum(cxuint encodingIndex) const
    { return calNotesTable[encodingIndex].size(); }
    
    /// get CAL Note header
    const CALNoteHeader& getCALNoteHeader(cxuint encodingIndex, uint32_t index) const
    { return *(calNotesTable[encodingIndex][index]).header; }
    
    /// get CAL Note header
    CALNoteHeader& getCALNoteHeader(cxuint encodingIndex, uint32_t index)
    { return *(calNotesTable[encodingIndex][index]).header; }
    
    /// get CAL Note data
    const cxbyte* getCALNoteData(cxuint encodingIndex, uint32_t index) const
    { return calNotesTable[encodingIndex][index].data; }
    /// get CAL Note data
    cxbyte* getCALNoteData(cxuint encodingIndex, uint32_t index)
    { return calNotesTable[encodingIndex][index].data; }
    
    /// get CALNote
    /**
     * \param encodingIndex index of encoding
     * \param index index of CALNote
     * \return CALNote
     */
    const CALNote& getCALNote(cxuint encodingIndex, uint32_t index) const
    { return calNotesTable[encodingIndex][index]; }
    
    /// get CALNote
    /**
     * \param encodingIndex index of encoding
     * \param index index of CALNote
     * \return CALNote
     */
    CALNote& getCALNote(cxuint encodingIndex, uint32_t index)
    { return calNotesTable[encodingIndex][index]; }
    
    /// get all CALNotes for encoding index
    const Array<CALNote>& getCALNotes(cxuint encodingIndex) const
    { return calNotesTable[encodingIndex]; }
    
    /// get all CALNotes for encoding index
    Array<CALNote>& getCALNotes(cxuint encodingIndex)
    { return calNotesTable[encodingIndex]; }
    
    /// find CAL encoding entry for specified device type
    /**
     * \param deviceType specified device type
     * \return CAL Encoding Entry index
     */
    cxuint findCALEncodingEntryIndex(GPUDeviceType deviceType) const;
};

/// AMD inner X86 binary
class AmdInnerX86Binary32: public ElfBinary32
{
public:
    AmdInnerX86Binary32() = default;
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerX86Binary32(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = ELF_CREATE_ALL);
    ~AmdInnerX86Binary32() = default;
    
    /// generate kernel info from this binary and save to KernelInfo array
    void getKernelInfos(Array<KernelInfo>& kernelInfos) const;
};

/// AMD inner binary for X86-64 binaries
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdInnerX86Binary64: public ElfBinary64
{
public:
    AmdInnerX86Binary64() = default;
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerX86Binary64(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = ELF_CREATE_ALL);
    ~AmdInnerX86Binary64() = default;
    
    /// generate kernel info from this binary and save to KernelInfo array
    void getKernelInfos(Array<KernelInfo>& kernelInfos) const;
};

/// AMD main binary type
enum class AmdMainType
{
    GPU_BINARY, ///< binary for GPU
    GPU_64_BINARY, ///< binary for GPU with 64-bit memory model
    X86_BINARY, ///< binary for x86 systems
    X86_64_BINARY, ///< binary for x86-64 systems
    GPU_CL2_BINARY, ///< OpenCL 2.0 binary type (32-bit)
    GPU_CL2_64_BINARY ///< OpenCL 2.0 binary type (64-bit)
};

/// main AMD binary base class
class AmdMainBinaryBase: public NonCopyableAndNonMovable
{
public:
    /// Kernel info map
    typedef Array<std::pair<CString, size_t> > KernelInfoMap;
protected:
    AmdMainType type;   ///< type of binaries
    Array<KernelInfo> kernelInfos;    ///< kernel informations
    KernelInfoMap kernelInfosMap;   ///< kernel informations map
    
    CString driverInfo; ///< driver info string
    CString compileOptions; ///< compiler options string
    
    /// constructor
    explicit AmdMainBinaryBase(AmdMainType type);
public:
    virtual ~AmdMainBinaryBase();
    
    /// get binary type
    AmdMainType getType() const
    { return type; }
    
    /// get kernel informations number
    size_t getKernelInfosNum() const
    { return kernelInfos.size(); }
    
    /// get kernel informations array
    const KernelInfo* getKernelInfos() const
    { return kernelInfos.data(); }
    
    /// get kernel information with specified index
    const KernelInfo& getKernelInfo(size_t index) const
    { return kernelInfos[index]; }
    
    /// get kernel information with specified kernel name (requires kernel info map)
    const KernelInfo& getKernelInfo(const char* name) const;
    
    /// get driver info string
    const CString& getDriverInfo() const
    { return driverInfo; }
    
    /// get compile options string
    const CString& getCompileOptions() const
    { return compileOptions; }
};

/// AMD GPU metadata for kernel
struct AmdGPUKernelMetadata
{
    size_t size;    ///< size
    char* data;     ///< data (unterminated string)
};

/// AMD GPU header for kernel
struct AmdGPUKernelHeader
{
    CString kernelName; ///< kernel name
    size_t size;    ///< size
    cxbyte* data;   ///< data
};

/// main AMD GPU binary base class
class AmdMainGPUBinaryBase: public AmdMainBinaryBase
{
public:
    /// inner binary map type
    typedef Array<std::pair<CString, size_t> > InnerBinaryMap;
    /// kernel header map type
    typedef Array<std::pair<CString, size_t> > KernelHeaderMap;
protected:
    Array<AmdInnerGPUBinary32> innerBinaries;   ///< inner binaries
    InnerBinaryMap innerBinaryMap;  ///< inner binary map
    std::unique_ptr<AmdGPUKernelMetadata[]> metadatas;  ///< AMD metadatas
    Array<AmdGPUKernelHeader> kernelHeaders;    ///< kernel headers
    KernelHeaderMap kernelHeaderMap;    ///< kernel header map
    size_t globalDataSize;  ///< global data size
    cxbyte* globalData; ///< global data content
    
    /// constructor
    explicit AmdMainGPUBinaryBase(AmdMainType type);
    
    /// initialize main gpu binary (internal use only)
    template<typename Types>
    void initMainGPUBinary(typename Types::ElfBinary& binary);
public:
    /// get number of inner binaries
    size_t getInnerBinariesNum() const
    { return innerBinaries.size(); }
    
    /// get inner binary with specified index
    AmdInnerGPUBinary32& getInnerBinary(size_t index)
    { return innerBinaries[index]; }
    
    /// get inner binary with specified index
    const AmdInnerGPUBinary32& getInnerBinary(size_t index) const
    { return innerBinaries[index]; }
    
    /// get inner binary with specified name (requires inner binary map)
    const AmdInnerGPUBinary32& getInnerBinary(const char* name) const;
    
    /// get metadata size for specified inner binary
    size_t getMetadataSize(size_t index) const
    { return metadatas[index].size; }
    
    /// get metadata for specified inner binary
    const char* getMetadata(size_t index) const
    { return metadatas[index].data; }
    
    /// get metadata for specified inner binary
    char* getMetadata(size_t index)
    { return metadatas[index].data; }
    
    /// get global data size
    size_t getGlobalDataSize() const
    { return globalDataSize; }
    
    /// get global data
    const cxbyte* getGlobalData() const
    { return globalData; }
    /// get global data
    cxbyte* getGlobalData()
    { return globalData; }
    
    /// get kernel header number
    size_t getKernelHeadersNum() const
    { return kernelHeaders.size(); }
    
    /// get kernel header entry for specified index
    const AmdGPUKernelHeader& getKernelHeaderEntry(size_t index) const
    { return kernelHeaders[index]; }
    
    /// get kernel header entry for specified index
    const AmdGPUKernelHeader& getKernelHeaderEntry(const char* name) const;
    
    /// get kernel header entry for specified index
    AmdGPUKernelHeader& getKernelHeaderEntry(size_t index)
    { return kernelHeaders[index]; }
    
    /// get kernel header size for specified index
    size_t getKernelHeaderSize(size_t index) const
    { return kernelHeaders[index].size; }
    
    /// get kernel header for specified index
    const cxbyte* getKernelHeader(size_t index) const
    { return kernelHeaders[index].data; }
    
    /// get kernel header for specified index
    cxbyte* getKernelHeader(size_t index)
    { return kernelHeaders[index].data; }
};

/// AMD main binary for GPU for 32-bit mode
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdMainGPUBinary32: public AmdMainGPUBinaryBase, public ElfBinary32
{
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainGPUBinary32(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = AMDBIN_CREATE_ALL);
    ~AmdMainGPUBinary32() = default;
    
    // determine GPU device type from this binary
    GPUDeviceType determineGPUDeviceType() const;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has inner binary map
    bool hasInnerBinaryMap() const
    { return (creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0; }
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    /// return true if binary has kernel headers
    bool hasKernelHeaders() const
    { return (creationFlags & AMDBIN_CREATE_KERNELHEADERS) != 0; }
    
    /// return true if binary has kernel header map
    bool hasKernelHeaderMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELHEADERMAP) != 0; }
};

/// AMD main binary for GPU for 64-bit mode
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdMainGPUBinary64: public AmdMainGPUBinaryBase, public ElfBinary64
{
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainGPUBinary64(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = AMDBIN_CREATE_ALL);
    ~AmdMainGPUBinary64() = default;
    
    // determine GPU device type from this binary
    GPUDeviceType determineGPUDeviceType() const;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has inner binary map
    bool hasInnerBinaryMap() const
    { return (creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0; }
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    /// return true if binary has kernel headers
    bool hasKernelHeaders() const
    { return (creationFlags & AMDBIN_CREATE_KERNELHEADERS) != 0; }
    
    /// return true if binary has kernel header map
    bool hasKernelHeaderMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELHEADERMAP) != 0; }
};

/// AMD main binary for X86 systems
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdMainX86Binary32: public AmdMainBinaryBase, public ElfBinary32
{
private:
    AmdInnerX86Binary32 innerBinary;
    
    void initKernelInfos(Flags creationFlags);
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainX86Binary32(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = AMDBIN_CREATE_ALL);
    ~AmdMainX86Binary32() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    /// return true if binary has inner binary
    bool hasInnerBinary() const
    { return innerBinary; }
    
    /// get inner binary
    AmdInnerX86Binary32& getInnerBinary()
    { return innerBinary; }
    
    /// get inner binary
    const AmdInnerX86Binary32& getInnerBinary() const
    { return innerBinary; }
};

/// AMD main binary for X86-64 systems
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdMainX86Binary64: public AmdMainBinaryBase, public ElfBinary64
{
private:
    AmdInnerX86Binary64 innerBinary;
    
    void initKernelInfos(Flags creationFlags);
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainX86Binary64(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = AMDBIN_CREATE_ALL);
    ~AmdMainX86Binary64() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    /// return true if binary has inner binary
    bool hasInnerBinary() const
    { return innerBinary; }
    
    /// get inner binary
    AmdInnerX86Binary64& getInnerBinary()
    { return innerBinary; }
    /// get inner binary
    const AmdInnerX86Binary64& getInnerBinary() const
    { return innerBinary; }
};

/// check whether binary code is Amd Catalyst binary
extern bool isAmdBinary(size_t binaryCodeSize, const cxbyte* binaryCode);

/// create AMD binary object from binary code
/**
 * \param binaryCodeSize binary code size
 * \param binaryCode pointer to binary code
 * \param creationFlags flags that specified what will be created during creation
 * \return binary object
 */
extern AmdMainBinaryBase* createAmdBinaryFromCode(
            size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = AMDBIN_CREATE_ALL);

};

#endif
