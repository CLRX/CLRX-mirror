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

#include <CLRX/Config.h>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* galliumPseudoOpNamesTbl[] =
{ "arg", "args", "entry", "proginfo" };

enum
{ GALLIUMOP_ARG, GALLIUMOP_ARGS, GALLIUMOP_ENTRY, GALLIUM_PROGINFO };

AsmFormatException::AsmFormatException(const std::string& message) : Exception(message)
{ }

AsmFormatHandler::AsmFormatHandler(Assembler& _assembler) : assembler(_assembler)
{ }

AsmFormatHandler::~AsmFormatHandler()
{ }

/* raw code handler */

AsmRawCodeHandler::AsmRawCodeHandler(Assembler& assembler): AsmFormatHandler(assembler)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
}

cxuint AsmRawCodeHandler::addKernel(const char* kernelName)
{
    throw AsmFormatException("In rawcode defining kernels is not allowed");
}

cxuint AsmRawCodeHandler::addSection(const char* name, cxuint kernelId)
{
    if (::strcmp(name, ".text")!=0)
        throw AsmFormatException("Only section '.text' can be in raw code");
    return 0;
}

cxuint AsmRawCodeHandler::getSectionId(const char* sectionName) const
{
    return ::strcmp(sectionName, ".text") ? ASMSECT_NONE : 0;
}

void AsmRawCodeHandler::setCurrentKernel(cxuint kernel)
{
    if (kernel != ASMKERN_GLOBAL)
        throw AsmFormatException("No kernels available");
}

void AsmRawCodeHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId!=0)
        throw AsmFormatException("Section doesn't exists");
}

AsmFormatHandler::SectionInfo AsmRawCodeHandler::getSectionInfo(cxuint sectionId) const
{
    if (sectionId >= 1)
        throw AsmFormatException("Section doesn't exists");
    return { ".text", AsmSectionType::CODE, ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE };
}

void AsmRawCodeHandler::parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char* linePtr)
{ }  // not recognized any pseudo-op

bool AsmRawCodeHandler::prepareBinary()
{ return true; }

void AsmRawCodeHandler::writeBinary(std::ostream& os) const
{
    const AsmSection& section = assembler.getSections()[0];
    if (!section.content.empty())
        os.write((char*)section.content.data(), section.content.size());
}

void AsmRawCodeHandler::writeBinary(Array<cxbyte>& array) const
{
    const AsmSection& section = assembler.getSections()[0];
    array.assign(section.content.begin(), section.content.end());
}

/*
 * AmdCatalyst format handler
 */

AsmAmdHandler::AsmAmdHandler(Assembler& assembler) : AsmFormatHandler(assembler),
                output{}, dataSection(0)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::DATA });
    savedSection = 0;
}

cxuint AsmAmdHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = output.kernels.size();
    cxuint thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    kernelStates.push_back({ ASMSECT_NONE, ASMSECT_NONE, ASMSECT_NONE,
            thisSection, ASMSECT_NONE, { } });
    sections.push_back({ thisKernel, AsmSectionType::CODE, ELFSECTID_TEXT, ".text" });
    /// save previous section
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentSection].savedSection = assembler.currentSection;
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    return thisSection;
}

