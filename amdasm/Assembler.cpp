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
#include <string>
#include <cassert>
#include <fstream>
#include <vector>
#include <stack>
#include <deque>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

#define THIS_FAIL_BY_ERROR(PLACE, STRING) \
    { \
        printError(PLACE, STRING); \
        return false; \
    }

#define THIS_NOTGOOD_BY_ERROR(PLACE, STRING) \
    { \
        printError(PLACE, STRING); \
        good = false; \
    }

using namespace CLRX;

// Assembler Exception costuctor
AsmException::AsmException(const std::string& message) : Exception(message)
{ }

// copy constructor - includes usageHandler copying
AsmSection::AsmSection(const AsmSection& section)
{
    name = section.name;
    kernelId = section.kernelId;
    type = section.type;
    flags = section.flags;
    alignment = section.alignment;
    size = section.size;
    content = section.content;
    relSpace = section.relSpace;
    relAddress = section.relAddress;
    
    if (section.usageHandler!=nullptr)
        usageHandler.reset(section.usageHandler->copy());
    if (section.linearDepHandler!=nullptr)
        linearDepHandler.reset(section.linearDepHandler->copy());
    if (section.waitHandler!=nullptr)
        waitHandler.reset(section.waitHandler->copy());
    codeFlow = section.codeFlow;
}

// copy assignment - includes usageHandler copying
AsmSection& AsmSection::operator=(const AsmSection& section)
{
    name = section.name;
    kernelId = section.kernelId;
    type = section.type;
    flags = section.flags;
    alignment = section.alignment;
    size = section.size;
    content = section.content;
    relSpace = section.relSpace;
    relAddress = section.relAddress;
    
    if (section.usageHandler!=nullptr)
        usageHandler.reset(section.usageHandler->copy());
    if (section.linearDepHandler!=nullptr)
        linearDepHandler.reset(section.linearDepHandler->copy());
    if (section.waitHandler!=nullptr)
        waitHandler.reset(section.waitHandler->copy());
    codeFlow = section.codeFlow;
    return *this;
}

// open code region - add new code region if needed
// called when kernel label encountered or region for this kernel begins
void AsmKernel::openCodeRegion(size_t offset)
{
    if (!codeRegions.empty() && codeRegions.back().second == SIZE_MAX)
        return;
    if (codeRegions.empty() || codeRegions.back().first != offset)
    {
        if (codeRegions.empty() || codeRegions.back().second != offset)
            codeRegions.push_back({ offset, SIZE_MAX });
        else    // set last region as not closed
            codeRegions.back().second = SIZE_MAX;
    }
}

// open code region - set end of region or if empty remove obsolete code region
// called when new kernel label encountered or region for this kernel ends
void AsmKernel::closeCodeRegion(size_t offset)
{
    if (codeRegions.empty())
        codeRegions.push_back({ size_t(0), SIZE_MAX });
    else if (codeRegions.back().second == SIZE_MAX) // only if not closed
    {
        codeRegions.back().second = offset;
        if (codeRegions.back().first == codeRegions.back().second)
            codeRegions.pop_back(); // remove empty region
    }
}

/* table of token characters
 * value for character means:
 * 7 bit - this character with previous character with this same
 *   (7 lower bits) continues token
 * 0-7 bits - class of token */
const cxbyte CLRX::tokenCharTable[96] =
{
    //' '   '!'   '"'   '#'   '$'   '%'   '&'   '''
    0x00, 0x01, 0x02, 0x03, 0x90, 0x85, 0x06, 0x07,
    //'('   ')'   '*'   '+'   ','   '-'   '.'   '/'
    0x88, 0x88, 0x8a, 0x0b, 0x0c, 0x8d, 0x90, 0x0f,
    //'0'   '1'   '2'   '3'   '4'   '5'   '6'   '7'  
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    //'8'   '9'   ':'   ';'   '<'   '='   '>'   '?'
    0x90, 0x90, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    //'@'   'A'   'B'   'C'   'D'   'E'   'F'   'G'
    0x26, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    //'H'   'I'   'J'   'K'   'L'   'M'   'N'   'O'
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    //'P'   'Q'   'R'   'S'   'T'   'U'   'V'   'W'
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    //'X'   'Y'   'Z'   '['   '\'   ']'   '^'   '_'
    0x90, 0x90, 0x90, 0x91, 0x92, 0x93, 0x14, 0x90,
    //'`'   'a'   'b'   'c'   'd'   'e'   'f'   'g'
    0x16, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    //'h'   'i'   'j'   'k'   'l'   'm'   'n'   'o'
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    //'p'   'q'   'r'   's'   't'   'u'   'v'   'w'
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    //'x'   'y'   'z'   '{'   '|'   '}'   '~'   ''
    0x90, 0x90, 0x90, 0x97, 0x18, 0x99, 0x1a, 0x1b
};

 CString CLRX::extractSymName(const char*& string, const char* end,
           bool localLabelSymName)
{
    const char* startString = string;
    if (string != end)
    {
        if(isAlpha(*string) || *string == '_' || *string == '.' || *string == '$')
            for (string++; string != end && (isAlnum(*string) || *string == '_' ||
                 *string == '.' || *string == '$') ; string++);
        else if (localLabelSymName && isDigit(*string)) // local label
        {
            for (string++; string!=end && isDigit(*string); string++);
            // check whether is local forward or backward label
            string = (string != end && (*string == 'f' || *string == 'b')) ?
                    string+1 : startString;
            // if not part of binary number or illegal bin number
            if (startString != string && (string!=end && (isAlnum(*string))))
                string = startString;
        }
    }
    return CString(startString, string);
}

CString CLRX::extractScopedSymName(const char*& string, const char* end,
           bool localLabelSymName)
{
    const char* startString = string;
    const char* lastString = string;
    if (localLabelSymName && isDigit(*string)) // local label
    {
        for (string++; string!=end && isDigit(*string); string++);
        // check whether is local forward or backward label
        string = (string != end && (*string == 'f' || *string == 'b')) ?
                string+1 : startString;
        // if not part of binary number or illegal bin number
        if (startString != string && (string!=end && (isAlnum(*string))))
            string = startString;
        return CString(startString, string);
    }
    while (string != end)
    {
        if (*string==':' && string+1!=end && string[1]==':')
            string += 2;
        else if (string != startString) // if not start
            break;
        // skip string
        const char* scopeName = string;
        if (string != end)
        {
            if(isAlpha(*string) || *string == '_' || *string == '.' || *string == '$')
                for (string++; string != end && (isAlnum(*string) || *string == '_' ||
                     *string == '.' || *string == '$') ; string++);
        }
        if (scopeName==string) // is not scope name
            break;
        lastString = string;
    }
    return CString(startString, lastString);
}

// skip spaces, labels and '\@' and \(): move to statement skipping all labels
void CLRX::skipSpacesAndLabels(const char*& linePtr, const char* end)
{
    const char* stmtStart;
    while (true)
    {
        skipSpacesToEnd(linePtr, end);
        stmtStart = linePtr;
        // skip labels and macro substitutions
        while (linePtr!=end && (isAlnum(*linePtr) || *linePtr=='$' || *linePtr=='.' ||
            *linePtr=='_' || *linePtr=='\\'))
        {
            if (*linePtr!='\\')
                linePtr++;
            else
            {
                /* handle \@ and \() for correct parsing */
                linePtr++;
                if (linePtr!=end)
                {
                    if (*linePtr=='@')
                        linePtr++;
                    else if (*linePtr=='(')
                    {
                        linePtr++;
                        if (linePtr!=end && *linePtr==')')
                            linePtr++;
                    }
                }
            }
        }
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!=':')
            break;
        linePtr++; // skip ':'
    }
    linePtr = stmtStart;
}

// check whether some garbages at end of line (true if not)
bool AsmParseUtils::checkGarbagesAtEnd(Assembler& asmr, const char* linePtr)
{
    skipSpacesToEnd(linePtr, asmr.line + asmr.lineSize);
    if (linePtr != asmr.line + asmr.lineSize)
        ASM_FAIL_BY_ERROR(linePtr, "Garbages at end of line")
    return true;
}

// get absolute value (by evaluating expression): all symbols must be resolved at this time
bool AsmParseUtils::getAbsoluteValueArg(Assembler& asmr, uint64_t& value,
                      const char*& linePtr, bool requiredExpr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* exprPlace = linePtr;
    if (AsmExpression::fastExprEvaluate(asmr, linePtr, value))
        return true;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr, false, true));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty() && requiredExpr)
        ASM_FAIL_BY_ERROR(exprPlace, "Expected expression")
    if (expr->isEmpty()) // do not set if empty expression
        return true;
    AsmSectionId sectionId; // for getting
    if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
        return false;
    else if (sectionId != ASMSECT_ABS)
        // if not absolute value
        ASM_FAIL_BY_ERROR(exprPlace, "Expression must be absolute!")
    return true;
}

// get any value (also position in section)
bool AsmParseUtils::getAnyValueArg(Assembler& asmr, uint64_t& value,
                   AsmSectionId& sectionId, const char*& linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* exprPlace = linePtr;
    if (AsmExpression::fastExprEvaluate(asmr, linePtr, value))
    {
        sectionId = ASMSECT_ABS;
        return true;
    }
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr, false, true));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty())
        ASM_FAIL_BY_ERROR(exprPlace, "Expected expression")
    if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
        return false;
    return true;
}

/* get jump value - if expression have unresolved symbols, then returns output
 * target expression to resolve later */
bool AsmParseUtils::getJumpValueArg(Assembler& asmr, uint64_t& value,
            std::unique_ptr<AsmExpression>& outTargetExpr, const char*& linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* exprPlace = linePtr;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr, false, false));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty())
        ASM_FAIL_BY_ERROR(exprPlace, "Expected expression")
    if (expr->getSymOccursNum()==0)
    {
        AsmSectionId sectionId;
        AsmTryStatus evalStatus = expr->tryEvaluate(asmr, value, sectionId,
                            asmr.withSectionDiffs());
        if (evalStatus == AsmTryStatus::FAILED) // failed evaluation!
            return false;
        else if (evalStatus == AsmTryStatus::TRY_LATER)
        {
            // store out target expression (some symbols is not resolved)
            asmr.unevalExpressions.push_back(expr.get());
            outTargetExpr = std::move(expr);
            return true;
        }
        if (sectionId != asmr.currentSection)
            // if jump outside current section (.text)
            ASM_FAIL_BY_ERROR(exprPlace, "Jump over current section!")
        return true;
    }
    else
    {
        // store out target expression (some symbols is not resolved)
        outTargetExpr = std::move(expr);
        return true;
    }
}

// get name of some object - accepts '_', '.' in name
// skipCommaAtError - skip ',' when error
bool AsmParseUtils::getNameArg(Assembler& asmr, CString& outStr, const char*& linePtr,
            const char* objName, bool requiredArg, bool skipCommaAtError)
{
    const char* end = asmr.line + asmr.lineSize;
    outStr.clear();
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
    {
        if (!requiredArg)
            return true; // succeed
        ASM_FAIL_BY_ERROR(linePtr, (std::string("Expected ")+objName).c_str())
    }
    const char* nameStr = linePtr;
    if (isAlpha(*linePtr) || *linePtr == '_' || *linePtr == '.')
    {
        linePtr++;
        while (linePtr != end && (isAlnum(*linePtr) ||
            *linePtr == '_' || *linePtr == '.')) linePtr++;
    }
    else
    {
        // if is not name
        if (!requiredArg)
            return true; // succeed
        asmr.printError(linePtr, (std::string("Some garbages at ")+objName+
                        " place").c_str());
        if (!skipCommaAtError)
            while (linePtr != end && !isSpace(*linePtr) && *linePtr != ',') linePtr++;
        else // skip comma
            while (linePtr != end && !isSpace(*linePtr)) linePtr++;
        return false;
    }
    outStr.assign(nameStr, linePtr);
    return true;
}

// get name of some object - accepts '_', '.' in name
// like previous function: but static allocated strings
bool AsmParseUtils::getNameArg(Assembler& asmr, size_t maxOutStrSize, char* outStr,
               const char*& linePtr, const char* objName, bool requiredArg,
               bool ignoreLongerName, bool skipCommaAtError)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
    {
        if (!requiredArg)
        {
            outStr[0] = 0;
            return true; // succeed
        }
        ASM_FAIL_BY_ERROR(linePtr, (std::string("Expected ")+objName).c_str())
    }
    const char* nameStr = linePtr;
    if (isAlpha(*linePtr) || *linePtr == '_' || *linePtr == '.')
    {
        linePtr++;
        while (linePtr != end && (isAlnum(*linePtr) ||
            *linePtr == '_' || *linePtr == '.')) linePtr++;
    }
    else
    {
        if (!requiredArg)
        {
            outStr[0] = 0;
            return true; // succeed
        }
        asmr.printError(linePtr, (std::string("Some garbages at ")+objName+
                " place").c_str());
        
        if (!skipCommaAtError)
            while (linePtr != end && !isSpace(*linePtr) && *linePtr != ',') linePtr++;
        else
            while (linePtr != end && !isSpace(*linePtr)) linePtr++;
        return false;
    }
    if (maxOutStrSize-1 < size_t(linePtr-nameStr))
    {
        if (ignoreLongerName)
        {
            // return empty string
            outStr[0] = 0; // null-char
            return true;
        }
        ASM_FAIL_BY_ERROR(linePtr, (std::string(objName)+" is too long").c_str())
    }
    const size_t outStrSize = std::min(maxOutStrSize-1, size_t(linePtr-nameStr));
    std::copy(nameStr, nameStr+outStrSize, outStr);
    outStr[outStrSize] = 0; // null-char
    return true;
}

