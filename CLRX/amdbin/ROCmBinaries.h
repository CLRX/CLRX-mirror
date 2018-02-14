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
/*! \file ROCmBinaries.h
 * \brief ROCm binaries handling
 */

#ifndef __CLRX_ROCMBINARIES_H__
#define __CLRX_ROCMBINARIES_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/amdbin/ElfBinaries.h>
#include <CLRX/amdbin/Commons.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/InputOutput.h>

/// main namespace
namespace CLRX
{

enum : Flags {
    ROCMBIN_CREATE_REGIONMAP = 0x10,    ///< create region map
    ROCMBIN_CREATE_METADATAINFO = 0x20,     ///< create metadata info object
    ROCMBIN_CREATE_KERNELINFOMAP = 0x40,    ///< create kernel metadata info map
    ROCMBIN_CREATE_ALL = ELF_CREATE_ALL | 0xfff0 ///< all ROCm binaries flags
};

/// ROCm region/symbol type
enum ROCmRegionType: uint8_t
{
    DATA,   ///< data object
    FKERNEL,   ///< function kernel (code)
    KERNEL  ///< OpenCL kernel to call ??
};

/// ROCm data region
struct ROCmRegion
{
    CString regionName; ///< region name
    size_t size;    ///< data size
    size_t offset;     ///< data
    ROCmRegionType type; ///< type
};

/// ROCm Value kind
enum class ROCmValueKind : cxbyte
{
    BY_VALUE = 0,       ///< value is just value
    GLOBAL_BUFFER,      ///< passed in global buffer
    DYN_SHARED_PTR,     ///< passed as dynamic shared pointer
    SAMPLER,            ///< sampler
    IMAGE,              ///< image
    PIPE,               ///< OpenCL pipe
    QUEUE,              ///< OpenCL queue
    HIDDEN_GLOBAL_OFFSET_X, ///< OpenCL global offset X
    HIDDEN_GLOBAL_OFFSET_Y, ///< OpenCL global offset Y
    HIDDEN_GLOBAL_OFFSET_Z, ///< OpenCL global offset Z
    HIDDEN_NONE,            ///< none (not used)
    HIDDEN_PRINTF_BUFFER,   ///< buffer for printf calls
    HIDDEN_DEFAULT_QUEUE,   ///< OpenCL default queue
    HIDDEN_COMPLETION_ACTION,    ///< ???
    MAX_VALUE = HIDDEN_COMPLETION_ACTION
};

/// ROCm argument's value type
enum class ROCmValueType : cxbyte
{
    STRUCTURE = 0,  ///< structure
    INT8,       ///< 8-bit signed integer
    UINT8,      ///< 8-bit unsigned integer
    INT16,      ///< 16-bit signed integer
    UINT16,     ///< 16-bit unsigned integer
    FLOAT16,    ///< half floating point
    INT32,      ///< 32-bit signed integer
    UINT32,     ///< 32-bit unsigned integer
    FLOAT32,    ///< single floating point
    INT64,      ///< 64-bit signed integer
    UINT64,     ///< 64-bit unsigned integer
    FLOAT64,     ///< double floating point
    MAX_VALUE = FLOAT64
};

/// ROCm argument address space
enum class ROCmAddressSpace : cxbyte
{
    NONE = 0,
    PRIVATE,
    GLOBAL,
    CONSTANT,
    LOCAL,
    GENERIC,
    REGION,
    MAX_VALUE = REGION
};

/// ROCm access qualifier
enum class ROCmAccessQual: cxbyte
{
    DEFAULT = 0,
    READ_ONLY,
    WRITE_ONLY,
    READ_WRITE,
    MAX_VALUE = READ_WRITE
};

/// ROCm kernel argument
struct ROCmKernelArgInfo
{
    CString name;       ///< name
    CString typeName;   ///< type name
    uint64_t size;      ///< argument size in bytes
    uint64_t align;     ///< argument alignment in bytes
    uint64_t pointeeAlign;      ///< alignemnt of pointed data of pointer
    ROCmValueKind valueKind;    ///< value kind
    ROCmValueType valueType;    ///< value type
    ROCmAddressSpace addressSpace;  ///< pointer address space
    ROCmAccessQual accessQual;      ///< access qualifier (for images and values)
    ROCmAccessQual actualAccessQual;    ///< actual access qualifier
    bool isConst;       ///< is constant
    bool isRestrict;    ///< is restrict
    bool isVolatile;    ///< is volatile
    bool isPipe;        ///< is pipe
};

/// ROCm kernel metadata
struct ROCmKernelMetadata
{
    CString name;       ///< kernel name
    CString symbolName; ///< symbol name
    std::vector<ROCmKernelArgInfo> argInfos;  ///< kernel arguments
    CString language;       ///< language
    cxuint langVersion[2];  ///< language version
    cxuint reqdWorkGroupSize[3];    ///< required work group size
    cxuint workGroupSizeHint[3];    ///< work group size hint
    CString vecTypeHint;    ///< vector type hint
    CString runtimeHandle;  ///< symbol of runtime handle
    uint64_t kernargSegmentSize;    ///< kernel argument segment size
    uint64_t groupSegmentFixedSize; ///< group segment size (fixed)
    uint64_t privateSegmentFixedSize;   ///< private segment size (fixed)
    uint64_t kernargSegmentAlign;       ///< alignment of kernel argument segment
    cxuint wavefrontSize;       ///< wavefront size
    cxuint sgprsNum;        ///< number of SGPRs
    cxuint vgprsNum;        ///< number of VGPRs
    uint64_t maxFlatWorkGroupSize;
    cxuint fixedWorkGroupSize[3];
    cxuint spilledSgprs;    ///< number of spilled SGPRs
    cxuint spilledVgprs;    ///< number of spilled VGPRs
    