cxuint AsmAmdHandler::addSection(const char* sectionName, cxuint kernelId)
{
    const cxuint thisSection = sections.size();
    Section section;
    section.kernelId = kernelId;
    if (::strcmp(sectionName, ".data") == 0) // data
    {
        if (kernelId == ASMKERN_GLOBAL)
            throw AsmFormatException("Section '.data' permitted only inside kernels");
        Kernel& kernelState = kernelStates[kernelId];
        kernelState.codeSection = thisSection;
        section.type = AsmSectionType::CODE;
        section.elfBinSectId = ELFSECTID_TEXT;
        section.name = ".text"; // set static name (available by whole lifecycle)*/
    }
    else if (::strcmp(sectionName, ".text") == 0) // code
    {
        if (kernelId == ASMKERN_GLOBAL)
            throw AsmFormatException("Section '.text' permitted only inside kernels");
        Kernel& kernelState = kernelStates[kernelId];
        kernelState.codeSection = thisSection;
        section.type = AsmSectionType::CODE;
        section.elfBinSectId = ELFSECTID_TEXT;
        section.name = ".text"; // set static name (available by whole lifecycle)*/
    }
    else if (kernelId == ASMKERN_GLOBAL)
    {
        auto out = extraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section  is already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    else
    {   /* inside kernel binary */
        Kernel& kernelState = kernelStates[kernelId];
        auto out = kernelState.extraSectionMap.insert(std::make_pair(
                    std::string(sectionName), thisSection));
        if (!out.second)
            throw AsmFormatException("Section  is already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = kernelState.extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    sections.push_back(section);
    
    /// save previous section
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentSection].savedSection = assembler.currentSection;
    
    assembler.currentKernel = kernelId;
    assembler.currentSection = thisSection;
    return thisSection;
}

cxuint AsmAmdHandler::getSectionId(const char* sectionName) const
{
    if (assembler.currentKernel == ASMKERN_GLOBAL)
    {
        SectionMap::const_iterator it = extraSectionMap.find(sectionName);
        if (it != extraSectionMap.end())
            return it->second;
        return ASMSECT_NONE;
    }
    else
    {
        const Kernel& kernelState = kernelStates[assembler.currentKernel];
        if (::strcmp(sectionName, ".text") == 0)
            return kernelState.codeSection;
        else if (::strcmp(sectionName, ".data") == 0)
            return kernelState.dataSection;
        else
        {
            SectionMap::const_iterator it = kernelState.extraSectionMap.find(sectionName);
            if (it != extraSectionMap.end())
                return it->second;
        }
        return ASMSECT_NONE;
    }
}

void AsmAmdHandler::setCurrentKernel(cxuint kernel)
{
    assembler.currentKernel = kernel;
    /// save previous section
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentSection].savedSection = assembler.currentSection;
    
    if (kernel != ASMKERN_GLOBAL)
        assembler.currentSection = kernelStates[kernel].savedSection;
    else
        assembler.currentSection = savedSection;
}

void AsmAmdHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    /// save previous section
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentSection].savedSection = assembler.currentSection;
    
    assembler.currentKernel = sections[sectionId].kernelId;
    assembler.currentSection = sectionId;
}

AsmFormatHandler::SectionInfo AsmAmdHandler::getSectionInfo(cxuint sectionId) const
{   /* find section */
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    const char* name = nullptr;
    const Section& section = sections[sectionId];
    Flags flags = 0;
    switch (section.type)
    {
        case AsmSectionType::CODE:
            name = ".text";
            flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
            break;
        case AsmSectionType::DATA:
            name = (section.kernelId != ASMKERN_GLOBAL) ? ".data" : nullptr;
            flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::CONFIG:
            name = ".config";
            break;
        case AsmSectionType::AMD_HEADER:
            flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::AMD_METADATA:
            flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::AMD_CALNOTE:
            flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::EXTRA_SECTION:
            flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        default:
            // error
            break;
    }
    return { name, section.type, flags };
}

void AsmAmdHandler::parsePseudoOp(const std::string& firstName,
       const char* stmtPlace, const char* linePtr)
{
}

bool AsmAmdHandler::prepareBinary()
{ return true; }

void AsmAmdHandler::writeBinary(std::ostream& os) const
{
    AmdGPUBinGenerator binGenerator(&output);
    binGenerator.generate(os);
}

void AsmAmdHandler::writeBinary(Array<cxbyte>& array) const
{
    AmdGPUBinGenerator binGenerator(&output);
    binGenerator.generate(array);
}

/*
 * GalliumCompute format handler
 */

AsmGalliumHandler::AsmGalliumHandler(Assembler& assembler): AsmFormatHandler(assembler),
             output{}, codeSection(ASMSECT_NONE), dataSection(0),
             commentSection(ASMSECT_NONE), extraSectionCount(0)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::CODE,
                ELFSECTID_TEXT, ".text" });
    insideArgs = insideProgInfo = false;
    savedSection = 0;
}

cxuint AsmGalliumHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = output.kernels.size();
    cxuint thisSection = sections.size();
    output.kernels.push_back({ kernelName, {
        /* default values */
        { 0x0000b848U, 0x000c0000U },
        { 0x0000b84cU, 0x00001788U },
        { 0x0000b860U, 0 } }, 0, {} });
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG });
    kernelStates.push_back({ thisSection, false });
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    insideArgs = insideProgInfo = false;
    return thisKernel;
}

