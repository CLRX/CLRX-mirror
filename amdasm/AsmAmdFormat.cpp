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

#include <CLRX/Config.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include "AsmAmdInternals.h"

using namespace CLRX;

// all AMD Catalyst pseudo-op names (sorted)
static const char* amdPseudoOpNamesTbl[] =
{
    "arg", "boolconsts", "calnote", "cbid",
    "cbmask", "compile_options", "condout", "config",
    "constantbuffers", "cws", "dims", "driver_info", "driver_version",
    "earlyexit", "entry", "exceptions",
    "floatconsts", "floatmode", "get_driver_version",
    "globalbuffers", "globaldata", "header", "hwlocal",
    "hwregion", "ieeemode", "inputs", "inputsamplers",
    "intconsts", "localsize", "metadata", "outputs", "persistentbuffers",
    "pgmrsrc2", "printfid", "privateid", "proginfo",
    "reqd_work_group_size",
    "sampler", "scratchbuffer", "scratchbuffers", "segment",
    "sgprsnum", "subconstantbuffers", "tgsize", "uav", "uavid",
    "uavmailboxsize", "uavopmask", "uavprivate", "useconstdata",
    "useprintf", "userdata", "vgprsnum"
};

// all AMD Catalyst pseudo-op names (sorted)
enum
{
    AMDOP_ARG = 0, AMDOP_BOOLCONSTS, AMDOP_CALNOTE, AMDOP_CBID,
    AMDOP_CBMASK, AMDOP_COMPILE_OPTIONS, AMDOP_CONDOUT, AMDOP_CONFIG,
    AMDOP_CONSTANTBUFFERS, AMDOP_CWS, AMDOP_DIMS, AMDOP_DRIVER_INFO, AMDOP_DRIVER_VERSION,
    AMDOP_EARLYEXIT, AMDOP_ENTRY, AMDOP_EXCEPTIONS,
    AMDOP_FLOATCONSTS, AMDOP_FLOATMODE, AMDOP_GETDRVVER,
    AMDOP_GLOBALBUFFERS, AMDOP_GLOBALDATA, AMDOP_HEADER, AMDOP_HWLOCAL,
    AMDOP_HWREGION, AMDOP_IEEEMODE, AMDOP_INPUTS, AMDOP_INPUTSAMPLERS,
    AMDOP_INTCONSTS, AMDOP_LOCALSIZE, AMDOP_METADATA,
    AMDOP_OUTPUTS,AMDOP_PERSISTENTBUFFERS, AMDOP_PGMRSRC2,
    AMDOP_PRINTFID, AMDOP_PRIVATEID, AMDOP_PROGINFO,
    AMDOP_REQD_WORK_GROUP_SIZE,
    AMDOP_SAMPLER, AMDOP_SCRATCHBUFFER, AMDOP_SCRATCHBUFFERS, AMDOP_SEGMENT,
    AMDOP_SGPRSNUM, AMDOP_SUBCONSTANTBUFFERS, AMDOP_TGSIZE, AMDOP_UAV, AMDOP_UAVID,
    AMDOP_UAVMAILBOXSIZE, AMDOP_UAVOPMASK, AMDOP_UAVPRIVATE, AMDOP_USECONSTDATA,
    AMDOP_USEPRINTF, AMDOP_USERDATA, AMDOP_VGPRSNUM
};

/*
 * AmdCatalyst format handler
 */

AsmAmdHandler::AsmAmdHandler(Assembler& assembler) : AsmFormatHandler(assembler),
                output{}, dataSection(0), extraSectionCount(0)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
    // first, add global data section (will be default)
    sections.push_back({ ASMKERN_GLOBAL, AsmSectionType::DATA, ELFSECTID_UNDEF, nullptr });
    savedSection = 0;
    // detect amd driver version once, for using many times
    detectedDriverVersion = detectAmdDriverVersion();
}

AsmAmdHandler::~AsmAmdHandler()
{
    for (Kernel* kernel: kernelStates)
        delete kernel;
}

// routine to deterime driver version while assemblying
cxuint AsmAmdHandler::determineDriverVersion() const
{
    if (output.driverVersion==0 && output.driverInfo.empty())
    {
        if (assembler.getDriverVersion() == 0)
            return detectedDriverVersion;
        else
            return assembler.getDriverVersion();
    }
    else
        return output.driverVersion;
}

void AsmAmdHandler::saveCurrentSection()
{
    /// save previous section
    if (assembler.currentKernel == ASMKERN_GLOBAL)
        savedSection = assembler.currentSection;
    else
        kernelStates[assembler.currentKernel]->savedSection = assembler.currentSection;
}


void AsmAmdHandler::restoreCurrentAllocRegs()
{
    if (assembler.currentKernel!=ASMKERN_GLOBAL &&
        assembler.currentSection==kernelStates[assembler.currentKernel]->codeSection)
        assembler.isaAssembler->setAllocatedRegisters(
                kernelStates[assembler.currentKernel]->allocRegs,
                kernelStates[assembler.currentKernel]->allocRegFlags);
}

void AsmAmdHandler::saveCurrentAllocRegs()
{
    if (assembler.currentKernel!=ASMKERN_GLOBAL &&
        assembler.currentSection==kernelStates[assembler.currentKernel]->codeSection)
    {
        size_t num;
        cxuint* destRegs = kernelStates[assembler.currentKernel]->allocRegs;
        const cxuint* regs = assembler.isaAssembler->getAllocatedRegisters(num,
                       kernelStates[assembler.currentKernel]->allocRegFlags);
        std::copy(regs, regs + num, destRegs);
    }
}

AsmKernelId AsmAmdHandler::addKernel(const char* kernelName)
{
    AsmKernelId thisKernel = output.kernels.size();
    AsmSectionId thisSection = sections.size();
    output.addEmptyKernel(kernelName);
    /* add new kernel and their section (.text) */
    kernelStates.push_back(new Kernel(thisSection));
    sections.push_back({ thisKernel, AsmSectionType::CODE, ELFSECTID_TEXT, ".text" });
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    
    assembler.currentKernel = thisKernel;
    assembler.currentSection = thisSection;
    assembler.isaAssembler->setAllocatedRegisters();
    return thisKernel;
}

