/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
#include <unordered_set>
#include <unordered_map>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/amdbin/AmdCL2BinGen.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include <CLRX/utils/Utilities.h>
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
    
    GALLIUM_COMMENT = LAST_COMMON+1,    ///< gallium comment section
    
    ROCM_COMMENT = LAST_COMMON+1,        ///< ROCm comment section
    ROCM_CONFIG_CTRL_DIRECTIVE,
    
    EXTRA_FIRST = 0xfc,
    EXTRA_PROGBITS = 0xfc,
    EXTRA_NOBITS = 0xfd,
    EXTRA_NOTE = 0xfe,
    EXTRA_SECTION = 0xff
};

enum: cxuint
{
    ASMSECT_ABS = UINT_MAX,  ///< absolute section id
    ASMSECT_NONE = UINT_MAX,  ///< none section id
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

/// assdembler format handler
class AsmFormatHandler: public NonCopyableAndNonMovable
{
public:
    /// section information
    struct SectionInfo
    {
        const char* name;   ///< section name
        AsmSectionType type;    ///< section type
        Flags flags;        ///< section flags
    };
protected:
    Assembler& assembler;   ///< assembler reference
    
    /// constructor
    explicit AsmFormatHandler(Assembler& assembler);
public:
    virtual ~AsmFormatHandler();
    
    /// add/set kernel
    /** adds new kernel. throw AsmFormatException when addition failed.
     * Method should handles any constraint that doesn't allow to add kernel except
     * duplicate of name.
     * \param kernelName kernel name
     * \return kernel id
     */
    virtual cxuint addKernel(const char* kernelName) = 0;
    /// add section
    /** adds new section . throw AsmFormatException when addition failed.
     * Method should handles any constraint that doesn't allow to add section except
     * duplicate of name.
     * \param sectionName section name
     * \param kernelId kernel id (may ASMKERN_GLOBAL)
     * \return section id
     */
    virtual cxuint addSection(const char* sectionName, cxuint kernelId) = 0;
    
    /// get section id if exists in current context, otherwise returns ASMSECT_NONE
    virtual cxuint getSectionId(const char* sectionName) const = 0;
    
    /// set current kernel
    virtual void setCurrentKernel(cxuint kernel) = 0;
    /// set current section, this method can change current kernel if that required 
    virtual void setCurrentSection(cxuint sectionId) = 0;
    
    /// get current section flags and type
    virtual SectionInfo getSectionInfo(cxuint sectionId) const = 0;
    /// parse pseudo-op (return true if recognized pseudo-op)
    virtual bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr) = 0;
    /// handle labels
    virtual void handleLabel(const CString& label);
    /// resolve symbol if needed (for example that comes from unresolvable sections)
    virtual bool resolveSymbol(const AsmSymbol& symbol,
               uint64_t& value, cxuint& sectionId);
    /// resolve relocation for specified expression
    virtual bool resolveRelocation(const AsmExpression* expr,
                uint64_t& value, cxuint& sectionId);
    /// prepare binary for use
    virtual bool prepareBinary() = 0;
    /// write binary to output stream
    virtual void writeBinary(std::ostream& os) const = 0;
    /// write binary to array
    virtual void writeBinary(Array<cxbyte>& array) const = 0;
};

/// handles raw code format
class AsmRawCodeHandler: public AsmFormatHandler
{
public:
    /// constructor
    explicit AsmRawCodeHandler(Assembler& assembler);
    /// destructor
    ~AsmRawCodeHandler() = default;
    
    cxuint addKernel(const char* kernelName);
    cxuint addSection(const char* sectionName, cxuint kernelId);

    cxuint getSectionId(const char* sectionName) const;
    
    void setCurrentKernel(cxuint kernel);
    void setCurrentSection(cxuint sectionId);
    
