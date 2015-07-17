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

AsmFormatHandler::AsmFormatHandler(Assembler& _assembler, GPUDeviceType _deviceType,
                   bool _is64Bit) : assembler(_assembler), deviceType(_deviceType),
                   _64Bit(_is64Bit), currentKernel(0), currentSection(0)
{ }

AsmFormatHandler::~AsmFormatHandler()
{ }

/* raw code handler */

AsmRawCodeHandler::AsmRawCodeHandler(Assembler& assembler, GPUDeviceType deviceType,
                 bool is64Bit): AsmFormatHandler(assembler, deviceType, is64Bit)
{ }

cxuint AsmRawCodeHandler::addKernel(const char* kernelName)
{
    if (!this->kernelName.empty() && this->kernelName != kernelName)
        throw AsmFormatException("Only one kernel can be defined for raw code");
    this->kernelName = kernelName;
    return 0; // default zero kernel
}

cxuint AsmRawCodeHandler::addSection(const char* name, cxuint kernelId)
{
    if (::strcmp(name, ".text")!=0)
        throw AsmFormatException("Only section '.text' can be in raw code");
    return 0;
}

bool AsmRawCodeHandler::sectionIsDefined(const char* sectionName) const
{ return true; }

void AsmRawCodeHandler::setCurrentKernel(cxuint kernel)
{   // do nothing, no checks (assembler checks kernel id before call)
    if (kernel >= 1)
        throw AsmFormatException("KernelId out of range");
}

void AsmRawCodeHandler::setCurrentSection(const char* name)
{
    if (::strcmp(name, ".text")!=0)
    {
        std::string message = "Section '";
        message += name;
        message += "' doesn't exists";
        throw AsmFormatException(message);
    }
}

AsmFormatHandler::SectionInfo AsmRawCodeHandler::getSectionInfo(cxuint sectionId) const
{
    if (sectionId >= 1)
        throw AsmFormatException("Section doesn't exists");
    return { ".text", AsmSectionType::CODE, ASMSECT_WRITEABLE };
}

void AsmRawCodeHandler::parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char*& string)
{ }  // not recognized any pseudo-op

bool AsmRawCodeHandler::writeBinary(std::ostream& os)
{
    const AsmSection& section = assembler.getSections()[0];
    if (!section.content.empty())
        os.write((char*)section.content.data(), section.content.size());
    return true;
}

/*
 * AmdCatalyst format handler
 */

AsmAmdHandler::AsmAmdHandler(Assembler& assembler, GPUDeviceType deviceType, bool is64Bit)
            : AsmFormatHandler(assembler, deviceType, is64Bit), input{},
              dataSection(0), llvmirSection(ASMSECT_NONE),
              sourceSection(ASMSECT_NONE)
{
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::DATA });
}

cxuint AsmAmdHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = input.kernels.size();
    cxuint thisSection = sections.size();
    input.addEmptyKernel(kernelName);
    kernelStates.push_back({ ASMSECT_NONE, ASMSECT_NONE, ASMSECT_NONE,
            thisSection, ASMSECT_NONE, { } });
    sections.push_back({ thisKernel, AsmSectionType::CODE });
    currentKernel = thisKernel;
    currentSection = thisSection;
    return currentKernel;
}

static const char* amdFormatSectionNamesTbl[] =
{ "config", "data", "header", "llvmir", "metadata", "source", "text", };

enum {
    AMDFMTSECT_CONFIG, AMDFMTSECT_DATA, AMDFMTSECT_HEADER, AMDFMTSECT_LLVMIR,
    AMDFMTSECT_METADATA, AMDFMTSECT_SOURCE, AMDFMTSECT_TEXT
};

