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
    ieeeMode = false;
    floatMode = 0xc0;
    priority = 0;
    exceptions = 0;
    tgSize = false;
    debugMode = false;
    privilegedMode = false;
    dx10Clamp = false;
}

AsmFormatHandler::AsmFormatHandler(Assembler& _assembler) : assembler(_assembler)
{ }

AsmFormatHandler::~AsmFormatHandler()
{ }

void AsmFormatHandler::handleLabel(const CString& label)
{ }

bool AsmFormatHandler::resolveSymbol(const AsmSymbol& symbol, uint64_t& value,
                 cxuint& sectionId)
{
    return false;
}

bool AsmFormatHandler::resolveRelocation(const AsmExpression* expr, uint64_t& value,
                 cxuint& sectionId)
{
    return false;
}

bool AsmFormatHandler::resolveLoHiRelocExpression(const AsmExpression* expr,
                RelocType& relType, cxuint& relSectionId, uint64_t& relValue)
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
        cxuint tmpSectionId;
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
        throw AsmFormatException("Section '.text' already exists");
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