    SectionInfo getSectionInfo(cxuint sectionId) const;
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
    typedef std::unordered_map<CString, cxuint> SectionMap;
    friend struct AsmAmdPseudoOps;
    AmdInput output;
    struct Section
    {
        cxuint kernelId;
        AsmSectionType type;
        cxuint elfBinSectId;
        const char* name;
        uint32_t extraId; // for example CALNote id
    };
    struct Kernel
    {
        cxuint headerSection;
        cxuint metadataSection;
        cxuint configSection;
        cxuint codeSection;
        cxuint dataSection;
        std::vector<cxuint> calNoteSections;
        SectionMap extraSectionMap;
        cxuint extraSectionCount;
        cxuint savedSection;
        std::unordered_set<CString> argNamesSet;
        cxuint allocRegs[2];
        Flags allocRegFlags;
    };
    std::vector<Section> sections;
    // use pointer to prevents copying Kernel objects
    std::vector<Kernel*> kernelStates;
    SectionMap extraSectionMap;
    cxuint dataSection; // global
    cxuint savedSection;
    cxuint extraSectionCount;
    
    void saveCurrentSection();
    void restoreCurrentAllocRegs();
    void saveCurrentAllocRegs();
public:
    /// constructor
    explicit AsmAmdHandler(Assembler& assembler);
    /// destructor
    ~AsmAmdHandler();
    
    cxuint addKernel(const char* kernelName);
    cxuint addSection(const char* sectionName, cxuint kernelId);
    
    cxuint getSectionId(const char* sectionName) const;
    void setCurrentKernel(cxuint kernel);
    void setCurrentSection(cxuint sectionId);
    
    SectionInfo getSectionInfo(cxuint sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
    /// get output structure pointer
    const AmdInput* getOutput() const
    { return &output; }
};

/// handles AMD OpenCL 2.0 binary format
class AsmAmdCL2Handler: public AsmFormatHandler
{
private:
    typedef std::unordered_map<CString, cxuint> SectionMap;
    friend struct AsmAmdCL2PseudoOps;
    AmdCL2Input output;
    struct Section
    {
        cxuint kernelId;
        AsmSectionType type;
        cxuint elfBinSectId;
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
    struct Kernel
    {
        cxuint stubSection;
        cxuint setupSection;
        cxuint metadataSection;
        cxuint isaMetadataSection;
        cxuint configSection;
        cxuint codeSection;
        cxuint savedSection;
        std::unordered_set<CString> argNamesSet;
        cxuint allocRegs[2];
        Flags allocRegFlags;
    };
    std::vector<Section> sections;
    // use pointer to prevents copying Kernel objects
    std::vector<Kernel*> kernelStates;
    RelocMap relocsMap;
    SectionMap extraSectionMap;
    SectionMap innerExtraSectionMap;
    cxuint rodataSection; // global inner
    cxuint dataSection; // global inner
    cxuint bssSection; // global inner
    cxuint samplerInitSection;
    cxuint savedSection;
    cxuint innerSavedSection;
    cxuint extraSectionCount;
    cxuint innerExtraSectionCount;
    
    void saveCurrentSection();
    void restoreCurrentAllocRegs();
    void saveCurrentAllocRegs();
    cxuint getDriverVersion() const;
public:
    /// constructor
    explicit AsmAmdCL2Handler(Assembler& assembler);
    /// destructor
    ~AsmAmdCL2Handler();
    
    cxuint addKernel(const char* kernelName);
    cxuint addSection(const char* sectionName, cxuint kernelId);
    
    cxuint getSectionId(const char* sectionName) const;
    void setCurrentKernel(cxuint kernel);
    void setCurrentSection(cxuint sectionId);
    
    SectionInfo getSectionInfo(cxuint sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    
    bool resolveSymbol(const AsmSymbol& symbol, uint64_t& value, cxuint& sectionId);
    bool resolveRelocation(const AsmExpression* expr, uint64_t& value, cxuint& sectionId);
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
    /// get output structure pointer
    const AmdCL2Input* getOutput() const
    { return &output; }
};

/// handles GalliumCompute format
class AsmGalliumHandler: public AsmFormatHandler
{
private:
    enum class Inside : cxbyte {
        MAINLAYOUT, CONFIG, ARGS, PROGINFO
    };
    
