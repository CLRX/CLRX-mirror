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
/*! \file AsmFormats.h
 * \brief an assembler formats
 */

#ifndef __CLRX_ASMFORMATS_H__
#define __CLRX_ASMFORMATS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/amdbin/AmdCL2BinGen.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Commons.h>

namespace CLRX
{

/// assembler section type
enum class AsmSectionType: cxbyte
{
    DATA = 0,       ///< kernel or global data
    CODE,           ///< code of program or kernel
    CONFIG,         ///< configuration (global or for kernel)
    LAST_COMMON = CONFIG,   ///< last common type
    
    AMD_HEADER = LAST_COMMON+1, ///< AMD Catalyst kernel's header
    AMD_METADATA,       ///< AMD Catalyst kernel's metadata
    AMD_CALNOTE,        ///< AMD CALNote
    
    AMDCL2_RWDATA = LAST_COMMON+1,
    AMDCL2_BSS,
    AMDCL2_SAMPLERINIT,
    AMDCL2_SETUP,
    AMDCL2_STUB,
    AMDCL2_METADATA,
    AMDCL2_ISAMETADATA,
    AMDCL2_CONFIG_CTRL_DIRECTIVE,
    AMDCL2_DUMMY,   ///< dummy (empty) section for kernel
    
    GALLIUM_COMMENT = LAST_COMMON+1,    ///< gallium comment section
    GALLIUM_CONFIG_CTRL_DIRECTIVE,
    GALLIUM_SCRATCH,    ///< empty section for scratch symbol
    
    ROCM_COMMENT = LAST_COMMON+1,        ///< ROCm comment section
    ROCM_CONFIG_CTRL_DIRECTIVE,
    ROCM_METADATA,
    ROCM_GOT,
    
    EXTRA_FIRST = 0xfc,
    EXTRA_PROGBITS = 0xfc,
    EXTRA_NOBITS = 0xfd,
    EXTRA_NOTE = 0xfe,
    EXTRA_SECTION = 0xff
};

enum: AsmSectionId
{
    ASMSECT_ABS = UINT_MAX,  ///< absolute section id
    ASMSECT_NONE = UINT_MAX,  ///< none section id
};

enum: AsmKernelId
{
    ASMKERN_GLOBAL = UINT_MAX, ///< no kernel, global space
    ASMKERN_INNER = UINT_MAX-1 ///< no kernel, inner global space
};

enum: Flags
{
    ASMSECT_WRITEABLE = 1,
    ASMSECT_ADDRESSABLE = 2,
    ASMSECT_ABS_ADDRESSABLE = 4,
    ASMSECT_UNRESOLVABLE = 8,   ///< section is unresolvable
    
    ASMELFSECT_ALLOCATABLE = 0x10,
    ASMELFSECT_WRITEABLE = 0x20,
    ASMELFSECT_EXECUTABLE = 0x40
};

class Assembler;
class AsmExpression;
struct AsmRelocation;
struct AsmSymbol;

/// assembler format exception
class AsmFormatException: public Exception
{
public:
    /// default constructor
    AsmFormatException() = default;
    /// constructor from message
    explicit AsmFormatException(const std::string& message);
    /// destructor
    virtual ~AsmFormatException() noexcept = default;
};

/// assembler format handler
class AsmFormatHandler: public NonCopyableAndNonMovable
{
public:
    /// section information
    struct SectionInfo
    {
        const char* name;   ///< section name
        AsmSectionType type;    ///< section type
        Flags flags;        ///< section flags
        cxuint relSpace;    ///< relative space
        
        SectionInfo() : name(nullptr), type(AsmSectionType::DATA), flags(0),
                    relSpace(UINT_MAX)
        { }
        SectionInfo(const char* _name, AsmSectionType _type, Flags _flags = 0,
                cxuint _relSpace = UINT_MAX) : name(_name), type(_type), flags(_flags),
                    relSpace(_relSpace)
        { }
    };
    
    struct KernelBase
    {
        cxuint allocRegs[MAX_REGTYPES_NUM];
        Flags allocRegFlags;
    };
protected:
    Assembler& assembler;   ///< assembler reference
    bool sectionDiffsResolvable;
    
