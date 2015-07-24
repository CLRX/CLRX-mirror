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

static const char* amdPseudoOpNamesTbl[] =
{
    "arg", "boolconsts", "calnote", "cbid",
    "cbmask", "compile_options", "condout", "config",
    "constantbuffers", "cws", "driver_info", "driver_version",
    "earlyexit", "entry", "floatconsts", "floatmode",
    "globalbuffers", "globaldata", "header", "hwlocal",
    "hwregion", "ieeemode", "inputs", "inputsamplers",
    "intconsts", "metadata", "outputs", "persistentbuffers",
    "pgmrsrc2", "printfid", "privateid", "proginfo",
    "sampler", "scratchbuffer", "scratchbuffers", "segment",
    "sgprsnum", "subconstantbuffers", "uav", "uavid",
    "uavmailboxsize", "uavopmask", "uavprivate", "useconstdata",
    "useprintf", "userdata", "vgprsnum"
};

enum
{
    AMDOP_ARG = 0, AMDOP_BOOLCONSTS, AMDOP_CALNOTE, AMDOP_CBID,
    AMDOP_CBMASK, AMDOP_COMPILE_OPTIONS, AMDOP_CONDOUT, AMDOP_CONFIG,
    AMDOP_CONSTANTBUFFERS, AMDOP_CWS, AMDOP_DRIVER_INFO, AMDOP_DRIVER_VERSION,
    AMDOP_EARLYEXIT, AMDOP_ENTRY, AMDOP_FLOATCONSTS, AMDOP_FLOATMODE,
    AMDOP_GLOBALBUFFERS, AMDOP_GLOBALDATA, AMDOP_HEADER, AMDOP_HWLOCAL,
    AMDOP_HWREGION, AMDOP_IEEEMODE, AMDOP_INPUTS, AMDOP_INPUTSAMPLERS,
    AMDOP_INTCONSTS, AMDOP_METADATA, AMDOP_OUTPUTS, AMDOP_PERSISTENTBUFFERS,
    AMDOP_PGMRSRC2, AMDOP_PRINTFID, AMDOP_PRIVATEID, AMDOP_PROGINFO,
    AMDOP_SAMPLER, AMDOP_SCRATCHBUFFER, AMDOP_SCRATCHBUFFERS, AMDOP_SEGMENT,
    AMDOP_SGPRSNUM, AMDOP_SUBCONSTANTBUFFERS, AMDOP_UAV, AMDOP_UAVID,
    AMDOP_UAVMAILBOXSIZE, AMDOP_UAVOPMASK, AMDOP_UAVPRIVATE, AMDOP_USECONSTDATA,
    AMDOP_USEPRINTF, AMDOP_USERDATA, AMDOP_VGPRSNUM
};

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
    else
        throw AsmFormatException("Section '.text' is already exists");
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

bool AsmRawCodeHandler::parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char* linePtr)
{   // not recognized any pseudo-op
    return false;
}

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
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::DATA, ELFSECTID_UNDEF, nullptr });
    savedSection = 0;
}

void AsmAmdHandler::saveCurrentSection()
{   /// save previous section
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentKernel].savedSection = assembler.currentSection;
}