    void initialize();
};

/// ROCm printf call info
struct ROCmPrintfInfo
{
    uint32_t id;    /// unique id of call
    Array<uint32_t> argSizes;   ///< argument sizes
    CString format;     ///< printf format
};

/// ROCm binary metadata
struct ROCmMetadata
{
    cxuint version[2];  ///< version
    std::vector<ROCmPrintfInfo> printfInfos;  ///< printf calls infos
    std::vector<ROCmKernelMetadata> kernels;  ///< kernel metadatas
    
    /// initialize metadata info
    void initialize();
    /// parse metadata info from metadata string
    void parse(size_t metadataSize, const char* metadata);
};

/// ROCm main binary for GPU for 64-bit mode
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class ROCmBinary : public ElfBinary64, public NonCopyableAndNonMovable
{
public:
    /// region map type
    typedef Array<std::pair<CString, size_t> > RegionMap;
private:
    size_t regionsNum;
    std::unique_ptr<ROCmRegion[]> regions;  ///< AMD metadatas
    RegionMap regionsMap;
    size_t codeSize;
    cxbyte* code;
    size_t globalDataSize;
    cxbyte* globalData;
    CString target;
    size_t metadataSize;
    char* metadata;
    std::unique_ptr<ROCmMetadata> metadataInfo;
    RegionMap kernelInfosMap;
    Array<size_t> gotSymbols;
    bool newBinFormat;
public:
    /// constructor
    ROCmBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = ROCMBIN_CREATE_ALL);
    /// default destructor
    ~ROCmBinary() = default;
    
    /// determine GPU device type from this binary
    GPUDeviceType determineGPUDeviceType(uint32_t& archMinor,
                     uint32_t& archStepping) const;
    
    /// get regions number
    size_t getRegionsNum() const
    { return regionsNum; }
    
    /// get region by index
    const ROCmRegion& getRegion(size_t index) const
    { return regions[index]; }
    
    /// get region by name
    const ROCmRegion& getRegion(const char* name) const;
    
    /// get code size
    size_t getCodeSize() const
    { return codeSize; }
    /// get code
    const cxbyte* getCode() const
    { return code; }
    /// get code
    cxbyte* getCode()
    { return code; }
    
    /// get global data size
    size_t getGlobalDataSize() const
    { return globalDataSize; }
    
    /// get global data
    const cxbyte* getGlobalData() const
    { return globalData; }
    /// get global data
    cxbyte* getGlobalData()
    { return globalData; }
    
    /// get metadata size
    size_t getMetadataSize() const
    { return metadataSize; }
    /// get metadata
    const char* getMetadata() const
    { return metadata; }
    /// get metadata
    char* getMetadata()
    { return metadata; }
    
    /// has metadata info
    bool hasMetadataInfo() const
    { return metadataInfo!=nullptr; }
    
    /// get metadata info
    const ROCmMetadata& getMetadataInfo() const
    { return *metadataInfo; }
    