    /// constructor
    explicit AsmFormatHandler(Assembler& assembler);
    
    // resolve LO32BIT/HI32BIT relocations (partially, helper)
    bool resolveLoHiRelocExpression(const AsmExpression* expr, RelocType& relType,
                    AsmSectionId& relSectionId, uint64_t& relValue);
public:
    virtual ~AsmFormatHandler();
    
    /// return true if format handler can resolve differences between sections
    bool isSectionDiffsResolvable() const
    { return sectionDiffsResolvable; }
    
    /// add/set kernel
    /** adds new kernel. throw AsmFormatException when addition failed.
     * Method should handles any constraint that doesn't allow to add kernel except
     * duplicate of name.
     * \param kernelName kernel name
     * \return kernel id
     */
    virtual AsmKernelId addKernel(const char* kernelName) = 0;
    /// add section
    /** adds new section . throw AsmFormatException when addition failed.
     * Method should handles any constraint that doesn't allow to add section except
     * duplicate of name.
     * \param sectionName section name
     * \param kernelId kernel id (may ASMKERN_GLOBAL)
     * \return section id
     */
    virtual AsmSectionId addSection(const char* sectionName, AsmKernelId kernelId) = 0;
    
    /// get section id if exists in current context, otherwise returns ASMSECT_NONE
    virtual AsmSectionId getSectionId(const char* sectionName) const = 0;
    
    /// set current kernel
    virtual void setCurrentKernel(AsmKernelId kernel) = 0;
    /// set current section, this method can change current kernel if that required 
    virtual void setCurrentSection(AsmSectionId sectionId) = 0;
    
    /// get current section flags and type
    virtual SectionInfo getSectionInfo(AsmSectionId sectionId) const = 0;
    /// parse pseudo-op (return true if recognized pseudo-op)
    virtual bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr) = 0;
    /// handle labels
    virtual void handleLabel(const CString& label);
    /// resolve symbol if needed (for example that comes from unresolvable sections)
    virtual bool resolveSymbol(const AsmSymbol& symbol,
               uint64_t& value, AsmSectionId& sectionId);
    /// resolve relocation for specified expression
    virtual bool resolveRelocation(const AsmExpression* expr,
                uint64_t& value, AsmSectionId& sectionId);
    /// prepare binary for use
    virtual bool prepareBinary() = 0;
    /// write binary to output stream
    virtual void writeBinary(std::ostream& os) const = 0;
    /// write binary to array
    virtual void writeBinary(Array<cxbyte>& array) const = 0;
    
    /// prepare before section diference resolving
    virtual bool prepareSectionDiffsResolving();
};

/// format handler with Kcode (kernel-code) handling
class AsmKcodeHandler: public AsmFormatHandler
{
protected:
    friend struct AsmKcodePseudoOps;
    std::vector<AsmKernelId> kcodeSelection; // kcode
    std::stack<std::vector<AsmKernelId> > kcodeSelStack;
    AsmKernelId currentKcodeKernel;
    AsmSectionId codeSection;
    
    explicit AsmKcodeHandler(Assembler& assembler);
    ~AsmKcodeHandler() = default;
    
    void restoreKcodeCurrentAllocRegs();
    void saveKcodeCurrentAllocRegs();
    // prepare kcode state while preparing binary
    void prepareKcodeState();
public:
    void handleLabel(const CString& label);
    
    /// return true if current section is code section
    virtual bool isCodeSection() const = 0;
    /// return KernelBase for kernel index
    virtual KernelBase& getKernelBase(AsmKernelId index) = 0;
    /// return kernel number
    virtual size_t getKernelsNum() const = 0;
};

/// handles raw code format
class AsmRawCodeHandler: public AsmFormatHandler
{
public:
    /// constructor
    explicit AsmRawCodeHandler(Assembler& assembler);
    /// destructor
    ~AsmRawCodeHandler() = default;
    
    AsmKernelId addKernel(const char* kernelName);
    AsmSectionId addSection(const char* sectionName, AsmKernelId kernelId);

    AsmSectionId getSectionId(const char* sectionName) const;
    
    void setCurrentKernel(AsmKernelId kernel);
    void setCurrentSection(AsmSectionId sectionId);
    