// skip comma (can be not present), return true in haveComma if ',' encountered
bool AsmParseUtils::skipComma(Assembler& asmr, bool& haveComma, const char*& linePtr,
                        bool errorWhenNoEnd)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
    {
        haveComma = false;
        return true;
    }
    if (*linePtr != ',')
    {
        if (errorWhenNoEnd)
            ASM_FAIL_BY_ERROR(linePtr, "Expected ',' before argument")
        else
        {
            haveComma = false;
            return true;
        }
    }
    linePtr++;
    haveComma = true;
    return true;
}

// skip comma (must be), if comma not present then fail
bool AsmParseUtils::skipRequiredComma(Assembler& asmr, const char*& linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end || *linePtr != ',')
        ASM_FAIL_BY_ERROR(linePtr, "Expected ',' before argument")
    linePtr++;
    return true;
}

// skip comma, but if fail and some character at place, then skip character
// skip also spaces after comma
bool AsmParseUtils::skipCommaForMultipleArgs(Assembler& asmr, const char*& linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end); // spaces before ','
    if (linePtr == end)
        return false;
    if (*linePtr != ',')
    {
        linePtr++;
        ASM_FAIL_BY_ERROR(linePtr-1, "Expected ',' before next value")
    }
    else
        skipCharAndSpacesToEnd(linePtr, end);
    return true;
}

// get enumeration (name of specified set), return value of recognized name from table
// prefix - optional prefix for name
bool AsmParseUtils::getEnumeration(Assembler& asmr, const char*& linePtr,
              const char* objName, size_t tableSize,
              const std::pair<const char*, cxuint>* table, cxuint& value,
              const char* prefix)
{
    const char* end = asmr.line + asmr.lineSize;
    char name[72];
    skipSpacesToEnd(linePtr, end);
    const char* namePlace = linePtr;
    // first, get name
    if (getNameArg(asmr, 72, name, linePtr, objName))
    {
        toLowerString(name);
        size_t namePos = 0;
        if (prefix!=nullptr)
        {
            size_t psize = ::strlen(prefix);
            namePos = ::memcmp(name, prefix, psize)==0 ? psize : 0;
        }
        cxuint index = binaryMapFind(table, table + tableSize, name+namePos, CStringLess())
                        - table;
        if (index != tableSize)
            value = table[index].second;
        else // end of this map
            ASM_FAIL_BY_ERROR(namePlace, (std::string("Unknown ")+objName).c_str())
        return true;
    }
    return false;
}

bool AsmParseUtils::parseSingleDimensions(Assembler& asmr, const char*& linePtr,
                cxuint& dimMask)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* dimPlace = linePtr;
    char buf[10];
    dimMask = 0;
    // get name and try to parse dimension
    if (getNameArg(asmr, 10, buf, linePtr, "dimension set", false))
    {
        toLowerString(buf);
        for (cxuint i = 0; buf[i]!=0; i++)
            if (buf[i]=='x')
                dimMask |= 1;
            else if (buf[i]=='y')
                dimMask |= 2;
            else if (buf[i]=='z')
                dimMask |= 4;
            else
                ASM_FAIL_BY_ERROR(dimPlace, "Unknown dimension type")
    }
    else // error
        return false;
    return true;
}

// used by asm format pseudo op: '.dim' to parse dimensions (xyz)
bool AsmParseUtils::parseDimensions(Assembler& asmr, const char*& linePtr, cxuint& dimMask,
            bool twoFields)
{
    dimMask = 0;
    if (!parseSingleDimensions(asmr, linePtr, dimMask))
        return false;
    if (twoFields)
    {
        dimMask |= dimMask<<3; // fill up second field
        bool haveComma = false;
        if (!skipComma(asmr, haveComma, linePtr))
            return false;
        if (haveComma)
        {
            cxuint secondField = 0;
            if (!parseSingleDimensions(asmr, linePtr, secondField))
                return false;
            // replace second field
            dimMask = (dimMask & 7) | (secondField<<3) | ASM_DIMMASK_SECONDFIELD_ENABLED;
        }
    }
    return true;
}

void AsmParseUtils::setSymbolValue(Assembler& asmr, const char* linePtr,
                    uint64_t value, AsmSectionId sectionId)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    const char* symNamePlace = linePtr;
    const CString symName = extractScopedSymName(linePtr, end, false);
    if (symName.empty())
        ASM_RETURN_BY_ERROR(symNamePlace, "Illegal symbol name")
    size_t symNameLength = symName.size();
    // special case for '.' symbol (check whether is in global scope)
    if (symNameLength >= 3 && symName.compare(symNameLength-3, 3, "::.")==0)
        ASM_RETURN_BY_ERROR(symNamePlace, "Symbol '.' can be only in global scope")
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    std::pair<AsmSymbolEntry*, bool> res = asmr.insertSymbolInScope(symName,
                AsmSymbol(sectionId, value));
    if (!res.second)
    {
        // if symbol found
        if (res.first->second.onceDefined && res.first->second.isDefined()) // if label
            asmr.printError(symNamePlace, (std::string("Symbol '")+symName.c_str()+
                        "' is already defined").c_str());
        else // set value of symbol
            asmr.setSymbol(*res.first, value, sectionId);
    }
    else // set hasValue (by isResolvableSection
        res.first->second.hasValue = asmr.isResolvableSection(sectionId);
}


ISAAssembler::ISAAssembler(Assembler& _assembler) : assembler(_assembler)
{ }

ISAAssembler::~ISAAssembler()
{ }

void AsmSymbol::removeOccurrenceInExpr(AsmExpression* expr, size_t argIndex,
               size_t opIndex)
{
    auto it = std::remove(occurrencesInExprs.begin(), occurrencesInExprs.end(),
            AsmExprSymbolOccurrence{expr, argIndex, opIndex});
    occurrencesInExprs.resize(it-occurrencesInExprs.begin());
}

void AsmSymbol::clearOccurrencesInExpr()
{
    /* iteration with index and occurrencesInExprs.size() is required for checking size
     * after expression deletion that removes occurrences in exprs in this symbol */
    for (size_t i = 0; i < occurrencesInExprs.size(); i++)
    {
        auto& occur = occurrencesInExprs[i];
        if (occur.expression!=nullptr && !occur.expression->unrefSymOccursNum())
        {
            // delete and move back iteration by number of removed elements
            const size_t oldSize = occurrencesInExprs.size();
            AsmExpression* occurExpr = occur.expression;
            occur.expression = nullptr;
            delete occurExpr;
            // subtract number of removed elements from counter
            i -= oldSize-occurrencesInExprs.size();
        }
    }
    occurrencesInExprs.clear();
}

void AsmSymbol::undefine()
{
    if (!regRange)
    {
        // delete expresion if symbol is not regrange
        delete expression;
        expression = nullptr;
    }
    *this = AsmSymbol();
}

void Assembler::undefineSymbol(AsmSymbolEntry& symEntry)
{
    cloneSymEntryIfNeeded(symEntry);
    symEntry.second.undefine();
}

struct CLRX_INTERNAL ScopeStackElem0
{
    AsmScope* scope;
    AsmScopeMap::iterator it;
};


AsmScope::~AsmScope()
{
    std::stack<ScopeStackElem0> scopeStack;
    scopeStack.push({ this, this->scopeMap.begin() });
    
    while (!scopeStack.empty())
    {
        ScopeStackElem0& entry = scopeStack.top();
        if (entry.it != entry.scope->scopeMap.end())
        {
            // next nested level
            scopeStack.push({ entry.it->second, entry.it->second->scopeMap.begin() });
            ++entry.it;
        }
        else
        {
            entry.scope->scopeMap.clear();
            /// remove expressions before symbol map deletion
            for (auto& symEntry: entry.scope->symbolMap)
                symEntry.second.clearOccurrencesInExpr();
            entry.scope->symbolMap.clear();
            if (scopeStack.size() > 1)
                delete entry.scope;
            scopeStack.pop();
        }
    }
}

// simple way to delete symbols recursively from scope
void AsmScope::deleteSymbolsRecursively()
{
    std::stack<ScopeStackElem0> scopeStack;
    scopeStack.push({ this, this->scopeMap.begin() });
    
    while (!scopeStack.empty())
    {
        ScopeStackElem0& entry = scopeStack.top();
        if (entry.it == entry.scope->scopeMap.begin())
            // first touch - clear symbol map
            entry.scope->symbolMap.clear();
        
        if (entry.it != entry.scope->scopeMap.end())
        {
            // next nested level
            scopeStack.push({ entry.it->second, entry.it->second->scopeMap.begin() });
            ++entry.it;
        }
        else
            scopeStack.pop();
    }
}

void AsmScope::startUsingScope(AsmScope* scope)
{
    // do add this
    auto res = usedScopesSet.insert({scope, usedScopes.end()});
    usedScopes.push_front(scope);
    if (!res.second)
        usedScopes.erase(res.first->second);
    res.first->second = usedScopes.begin();
}

void AsmScope::stopUsingScope(AsmScope* scope)
{
    auto it = usedScopesSet.find(scope);
    if (it != usedScopesSet.end()) // do erase from list
    {
        usedScopes.erase(it->second);
        usedScopesSet.erase(it);
    }
}

/*
 * Assembler
 */

Assembler::Assembler(const CString& filename, std::istream& input, Flags _flags,
        BinaryFormat _format, GPUDeviceType _deviceType, std::ostream& msgStream,
        std::ostream& _printStream)
        : format(_format),
          deviceType(_deviceType),
          driverVersion(0), llvmVersion(0),
          _64bit(false), newROCmBinFormat(false),
          policyVersion(ASM_POLICY_DEFAULT),
          isaAssembler(nullptr),
          // initialize global scope: adds '.' to symbols
          globalScope({nullptr,{std::make_pair(".", AsmSymbol(0, uint64_t(0)))}}),
          currentScope(&globalScope),
          flags(_flags),
          lineSize(0), line(nullptr),
          endOfAssembly(false),
          messageStream(msgStream),
          printStream(_printStream),
          // value reference and section reference from first symbol: '.'
          currentSection(globalScope.symbolMap.begin()->second.sectionId),
          currentOutPos(globalScope.symbolMap.begin()->second.value)
{
    filenameIndex = 0;
    alternateMacro = (flags & ASM_ALTMACRO)!=0;
    buggyFPLit = (flags & ASM_BUGGYFPLIT)!=0;
    macroCase = (flags & ASM_MACRONOCASE)==0;
    oldModParam = (flags & ASM_OLDMODPARAM)!=0;
    localCount = macroCount = inclusionLevel = 0;
    macroSubstLevel = repetitionLevel = 0;
    lineAlreadyRead = false;
    good = true;
    resolvingRelocs = false;
    collectSourcePoses = false;
    formatHandler = nullptr;
    input.exceptions(std::ios::badbit);
    std::unique_ptr<AsmInputFilter> thatInputFilter(
                    new AsmStreamInputFilter(input, filename));
    asmInputFilters.push(thatInputFilter.get());
    currentInputFilter = thatInputFilter.release();
}

Assembler::Assembler(const Array<CString>& _filenames, Flags _flags,
        BinaryFormat _format, GPUDeviceType _deviceType, std::ostream& msgStream,
        std::ostream& _printStream)
        : format(_format),
          deviceType(_deviceType),
          driverVersion(0), llvmVersion(0),
          _64bit(false), newROCmBinFormat(false),
          policyVersion(ASM_POLICY_DEFAULT),
          isaAssembler(nullptr),
          // initialize global scope: adds '.' to symbols
          globalScope({nullptr,{std::make_pair(".", AsmSymbol(0, uint64_t(0)))}}),
          currentScope(&globalScope),
          flags(_flags),
          lineSize(0), line(nullptr),
          endOfAssembly(false),
          messageStream(msgStream),
          printStream(_printStream),
          // value reference and section reference from first symbol: '.'
          currentSection(globalScope.symbolMap.begin()->second.sectionId),
          currentOutPos(globalScope.symbolMap.begin()->second.value)
{
    filenameIndex = 0;
    filenames = _filenames;
    alternateMacro = (flags & ASM_ALTMACRO)!=0;
    buggyFPLit = (flags & ASM_BUGGYFPLIT)!=0;
    macroCase = (flags & ASM_MACRONOCASE)==0;
    oldModParam = (flags & ASM_OLDMODPARAM)!=0;
    localCount = macroCount = inclusionLevel = 0;
    macroSubstLevel = repetitionLevel = 0;
    lineAlreadyRead = false;
    good = true;
    resolvingRelocs = false;
    collectSourcePoses = false;
    formatHandler = nullptr;
    if (filenames.empty())
        throw AsmException("Filename list is empty");
    for (cxuint i = 0; i < filenames.size(); i++)
        if (isDirectory(filenames[i].c_str()))
            throw AsmException(std::string("File '")+
                        filenames[i].c_str()+"' is directory");
    
    std::unique_ptr<AsmInputFilter> thatInputFilter(
                new AsmStreamInputFilter(filenames[filenameIndex++]));
    asmInputFilters.push(thatInputFilter.get());
    currentInputFilter = thatInputFilter.release();
}