cxuint AsmGalliumHandler::addSection(const char* sectionName, cxuint kernelId)
{
    if (kernelId != ASMKERN_GLOBAL)
        throw AsmFormatException("Adding sections permitted only for global space");
    const cxuint thisSection = sections.size();
    Section section;
    section.kernelId = ASMKERN_GLOBAL;
    if (::strcmp(sectionName, ".rodata") == 0) // data
    {
        dataSection = thisSection;
        section.type = AsmSectionType::DATA;
        section.elfBinSectId = ELFSECTID_RODATA;
        section.name = ".rodata"; // set static name (available by whole lifecycle)
    }
    else if (::strcmp(sectionName, ".text") == 0) // code
    {
        codeSection = thisSection;
        section.type = AsmSectionType::CODE;
        section.elfBinSectId = ELFSECTID_TEXT;
        section.name = ".text"; // set static name (available by whole lifecycle)
    }
    else if (::strcmp(sectionName, ".comment") == 0) // comment
    {
        commentSection = thisSection;
        section.type = AsmSectionType::GALLIUM_COMMENT;
        section.elfBinSectId = ELFSECTID_COMMENT;
        section.name = ".comment"; // set static name (available by whole lifecycle)
    }
    else
    {
        auto out = extraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section  is already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    sections.push_back(section);
    
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = thisSection;
    insideArgs = insideProgInfo = false;
    return thisSection;
}

cxuint AsmGalliumHandler::getSectionId(const char* sectionName) const
{
    if (::strcmp(sectionName, ".rodata") == 0) // data
        return dataSection;
    else if (::strcmp(sectionName, ".text") == 0) // code
        return codeSection;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        return commentSection;
    else
    {
        SectionMap::const_iterator it = extraSectionMap.find(sectionName);
        if (it != extraSectionMap.end())
            return it->second;
    }
    return ASMSECT_NONE;
}

void AsmGalliumHandler::setCurrentKernel(cxuint kernel)
{   // set kernel and their default section
    if (kernel != ASMKERN_GLOBAL && kernel >= kernelStates.size())
        throw AsmFormatException("KernelId out of range");
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentKernel = kernel;
    if (kernel != ASMKERN_GLOBAL)
        assembler.currentSection = kernelStates[kernel].defaultSection;
    else // default main section
        assembler.currentSection = savedSection;
}

void AsmGalliumHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentSection = sectionId;
    assembler.currentKernel = sections[sectionId].kernelId;
    insideArgs = insideProgInfo = false;
}

AsmFormatHandler::SectionInfo AsmGalliumHandler::getSectionInfo(cxuint sectionId) const
{
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    
    AsmFormatHandler::SectionInfo info;
    info.type = sections[sectionId].type;
    info.flags = 0;
    if (sectionId == codeSection)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
    else if (sectionId == dataSection)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    else if (sectionId == commentSection)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    else if (sections[sectionId].type == AsmSectionType::EXTRA_SECTION)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    info.name = sections[sectionId].name;
    return info;
}

namespace CLRX
{
void AsmGalliumPseudoOps::doArgs(AsmGalliumHandler& handler,
               const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Arguments outside kernel definition");
        return;
    }
    handler.insideArgs = true;
    handler.insideProgInfo = false;
}

static const std::pair<const char*, GalliumArgType> galliumArgTypesMap[9] =
{
    { "constant", GalliumArgType::CONSTANT },
    { "global", GalliumArgType::GLOBAL },
    { "image2d_rd", GalliumArgType::IMAGE2D_RDONLY },
    { "image2d_wr", GalliumArgType::IMAGE2D_WRONLY },
    { "image3d_rd", GalliumArgType::IMAGE3D_RDONLY },
    { "image3d_wr", GalliumArgType::IMAGE3D_WRONLY },
    { "scalar", GalliumArgType::SCALAR },
    { "local", GalliumArgType::LOCAL },
    { "sampler", GalliumArgType::SAMPLER }
};