    typedef std::unordered_map<CString, cxuint> SectionMap;
    friend struct AsmGalliumPseudoOps;
    GalliumInput output;
    struct Section
    {
        cxuint kernelId;
        AsmSectionType type;
        cxuint elfBinSectId;
        const char* name;    // must be available by whole lifecycle
    };
    struct Kernel
    {
        cxuint defaultSection;
        bool hasProgInfo;
        cxbyte progInfoEntries;
        cxuint allocRegs[2];
        Flags allocRegFlags;
    };
    std::vector<Kernel> kernelStates;
    std::vector<Section> sections;
    std::vector<cxuint> kcodeSelection; // kcode
    std::stack<std::vector<cxuint> > kcodeSelStack;
    cxuint currentKcodeKernel;
    SectionMap extraSectionMap;
    cxuint codeSection;
    cxuint dataSection;
    cxuint commentSection;
    cxuint savedSection;
    Inside inside;
    cxuint extraSectionCount;
    
    void restoreKcodeCurrentAllocRegs();
    void saveKcodeCurrentAllocRegs();
public:
    /// construcror
    explicit AsmGalliumHandler(Assembler& assembler);
    /// destructor
    ~AsmGalliumHandler() = default;
    
    cxuint addKernel(const char* kernelName);
    cxuint addSection(const char* sectionName, cxuint kernelId);
    
    cxuint getSectionId(const char* sectionName) const;
    void setCurrentKernel(cxuint kernel);
    void setCurrentSection(cxuint sectionId);
    
    SectionInfo getSectionInfo(cxuint sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    void handleLabel(const CString& label);
    
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
    /// get output object (input for bingenerator)
    const GalliumInput* getOutput() const
    { return &output; }
};

struct AsmROCmKernelConfig: ROCmKernelConfig
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
};

/// handles ROCM binary format
class AsmROCmHandler: public AsmFormatHandler
{
private:
    typedef std::unordered_map<CString, cxuint> SectionMap;
    friend struct AsmROCmPseudoOps;
    ROCmInput output;
    struct Section
    {
        cxuint kernelId;
        AsmSectionType type;
        cxuint elfBinSectId;
        const char* name;    // must be available by whole lifecycle
    };
    struct Kernel
    {
        cxuint configSection;
        std::unique_ptr<AsmROCmKernelConfig> config;
        bool isFKernel;
        cxuint ctrlDirSection;
        cxuint savedSection;
        Flags allocRegFlags;
        cxuint allocRegs[2];
        
        void initializeKernelConfig();
    };
    std::vector<Kernel*> kernelStates;
    std::vector<Section> sections;
    std::vector<cxuint> kcodeSelection; // kcode
    std::stack<std::vector<cxuint> > kcodeSelStack;
    cxuint currentKcodeKernel;
    SectionMap extraSectionMap;
    cxuint codeSection;
    cxuint commentSection;
    cxuint savedSection;
    cxuint extraSectionCount;
    
    void restoreKcodeCurrentAllocRegs();
    void saveKcodeCurrentAllocRegs();
    
    
public:
    /// construcror
    explicit AsmROCmHandler(Assembler& assembler);
    /// destructor
    ~AsmROCmHandler();
    
    cxuint addKernel(const char* kernelName);
    cxuint addSection(const char* sectionName, cxuint kernelId);
    
    cxuint getSectionId(const char* sectionName) const;
    void setCurrentKernel(cxuint kernel);
    void setCurrentSection(cxuint sectionId);
    
    SectionInfo getSectionInfo(cxuint sectionId) const;
    bool parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr);
    void handleLabel(const CString& label);
    
    bool prepareBinary();
    void writeBinary(std::ostream& os) const;
    void writeBinary(Array<cxbyte>& array) const;
    /// get output object (input for bingenerator)
    const ROCmInput* getOutput() const
    { return &output; }
};

};

#endif