Assembler::~Assembler()
{
    delete formatHandler;
    if (isaAssembler != nullptr)
        delete isaAssembler;
    while (!asmInputFilters.empty())
    {
        delete asmInputFilters.top();
        asmInputFilters.pop();
    }
    
    /// remove expressions before symbol map deletion
    for (auto& entry: globalScope.symbolMap)
        entry.second.clearOccurrencesInExpr();
    for (const auto& entry: globalScope.scopeMap)
        delete entry.second;
    for (AsmScope* entry: abandonedScopes)
        delete entry;
    globalScope.scopeMap.clear();
    /// remove expressions before symbol snapshots
    for (auto& entry: symbolSnapshots)
        entry->second.clearOccurrencesInExpr();
    
    for (auto& entry: symbolSnapshots)
        delete entry;
    
    /// remove expressions before symbol clones
    for (auto& entry: symbolClones)
        entry->second.clearOccurrencesInExpr();
    
    for (auto& entry: symbolClones)
        delete entry;
    
    for (auto& expr: unevalExpressions)
        delete expr;
}

// routine to parse string in assembly syntax
bool Assembler::parseString(std::string& strarray, const char*& linePtr)
{
    const char* end = line+lineSize;
    const char* startPlace = linePtr;
    skipSpacesToEnd(linePtr, end);
    strarray.clear();
    if (linePtr == end || *linePtr != '"')
    {
        while (linePtr != end && !isSpace(*linePtr) && *linePtr != ',') linePtr++;
        THIS_FAIL_BY_ERROR(startPlace, "Expected string")
    }
    linePtr++;
    
    // main loop, where is character parsing
    while (linePtr != end && *linePtr != '"')
    {
        if (*linePtr == '\\')
        {
            // escape
            linePtr++;
            uint16_t value;
            if (linePtr == end)
                THIS_FAIL_BY_ERROR(startPlace, "Unterminated character of string")
            if (*linePtr == 'x')
            {
                // hex literal
                const char* charPlace = linePtr-1;
                linePtr++;
                if (linePtr == end)
                    THIS_FAIL_BY_ERROR(startPlace, "Unterminated character of string")
                value = 0;
                if (isXDigit(*linePtr))
                    for (; linePtr != end; linePtr++)
                    {
                        cxuint digit;
                        if (*linePtr >= '0' && *linePtr <= '9')
                            digit = *linePtr-'0';
                        else if (*linePtr >= 'a' && *linePtr <= 'f')
                            digit = *linePtr-'a'+10;
                        else if (*linePtr >= 'A' && *linePtr <= 'F')
                            digit = *linePtr-'A'+10;
                        else
                            break;
                        value = (value<<4) + digit;
                    }
                else
                    THIS_FAIL_BY_ERROR(charPlace, "Expected hexadecimal character code")
                value &= 0xff;
            }
            else if (isODigit(*linePtr))
            {
                // octal literal
                value = 0;
                const char* charPlace = linePtr-1;
                for (cxuint i = 0; linePtr != end && i < 3; i++, linePtr++)
                {
                    if (!isODigit(*linePtr))
                        break;
                    value = (value<<3) + uint64_t(*linePtr-'0');
                    // checking range
                    if (value > 255)
                        THIS_FAIL_BY_ERROR(charPlace, "Octal code out of range")
                }
            }
            else
            {
                // normal escapes
                const char c = *linePtr++;
                switch (c)
                {
                    case 'a':
                        value = '\a';
                        break;
                    case 'b':
                        value = '\b';
                        break;
                    case 'r':
                        value = '\r';
                        break;
                    case 'n':
                        value = '\n';
                        break;
                    case 'f':
                        value = '\f';
                        break;
                    case 'v':
                        value = '\v';
                        break;
                    case 't':
                        value = '\t';
                        break;
                    case '\\':
                        value = '\\';
                        break;
                    case '\'':
                        value = '\'';
                        break;
                    case '\"':
                        value = '\"';
                        break;
                    default:
                        value = c;
                }
            }
            strarray.push_back(value);
        }
        else // regular character
            strarray.push_back(*linePtr++);
    }
    if (linePtr == end)
        THIS_FAIL_BY_ERROR(startPlace, "Unterminated string")
    linePtr++;
    return true;
}

// routine to parse single character literal
bool Assembler::parseLiteral(uint64_t& value, const char*& linePtr)
{
    const char* startPlace = linePtr;
    const char* end = line+lineSize;
    // if literal begins '
    if (linePtr != end && *linePtr == '\'')
    {
        linePtr++;
        if (linePtr == end)
            THIS_FAIL_BY_ERROR(startPlace, "Unterminated character literal")
        if (*linePtr == '\'')
            THIS_FAIL_BY_ERROR(startPlace, "Empty character literal")
        
        if (*linePtr != '\\')
        {
            value = *linePtr++;
            if (linePtr == end || *linePtr != '\'')
                THIS_FAIL_BY_ERROR(startPlace, "Missing ''' at end of literal")
            linePtr++;
            return true;
        }
        else // escapes
        {
            linePtr++;
            if (linePtr == end)
                THIS_FAIL_BY_ERROR(startPlace, "Unterminated character literal")
            if (*linePtr == 'x')
            {
                // hex literal
                linePtr++;
                if (linePtr == end)
                    THIS_FAIL_BY_ERROR(startPlace, "Unterminated character literal")
                value = 0;
                if (isXDigit(*linePtr))
                    for (; linePtr != end; linePtr++)
                    {
                        cxuint digit;
                        if (*linePtr >= '0' && *linePtr <= '9')
                            digit = *linePtr-'0';
                        else if (*linePtr >= 'a' && *linePtr <= 'f')
                            digit = *linePtr-'a'+10;
                        else if (*linePtr >= 'A' && *linePtr <= 'F')
                            digit = *linePtr-'A'+10;
                        else
                            break; // end of literal
                        value = (value<<4) + digit;
                    }
                else
                    THIS_FAIL_BY_ERROR(startPlace, "Expected hexadecimal character code")
                value &= 0xff;
            }
            else if (isODigit(*linePtr))
            {
                // octal literal
                value = 0;
                for (cxuint i = 0; linePtr != end && i < 3 && *linePtr != '\'';
                     i++, linePtr++)
                {
                    if (!isODigit(*linePtr))
                        THIS_FAIL_BY_ERROR(startPlace, "Expected octal character code")
                    value = (value<<3) + uint64_t(*linePtr-'0');
                    // checking range
                    if (value > 255)
                        THIS_FAIL_BY_ERROR(startPlace, "Octal code out of range")
                }
            }
            else
            {
                // normal escapes
                const char c = *linePtr++;
                switch (c)
                {
                    case 'a':
                        value = '\a';
                        break;
                    case 'b':
                        value = '\b';
                        break;
                    case 'r':
                        value = '\r';
                        break;
                    case 'n':
                        value = '\n';
                        break;
                    case 'f':
                        value = '\f';
                        break;
                    case 'v':
                        value = '\v';
                        break;
                    case 't':
                        value = '\t';
                        break;
                    case '\\':
                        value = '\\';
                        break;
                    case '\'':
                        value = '\'';
                        break;
                    case '\"':
                        value = '\"';
                        break;
                    default:
                        value = c;
                }
            }
            if (linePtr == end || *linePtr != '\'')
                THIS_FAIL_BY_ERROR(startPlace, "Missing ''' at end of literal")
            linePtr++;
            return true;
        }
    }
    // try to parse integer value
    try
    { value = cstrtovCStyle<uint64_t>(startPlace, line+lineSize, linePtr); }
    catch(const ParseException& ex)
    {
        printError(startPlace, ex.what());
        return false;
    }
    return true;
}

bool Assembler::parseLiteralNoError(uint64_t& value, const char*& linePtr)
{
    const char* startPlace = linePtr;
    const char* end = line+lineSize;
    // if literal begins '
    if (linePtr != end && *linePtr == '\'')
    {
        linePtr++;
        if (linePtr == end || *linePtr == '\'')
            return false;
        
        if (*linePtr != '\\')
        {
            value = *linePtr++;
            if (linePtr == end || *linePtr != '\'')
                return false;
            linePtr++;
            return true;
        }
        else // escapes
        {
            linePtr++;
            if (linePtr == end)
                return false;
            if (*linePtr == 'x')
            {
                // hex literal
                linePtr++;
                if (linePtr == end)
                    return false;
                value = 0;
                if (isXDigit(*linePtr))
                    for (; linePtr != end; linePtr++)
                    {
                        cxuint digit;
                        if (*linePtr >= '0' && *linePtr <= '9')
                            digit = *linePtr-'0';
                        else if (*linePtr >= 'a' && *linePtr <= 'f')
                            digit = *linePtr-'a'+10;
                        else if (*linePtr >= 'A' && *linePtr <= 'F')
                            digit = *linePtr-'A'+10;
                        else
                            break; // end of literal
                        value = (value<<4) + digit;
                    }
                else
                    return false;
                value &= 0xff;
            }
            else if (isODigit(*linePtr))
            {
                // octal literal
                value = 0;
                for (cxuint i = 0; linePtr != end && i < 3 && *linePtr != '\'';
                     i++, linePtr++)
                {
                    if (!isODigit(*linePtr))
                        return false;
                    value = (value<<3) + uint64_t(*linePtr-'0');
                    // checking range
                    if (value > 255)
                        return false;
                }
            }
            else
            {
                // normal escapes
                const char c = *linePtr++;
                switch (c)
                {
                    case 'a':
                        value = '\a';
                        break;
                    case 'b':
                        value = '\b';
                        break;
                    case 'r':
                        value = '\r';
                        break;
                    case 'n':
                        value = '\n';
                        break;
                    case 'f':
                        value = '\f';
                        break;
                    case 'v':
                        value = '\v';
                        break;
                    case 't':
                        value = '\t';
                        break;
                    case '\\':
                        value = '\\';
                        break;
                    case '\'':
                        value = '\'';
                        break;
                    case '\"':
                        value = '\"';
                        break;
                    default:
                        value = c;
                }
            }
            if (linePtr == end || *linePtr != '\'')
                return false;
            linePtr++;
            return true;
        }
    }
    // try to parse integer value
    try
    { value = cstrtovCStyle<uint64_t>(startPlace, line+lineSize, linePtr); }
    catch(const ParseException& ex)
    { return false; }
    return true;
}

// parse symbol. return PARSED - when successfuly parsed symbol,
// MISSING - when no symbol in this place, FAILED - when failed
// routine try to create new unresolved symbol if not defined and if dontCreateSymbol=false
Assembler::ParseState Assembler::parseSymbol(const char*& linePtr,
                AsmSymbolEntry*& entry, bool localLabel, bool dontCreateSymbol)
{
    const char* startPlace = linePtr;
    const CString symName = extractScopedSymName(linePtr, line+lineSize, localLabel);
    if (symName.empty())
    {
        // this is not symbol or a missing symbol
        while (linePtr != line+lineSize && !isSpace(*linePtr) && *linePtr != ',')
            linePtr++;
        entry = nullptr;
        return Assembler::ParseState::MISSING;
    }
    if (symName == ".") // any usage of '.' causes format initialization
    {
        // special case ('.' - always global)
        initializeOutputFormat();
        entry = &*globalScope.symbolMap.find(".");
        return Assembler::ParseState::PARSED;
    }
    
    Assembler::ParseState state = Assembler::ParseState::PARSED;
    bool symHasValue;
    if (!isDigit(symName.front()))
    {
        // regular symbol name (not local label)
        AsmScope* outScope;
        CString sameSymName;
        entry = findSymbolInScope(symName, outScope, sameSymName);
        if (sameSymName == ".")
        {
            // illegal name of symbol (must be in global)
            printError(startPlace, "Symbol '.' can be only in global scope");
            return Assembler::ParseState::FAILED;
        }
        if (!dontCreateSymbol && entry==nullptr)
        {
            // create unresolved symbol if not found
            std::pair<AsmSymbolMap::iterator, bool> res =
                    outScope->symbolMap.insert(std::make_pair(sameSymName, AsmSymbol()));
            entry = &*res.first;
            symHasValue = res.first->second.hasValue;
        }
        else // only find symbol and set isDefined and entry
            symHasValue = (entry != nullptr && entry->second.hasValue);
    }
    else
    {
        // local labels is in global scope
        if (!dontCreateSymbol)
        {
            // create symbol if not found
            std::pair<AsmSymbolMap::iterator, bool> res =
                    globalScope.symbolMap.insert(std::make_pair(symName, AsmSymbol()));
            entry = &*res.first;
            symHasValue = res.first->second.hasValue;
        }
        else
        {
            // only find symbol and set isDefined and entry
            AsmSymbolMap::iterator it = globalScope.symbolMap.find(symName);
            entry = (it != globalScope.symbolMap.end()) ? &*it : nullptr;
            symHasValue = (it != globalScope.symbolMap.end() && it->second.hasValue);
        }
    }
    
    if (isDigit(symName.front()) && symName[linePtr-startPlace-1] == 'b' && !symHasValue)
    {
        // failed at finding
        std::string error = "Undefined previous local label '";
        error.append(symName.begin(), linePtr-startPlace);
        error += "'";
        printError(startPlace, error.c_str());
        state = Assembler::ParseState::FAILED;
    }
    
    return state;
}