void AsmGalliumPseudoOps::doArg(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string name;
    bool good = true;
    const char* nameStringPlace = linePtr;
    GalliumArgType argType = GalliumArgType::GLOBAL;
    if (getNameArg(asmr, name, linePtr, "argument type"))
    {
        argType = GalliumArgType(binaryMapFind(galliumArgTypesMap, galliumArgTypesMap + 9,
                     name.c_str(), CStringLess())-galliumArgTypesMap);
        if (int(argType) == 9) // end of this map
        {
            asmr.printError(nameStringPlace, "Unknown argument type");
            good = false;
        }
    }
    else
        good = false;
    //
    if (!skipRequiredComma(asmr, linePtr, "absolute value"))
        return;
    skipSpacesToEnd(linePtr, end);
    const char* sizeStrPlace = linePtr;
    uint64_t size = 4;
    good &= getAbsoluteValueArg(asmr, size, linePtr, true);
    
    if (good && (size > UINT32_MAX || size == 0))
        asmr.printWarning(sizeStrPlace, "Size of argument out of range");
    
    uint64_t targetSize = size;
    uint64_t targetAlign = 1U<<(31-CLZ32(size));
    bool sext = false;
    GalliumArgSemantic argSemantic = GalliumArgSemantic::GENERAL;
    
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        const char* targetSizePlace = linePtr;
        uint64_t tgtSize = size;
        good &= getAbsoluteValueArg(asmr, tgtSize, linePtr, false);
        
        if (tgtSize > UINT32_MAX || tgtSize == 0)
            asmr.printWarning(targetSizePlace, "Target size of argument out of range");
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            const char* targetAlignPlace = linePtr;
            uint64_t tgtAlign = 4;
            good &= getAbsoluteValueArg(asmr, tgtAlign, linePtr, false);
            
            if (tgtAlign > UINT32_MAX || tgtAlign == 0)
                asmr.printWarning(targetAlignPlace,
                                  "Target alignment of argument out of range");
            if (tgtAlign == (1U<<(31-CLZ32(tgtAlign))))
            {
                asmr.printError(targetAlignPlace, "Target alignment is not power of 2");
                good = false;
            }
            
            if (!skipComma(asmr, haveComma, linePtr))
                return;
            if (haveComma)
            {
                skipSpacesToEnd(linePtr, end);
                const char* numExtPlace = linePtr;
                if (getNameArg(asmr, name, linePtr, "numeric extension", false))
                {
                    if (name == "sext")
                        sext = true;
                    else if (name != "zext")
                    {
                        asmr.printError(numExtPlace, "Unknown numeric extension");
                        good = false;
                    }
                }
                else
                    good = false;
                
                if (!skipComma(asmr, haveComma, linePtr))
                    return;
                if (haveComma)
                {
                    skipSpacesToEnd(linePtr, end);
                    const char* semanticPlace = linePtr;
                    if (getNameArg(asmr, name, linePtr, "argument semantic", false))
                    {
                        if (name == "griddim")
                            argSemantic = GalliumArgSemantic::GRID_DIMENSION;
                        else if (name == "gridoffset")
                            argSemantic = GalliumArgSemantic::GRID_OFFSET;
                        else if (name != "general")
                        {
                            asmr.printError(semanticPlace,
                                    "Unknown argument semantic type");
                            good = false;
                        }
                    }
                    else
                        good = false;
                }
            }
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Argument definition outside kernel configuration");
        return;
    }
    if (!handler.insideArgs)
    {
        asmr.printError(pseudoOpPlace, "Argument definition outside arguments list");
        return;
    }
    // put this definition to argument list
    handler.output.kernels[asmr.currentKernel].argInfos.push_back(
        { argType, sext, argSemantic, uint32_t(size),
            uint32_t(targetSize), uint32_t(targetAlign) });
}

void AsmGalliumPseudoOps::doProgInfo(AsmGalliumHandler& handler,
                 const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo outside kernel definition");
        return;
    }
    handler.insideArgs = false;
    handler.insideProgInfo = true;
    handler.kernelStates[asmr.currentKernel].hasProgInfo = true;
    handler.kernelStates[asmr.currentKernel].progInfoEntries = 0;
}

void AsmGalliumPseudoOps::doEntry(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* addrPlace = linePtr;
    uint64_t entryAddr;
    bool good = true;
    if (getAbsoluteValueArg(asmr, entryAddr, linePtr, true))
    {
        if (entryAddr > UINT32_MAX)
            asmr.printWarning(addrPlace, "64-bit value of address has been truncated");
    }
    else
        good = false;
    if (!skipRequiredComma(asmr, linePtr, "value of entry"))
        return;
    
    skipSpacesToEnd(linePtr, end);
    const char* entryValPlace = linePtr;
    uint64_t entryVal;
    if (getAbsoluteValueArg(asmr, entryVal, linePtr, true))
    {
        if (entryVal > UINT32_MAX)
            asmr.printWarning(entryValPlace, "64-bit value has been truncated");
    }
    else
        good = false;
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // do operation
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo entry outside kernel configuration");
        return;
    }
    if (!handler.insideProgInfo)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo entry definition outside ProgInfo");
        return;
    }
    AsmGalliumHandler::Kernel& kstate = handler.kernelStates[asmr.currentKernel];
    kstate.hasProgInfo = true;
    if (kstate.progInfoEntries == 3)
    {
        asmr.printError(pseudoOpPlace, "Maximum 3 entries can be in ProgInfo");
        return;
    }
    GalliumProgInfoEntry& pentry = handler.output.kernels[asmr.currentKernel]
            .progInfo[kstate.progInfoEntries++];
    pentry.address = entryAddr;
    pentry.value = entryVal;
}

}

