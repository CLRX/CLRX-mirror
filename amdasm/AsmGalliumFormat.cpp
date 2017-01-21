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

#include <CLRX/Config.h>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* galliumPseudoOpNamesTbl[] =
{
    "arg", "args", "config",
    "debugmode", "dims", "dx10clamp",
    "entry", "exceptions", "floatmode",
    "globaldata", "ieeemode",
    "kcode", "kcodeend",
    "localsize", "pgmrsrc1", "pgmrsrc2", "priority",
    "privmode", "proginfo",
    "scratchbuffer", "sgprsnum", "tgsize",
    "userdatanum", "vgprsnum"
};

enum
{
    GALLIUMOP_ARG = 0, GALLIUMOP_ARGS, GALLIUMOP_CONFIG,
    GALLIUMOP_DEBUGMODE, GALLIUMOP_DIMS, GALLIUMOP_DX10CLAMP,
    GALLIUMOP_ENTRY, GALLIUMOP_EXCEPTIONS, GALLIUMOP_FLOATMODE,
    GALLIUMOP_GLOBALDATA, GALLIUMOP_IEEEMODE,
    GALLIUMOP_KCODE, GALLIUMOP_KCODEEND,
    GALLIUMOP_LOCALSIZE, GALLIUMOP_PGMRSRC1, GALLIUMOP_PGMRSRC2, GALLIUMOP_PRIORITY,
    GALLIUMOP_PRIVMODE, GALLIUMOP_PROGINFO,
    GALLIUMOP_SCRATCHBUFFER, GALLIUMOP_SGPRSNUM, GALLIUMOP_TGSIZE,
    GALLIUMOP_USERDATANUM, GALLIUMOP_VGPRSNUM
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
    inside = Inside::MAINLAYOUT;
    currentKcodeKernel = ASMKERN_GLOBAL;
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
    inside = Inside::MAINLAYOUT;
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
        /// reference entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    sections.push_back(section);
    
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = thisSection;
    inside = Inside::MAINLAYOUT;
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
    inside = Inside::MAINLAYOUT;
}

void AsmGalliumHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    
    assembler.currentSection = sectionId;
    assembler.currentKernel = sections[sectionId].kernelId;
    inside = Inside::MAINLAYOUT;
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

// check whether label is kernel label and restore register allocation to this kernel
void AsmGalliumHandler::handleLabel(const CString& label)
{
    if (assembler.sections[assembler.currentSection].type != AsmSectionType::CODE)
        return;
    auto kit = assembler.kernelMap.find(label);
    if (kit == assembler.kernelMap.end())
        return;
    if (!kcodeSelection.empty())
        return; // do not change if inside kcode
    // save other state
    saveKcodeCurrentAllocRegs();
    // restore this state
    currentKcodeKernel = kit->second;
    restoreKcodeCurrentAllocRegs();
}

void AsmGalliumHandler::restoreKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {
        Kernel& newKernel = kernelStates[currentKcodeKernel];
        assembler.isaAssembler->setAllocatedRegisters(newKernel.allocRegs,
                            newKernel.allocRegFlags);
    }
}

void AsmGalliumHandler::saveKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {   // save other state
        size_t regTypesNum;
        Kernel& oldKernel = kernelStates[currentKcodeKernel];
        const cxuint* regs = assembler.isaAssembler->getAllocatedRegisters(
                            regTypesNum, oldKernel.allocRegFlags);
        std::copy(regs, regs+2, oldKernel.allocRegs);
    }
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