// parse argument's value 
bool Assembler::parseMacroArgValue(const char*& string, std::string& outStr)
{
    const char* end = line+lineSize;
    bool firstNonSpace = false;
    cxbyte prevTok = 0;
    cxuint backslash = 0;
    
    if ((alternateMacro && string != end && *string=='%') ||
        (!alternateMacro && string+2 <= end && *string=='\\' && string[1]=='%'))
    {
        // alternate syntax, parse expression evaluation
        // too in \% form
        const char* exprPlace = string + ((alternateMacro) ? 1 : 2);
        uint64_t value;
        if (AsmParseUtils::getAbsoluteValueArg(*this, value, exprPlace, true))
        {
            char buf[32];
            itocstrCStyle<int64_t>(value, buf, 32);
            string = exprPlace;
            outStr = buf;
            return true;
        }
        else
        {
            /* if error */
            string = exprPlace;
            return false;
        }
    }
    
    if (alternateMacro && string != end && (*string=='<' || *string=='\'' || *string=='"'))
    {
        /* alternate string quoting */
        const char termChar = (*string=='<') ? '>' : *string;
        string++;
        bool escape = false;
        while (string != end && (*string != termChar || escape))
        {
            if (!escape && *string=='!')
            {
                /* skip this escaping */
                escape = true;
                string++;
            }
            else
            {
                /* put character */
                escape = false;
                outStr.push_back(*string++);
            }
        }
        if (string == end) /* if unterminated string */
            THIS_FAIL_BY_ERROR(string, "Unterminated quoted string")
        string++;
        return true;
    }
    else if (string != end && *string=='"')
    {
        // if arg begins from '"'. quoting in old mode
        string++;
        while (string != end && (*string != '\"' || (backslash&1)!=0))
        {
            if (*string=='\\')
                backslash++;
            else
                backslash = 0;
            outStr.push_back(*string++);
        }
        if (string == end)
            THIS_FAIL_BY_ERROR(string, "Unterminated quoted string")
        string++;
        return true;
    }
    
    // argument begins from other character
    for (; string != end && *string != ','; string++)
    {
        if(*string == '"') // quoted
            return true; // next argument
        if (!isSpace(*string))
        {
            const cxbyte thisTok = (cxbyte(*string) >= 0x20 && cxbyte(*string) < 0x80) ?
                tokenCharTable[*string-0x20] : 0;
            bool prevTokCont = (prevTok&0x80)!=0;
            bool thisTokCont = (thisTok&0x80)!=0;
            if (firstNonSpace && ((prevTok!=thisTok && !(thisTokCont ^ prevTokCont)) || 
                (prevTok==thisTok && (thisTokCont&prevTokCont))))
                break;  // end of token list for macro arg
            outStr.push_back(*string);
            firstNonSpace = false;
            prevTok = thisTok;
        }
        else
        {
            firstNonSpace = true;
            continue; // space
        }
    }
    return true;
}

bool Assembler::resolveExprTarget(const AsmExpression* expr,
                        uint64_t value, AsmSectionId sectionId)
{
    const AsmExprTarget& target = expr->getTarget();
    switch(target.type)
    {
        case ASMXTGT_SYMBOL:
            // resolve symbol
            return setSymbol(*target.symbol, value, sectionId);
        case ASMXTGT_DATA8:
            if (sectionId != ASMSECT_ABS)
                THIS_FAIL_BY_ERROR(expr->getSourcePos(),
                        "Relative value is illegal in data expressions")
            else
            {
                printWarningForRange(8, value, expr->getSourcePos());
                sections[target.sectionId].content[target.offset] = cxbyte(value);
            }
            break;
        case ASMXTGT_DATA16:
            if (sectionId != ASMSECT_ABS)
                THIS_FAIL_BY_ERROR(expr->getSourcePos(),
                        "Relative value is illegal in data expressions")
            else
            {
                printWarningForRange(16, value, expr->getSourcePos());
                SULEV(*reinterpret_cast<uint16_t*>(sections[target.sectionId]
                        .content.data() + target.offset), uint16_t(value));
            }
            break;
        case ASMXTGT_DATA32:
            if (sectionId != ASMSECT_ABS)
                THIS_FAIL_BY_ERROR(expr->getSourcePos(),
                        "Relative value is illegal in data expressions")
            else
            {
                printWarningForRange(32, value, expr->getSourcePos());
                SULEV(*reinterpret_cast<uint32_t*>(sections[target.sectionId]
                        .content.data() + target.offset), uint32_t(value));
            }
            break;
        case ASMXTGT_DATA64:
            if (sectionId != ASMSECT_ABS)
                THIS_FAIL_BY_ERROR(expr->getSourcePos(),
                        "Relative value is illegal in data expressions")
            else
                SULEV(*reinterpret_cast<uint64_t*>(sections[target.sectionId]
                        .content.data() + target.offset), uint64_t(value));
            break;
        // special case for Code flow
        case ASMXTGT_CODEFLOW:
            if (target.sectionId != sectionId)
                THIS_FAIL_BY_ERROR(expr->getSourcePos(), "Jump over current section!")
            else
                sections[target.sectionId].codeFlow[target.cflowId].target = value;
            break;
        default:
            // ISA assembler resolves this dependency
            if (!isaAssembler->resolveCode(expr->getSourcePos(),
                    target.sectionId, sections[target.sectionId].content.data(),
                    target.offset, target.type, sectionId, value))
                return false;
            break;
    }
    return true;
}

void Assembler::cloneSymEntryIfNeeded(AsmSymbolEntry& symEntry)
{
    if (!symEntry.second.occurrencesInExprs.empty() &&
        !symEntry.second.base && !symEntry.second.regRange &&
        ((symEntry.second.expression != nullptr && (
          // if symbol have unevaluated expression but we clone symbol only if
          // before section diffs preparation
          (symEntry.second.withUnevalExpr && !sectionDiffsPrepared) ||
          // or expression have unresolved symbols
                  symEntry.second.expression->getSymOccursNum()!=0)) ||
          // to resolve relocations (no expression but no have hasValue
         (!resolvingRelocs && symEntry.second.expression == nullptr &&
          !symEntry.second.hasValue && !isResolvableSection(symEntry.second.sectionId))))
    {   // create new symbol with this expression
        std::unique_ptr<AsmSymbolEntry> newSymEntry;
        if (symEntry.second.expression!=nullptr)
        {
            newSymEntry.reset(new AsmSymbolEntry(symEntry.first,
                    AsmSymbol(symEntry.second.expression, symEntry.second.onceDefined,
                            symEntry.second.base)));
            symEntry.second.expression->setTarget(
                        AsmExprTarget::symbolTarget(newSymEntry.get()));
        }
        else
        {
            // if have unresolvable section
            newSymEntry.reset(new AsmSymbolEntry(symEntry.first,
                    AsmSymbol(symEntry.second.sectionId, symEntry.second.value,
                              symEntry.second.onceDefined)));
            newSymEntry->second.resolving = symEntry.second.resolving;
            newSymEntry->second.hasValue = symEntry.second.hasValue;
        }
        // replace in expression occurrences
        for (const AsmExprSymbolOccurrence& occur: symEntry.second.occurrencesInExprs)
            occur.expression->replaceOccurrenceSymbol(occur, newSymEntry.get());
        newSymEntry->second.occurrencesInExprs = symEntry.second.occurrencesInExprs;
        symEntry.second.occurrencesInExprs.clear();
        newSymEntry->second.detached = true;
        symbolClones.insert(newSymEntry.release());
        
        symEntry.second.expression = nullptr;
        symEntry.second.hasValue = false;
        symEntry.second.withUnevalExpr = false;
    }
}

bool Assembler::setSymbol(AsmSymbolEntry& symEntry, uint64_t value, AsmSectionId sectionId)
{
    cloneSymEntryIfNeeded(symEntry);
    symEntry.second.value = value;
    symEntry.second.expression = nullptr;
    symEntry.second.sectionId = sectionId;
    symEntry.second.hasValue = isResolvableSection(sectionId) || resolvingRelocs;
    symEntry.second.regRange = false;
    symEntry.second.base = false;
    symEntry.second.withUnevalExpr = false;
    if (!symEntry.second.hasValue) // if not resolved we just return
        return true; // no error
    bool good = true;
    
    // resolve value of pending symbols
    std::stack<std::pair<AsmSymbolEntry*, size_t> > symbolStack;
    symbolStack.push(std::make_pair(&symEntry, 0));
    symEntry.second.resolving = true;
    
    // recursive algorithm in loop form
    while (!symbolStack.empty())
    {
        std::pair<AsmSymbolEntry*, size_t>& entry = symbolStack.top();
        if (entry.second < entry.first->second.occurrencesInExprs.size())
        {
            AsmExprSymbolOccurrence& occurrence =
                    entry.first->second.occurrencesInExprs[entry.second];
            AsmExpression* expr = occurrence.expression;
            expr->substituteOccurrence(occurrence, entry.first->second.value,
                       (!isAbsoluteSymbol(entry.first->second)) ?
                       entry.first->second.sectionId : ASMSECT_ABS);
            entry.second++;
            
            if (!expr->unrefSymOccursNum())
            {
                // expresion has been fully resolved
                uint64_t value;
                AsmSectionId sectionId;
                const AsmExprTarget& target = expr->getTarget();
                if (!resolvingRelocs || target.type==ASMXTGT_SYMBOL)
                {
                    // standard mode
                    AsmTryStatus evalStatus = expr->tryEvaluate(*this, value, sectionId,
                                        withSectionDiffs());
                    if (evalStatus == AsmTryStatus::FAILED)
                    {
                        // if failed
                        delete occurrence.expression; // delete expression
                        good = false;
                        continue;
                    }
                    else if (evalStatus == AsmTryStatus::TRY_LATER)
                    {   // try later if can not be evaluated
                        unevalExpressions.push_back(occurrence.expression);
                        // mark that symbol with this expression have unevaluated
                        // expression and it can be cloned while replacing value
                        if (target.type==ASMXTGT_SYMBOL)
                            target.symbol->second.withUnevalExpr = true;
                        // but still good
                        continue;
                    }
                }
                // resolve expression if at resolving symbol phase
                else if (formatHandler==nullptr ||
                        !formatHandler->resolveRelocation(expr, value, sectionId))
                {
                    // if failed
                    delete occurrence.expression; // delete expression
                    good = false;
                    continue;
                }
                
                // resolving
                if (target.type == ASMXTGT_SYMBOL)
                {    // resolve symbol
                    AsmSymbolEntry& curSymEntry = *target.symbol;
                    if (!curSymEntry.second.resolving &&
                        (!curSymEntry.second.regRange &&
                            curSymEntry.second.expression==expr))
                    {
                        curSymEntry.second.value = value;
                        curSymEntry.second.sectionId = sectionId;
                        curSymEntry.second.withUnevalExpr = false;
                        curSymEntry.second.hasValue =
                            isResolvableSection(sectionId) || resolvingRelocs;
                        symbolStack.push(std::make_pair(&curSymEntry, 0));
                        if (!curSymEntry.second.hasValue)
                            continue;
                        curSymEntry.second.resolving = true;
                        curSymEntry.second.expression = nullptr;
                    }
                    // otherwise we ignore circular dependencies
                }
                else
                    good &= resolveExprTarget(expr, value, sectionId);
                delete occurrence.expression; // delete expression
            }
            else // otherwise we only clear occurrence expression
                occurrence.expression = nullptr; // clear expression
        }
        else // pop
        {
            entry.first->second.resolving = false;
            entry.first->second.occurrencesInExprs.clear();
            if (entry.first->second.snapshot && --(entry.first->second.refCount) == 0)
            {
                symbolSnapshots.erase(entry.first);
                delete entry.first; // delete this symbol snapshot
                entry.first = nullptr;
            }
            // if detached (cloned symbol) while replacing value by unevaluated expression
            if (!doNotRemoveFromSymbolClones &&
                entry.first!=nullptr && entry.first->second.detached)
            {
                symbolClones.erase(entry.first);
                delete entry.first; // delete this symbol snapshot
            }
            symbolStack.pop();
        }
    }
    return good;
}

