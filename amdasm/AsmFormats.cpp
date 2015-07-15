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
                 bool is64Bit): AsmFormatHandler(assembler, deviceType, is64Bit),
                 haveCode(false)
{ }

cxuint AsmRawCodeHandler::addKernel(const char* kernelName)
{
    if (haveCode && !this->kernelName.empty() && this->kernelName != kernelName)
        throw AsmFormatException("Only one kernel can be defined for raw code");
    this->kernelName = kernelName;
    haveCode = true;
    return 0; // default zero kernel
}

cxuint AsmRawCodeHandler::addSection(const char* name, cxuint kernelId)
{
    if (::strcmp(name, ".text")!=0)
        throw AsmFormatException("Only section '.text' can be in raw code");
    haveCode = true;
    return 0;
}

bool AsmRawCodeHandler::sectionIsDefined(const char* sectionName) const
{ return haveCode; }

void AsmRawCodeHandler::setCurrentKernel(cxuint kernel)
{   // do nothing, no checks (assembler checks kernel id before call)
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
    return { ".text", AsmSectionType::CODE, ASMSECT_WRITEABLE };
}

void AsmRawCodeHandler::parsePseudoOp(const std::string firstName,
           const char* stmtStartStr, const char*& string)
{ }  // not recognized any pseudo-op

bool AsmRawCodeHandler::writeBinary(std::ostream& os)
{
    const AsmSection& section = assembler.getSections()[0];
    if (!section.content.empty())
        os.write((char*)section.content.data(), section.content.size());
    return true;
}

/*
 * GalliumCompute format handler
 */

AsmGalliumHandler::AsmGalliumHandler(Assembler& assembler, GPUDeviceType deviceType,
                     bool is64Bit): AsmFormatHandler(assembler, deviceType, is64Bit),
                 codeSection(ASMSECT_NONE), dataSection(ASMSECT_NONE),
                 disasmSection(ASMSECT_NONE), commentSection(ASMSECT_NONE)
{
    insideArgs = insideProgInfo = false;
}

cxuint AsmGalliumHandler::addKernel(const char* kernelName)
{
    input.kernels.push_back({ kernelName, {}, 0, {} });
    cxuint thisKernel = input.kernels.size()-1;
    /// add kernel config section
    cxuint thisSection = sections.size();
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
    currentKernel = kernel;
    currentSection = kernelStates[kernel].defaultSection;
}

void AsmGalliumHandler::setCurrentSection(const char* sectionName)
{
    currentKernel = ASMKERN_GLOBAL;
    if (::strcmp(sectionName, ".data") == 0) // data
        currentSection = dataSection;
    else if (::strcmp(sectionName, ".text") == 0) // code
        currentSection = codeSection;
    else if (::strcmp(sectionName, ".disasm") == 0) // disassembly
        currentSection = disasmSection;
    else if (::strcmp(sectionName, ".comment") == 0) // comment
        currentSection = commentSection;
    else
    {
        std::string message = "Section '";
        message += sectionName;
        message += "' is not supported";
        throw AsmFormatException(message);
    }
    currentKernel = ASMKERN_GLOBAL;
    insideArgs = insideProgInfo = false;
}

AsmFormatHandler::SectionInfo AsmGalliumHandler::getSectionInfo(cxuint sectionId) const
{
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
void AsmFormatPseudoOps::galliumDoArgs(AsmGalliumHandler& handler, const char* pseudoOpStr,
              const char*& string)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (!checkGarbagesAtEnd(asmr, string))
        return;
    if (handler.sections[handler.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpStr, "Arguments outside kernel definition");
        return;
    }
    handler.insideArgs = true;
}

void AsmFormatPseudoOps::galliumDoArg(AsmGalliumHandler& handler, const char* pseudoOpStr,
                      const char*& string)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
}

}

void AsmGalliumHandler::parsePseudoOp(const std::string firstName,
           const char* stmtStartStr, const char*& string)
{
    const size_t pseudoOp = binaryFind(galliumPseudoOpNamesTbl, galliumPseudoOpNamesTbl +
                    sizeof(galliumPseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - galliumPseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case GALLIUMOP_ARG:
            AsmFormatPseudoOps::galliumDoArgs(*this, stmtStartStr, string);
            break;
        case GALLIUMOP_ARGS:
            break;
        case GALLIUMOP_ENTRY:
            break;
        case GALLIUM_PROGINFO:
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
        {
            std::string message = "Symbol for kernel '";
            message += kinput.kernelName;
            message += "' is undefined";
            assembler.printError(assembler.getKernelPosition(ki), message.c_str());
            good = false;
            continue;
        }
        const AsmSymbol& symbol = it->second;
        if (!symbol.hasValue)
        {   // error, undefined
            std::string message = "Symbol for kernel '";
            message += kinput.kernelName;
            message += "' is not resolved";
            assembler.printError(assembler.getKernelPosition(ki), message.c_str());
            good = false;
            continue;
        }
        if (!symbol.sectionId != codeSection)
        {   /// error
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
    
    GalliumBinGenerator binGenerator(&input);
    binGenerator.generate(os);
    return true;
}