cxuint AsmAmdHandler::addSection(const char* name, cxuint kernelId)
{
    if (*name!='.')
    {
        std::string message = "Section '";
        message += name;
        message += "' is not supported";
        throw AsmFormatException(message);
    }
    
    const cxuint thisSection = sections.size();
    size_t sectionNameId = binaryFind(amdFormatSectionNamesTbl, amdFormatSectionNamesTbl +
            sizeof(amdFormatSectionNamesTbl)/sizeof(char*), name+1, CStringLess()) -
            amdFormatSectionNamesTbl;
    
    switch(sectionNameId)
    {
        case AMDFMTSECT_CONFIG:
        {
            if (currentKernel == ASMKERN_GLOBAL)
                throw AsmFormatException("Kernel config must be defined inside kernel");
            AsmAmdHandler::Kernel& kstate = kernelStates[currentKernel];
            if (!kstate.calNoteSections.empty() ||
                kstate.headerSection != ASMSECT_NONE ||
                kstate.metadataSection != ASMSECT_NONE)
                throw AsmFormatException("Config can be defined only if no "
                        "kernel header, metadata, CALNotes");
            kernelStates[currentKernel].configSection = thisSection;
            sections.push_back({ currentKernel, AsmSectionType::CONFIG });
            break;
        }
        case AMDFMTSECT_DATA:
            if (currentKernel == ASMKERN_GLOBAL)
                dataSection = thisSection;
            else
                kernelStates[currentKernel].dataSection = thisSection;
            sections.push_back({ currentKernel, AsmSectionType::DATA });
            break;
        case AMDFMTSECT_HEADER:
            if (currentKernel == ASMKERN_GLOBAL)
                throw AsmFormatException("Kernel header must be defined inside kernel");
            if (kernelStates[currentKernel].configSection != ASMSECT_NONE)
                throw AsmFormatException("Kernel header can be defined only "
                            "if no configuration defined");
            
            kernelStates[currentKernel].headerSection = thisSection;
            sections.push_back({ currentKernel, AsmSectionType::AMD_HEADER });
            break;
        case AMDFMTSECT_LLVMIR:
            currentKernel = ASMKERN_GLOBAL;
            llvmirSection = thisSection;
            sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::AMD_LLVMIR });
            break;
        case AMDFMTSECT_METADATA:
            if (currentKernel == ASMKERN_GLOBAL)
                throw AsmFormatException("Metadata must be defined inside kernel");
            if (kernelStates[currentKernel].configSection != ASMSECT_NONE)
                throw AsmFormatException("Kernel metadata can be defined only "
                            "if no configuration defined");
            
            kernelStates[currentKernel].metadataSection = thisSection;
            sections.push_back({ currentKernel, AsmSectionType::AMD_METADATA});
            break;
        case AMDFMTSECT_SOURCE:
            currentKernel = ASMKERN_GLOBAL;
            sourceSection = thisSection;
            sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::AMD_SOURCE });
            break;
        case AMDFMTSECT_TEXT:
            if (currentKernel == ASMKERN_GLOBAL)
                throw AsmFormatException("Section '.text' must be defined inside kernel");
            kernelStates[currentKernel].codeSection = thisSection;
            sections.push_back({ currentKernel, AsmSectionType::CODE });
            break;
        default:
            std::string message = "Section '";
            message += name;
            message += "' is not supported";
            throw AsmFormatException(message);
            break;
    }
    return 0;
}

bool AsmAmdHandler::sectionIsDefined(const char* name) const
{
    if (*name!='.')
        return false;
    
    size_t sectionNameId = binaryFind(amdFormatSectionNamesTbl, amdFormatSectionNamesTbl +
            sizeof(amdFormatSectionNamesTbl)/sizeof(char*), name+1, CStringLess()) -
            amdFormatSectionNamesTbl;
    switch (sectionNameId)
    {
        case AMDFMTSECT_CONFIG:
            return currentKernel!=ASMKERN_GLOBAL &&
                    kernelStates[currentKernel].configSection!=ASMSECT_NONE;
        case AMDFMTSECT_DATA:
            if (currentKernel!=ASMKERN_GLOBAL)
                return kernelStates[currentKernel].dataSection!=ASMSECT_NONE;
            else
                return dataSection!=ASMSECT_NONE;
        case AMDFMTSECT_HEADER:
            return currentKernel!=ASMKERN_GLOBAL &&
                    kernelStates[currentKernel].headerSection!=ASMSECT_NONE;
        case AMDFMTSECT_LLVMIR:
            return currentKernel==ASMKERN_GLOBAL && llvmirSection!=ASMSECT_NONE;
        case AMDFMTSECT_METADATA:
            return currentKernel!=ASMKERN_GLOBAL &&
                    kernelStates[currentKernel].metadataSection!=ASMSECT_NONE;
        case AMDFMTSECT_SOURCE:
            return currentKernel==ASMKERN_GLOBAL && sourceSection!=ASMSECT_NONE;
        case AMDFMTSECT_TEXT:
            return currentKernel!=ASMKERN_GLOBAL &&
                    kernelStates[currentKernel].codeSection!=ASMSECT_NONE;
        default:
            return false;
    }
}