void AsmGalliumPseudoOps::doConfig(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Configuration outside kernel definition");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel].hasProgInfo)
    {
        asmr.printError(pseudoOpPlace,
                "Configuration can't be defined if progInfo was defined");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.inside = AsmGalliumHandler::Inside::CONFIG;
    handler.output.kernels[asmr.currentKernel].useConfig = true;
}

void AsmGalliumPseudoOps::doGlobalData(AsmGalliumHandler& handler,
                   const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    asmr.goToSection(pseudoOpPlace, ".rodata");
}

void AsmGalliumPseudoOps::setDimensions(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    cxuint dimMask = 0;
    if (!parseDimensions(asmr, linePtr, dimMask))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.kernels[asmr.currentKernel].config.dimMask = dimMask;
}

void AsmGalliumPseudoOps::setConfigValue(AsmGalliumHandler& handler,
         const char* pseudoOpPlace, const char* linePtr, GalliumConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value = BINGEN_NOTSUPPLIED;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    /* ranges checking */
    if (good)
    {
        switch(target)
        {
            case GALLIUMCVAL_SGPRSNUM:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
                if (value > maxSGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Used SGPRs number out of range (0-%u)", maxSGPRsNum);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case GALLIUMCVAL_VGPRSNUM:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxVGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_VGPR, 0);
                if (value > maxVGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Used VGPRs number out of range (0-%u)", maxVGPRsNum);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case GALLIUMCVAL_EXCEPTIONS:
                asmr.printWarningForRange(7, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0x7f;
                break;
            case GALLIUMCVAL_FLOATMODE:
                asmr.printWarningForRange(8, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0xff;
                break;
            case GALLIUMCVAL_PRIORITY:
                asmr.printWarningForRange(2, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 3;
                break;
            case GALLIUMCVAL_LOCALSIZE:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                const cxuint maxLocalSize = getGPUMaxLocalSize(arch);
                if (value > maxLocalSize)
                {
                    char buf[64];
                    snprintf(buf, 64, "LocalSize out of range (0-%u)", maxLocalSize);
                    asmr.printError(valuePlace, buf);
                    good = false;
                }
                break;
            }
            case GALLIUMCVAL_USERDATANUM:
                if (value > 16)
                {
                    asmr.printError(valuePlace, "UserDataNum out of range (0-16)");
                    good = false;
                }
                break;
            case GALLIUMCVAL_PGMRSRC1:
            case GALLIUMCVAL_PGMRSRC2:
                asmr.printWarningForRange(32, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                break;
            default:
                break;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    GalliumKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    // set value
    switch(target)
    {
        case GALLIUMCVAL_SGPRSNUM:
            config.usedSGPRsNum = value;
            break;
        case GALLIUMCVAL_VGPRSNUM:
            config.usedVGPRsNum = value;
            break;
        case GALLIUMCVAL_PGMRSRC1:
            config.pgmRSRC1 = value;
            break;
        case GALLIUMCVAL_PGMRSRC2:
            config.pgmRSRC2 = value;
            break;
        case GALLIUMCVAL_FLOATMODE:
            config.floatMode = value;
            break;
        case GALLIUMCVAL_LOCALSIZE:
            config.localSize = value;
            break;
        case GALLIUMCVAL_SCRATCHBUFFER:
            config.scratchBufferSize = value;
            break;
        case GALLIUMCVAL_PRIORITY:
            config.priority = value;
            break;
        case GALLIUMCVAL_USERDATANUM:
            config.userDataNum = value;
            break;
        case GALLIUMCVAL_EXCEPTIONS:
            config.exceptions = value;
            break;
        default:
            break;
    }
}

void AsmGalliumPseudoOps::setConfigBoolValue(AsmGalliumHandler& handler,
         const char* pseudoOpPlace, const char* linePtr, GalliumConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.inside != AsmGalliumHandler::Inside::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    GalliumKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    switch(target)
    {
        case GALLIUMCVAL_DEBUGMODE:
            config.debugMode = true;
            break;
        case GALLIUMCVAL_DX10CLAMP:
            config.dx10Clamp = true;
            break;
        case GALLIUMCVAL_IEEEMODE:
            config.ieeeMode = true;
            break;
        case GALLIUMCVAL_PRIVMODE:
            config.privilegedMode = true;
            break;
        case GALLIUMCVAL_TGSIZE:
            config.tgSize = true;
            break;
        default:
            break;
    }
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
    
    handler.inside = AsmGalliumHandler::Inside::ARGS;
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

static const std::pair<const char*, cxuint> galliumArgSemanticsMap[5] =
{
    { "general", cxuint(GalliumArgSemantic::GENERAL) },
    { "griddim", cxuint(GalliumArgSemantic::GRID_DIMENSION) },
    { "gridoffset", cxuint(GalliumArgSemantic::GRID_OFFSET) },
    { "imgformat", cxuint(GalliumArgSemantic::IMAGE_FORMAT) },
    { "imgsize", cxuint(GalliumArgSemantic::IMAGE_SIZE) },
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
    if (handler.inside != AsmGalliumHandler::Inside::ARGS)
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
    
    // parse rest of arguments
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    const char* sizeStrPlace = linePtr;
    uint64_t size = 4;
    good &= getAbsoluteValueArg(asmr, size, linePtr, true);
    
    if (size > UINT32_MAX || size == 0)
        asmr.printWarning(sizeStrPlace, "Size of argument out of range");
    
    uint64_t targetSize = (size+3ULL)&~3ULL;
    uint64_t targetAlign = (size!=0) ? 1ULL<<(63-CLZ64(targetSize)) : 1;
    if (targetSize > targetAlign)
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
                if (targetAlign==0 || targetAlign != (1ULL<<(63-CLZ64(targetAlign))))
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
                        cxuint semantic;
                        if (getEnumeration(asmr, linePtr, "argument semantic", 5,
                                    galliumArgSemanticsMap, semantic))
                            argSemantic = GalliumArgSemantic(semantic);
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
    if (handler.output.kernels[asmr.currentKernel].useConfig)
    {
        asmr.printError(pseudoOpPlace,
                "ProgInfo can't be defined if configuration was exists");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    handler.inside = AsmGalliumHandler::Inside::PROGINFO;
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
    if (handler.inside != AsmGalliumHandler::Inside::PROGINFO)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo entry definition outside ProgInfo");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* addrPlace = linePtr;
    uint64_t entryAddr;
    bool good = true;
    if (getAbsoluteValueArg(asmr, entryAddr, linePtr, true))
        asmr.printWarningForRange(32, entryAddr, asmr.getSourcePos(addrPlace),
                              WS_UNSIGNED);
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

/* update kernel code selection, join all regallocation and store to
 * current kernel regalloc */
void AsmGalliumPseudoOps::updateKCodeSel(AsmGalliumHandler& handler,
          const std::vector<cxuint>& oldset)
{
    Assembler& asmr = handler.assembler;
    // old elements - join current regstate with all them
    size_t regTypesNum;
    for (auto it = oldset.begin(); it != oldset.end(); ++it)
    {
        Flags curAllocRegFlags;
        const cxuint* curAllocRegs = asmr.isaAssembler->getAllocatedRegisters(regTypesNum,
                               curAllocRegFlags);
        cxuint newAllocRegs[2];
        AsmGalliumHandler::Kernel& kernel = handler.kernelStates[*it];
        newAllocRegs[0] = std::max(curAllocRegs[0], kernel.allocRegs[0]);
        newAllocRegs[1] = std::max(curAllocRegs[1], kernel.allocRegs[1]);
        kernel.allocRegFlags |= curAllocRegFlags;
        std::copy(newAllocRegs, newAllocRegs+2, kernel.allocRegs);
    }
    asmr.isaAssembler->setAllocatedRegisters();
}

void AsmGalliumPseudoOps::doKCode(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    bool good = true;
    skipSpacesToEnd(linePtr, end);
    if (linePtr==end)
        return;
    std::unordered_set<cxuint> newSel(handler.kcodeSelection.begin(),
                          handler.kcodeSelection.end());
    do {
        CString kname;
        const char* knamePlace = linePtr;
        skipSpacesToEnd(linePtr, end);
        bool removeKernel = false;
        if (linePtr!=end && *linePtr=='-')
        {   // '-' - remove this kernel from current kernel selection
            removeKernel = true;
            linePtr++;
        }
        else if (linePtr!=end && *linePtr=='+')
        {
            linePtr++;
            skipSpacesToEnd(linePtr, end);
            if (linePtr==end)
            {   // add all kernels
                for (cxuint k = 0; k < handler.kernelStates.size(); k++)
                    newSel.insert(k);
                break;
            }
        }
        
        if (!getNameArg(asmr, kname, linePtr, "kernel"))
        { good = false; continue; }
        auto kit = asmr.kernelMap.find(kname);
        if (kit == asmr.kernelMap.end())
        {
            asmr.printError(knamePlace, "Kernel not found");
            continue;
        }
        if (!removeKernel)
            newSel.insert(kit->second);
        else // remove kernel
            newSel.erase(kit->second);
    } while (skipCommaForMultipleArgs(asmr, linePtr));
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CODE)
    {
        asmr.printError(pseudoOpPlace, "KCode outside code");
        return;
    }
    if (handler.kcodeSelStack.empty())
        handler.saveKcodeCurrentAllocRegs();
    // push to stack
    handler.kcodeSelStack.push(handler.kcodeSelection);
    // set current sel
    handler.kcodeSelection.assign(newSel.begin(), newSel.end());
    
    std::sort(handler.kcodeSelection.begin(), handler.kcodeSelection.end());
    updateKCodeSel(handler, handler.kcodeSelStack.top());
}

void AsmGalliumPseudoOps::doKCodeEnd(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (handler.sections[asmr.currentSection].type != AsmSectionType::CODE)
    {
        asmr.printError(pseudoOpPlace, "KCodeEnd outside code");
        return;
    }
    if (handler.kcodeSelStack.empty())
    {
        asmr.printError(pseudoOpPlace, "'.kcodeend' without '.kcode'");
        return;
    }
    updateKCodeSel(handler, handler.kcodeSelection);
    handler.kcodeSelection = handler.kcodeSelStack.top();
    handler.kcodeSelStack.pop();
    if (handler.kcodeSelStack.empty())
        handler.restoreKcodeCurrentAllocRegs();
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
        case GALLIUMOP_CONFIG:
            AsmGalliumPseudoOps::doConfig(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_DEBUGMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_DEBUGMODE);
            break;
        case GALLIUMOP_DIMS:
            AsmGalliumPseudoOps::setDimensions(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_DX10CLAMP:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_DX10CLAMP);
            break;
        case GALLIUMOP_ENTRY:
            AsmGalliumPseudoOps::doEntry(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_EXCEPTIONS:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_EXCEPTIONS);
            break;
        case GALLIUMOP_FLOATMODE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_FLOATMODE);
            break;
        case GALLIUMOP_GLOBALDATA:
            AsmGalliumPseudoOps::doGlobalData(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_IEEEMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_IEEEMODE);
            break;
        case GALLIUMOP_KCODE:
            AsmGalliumPseudoOps::doKCode(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_KCODEEND:
            AsmGalliumPseudoOps::doKCodeEnd(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_LOCALSIZE:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_LOCALSIZE);
            break;
        case GALLIUMOP_PRIORITY:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_PRIORITY);
            break;
        case GALLIUMOP_PRIVMODE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_PRIVMODE);
            break;
        case GALLIUMOP_PGMRSRC1:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_PGMRSRC1);
            break;
        case GALLIUMOP_PGMRSRC2:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_PGMRSRC2);
            break;
        case GALLIUMOP_PROGINFO:
            AsmGalliumPseudoOps::doProgInfo(*this, stmtPlace, linePtr);
            break;
        case GALLIUMOP_SCRATCHBUFFER:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_SCRATCHBUFFER);
            break;
        case GALLIUMOP_SGPRSNUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_SGPRSNUM);
            break;
        case GALLIUMOP_TGSIZE:
            AsmGalliumPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                GALLIUMCVAL_TGSIZE);
            break;
        case GALLIUMOP_USERDATANUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_USERDATANUM);
            break;
        case GALLIUMOP_VGPRSNUM:
            AsmGalliumPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                    GALLIUMCVAL_VGPRSNUM);
            break;
        default:
            return false;
    }
    return true;
}