bool Assembler::assignSymbol(const CString& symbolName, const char* symbolPlace,
             const char* linePtr, bool reassign, bool baseExpr)
{
    skipSpacesToEnd(linePtr, line+lineSize);
    size_t symNameLength = symbolName.size();
    if (symNameLength >= 3 && symbolName.compare(symNameLength-3, 3, "::.")==0)
        THIS_FAIL_BY_ERROR(symbolPlace, "Symbol '.' can be only in global scope")
    
    if (linePtr!=line+lineSize && *linePtr=='%')
    {
        if (symbolName == ".")
            THIS_FAIL_BY_ERROR(symbolPlace, "Symbol '.' requires a resolved expression")
        initializeOutputFormat();
        ++linePtr;
        cxuint regStart, regEnd;
        const AsmRegVar* regVar;
        if (!isaAssembler->parseRegisterRange(linePtr, regStart, regEnd, regVar))
            return false;
        skipSpacesToEnd(linePtr, line+lineSize);
        if (linePtr != line+lineSize)
            THIS_FAIL_BY_ERROR(linePtr, "Garbages at end of expression")
        
        std::pair<AsmSymbolEntry*, bool> res =
                insertSymbolInScope(symbolName, AsmSymbol());
        if (!res.second && ((res.first->second.onceDefined || !reassign) &&
            res.first->second.isDefined()))
        {
            // found and can be only once defined
            printError(symbolPlace, (std::string("Symbol '") + symbolName.c_str() +
                        "' is already defined").c_str());
            return false;
        }
        
        // create symbol clone before setting value
        cloneSymEntryIfNeeded(*res.first);
        
        if (!res.first->second.occurrencesInExprs.empty())
        {
            // regrange found in expressions (error)
            std::vector<std::pair<const AsmExpression*, size_t> > exprs;
            size_t i = 0;
            for (AsmExprSymbolOccurrence occur: res.first->second.occurrencesInExprs)
                exprs.push_back(std::make_pair(occur.expression, i++));
            // remove duplicates
            // sort by value
            std::sort(exprs.begin(), exprs.end(),
                      [](const std::pair<const AsmExpression*, size_t>& a,
                         const std::pair<const AsmExpression*, size_t>& b)
                    {
                        if (a.first==b.first)
                            return a.second<b.second;
                        return a.first<b.first;
                    });
            // make unique and resize (remove duplicates
            exprs.resize(std::unique(exprs.begin(), exprs.end(),
                       [](const std::pair<const AsmExpression*, size_t>& a,
                         const std::pair<const AsmExpression*, size_t>& b)
                       { return a.first==b.first; })-exprs.begin());
            // sort by occurrence
            std::sort(exprs.begin(), exprs.end(),
                      [](const std::pair<const AsmExpression*, size_t>& a,
                         const std::pair<const AsmExpression*, size_t>& b)
                    { return a.second<b.second; });
            // print errors (in order without duplicates)
            for (std::pair<const AsmExpression*, size_t> elem: exprs)
                printError(elem.first->getSourcePos(), "Expression have register symbol");
            
            printError(symbolPlace, (std::string("Register range symbol '") +
                            symbolName.c_str() + "' was used in some expressions").c_str());
            return false;
        }
        
        // setup symbol entry (required)
        AsmSymbolEntry& symEntry = *res.first;
        symEntry.second.expression = nullptr;
        symEntry.second.regVar = regVar;
        symEntry.second.onceDefined = !reassign;
        symEntry.second.base = false;
        symEntry.second.sectionId = ASMSECT_ABS;
        symEntry.second.regRange = symEntry.second.hasValue = true;
        symEntry.second.value = (regStart | (uint64_t(regEnd)<<32));
        symEntry.second.withUnevalExpr = false;
        return true;
    }
    
    const char* exprPlace = linePtr;
    // make base expr if baseExpr=true and symbolName is not output counter
    bool makeBaseExpr = (baseExpr && symbolName != ".");
    
    std::unique_ptr<AsmExpression> expr;
    uint64_t value;
    AsmSectionId sectionId = ASMSECT_ABS;
    if (makeBaseExpr || !AsmExpression::fastExprEvaluate(*this, linePtr, value))
    {
        expr.reset(AsmExpression::parse(*this, linePtr, makeBaseExpr));
        if (!expr) // no expression, errors
            return false;
    }
    
    if (linePtr != line+lineSize)
        THIS_FAIL_BY_ERROR(linePtr, "Garbages at end of expression")
    if (expr && expr->isEmpty()) // empty expression, we treat as error
        THIS_FAIL_BY_ERROR(exprPlace, "Expected assignment expression")
    
    if (symbolName == ".")
    {
        // assigning '.'
        if (!expr) // if fast path
            return assignOutputCounter(symbolPlace, value, sectionId);
        if (!expr->evaluate(*this, value, sectionId))
            return false;
        if (expr->getSymOccursNum()==0)
            return assignOutputCounter(symbolPlace, value, sectionId);
        else
            THIS_FAIL_BY_ERROR(symbolPlace, "Symbol '.' requires a resolved expression")
    }
    
    std::pair<AsmSymbolEntry*, bool> res = insertSymbolInScope(symbolName, AsmSymbol());
    if (!res.second && ((res.first->second.onceDefined || !reassign) &&
        res.first->second.isDefined()))
    {
        // found and can be only once defined
        printError(symbolPlace, (std::string("Symbol '") + symbolName.c_str() +
                    "' is already defined").c_str());
        return false;
    }
    AsmSymbolEntry& symEntry = *res.first;
    
    bool tryLater = false;
    if (!expr)
    {
        setSymbol(symEntry, value, sectionId);
        symEntry.second.onceDefined = !reassign;
    }
    else if (expr->getSymOccursNum()==0)
    {
        // can evalute, assign now
        uint64_t value;
        AsmSectionId sectionId;
        
        AsmTryStatus evalStatus = expr->tryEvaluate(*this, value, sectionId,
                                    withSectionDiffs());
        if (evalStatus == AsmTryStatus::FAILED)
            return false;
        else if (evalStatus == AsmTryStatus::TRY_LATER)
            tryLater = true;
        else
        {   // success
            setSymbol(symEntry, value, sectionId);
            symEntry.second.onceDefined = !reassign;
        }
    }
    else
        tryLater = true;
    
    if (tryLater) // set expression
    {
        cloneSymEntryIfNeeded(symEntry);
        expr->setTarget(AsmExprTarget::symbolTarget(&symEntry));
        if (expr->getSymOccursNum() == 0)
        {
            unevalExpressions.push_back(expr.get());
            // mark that symbol with this expression have unevaluated
            // expression and it can be cloned while replacing value
            // we set it after cloning previous symbol state
            symEntry.second.withUnevalExpr = true;
        }
        else // standard behaviour
            symEntry.second.withUnevalExpr = false;
        symEntry.second.expression = expr.release();
        symEntry.second.regRange = symEntry.second.hasValue = false;
        symEntry.second.onceDefined = !reassign;
        symEntry.second.base = baseExpr;
        if (baseExpr && !symEntry.second.occurrencesInExprs.empty())
        {
            /* make snapshot now resolving dependencies */
            AsmSymbolEntry* tempSymEntry;
            if (!AsmExpression::makeSymbolSnapshot(*this, symEntry, tempSymEntry,
                    &symEntry.second.occurrencesInExprs[0].expression->getSourcePos()))
                return false;
            tempSymEntry->second.occurrencesInExprs =
                        symEntry.second.occurrencesInExprs;
            // clear occurrences after copy
            symEntry.second.occurrencesInExprs.clear();
            if (tempSymEntry->second.hasValue) // set symbol chain
                setSymbol(*tempSymEntry, tempSymEntry->second.value,
                          tempSymEntry->second.sectionId);
        }
    }
    return true;
}

bool Assembler::assignOutputCounter(const char* symbolPlace, uint64_t value,
            AsmSectionId sectionId, cxbyte fillValue)
{
    initializeOutputFormat();
    // checking conditions and fail if not satisfied
    if (currentSection != sectionId && sectionId != ASMSECT_ABS)
        THIS_FAIL_BY_ERROR(symbolPlace, "Illegal section change for symbol '.'")
    if (currentSection != ASMSECT_ABS && int64_t(currentOutPos) > int64_t(value))
        /* check attempt to move backwards only for section that is not absolute */
        THIS_FAIL_BY_ERROR(symbolPlace, "Attempt to move backwards")
    if (!isAddressableSection())
        THIS_FAIL_BY_ERROR(symbolPlace,
                   "Change output counter inside non-addressable section is illegal")
    if (currentSection==ASMSECT_ABS && fillValue!=0)
        printWarning(symbolPlace, "Fill value is ignored inside absolute section");
    if (value-currentOutPos!=0)
        reserveData(value-currentOutPos, fillValue);
    currentOutPos = value;
    return true;
}

// skip symbol name, return false if not symbol
bool Assembler::skipSymbol(const char*& linePtr)
{
    const char* end = line+lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* start = linePtr;
    if (linePtr != end)
    {
        /* skip only symbol name */
        if(isAlpha(*linePtr) || *linePtr == '_' || *linePtr == '.' || *linePtr == '$')
            for (linePtr++; linePtr != end && (isAlnum(*linePtr) || *linePtr == '_' ||
                 *linePtr == '.' || *linePtr == '$') ; linePtr++);
    }
    if (start == linePtr)
    {
        // this is not symbol name
        while (linePtr != end && !isSpace(*linePtr) && *linePtr != ',') linePtr++;
        printError(start, "Expected symbol name");
        return false;
    }
    return true;
}

bool Assembler::isAbsoluteSymbol(const AsmSymbol& symbol) const
{
    if (symbol.sectionId == ASMSECT_ABS)
        return true;
    // otherwise check section
    if (sections.empty())
        return false; // fallback
    const AsmSection& section = sections[symbol.sectionId];
    return (section.flags&ASMSECT_ABS_ADDRESSABLE) != 0;
}

void Assembler::printWarning(const AsmSourcePos& pos, const char* message)
{
    if ((flags & ASM_WARNINGS) == 0)
        return; // do nothing
    pos.print(messageStream);
    messageStream.write(": Warning: ", 11);
    messageStream.write(message, ::strlen(message));
    messageStream.put('\n');
}

void Assembler::printError(const AsmSourcePos& pos, const char* message)
{
    good = false;
    pos.print(messageStream);
    messageStream.write(": Error: ", 9);
    messageStream.write(message, ::strlen(message));
    messageStream.put('\n');
}

// special routine to printing warning when value out of range
void Assembler::printWarningForRange(cxuint bits, uint64_t value, const AsmSourcePos& pos,
        cxbyte signess)
{
    if (bits < 64)
    {
        /* signess - WS_BOTH - check value range as signed value 
         * WS_UNSIGNED - check value as unsigned value */
        if (signess == WS_BOTH &&
            !(int64_t(value) >= (1LL<<bits) || int64_t(value) < -(1LL<<(bits-1))))
            return;
        if (signess == WS_UNSIGNED && (value < (1ULL<<bits)))
            return;
        std::string warning = "Value ";
        char buf[32];
        itocstrCStyle(value, buf, 32, 16);
        warning += buf;
        warning += " truncated to ";
        itocstrCStyle(value&((1ULL<<bits)-1), buf, 32, 16);
        warning += buf;
        printWarning(pos, warning.c_str());
    }
}

void Assembler::addIncludeDir(const CString& includeDir)
{
    includeDirs.push_back(includeDir);
}

void Assembler::addInitialDefSym(const CString& symName, uint64_t value)
{
    defSyms.push_back({symName, value});
}

// push clause to stack ('.ifXXX','macro','rept'), return true if no error
// for '.else' changes current clause
bool Assembler::pushClause(const char* string, AsmClauseType clauseType, bool satisfied,
               bool& included)
{
    if (clauseType == AsmClauseType::MACRO || clauseType == AsmClauseType::IF ||
        clauseType == AsmClauseType::REPEAT)
    {
        // add new clause
        clauses.push({ clauseType, getSourcePos(string), satisfied, { } });
        included = satisfied;
        return true;
    }
    if (clauses.empty())
    {
        // no clauses
        if (clauseType == AsmClauseType::ELSEIF)
            printError(string, "No '.if' before '.elseif");
        else // else
            printError(string, "No '.if' before '.else'");
        return false;
    }
    AsmClause& clause = clauses.top();
    switch(clause.type)
    {
        case AsmClauseType::ELSE:
            if (clauseType == AsmClauseType::ELSEIF)
                printError(string, "'.elseif' after '.else'");
            else // else
                printError(string, "Duplicate of '.else'");
            printError(clause.sourcePos, "here is previous '.else'"); 
            printError(clause.prevIfPos, "here is begin of conditional clause"); 
            return false;
        case AsmClauseType::MACRO:
            THIS_FAIL_BY_ERROR(string, clauseType == AsmClauseType::ELSEIF ?
                        "No '.if' before '.elseif' inside macro" :
                        "No '.if' before '.else' inside macro")
        case AsmClauseType::REPEAT:
            THIS_FAIL_BY_ERROR(string, clauseType == AsmClauseType::ELSEIF ?
                        "No '.if' before '.elseif' inside repetition" :
                        "No '.if' before '.else' inside repetition")
        default:
            break;
    }
    included = satisfied && !clause.condSatisfied;
    clause.condSatisfied |= included;
    if (clause.type == AsmClauseType::IF)
        clause.prevIfPos = clause.sourcePos;
    clause.type = clauseType;
    clause.sourcePos = getSourcePos(string);
    return true;
}