void AsmAmdHandler::setCurrentKernel(cxuint kernel)
{
    currentKernel = kernel;
    currentSection = kernelStates[kernel].codeSection;
}

void AsmAmdHandler::setCurrentSection(const char* name)
{
    if (*name!='.')
    {
        std::string message = "Section '";
        message += name;
        message += "' is not supported";
        throw AsmFormatException(message);
    }
    
    size_t sectionNameId = binaryFind(amdFormatSectionNamesTbl, amdFormatSectionNamesTbl +
            sizeof(amdFormatSectionNamesTbl)/sizeof(char*), name+1, CStringLess()) -
            amdFormatSectionNamesTbl;
    
    cxuint thisSection = ASMSECT_NONE;
    switch(sectionNameId)
    {
        case AMDFMTSECT_CONFIG:
            if (currentKernel == ASMKERN_GLOBAL)
                throw AsmFormatException("Kernel configuration doesn't "
                            "exists outside kernel");
            thisSection = kernelStates[currentKernel].configSection;
            break;
        case AMDFMTSECT_DATA:
            thisSection = (currentKernel!=ASMKERN_GLOBAL) ?
                    kernelStates[currentKernel].dataSection : dataSection;
            break;
        case AMDFMTSECT_HEADER:
            if (currentKernel == ASMKERN_GLOBAL)
                throw AsmFormatException("Kernel header doesn't exists outside kernel");
            thisSection = kernelStates[currentKernel].headerSection;
            break;
        case AMDFMTSECT_METADATA:
            if (currentKernel == ASMKERN_GLOBAL)
                throw AsmFormatException("Kernel metadata doesn't exists outside kernel");
            thisSection  = kernelStates[currentKernel].metadataSection;
            break;
        case AMDFMTSECT_TEXT:
            if (currentKernel == ASMKERN_GLOBAL)
                throw AsmFormatException("Kernel code doesn't exists outside kernel");
            thisSection = kernelStates[currentKernel].codeSection;
            break;
        case AMDFMTSECT_LLVMIR:
            if (currentKernel != ASMKERN_GLOBAL)
                throw AsmFormatException("LLVMIR bytecode exists only on global space");
            thisSection = llvmirSection;
            break;
        case AMDFMTSECT_SOURCE:
            if (currentKernel != ASMKERN_GLOBAL)
                throw AsmFormatException("Source exists only on global space");
            thisSection = sourceSection;
            break;
        default:
            std::string message = "Section '";
            message += name;
            message += "' is not supported";
            throw AsmFormatException(message);
            break;
    }
    if (thisSection==ASMSECT_NONE)
    {
        std::string message = "Section '";
        message += name;
        message += "' is not defined";
        throw AsmFormatException(message);
    }
    currentSection = thisSection;
}

//static 

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
            flags = ASMSECT_WRITEABLE;
            break;
        case AsmSectionType::DATA:
            name = ".data";
            flags = ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::CONFIG:
            name = ".config";
            break;
        case AsmSectionType::AMD_HEADER:
            name = ".header";
            flags = ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::AMD_METADATA:
            name = ".metadata";
            flags = ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::AMD_LLVMIR:
            name = ".llvmir";
            flags = ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::AMD_SOURCE:
            name = ".source";
            flags = ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        case AsmSectionType::AMD_CALNOTE:
            name = ".calnote";
            flags = ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
            break;
        default:
            // error
            break;
    }
    return { name, section.type, flags };
}

void AsmAmdHandler::parsePseudoOp(const std::string& firstName,
       const char* stmtPlace, const char*& string)
{
}

bool AsmAmdHandler::writeBinary(std::ostream& os)
{   // before call we initialize pointers and datas
    //for (const AsmSection& section: assembler.getSections())
    
    AmdGPUBinGenerator binGenerator(&input);
    binGenerator.generate(os);
    return true;
}

/*
 * GalliumCompute format handler
 */

AsmGalliumHandler::AsmGalliumHandler(Assembler& assembler, GPUDeviceType deviceType,
                     bool is64Bit): AsmFormatHandler(assembler, deviceType, is64Bit),
             input{}, codeSection(ASMSECT_NONE), dataSection(0),
             disasmSection(ASMSECT_NONE), commentSection(ASMSECT_NONE)
{
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::DATA });
    insideArgs = insideProgInfo = false;
}