    SectionInfo getSectionInfo(AsmSectionId sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
};

/// handles AMD Catalyst format
class AsmAmdHandler: public AsmFormatHandler
{
private:
    typedef std::unordered_map<CString, AsmSectionId> SectionMap;
    friend struct AsmAmdPseudoOps;
    AmdInput output;
    struct Section
    {
        AsmKernelId kernelId;
        AsmSectionType type;
        AsmSectionId elfBinSectId;
        const char* name;
        uint32_t extraId; // for example CALNote id
    };
    struct Kernel : KernelBase
    {
        AsmSectionId headerSection;
        AsmSectionId metadataSection;
        AsmSectionId configSection;
        AsmSectionId codeSection;
        AsmSectionId dataSection;
        std::vector<AsmSectionId> calNoteSections;
        SectionMap extraSectionMap;
        AsmSectionId extraSectionCount;
        AsmSectionId savedSection;
        std::unordered_set<CString> argNamesSet;
        
        explicit Kernel(AsmSectionId _codeSection = ASMSECT_NONE) : KernelBase{},
                headerSection(ASMSECT_NONE), metadataSection(ASMSECT_NONE),
                configSection(ASMSECT_NONE), codeSection(_codeSection),
                dataSection(ASMSECT_NONE), extraSectionCount(0),
                savedSection(ASMSECT_NONE)
        { }
    };
    std::vector<Section> sections;
    // use pointer to prevents copying Kernel objects
    std::vector<Kernel*> kernelStates;
    SectionMap extraSectionMap;
    AsmSectionId dataSection; // global
    AsmSectionId savedSection;
    AsmSectionId extraSectionCount;
    
    cxuint detectedDriverVersion;
    
    void saveCurrentSection();
    void restoreCurrentAllocRegs();
    void saveCurrentAllocRegs();
    
    cxuint determineDriverVersion() const;
public:
    /// constructor
    explicit AsmAmdHandler(Assembler& assembler);
    /// destructor
    ~AsmAmdHandler();
    
    AsmKernelId addKernel(const char* kernelName);
    AsmSectionId addSection(const char* sectionName, AsmKernelId kernelId);
    
    AsmSectionId getSectionId(const char* sectionName) const;
    void setCurrentKernel(AsmKernelId kernel);
    void setCurrentSection(AsmSectionId sectionId);
    
    SectionInfo getSectionInfo(AsmSectionId sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
    /// get output structure pointer
    const AmdInput* getOutput() const
    { return &output; }
};

/// Asm AMD HSA kernel configuration
struct AsmAmdHsaKernelConfig: AmdHsaKernelConfig
{
    cxuint dimMask;    ///< mask of dimension (bits: 0 - X, 1 - Y, 2 - Z)
    cxuint usedVGPRsNum;  ///< number of used VGPRs
    cxuint usedSGPRsNum;  ///< number of used SGPRs
    cxbyte userDataNum;   ///< number of user data
    bool ieeeMode;  ///< IEEE mode
    cxbyte floatMode; ///< float mode
    cxbyte priority;    ///< priority
    cxbyte exceptions;      ///< enabled exceptions
    bool tgSize;        ///< enable TG_SIZE_EN bit
    bool debugMode;     ///< debug mode
    bool privilegedMode;   ///< prvileged mode
    bool dx10Clamp;     ///< DX10 CLAMP mode
    
    void initialize();
};

/// handles AMD OpenCL 2.0 binary format
class AsmAmdCL2Handler: public AsmKcodeHandler
{
private:
    typedef std::unordered_map<CString, AsmSectionId> SectionMap;
    friend struct AsmAmdCL2PseudoOps;
    AmdCL2Input output;
    struct Section
    {
        AsmKernelId kernelId;
        AsmSectionType type;
        AsmSectionId elfBinSectId;
        const char* name;
        uint32_t extraId;
    };
    struct Relocation
    {
        RelocType type;
        cxuint symbol;  // 0,1,2
        size_t addend;
    };
    /* relocmap: key - symbol, value - relocation */
    typedef std::unordered_map<CString, Relocation> RelocMap;
    struct Kernel : KernelBase
    {
        AsmSectionId stubSection;
        AsmSectionId setupSection;
        AsmSectionId metadataSection;
        AsmSectionId isaMetadataSection;
        AsmSectionId configSection;
        AsmSectionId ctrlDirSection;
        AsmSectionId codeSection;
        AsmSectionId savedSection;
        bool useHsaConfig; // 
        std::unique_ptr<AsmAmdHsaKernelConfig> hsaConfig; // hsaConfig
        std::unordered_set<CString> argNamesSet;
        