void AsmGalliumHandler::parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char* linePtr)
{
    const size_t pseudoOp = binaryFind(galliumPseudoOpNamesTbl, galliumPseudoOpNamesTbl +
                    sizeof(galliumPseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - galliumPseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case GALLIUMOP_ARG:
            AsmGalliumPseudoOps::doArg(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_ARGS:
            AsmGalliumPseudoOps::doArgs(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_ENTRY:
            AsmGalliumPseudoOps::doEntry(*this, stmtPlace, linePtr);
            break;
        case GALLIUM_PROGINFO:
            AsmGalliumPseudoOps::doProgInfo(*this, stmtPlace, linePtr);
            break;
        default:
            break;
    }
}

bool AsmGalliumHandler::prepareBinary()
{   // before call we initialize pointers and datas
    size_t sectionsNum = sections.size();
    for (size_t i = 0; i < sectionsNum; i++)
    {
        const AsmSection& asmSection = assembler.sections[i];
        const Section& section = sections[i];
        switch(asmSection.type)
        {
            case AsmSectionType::CODE:
                output.codeSize = asmSection.content.size();
                output.code = asmSection.content.data();
                break;
            case AsmSectionType::DATA:
                output.globalDataSize = asmSection.content.size();
                output.globalData = asmSection.content.data();
                break;
            case AsmSectionType::EXTRA_SECTION:
            {
                output.extraSections.push_back({section.name,
                    asmSection.content.size(), asmSection.content.data(),
                    1, SHT_PROGBITS, 0, ELFSECTID_NULL, 0, 0 });
                break;
            }
            case AsmSectionType::GALLIUM_COMMENT:
                output.commentSize = asmSection.content.size();
                output.comment = (const char*)asmSection.content.data();
                break;
            default:
                abort(); /// fatal error
                break;
        }
    }
    // if set adds symbols to binary
    if (assembler.getFlags() & ASM_FORCE_ADD_SYMBOLS)
        for (const AsmSymbolEntry& symEntry: assembler.symbolMap)
        {
            if (!symEntry.second.hasValue &&
                ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // unresolved or local
            if (assembler.kernelMap.find(symEntry.first) != assembler.kernelMap.end())
                continue; // if kernel name
            cxuint binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            
            output.extraSymbols.push_back({ symEntry.first, symEntry.second.value,
                    symEntry.second.size, binSectId, false, symEntry.second.info,
                    symEntry.second.other });
        }
    
    /// checking symbols and set offset for kernels
    bool good = true;
    const AsmSymbolMap& symbolMap = assembler.getSymbolMap();
    for (size_t ki = 0; ki < output.kernels.size(); ki++)
    {
        GalliumKernelInput& kinput = output.kernels[ki];
        auto it = symbolMap.find(kinput.kernelName);
        if (it == symbolMap.end() || !it->second.isDefined())
        {   // error, undefined
            std::string message = "Symbol for kernel '";
            message += kinput.kernelName;
            message += "' is undefined";
            assembler.printError(assembler.kernels[ki].sourcePos, message.c_str());
            good = false;
            continue;
        }
        const AsmSymbol& symbol = it->second;
        if (!symbol.hasValue)
        {   // error, unresolved
            std::string message = "Symbol for kernel '";
            message += kinput.kernelName;
            message += "' is not resolved";
            assembler.printError(assembler.kernels[ki].sourcePos, message.c_str());
            good = false;
            continue;
        }
        if (symbol.sectionId != codeSection)
        {   /// error, wrong section
            std::string message = "Symbol for kernel '";
            message += kinput.kernelName;
            message += "' is defined for section other than '.text'";
            assembler.printError(assembler.kernels[ki].sourcePos, message.c_str());
            good = false;
            continue;
        }
        kinput.offset = symbol.value;
    }
    /* initialize progInfos */
    //////////////////////////
    return good;
}

void AsmGalliumHandler::writeBinary(std::ostream& os) const
{
    GalliumBinGenerator binGenerator(&output);
    binGenerator.generate(os);
}

void AsmGalliumHandler::writeBinary(Array<cxbyte>& array) const
{
    GalliumBinGenerator binGenerator(&output);
    binGenerator.generate(array);
}