cxuint AsmAmdHandler::addKernel(const char* kernelName)
{
    cxuint thisKernel = output.kernels.size();
    cxuint thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    kernelStates.push_back({ ASMSECT_NONE, ASMSECT_NONE, ASMSECT_NONE,
            thisSection, ASMSECT_NONE });
    sections.push_back({ thisKernel, AsmSectionType::CODE, ELFSECTID_TEXT, ".text" });
    
    saveCurrentSection();
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    return thisKernel;
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
        kernelStates[kernelId].dataSection = thisSection;
        section.type = AsmSectionType::DATA;
        section.elfBinSectId = ELFSECTID_DATA;
        section.name = ".data"; // set static name (available by whole lifecycle)*/
    }
    else if (::strcmp(sectionName, ".text") == 0) // code
    {
        if (kernelId == ASMKERN_GLOBAL)
            throw AsmFormatException("Section '.text' permitted only inside kernels");
        kernelStates[kernelId].codeSection = thisSection;
        section.type = AsmSectionType::CODE;
        section.elfBinSectId = ELFSECTID_TEXT;
        section.name = ".text"; // set static name (available by whole lifecycle)*/
    }
    else if (kernelId == ASMKERN_GLOBAL)
    {
        auto out = extraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section is already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    else
    {   /* inside kernel binary */
        if (kernelId >= kernelStates.size())
            throw AsmFormatException("KernelId out of range");
        Kernel& kernelState = kernelStates[kernelId];
        auto out = kernelState.extraSectionMap.insert(std::make_pair(
                    std::string(sectionName), thisSection));
        if (!out.second)
            throw AsmFormatException("Section is already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = kernelState.extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    sections.push_back(section);
    
    saveCurrentSection();
    
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
    if (kernel != ASMKERN_GLOBAL && kernel >= kernelStates.size())
        throw AsmFormatException("KernelId out of range");
        
    saveCurrentSection();
    assembler.currentKernel = kernel;
    if (kernel != ASMKERN_GLOBAL)
        assembler.currentSection = kernelStates[kernel].savedSection;
    else
        assembler.currentSection = savedSection;
}

void AsmAmdHandler::setCurrentSection(cxuint sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    saveCurrentSection();
    assembler.currentKernel = sections[sectionId].kernelId;
    assembler.currentSection = sectionId;
}

AsmFormatHandler::SectionInfo AsmAmdHandler::getSectionInfo(cxuint sectionId) const
{   /* find section */
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

void AsmAmdPseudoOps::setCompileOptions(AsmAmdHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (!asmr.parseString(out, linePtr))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.compileOptions = out;
}

void AsmAmdPseudoOps::setDriverInfo(AsmAmdHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (!asmr.parseString(out, linePtr))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.driverInfo = out;
}

void AsmAmdPseudoOps::setDriverVersion(AsmAmdHandler& handler, const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.driverVersion = value;
}

void AsmAmdPseudoOps::doGlobalData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.dataSection==ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ ASMKERN_GLOBAL,  AsmSectionType::DATA,
            ELFSECTID_UNDEF, nullptr });
        handler.dataSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, handler.dataSection);
}

void AsmAmdPseudoOps::addMetadata(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "Metadata can be defined only inside kernel");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel].configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace,
                    "Metadata can't be defined if configuration defined");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint& metadataSection = handler.kernelStates[asmr.currentKernel].metadataSection;
    if (metadataSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMD_METADATA,
            ELFSECTID_UNDEF, nullptr });
        metadataSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, metadataSection);
}

void AsmAmdPseudoOps::doConfig(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "Kernel config can be defined only inside kernel");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    AsmAmdHandler::Kernel& kernel = handler.kernelStates[asmr.currentKernel];
    if (kernel.metadataSection!=ASMSECT_NONE || kernel.headerSection!=ASMSECT_NONE ||
        !kernel.calNoteSections.empty())
    {
        asmr.printError(pseudoOpPlace, "Config can't be defined if metadata,header and/or"
                        " CALnotes section exists");
        return;
    }

    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
        
    if (kernel.configSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::CONFIG,
            ELFSECTID_UNDEF, nullptr });
        kernel.configSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, kernel.configSection);
    handler.output.kernels[asmr.currentKernel].useConfig = true;
}

static const uint32_t singleValueCALNotesMask =
        (1U<<CALNOTE_ATI_EARLYEXIT) | (1U<<CALNOTE_ATI_CONDOUT) |
        (1U<<CALNOTE_ATI_UAV_OP_MASK) | (1U<<CALNOTE_ATI_UAV_MAILBOX_SIZE);

void AsmAmdPseudoOps::addCALNote(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, uint32_t calNoteId)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "CALNote can be defined only inside kernel");
        return;
    }
    AsmAmdHandler::Kernel& kernel = handler.kernelStates[asmr.currentKernel];
    if (kernel.configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace, "CALNote can't be defined if configuration defined");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t value = 0;
    const char* valuePlace = linePtr;
    const bool singleValue = calNoteId < 32 &&
            (singleValueCALNotesMask & (1U<<calNoteId)) && linePtr != end;
    if (singleValue)
    {
        if (!getAbsoluteValueArg(asmr, value, linePtr, false))
            return; // error
        if (value > UINT32_MAX)
            asmr.printWarning(valuePlace, "64-bit CALNoteId has been truncated");
    }
    
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // always add new CALnote
    const cxuint thisSection = handler.sections.size();
    handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMD_CALNOTE,
            ELFSECTID_UNDEF, nullptr, calNoteId });
    kernel.calNoteSections.push_back({thisSection});
    asmr.goToSection(pseudoOpPlace, thisSection);
    if (singleValue)
    {   // with single value
        uint32_t outValue = LEV(uint32_t(value));
        asmr.putData(4, (const cxbyte*)&outValue);
    }
}