        explicit Kernel(AsmSectionId _codeSection = ASMSECT_NONE) : KernelBase{},
            stubSection(ASMSECT_NONE), setupSection(ASMSECT_NONE),
            metadataSection(ASMSECT_NONE), isaMetadataSection(ASMSECT_NONE),
            configSection(ASMSECT_NONE), ctrlDirSection(ASMSECT_NONE),
            codeSection(_codeSection), savedSection(ASMSECT_NONE),
            useHsaConfig(false)
        { }
        
        void initializeKernelConfig();
    };
    std::vector<Section> sections;
    // use pointer to prevents copying Kernel objects
    std::vector<Kernel*> kernelStates;
    RelocMap relocsMap;
    SectionMap extraSectionMap;
    SectionMap innerExtraSectionMap;
    AsmSectionId rodataSection; // global inner
    AsmSectionId dataSection; // global inner
    AsmSectionId bssSection; // global inner
    AsmSectionId samplerInitSection;
    AsmSectionId savedSection;
    AsmSectionId innerSavedSection;
    AsmSectionId extraSectionCount;
    AsmSectionId innerExtraSectionCount;
    bool hsaLayout;
    
    cxuint detectedDriverVersion;
    
    void saveCurrentSection();
    void restoreCurrentAllocRegs();
    void saveCurrentAllocRegs();
    cxuint getDriverVersion() const;
public:
    /// constructor
    explicit AsmAmdCL2Handler(Assembler& assembler);
    /// destructor
    ~AsmAmdCL2Handler();
    
    AsmKernelId addKernel(const char* kernelName);
    AsmSectionId addSection(const char* sectionName, AsmKernelId kernelId);
    
    AsmSectionId getSectionId(const char* sectionName) const;
    void setCurrentKernel(AsmKernelId kernel);
    void setCurrentSection(AsmSectionId sectionId);
    
    SectionInfo getSectionInfo(AsmSectionId sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    
    bool resolveSymbol(const AsmSymbol& symbol, uint64_t& value, AsmSectionId& sectionId);
    bool resolveRelocation(const AsmExpression* expr, uint64_t& value,
                           AsmSectionId& sectionId);
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
    /// get output structure pointer
    const AmdCL2Input* getOutput() const
    { return &output; }
    
    // kcode support
    bool isCodeSection() const;
    KernelBase& getKernelBase(AsmKernelId index);
    size_t getKernelsNum() const;
    void handleLabel(const CString& label);
};

/// handles GalliumCompute format
class AsmGalliumHandler: public AsmKcodeHandler
{
private:
    enum class Inside : cxbyte {
        MAINLAYOUT, CONFIG, ARGS, PROGINFO
    };
    
    typedef std::unordered_map<CString, AsmSectionId> SectionMap;
    friend struct AsmGalliumPseudoOps;
    GalliumInput output;
    struct Section
    {
        AsmKernelId kernelId;
        AsmSectionType type;
        AsmSectionId elfBinSectId;
        const char* name;    // must be available by whole lifecycle
    };
    struct Kernel : KernelBase
    {
        AsmSectionId defaultSection;
        std::unique_ptr<AsmAmdHsaKernelConfig> hsaConfig;
        AsmSectionId ctrlDirSection;
        bool hasProgInfo;
        cxbyte progInfoEntries;
        
        explicit Kernel(AsmSectionId _defaultSection = ASMSECT_NONE) : KernelBase{},
                defaultSection(_defaultSection), hsaConfig(nullptr),
                ctrlDirSection(ASMSECT_NONE), hasProgInfo(false), progInfoEntries(0)
        { }
        