AsmSectionId AsmAmdHandler::addSection(const char* sectionName, AsmKernelId kernelId)
{
    const AsmSectionId thisSection = sections.size();
    Section section;
    section.kernelId = kernelId;
    if (::strcmp(sectionName, ".data") == 0)
    {
        // .data section (only in kernels)
        if (kernelId == ASMKERN_GLOBAL)
            throw AsmFormatException("Section '.data' permitted only inside kernels");
        kernelStates[kernelId]->dataSection = thisSection;
        section.type = AsmSectionType::DATA;
        section.elfBinSectId = ELFSECTID_DATA;
        section.name = ".data"; // set static name (available by whole lifecycle)*/
    }
    else if (kernelId == ASMKERN_GLOBAL)
    {
        // add extra section to main binary
        auto out = extraSectionMap.insert(std::make_pair(std::string(sectionName),
                    thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    else
    {
        /* inside kernel binary */
        if (kernelId >= kernelStates.size())
            throw AsmFormatException("KernelId out of range");
        Kernel& kernelState = *kernelStates[kernelId];
        auto out = kernelState.extraSectionMap.insert(std::make_pair(
                    CString(sectionName), thisSection));
        if (!out.second)
            throw AsmFormatException("Section already exists");
        section.type = AsmSectionType::EXTRA_SECTION;
        section.elfBinSectId = kernelState.extraSectionCount++;
        /// referfence entry is available and unchangeable by whole lifecycle of section map
        section.name = out.first->first.c_str();
    }
    sections.push_back(section);
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    
    assembler.currentKernel = kernelId;
    assembler.currentSection = thisSection;
    
    restoreCurrentAllocRegs();
    return thisSection;
}

AsmSectionId AsmAmdHandler::getSectionId(const char* sectionName) const
{
    if (assembler.currentKernel == ASMKERN_GLOBAL)
    {
        // get extra section from main binary
        SectionMap::const_iterator it = extraSectionMap.find(sectionName);
        if (it != extraSectionMap.end())
            return it->second;
        return ASMSECT_NONE;
    }
    else
    {
        const Kernel& kernelState = *kernelStates[assembler.currentKernel];
        if (::strcmp(sectionName, ".text") == 0)
            return kernelState.codeSection;
        else if (::strcmp(sectionName, ".data") == 0)
            return kernelState.dataSection;
        else
        {
            // if extra section, the find it
            SectionMap::const_iterator it = kernelState.extraSectionMap.find(sectionName);
            if (it != kernelState.extraSectionMap.end())
                return it->second;
        }
        return ASMSECT_NONE;
    }
}

void AsmAmdHandler::setCurrentKernel(AsmKernelId kernel)
{
    if (kernel != ASMKERN_GLOBAL && kernel >= kernelStates.size())
        throw AsmFormatException("KernelId out of range");
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    assembler.currentKernel = kernel;
    if (kernel != ASMKERN_GLOBAL)
        assembler.currentSection = kernelStates[kernel]->savedSection;
    else
        assembler.currentSection = savedSection;
    restoreCurrentAllocRegs();
}

void AsmAmdHandler::setCurrentSection(AsmSectionId sectionId)
{
    if (sectionId >= sections.size())
        throw AsmFormatException("SectionId out of range");
    
    saveCurrentAllocRegs();
    saveCurrentSection();
    assembler.currentKernel = sections[sectionId].kernelId;
    assembler.currentSection = sectionId;
    restoreCurrentAllocRegs();
}

AsmFormatHandler::SectionInfo AsmAmdHandler::getSectionInfo(AsmSectionId sectionId) const
{
    /* find section */
    if (sectionId >= sections.size())
        throw AsmFormatException("Section doesn't exists");
    AsmFormatHandler::SectionInfo info;
    info.type = sections[sectionId].type;
    info.flags = 0;
    // code is addressable and writeable
    if (info.type == AsmSectionType::CODE)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE;
    // any other section (except config) are absolute addressable and writeable
    else if (info.type != AsmSectionType::CONFIG)
        info.flags = ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE | ASMSECT_ABS_ADDRESSABLE;
    info.name = sections[sectionId].name;
    return info;
}

namespace CLRX
{

bool AsmAmdPseudoOps::checkPseudoOpName(const CString& string)
{
    if (string.empty() || string[0] != '.')
        return false;
    const size_t pseudoOp = binaryFind(amdPseudoOpNamesTbl, amdPseudoOpNamesTbl +
                sizeof(amdPseudoOpNamesTbl)/sizeof(char*), string.c_str()+1,
               CStringLess()) - amdPseudoOpNamesTbl;
    return pseudoOp < sizeof(amdPseudoOpNamesTbl)/sizeof(char*);
}

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

void AsmAmdPseudoOps::getDriverVersion(AsmAmdHandler& handler, const char* linePtr)
{
    AsmParseUtils::setSymbolValue(handler.assembler, linePtr,
                handler.determineDriverVersion(), ASMSECT_ABS);
}

void AsmAmdPseudoOps::doGlobalData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (handler.dataSection==ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
    if (asmr.currentKernel==ASMKERN_GLOBAL)
        PSEUDOOP_RETURN_BY_ERROR("Metadata can be defined only inside kernel")
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Metadata can't be defined if configuration was defined")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmSectionId& metadataSection =
            handler.kernelStates[asmr.currentKernel]->metadataSection;
    if (metadataSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
    if (asmr.currentKernel==ASMKERN_GLOBAL)
        PSEUDOOP_RETURN_BY_ERROR("Kernel config can be defined only inside kernel")
    AsmAmdHandler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.metadataSection!=ASMSECT_NONE || kernel.headerSection!=ASMSECT_NONE ||
        !kernel.calNoteSections.empty())
        PSEUDOOP_RETURN_BY_ERROR("Config can't be defined if metadata,header and/or"
                        " CALnotes section exists")

    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
        
    if (kernel.configSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
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
        PSEUDOOP_RETURN_BY_ERROR("CALNote can be defined only inside kernel")
    AsmAmdHandler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.configSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("CALNote can't be defined if configuration was defined")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t value = 0;
    const char* valuePlace = linePtr;
    // check whether CAL note hold only single value
    const bool singleValue = calNoteId < 32 &&
            (singleValueCALNotesMask & (1U<<calNoteId)) && linePtr != end;
    if (singleValue)
    {
        /* if pseudo-op for this calnote accept single 32-bit value */
        if (!getAbsoluteValueArg(asmr, value, linePtr, false))
            return; // error
        asmr.printWarningForRange(32, value, asmr.getSourcePos(valuePlace));
    }
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // always add new CALnote
    const AsmSectionId thisSection = handler.sections.size();
    handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMD_CALNOTE,
            ELFSECTID_UNDEF, nullptr, calNoteId });
    kernel.calNoteSections.push_back({thisSection});
    asmr.goToSection(pseudoOpPlace, thisSection);
    
    if (singleValue)
    {
        // with single value
        uint32_t outValue = LEV(uint32_t(value));
        asmr.putData(4, (const cxbyte*)&outValue);
    }
}

void AsmAmdPseudoOps::addCustomCALNote(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    uint64_t value;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL)
        PSEUDOOP_RETURN_BY_ERROR("CALNote can be defined only inside kernel")
    AsmAmdHandler::Kernel& kernel = *handler.kernelStates[asmr.currentKernel];
    if (kernel.configSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("CALNote can't be defined if configuration was defined")
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    asmr.printWarningForRange(32, value, asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    // resue code in addCALNote
    addCALNote(handler, pseudoOpPlace, linePtr, value);
}

void AsmAmdPseudoOps::addHeader(AsmAmdHandler& handler, const char* pseudoOpPlace,
                  const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL)
        PSEUDOOP_RETURN_BY_ERROR("Header can be defined only inside kernel")
    if (handler.kernelStates[asmr.currentKernel]->configSection!=ASMSECT_NONE)
        PSEUDOOP_RETURN_BY_ERROR("Header can't be defined if configuration was defined")
    
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AsmSectionId& headerSection = handler.kernelStates[asmr.currentKernel]->headerSection;
    if (headerSection == ASMSECT_NONE)
    {
        /* add this section */
        AsmSectionId thisSection = handler.sections.size();
        handler.sections.push_back({ asmr.currentKernel, AsmSectionType::AMD_HEADER,
            ELFSECTID_UNDEF, nullptr });
        headerSection = thisSection;
    }
    asmr.goToSection(pseudoOpPlace, headerSection);
}

void AsmAmdPseudoOps::doEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
              const char* linePtr, uint32_t requiredCalNoteIdMask, const char* entryName)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    /* check place where is entry */
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        handler.sections[asmr.currentSection].type != AsmSectionType::AMD_CALNOTE ||
        (handler.sections[asmr.currentSection].extraId >= 32 ||
        ((1U<<handler.sections[asmr.currentSection].extraId) & requiredCalNoteIdMask))==0)
        PSEUDOOP_RETURN_BY_ERROR((std::string("Illegal place of ")+entryName).c_str())
    
    if (handler.sections[asmr.currentSection].extraId == CALNOTE_ATI_UAV)
    {
        // special version for uav (four values per entry)
        doUavEntry(handler, pseudoOpPlace, linePtr);
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    const char* value1Place = linePtr;
    uint64_t value1 = 0, value2 = 0;
    // parse address (first value)
    bool good = getAbsoluteValueArg(asmr, value1, linePtr, true);
    if (good)
        asmr.printWarningForRange(32, value1, asmr.getSourcePos(value1Place));
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    const char* value2Place = linePtr;
    // parse value (second value)
    if (getAbsoluteValueArg(asmr, value2, linePtr, true))
        asmr.printWarningForRange(32, value2, asmr.getSourcePos(value2Place));
    else
        good = false;
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    uint32_t outVals[2];
    SLEV(outVals[0], value1);
    SLEV(outVals[1], value2);
    asmr.putData(8, (const cxbyte*)outVals);
}

void AsmAmdPseudoOps::doUavEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value1 = 0, value2 = 0, value3 = 0, value4 = 0;
    bool good = getAbsoluteValueArg(asmr, value1, linePtr, true);
    if (good)
        asmr.printWarningForRange(32, value1, asmr.getSourcePos(valuePlace));
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    skipSpacesToEnd(linePtr, end);
    valuePlace = linePtr;
    if (getAbsoluteValueArg(asmr, value2, linePtr, true))
        asmr.printWarningForRange(32, value2, asmr.getSourcePos(valuePlace));
    else
        good = false;
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    skipSpacesToEnd(linePtr, end);
    valuePlace = linePtr;
    if (getAbsoluteValueArg(asmr, value3, linePtr, true))
        asmr.printWarningForRange(32, value3, asmr.getSourcePos(valuePlace));
    else
        good = false;
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    skipSpacesToEnd(linePtr, end);
    valuePlace = linePtr;
    if (getAbsoluteValueArg(asmr, value4, linePtr, true))
        asmr.printWarningForRange(32, value4, asmr.getSourcePos(valuePlace));
    else
        good = false;
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    uint32_t outVals[4];
    SLEV(outVals[0], value1);
    SLEV(outVals[1], value2);
    SLEV(outVals[2], value3);
    SLEV(outVals[3], value4);
    asmr.putData(16, (const cxbyte*)outVals);
}