cxuint AsmGalliumHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = input.kernels.size();
    cxuint thisSection = sections.size();
    input.kernels.push_back({ kernelName, {
        /* default values */
        { 0x0000b848U, 0x000c0000U },
        { 0x0000b84cU, 0x00001788U },
        { 0x0000b860U, 0 } }, 0, {} });
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG });
    kernelStates.push_back({ thisSection, false });
    currentKernel = thisKernel;
    currentSection = thisSection;
    insideArgs = insideProgInfo = false;
    return currentKernel;
}

cxuint AsmGalliumHandler::addSection(const char* sectionName, cxuint kernelId)
{
    const cxuint thisSection = sections.size();
    if (::strcmp(sectionName, ".data") == 0) // data
        dataSection = thisSection;
    else if (::strcmp(sectionName, ".text") == 0) // code
        codeSection = thisSection;
    else if (::strcmp(sectionName, ".disasm") == 0) // disassembly
        disasmSection = thisSection;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        commentSection = thisSection;
    else
    {
        std::string message = "Section '";
        message += sectionName;
        message += "' is not supported";
        throw AsmFormatException(message);
    }
    currentKernel = ASMKERN_GLOBAL;
    currentSection = thisSection;
    insideArgs = insideProgInfo = false;
    return thisSection;
}

bool AsmGalliumHandler::sectionIsDefined(const char* sectionName) const
{
    if (::strcmp(sectionName, ".data") == 0) // data
        return dataSection!=ASMSECT_NONE;
    else if (::strcmp(sectionName, ".text") == 0) // code
        return codeSection!=ASMSECT_NONE;
    else if (::strcmp(sectionName, ".disasm") == 0) // disassembly
        return disasmSection!=ASMSECT_NONE;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        return commentSection!=ASMSECT_NONE;
    else
        return false;
}

void AsmGalliumHandler::setCurrentKernel(cxuint kernel)
{   // set kernel and their default section
    if (kernel >= kernelStates.size())
        throw AsmFormatException("KernelId out of range");
    currentKernel = kernel;
    currentSection = kernelStates[kernel].defaultSection;
}

void AsmGalliumHandler::setCurrentSection(const char* sectionName)
{
    cxuint thisSection = ASMSECT_NONE;
    if (::strcmp(sectionName, ".data") == 0) // data
        thisSection = dataSection;
    else if (::strcmp(sectionName, ".text") == 0) // code
        thisSection = codeSection;
    else if (::strcmp(sectionName, ".disasm") == 0) // disassembly
        thisSection = disasmSection;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        thisSection = commentSection;
    else
    {   /* kernel configuration section has been set via '.kernel' pseudo-op */
        std::string message = "Section '";
        message += sectionName;
        message += "' is not supported";
        throw AsmFormatException(message);
    }
    
    if (thisSection==ASMSECT_NONE)
    {
        std::string message = "Section '";
        message += sectionName;
        message += "' is not defined";
        throw AsmFormatException(message);
    }
    
    currentSection = thisSection;
    currentKernel = ASMKERN_GLOBAL;
    insideArgs = insideProgInfo = false;
}

AsmFormatHandler::SectionInfo AsmGalliumHandler::getSectionInfo(cxuint sectionId) const
{
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    if (sectionId == codeSection)
        return { ".text", AsmSectionType::CODE, ASMSECT_WRITEABLE };
    else if (sectionId == dataSection)
        return { ".data", AsmSectionType::DATA, ASMSECT_WRITEABLE|ASMSECT_ABS_ADDRESSABLE };
    else if (sectionId == commentSection)
        return { ".comment", AsmSectionType::GALLIUM_COMMENT,
            ASMSECT_WRITEABLE|ASMSECT_ABS_ADDRESSABLE };
    else if (sectionId == disasmSection)
        return { ".disasm", AsmSectionType::GALLIUM_DISASM,
            ASMSECT_WRITEABLE|ASMSECT_ABS_ADDRESSABLE };
    else // kernel configuration
        return { ".config", AsmSectionType::CONFIG, 0 };
}

namespace CLRX
{
void AsmGalliumPseudoOps::doArgs(AsmGalliumHandler& handler,
               const char* pseudoOpPlace, const char*& linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (handler.sections[handler.currentSection].type != AsmSectionType::CONFIG)
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
                      const char*& linePtr)
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
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (!haveComma)
    {
        asmr.printError(linePtr, "Expected absolute value");
        return;
    }
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
    