void AsmAmdPseudoOps::addHeader(AsmAmdHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL)
    {
        asmr.printError(pseudoOpPlace, "Header can be defined only inside kernel");
        return;
    }
    if (handler.kernelStates[asmr.currentKernel].configSection!=ASMSECT_NONE)
    {
        asmr.printError(pseudoOpPlace, "Header can't be defined if configuration defined");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint& headerSection = handler.kernelStates[asmr.currentKernel].headerSection;
    if (headerSection == ASMSECT_NONE)
    {
        cxuint thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMD_HEADER,
            ELFSECTID_UNDEF, nullptr });
        headerSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, headerSection);
}

void AsmAmdPseudoOps::doEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, uint32_t requiredCalNoteIdMask)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    /* check place where is entry */
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.sections[asmr.currentKernel].type != AsmSectionType::AMD_CALNOTE ||
        (handler.sections[asmr.currentKernel].extraId >= 32 ||
            handler.sections[asmr.currentKernel].extraId & (1U<<requiredCalNoteIdMask)))
    {
        asmr.printError(pseudoOpPlace, "Illegal place of entry");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* value1Place = linePtr;
    uint64_t value1, value2;
    bool good = getAbsoluteValueArg(asmr, value1, linePtr, true);
    
    if (value1 > UINT32_MAX)
        asmr.printWarning(value1Place, "First 64-bit value has been truncated");
    
    if (!skipRequiredComma(asmr, linePtr, "second value"))
        return;
    const char* value2Place = linePtr;
    good &= getAbsoluteValueArg(asmr, value2, linePtr, true);
    if (value2 > UINT32_MAX)
        asmr.printWarning(value2Place, "Second 64-bit value has been truncated");
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    uint32_t outVals[2];
    SLEV(outVals[0], value1);
    SLEV(outVals[1], value2);
    asmr.putData(4, (const cxbyte*)outVals);
}

void AsmAmdPseudoOps::setConfigValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    if ((target != AMDCVAL_HWLOCAL && value > UINT32_MAX))
        asmr.printWarning(valuePlace, "64-bit configuration value has been truncated");
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    // set value
    switch(target)
    {
        case AMDCVAL_SAMPLER:
            config.samplers.push_back(value);
            break;
        case AMDCVAL_SGPRSNUM:
            config.usedSGPRsNum = value;
            break;
        case AMDCVAL_VGPRSNUM:
            config.usedVGPRsNum = value;
            break;
        case AMDCVAL_PGMRSRC2:
            config.pgmRSRC2 = value;
            break;
        case AMDCVAL_IEEEMODE:
            config.ieeeMode = value;
            break;
        case AMDCVAL_FLOATMODE:
            config.floatMode = value;
            break;
        case AMDCVAL_HWLOCAL:
            config.hwLocalSize = value;
            break;
        case AMDCVAL_HWREGION:
            config.hwRegion = value;
            break;
        case AMDCVAL_PRIVATEID:
            config.privateId = value;
            break;
        case AMDCVAL_SCRATCHBUFFER:
            config.scratchBufferSize = value;
            break;
        case AMDCVAL_UAVPRIVATE:
            config.uavPrivate = value;
            break;
        case AMDCVAL_UAVID:
            config.uavId = value;
            break;
        case AMDCVAL_CBID:
            config.constBufferId = value;
            break;
        case AMDCVAL_PRINTFID:
            config.printfId = value;
            break;
        case AMDCVAL_EARLYEXIT:
            config.earlyExit = value;
            break;
        case AMDCVAL_CONDOUT:
            config.condOut = value;
            break;
        default:
            break;
    }
}

void AsmAmdPseudoOps::setConfigBoolValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration pseudo-op");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    if (target == AMDCVAL_USECONSTDATA)
        config.useConstantData = true;
    else if (target == AMDCVAL_USEPRINTF)
        config.usePrintf = true;
}

