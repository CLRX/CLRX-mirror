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
#include <string>
#include <cassert>
#include <fstream>
#include <vector>
#include <stack>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

bool AsmSection::addRegVar(const CString& name, const AsmRegVar& var)
{ return regVars.insert(std::make_pair(name, var)).second; }

bool AsmSection::getRegVar(const CString& name, const AsmRegVar*& regVar) const
{ 
    auto it = regVars.find(name);
    if (it==regVars.end())
        return false;
    regVar = &it->second;
    return true;
}

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
            {   /* handle \@ and \() for correct parsing */
                linePtr++;
                if (linePtr!=end)
                {
                    if (*linePtr=='@')
                        linePtr++;
                    if (*linePtr=='(')
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

bool AsmParseUtils::checkGarbagesAtEnd(Assembler& asmr, const char* linePtr)
{
    skipSpacesToEnd(linePtr, asmr.line + asmr.lineSize);
    if (linePtr != asmr.line + asmr.lineSize)
    {
        asmr.printError(linePtr, "Garbages at end of line");
        return false;
    }
    return true;
}

bool AsmParseUtils::getAbsoluteValueArg(Assembler& asmr, uint64_t& value,
                      const char*& linePtr, bool requiredExpr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* exprPlace = linePtr;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr, false, true));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty() && requiredExpr)
    {
        asmr.printError(exprPlace, "Expected expression");
        return false;
    }
    if (expr->isEmpty()) // do not set if empty expression
        return true;
    cxuint sectionId; // for getting
    if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
        return false;
    else if (sectionId != ASMSECT_ABS)
    {   // if not absolute value
        asmr.printError(exprPlace, "Expression must be absolute!");
        return false;
    }
    return true;
}

bool AsmParseUtils::getAnyValueArg(Assembler& asmr, uint64_t& value,
                   cxuint& sectionId, const char*& linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* exprPlace = linePtr;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr, false, true));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty())
    {
        asmr.printError(exprPlace, "Expected expression");
        return false;
    }
    if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
        return false;
    return true;
}

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
    {
        asmr.printError(exprPlace, "Expected expression");
        return false;
    }
    if (expr->getSymOccursNum()==0)
    {
        cxuint sectionId;
        if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
            return false;
        if (sectionId != asmr.currentSection)
        {   // if jump outside current section (.text)
            asmr.printError(exprPlace, "Jump over current section!");
            return false;
        }
        return true;
    }
    else
    {
        outTargetExpr = std::move(expr);
        return true;
    }
}

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
        asmr.printError(linePtr, (std::string("Expected ")+objName).c_str());
        return false;
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
            return true; // succeed
        asmr.printError(linePtr, (std::string("Some garbages at ")+objName+
                        " place").c_str());
        if (!skipCommaAtError)
            while (linePtr != end && !isSpace(*linePtr) && *linePtr != ',') linePtr++;
        else
            while (linePtr != end && !isSpace(*linePtr)) linePtr++;
        return false;
    }
    outStr.assign(nameStr, linePtr);
    return true;
}

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
        asmr.printError(linePtr, (std::string("Expected ")+objName).c_str());
        return false;
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
        {   // return empty string
            outStr[0] = 0; // null-char
            return true;
        }
        asmr.printError(linePtr, (std::string(objName)+" is too long").c_str());
        return false;
    }
    const size_t outStrSize = std::min(maxOutStrSize-1, size_t(linePtr-nameStr));
    std::copy(nameStr, nameStr+outStrSize, outStr);
    outStr[outStrSize] = 0; // null-char
    return true;
}

bool AsmParseUtils::skipComma(Assembler& asmr, bool& haveComma, const char*& linePtr)
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
        asmr.printError(linePtr, "Expected ',' before argument");
        return false;
    }
    linePtr++;
    haveComma = true;
    return true;
}

bool AsmParseUtils::skipRequiredComma(Assembler& asmr, const char*& linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end || *linePtr != ',')
    {
        asmr.printError(linePtr, "Expected ',' before argument");
        return false;
    }
    linePtr++;
    return true;
}

bool AsmParseUtils::skipCommaForMultipleArgs(Assembler& asmr, const char*& linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end); // spaces before ','
    if (linePtr == end)
        return false;
    if (*linePtr != ',')
    {
        asmr.printError(linePtr, "Expected ',' before next value");
        linePtr++;
        return false;
    }
    else
        skipCharAndSpacesToEnd(linePtr, end);
    return true;
}

