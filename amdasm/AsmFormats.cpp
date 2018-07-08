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
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

AsmFormatException::AsmFormatException(const std::string& message) : Exception(message)
{ }

// common routine to initialize AMD HSA config (used by AMDCL2, Gallium and ROCm handlers)
void AsmAmdHsaKernelConfig::initialize()
{
    // set default values to kernel config
    ::memset(this, 0xff, 128);
    ::memset(controlDirective, 0, 128);
    computePgmRsrc1 = computePgmRsrc2 = 0;
    enableSgprRegisterFlags = 0;
    enableFeatureFlags = 0;
    reserved1[0] = reserved1[1] = reserved1[2] = 0;
    dimMask = 0;
    usedVGPRsNum = BINGEN_DEFAULT;
    usedSGPRsNum = BINGEN_DEFAULT;
    userDataNum = BINGEN8_DEFAULT;
    callConvention = 0;
    ieeeMode = false;
    floatMode = 0xc0;
    priority = 0;
    exceptions = 0;
    tgSize = false;
    debugMode = false;
    privilegedMode = false;
    dx10Clamp = false;
}

AsmFormatHandler::AsmFormatHandler(Assembler& _assembler) : assembler(_assembler),
        sectionDiffsResolvable(false)
{ }

AsmFormatHandler::~AsmFormatHandler()
{ }

void AsmFormatHandler::handleLabel(const CString& label)
{ }

bool AsmFormatHandler::resolveSymbol(const AsmSymbol& symbol, uint64_t& value,
                 AsmSectionId& sectionId)
{
    return false;
}

bool AsmFormatHandler::resolveRelocation(const AsmExpression* expr, uint64_t& value,
                 AsmSectionId& sectionId)
{
    return false;
}

bool AsmFormatHandler::resolveLoHiRelocExpression(const AsmExpression* expr,
                RelocType& relType, AsmSectionId& relSectionId, uint64_t& relValue)
{
    const AsmExprTarget& target = expr->getTarget();
    const AsmExprTargetType tgtType = target.type;
    if ((tgtType!=ASMXTGT_DATA32 &&
        !assembler.isaAssembler->relocationIsFit(32, tgtType)))
    {
        assembler.printError(expr->getSourcePos(),
                        "Can't resolve expression for non 32-bit integer");
        return false;
    }
    if (target.sectionId==ASMSECT_ABS ||
        assembler.sections[target.sectionId].type!=AsmSectionType::CODE)
    {
        assembler.printError(expr->getSourcePos(), "Can't resolve expression outside "
                "code section");
        return false;
    }
    const Array<AsmExprOp>& ops = expr->getOps();
    
    size_t relOpStart = 0;
    size_t relOpEnd = ops.size();
    relType = RELTYPE_LOW_32BIT;
    // checking what is expression
    // get () OP () - operator between two parts
    AsmExprOp lastOp = ops.back();
    if (lastOp==AsmExprOp::BIT_AND || lastOp==AsmExprOp::MODULO ||
        lastOp==AsmExprOp::SIGNED_MODULO || lastOp==AsmExprOp::DIVISION ||
        lastOp==AsmExprOp::SIGNED_DIVISION || lastOp==AsmExprOp::SHIFT_RIGHT)
    {
        // check low or high relocation
        relOpStart = 0;
        relOpEnd = expr->toTop(ops.size()-2);
        /// evaluate second argument
        AsmSectionId tmpSectionId;
        uint64_t secondArg;
        if (!expr->evaluate(assembler, relOpEnd, ops.size()-1, secondArg, tmpSectionId))
            return false;
        if (tmpSectionId!=ASMSECT_ABS)
        {
            // must be absolute
            assembler.printError(expr->getSourcePos(),
                        "Second argument for relocation operand must be absolute");
            return false;
        }
        bool good = true;
        switch (lastOp)
        {
            case AsmExprOp::BIT_AND:
                // handle (x&0xffffffff)
                relType = RELTYPE_LOW_32BIT;
                good = ((secondArg & 0xffffffffULL) == 0xffffffffULL);
                break;
            case AsmExprOp::MODULO:
            case AsmExprOp::SIGNED_MODULO:
                // handle (x%0x100000000)
                relType = RELTYPE_LOW_32BIT;
                good = ((secondArg>>32)!=0 && (secondArg & 0xffffffffULL) == 0);
                break;
            case AsmExprOp::DIVISION:
            case AsmExprOp::SIGNED_DIVISION:
                // handle (x/0x100000000)
                relType = RELTYPE_HIGH_32BIT;
                good = (secondArg == 0x100000000ULL);
                break;
            case AsmExprOp::SHIFT_RIGHT:
                // handle (x>>32)
                relType = RELTYPE_HIGH_32BIT;
                good = (secondArg == 32);
                break;
            default:
                break;
        }
        if (!good)
        {
            assembler.printError(expr->getSourcePos(),
                        "Can't resolve relocation for this expression");
            return false;
        }
    }
    
    relSectionId = 0;
    relValue = 0;
    // 
    return expr->evaluate(assembler, relOpStart, relOpEnd, relValue, relSectionId);
}