    if (handler.sections[handler.currentSection].type != AsmSectionType::CONFIG)
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
    handler.input.kernels[handler.currentKernel].argInfos.push_back(
        { argType, sext, argSemantic, uint32_t(size),
            uint32_t(targetSize), uint32_t(targetAlign) });
}

void AsmGalliumPseudoOps::doProgInfo(AsmGalliumHandler& handler,
                 const char* pseudoOpPlace, const char*& linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (handler.sections[handler.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo outside kernel definition");
        return;
    }
    handler.insideArgs = false;
    handler.insideProgInfo = true;
    handler.kernelStates[handler.currentKernel].hasProgInfo = true;
    handler.kernelStates[handler.currentKernel].progInfoEntries = 0;
}

void AsmGalliumPseudoOps::doEntry(AsmGalliumHandler& handler,
                    const char* pseudoOpPlace, const char*& linePtr)
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
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (!haveComma)
    {
        asmr.printError(linePtr, "Expected value of entry");
        return;
    }
    
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
    if (handler.sections[handler.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo entry outside kernel configuration");
        return;
    }
    if (!handler.insideProgInfo)
    {
        asmr.printError(pseudoOpPlace, "ProgInfo entry definition outside ProgInfo");
        return;
    }
    AsmGalliumHandler::Kernel& kstate = handler.kernelStates[handler.currentKernel];
    kstate.hasProgInfo = true;
    if (kstate.progInfoEntries == 3)
    {
        asmr.printError(pseudoOpPlace, "Maximum 3 entries can be in ProgInfo");
        return;
    }
    GalliumProgInfoEntry& pentry = handler.input.kernels[handler.currentKernel]
            .progInfo[kstate.progInfoEntries++];
    pentry.address = entryAddr;
    pentry.value = entryVal;
}

}

void AsmGalliumHandler::parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char*& linePtr)
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

bool AsmGalliumHandler::writeBinary(std::ostream& os)
{   // before call we initialize pointers and datas
    for (const AsmSection& section: assembler.getSections())
    {
        switch(section.type)
        {
            case AsmSectionType::CODE:
                input.codeSize = section.content.size();
                input.code = section.content.data();
                break;
            case AsmSectionType::DATA:
                input.globalDataSize = section.content.size();
                input.globalData = section.content.data();
                break;
            case AsmSectionType::GALLIUM_COMMENT:
                input.commentSize = section.content.size();
                input.comment = (const char*)section.content.data();
                break;
            case AsmSectionType::GALLIUM_DISASM:
                input.disassemblySize = section.content.size();
                input.disassembly = (const char*)section.content.data();
                break;
            default:
                abort(); /// fatal error
                break;
        }
    }
    /// checking symbols and set offset for kernels
    bool good = true;
    const AsmSymbolMap& symbolMap = assembler.getSymbolMap();
    for (size_t ki = 0; ki < input.kernels.size(); ki++)
    {
        GalliumKernelInput& kinput = input.kernels[ki];
        auto it = symbolMap.find(kinput.kernelName);
        if (it == symbolMap.end() || !it->second.isDefined())
        {   // error, undefined
            std::string message = "Symbol for kernel '";
            message += kinput.kernelName;
            message += "' is undefined";
            assembler.printError(assembler.getKernelPosition(ki), message.c_str());
            good = false;
            continue;
        }
        const AsmSymbol& symbol = it->second;
        if (!symbol.hasValue)
        {   // error, unresolved
            std::string message = "Symbol for kernel '";
            message += kinput.kernelName;
            message += "' is not resolved";
            assembler.printError(assembler.getKernelPosition(ki), message.c_str());
            good = false;
            continue;
        }
        if (symbol.sectionId != codeSection)
        {   /// error, wrong section
            std::string message = "Symbol for kernel '";
            message += kinput.kernelName;
            message += "' is defined for section other than '.text'";
            assembler.printError(assembler.getKernelPosition(ki), message.c_str());
            good = false;
            continue;
        }
        kinput.offset = symbol.value;
    }
    if (!good) // failed!!!
        return false;
    
    /* initialize progInfos */
    //////////////////////////
    
    GalliumBinGenerator binGenerator(&input);
    binGenerator.generate(os);
    return true;
}