bool AsmParseUtils::getEnumeration(Assembler& asmr, const char*& linePtr,
              const char* objName, size_t tableSize,
              const std::pair<const char*, cxuint>* table, cxuint& value,
              const char* prefix)
{
    const char* end = asmr.line + asmr.lineSize;
    char name[72];
    skipSpacesToEnd(linePtr, end);
    const char* namePlace = linePtr;
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
        {
            asmr.printError(namePlace, (std::string("Unknown ")+objName).c_str());
            return false;
        }
        return true;
    }
    return false;
}

bool AsmParseUtils::parseDimensions(Assembler& asmr, const char*& linePtr, cxuint& dimMask)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* dimPlace = linePtr;
    char buf[10];
    dimMask = 0;
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
            {
                asmr.printError(dimPlace, "Unknown dimension type");
                return false;
            }
    }
    else // error
        return false;
    return true;
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
        {   // delete and move back iteration by number of removed elements
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
    hasValue = false;
    base = false;
    delete expression;
    expression = nullptr;
    onceDefined = false;
}

/*
 * Assembler
 */

Assembler::Assembler(const CString& filename, std::istream& input, Flags _flags,
        BinaryFormat _format, GPUDeviceType _deviceType, std::ostream& msgStream,
        std::ostream& _printStream)
        : format(_format),
          deviceType(_deviceType),
          driverVersion(0),
          _64bit(false),
          isaAssembler(nullptr),
          symbolMap({std::make_pair(".", AsmSymbol(0, uint64_t(0)))}),
          flags(_flags), 
          lineSize(0), line(nullptr),
          endOfAssembly(false),
          messageStream(msgStream),
          printStream(_printStream),
          // value reference and section reference from first symbol: '.'
          currentSection(symbolMap.begin()->second.sectionId),
          currentOutPos(symbolMap.begin()->second.value)
{
    filenameIndex = 0;
    alternateMacro = (flags & ASM_ALTMACRO)!=0;
    buggyFPLit = (flags & ASM_BUGGYFPLIT)!=0;
    localCount = macroCount = inclusionLevel = 0;
    macroSubstLevel = repetitionLevel = 0;
    lineAlreadyRead = false;
    good = true;
    resolvingRelocs = false;
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
          driverVersion(0),
          _64bit(false),
          isaAssembler(nullptr),
          symbolMap({std::make_pair(".", AsmSymbol(0, uint64_t(0)))}),
          flags(_flags), 
          lineSize(0), line(nullptr),
          endOfAssembly(false),
          messageStream(msgStream),
          printStream(_printStream),
          // value reference and section reference from first symbol: '.'
          currentSection(symbolMap.begin()->second.sectionId),
          currentOutPos(symbolMap.begin()->second.value)
{
    filenameIndex = 0;
    filenames = _filenames;
    alternateMacro = (flags & ASM_ALTMACRO)!=0;
    buggyFPLit = (flags & ASM_BUGGYFPLIT)!=0;
    localCount = macroCount = inclusionLevel = 0;
    macroSubstLevel = repetitionLevel = 0;
    lineAlreadyRead = false;
    good = true;
    resolvingRelocs = false;
    formatHandler = nullptr;
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
    for (auto& entry: symbolMap)
        entry.second.clearOccurrencesInExpr();
    /// remove expressions before symbol snapshots
    for (auto& entry: symbolSnapshots)
        entry->second.clearOccurrencesInExpr();
    
    for (auto& entry: symbolSnapshots)
        delete entry;
}

bool Assembler::parseString(std::string& strarray, const char*& linePtr)
{
    const char* end = line+lineSize;
    const char* startPlace = linePtr;
    skipSpacesToEnd(linePtr, end);
    strarray.clear();
    if (linePtr == end || *linePtr != '"')
    {
        while (linePtr != end && !isSpace(*linePtr) && *linePtr != ',') linePtr++;
        printError(startPlace, "Expected string");
        return false;
    }
    linePtr++;
    
    while (linePtr != end && *linePtr != '"')
    {
        if (*linePtr == '\\')
        {   // escape
            linePtr++;
            uint16_t value;
            if (linePtr == end)
            {
                printError(startPlace, "Unterminated character of string");
                return false;
            }
            if (*linePtr == 'x')
            {   // hex
                const char* charPlace = linePtr-1;
                linePtr++;
                if (linePtr == end)
                {
                    printError(startPlace, "Unterminated character of string");
                    return false;
                }
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
                {
                    printError(charPlace, "Expected hexadecimal character code");
                    return false;
                }
                value &= 0xff;
            }
            else if (isODigit(*linePtr))
            {   // octal
                value = 0;
                const char* charPlace = linePtr-1;
                for (cxuint i = 0; linePtr != end && i < 3; i++, linePtr++)
                {
                    if (!isODigit(*linePtr))
                        break;
                    value = (value<<3) + uint64_t(*linePtr-'0');
                    if (value > 255)
                    {
                        printError(charPlace, "Octal code out of range");
                        return false;
                    }
                }
            }
            else
            {   // normal escapes
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
    {
        printError(startPlace, "Unterminated string");
        return false;
    }
    linePtr++;
    return true;
}

bool Assembler::parseLiteral(uint64_t& value, const char*& linePtr)
{
    const char* startPlace = linePtr;
    const char* end = line+lineSize;
    if (linePtr != end && *linePtr == '\'')
    {
        linePtr++;
        if (linePtr == end)
        {
            printError(startPlace, "Unterminated character literal");
            return false;
        }
        if (*linePtr == '\'')
        {
            printError(startPlace, "Empty character literal");
            return false;
        }
        
        if (*linePtr != '\\')
        {
            value = *linePtr++;
            if (linePtr == end || *linePtr != '\'')
            {
                printError(startPlace, "Missing ''' at end of literal");
                return false;
            }
            linePtr++;
            return true;
        }
        else // escapes
        {
            linePtr++;
            if (linePtr == end)
            {
                printError(startPlace, "Unterminated character literal");
                return false;
            }
            if (*linePtr == 'x')
            {   // hex
                linePtr++;
                if (linePtr == end)
                {
                    printError(startPlace, "Unterminated character literal");
                    return false;
                }
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
                {
                    printError(startPlace, "Expected hexadecimal character code");
                    return false;
                }
                value &= 0xff;
            }
            else if (isODigit(*linePtr))
            {   // octal
                value = 0;
                for (cxuint i = 0; linePtr != end && i < 3 && *linePtr != '\'';
                     i++, linePtr++)
                {
                    if (!isODigit(*linePtr))
                    {
                        printError(startPlace, "Expected octal character code");
                        return false;
                    }
                    value = (value<<3) + uint64_t(*linePtr-'0');
                    if (value > 255)
                    {
                        printError(startPlace, "Octal code out of range");
                        return false;
                    }
                }
            }
            else
            {   // normal escapes
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
            {
                printError(startPlace, "Missing ''' at end of literal");
                return false;
            }
            linePtr++;
            return true;
        }
    }
    try
    { value = cstrtovCStyle<uint64_t>(startPlace, line+lineSize, linePtr); }
    catch(const ParseException& ex)
    {
        printError(startPlace, ex.what());
        return false;
    }
    return true;
}

Assembler::ParseState Assembler::parseSymbol(const char*& linePtr,
                AsmSymbolEntry*& entry, bool localLabel, bool dontCreateSymbol)
{
    const char* startPlace = linePtr;
    const CString symName = extractSymName(linePtr, line+lineSize, localLabel);
    if (symName.empty())
    {   // this is not symbol or a missing symbol
        while (linePtr != line+lineSize && !isSpace(*linePtr) && *linePtr != ',')
            linePtr++;
        entry = nullptr;
        return Assembler::ParseState::MISSING;
    }
    if (symName == ".") // any usage of '.' causes format initialization
        initializeOutputFormat();
    
    Assembler::ParseState state = Assembler::ParseState::PARSED;
    bool symHasValue;
    if (!dontCreateSymbol)
    {   // create symbol if not found
        std::pair<AsmSymbolMap::iterator, bool> res =
                symbolMap.insert(std::make_pair(symName, AsmSymbol()));
        entry = &*res.first;
        symHasValue = res.first->second.hasValue;
    }
    else
    {   // only find symbol and set isDefined and entry
        AsmSymbolMap::iterator it = symbolMap.find(symName);
        entry = (it != symbolMap.end()) ? &*it : nullptr;
        symHasValue = (it != symbolMap.end() && it->second.hasValue);
    }
    if (isDigit(symName.front()) && symName[linePtr-startPlace-1] == 'b' && !symHasValue)
    {   // failed at finding
        std::string error = "Undefined previous local label '";
        error.append(symName.begin(), linePtr-startPlace);
        error += "'";
        printError(linePtr, error.c_str());
        state = Assembler::ParseState::FAILED;
    }
    
    return state;
}

bool Assembler::parseMacroArgValue(const char*& string, std::string& outStr)
{
    const char* end = line+lineSize;
    bool firstNonSpace = false;
    cxbyte prevTok = 0;
    cxuint backslash = 0;
    
    if ((alternateMacro && string != end && *string=='%') ||
        (!alternateMacro && string+2 <= end && *string=='\\' && string[1]=='%'))
    {   // alternate syntax, parse expression evaluation
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
        {   /* if error */
            string = exprPlace;
            return false;
        }
    }
    
    if (alternateMacro && string != end && (*string=='<' || *string=='\'' || *string=='"'))
    {   /* alternate string quoting */
        const char termChar = (*string=='<') ? '>' : *string;
        string++;
        bool escape = false;
        while (string != end && (*string != termChar || escape))
        {
            if (!escape && *string=='!')
            {   /* skip this escaping */
                escape = true;
                string++;
            }
            else
            {   /* put character */
                escape = false;
                outStr.push_back(*string++);
            }
        }
        if (string == end)
        {   /* if unterminated string */
            printError(string, "Unterminated quoted string");
            return false;
        }
        string++;
        return true;
    }
    else if (string != end && *string=='"')
    {
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
        {
            printError(string, "Unterminated quoted string");
            return false;
        }
        string++;
        return true;
    }
    
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

bool Assembler::setSymbol(AsmSymbolEntry& symEntry, uint64_t value, cxuint sectionId)
{
    symEntry.second.value = value;
    symEntry.second.expression = nullptr;
    symEntry.second.sectionId = sectionId;
    symEntry.second.hasValue = isResolvableSection(sectionId) || resolvingRelocs;
    symEntry.second.regRange = false;
    symEntry.second.base = false;
    if (!symEntry.second.hasValue) // if not resolved we just return
        return true; // no error
    bool good = true;
    
    // resolve value of pending symbols
    std::stack<std::pair<AsmSymbolEntry*, size_t> > symbolStack;
    symbolStack.push(std::make_pair(&symEntry, 0));
    symEntry.second.resolving = true;
    
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
            {   // expresion has been fully resolved
                uint64_t value;
                cxuint sectionId;
                const AsmExprTarget& target = expr->getTarget();
                if (!resolvingRelocs || target.type==ASMXTGT_SYMBOL)
                {   // standard mode
                    if (!expr->evaluate(*this, value, sectionId))
                    {   // if failed
                        delete occurrence.expression; // delete expression
                        good = false;
                        continue;
                    }
                }
                // resolve expression if at resolving symbol phase
                else if (formatHandler==nullptr ||
                        !formatHandler->resolveRelocation(expr, value, sectionId))
                {   // if failed
                    delete occurrence.expression; // delete expression
                    good = false;
                    continue;
                }
                
                switch(target.type)
                {
                    case ASMXTGT_SYMBOL:
                    {    // resolve symbol
                        AsmSymbolEntry& curSymEntry = *target.symbol;
                        if (!curSymEntry.second.resolving &&
                            curSymEntry.second.expression==expr)
                        {
                            curSymEntry.second.value = value;
                            curSymEntry.second.sectionId = sectionId;
                            curSymEntry.second.hasValue =
                                isResolvableSection(sectionId) || resolvingRelocs;
                            symbolStack.push(std::make_pair(&curSymEntry, 0));
                            if (!curSymEntry.second.hasValue)
                                continue;
                            curSymEntry.second.resolving = true;
                        }
                        // otherwise we ignore circular dependencies
                        break;
                    }
                    case ASMXTGT_DATA8:
                        if (sectionId != ASMSECT_ABS)
                        {
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                            good = false;
                        }
                        else
                        {
                            printWarningForRange(8, value, expr->getSourcePos());
                            sections[target.sectionId].content[target.offset] =
                                    cxbyte(value);
                        }
                        break;
                    case ASMXTGT_DATA16:
                        if (sectionId != ASMSECT_ABS)
                        {
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                            good = false;
                        }
                        else
                        {
                            printWarningForRange(16, value, expr->getSourcePos());
                            SULEV(*reinterpret_cast<uint16_t*>(sections[target.sectionId]
                                    .content.data() + target.offset), uint16_t(value));
                        }
                        break;
                    case ASMXTGT_DATA32:
                        if (sectionId != ASMSECT_ABS)
                        {
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                            good = false;
                        }
                        else
                        {
                            printWarningForRange(32, value, expr->getSourcePos());
                            SULEV(*reinterpret_cast<uint32_t*>(sections[target.sectionId]
                                    .content.data() + target.offset), uint32_t(value));
                        }
                        break;
                    case ASMXTGT_DATA64:
                        if (sectionId != ASMSECT_ABS)
                        {
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                            good = false;
                        }
                        else
                            SULEV(*reinterpret_cast<uint64_t*>(sections[target.sectionId]
                                    .content.data() + target.offset), uint64_t(value));
                        break;
                    default: // ISA assembler resolves this dependency
                        if (!isaAssembler->resolveCode(expr->getSourcePos(),
                                target.sectionId, sections[target.sectionId].content.data(),
                                target.offset, target.type, sectionId, value))
                            good = false;
                        break;
                }
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
    if (linePtr!=line+lineSize && *linePtr=='%')
    {
        if (symbolName == ".")
        {
            printError(symbolPlace, "Symbol '.' requires a resolved expression");
            return false;
        }
        initializeOutputFormat();
        ++linePtr;
        cxuint regStart, regEnd;
        const AsmRegVar* regVar;
        if (!isaAssembler->parseRegisterRange(linePtr, regStart, regEnd, regVar))
            return false;
        skipSpacesToEnd(linePtr, line+lineSize);
        if (linePtr != line+lineSize)
        {
            printError(linePtr, "Garbages at end of expression");
            return false;
        }
        
        std::pair<AsmSymbolMap::iterator, bool> res =
                symbolMap.insert(std::make_pair(symbolName, AsmSymbol()));
        if (!res.second && ((res.first->second.onceDefined || !reassign) &&
            res.first->second.isDefined()))
        {   // found and can be only once defined
            printError(symbolPlace, (std::string("Symbol '") + symbolName.c_str() +
                        "' is already defined").c_str());
            return false;
        }
        if (!res.first->second.occurrencesInExprs.empty())
        {   // found in expressions
            std::vector<std::pair<const AsmExpression*, size_t> > exprs;
            size_t i = 0;
            for (AsmExprSymbolOccurrence occur: res.first->second.occurrencesInExprs)
                exprs.push_back(std::make_pair(occur.expression, i++));
            // remove duplicates
            std::sort(exprs.begin(), exprs.end(),
                      [](const std::pair<const AsmExpression*, size_t>& a,
                         const std::pair<const AsmExpression*, size_t>& b)
                    {
                        if (a.first==b.first)
                            return a.second<b.second;
                        return a.first<b.first;
                    });
            exprs.resize(std::unique(exprs.begin(), exprs.end(),
                       [](const std::pair<const AsmExpression*, size_t>& a,
                         const std::pair<const AsmExpression*, size_t>& b)
                       { return a.first==b.first; })-exprs.begin());
            
            std::sort(exprs.begin(), exprs.end(),
                      [](const std::pair<const AsmExpression*, size_t>& a,
                         const std::pair<const AsmExpression*, size_t>& b)
                    { return a.second<b.second; });
            // print errors
            for (std::pair<const AsmExpression*, size_t> elem: exprs)
                printError(elem.first->getSourcePos(), "Expression have register symbol");
            
            printError(symbolPlace, (std::string("Register range symbol '") +
                            symbolName.c_str() + "' was used in some expressions").c_str());
            return false;
        }
        // setup symbol entry (required)
        AsmSymbolEntry& symEntry = *res.first;
        symEntry.second.expression = nullptr;
        symEntry.second.onceDefined = !reassign;
        symEntry.second.base = false;
        symEntry.second.sectionId = ASMSECT_ABS;
        symEntry.second.regRange = symEntry.second.hasValue = true;
        symEntry.second.value = (regStart | (uint64_t(regEnd)<<32));
        return true;
    }
    
    const char* exprPlace = linePtr;
    // make base expr if baseExpr=true and symbolName is not output counter
    bool makeBaseExpr = (baseExpr && symbolName != ".");
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(*this, linePtr, makeBaseExpr));
    if (!expr) // no expression, errors
        return false;
    
    if (linePtr != line+lineSize)
    {
        printError(linePtr, "Garbages at end of expression");
        return false;
    }
    if (expr->isEmpty()) // empty expression, we treat as error
    {
        printError(exprPlace, "Expected assignment expression");
        return false;
    }
    
    if (symbolName == ".")
    {   // assigning '.'
        uint64_t value;
        cxuint sectionId;
        if (!expr->evaluate(*this, value, sectionId))
            return false;
        if (expr->getSymOccursNum()==0)
            return assignOutputCounter(symbolPlace, value, sectionId);
        else
        {
            printError(symbolPlace, "Symbol '.' requires a resolved expression");
            return false;
        }
    }
    
    std::pair<AsmSymbolMap::iterator, bool> res =
            symbolMap.insert(std::make_pair(symbolName, AsmSymbol()));
    if (!res.second && ((res.first->second.onceDefined || !reassign) &&
        res.first->second.isDefined()))
    {   // found and can be only once defined
        printError(symbolPlace, (std::string("Symbol '") + symbolName.c_str() +
                    "' is already defined").c_str());
        return false;
    }
    AsmSymbolEntry& symEntry = *res.first;
    
    if (expr->getSymOccursNum()==0)
    {   // can evalute, assign now
        uint64_t value;
        cxuint sectionId;
        if (!expr->evaluate(*this, value, sectionId))
            return false;
        
        setSymbol(symEntry, value, sectionId);
        symEntry.second.onceDefined = !reassign;
    }
    else // set expression
    {
        expr->setTarget(AsmExprTarget::symbolTarget(&symEntry));
        symEntry.second.expression = expr.release();
        symEntry.second.regRange = symEntry.second.hasValue = false;
        symEntry.second.onceDefined = !reassign;
        symEntry.second.base = baseExpr;
        if (baseExpr && !symEntry.second.occurrencesInExprs.empty())
        {   /* make snapshot now resolving dependencies */
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
            cxuint sectionId, cxbyte fillValue)
{
    initializeOutputFormat();
    if (currentSection != sectionId && sectionId != ASMSECT_ABS)
    {
        printError(symbolPlace, "Illegal section change for symbol '.'");
        return false;
    }
    if (currentSection != ASMSECT_ABS && int64_t(currentOutPos) > int64_t(value))
    {   /* check attempt to move backwards only for section that is not absolute */
        printError(symbolPlace, "Attempt to move backwards");
        return false;
    }
    if (!isAddressableSection())
    {
        printError(symbolPlace,
                   "Change output counter inside non-addressable section is illegal");
        return false;
    }
    if (currentSection==ASMSECT_ABS && fillValue!=0)
        printWarning(symbolPlace, "Fill value is ignored inside absolute section");
    if (value-currentOutPos!=0)
        reserveData(value-currentOutPos, fillValue);
    currentOutPos = value;
    return true;
}

bool Assembler::skipSymbol(const char*& linePtr)
{
    const char* end = line+lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* start = linePtr;
    if (linePtr != end)
    {   /* skip only symbol name */
        if(isAlpha(*linePtr) || *linePtr == '_' || *linePtr == '.' || *linePtr == '$')
            for (linePtr++; linePtr != end && (isAlnum(*linePtr) || *linePtr == '_' ||
                 *linePtr == '.' || *linePtr == '$') ; linePtr++);
    }
    if (start == linePtr)
    {   // this is not symbol name
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

void Assembler::printWarningForRange(cxuint bits, uint64_t value, const AsmSourcePos& pos,
        cxbyte signess)
{
    if (bits < 64)
    {   /* signess - WS_BOTH - check value range as signed value 
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

bool Assembler::checkReservedName(const CString& name)
{
    return false;
}

void Assembler::addIncludeDir(const CString& includeDir)
{
    includeDirs.push_back(includeDir);
}

void Assembler::addInitialDefSym(const CString& symName, uint64_t value)
{
    defSyms.push_back({symName, value});
}

bool Assembler::pushClause(const char* string, AsmClauseType clauseType, bool satisfied,
               bool& included)
{
    if (clauseType == AsmClauseType::MACRO || clauseType == AsmClauseType::IF ||
        clauseType == AsmClauseType::REPEAT)
    {   // add new clause
        clauses.push({ clauseType, getSourcePos(string), satisfied, { } });
        included = satisfied;
        return true;
    }
    if (clauses.empty())
    {   // no clauses
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
            if (clauseType == AsmClauseType::ELSEIF)
                printError(string,
                       "No '.if' before '.elseif' inside macro");
            else // else
                printError(string,
                       "No '.if' before '.else' inside macro");
            return false;
        case AsmClauseType::REPEAT:
            if (clauseType == AsmClauseType::ELSEIF)
                printError(string,
                       "No '.if' before '.elseif' inside repetition");
            else // else
                printError(string,
                       "No '.if' before '.else' inside repetition");
            return false;
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

bool Assembler::popClause(const char* string, AsmClauseType clauseType)
{
    if (clauses.empty())
    {   // no clauses
        if (clauseType == AsmClauseType::IF)
            printError(string, "No conditional before '.endif'");
        else if (clauseType == AsmClauseType::MACRO) // macro
            printError(string, "No '.macro' before '.endm'");
        else if (clauseType == AsmClauseType::REPEAT) // repeat
            printError(string, "No '.rept' before '.endr'");
        return false;
    }
    AsmClause& clause = clauses.top();
    switch(clause.type)
    {
        case AsmClauseType::IF:
        case AsmClauseType::ELSE:
        case AsmClauseType::ELSEIF:
            if (clauseType == AsmClauseType::MACRO)
            {
                printError(string, "Ending macro across conditionals");
                return false;
            }
            else if (clauseType == AsmClauseType::REPEAT)
            {
                printError(string, "Ending repetition across conditionals");
                return false;
            }
            break;
        case AsmClauseType::MACRO:
            if (clauseType == AsmClauseType::REPEAT)
            {
                printError(string, "Ending repetition across macro");
                return false;
            }
            if (clauseType == AsmClauseType::IF)
            {
                printError(string, "Ending conditional across macro");
                return false;
            }
            break;
        case AsmClauseType::REPEAT:
            if (clauseType == AsmClauseType::MACRO)
            {
                printError(string, "Ending macro across repetition");
                return false;
            }
            if (clauseType == AsmClauseType::IF)
            {
                printError(string, "Ending conditional across repetition");
                return false;
            }
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
    toLowerString(macroName);
    MacroMap::const_iterator it = macroMap.find(macroName);
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
            if (!parseMacroArgValue(linePtr, macroArg))
            {
                good = false;
                continue;
            }
        }
        else
        {   /* parse variadic arguments, they requires ',' separator */
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
        {   // error, value required
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

bool Assembler::includeFile(const char* pseudoOpPlace, const std::string& filename)
{
    if (inclusionLevel == 500)
    {
        printError(pseudoOpPlace, "Inclusion level is greater than 500");
        return false;
    }
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
    {   // no line
        if (asmInputFilters.size() > 1)
        {   /* decrease some level of a nesting */
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
        {   /* handling input assembler that have many files */
            do { // delete previous filter
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

cxbyte* Assembler::reserveData(size_t size, cxbyte fillValue)
{
    if (currentSection != ASMSECT_ABS)
    {
        size_t oldOutPos = currentOutPos;
        AsmSection& section = sections[currentSection];
        if ((section.flags & ASMSECT_WRITEABLE) == 0) // non writeable
        {
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
    {   // not found, add new kernel
        cxuint kernelId;
        try
        { kernelId = formatHandler->addKernel(kernelName); }
        catch(const AsmFormatException& ex)
        {   // error!
            printError(pseudoOpPlace, ex.what());
            return;
        }
        // add new kernel entries and section entry
        auto it = kernelMap.insert(std::make_pair(kernelName, kernelId)).first;
        kernels.push_back({ it->first.c_str(),  getSourcePos(pseudoOpPlace) });
        auto info = formatHandler->getSectionInfo(currentSection);
        sections.push_back({ info.name, currentKernel, info.type, info.flags, 0 });
        currentOutPos = 0;
    }
    else
    {   // found
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
    const cxuint sectionId = formatHandler->getSectionId(sectionName);
    if (sectionId == ASMSECT_NONE)
    {   // try to add new section
        cxuint sectionId;
        try
        { sectionId = formatHandler->addSection(sectionName, currentKernel); }
        catch(const AsmFormatException& ex)
        {   // error!
            printError(pseudoOpPlace, ex.what());
            return;
        }
        auto info = formatHandler->getSectionInfo(sectionId);
        sections.push_back({ info.name, currentKernel, info.type, info.flags, align });
        currentOutPos = 0;
    }
    else // if section exists
    {   // found, try to set
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

void Assembler::goToSection(const char* pseudoOpPlace, const char* sectionName,
        AsmSectionType type, Flags flags, uint64_t align)
{
    const cxuint sectionId = formatHandler->getSectionId(sectionName);
    if (sectionId == ASMSECT_NONE)
    {   // try to add new section
        cxuint sectionId;
        try
        { sectionId = formatHandler->addSection(sectionName, currentKernel); }
        catch(const AsmFormatException& ex)
        {   // error!
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
        sections.push_back({ info.name, currentKernel, info.type, info.flags, align });
        currentOutPos = 0;
    }
    else // if section exists
    {   // found, try to set
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

void Assembler::goToSection(const char* pseudoOpPlace, cxuint sectionId, uint64_t align)
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
    sections.push_back({ info.name, currentKernel, info.type, info.flags, 0 });
    currentOutPos = 0;
}

bool Assembler::assemble()
{
    resolvingRelocs = false;
    
    for (const DefSym& defSym: defSyms)
        if (defSym.first!=".")
            symbolMap[defSym.first] = AsmSymbol(ASMSECT_ABS, defSym.second);
        else if ((flags & ASM_WARNINGS) != 0)// ignore for '.'
            messageStream << "<command-line>: Warning: Definition for symbol '.' "
                    "was ignored" << std::endl;
    
    good = true;
    while (!endOfAssembly)
    {
        if (!lineAlreadyRead)
        {   // read line
            if (!readLine())
                break;
        }
        else
        {   // already line is read
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
        while (!firstName.empty() && linePtr != end && *linePtr == ':')
        {   // labels
            linePtr++;
            skipSpacesToEnd(linePtr, end);
            initializeOutputFormat();
            if (firstName.front() >= '0' && firstName.front() <= '9')
            {   // handle local labels
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
                std::pair<AsmSymbolMap::iterator, bool> prevLRes =
                        symbolMap.insert(std::make_pair(
                            std::string(firstName.c_str())+"b", AsmSymbol()));
                std::pair<AsmSymbolMap::iterator, bool> nextLRes =
                        symbolMap.insert(std::make_pair(
                            std::string(firstName.c_str())+"f", AsmSymbol()));
                /* resolve forward symbol of label now */
                assert(setSymbol(*nextLRes.first, currentOutPos, currentSection));
                // move symbol value from next local label into previous local label
                // clearOccurrences - obsolete - back local labels are undefined!
                prevLRes.first->second.value = nextLRes.first->second.value;
                prevLRes.first->second.hasValue = isResolvableSection();
                prevLRes.first->second.sectionId = currentSection;
                /// make forward symbol of label as undefined
                nextLRes.first->second.hasValue = false;
            }
            else
            {   // regular labels
                std::pair<AsmSymbolMap::iterator, bool> res = 
                        symbolMap.insert(std::make_pair(firstName, AsmSymbol()));
                if (!res.second)
                {   // found
                    if (res.first->second.onceDefined && res.first->second.isDefined())
                    {   // if label
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
        {   // assignment
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
        
        if (firstName.size() >= 2 && firstName[0] == '.') // check for pseudo-op
            parsePseudoOps(firstName, stmtPlace, linePtr);
        else if (firstName.size() >= 1 && isDigit(firstName[0]))
            printError(stmtPlace, "Illegal number at statement begin");
        else
        {   // try to parse processor instruction or macro substitution
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
                isaAssembler->assemble(firstName, stmtPlace, linePtr, end,
                           sections[currentSection].content);
                currentOutPos = sections[currentSection].getSize();
            }
        }
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
    
    resolvingRelocs = true;
    for (AsmSymbolEntry& symEntry: symbolMap)
        if (!symEntry.second.occurrencesInExprs.empty() || 
            (symEntry.first!="."  && !isResolvableSection(symEntry.second.sectionId)))
        {   // try to resolve symbols
            uint64_t value;
            cxuint sectionId;
            if (formatHandler!=nullptr &&
                formatHandler->resolveSymbol(symEntry.second, value, sectionId))
                setSymbol(symEntry, value, sectionId);
        }
    
    if ((flags&ASM_TESTRUN) == 0)
        for (AsmSymbolEntry& symEntry: symbolMap)
            if (!symEntry.second.occurrencesInExprs.empty())
                for (AsmExprSymbolOccurrence occur: symEntry.second.occurrencesInExprs)
                    printError(occur.expression->getSourcePos(),(std::string(
                        "Unresolved symbol '")+symEntry.first.c_str()+"'").c_str());
    
    if (good && formatHandler!=nullptr)
        formatHandler->prepareBinary();
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
                throw Exception(std::string("Can't open output file '")+filename+"'");
        }
        else
            throw Exception("No output binary");
    }
    else // failed
        throw Exception("Assembler failed!");
}

void Assembler::writeBinary(std::ostream& outStream) const
{
    if (good)
    {
        const AsmFormatHandler* formatHandler = getFormatHandler();
        if (formatHandler!=nullptr)
            formatHandler->writeBinary(outStream);
        else
            throw Exception("No output binary");
    }
    else // failed
        throw Exception("Assembler failed!");
}

void Assembler::writeBinary(Array<cxbyte>& array) const
{
    if (good)
    {
        const AsmFormatHandler* formatHandler = getFormatHandler();
        if (formatHandler!=nullptr)
            formatHandler->writeBinary(array);
        else
            throw Exception("No output binary");
    }
    else // failed
        throw Exception("Assembler failed!");
}