void AsmAmdPseudoOps::doCBId(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel!=ASMKERN_GLOBAL &&
        asmr.sections[asmr.currentSection].type == AsmSectionType::CONFIG)
        setConfigValue(handler, pseudoOpPlace, linePtr, AMDCVAL_CBID);
    else // do entry (if no configuration)
        doEntry(handler, pseudoOpPlace, linePtr, 1U<<CALNOTE_ATI_CONSTANT_BUFFERS, "cbid");
}

void AsmAmdPseudoOps::doCondOut(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel!=ASMKERN_GLOBAL &&
        asmr.sections[asmr.currentSection].type == AsmSectionType::CONFIG)
        setConfigValue(handler, pseudoOpPlace, linePtr, AMDCVAL_CONDOUT);
    else // make calnote CONDOUT
        addCALNote(handler, pseudoOpPlace, linePtr, CALNOTE_ATI_CONDOUT);
}

void AsmAmdPseudoOps::doEarlyExit(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel!=ASMKERN_GLOBAL &&
        asmr.sections[asmr.currentSection].type == AsmSectionType::CONFIG)
        setConfigValue(handler, pseudoOpPlace, linePtr, AMDCVAL_EARLYEXIT);
    else // make calnote EARLYEXIT
        addCALNote(handler, pseudoOpPlace, linePtr, CALNOTE_ATI_EARLYEXIT);
}

void AsmAmdPseudoOps::doSampler(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel!=ASMKERN_GLOBAL &&
        asmr.sections[asmr.currentSection].type == AsmSectionType::CONFIG)
    {
        // accepts many values (this same format like
        const char* end = asmr.line + asmr.lineSize;
        
        if (asmr.currentKernel==ASMKERN_GLOBAL ||
            asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
            PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
        
        AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end)
            return; /* if no samplers */
        do {
            uint64_t value = 0;
            const char* valuePlace = linePtr;
            if (getAbsoluteValueArg(asmr, value, linePtr, true))
            {
                asmr.printWarningForRange(sizeof(cxuint)<<3, value,
                                 asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                config.samplers.push_back(value);
            }
        } while(skipCommaForMultipleArgs(asmr, linePtr));
        checkGarbagesAtEnd(asmr, linePtr);
    }
    else // add entry to input samplers CALNote
        doEntry(handler, pseudoOpPlace, linePtr, 1U<<CALNOTE_ATI_INPUT_SAMPLERS, "sampler");
}

static const uint32_t argIsOptionalMask =  (1U<<AMDCVAL_PRIVATEID) |
        (1U<<AMDCVAL_UAVID) | (1U<<AMDCVAL_CBID) | (1U<<AMDCVAL_PRINTFID);