// pop clause ('.endif','.endm','.endr') from stack
bool Assembler::popClause(const char* string, AsmClauseType clauseType)
{
    if (clauses.empty())
    {
        // no clauses
        if (clauseType == AsmClauseType::IF)
            printError(string, "No conditional before '.endif'");
        else if (clauseType == AsmClauseType::MACRO) // macro
            printError(string, "No '.macro' before '.endm'");
        else if (clauseType == AsmClauseType::REPEAT) // repeat
            printError(string, "No '.rept' before '.endr'");
        return false;
    }
    AsmClause& clause = clauses.top();
    // when macro, reepats or conditional finished by wrong pseudo-op
    // ('.macro' -> '.endif', etc)
    switch(clause.type)
    {
        case AsmClauseType::IF:
        case AsmClauseType::ELSE:
        case AsmClauseType::ELSEIF:
            if (clauseType == AsmClauseType::MACRO)
                THIS_FAIL_BY_ERROR(string, "Ending macro across conditionals")
            if (clauseType == AsmClauseType::REPEAT)
                THIS_FAIL_BY_ERROR(string, "Ending repetition across conditionals")
            break;
        case AsmClauseType::MACRO:
            if (clauseType == AsmClauseType::REPEAT)
                THIS_FAIL_BY_ERROR(string, "Ending repetition across macro")
            if (clauseType == AsmClauseType::IF)
                THIS_FAIL_BY_ERROR(string, "Ending conditional across macro")
            break;
        case AsmClauseType::REPEAT:
            if (clauseType == AsmClauseType::MACRO)
                THIS_FAIL_BY_ERROR(string, "Ending macro across repetition")
            if (clauseType == AsmClauseType::IF)
                THIS_FAIL_BY_ERROR(string, "Ending conditional across repetition")
            break;
        default:
            break;
    }
    clauses.pop();
    return true;
}

Assembler::ParseState Assembler::makeMacroSubstitution(const char* linePtr)
{
    const char* end = line+lineSize;
    const char* macroStartPlace = linePtr;
    
    CString macroName = extractSymName(linePtr, end, false);
    if (macroName.empty())
        return ParseState::MISSING;
    if (macroCase)
        toLowerString(macroName);
    AsmMacroMap::const_iterator it = macroMap.find(macroName);
    if (it == macroMap.end())
        return ParseState::MISSING; // macro not found
    
    /* parse arguments */
    RefPtr<const AsmMacro> macro = it->second;
    const size_t macroArgsNum = macro->getArgsNum();
    bool good = true;
    AsmMacroInputFilter::MacroArgMap argMap(macroArgsNum);
    for (size_t i = 0; i < macroArgsNum; i++) // set name of args
        argMap[i].first = macro->getArg(i).name;
    
    for (size_t i = 0; i < macroArgsNum; i++)
    {
        const AsmMacroArg& arg = macro->getArg(i);
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr==',' && i!=0)
            skipCharAndSpacesToEnd(linePtr, end);
        
        std::string macroArg;
        const char* argPlace = linePtr;
        if (!arg.vararg)
        {
            // regular macro argument
            if (!parseMacroArgValue(linePtr, macroArg))
            {
                good = false;
                continue;
            }
        }
        else
        {
            /* parse variadic arguments, they requires ',' separator */
            bool argGood = true;
            while (linePtr != end)
            {
                if (!parseMacroArgValue(linePtr, macroArg))
                {
                    argGood = good = false;
                    break;
                }
                skipSpacesToEnd(linePtr, end);
                if (linePtr!=end)
                {
                    if(*linePtr==',')
                    {
                        skipCharAndSpacesToEnd(linePtr, end);
                        macroArg.push_back(',');
                    }
                    else
                    {
                        printError(linePtr, "Garbages at end of line");
                        argGood = good = false;
                        break;
                    }
                }
            }
            if (!argGood) // not so good
                continue;
        }
        argMap[i].second = macroArg;
        if (arg.required && argMap[i].second.empty())
        {
            // error, value required
            printError(argPlace, (std::string("Value required for macro argument '") +
                    arg.name.c_str() + '\'').c_str());
            good = false;
        }
        else if (argMap[i].second.empty())
            argMap[i].second = arg.defaultValue;
    }
    
    skipSpacesToEnd(linePtr, end);
    if (!good)
        return ParseState::FAILED;
    if (linePtr != end)
    {
        printError(linePtr, "Garbages at end of line");
        return ParseState::FAILED;
    }
    
    // check depth of macro substitution
    if (macroSubstLevel == 1000)
    {
        printError(macroStartPlace, "Macro substitution level is greater than 1000");
        return ParseState::FAILED;
    }
    // sort argmap before using
    mapSort(argMap.begin(), argMap.end());
    // create macro input filter and push to stack
    std::unique_ptr<AsmInputFilter> macroFilter(new AsmMacroInputFilter(macro,
            getSourcePos(macroStartPlace), std::move(argMap), macroCount++,
            alternateMacro));
    asmInputFilters.push(macroFilter.release());
    currentInputFilter = asmInputFilters.top();
    macroSubstLevel++;
    return ParseState::PARSED;
}

struct ScopeUsingStackElem
{
    AsmScope* scope;
    std::list<AsmScope*>::iterator usingIt;
};

// routine to find scope in scope (only traversing by '.using's)
AsmScope* Assembler::findScopeInScope(AsmScope* scope, const CString& scopeName,
                  std::unordered_set<AsmScope*>& scopeSet)
{
    if (!scopeSet.insert(scope).second)
        return nullptr;
    std::stack<ScopeUsingStackElem> usingStack;
    usingStack.push(ScopeUsingStackElem{ scope, scope->usedScopes.begin() });
    while (!usingStack.empty())
    {
        ScopeUsingStackElem& current = usingStack.top();
        AsmScope* curScope = current.scope;
        if (current.usingIt == curScope->usedScopes.begin())
        {
            // first we found in this scope
            auto it = curScope->scopeMap.find(scopeName);
            if (it != curScope->scopeMap.end())
                return it->second;
        }
        // next we find in used children
        if (current.usingIt != curScope->usedScopes.end())
        {
            AsmScope* child = *current.usingIt;
            if (scopeSet.insert(child).second) // we insert, new
                usingStack.push(ScopeUsingStackElem{ child, child->usedScopes.begin() });
            ++current.usingIt; // next
        }
        else // back
            usingStack.pop();
    }
    return nullptr;
}

AsmScope* Assembler::getRecurScope(const CString& scopePlace, bool ignoreLast,
                    const char** lastStep)
{
    AsmScope* scope = currentScope;
    const char* str = scopePlace.c_str();
    if (*str==':' && str[1]==':')
    {
        // choose global scope
        scope = &globalScope;
        str += 2;
    }
    
    std::vector<CString> scopeTrack;
    const char* lastStepCur = str;
    while (*str != 0)
    {
        const char* scopeNameStr = str;
        while (*str!=':' && *str!=0) str++;
        if (*str==0 && ignoreLast) // ignore last
            break;
        scopeTrack.push_back(CString(scopeNameStr, str));
        if (*str==':' && str[1]==':')
            str += 2;
        lastStepCur = str;
    }
    if (lastStep != nullptr)
        *lastStep = lastStepCur;
    if (scopeTrack.empty()) // no scope path
        return scope;
    
    std::unordered_set<AsmScope*> scopeSet;
    for (AsmScope* scope2 = scope; scope2 != nullptr; scope2 = scope2->parent)
    {  // find this scope
        AsmScope* newScope = findScopeInScope(scope2, scopeTrack[0], scopeSet);
        if (newScope != nullptr)
        {
            scope = newScope->parent;
            break;
        }
    }
    
    // otherwise create in current/global scope
    for (const CString& name: scopeTrack)
        getScope(scope, name, scope);
    return scope;
}

// internal routine to find symbol in scope (only traversing by '.using's)
AsmSymbolEntry* Assembler::findSymbolInScopeInt(AsmScope* scope,
                    const CString& symName, std::unordered_set<AsmScope*>& scopeSet)
{
    if (!scopeSet.insert(scope).second)
        return nullptr;
    std::stack<ScopeUsingStackElem> usingStack;
    usingStack.push(ScopeUsingStackElem{ scope, scope->usedScopes.begin() });
    while (!usingStack.empty())
    {
        ScopeUsingStackElem& current = usingStack.top();
        AsmScope* curScope = current.scope;
        if (current.usingIt == curScope->usedScopes.begin())
        {
            // first we found in this scope
            AsmSymbolMap::iterator it = curScope->symbolMap.find(symName);
            if (it != curScope->symbolMap.end())
                return &*it;
        }
        // next we find in used children
        if (current.usingIt != curScope->usedScopes.end())
        {
            AsmScope* child = *current.usingIt;
            if (scopeSet.insert(child).second) // we insert, new
                usingStack.push(ScopeUsingStackElem{ child, child->usedScopes.begin() });
            ++current.usingIt; // next
        }
        else // back
            usingStack.pop();
    }
    return nullptr;
}

// real routine to find symbol in scope (traverse by all visible scopes)
AsmSymbolEntry* Assembler::findSymbolInScope(const CString& symName, AsmScope*& scope,
            CString& sameSymName, bool insertMode)
{
    const char* lastStep = nullptr;
    scope = getRecurScope(symName, true, &lastStep);
    std::unordered_set<AsmScope*> scopeSet;
    AsmSymbolEntry* foundSym = findSymbolInScopeInt(scope, lastStep, scopeSet);
    sameSymName = lastStep;
    if (foundSym != nullptr)
        return foundSym;
    if (lastStep != symName)
        return nullptr;
    // otherwise is symName is not normal symName
    scope = currentScope;
    if (insertMode)
        return nullptr;
    
    for (AsmScope* scope2 = scope; scope2 != nullptr; scope2 = scope2->parent)
    {  // find this scope
        foundSym = findSymbolInScopeInt(scope2, lastStep, scopeSet);
        if (foundSym != nullptr)
            return foundSym;
    }
    return nullptr;
}

std::pair<AsmSymbolEntry*, bool> Assembler::insertSymbolInScope(const CString& symName,
                 const AsmSymbol& symbol)
{
    AsmScope* outScope;
    CString sameSymName;
    AsmSymbolEntry* symEntry = findSymbolInScope(symName, outScope, sameSymName, true);
    if (symEntry==nullptr)
    {
        auto res = outScope->symbolMap.insert({ sameSymName, symbol });
        return std::make_pair(&*res.first, res.second);
    }
    return std::make_pair(symEntry, false);
}

// internal routine to find regvar in scope (only traversing by '.using's)
AsmRegVarEntry* Assembler::findRegVarInScopeInt(AsmScope* scope, const CString& rvName,
                std::unordered_set<AsmScope*>& scopeSet)
{
    if (!scopeSet.insert(scope).second)
        return nullptr;
    std::stack<ScopeUsingStackElem> usingStack;
    usingStack.push(ScopeUsingStackElem{ scope, scope->usedScopes.begin() });
    while (!usingStack.empty())
    {
        ScopeUsingStackElem& current = usingStack.top();
        AsmScope* curScope = current.scope;
        if (current.usingIt == curScope->usedScopes.begin())
        {
            // first we found in this scope
            AsmRegVarMap::iterator it = curScope->regVarMap.find(rvName);
            if (it != curScope->regVarMap.end())
                return &*it;
        }
        // next we find in used children
        if (current.usingIt != curScope->usedScopes.end())
        {
            AsmScope* child = *current.usingIt;
            if (scopeSet.insert(child).second) // we insert, new
                usingStack.push(ScopeUsingStackElem{ child, child->usedScopes.begin() });
            ++current.usingIt; // next
        }
        else // back
            usingStack.pop();
    }
    return nullptr;
}

// real routine to find regvar in scope (traverse by all visible scopes)
AsmRegVarEntry* Assembler::findRegVarInScope(const CString& rvName, AsmScope*& scope,
                      CString& sameRvName, bool insertMode)
{
    const char* lastStep = nullptr;
    scope = getRecurScope(rvName, true, &lastStep);
    std::unordered_set<AsmScope*> scopeSet;
    AsmRegVarEntry* foundRv = findRegVarInScopeInt(scope, lastStep, scopeSet);
    sameRvName = lastStep;
    if (foundRv != nullptr)
        return foundRv;
    if (lastStep != rvName)
        return nullptr;
    // otherwise is rvName is not normal rvName
    scope = currentScope;
    if (insertMode)
        return nullptr;
    
    for (AsmScope* scope2 = scope; scope2 != nullptr; scope2 = scope2->parent)
    {  // find this scope
        foundRv = findRegVarInScopeInt(scope2, lastStep, scopeSet);
        if (foundRv != nullptr)
            return foundRv;
    }
    return nullptr;
}