void AsmAmdPseudoOps::setCWS(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
    {
        asmr.printError(pseudoOpPlace, "Illegal place of configuration value");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t value1 = 1;
    uint64_t value2 = 0;
    uint64_t value3 = 0;
    const char* valuePlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, value1, linePtr, true);
    if (good && value1 > UINT_MAX)
        asmr.printWarning(valuePlace, "64-bit value has been truncated");
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        valuePlace = linePtr;
        good &= getAbsoluteValueArg(asmr, value2, linePtr, false);
        if (value2 > UINT_MAX)
            asmr.printWarning(valuePlace, "64-bit value has been truncated");
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            valuePlace = linePtr;
            good &= getAbsoluteValueArg(asmr, value3, linePtr, false);
            if (value3 > UINT_MAX)
                asmr.printWarning(valuePlace, "64-bit value has been truncated");
        }   
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    config.reqdWorkGroupSize[0] = value1;
    config.reqdWorkGroupSize[1] = value2;
    config.reqdWorkGroupSize[2] = value3;
}

}

bool AsmAmdHandler::parsePseudoOp(const std::string& firstName,
       const char* stmtPlace, const char* linePtr)
{
    const size_t pseudoOp = binaryFind(amdPseudoOpNamesTbl, amdPseudoOpNamesTbl +
                    sizeof(amdPseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - amdPseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case AMDOP_ARG:
            break;
        case AMDOP_BOOLCONSTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_BOOL32CONSTS);
            break;
        case AMDOP_CALNOTE:
            break;
        case AMDOP_CBID:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_CBID);
            break;
        case AMDOP_CBMASK:
            AsmAmdPseudoOps::doEntry(*this, stmtPlace, linePtr,
                         1U<<CALNOTE_ATI_CONSTANT_BUFFERS);
            break;
        case AMDOP_COMPILE_OPTIONS:
            AsmAmdPseudoOps::setCompileOptions(*this, linePtr);
            break;
        case AMDOP_CONDOUT:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_CONDOUT);
            break;
        case AMDOP_CONFIG:
            AsmAmdPseudoOps::doConfig(*this, stmtPlace, linePtr);
            break;
        case AMDOP_CONSTANTBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_CONSTANT_BUFFERS);
            break;
        case AMDOP_CWS:
            AsmAmdPseudoOps::setCWS(*this, stmtPlace, linePtr);
            break;
        case AMDOP_DRIVER_INFO:
            AsmAmdPseudoOps::setDriverInfo(*this, linePtr);
            break;
        case AMDOP_DRIVER_VERSION:
            AsmAmdPseudoOps::setDriverVersion(*this, linePtr);
            break;
        case AMDOP_EARLYEXIT:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_EARLYEXIT);
            break;
        case AMDOP_ENTRY:
            AsmAmdPseudoOps::doEntry(*this, stmtPlace, linePtr,
                         1U<<CALNOTE_ATI_PROGINFO);
            break;
        case AMDOP_FLOATCONSTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_FLOAT32CONSTS);
            break;
        case AMDOP_FLOATMODE:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_FLOATMODE);
            break;
        case AMDOP_GLOBALBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_GLOBAL_BUFFERS);
            break;
        case AMDOP_GLOBALDATA:
            AsmAmdPseudoOps::doGlobalData(*this, stmtPlace, linePtr);
            break;
        case AMDOP_HEADER:
            AsmAmdPseudoOps::addHeader(*this, stmtPlace, linePtr);
            break;
        case AMDOP_HWLOCAL:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_HWLOCAL);
            break;
        case AMDOP_HWREGION:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_HWREGION);
            break;
        case AMDOP_IEEEMODE:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_IEEEMODE);
            break;
        case AMDOP_INPUTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_INPUTS);
            break;
        case AMDOP_INPUTSAMPLERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_INPUT_SAMPLERS);
            break;
        case AMDOP_INTCONSTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_INT32CONSTS);
            break;
        case AMDOP_METADATA:
            AsmAmdPseudoOps::addMetadata(*this, stmtPlace, linePtr);
            break;
        case AMDOP_OUTPUTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_OUTPUTS);
            break;
        case AMDOP_PERSISTENTBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_PERSISTENT_BUFFERS);
            break;
        case AMDOP_PGMRSRC2:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_PGMRSRC2);
            break;
        case AMDOP_PRINTFID:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_PRINTFID);
            break;
        case AMDOP_PRIVATEID:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_PRIVATEID);
            break;
        case AMDOP_PROGINFO:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_PROGINFO);
            break;
        case AMDOP_SAMPLER:
            break;
        case AMDOP_SCRATCHBUFFER:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr,
                                AMDCVAL_SCRATCHBUFFER);
            break;
        case AMDOP_SCRATCHBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_SCRATCH_BUFFERS);
            break;
        case AMDOP_SEGMENT:
            AsmAmdPseudoOps::doEntry(*this, stmtPlace, linePtr,
                    (1U<<CALNOTE_ATI_INT32CONSTS) | (1U<<CALNOTE_ATI_FLOAT32CONSTS) |
                    (1U<<CALNOTE_ATI_BOOL32CONSTS));
            break;
        case AMDOP_SGPRSNUM:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_SGPRSNUM);
            break;
        case AMDOP_SUBCONSTANTBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_SUB_CONSTANT_BUFFERS);
            break;
        case AMDOP_UAV:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_UAV);
            break;
        case AMDOP_UAVID:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_UAVID);
            break;
        case AMDOP_UAVMAILBOXSIZE:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_UAV_MAILBOX_SIZE);
            break;
        case AMDOP_UAVOPMASK:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_UAV_OP_MASK);
            break;
        case AMDOP_UAVPRIVATE:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_UAVPRIVATE);
            break;
        case AMDOP_USECONSTDATA:
            AsmAmdPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                            AMDCVAL_USECONSTDATA);
            break;
        case AMDOP_USEPRINTF:
            AsmAmdPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                            AMDCVAL_USEPRINTF);
            break;
        case AMDOP_USERDATA:
            break;
        case AMDOP_VGPRSNUM:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_VGPRSNUM);
            break;
        default:
            return false;
    }
    return true;
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
    output.kernels.push_back({ kernelName, {
        /* default values */
        { 0x0000b848U, 0x000c0000U },
        { 0x0000b84cU, 0x00001788U },
        { 0x0000b860U, 0 } }, 0, {} });
    /// add kernel config section
    sections.push_back({ thisKernel, AsmSectionType::CONFIG, ELFSECTID_UNDEF, nullptr });
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
    const cxuint thisSection = sections.size();
    Section section;
    section.kernelId = ASMKERN_GLOBAL;
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
            throw AsmFormatException("Only section '.comment' can be in binary");
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
            throw AsmFormatException("Section is already exists");
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
    std::string name;
    bool good = true;
    const char* nameStringPlace = linePtr;
    GalliumArgType argType = GalliumArgType::GLOBAL;
    if (getNameArg(asmr, name, linePtr, "argument type"))
    {
        cxuint index = binaryMapFind(galliumArgTypesMap, galliumArgTypesMap + 9,
                     name.c_str(), CStringLess())-galliumArgTypesMap;
        if (index == 9) // end of this map
        {
            asmr.printError(nameStringPlace, "Unknown argument type");
            good = false;
        }
        else
            argType = galliumArgTypesMap[index].second;
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
    uint64_t targetAlign = (size!=0) ? 1ULL<<(63-CLZ64(size)) : 1;
    if (size > targetAlign)
        targetAlign <<= 1;
    bool sext = false;
    GalliumArgSemantic argSemantic = GalliumArgSemantic::GENERAL;
    
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        const char* targetSizePlace = linePtr;
        targetSize = size;
        good &= getAbsoluteValueArg(asmr, targetSize, linePtr, false);
        
        if (targetSize > UINT32_MAX || targetSize == 0)
            asmr.printWarning(targetSizePlace, "Target size of argument out of range");
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            const char* targetAlignPlace = linePtr;
            good &= getAbsoluteValueArg(asmr, targetAlign, linePtr, false);
            
            if (targetAlign > UINT32_MAX || targetAlign == 0)
                asmr.printWarning(targetAlignPlace,
                                  "Target alignment of argument out of range");
            if (targetAlign != (1ULL<<(63-CLZ64(targetAlign))))
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
                    else if (name != "zext" && !name.empty())
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
                        else if (name != "general" && !name.empty())
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
    handler.kernelStates[asmr.currentKernel].progInfoEntries = 0;
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

bool AsmGalliumHandler::parsePseudoOp(const std::string& firstName,
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
            default: // ignore other sections
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