bool AsmFormatHandler::prepareSectionDiffsResolving()
{
    return false;
}

/* AsmKcodeHandler */

AsmKcodeHandler::AsmKcodeHandler(Assembler& assembler) : AsmFormatHandler(assembler),
        currentKcodeKernel(ASMKERN_GLOBAL), codeSection(ASMSECT_NONE)
{ }

void AsmKcodeHandler::restoreKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {
        KernelBase& newKernel = getKernelBase(currentKcodeKernel);
        assembler.isaAssembler->setAllocatedRegisters(newKernel.allocRegs,
                            newKernel.allocRegFlags);
    }
}

void AsmKcodeHandler::saveKcodeCurrentAllocRegs()
{
    if (currentKcodeKernel != ASMKERN_GLOBAL)
    {
        // save other state
        size_t regTypesNum;
        KernelBase& oldKernel = getKernelBase(currentKcodeKernel);
        const cxuint* regs = assembler.isaAssembler->getAllocatedRegisters(
                            regTypesNum, oldKernel.allocRegFlags);
        std::copy(regs, regs+regTypesNum, oldKernel.allocRegs);
    }
}

void AsmKcodeHandler::prepareKcodeState()
{
    if (assembler.isaAssembler!=nullptr)
    {
        // make last kernel registers pool updates
        if (kcodeSelStack.empty())
            saveKcodeCurrentAllocRegs();
        else
            while (!kcodeSelStack.empty())
            {
                // pop from kcode stack and apply changes
                AsmKcodePseudoOps::updateKCodeSel(*this, kcodeSelection);
                kcodeSelection = kcodeSelStack.top();
                kcodeSelStack.pop();
            }
    }
}

void AsmKcodeHandler::handleLabel(const CString& label)
{
    if (assembler.sections[assembler.currentSection].type != AsmSectionType::CODE)
        return;
    auto kit = assembler.kernelMap.find(label);
    if (kit == assembler.kernelMap.end())
        return;
    if (!kcodeSelection.empty())
        return; // do not change if inside kcode
    // add code start
    assembler.sections[assembler.currentSection].addCodeFlowEntry({
                    size_t(assembler.currentOutPos), 0, AsmCodeFlowType::START });
    // save other state
    saveKcodeCurrentAllocRegs();
    if (currentKcodeKernel != ASMKERN_GLOBAL)
        assembler.kernels[currentKcodeKernel].closeCodeRegion(
                        assembler.sections[codeSection].content.size());
    // restore this state
    currentKcodeKernel = kit->second;
    restoreKcodeCurrentAllocRegs();
    if (currentKcodeKernel != ASMKERN_GLOBAL)
        assembler.kernels[currentKcodeKernel].openCodeRegion(
                        assembler.sections[codeSection].content.size());
}

/* AsmKcodePseudoOps */

/* update kernel code selection, join all regallocation and store to
 * current kernel regalloc */
void AsmKcodePseudoOps::updateKCodeSel(AsmKcodeHandler& handler,
                    const std::vector<AsmKernelId>& oldset)
{
    Assembler& asmr = handler.assembler;
    // old elements - join current regstate with all them
    size_t regTypesNum;
    for (auto it = oldset.begin(); it != oldset.end(); ++it)
    {
        Flags curAllocRegFlags;
        const cxuint* curAllocRegs = asmr.isaAssembler->getAllocatedRegisters(regTypesNum,
                               curAllocRegFlags);
        cxuint newAllocRegs[MAX_REGTYPES_NUM];
        AsmKcodeHandler::KernelBase& kernel = handler.getKernelBase(*it);
        for (size_t i = 0; i < regTypesNum; i++)
            newAllocRegs[i] = std::max(curAllocRegs[i], kernel.allocRegs[i]);
        kernel.allocRegFlags |= curAllocRegFlags;
        std::copy(newAllocRegs, newAllocRegs+regTypesNum, kernel.allocRegs);
    }
    asmr.isaAssembler->setAllocatedRegisters();
}