std::pair<AsmRegVarEntry*, bool> Assembler::insertRegVarInScope(const CString& rvName,
                 const AsmRegVar& regVar)
{
    AsmScope* outScope;
    CString sameRvName;
    AsmRegVarEntry* rvEntry = findRegVarInScope(rvName, outScope, sameRvName, true);
    if (rvEntry==nullptr)
    {
        auto res = outScope->regVarMap.insert({ sameRvName, regVar });
        return std::make_pair(&*res.first, res.second);
    }
    return std::make_pair(rvEntry, false);
}

bool Assembler::getScope(AsmScope* parent, const CString& scopeName, AsmScope*& scope)
{
    std::unordered_set<AsmScope*> scopeSet;
    AsmScope* foundScope = findScopeInScope(parent, scopeName, scopeSet);
    if (foundScope != nullptr)
    {
        scope = foundScope;
        return false;
    }
    std::unique_ptr<AsmScope> newScope(new AsmScope(parent));
    auto res = parent->scopeMap.insert(std::make_pair(scopeName, newScope.get()));
    scope = newScope.release();
    return res.second;
}

bool Assembler::pushScope(const CString& scopeName)
{
    if (scopeName.empty())
    {
        // temporary scope
        std::unique_ptr<AsmScope> newScope(new AsmScope(currentScope, true));
        currentScope->scopeMap.insert(std::make_pair("", newScope.get()));
        currentScope = newScope.release();
    }
    else
        getScope(currentScope, scopeName, currentScope);
    scopeStack.push(currentScope);
    return true; // always good even if scope exists
}

bool Assembler::popScope()
{
    if (scopeStack.empty())
        return false; // can't pop scope
    if (currentScope->temporary)
    {
        // delete scope
        currentScope->parent->scopeMap.erase("");
        const bool oldResolvingRelocs = resolvingRelocs;
        resolvingRelocs = true; // allow to resolve relocations
        tryToResolveSymbols(currentScope);
        printUnresolvedSymbols(currentScope);
        resolvingRelocs = oldResolvingRelocs;
        currentScope->deleteSymbolsRecursively();
        abandonedScopes.push_back(currentScope);
    }
    scopeStack.pop();
    currentScope = (!scopeStack.empty()) ? scopeStack.top() : &globalScope;
    return true;
}

bool Assembler::includeFile(const char* pseudoOpPlace, const std::string& filename)
{
    if (inclusionLevel == 500)
        THIS_FAIL_BY_ERROR(pseudoOpPlace, "Inclusion level is greater than 500")
    std::unique_ptr<AsmInputFilter> newInputFilter(new AsmStreamInputFilter(
                getSourcePos(pseudoOpPlace), filename));
    asmInputFilters.push(newInputFilter.release());
    currentInputFilter = asmInputFilters.top();
    inclusionLevel++;
    return true;
}

bool Assembler::readLine()
{
    line = currentInputFilter->readLine(*this, lineSize);
    while (line == nullptr)
    {
        // no line
        if (asmInputFilters.size() > 1)
        {
            /* decrease some level of a nesting */
            if (currentInputFilter->getType() == AsmInputFilterType::MACROSUBST)
                macroSubstLevel--;
            else if (currentInputFilter->getType() == AsmInputFilterType::STREAM)
                inclusionLevel--;
            else if (currentInputFilter->getType() == AsmInputFilterType::REPEAT)
                repetitionLevel--;
            delete asmInputFilters.top();
            asmInputFilters.pop();
        }
        else if (filenameIndex<filenames.size())
        {
            /* handling input assembler that have many files */
            do {
                // delete previous filter
                delete asmInputFilters.top();
                asmInputFilters.pop();
                /// create new input filter
                std::unique_ptr<AsmStreamInputFilter> thatFilter(
                    new AsmStreamInputFilter(filenames[filenameIndex++]));
                asmInputFilters.push(thatFilter.get());
                currentInputFilter = thatFilter.release();
                line = currentInputFilter->readLine(*this, lineSize);
            } while (line==nullptr && filenameIndex<filenames.size());
            
            return (line!=nullptr);
        }
        else
            return false;
        currentInputFilter = asmInputFilters.top();
        line = currentInputFilter->readLine(*this, lineSize);
    }
    return true;
}

// reserve data in current section and fill values (used by '.skip' or position movement)
cxbyte* Assembler::reserveData(size_t size, cxbyte fillValue)
{
    if (currentSection != ASMSECT_ABS)
    {
        size_t oldOutPos = currentOutPos;
        AsmSection& section = sections[currentSection];
        if ((section.flags & ASMSECT_WRITEABLE) == 0)
        {
             // non writeable, only change output position (do not fill)
            currentOutPos += size;
            section.size += size;
            return nullptr;
        }
        else
        {
            section.content.insert(section.content.end(), size, fillValue);
            currentOutPos += size;
            return section.content.data() + oldOutPos;
        }
    }
    else
    {
        currentOutPos += size;
        return nullptr;
    }
}


void Assembler::goToMain(const char* pseudoOpPlace)
{
    try
    { formatHandler->setCurrentKernel(ASMKERN_GLOBAL); }
    catch(const AsmFormatException& ex) // if error
    { printError(pseudoOpPlace, ex.what()); }
    
    currentOutPos = sections[currentSection].getSize();
}

void Assembler::goToKernel(const char* pseudoOpPlace, const char* kernelName)
{
    auto kmit = kernelMap.find(kernelName);
    if (kmit == kernelMap.end())
    {
        // not found, add new kernel
        AsmKernelId kernelId;
        try
        { kernelId = formatHandler->addKernel(kernelName); }
        catch(const AsmFormatException& ex)
        {
            // error!
            printError(pseudoOpPlace, ex.what());
            return;
        }
        // add new kernel entries and section entry
        auto it = kernelMap.insert(std::make_pair(kernelName, kernelId)).first;
        kernels.push_back({ it->first.c_str(),  getSourcePos(pseudoOpPlace) });
        auto info = formatHandler->getSectionInfo(currentSection);
        sections.push_back({ info.name, currentKernel, info.type, info.flags, 0,
                    0, info.relSpace });
        currentOutPos = 0;
    }
    else
    {
        // found
        try
        { formatHandler->setCurrentKernel(kmit->second); }
        catch(const AsmFormatException& ex) // if error
        {
            printError(pseudoOpPlace, ex.what());
            return;
        }
        
        currentOutPos = sections[currentSection].getSize();
    }
}

void Assembler::goToSection(const char* pseudoOpPlace, const char* sectionName,
                            uint64_t align)
{
    const AsmSectionId sectionId = formatHandler->getSectionId(sectionName);
    if (sectionId == ASMSECT_NONE)
    {
        // try to add new section
        AsmSectionId sectionId;
        try
        { sectionId = formatHandler->addSection(sectionName, currentKernel); }
        catch(const AsmFormatException& ex)
        {
            // error!
            printError(pseudoOpPlace, ex.what());
            return;
        }
        auto info = formatHandler->getSectionInfo(sectionId);
        sections.push_back({ info.name, currentKernel, info.type, info.flags, align,
                    0, info.relSpace });
        currentOutPos = 0;
    }
    else // if section exists
    {
        // found, try to set
        try
        { formatHandler->setCurrentSection(sectionId); }
        catch(const AsmFormatException& ex) // if error
        {
            printError(pseudoOpPlace, ex.what());
            return;
        }
        
        if (align!=0)
            printWarning(pseudoOpPlace, "Section alignment was ignored");
        currentOutPos = sections[currentSection].getSize();
    }
}

// go to section, when not exists create it with specified alignment and flags
void Assembler::goToSection(const char* pseudoOpPlace, const char* sectionName,
        AsmSectionType type, Flags flags, uint64_t align)
{
    const AsmSectionId sectionId = formatHandler->getSectionId(sectionName);
    if (sectionId == ASMSECT_NONE)
    {
        // try to add new section
        AsmSectionId sectionId;
        try
        { sectionId = formatHandler->addSection(sectionName, currentKernel); }
        catch(const AsmFormatException& ex)
        {
            // error!
            printError(pseudoOpPlace, ex.what());
            return;
        }
        auto info = formatHandler->getSectionInfo(sectionId);
        if (info.type == AsmSectionType::EXTRA_SECTION)
        {
            info.type = type;
            info.flags |= flags;
        }
        else
            printWarning(pseudoOpPlace,
                     "Section type and flags was ignored for builtin section");
        sections.push_back({ info.name, currentKernel, info.type, info.flags, align,
                    0, info.relSpace });
        currentOutPos = 0;
    }
    else // if section exists
    {
        // found, try to set
        try
        { formatHandler->setCurrentSection(sectionId); }
        catch(const AsmFormatException& ex) // if error
        {
            printError(pseudoOpPlace, ex.what());
            return;
        }
        
        printWarning(pseudoOpPlace, "Section type, flags and alignment was ignored");
        currentOutPos = sections[currentSection].getSize();
    }
}

// go to section, when not exists create it with specified alignment
void Assembler::goToSection(const char* pseudoOpPlace, AsmSectionId sectionId,
                        uint64_t align)
{
    try
    { formatHandler->setCurrentSection(sectionId); }
    catch(const AsmFormatException& ex) // if error
    { printError(pseudoOpPlace, ex.what()); }
    if (sectionId >= sections.size())
    {
        auto info = formatHandler->getSectionInfo(sectionId);
        sections.push_back({ info.name, currentKernel, info.type, info.flags, align });
    }
    else if (align!=0)
        printWarning(pseudoOpPlace, "Section alignment was ignored");
    
    currentOutPos = sections[currentSection].getSize();
}

// used in places, to initialize lazily (if needed) output format 
void Assembler::initializeOutputFormat()
{
    if (formatHandler!=nullptr)
        return;
    switch(format)
    {
        case BinaryFormat::AMD:
            formatHandler = new AsmAmdHandler(*this);
            break;
        case BinaryFormat::AMDCL2:
            formatHandler = new AsmAmdCL2Handler(*this);
            break;
        case BinaryFormat::GALLIUM:
            formatHandler = new AsmGalliumHandler(*this);
            break;
        case BinaryFormat::ROCM:
            formatHandler = new AsmROCmHandler(*this);
            break;
        default:
            formatHandler = new AsmRawCodeHandler(*this);
            break;
    }
    isaAssembler = new GCNAssembler(*this);
    // add first section
    auto info = formatHandler->getSectionInfo(currentSection);
    sections.push_back({ info.name, currentKernel, info.type, info.flags, 0,
                0, info.relSpace });
    currentOutPos = 0;
}

bool Assembler::getRegVar(const CString& name, const AsmRegVar*& regVar)
{ 
    regVar = nullptr;
    CString sameRvName;
    AsmScope* scope;
    auto it = findRegVarInScope(name, scope, sameRvName);
    if (it == nullptr)
        return false;
    regVar = &it->second;
    return true;
}

void Assembler::handleRegionsOnKernels(const std::vector<AsmKernelId>& newKernels,
                const std::vector<AsmKernelId>& oldKernels, AsmSectionId codeSection)
{
    auto oldit = oldKernels.begin();
    auto newit = newKernels.begin();
    auto olditend = oldKernels.end();
    auto newitend = newKernels.end();
    
    while (oldit != olditend || newit != newitend)
    {
        if (newit == newitend || (oldit != olditend &&  *oldit < *newit))
        {
            // no kernel in new set (close this region)
            kernels[*oldit].closeCodeRegion(sections[codeSection].content.size());
            ++oldit;
        }
        else if (oldit == olditend || (newit != newitend && *newit < *oldit))
        {
            // kernel in new set but not in old (open this region)
            kernels[*newit].openCodeRegion(sections[codeSection].content.size());
            ++newit;
        }
        else
        {
            // this same kernel in kernel, no changes
            ++oldit;
            ++newit;
        }
    }
}

struct ScopeStackElem
{
    std::pair<CString, AsmScope*> scope;
    AsmScopeMap::iterator childIt;
};

void Assembler::tryToResolveSymbol(AsmSymbolEntry& symEntry)
{
    if (!symEntry.second.occurrencesInExprs.empty() ||
        (symEntry.first!="." &&
                !isResolvableSection(symEntry.second.sectionId)))
    {
        // try to resolve symbols
        uint64_t value;
        AsmSectionId sectionId;
        if (formatHandler!=nullptr &&
            formatHandler->resolveSymbol(symEntry.second, value, sectionId))
            setSymbol(symEntry, value, sectionId);
    }
}

