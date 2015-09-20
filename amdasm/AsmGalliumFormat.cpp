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
{ "arg", "args", "entry", "globaldata", "proginfo" };

enum
{
    GALLIUMOP_ARG = 0, GALLIUMOP_ARGS, GALLIUMOP_ENTRY,
    GALLIUMOP_GLOBALDATA, GALLIUMOP_PROGINFO
};

/*
 * GalliumCompute format handler
 */

AsmGalliumHandler::AsmGalliumHandler(Assembler& assembler): AsmFormatHandler(assembler),
             output{}, codeSection(0), dataSection(ASMSECT_NONE),
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
    output.addEmptyKernel(kernelName);
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG, ELFSECTID_UNDEF, nullptr });
    kernelStates.push_back({ thisSection, false, 0 });
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    insideArgs = insideProgInfo = false;
    return thisKernel;
}

cxuint AsmGalliumHandler::addSection(const char* sectionName, cxuint kernelId)
{
    const cxuint thisSection = sections.size();
    Section section;
    section.kernelId = ASMKERN_GLOBAL;  // we ignore input kernelId, we go to main
    
    if (::strcmp(sectionName, ".rodata") == 0) // data
    {
        if (dataSection!=ASMSECT_NONE)
            throw AsmFormatException("Only section '.rodata' can be in binary");
        dataSection = thisSection;
        section.type = AsmSectionType::DATA;
        section.elfBinSectId = ELFSECTID_RODATA;
        section.name = ".rodata"; // set static name (available by whole lifecycle)
    }
    else if (::strcmp(sectionName, ".text") == 0) // code
    {
        if (codeSection!=ASMSECT_NONE)
            throw AsmFormatException("Only one section '.text' can be in binary");
        codeSection = thisSection;
        section.type = AsmSectionType::CODE;
        section.elfBinSectId = ELFSECTID_TEXT;
        section.name = ".text"; // set static name (available by whole lifecycle)
    }
    else if (::strcmp(sectionName, ".comment") == 0) // comment
    {
        if (commentSection!=ASMSECT_NONE)
            throw AsmFormatException("Only one section '.comment' can be in binary");
        commentSection = thisSection;
        section.type = AsmSectionType::GALLIUM_COMMENT;
        section.elfBinSectId = ELFSECTID_COMMENT;
        section.name = ".comment"; // set static name (available by whole lifecycle)
    }
    else
    {
        auto out = extraSectionMap.insert(std::make_pair(CString(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
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
    insideArgs = insideProgInfo = false;
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
    if (info.type == AsmSectionType::CODE)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
    else if (info.type != AsmSectionType::CONFIG)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    
    info.name = sections[sectionId].name;
    return info;
}

namespace CLRX
{

bool AsmGalliumPseudoOps::checkPseudoOpName(const CString& string)
{
    if (string.empty() || string[0] != '.')
        return false;
    const size_t pseudoOp = binaryFind(galliumPseudoOpNamesTbl, galliumPseudoOpNamesTbl +
                sizeof(galliumPseudoOpNamesTbl)/sizeof(char*), string.c_str()+1,
               CStringLess()) - galliumPseudoOpNamesTbl;
    return pseudoOp < sizeof(galliumPseudoOpNamesTbl)/sizeof(char*);
}

void AsmGalliumPseudoOps::doGlobalData(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    asmr.goToSection(pseudoOpPlace, ".rodata");
}
    
void AsmGalliumPseudoOps::doArgs(AsmGalliumHandler& handler,
               const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Arguments outside kernel definition");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
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
    { "local", GalliumArgType::LOCAL },
    { "sampler", GalliumArgType::SAMPLER },
    { "scalar", GalliumArgType::SCALAR }
};

void AsmGalliumPseudoOps::doArg(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
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
    
    skipSpacesToEnd(linePtr, end);
    char name[20];
    bool good = true;
    const char* nameStringPlace = linePtr;
    GalliumArgType argType = GalliumArgType::GLOBAL;
    
    GalliumArgSemantic argSemantic = GalliumArgSemantic::GENERAL;
    bool semanticDefined = false;
    if (getNameArg(asmr, 20, name, linePtr, "argument type"))
    {
        toLowerString(name);
        if (::strcmp(name, "griddim")==0)
        {   // shortcur for grid dimension
            argSemantic = GalliumArgSemantic::GRID_DIMENSION;
            argType = GalliumArgType::SCALAR;
            semanticDefined = true;
        }
        else if (::strcmp(name, "gridoffset")==0)
        {   // shortcut for grid dimension
            argSemantic = GalliumArgSemantic::GRID_OFFSET;
            argType = GalliumArgType::SCALAR;
            semanticDefined = true;
        }
        else
        {
            cxuint index = binaryMapFind(galliumArgTypesMap, galliumArgTypesMap + 9,
                         name, CStringLess()) - galliumArgTypesMap;
            if (index != 9) // end of this map
                argType = galliumArgTypesMap[index].second;
            else
            {
                asmr.printError(nameStringPlace, "Unknown argument type");
                good = false;
            }
        }
    }
    else
        good = false;
    //
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    const char* sizeStrPlace = linePtr;
    uint64_t size = 4;
    good &= getAbsoluteValueArg(asmr, size, linePtr, true);
    
    if (size > UINT32_MAX || size == 0)
        asmr.printWarning(sizeStrPlace, "Size of argument out of range");
    
    uint64_t targetSize = size;
    uint64_t targetAlign = (size!=0) ? 1ULL<<(63-CLZ64(size)) : 1;
    if (size > targetAlign)
        targetAlign <<= 1;
    bool sext = false;
    
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        const char* targetSizePlace = linePtr;
        if (getAbsoluteValueArg(asmr, targetSize, linePtr, false))
        {
            if (targetSize > UINT32_MAX || targetSize == 0)
                asmr.printWarning(targetSizePlace, "Target size of argument out of range");
        }
        else
            good = false;
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            const char* targetAlignPlace = linePtr;
            if (getAbsoluteValueArg(asmr, targetAlign, linePtr, false))
            {
                if (targetAlign > UINT32_MAX || targetAlign == 0)
                    asmr.printWarning(targetAlignPlace,
                                      "Target alignment of argument out of range");
                if (targetAlign != (1ULL<<(63-CLZ64(targetAlign))))
                {
                    asmr.printError(targetAlignPlace, "Target alignment is not power of 2");
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
                const char* numExtPlace = linePtr;
                if (getNameArg(asmr, 5, name, linePtr, "numeric extension", false))
                {
                    toLowerString(name);
                    if (::strcmp(name, "sext")==0)
                        sext = true;
                    else if (::strcmp(name, "zext")!=0 && *name!=0)
                    {
                        asmr.printError(numExtPlace, "Unknown numeric extension");
                        good = false;
                    }
                }
                else
                    good = false;
                
                if (!semanticDefined)
                {   /// if semantic has not been defined in the first argument
                    if (!skipComma(asmr, haveComma, linePtr))
                        return;
                    if (haveComma)
                    {
                        skipSpacesToEnd(linePtr, end);
                        const char* semanticPlace = linePtr;
                        if (getNameArg(asmr, 15, name, linePtr, "argument semantic", false))
                        {
                            toLowerString(name);
                            if (::strcmp(name, "griddim")==0)
                                argSemantic = GalliumArgSemantic::GRID_DIMENSION;
                            else if (::strcmp(name, "gridoffset")==0)
                                argSemantic = GalliumArgSemantic::GRID_OFFSET;
                            else if (::strcmp(name, "general")!=0 && *name!=0)
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
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
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
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo outside kernel definition");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.insideArgs = false;
    handler.insideProgInfo = true;
    handler.kernelStates[asmr.currentKernel].hasProgInfo = true;
}

void AsmGalliumPseudoOps::doEntry(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
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
    
    skipSpacesToEnd(linePtr, end);
    const char* addrPlace = linePtr;
    uint64_t entryAddr;
    bool good = true;
    if (getAbsoluteValueArg(asmr, entryAddr, linePtr, true))
        asmr.printWarningForRange(32, entryAddr, asmr.getSourcePos(addrPlace), false);
    else
        good = false;
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    skipSpacesToEnd(linePtr, end);
    const char* entryValPlace = linePtr;
    uint64_t entryVal;
    if (getAbsoluteValueArg(asmr, entryVal, linePtr, true))
        asmr.printWarningForRange(32, entryVal, asmr.getSourcePos(entryValPlace));
    else
        good = false;
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // do operation
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

bool AsmGalliumHandler::parsePseudoOp(const CString& firstName,
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
        case GALLIUMOP_GLOBALDATA:
            AsmGalliumPseudoOps::doGlobalData(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_PROGINFO:
            AsmGalliumPseudoOps::doProgInfo(*this, stmtPlace, linePtr);
            break;
        default:
            return false;
    }
    return true;
}

bool AsmGalliumHandler::prepareBinary()
{   // before call we initialize pointers and datas
    size_t sectionsNum = sections.size();
    for (size_t i = 0; i < sectionsNum; i++)
    {
        const AsmSection& asmSection = assembler.sections[i];
        const Section& section = sections[i];
        const size_t sectionSize = asmSection.content.size();
        const cxbyte* sectionData = (!asmSection.content.empty()) ?
                asmSection.content.data() : (const cxbyte*)"";
        switch(asmSection.type)
        {
            case AsmSectionType::CODE:
                output.codeSize = sectionSize;
                output.code = sectionData;
                break;
            case AsmSectionType::DATA:
                output.globalDataSize = sectionSize;
                output.globalData = sectionData;
                break;
            case AsmSectionType::EXTRA_SECTION:
                output.extraSections.push_back({section.name, sectionSize, sectionData,
                    1, SHT_PROGBITS, 0, ELFSECTID_NULL, 0, 0 });
                break;
            case AsmSectionType::GALLIUM_COMMENT:
                output.commentSize = sectionSize;
                output.comment = (const char*)sectionData;
                break;
            default: // ignore other sections
                break;
        }
    }
    // if set adds symbols to binary
    if (assembler.getFlags() & ASM_FORCE_ADD_SYMBOLS)
        for (const AsmSymbolEntry& symEntry: assembler.symbolMap)
        {
            if (!symEntry.second.hasValue ||
                ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // unresolved or local
            if (assembler.kernelMap.find(symEntry.first.c_str()) != assembler.kernelMap.end())
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
            
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                        "Symbol for kernel '")+kinput.kernelName.c_str()+
                        "' is undefined").c_str());
            good = false;
            continue;
        }
        const AsmSymbol& symbol = it->second;
        if (!symbol.hasValue)
        {   // error, unresolved
            
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '") + kinput.kernelName.c_str() +
                    "' is not resolved").c_str());
            good = false;
            continue;
        }
        if (symbol.sectionId != codeSection)
        {   /// error, wrong section
            
            assembler.printError(assembler.kernels[ki].sourcePos, (std::string(
                    "Symbol for kernel '")+kinput.kernelName.c_str()+
                    "' is defined for section other than '.text'").c_str());
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