    /// get kernel metadata infos number
    size_t getKernelInfosNum() const
    { return metadataInfo->kernels.size(); }
    
    /// get kernel metadata info
    const ROCmKernelMetadata& getKernelInfo(size_t index) const
    { return metadataInfo->kernels[index]; }
    
    /// get kernel metadata info by name
    const ROCmKernelMetadata& getKernelInfo(const char* name) const;
    
    /// get target
    const CString& getTarget() const
    { return target; }
    
    /// return true is new binary format
    bool isNewBinaryFormat() const
    { return newBinFormat; }
    
    /// get GOT symbol index (from elfbin dynsymbols)
    size_t getGotSymbolsNum() const
    { return gotSymbols.size(); }
    
    /// get GOT symbols (indices) (from elfbin dynsymbols)
    const Array<size_t> getGotSymbols() const
    { return gotSymbols; }
    
    /// get GOT symbol index (from elfbin dynsymbols)
    size_t getGotSymbol(size_t index) const
    { return gotSymbols[index]; }
    
    /// returns true if kernel map exists
    bool hasRegionMap() const
    { return (creationFlags & ROCMBIN_CREATE_REGIONMAP) != 0; }
    /// returns true if object has kernel map
    bool hasKernelInfoMap() const
    { return (creationFlags & ROCMBIN_CREATE_KERNELINFOMAP) != 0; }
};

enum {
    ROCMFLAG_USE_PRIVATE_SEGMENT_BUFFER = AMDHSAFLAG_USE_PRIVATE_SEGMENT_BUFFER,
    ROCMFLAG_USE_DISPATCH_PTR = AMDHSAFLAG_USE_DISPATCH_PTR,
    ROCMFLAG_USE_QUEUE_PTR = AMDHSAFLAG_USE_QUEUE_PTR,
    ROCMFLAG_USE_KERNARG_SEGMENT_PTR = AMDHSAFLAG_USE_KERNARG_SEGMENT_PTR,
    ROCMFLAG_USE_DISPATCH_ID = AMDHSAFLAG_USE_DISPATCH_ID,
    ROCMFLAG_USE_FLAT_SCRATCH_INIT = AMDHSAFLAG_USE_FLAT_SCRATCH_INIT,
    ROCMFLAG_USE_PRIVATE_SEGMENT_SIZE = AMDHSAFLAG_USE_PRIVATE_SEGMENT_SIZE,
    ROCMFLAG_USE_GRID_WORKGROUP_COUNT_BIT = AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_BIT,
    ROCMFLAG_USE_GRID_WORKGROUP_COUNT_X = AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_X,
    ROCMFLAG_USE_GRID_WORKGROUP_COUNT_Y = AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Y,
    ROCMFLAG_USE_GRID_WORKGROUP_COUNT_Z = AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Z,
    
    ROCMFLAG_USE_ORDERED_APPEND_GDS = AMDHSAFLAG_USE_ORDERED_APPEND_GDS,
    ROCMFLAG_PRIVATE_ELEM_SIZE_BIT = AMDHSAFLAG_PRIVATE_ELEM_SIZE_BIT,
    ROCMFLAG_USE_PTR64 = AMDHSAFLAG_USE_PTR64,
    ROCMFLAG_USE_DYNAMIC_CALL_STACK = AMDHSAFLAG_USE_DYNAMIC_CALL_STACK,
    ROCMFLAG_USE_DEBUG_ENABLED = AMDHSAFLAG_USE_DEBUG_ENABLED,
    ROCMFLAG_USE_XNACK_ENABLED = AMDHSAFLAG_USE_XNACK_ENABLED
};

/// ROCm kernel configuration structure
typedef AmdHsaKernelConfig ROCmKernelConfig;

/// check whether is Amd OpenCL 2.0 binary
extern bool isROCmBinary(size_t binarySize, const cxbyte* binary);

/*
 * ROCm Binary Generator
 */

enum: cxuint {
    ROCMSECTID_HASH = ELFSECTID_OTHER_BUILTIN,
    ROCMSECTID_DYNAMIC,
    ROCMSECTID_NOTE,
    ROCMSECTID_GPUCONFIG,
    ROCMSECTID_RELADYN,
    ROCMSECTID_GOT,
    ROCMSECTID_MAX = ROCMSECTID_GOT
};