bool AsmGalliumHandler::prepareBinary()
{   // before call we initialize pointers and datas
    bool good = true;
    
    output.is64BitElf = assembler.is64Bit();
    size_t sectionsNum = sections.size();
    size_t kernelsNum = kernelStates.size();
    output.deviceType = assembler.getDeviceType();
    if (assembler.isaAssembler!=nullptr)
    {   // make last kernel registers pool updates
        if (kcodeSelStack.empty())
            saveKcodeCurrentAllocRegs();
        else
            while (!kcodeSelStack.empty())
            {   // pop from kcode stack and apply changes
                AsmGalliumPseudoOps::updateKCodeSel(*this, kcodeSelection);
                kcodeSelection = kcodeSelStack.top();
                kcodeSelStack.pop();
            }
    }
    
    for (size_t i = 0; i < sectionsNum; i++)
    {
        const AsmSection& asmSection = assembler.sections[i];
        const Section& section = sections[i];
        const size_t sectionSize = asmSection.getSize();
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
            case AsmSectionType::EXTRA_PROGBITS:
            case AsmSectionType::EXTRA_NOTE:
            case AsmSectionType::EXTRA_NOBITS:
            case AsmSectionType::EXTRA_SECTION:
            {
                uint32_t elfSectType =
                       (asmSection.type==AsmSectionType::EXTRA_NOTE) ? SHT_NOTE :
                       (asmSection.type==AsmSectionType::EXTRA_NOBITS) ? SHT_NOBITS :
                             SHT_PROGBITS;
                uint32_t elfSectFlags = 
                    ((asmSection.flags&ASMELFSECT_ALLOCATABLE) ? SHF_ALLOC : 0) |
                    ((asmSection.flags&ASMELFSECT_WRITEABLE) ? SHF_WRITE : 0) |
                    ((asmSection.flags&ASMELFSECT_EXECUTABLE) ? SHF_EXECINSTR : 0);
                output.extraSections.push_back({section.name, sectionSize, sectionData,
                    asmSection.alignment!=0?asmSection.alignment:1, elfSectType,
                    elfSectFlags, ELFSECTID_NULL, 0, 0 });
                break;
            }
            case AsmSectionType::GALLIUM_COMMENT:
                output.commentSize = sectionSize;
                output.comment = (const char*)sectionData;
                break;
            default: // ignore other sections
                break;
        }
    }
    
    GPUArchitecture arch = getGPUArchitectureFromDeviceType(assembler.deviceType);
    // set up number of the allocated SGPRs and VGPRs for kernel
    cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    for (size_t i = 0; i < kernelsNum; i++)
    {
        if (!output.kernels[i].useConfig)
            continue;
        GalliumKernelConfig& config = output.kernels[i].config;
        cxuint userSGPRsNum = 4;
        /* include userData sgprs */
        cxuint dimMask = (config.dimMask!=BINGEN_DEFAULT) ? config.dimMask :
                ((config.pgmRSRC2>>7)&7);
        // extra sgprs for dimensions
        cxuint minRegsNum[2];
        getGPUSetupMinRegistersNum(arch, dimMask, userSGPRsNum,
                   ((config.tgSize) ? GPUSETUP_TGSIZE_EN : 0) |
                   ((config.scratchBufferSize!=0) ? GPUSETUP_SCRATCH_EN : 0), minRegsNum);
        
        if (config.usedSGPRsNum!=BINGEN_DEFAULT && maxSGPRsNum < config.usedSGPRsNum)
        {   // check only if sgprsnum set explicitly
            char numBuf[64];
            snprintf(numBuf, 64, "(max %u)", maxSGPRsNum);
            assembler.printError(assembler.kernels[i].sourcePos, (std::string(
                    "Number of total SGPRs for kernel '")+
                    output.kernels[i].kernelName.c_str()+"' is too high "+numBuf).c_str());
            good = false;
        }
        
        if (config.usedSGPRsNum==BINGEN_DEFAULT)
        {
            config.usedSGPRsNum = std::min(
                std::max(minRegsNum[0], kernelStates[i].allocRegs[0]) +
                    getGPUExtraRegsNum(arch, REGTYPE_SGPR, kernelStates[i].allocRegFlags),
                    maxSGPRsNum); // include all extra sgprs
        }
        if (config.usedVGPRsNum==BINGEN_DEFAULT)
            config.usedVGPRsNum = std::max(minRegsNum[1], kernelStates[i].allocRegs[1]);
    }
    
    // if set adds symbols to binary
    if (assembler.getFlags() & ASM_FORCE_ADD_SYMBOLS)
        for (const AsmSymbolEntry& symEntry: assembler.symbolMap)
        {
            if (!symEntry.second.hasValue)
                continue; // unresolved
            if (ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // local
            if (ELF32_ST_BIND(symEntry.second.info) == STB_GLOBAL)
            {
                assembler.printError(AsmSourcePos(), (std::string("Added symbol '")+
                    symEntry.first.c_str()+"' must not be a global").c_str());
                good = false;
                continue; // local
            }
            if (assembler.kernelMap.find(symEntry.first.c_str())!=assembler.kernelMap.end())
                continue; // if kernel name
            cxuint binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            
            output.extraSymbols.push_back({ symEntry.first, symEntry.second.value,
                    symEntry.second.size, binSectId, false, symEntry.second.info,
                    symEntry.second.other });
        }
    
    /// checking symbols and set offset for kernels
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