        void initializeAmdHsaKernelConfig();
    };
    std::vector<Kernel*> kernelStates;
    std::vector<Section> sections;
    SectionMap extraSectionMap;
    AsmSectionId dataSection;
    AsmSectionId commentSection;
    AsmSectionId scratchSection;
    AsmSectionId savedSection;
    Inside inside;
    AsmSectionId extraSectionCount;
    
    cxuint detectedDriverVersion;
    cxuint detectedLLVMVersion;
    
    uint32_t archMinor;
    uint32_t archStepping;
    
    cxuint determineDriverVersion() const;
    cxuint determineLLVMVersion() const;
public:
    /// construcror
    explicit AsmGalliumHandler(Assembler& assembler);
    /// destructor
    ~AsmGalliumHandler();
    
    AsmKernelId addKernel(const char* kernelName);
    AsmSectionId addSection(const char* sectionName, AsmKernelId kernelId);
    
    AsmSectionId getSectionId(const char* sectionName) const;
    void setCurrentKernel(AsmKernelId kernel);
    void setCurrentSection(AsmSectionId sectionId);
    
    SectionInfo getSectionInfo(AsmSectionId sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    
    bool resolveSymbol(const AsmSymbol& symbol, uint64_t& value, AsmSectionId& sectionId);
    bool resolveRelocation(const AsmExpression* expr, uint64_t& value,
                    AsmSectionId& sectionId);
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
    /// get output object (input for bingenerator)
    const GalliumInput* getOutput() const
    { return &output; }
    
    // kcode support
    bool isCodeSection() const;
    KernelBase& getKernelBase(AsmKernelId index);
    size_t getKernelsNum() const;
};

typedef AsmAmdHsaKernelConfig AsmROCmKernelConfig;

/// handles ROCM binary format
class AsmROCmHandler: public AsmKcodeHandler
{
private:
    typedef std::unordered_map<CString, AsmSectionId> SectionMap;
    friend struct AsmROCmPseudoOps;
    ROCmInput output;
    std::unique_ptr<ROCmBinGenerator> binGen;
    struct Section
    {
        AsmKernelId kernelId;
        AsmSectionType type;
        AsmSectionId elfBinSectId;
        const char* name;    // must be available by whole lifecycle
    };
    struct Kernel : KernelBase
    {
        AsmSectionId configSection;
        std::unique_ptr<AsmROCmKernelConfig> config;
        bool isFKernel;
        AsmSectionId ctrlDirSection;
        AsmSectionId savedSection;
        
        explicit Kernel(AsmSectionId _configSection = ASMSECT_NONE): KernelBase{},
                configSection(_configSection), config(nullptr), isFKernel(false),
                ctrlDirSection(ASMSECT_NONE), savedSection(ASMSECT_NONE)
        { }
        
        void initializeKernelConfig();
    };
    std::vector<Kernel*> kernelStates;
    std::vector<Section> sections;
    std::vector<CString> gotSymbols;
    SectionMap extraSectionMap;
    AsmSectionId commentSection;
    AsmSectionId metadataSection;
    AsmSectionId dataSection;
    AsmSectionId gotSection;
    AsmSectionId savedSection;
    AsmSectionId extraSectionCount;
    
    size_t prevSymbolsCount;
    
    bool unresolvedGlobals;
    bool good;
    
    void addSymbols(bool sectionDiffsPrepared);
public:
    /// construcror
    explicit AsmROCmHandler(Assembler& assembler);
    /// destructor
    ~AsmROCmHandler();
    
    AsmKernelId addKernel(const char* kernelName);
    AsmSectionId addSection(const char* sectionName, AsmKernelId kernelId);
    
    AsmSectionId getSectionId(const char* sectionName) const;
    void setCurrentKernel(AsmKernelId kernel);
    void setCurrentSection(AsmSectionId sectionId);
    
    SectionInfo getSectionInfo(AsmSectionId sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
    /// get output object (input for bingenerator)
    const ROCmInput* getOutput() const
    { return &output; }
    
    bool prepareSectionDiffsResolving();
    
    // kcode support
    bool isCodeSection() const;
    KernelBase& getKernelBase(AsmKernelId index);
    size_t getKernelsNum() const;
};

};

#endif