/// ROCm binary symbol input
struct ROCmSymbolInput
{
    CString symbolName; ///< symbol name
    size_t offset;  ///< offset in code
    size_t size;    ///< size of symbol
    ROCmRegionType type;  ///< type
};

/// ROCm binary input structure
struct ROCmInput
{
    GPUDeviceType deviceType;   ///< GPU device type
    uint32_t archMinor;         ///< GPU arch minor
    uint32_t archStepping;      ///< GPU arch stepping
    uint32_t eflags;    ///< ELF headef e_flags field
    bool newBinFormat;       ///< use new binary format for ROCm
    size_t globalDataSize;  ///< global data size
    const cxbyte* globalData;   ///< global data
    std::vector<ROCmSymbolInput> symbols;   ///< symbols
    size_t codeSize;        ///< code size
    const cxbyte* code;     ///< code
    size_t commentSize; ///< comment size (can be null)
    const char* comment; ///< comment
    CString target;     ///< LLVM target triple with device name
    CString targetTripple; ///< same LLVM target tripple
    size_t metadataSize;    ///< metadata size
    const char* metadata;   ///< metadata
    bool useMetadataInfo;   ///< use metadatainfo instead same metadata
    ROCmMetadata metadataInfo; ///< metadata info
    
    /// list of indices of symbols to GOT section
    /**  list of indices of symbols to GOT section.
     * If symbol index is lower than symbols.size(), then it refer to symbols
     * otherwise it refer to extraSymbols (index - symbols.size())
     */
    std::vector<size_t> gotSymbols;
    std::vector<BinSection> extraSections;  ///< extra sections
    std::vector<BinSymbol> extraSymbols;    ///< extra symbols
    
    /// add empty kernel with default values
    void addEmptyKernel(const char* kernelName);
};

/// ROCm binary generator
class ROCmBinGenerator: public NonCopyableAndNonMovable
{
private:
    private:
    bool manageable;
    const ROCmInput* input;
    std::unique_ptr<ElfBinaryGen64> elfBinGen64;
    size_t binarySize;
    size_t commentSize;
    const char* comment;
    std::string target;
    std::unique_ptr<cxbyte[]> noteBuf;
    std::string metadataStr;
    size_t metadataSize;
    const char* metadata;
    cxuint mainSectionsNum;
    uint16_t mainBuiltinSectTable[ROCMSECTID_MAX-ELFSECTID_START+1];
    void* rocmGotGen;
    void* rocmRelaDynGen;
    
    void generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr);
public:
    /// constructor
    ROCmBinGenerator();
    /// constructor with ROCm input
    explicit ROCmBinGenerator(const ROCmInput* rocmInput);
    
    /// constructor
    /**
     * \param deviceType device type
     * \param archMinor architecture minor number
     * \param archStepping architecture stepping number
     * \param codeSize size of code
     * \param code code pointer
     * \param globalDataSize size of global data
     * \param globalData global data pointer
     * \param symbols symbols (kernels, datas,...)
     */
    ROCmBinGenerator(GPUDeviceType deviceType, uint32_t archMinor, uint32_t archStepping,
            size_t codeSize, const cxbyte* code,
            size_t globalDataSize, const cxbyte* globalData,
            const std::vector<ROCmSymbolInput>& symbols);
    /// constructor
    ROCmBinGenerator(GPUDeviceType deviceType, uint32_t archMinor, uint32_t archStepping,
            size_t codeSize, const cxbyte* code,
            size_t globalDataSize, const cxbyte* globalData,
            std::vector<ROCmSymbolInput>&& symbols);
    /// destructor
    ~ROCmBinGenerator();
    
    /// get input
    const ROCmInput* getInput() const
    { return input; }
    /// set input
    void setInput(const ROCmInput* input);
    
    /// prepare binary generator (for section diffs)
    void prepareBinaryGen();
    /// get section offset (from main section)
    size_t getSectionOffset(cxuint sectionId) const
    { return elfBinGen64->getRegionOffset(
                    mainBuiltinSectTable[sectionId - ELFSECTID_START]); }
    /// update symbols
    void updateSymbols();
    
    /// generates binary to array of bytes
    void generate(Array<cxbyte>& array);
    
    /// generates binary to output stream
    void generate(std::ostream& os);
    
    /// generates binary to vector of char
    void generate(std::vector<char>& vector);
};

};

#endif