// try to resolve symbols in scope (after closing temporary scope or
// ending assembly for global scope)
void Assembler::tryToResolveSymbols(AsmScope* thisScope)
{
    std::deque<ScopeStackElem> scopeStack;
    std::pair<CString, AsmScope*> globalScopeEntry = { "", thisScope };
    scopeStack.push_back({ globalScopeEntry, thisScope->scopeMap.begin() });
    
    while (!scopeStack.empty())
    {
        ScopeStackElem& elem = scopeStack.back();
        if (elem.childIt == elem.scope.second->scopeMap.begin())
        {
            // first we check symbol of current scope
            AsmScope* curScope = elem.scope.second;
            for (AsmSymbolEntry& symEntry: curScope->symbolMap)
                tryToResolveSymbol(symEntry);
        }
        // next, we travere on children
        if (elem.childIt != elem.scope.second->scopeMap.end())
        {
            scopeStack.push_back({ *elem.childIt,
                        elem.childIt->second->scopeMap.begin() });
            ++elem.childIt;
        }
        else // if end, we pop from stack
            scopeStack.pop_back();
    }
}

// print unresolved symbols in global scope after assemblying
// or when popping temporary scope
void Assembler::printUnresolvedSymbols(AsmScope* thisScope)
{
    if ((flags&ASM_TESTRUN) != 0 && (flags&ASM_TESTRESOLVE) == 0)
        return;
    
    std::deque<ScopeStackElem> scopeStack;
    std::pair<CString, AsmScope*> globalScopeEntry = { "", thisScope };
    scopeStack.push_back({ globalScopeEntry, thisScope->scopeMap.begin() });
    
    while (!scopeStack.empty())
    {
        ScopeStackElem& elem = scopeStack.back();
        if (elem.childIt == elem.scope.second->scopeMap.begin())
        {
            // first we check symbol of current scope
            AsmScope* curScope = elem.scope.second;
            for (AsmSymbolEntry& symEntry: curScope->symbolMap)
                if (!symEntry.second.occurrencesInExprs.empty())
                    for (AsmExprSymbolOccurrence occur:
                            symEntry.second.occurrencesInExprs)
                    {
                        std::string scopePath;
                        auto it = scopeStack.begin(); // skip global scope
                        for (++it; it != scopeStack.end(); ++it)
                        {
                            // generate scope path
                            scopePath += it->scope.first.c_str();
                            scopePath += "::";
                        }
                        // print error, if symbol is unresolved
                        printError(occur.expression->getSourcePos(),(std::string(
                            "Unresolved symbol '")+scopePath+
                            symEntry.first.c_str()+"'").c_str());
                    }
        }
        // next, we travere on children
        if (elem.childIt != elem.scope.second->scopeMap.end())
        {
            scopeStack.push_back({ *elem.childIt,
                        elem.childIt->second->scopeMap.begin() });
            ++elem.childIt;
        }
        else // if end, we pop from stack
            scopeStack.pop_back();
    }
}

bool Assembler::assemble()
{
    resolvingRelocs = false;
    doNotRemoveFromSymbolClones = false;
    sectionDiffsPrepared = false;
    
    for (const DefSym& defSym: defSyms)
        if (defSym.first!=".")
            globalScope.symbolMap[defSym.first] = AsmSymbol(ASMSECT_ABS, defSym.second);
        else if ((flags & ASM_WARNINGS) != 0)// ignore for '.'
            messageStream << "<command-line>: Warning: Definition for symbol '.' "
                    "was ignored" << std::endl;
    
    good = true;
    while (!endOfAssembly)
    {
        if (!lineAlreadyRead)
        {
            // read line
            if (!readLine())
                break;
        }
        else
        {
            // already line is read
            lineAlreadyRead = false;
            if (line == nullptr)
                break; // end of stream
        }
        
        const char* linePtr = line; // string points to place of line
        const char* end = line+lineSize;
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end)
            continue; // only empty line
        
        // statement start (except labels). in this time can point to labels
        const char* stmtPlace = linePtr;
        CString firstName = extractLabelName(linePtr, end);
        
        skipSpacesToEnd(linePtr, end);
        
        bool doNextLine = false;
        while (!firstName.empty() && linePtr != end && *linePtr == ':' &&
                    (linePtr+1==end || linePtr[1]!=':'))
        {
            // labels
            linePtr++;
            skipSpacesToEnd(linePtr, end);
            initializeOutputFormat();
            if (firstName.front() >= '0' && firstName.front() <= '9')
            {
                // handle local labels
                if (sections.empty())
                {
                    printError(stmtPlace, "Local label can't be defined outside section");
                    doNextLine = true;
                    break;
                }
                if (!isAddressableSection())
                {
                    printError(stmtPlace, "Local label can't be defined in "
                            "non-addressable section ");
                    doNextLine = true;
                    break;
                }
                /* prevLRes - iterator to previous instance of local label (with 'b)
                 * nextLRes - iterator to next instance of local label (with 'f) */
                AsmSymbolEntry& prevLRes =
                        *globalScope.symbolMap.insert(std::make_pair(
                            std::string(firstName.c_str())+"b", AsmSymbol())).first;
                AsmSymbolEntry& nextLRes =
                        *globalScope.symbolMap.insert(std::make_pair(
                            std::string(firstName.c_str())+"f", AsmSymbol())).first;
                /* resolve forward symbol of label now */
                assert(setSymbol(nextLRes, currentOutPos, currentSection));
                // move symbol value from next local label into previous local label
                // clearOccurrences - obsolete - back local labels are undefined!
                prevLRes.second.value = nextLRes.second.value;
                prevLRes.second.hasValue = isResolvableSection();
                prevLRes.second.sectionId = currentSection;
                /// make forward symbol of label as undefined
                nextLRes.second.hasValue = false;
            }
            else
            {
                // regular labels
                if (firstName==".")
                {
                    printError(stmtPlace, "Symbol '.' can't be a label");
                    break;
                }
                /*std::pair<AsmSymbolMap::iterator, bool> res = 
                        currentScope->symbolMap.insert(
                            std::make_pair(firstName, AsmSymbol()));*/
                std::pair<AsmSymbolEntry*, bool> res =
                            insertSymbolInScope(firstName, AsmSymbol());
                if (!res.second)
                {
                    // found
                    if (res.first->second.onceDefined && res.first->second.isDefined())
                    {
                        // if label
                        printError(stmtPlace, (std::string("Symbol '")+firstName.c_str()+
                                    "' is already defined").c_str());
                        doNextLine = true;
                        break;
                    }
                }
                if (sections.empty())
                {
                    printError(stmtPlace,
                               "Label can't be defined outside section");
                    doNextLine = true;
                    break;
                }
                if (!isAddressableSection())
                {
                    printError(stmtPlace, "Label can't be defined in "
                            "non-addressable section ");
                    doNextLine = true;
                    break;
                }
                
                setSymbol(*res.first, currentOutPos, currentSection);
                res.first->second.onceDefined = true;
                res.first->second.sectionId = currentSection;
                
                formatHandler->handleLabel(res.first->first);
            }
            // new label or statement
            stmtPlace = linePtr;
            firstName = extractLabelName(linePtr, end);
        }
        if (doNextLine)
            continue;
        
        /* now stmtStartStr - points to first string of statement
         * (labels has been skipped) */
        skipSpacesToEnd(linePtr, end);
        if (linePtr != end && *linePtr == '=' &&
            // not for local labels
            !isDigit(firstName.front()))
        {
            // assignment
            skipCharAndSpacesToEnd(linePtr, line+lineSize);
            if (linePtr == end)
            {
                printError(linePtr, "Expected assignment expression");
                continue;
            }
            assignSymbol(firstName, stmtPlace, linePtr);
            continue;
        }
        // make firstname as lowercase
        toLowerString(firstName);
        
        const AsmSectionId oldCurrentSection = currentSection;
        const uint64_t oldCurrentOutPos = currentOutPos;
        AsmSourcePos sourcePos{};
        if (collectSourcePoses)
            // source pos for sourcePosHandler
            sourcePos = getSourcePos(stmtPlace);
        
        if (firstName.size() >= 2 && firstName[0] == '.') // check for pseudo-op
            parsePseudoOps(firstName, stmtPlace, linePtr);
        else if (firstName.size() >= 1 && isDigit(firstName[0]))
            printError(stmtPlace, "Illegal number at statement begin");
        else
        {
            // try to parse processor instruction or macro substitution
            if (makeMacroSubstitution(stmtPlace) == ParseState::MISSING)
            {  
                if (firstName.empty()) // if name is empty
                {
                    if (linePtr!=end) // error
                        printError(stmtPlace, "Garbages at statement place");
                    continue;
                }
                initializeOutputFormat();
                // try parse instruction
                if (!isWriteableSection())
                {
                    printError(stmtPlace,
                       "Writing data into non-writeable section is illegal");
                    continue;
                }
                
                if (sections[currentSection].usageHandler == nullptr)
                    sections[currentSection].usageHandler.reset(
                            isaAssembler->createUsageHandler());
                
                if (sections[currentSection].linearDepHandler == nullptr)
                    sections[currentSection].linearDepHandler.reset(
                            new ISALinearDepHandler());
                
                if (sections[currentSection].waitHandler == nullptr)
                    sections[currentSection].waitHandler.reset(new ISAWaitHandler());
                
                isaAssembler->assemble(firstName, stmtPlace, linePtr, end,
                           sections[currentSection].content,
                           sections[currentSection].usageHandler.get(),
                           sections[currentSection].waitHandler.get());
                currentOutPos = sections[currentSection].getSize();
            }
        }
        
        // register offset-sourcePos (only if enabled)
        if (collectSourcePoses && oldCurrentSection == currentSection &&
            currentSection != ASMSECT_ABS && oldCurrentOutPos != currentOutPos)
            sections[currentSection].sourcePosHandler.pushSourcePos(
                            oldCurrentOutPos, sourcePos);
    }
    /* check clauses and print errors */
    while (!clauses.empty())
    {
        const AsmClause& clause = clauses.top();
        switch(clause.type)
        {
            case AsmClauseType::IF:
                printError(clause.sourcePos, "Unterminated '.if'");
                break;
            case AsmClauseType::ELSEIF:
                printError(clause.sourcePos, "Unterminated '.elseif'");
                printError(clause.prevIfPos, "here is begin of conditional clause"); 
                break;
            case AsmClauseType::ELSE:
                printError(clause.sourcePos, "Unterminated '.else'");
                printError(clause.prevIfPos, "here is begin of conditional clause"); 
                break;
            case AsmClauseType::MACRO:
                printError(clause.sourcePos, "Unterminated macro definition");
                break;
            case AsmClauseType::REPEAT:
                printError(clause.sourcePos, "Unterminated repetition");
            default:
                break;
        }
        clauses.pop();
    }
    
    if (withSectionDiffs())
    {
        formatHandler->prepareSectionDiffsResolving();
        sectionDiffsPrepared = true;
    }
    
    resolvingRelocs = true;
    tryToResolveSymbols(&globalScope);
    doNotRemoveFromSymbolClones = true;
    for (AsmSymbolEntry* symEntry: symbolClones)
        tryToResolveSymbol(*symEntry);
    doNotRemoveFromSymbolClones = false;
    
    if (withSectionDiffs())
    {
        resolvingRelocs = false;
        for (AsmExpression*& expr: unevalExpressions)
        {
            // try to resolve unevaluated expressions
            uint64_t value;
            AsmSectionId sectionId;
            if (expr->evaluate(*this, value, sectionId))
                resolveExprTarget(expr, value, sectionId);
            delete expr;
            expr = nullptr;
        }
        resolvingRelocs = true;
    }
    
    printUnresolvedSymbols(&globalScope);
    
    if (good && formatHandler!=nullptr)
    {
        // code opened regions for kernels
        for (AsmKernelId i = 0; i < kernels.size(); i++)
        {
            currentKernel = i;
            AsmSectionId sectionId = formatHandler->getSectionId(".text");
            if (sectionId == ASMSECT_NONE)
            {
                currentKernel = ASMKERN_GLOBAL;
                sectionId = formatHandler->getSectionId(".text");
            }
            const size_t contentSize = (sectionId != ASMSECT_NONE) ?
                    sections[sectionId].content.size() : size_t(0);
            kernels[i].closeCodeRegion(contentSize);
        }
        // prepare binary
        formatHandler->prepareBinary();
    }
    return good;
}

void Assembler::writeBinary(const char* filename) const
{
    if (good)
    {
        const AsmFormatHandler* formatHandler = getFormatHandler();
        if (formatHandler!=nullptr)
        {
            std::ofstream ofs(filename, std::ios::binary);
            if (ofs)
                formatHandler->writeBinary(ofs);
            else
                throw AsmException(std::string("Can't open output file '")+filename+"'");
        }
        else
            throw AsmException("No output binary");
    }
    else // failed
        throw AsmException("Assembler failed!");
}

void Assembler::writeBinary(std::ostream& outStream) const
{
    if (good)
    {
        const AsmFormatHandler* formatHandler = getFormatHandler();
        if (formatHandler!=nullptr)
            formatHandler->writeBinary(outStream);
        else
            throw AsmException("No output binary");
    }
    else // failed
        throw AsmException("Assembler failed!");
}

void Assembler::writeBinary(Array<cxbyte>& array) const
{
    if (good)
    {
        const AsmFormatHandler* formatHandler = getFormatHandler();
        if (formatHandler!=nullptr)
            formatHandler->writeBinary(array);
        else
            throw AsmException("No output binary");
    }
    else // failed
        throw AsmException("Assembler failed!");
}