void AsmAmdPseudoOps::setConfigValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    uint64_t value = BINGEN_NOTSUPPLIED;
    const bool argIsOptional = ((1U<<target) & argIsOptionalMask)!=0;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, !argIsOptional);
    /* ranges checking */
    if (good)
    {
        switch(target)
        {
            case AMDCVAL_SGPRSNUM:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxSGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR,
                          (asmr.policyVersion<ASM_POLICY_UNIFIED_SGPR_COUNT ?
                          REGCOUNT_NO_VCC : 0));
                if (value > maxSGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Used SGPRs number out of range (0-%u)", maxSGPRsNum);
                    ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
                }
                break;
            }
            case AMDCVAL_VGPRSNUM:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                cxuint maxVGPRsNum = getGPUMaxRegistersNum(arch, REGTYPE_VGPR, 0);
                if (value > maxVGPRsNum)
                {
                    char buf[64];
                    snprintf(buf, 64, "Used VGPRs number out of range (0-%u)", maxVGPRsNum);
                    ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
                }
                break;
            }
            case AMDCVAL_HWLOCAL:
            {
                const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                            asmr.deviceType);
                const cxuint maxLocalSize = getGPUMaxLocalSize(arch);
                if (value > maxLocalSize)
                {
                    char buf[64];
                    snprintf(buf, 64, "HWLocalSize out of range (0-%u)", maxLocalSize);
                    ASM_NOTGOOD_BY_ERROR(valuePlace, buf)
                }
                break;
            }
            case AMDCVAL_FLOATMODE:
                asmr.printWarningForRange(8, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0xff;
                break;
            case AMDCVAL_EXCEPTIONS:
                asmr.printWarningForRange(7, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                value &= 0x7f;
                break;
            case AMDCVAL_UAVID:
                if (value != BINGEN_NOTSUPPLIED && value >= 1024)
                    ASM_NOTGOOD_BY_ERROR(valuePlace, "UAVId out of range (0-1023)")
                break;
            case AMDCVAL_CBID:
                if (value != BINGEN_NOTSUPPLIED && value >= 1024)
                    ASM_NOTGOOD_BY_ERROR(valuePlace, "ConstBufferId out of range (0-1023)")
                break;
            case AMDCVAL_PRINTFID:
                if (value != BINGEN_NOTSUPPLIED && value >= 1024)
                    ASM_NOTGOOD_BY_ERROR(valuePlace, "PrintfId out of range (0-1023)")
                break;
            case AMDCVAL_PRIVATEID:
                if (value != BINGEN_NOTSUPPLIED && value >= 1024)
                    ASM_NOTGOOD_BY_ERROR(valuePlace, "PrivateId out of range (0-1023)")
                break;
            case AMDCVAL_CONDOUT:
            case AMDCVAL_EARLYEXIT:
            case AMDCVAL_HWREGION:
            case AMDCVAL_PGMRSRC2:
                asmr.printWarningForRange(32, value,
                                  asmr.getSourcePos(valuePlace), WS_UNSIGNED);
                break;
            default:
                asmr.printWarningForRange(32, value, asmr.getSourcePos(valuePlace));
                break;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    // set value
    switch(target)
    {
        case AMDCVAL_SGPRSNUM:
            if (asmr.policyVersion < ASM_POLICY_UNIFIED_SGPR_COUNT)
                config.usedSGPRsNum = value;
            else
                config.usedSGPRsNum = std::max(value, uint64_t(2))-2;
            break;
        case AMDCVAL_VGPRSNUM:
            config.usedVGPRsNum = value;
            break;
        case AMDCVAL_PGMRSRC2:
            config.pgmRSRC2 = value;
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
        case AMDCVAL_EXCEPTIONS:
            config.exceptions = value;
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
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    if (target == AMDCVAL_USECONSTDATA)
        config.useConstantData = true;
    else if (target == AMDCVAL_USEPRINTF)
        config.usePrintf = true;
    else if (target == AMDCVAL_IEEEMODE)
        config.ieeeMode = true;
    else if (target == AMDCVAL_TGSIZE)
        config.tgSize = true;
}

bool AsmAmdPseudoOps::parseCWS(Assembler& asmr, const char* pseudoOpPlace,
              const char* linePtr, uint64_t* out)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    // default value is (1,1,1)
    out[0] = 1;
    out[1] = 1;
    out[2] = 1;
    const char* valuePlace1 = linePtr;
    const char* valuePlace2 = nullptr;
    const char* valuePlace3 = nullptr;
    bool good = getAbsoluteValueArg(asmr, out[0], linePtr, false);
    if (good)
        asmr.printWarningForRange(32, out[0], asmr.getSourcePos(valuePlace1), WS_UNSIGNED);
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return false;
    if (haveComma)
    {
        // second and third argument is optional
        skipSpacesToEnd(linePtr, end);
        valuePlace2 = linePtr;
        if (getAbsoluteValueArg(asmr, out[1], linePtr, false))
            asmr.printWarningForRange(32, out[1], asmr.getSourcePos(valuePlace2),
                              WS_UNSIGNED);
        else
            good = false;
        
        if (!skipComma(asmr, haveComma, linePtr))
            return false;
        if (haveComma)
        {
            valuePlace3 = linePtr;
            if (getAbsoluteValueArg(asmr, out[2], linePtr, false))
                asmr.printWarningForRange(32, out[2], asmr.getSourcePos(valuePlace3),
                            WS_UNSIGNED);
            else
                good = false;
        }
    }
    if (out[0] != 0 || out[1]!=0 || out[2]!=0)
    {
        if (out[0] == 0)
            ASM_NOTGOOD_BY_ERROR(valuePlace1, "Group size comment must be not zero");
        if (out[1] == 0)
            ASM_NOTGOOD_BY_ERROR(valuePlace2, "Group size comment must be not zero");
        if (out[2] == 0)
            ASM_NOTGOOD_BY_ERROR(valuePlace3, "Group size comment must be not zero");
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    return true;
}

void AsmAmdPseudoOps::setCWS(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    
    uint64_t out[3] = { 1, 1, 1 };
    if (!parseCWS(asmr, pseudoOpPlace, linePtr, out))
        return;
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    config.reqdWorkGroupSize[0] = out[0];
    config.reqdWorkGroupSize[1] = out[1];
    config.reqdWorkGroupSize[2] = out[2];
}

/// data class names
static const std::pair<const char*, cxuint> dataClassMap[] =
{
    { "imm_alu_bool32_const", 0x06 },
    { "imm_alu_float_const", 0x05 },
    { "imm_const_buffer", 0x02 },
    { "imm_context_base", 0x1d },
    { "imm_dispatch_id", 0x0c },
    { "imm_gds_counter_range", 0x07 },
    { "imm_gds_memory_range", 0x08 },
    { "imm_generic_user_data", 0x20 },
    { "imm_global_offset", 0x1f },
    { "imm_gws_base", 0x09 },
    { "imm_heap_buffer", 0x0e },
    { "imm_kernel_arg", 0x0f },
    { "imm_lds_esgs_size", 0x1e },
    { "imm_resource", 0x00 },
    { "imm_sampler", 0x01 },
    { "imm_scratch_buffer", 0x0d },
    { "imm_uav", 0x04 },
    { "imm_vertex_buffer", 0x03 },
    { "imm_work_group_range", 0x0b },
    { "imm_work_item_range", 0x0a },
    { "ptr_const_buffer_table", 0x14 },
    { "ptr_extended_user_data", 0x19 },
    { "ptr_indirect_internal_resource", 0x1b },
    { "ptr_indirect_resource", 0x1a },
    { "ptr_indirect_uav", 0x1c },
    { "ptr_internal_global_table", 0x18 },
    { "ptr_internal_resource_table", 0x12 },
    { "ptr_resource_table", 0x11 },
    { "ptr_sampler_table", 0x13 },
    { "ptr_so_buffer_table", 0x16 },
    { "ptr_uav_table", 0x17 },
    { "ptr_vertex_buffer_table", 0x15 },
    { "sub_ptr_fetch_shader", 0x10 }
};

static const size_t dataClassMapSize = sizeof(dataClassMap) /
        sizeof(std::pair<const char*, uint32_t>);

void AsmAmdPseudoOps::addUserData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of UserData")
    
    cxuint dataClass = 0;
    bool good = true;
    // parse user data class
    good &= getEnumeration(asmr, linePtr, "Data Class", dataClassMapSize,
                    dataClassMap, dataClass);
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    uint64_t apiSlot = 0;
    const char* apiSlotPlace = linePtr;
    // api slot (is 32-bit value)
    if (getAbsoluteValueArg(asmr, apiSlot, linePtr, true))
        asmr.printWarningForRange(32, apiSlot, asmr.getSourcePos(apiSlotPlace),
                                  WS_UNSIGNED);
    else
        good = false;
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    uint64_t regStart = 0;
    const char* regStartPlace = linePtr;
    if (getAbsoluteValueArg(asmr, regStart, linePtr, true))
    {
        // must be in (0-15)
        if (regStart > 15)
            ASM_NOTGOOD_BY_ERROR(regStartPlace, "RegStart out of range (0-15)")
    }
    else
        good = false;
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    uint64_t regSize = 0;
    if (getAbsoluteValueArg(asmr, regSize, linePtr, true))
    {
        if (usumGt(regStart, regSize, 16U))
            ASM_NOTGOOD_BY_ERROR(regStartPlace, "RegStart+RegSize out of range (0-16)")
    }
    else
        good = false;
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    if (config.userDatas.size() == 16)
        PSEUDOOP_RETURN_BY_ERROR("Too many UserData elements")
    AmdUserData userData;
    userData.dataClass = dataClass;
    userData.apiSlot = apiSlot;
    userData.regStart = regStart;
    userData.regSize = regSize;
    config.userDatas.push_back(userData);
}

void AsmAmdPseudoOps::setDimensions(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of configuration pseudo-op")
    cxuint dimMask = 0;
    if (!parseDimensions(asmr, linePtr, dimMask, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    handler.output.kernels[asmr.currentKernel].config.dimMask = dimMask;
}

/* argument type map - value is cxuint for getEnumaration */
static const std::pair<const char*, cxuint> argTypeNameMap[] =
{
    { "char", cxuint(KernelArgType::CHAR) },
    { "char16", cxuint(KernelArgType::CHAR16) },
    { "char2", cxuint(KernelArgType::CHAR2) },
    { "char3", cxuint(KernelArgType::CHAR3) },
    { "char4", cxuint(KernelArgType::CHAR4) },
    { "char8", cxuint(KernelArgType::CHAR8) },
    { "clkevent", cxuint(KernelArgType::CLKEVENT) },
    { "counter32", cxuint(KernelArgType::COUNTER32) },
    { "counter64", cxuint(KernelArgType::COUNTER64) },
    { "double", cxuint(KernelArgType::DOUBLE) },
    { "double16", cxuint(KernelArgType::DOUBLE16) },
    { "double2", cxuint(KernelArgType::DOUBLE2) },
    { "double3", cxuint(KernelArgType::DOUBLE3) },
    { "double4", cxuint(KernelArgType::DOUBLE4) },
    { "double8", cxuint(KernelArgType::DOUBLE8) },
    { "float", cxuint(KernelArgType::FLOAT) },
    { "float16", cxuint(KernelArgType::FLOAT16) },
    { "float2", cxuint(KernelArgType::FLOAT2) },
    { "float3", cxuint(KernelArgType::FLOAT3) },
    { "float4", cxuint(KernelArgType::FLOAT4) },
    { "float8", cxuint(KernelArgType::FLOAT8) },
    { "image", cxuint(KernelArgType::IMAGE) },
    { "image1d", cxuint(KernelArgType::IMAGE1D) },
    { "image1d_array", cxuint(KernelArgType::IMAGE1D_ARRAY) },
    { "image1d_buffer", cxuint(KernelArgType::IMAGE1D_BUFFER) },
    { "image2d", cxuint(KernelArgType::IMAGE2D) },
    { "image2d_array", cxuint(KernelArgType::IMAGE2D_ARRAY) },
    { "image3d", cxuint(KernelArgType::IMAGE3D) },
    { "int", cxuint(KernelArgType::INT) },
    { "int16", cxuint(KernelArgType::INT16) },
    { "int2", cxuint(KernelArgType::INT2) },
    { "int3", cxuint(KernelArgType::INT3) },
    { "int4", cxuint(KernelArgType::INT4) },
    { "int8", cxuint(KernelArgType::INT8) },
    { "long", cxuint(KernelArgType::LONG) },
    { "long16", cxuint(KernelArgType::LONG16) },
    { "long2", cxuint(KernelArgType::LONG2) },
    { "long3", cxuint(KernelArgType::LONG3) },
    { "long4", cxuint(KernelArgType::LONG4) },
    { "long8", cxuint(KernelArgType::LONG8) },
    { "pipe", cxuint(KernelArgType::PIPE) },
    { "queue", cxuint(KernelArgType::CMDQUEUE) },
    { "sampler", cxuint(KernelArgType::SAMPLER) },
    { "short", cxuint(KernelArgType::SHORT) },
    { "short16", cxuint(KernelArgType::SHORT16) },
    { "short2", cxuint(KernelArgType::SHORT2) },
    { "short3", cxuint(KernelArgType::SHORT3) },
    { "short4", cxuint(KernelArgType::SHORT4) },
    { "short8", cxuint(KernelArgType::SHORT8) },
    { "structure", cxuint(KernelArgType::STRUCTURE) },
    { "uchar", cxuint(KernelArgType::UCHAR) },
    { "uchar16", cxuint(KernelArgType::UCHAR16) },
    { "uchar2", cxuint(KernelArgType::UCHAR2) },
    { "uchar3", cxuint(KernelArgType::UCHAR3) },
    { "uchar4", cxuint(KernelArgType::UCHAR4) },
    { "uchar8", cxuint(KernelArgType::UCHAR8) },
    { "uint", cxuint(KernelArgType::UINT) },
    { "uint16", cxuint(KernelArgType::UINT16) },
    { "uint2", cxuint(KernelArgType::UINT2) },
    { "uint3", cxuint(KernelArgType::UINT3) },
    { "uint4", cxuint(KernelArgType::UINT4) },
    { "uint8", cxuint(KernelArgType::UINT8) },
    { "ulong", cxuint(KernelArgType::ULONG)},
    { "ulong16", cxuint(KernelArgType::ULONG16) },
    { "ulong2", cxuint(KernelArgType::ULONG2) },
    { "ulong3", cxuint(KernelArgType::ULONG3) },
    { "ulong4", cxuint(KernelArgType::ULONG4) },
    { "ulong8", cxuint(KernelArgType::ULONG8) },
    { "ushort", cxuint(KernelArgType::USHORT) },
    { "ushort16", cxuint(KernelArgType::USHORT16) },
    { "ushort2", cxuint(KernelArgType::USHORT2) },
    { "ushort3", cxuint(KernelArgType::USHORT3) },
    { "ushort4", cxuint(KernelArgType::USHORT4) },
    { "ushort8", cxuint(KernelArgType::USHORT8) },
    { "void", cxuint(KernelArgType::VOID) }
};

static const char* defaultArgTypeNames[] = 
{
    "void", "uchar", "char", "ushort", "short", "uint", "int",
    "ulong", "long", "float", "double", "pointer", "image2d_t",
    "image1d_t", "image1d_array_t", "image1d_buffer_t",
    "image2d_t", "image2d_array_t", "image3d_t",
    "uchar2", "uchar3", "uchar4", "uchar8", "uchar16",
    "char2", "char3", "char4", "char8", "char16",
    "ushort2", "ushort3", "ushort4", "ushort8", "ushort16",
    "short2", "short3", "short4", "short8", "short16",
    "uint2", "uint3", "uint4", "uint8", "uint16",
    "int2", "int3", "int4", "int8", "int16",
    "ulong2", "ulong3", "ulong4", "ulong8", "ulong16",
    "long2", "long3", "long4", "long8", "long16",
    "float2", "float3", "float4", "float8", "float16",
    "double2", "double3", "double4", "double8", "double16",
    "sampler_t", "structure", "counter32_t", "counter64_t",
    "pipe", "queue_t", "clk_event_t"
};

static const size_t argTypeNameMapSize = sizeof(argTypeNameMap) /
        sizeof(std::pair<const char*, KernelArgType>);

// main routine to parse argument
bool AsmAmdPseudoOps::parseArg(Assembler& asmr, const char* pseudoOpPlace,
          const char* linePtr, const std::unordered_set<CString>& argNamesSet,
          AmdKernelArgInput& argInput, bool cl20)
{
    CString argName;
    const char* end = asmr.line + asmr.lineSize;
    const char* argNamePlace = linePtr;
    
    bool good = getNameArg(asmr, argName, linePtr, "argument name", true);
    if (argNamesSet.find(argName) != argNamesSet.end())
        // if found kernel arg with this same name
        ASM_NOTGOOD_BY_ERROR(argNamePlace, (std::string("Kernel argument '")+
                    argName.c_str()+"' is already defined").c_str())
    
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    skipSpacesToEnd(linePtr, end);
    bool typeNameDefined = false;
    std::string typeName;
    if (linePtr!=end && *linePtr=='"')
    {
        // if type name defined by user
        good &= asmr.parseString(typeName, linePtr);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        typeNameDefined = true;
    }
    
    bool pointer = false;
    KernelArgType argType = KernelArgType::VOID;
    char name[20];
    skipSpacesToEnd(linePtr, end);
    const char* argTypePlace = linePtr;
    cxuint argTypeValue;
    if (getEnumeration(asmr, linePtr, "argument type", argTypeNameMapSize, argTypeNameMap,
                    argTypeValue))
    {
        argType = KernelArgType(argTypeValue);
        if (cl20 || argType <= KernelArgType::MAX_VALUE)
        {
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr == '*')
            {
                pointer = true; // if is pointer
                linePtr++;
            }
        }
        else
        {
            // if not OpenCL 2.0 and argument type only present in OpenCL 2.0
            skipSpacesToEnd(linePtr, end);
            ASM_NOTGOOD_BY_ERROR(linePtr, "Unknown argument type")
        }
    }
    else // if failed
        good = false;
    
    if (!pointer && argType == KernelArgType::COUNTER64)
        ASM_NOTGOOD_BY_ERROR(argTypePlace, "Unsupported counter64 type")
    if (pointer && (isKernelArgImage(argType) ||
            argType == KernelArgType::SAMPLER || argType == KernelArgType::POINTER ||
            argType == KernelArgType::COUNTER32 || argType == KernelArgType::COUNTER64))
        ASM_NOTGOOD_BY_ERROR(argTypePlace, "Illegal pointer type")
    
    if (!typeNameDefined)
    {
        // if type name is not supplied
        typeName = defaultArgTypeNames[cxuint(argType)];
        if (pointer)
            typeName.push_back('*');
    }
    
    KernelPtrSpace ptrSpace = KernelPtrSpace::NONE;
    cxbyte ptrAccess = 0;
    uint64_t structSizeVal = 0;
    uint64_t constSpaceSizeVal = 0;
    uint64_t resIdVal = BINGEN_DEFAULT;
    cxbyte usedArg = (cl20) ? 3 : 1;
    
    bool haveComma;
    bool haveLastArgument = false;
    if (pointer)
    {
        // if type is pointer
        if (argType == KernelArgType::STRUCTURE)
        {
            if (!skipRequiredComma(asmr, linePtr))
                return false;
            skipSpacesToEnd(linePtr, end);
            // parse extra structure size
            const char* structSizePlace = linePtr;
            if (getAbsoluteValueArg(asmr, structSizeVal, linePtr, true))
                asmr.printWarningForRange(sizeof(cxuint)<<3, structSizeVal,
                                  asmr.getSourcePos(structSizePlace), WS_UNSIGNED);
            else
                good = false;
        }
        
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        skipSpacesToEnd(linePtr, end);
        const char* ptrSpacePlace = linePtr;
        // parse ptrSpace
        if (getNameArg(asmr, 10, name, linePtr, "pointer space", true))
        {
            toLowerString(name);
            if (::strcmp(name, "local")==0)
                ptrSpace = KernelPtrSpace::LOCAL;
            else if (::strcmp(name, "global")==0)
                ptrSpace = KernelPtrSpace::GLOBAL;
            else if (::strcmp(name, "constant")==0)
                ptrSpace = KernelPtrSpace::CONSTANT;
            else // not known or not given
                ASM_NOTGOOD_BY_ERROR(ptrSpacePlace, "Unknown pointer space")
        }
        else
            good = false;
        
        if (!skipComma(asmr, haveComma, linePtr))
            return false;
        if (haveComma)
        {
            // parse ptr access
            while (linePtr!=end && *linePtr!=',')
            {
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end || *linePtr==',')
                    break;
                const char* ptrAccessPlace = linePtr;
                // parse acces qualifier (const,restrict,volatile)
                if (getNameArg(asmr, 10, name, linePtr, "access qualifier", true))
                {
                    if (::strcasecmp(name, "const")==0)
                        ptrAccess |= KARG_PTR_CONST;
                    else if (::strcasecmp(name, "restrict")==0)
                        ptrAccess |= KARG_PTR_RESTRICT;
                    else if (::strcasecmp(name, "volatile")==0)
                        ptrAccess |= KARG_PTR_VOLATILE;
                    else
                        ASM_NOTGOOD_BY_ERROR(ptrAccessPlace, "Unknown access qualifier")
                }
                else
                    good = false;
            }
            
            bool havePrevArgument = false;
            const char* place;
            if (ptrSpace == KernelPtrSpace::CONSTANT && !cl20)
            {
                /* parse constant space size for constant pointer */
                if (!skipComma(asmr, haveComma, linePtr))
                    return false;
                if (haveComma)
                {
                    skipSpacesToEnd(linePtr, end);
                    const char* place = linePtr;
                    if (getAbsoluteValueArg(asmr, constSpaceSizeVal, linePtr, false))
                        asmr.printWarningForRange(sizeof(cxuint)<<3, constSpaceSizeVal,
                                          asmr.getSourcePos(place), WS_UNSIGNED);
                    else
                        good = false;
                    havePrevArgument = true;
                }
            }
            else
                havePrevArgument = true;
            
            if (havePrevArgument && ptrSpace != KernelPtrSpace::LOCAL && !cl20)
            {
                /* global and constant have resource id (uavId) */
                if (!skipComma(asmr, haveComma, linePtr))
                    return false;
                if (haveComma)
                {
                    haveLastArgument = true;
                    skipSpacesToEnd(linePtr, end);
                    place = linePtr;
                    if (getAbsoluteValueArg(asmr, resIdVal, linePtr, false))
                    {
                        // for constant buffers, uavid is in range (0-159)
                        const cxuint maxUavId = (ptrSpace==KernelPtrSpace::CONSTANT) ?
                                159 : 1023;
                        
                        if (resIdVal != BINGEN_DEFAULT && resIdVal > maxUavId)
                        {
                            char buf[80];
                            snprintf(buf, 80, "UAVId out of range (0-%u)", maxUavId);
                            ASM_NOTGOOD_BY_ERROR(place, buf)
                        }
                    }
                    else
                        good = false;
                }
            }
            else
                haveLastArgument = havePrevArgument;
        }
    }
    else if (!pointer && isKernelArgImage(argType))
    {
        // if image type
        ptrSpace = KernelPtrSpace::GLOBAL;
        ptrAccess = KARG_PTR_READ_ONLY;
        if (!skipComma(asmr, haveComma, linePtr))
            return false;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            const char* ptrAccessPlace = linePtr;
            // access qualifier for image (rdonly,wronly)
            if (getNameArg(asmr, 15, name, linePtr, "access qualifier", false))
            {
                if (::strcmp(name, "read_only")==0 || ::strcmp(name, "rdonly")==0)
                    ptrAccess = KARG_PTR_READ_ONLY;
                else if (::strcmp(name, "write_only")==0 || ::strcmp(name, "wronly")==0)
                    ptrAccess = KARG_PTR_WRITE_ONLY;
                // OpenCL 2.0 allow read-write access for image
                else if (cl20 && (::strcmp(name, "read_write")==0 ||
                            ::strcmp(name, "rdwr")==0))
                    ptrAccess = KARG_PTR_READ_WRITE;
                else if (*name!=0) // unknown
                    ASM_NOTGOOD_BY_ERROR(ptrAccessPlace, "Unknown access qualifier")
            }
            else
                good = false;
            
            if (!skipComma(asmr, haveComma, linePtr))
                return false;
            if (haveComma)
            {
                haveLastArgument = true;
                skipSpacesToEnd(linePtr, end);
                const char* place = linePtr;
                // resid for image (0-7 for write-only images, 0-127 for read-only images)
                if (getAbsoluteValueArg(asmr, resIdVal, linePtr, false))
                {
                    cxuint maxResId = (ptrAccess == KARG_PTR_READ_ONLY) ? 127 : 7;
                    if (resIdVal!=BINGEN_DEFAULT && resIdVal > maxResId)
                    {
                        char buf[80];
                        snprintf(buf, 80, "Resource Id out of range (0-%u)", maxResId);
                        ASM_NOTGOOD_BY_ERROR(place, buf)
                    }
                }
                else
                    good = false;
            }
        }
    }
    else if (!pointer && ((!cl20 && argType == KernelArgType::COUNTER32) ||
            (cl20 && argType == KernelArgType::SAMPLER)))
    {
        // counter uavId
        if (!skipComma(asmr, haveComma, linePtr))
            return false;
        if (haveComma)
        {
            haveLastArgument = true;
            skipSpacesToEnd(linePtr, end);
            const char* place = linePtr;
            if (getAbsoluteValueArg(asmr, resIdVal, linePtr, true))
            {
                // for OpenCL 1.2 is - resid in 0-7
                if (resIdVal!=BINGEN_DEFAULT && (!cl20 && resIdVal > 7))
                    ASM_NOTGOOD_BY_ERROR(place, "Resource Id out of range (0-7)")
                    // for OpenCL 1.2 is - resid in 0-15
                else if (resIdVal!=BINGEN_DEFAULT && cl20 && resIdVal > 15)
                    ASM_NOTGOOD_BY_ERROR(place, "Sampler Id out of range (0-15)")
            }
            else
                good = false;
        }
    }
    else if (!pointer && argType == KernelArgType::STRUCTURE)
    {
        /* parse structure size */
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        skipSpacesToEnd(linePtr, end);
        const char* structSizePlace = linePtr;
        if (getAbsoluteValueArg(asmr, structSizeVal, linePtr, true))
            asmr.printWarningForRange(sizeof(cxuint)<<3, structSizeVal,
                              asmr.getSourcePos(structSizePlace), WS_UNSIGNED);
        else
            good = false;
        haveLastArgument = true;
    }
    else
        haveLastArgument = true;
    
    /* last argument is 'used' - indicate whether argument is used by kernel */
    if (haveLastArgument)
    {
        if (!skipComma(asmr, haveComma, linePtr))
            return false;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            const char* place = linePtr;
            good &= getNameArg(asmr, 10, name, linePtr, "unused specifier");
            toLowerString(name);
            if (::strcmp(name, "unused")==0)
                usedArg = false;
            else if (cl20 && ::strcmp(name, "rdonly")==0)
                usedArg = 1;
            else if (cl20 && ::strcmp(name, "wronly")==0)
                usedArg = 2;
            else
                ASM_NOTGOOD_BY_ERROR(place, "This is not 'unused' specifier")
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    argInput = { argName, typeName, (pointer) ? KernelArgType::POINTER :  argType,
        (pointer) ? argType : KernelArgType::VOID, ptrSpace, ptrAccess,
        cxuint(structSizeVal), size_t(constSpaceSizeVal), uint32_t(resIdVal), usedArg };
    return true;
}

void AsmAmdPseudoOps::doArg(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (asmr.currentKernel==ASMKERN_GLOBAL ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CONFIG)
        PSEUDOOP_RETURN_BY_ERROR("Illegal place of kernel argument")
    
    auto& kernelState = *handler.kernelStates[asmr.currentKernel];
    AmdKernelArgInput argInput;
    if (!parseArg(asmr, pseudoOpPlace, linePtr, kernelState.argNamesSet, argInput, false))
        return;
    /* setup argument */
    AmdKernelConfig& config = handler.output.kernels[asmr.currentKernel].config;
    const CString argName = argInput.argName;
    config.args.push_back(std::move(argInput));
    /// put argName
    kernelState.argNamesSet.insert(argName);
}

}