void AsmKcodePseudoOps::doKCode(AsmKcodeHandler& handler, const char* pseudoOpPlace,
                    const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    const char* end = asmr.line + asmr.lineSize;
    bool good = true;
    skipSpacesToEnd(linePtr, end);
    if (linePtr==end)
        return;
    std::unordered_set<AsmKernelId> newSel(handler.kcodeSelection.begin(),
                          handler.kcodeSelection.end());
    do {
        CString kname;
        const char* knamePlace = linePtr;
        skipSpacesToEnd(linePtr, end);
        bool removeKernel = false;
        if (linePtr!=end && *linePtr=='-')
        {
            // '-' - remove this kernel from current kernel selection
            removeKernel = true;
            linePtr++;
        }
        else if (linePtr!=end && *linePtr=='+')
        {
            linePtr++;
            skipSpacesToEnd(linePtr, end);
            if (linePtr==end)
            {
                // add all kernels
                const AsmKernelId kernelsNum = handler.getKernelsNum();
                for (AsmKernelId k = 0; k < kernelsNum; k++)
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
    
    if (!handler.isCodeSection())
        PSEUDOOP_RETURN_BY_ERROR("KCode outside code")
    if (handler.kcodeSelStack.empty())
        handler.saveKcodeCurrentAllocRegs();
    // push to stack
    handler.kcodeSelStack.push(handler.kcodeSelection);
    // set current sel
    handler.kcodeSelection.assign(newSel.begin(), newSel.end());
    std::sort(handler.kcodeSelection.begin(), handler.kcodeSelection.end());
    
    const std::vector<AsmKernelId>& oldKCodeSel = handler.kcodeSelStack.top();
    if (!oldKCodeSel.empty())
        asmr.handleRegionsOnKernels(handler.kcodeSelection, oldKCodeSel,
                            handler.codeSection);
    else if (handler.currentKcodeKernel != ASMKERN_GLOBAL)
    {
        std::vector<AsmKernelId> tempKCodeSel;
        tempKCodeSel.push_back(handler.currentKcodeKernel);
        asmr.handleRegionsOnKernels(handler.kcodeSelection, tempKCodeSel,
                            handler.codeSection);
    }
    
    updateKCodeSel(handler, handler.kcodeSelStack.top());
}

void AsmKcodePseudoOps::doKCodeEnd(AsmKcodeHandler& handler, const char* pseudoOpPlace,
                    const char* linePtr)
{
    Assembler& asmr = handler.assembler;
    if (!handler.isCodeSection())
        PSEUDOOP_RETURN_BY_ERROR("KCodeEnd outside code")
    if (handler.kcodeSelStack.empty())
        PSEUDOOP_RETURN_BY_ERROR("'.kcodeend' without '.kcode'")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    updateKCodeSel(handler, handler.kcodeSelection);
    std::vector<AsmKernelId> oldKCodeSel = handler.kcodeSelection;
    handler.kcodeSelection = handler.kcodeSelStack.top();
    
    if (!handler.kcodeSelection.empty())
        asmr.handleRegionsOnKernels(handler.kcodeSelection, oldKCodeSel,
                        handler.codeSection);
    else if (handler.currentKcodeKernel != ASMKERN_GLOBAL)
    {
        // if choosen current kernel
        std::vector<AsmKernelId> curKernelSel;
        curKernelSel.push_back(handler.currentKcodeKernel);
        asmr.handleRegionsOnKernels(curKernelSel, oldKCodeSel, handler.codeSection);
    }
    
    handler.kcodeSelStack.pop();
    if (handler.kcodeSelStack.empty())
        handler.restoreKcodeCurrentAllocRegs();
}

/* raw code handler */

AsmRawCodeHandler::AsmRawCodeHandler(Assembler& assembler): AsmFormatHandler(assembler)
{
    assembler.currentKernel = ASMKERN_GLOBAL;
    assembler.currentSection = 0;
}

AsmKernelId AsmRawCodeHandler::addKernel(const char* kernelName)
{
    throw AsmFormatException("In rawcode defining kernels is not allowed");
}

AsmSectionId AsmRawCodeHandler::addSection(const char* name, AsmKernelId kernelId)
{
    if (::strcmp(name, ".text")!=0)
        throw AsmFormatException("Only section '.text' can be in raw code");
    else
        throw AsmFormatException("Section '.text' already exists");
}

AsmSectionId AsmRawCodeHandler::getSectionId(const char* sectionName) const
{
    return ::strcmp(sectionName, ".text") ? ASMSECT_NONE : 0;
}

void AsmRawCodeHandler::setCurrentKernel(AsmKernelId kernel)
{
    if (kernel != ASMKERN_GLOBAL)
        throw AsmFormatException("No kernels available");
}

void AsmRawCodeHandler::setCurrentSection(AsmSectionId sectionId)
{
    if (sectionId!=0)
        throw AsmFormatException("Section doesn't exists");
}

AsmFormatHandler::SectionInfo AsmRawCodeHandler::getSectionInfo(
                                AsmSectionId sectionId) const
{
    if (sectionId!=0)
        throw AsmFormatException("Section doesn't exists");
    return { ".text", AsmSectionType::CODE, ASMSECT_ADDRESSABLE | ASMSECT_WRITEABLE };
}

bool AsmRawCodeHandler::parsePseudoOp(const CString& firstName,
           const char* stmtPlace, const char* linePtr)
{
    // not recognized any pseudo-op
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