bool AsmAmdHandler::parsePseudoOp(const CString& firstName,
       const char* stmtPlace, const char* linePtr)
{
    const size_t pseudoOp = binaryFind(amdPseudoOpNamesTbl, amdPseudoOpNamesTbl +
                    sizeof(amdPseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - amdPseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case AMDOP_ARG:
            AsmAmdPseudoOps::doArg(*this, stmtPlace, linePtr);
            break;
        case AMDOP_BOOLCONSTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_BOOL32CONSTS);
            break;
        case AMDOP_CALNOTE:
            AsmAmdPseudoOps::addCustomCALNote(*this, stmtPlace, linePtr);
            break;
        case AMDOP_CBID:
            AsmAmdPseudoOps::doCBId(*this, stmtPlace, linePtr);
            break;
        case AMDOP_CBMASK:
            AsmAmdPseudoOps::doEntry(*this, stmtPlace, linePtr,
                         1U<<CALNOTE_ATI_CONSTANT_BUFFERS, "cbmask");
            break;
        case AMDOP_COMPILE_OPTIONS:
            AsmAmdPseudoOps::setCompileOptions(*this, linePtr);
            break;
        case AMDOP_CONDOUT:
            AsmAmdPseudoOps::doCondOut(*this, stmtPlace, linePtr);
            break;
        case AMDOP_CONFIG:
            AsmAmdPseudoOps::doConfig(*this, stmtPlace, linePtr);
            break;
        case AMDOP_CONSTANTBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_CONSTANT_BUFFERS);
            break;
        case AMDOP_CWS:
        case AMDOP_REQD_WORK_GROUP_SIZE:
            AsmAmdPseudoOps::setCWS(*this, stmtPlace, linePtr);
            break;
        case AMDOP_DIMS:
            AsmAmdPseudoOps::setDimensions(*this, stmtPlace, linePtr);
            break;
        case AMDOP_DRIVER_INFO:
            AsmAmdPseudoOps::setDriverInfo(*this, linePtr);
            break;
        case AMDOP_DRIVER_VERSION:
            AsmAmdPseudoOps::setDriverVersion(*this, linePtr);
            break;
        case AMDOP_EARLYEXIT:
            AsmAmdPseudoOps::doEarlyExit(*this, stmtPlace, linePtr);
            break;
        case AMDOP_ENTRY:
            AsmAmdPseudoOps::doEntry(*this, stmtPlace, linePtr,
                         (1U<<CALNOTE_ATI_PROGINFO) | (1<<CALNOTE_ATI_UAV), "entry");
            break;
        case AMDOP_EXCEPTIONS:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_EXCEPTIONS);
            break;
        case AMDOP_FLOATCONSTS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_FLOAT32CONSTS);
            break;
        case AMDOP_FLOATMODE:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_FLOATMODE);
            break;
        case AMDOP_GETDRVVER:
            AsmAmdPseudoOps::getDriverVersion(*this, linePtr);
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
        case AMDOP_LOCALSIZE:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_HWLOCAL);
            break;
        case AMDOP_HWREGION:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_HWREGION);
            break;
        case AMDOP_IEEEMODE:
            AsmAmdPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                                AMDCVAL_IEEEMODE);
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
            AsmAmdPseudoOps::doSampler(*this, stmtPlace, linePtr);
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
                    (1U<<CALNOTE_ATI_BOOL32CONSTS), "segment");
            break;
        case AMDOP_SGPRSNUM:
            AsmAmdPseudoOps::setConfigValue(*this, stmtPlace, linePtr, AMDCVAL_SGPRSNUM);
            break;
        case AMDOP_SUBCONSTANTBUFFERS:
            AsmAmdPseudoOps::addCALNote(*this, stmtPlace, linePtr,
                            CALNOTE_ATI_SUB_CONSTANT_BUFFERS);
            break;
        case AMDOP_TGSIZE:
            AsmAmdPseudoOps::setConfigBoolValue(*this, stmtPlace, linePtr,
                            AMDCVAL_TGSIZE);
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
            AsmAmdPseudoOps::addUserData(*this, stmtPlace, linePtr);
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
{
    if (assembler.isaAssembler!=nullptr)
        saveCurrentAllocRegs(); // save last kernel allocated registers to kernel state
    
    output.is64Bit = assembler.is64Bit();
    output.deviceType = assembler.getDeviceType();
    /* initialize sections */
    const size_t sectionsNum = sections.size();
    const size_t kernelsNum = kernelStates.size();
    
    // set sections as outputs
    for (size_t i = 0; i < sectionsNum; i++)
    {
        const AsmSection& asmSection = assembler.sections[i];
        const Section& section = sections[i];
        const size_t sectionSize = asmSection.getSize();
        const cxbyte* sectionData = (!asmSection.content.empty()) ?
                asmSection.content.data() : (const cxbyte*)"";
        AmdKernelInput* kernel = (section.kernelId!=ASMKERN_GLOBAL) ?
                    &output.kernels[section.kernelId] : nullptr;
                
        switch(asmSection.type)
        {
            case AsmSectionType::CODE:
                kernel->codeSize = sectionSize;
                kernel->code = sectionData;
                break;
            case AsmSectionType::AMD_HEADER:
                kernel->headerSize = sectionSize;
                kernel->header = sectionData;
                break;
            case AsmSectionType::AMD_METADATA:
                kernel->metadataSize = sectionSize;
                kernel->metadata = (const char*)sectionData;
                break;
            case AsmSectionType::DATA:
                if (section.kernelId == ASMKERN_GLOBAL)
                {
                    // this is global data
                    if (sectionSize!=0)
                    {
                        output.globalDataSize = sectionSize;
                        output.globalData = sectionData;
                    }
                }
                else
                {
                    // this is kernel data
                    kernel->dataSize = sectionSize;
                    kernel->data = sectionData;
                }
                break;
            case AsmSectionType::AMD_CALNOTE:
            {
                // add new CAL note to output
                CALNoteInput calNote;
                calNote.header.type = section.extraId;
                calNote.header.descSize = sectionSize;
                calNote.header.nameSize = 8;
                ::memcpy(calNote.header.name, "ATI CAL", 8);
                calNote.data = sectionData;
                kernel->calNotes.push_back(calNote);
                break;
            }
            case AsmSectionType::EXTRA_PROGBITS:
            case AsmSectionType::EXTRA_NOTE:
            case AsmSectionType::EXTRA_NOBITS:
            case AsmSectionType::EXTRA_SECTION:
            {
                // handle extra (user) section, set section type and its flags
                uint32_t elfSectType =
                       (asmSection.type==AsmSectionType::EXTRA_NOTE) ? SHT_NOTE :
                       (asmSection.type==AsmSectionType::EXTRA_NOBITS) ? SHT_NOBITS :
                             SHT_PROGBITS;
                uint32_t elfSectFlags = 
                    ((asmSection.flags&ASMELFSECT_ALLOCATABLE) ? SHF_ALLOC : 0) |
                    ((asmSection.flags&ASMELFSECT_WRITEABLE) ? SHF_WRITE : 0) |
                    ((asmSection.flags&ASMELFSECT_EXECUTABLE) ? SHF_EXECINSTR : 0);
                // put extra sections to binary
                if (section.kernelId == ASMKERN_GLOBAL)
                    output.extraSections.push_back({section.name, sectionSize, sectionData,
                            asmSection.alignment!=0?asmSection.alignment:1, elfSectType,
                            elfSectFlags, ELFSECTID_NULL, 0, 0 });
                else // to inner binary
                    kernel->extraSections.push_back({section.name, sectionSize, sectionData,
                            asmSection.alignment!=0?asmSection.alignment:1, elfSectType,
                            elfSectFlags, ELFSECTID_NULL, 0, 0 });
                break;
            }
            default: // ignore other sections
                break;
        }
    }
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(assembler.deviceType);
    // determine max SGPRs number for architecture excluding VCC
    const cxuint maxSGPRsNumWithoutVCC = getGPUMaxRegistersNum(arch, REGTYPE_SGPR,
                REGCOUNT_NO_VCC);
    // set up number of the allocated SGPRs and VGPRs for kernel
    for (size_t i = 0; i < kernelsNum; i++)
    {
        if (!output.kernels[i].useConfig)
            continue;
        AmdKernelConfig& config = output.kernels[i].config;
        cxuint userSGPRsNum = 0;
        /* include userData sgprs */
        for (cxuint i = 0; i < config.userDatas.size(); i++)
            userSGPRsNum = std::max(userSGPRsNum,
                        config.userDatas[i].regStart+config.userDatas[i].regSize);
        
        cxuint dimMask = (config.dimMask!=BINGEN_DEFAULT) ? config.dimMask :
                ((config.pgmRSRC2>>7)&7);
        cxuint minRegsNum[2];
        // get minimum required register by user data
        getGPUSetupMinRegistersNum(arch, dimMask, userSGPRsNum,
                   ((config.tgSize) ? GPUSETUP_TGSIZE_EN : 0) |
                   ((config.scratchBufferSize!=0) ? GPUSETUP_SCRATCH_EN : 0), minRegsNum);
        // set used SGPRs number if not specified by user
        if (config.usedSGPRsNum==BINGEN_DEFAULT)
            config.usedSGPRsNum = std::min(maxSGPRsNumWithoutVCC,
                std::max(minRegsNum[0], kernelStates[i]->allocRegs[0]));
        if (config.usedVGPRsNum==BINGEN_DEFAULT)
            config.usedVGPRsNum = std::max(minRegsNum[1], kernelStates[i]->allocRegs[1]);
    }
    
    /* put extra symbols */
    if (assembler.flags & ASM_FORCE_ADD_SYMBOLS)
        for (const AsmSymbolEntry& symEntry: assembler.globalScope.symbolMap)
        {
            if (!symEntry.second.hasValue ||
                ELF32_ST_BIND(symEntry.second.info) == STB_LOCAL)
                continue; // unresolved or local
            AsmSectionId binSectId = (symEntry.second.sectionId != ASMSECT_ABS) ?
                    sections[symEntry.second.sectionId].elfBinSectId : ELFSECTID_ABS;
            if (binSectId==ELFSECTID_UNDEF)
                continue; // no section
            
            const BinSymbol binSym = { symEntry.first, symEntry.second.value,
                        symEntry.second.size, binSectId, false, symEntry.second.info,
                        symEntry.second.other };
            
            if (symEntry.second.sectionId == ASMSECT_ABS ||
                sections[symEntry.second.sectionId].kernelId == ASMKERN_GLOBAL)
                output.extraSymbols.push_back(std::move(binSym));
            else // to kernel extra symbols
                output.kernels[sections[symEntry.second.sectionId].kernelId].extraSymbols
                            .push_back(std::move(binSym));
        }
    // driver version setup
    if (output.driverVersion==0 && output.driverInfo.empty() &&
        (assembler.flags&ASM_TESTRUN)==0)
    {
        if (assembler.driverVersion==0) // just detect driver version
            output.driverVersion = detectedDriverVersion;
        else // from assembler setup
            output.driverVersion = assembler.driverVersion;
    }
    return true;
}

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
