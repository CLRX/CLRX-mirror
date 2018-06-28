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
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <memory>
#include <unordered_set>
#include <CLRX/amdbin/ElfBinaries.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/ROCmBinaries.h>

using namespace CLRX;

/*
 * ROCm metadata YAML parser
 */

void ROCmKernelMetadata::initialize()
{
    langVersion[0] = langVersion[1] = BINGEN_NOTSUPPLIED;
    reqdWorkGroupSize[0] = reqdWorkGroupSize[1] = reqdWorkGroupSize[2] = 0;
    workGroupSizeHint[0] = workGroupSizeHint[1] = workGroupSizeHint[2] = 0;
    kernargSegmentSize = BINGEN64_NOTSUPPLIED;
    groupSegmentFixedSize = BINGEN64_NOTSUPPLIED;
    privateSegmentFixedSize = BINGEN64_NOTSUPPLIED;
    kernargSegmentAlign = BINGEN64_NOTSUPPLIED;
    wavefrontSize = BINGEN_NOTSUPPLIED;
    sgprsNum = BINGEN_NOTSUPPLIED;
    vgprsNum = BINGEN_NOTSUPPLIED;
    maxFlatWorkGroupSize = BINGEN64_NOTSUPPLIED;
    fixedWorkGroupSize[0] = fixedWorkGroupSize[1] = fixedWorkGroupSize[2] = 0;
    spilledSgprs = BINGEN_NOTSUPPLIED;
    spilledVgprs = BINGEN_NOTSUPPLIED;
}

void ROCmMetadata::initialize()
{
    version[0] = 1;
    version[1] = 0;
}

// return trailing spaces
static size_t skipSpacesAndComments(const char*& ptr, const char* end, size_t& lineNo)
{
    const char* lineStart = ptr;
    while (ptr != end)
    {
        lineStart = ptr;
        while (ptr != end && *ptr!='\n' && isSpace(*ptr)) ptr++;
        if (ptr == end)
            break; // end of stream
        if (*ptr=='#')
        {
            // skip comment
            while (ptr != end && *ptr!='\n') ptr++;
            if (ptr == end)
                return 0; // no trailing spaces and end
        }
        else if (*ptr!='\n')
            break; // no comment and no end of line
        else
        {
            ptr++;
            lineNo++; // next line
        }
    }
    return ptr - lineStart;
}

static inline void skipSpacesToLineEnd(const char*& ptr, const char* end)
{
    while (ptr != end && *ptr!='\n' && isSpace(*ptr)) ptr++;
}

static void skipSpacesToNextLine(const char*& ptr, const char* end, size_t& lineNo)
{
    skipSpacesToLineEnd(ptr, end);
    if (ptr != end && *ptr != '\n' && *ptr!='#')
        throw ParseException(lineNo, "Garbages at line");
    if (ptr != end && *ptr == '#')
        // skip comment at end of line
        while (ptr!=end && *ptr!='\n') ptr++;
    if (ptr!=end)
    {   // newline
        ptr++;
        lineNo++;
    }
}

enum class YAMLValType
{
    NONE,
    NIL,
    BOOL,
    INT,
    FLOAT,
    STRING,
    SEQ
};

static YAMLValType parseYAMLType(const char*& ptr, const char* end, size_t lineNo)
{
    if (ptr+2>end || *ptr!='!' || ptr[1]!='!')
        return YAMLValType::NONE; // no type
    if (ptr+7 && ::strncmp(ptr+2, "null", 4)==0 && isSpace(ptr[6]) && ptr[6]!='\n')
    {
        ptr += 6;
        return YAMLValType::NIL;
    }
    else if (ptr+7 && ::strncmp(ptr+2, "bool", 4)==0 && isSpace(ptr[6]) && ptr[6]!='\n')
    {
        ptr += 6;
        return YAMLValType::BOOL;
    }
    else if (ptr+6 && ::strncmp(ptr+2, "int", 3)==0 && isSpace(ptr[5]) && ptr[5]!='\n')
    {
        ptr += 5;
        return YAMLValType::INT;
    }
    else if (ptr+8 && ::strncmp(ptr+2, "float", 5)==0 && isSpace(ptr[7]) && ptr[7]!='\n')
    {
        ptr += 7;
        return YAMLValType::FLOAT;
    }
    else if (ptr+6 && ::strncmp(ptr+2, "str", 3)==0 && isSpace(ptr[5]) && ptr[5]!='\n')
    {
        ptr += 5;
        return YAMLValType::STRING;
    }
    else if (ptr+6 && ::strncmp(ptr+2, "seq", 3)==0 && isSpace(ptr[5]) && ptr[5]!='\n')
    {
        ptr += 5;
        return YAMLValType::SEQ;
    }
    throw ParseException(lineNo, "Unknown YAML value type");
}

// parse YAML key (keywords - recognized keys)
static size_t parseYAMLKey(const char*& ptr, const char* end, size_t lineNo,
            size_t keywordsNum, const char** keywords)
{
    const char* keyPtr = ptr;
    while (ptr != end && (isAlnum(*ptr) || *ptr=='_')) ptr++;
    if (keyPtr == end)
        throw ParseException(lineNo, "Expected key name");
    const char* keyEnd = ptr;
    skipSpacesToLineEnd(ptr, end);
    if (ptr == end || *ptr!=':')
        throw ParseException(lineNo, "Expected colon");
    ptr++;
    const char* afterColon = ptr;
    skipSpacesToLineEnd(ptr, end);
    if (afterColon == ptr && ptr != end && *ptr!='\n')
        // only if not immediate newline
        throw ParseException(lineNo, "After key and colon must be space");
    CString keyword(keyPtr, keyEnd);
    const size_t index = binaryFind(keywords, keywords+keywordsNum,
                        keyword.c_str(), CStringLess()) - keywords;
    return index;
}

// parse YAML integer value
template<typename T>
static T parseYAMLIntValue(const char*& ptr, const char* end, size_t& lineNo,
                bool singleValue = false)
{
    skipSpacesToLineEnd(ptr, end);
    if (ptr == end || *ptr=='\n')
        throw ParseException(lineNo, "Expected integer value");
    
    // skip !!int
    YAMLValType valType = parseYAMLType(ptr, end, lineNo);
    if (valType == YAMLValType::INT)
    {   // if 
        skipSpacesToLineEnd(ptr, end);
        if (ptr == end || *ptr=='\n')
            throw ParseException(lineNo, "Expected integer value");
    }
    else if (valType != YAMLValType::NONE)
        throw ParseException(lineNo, "Expected value of integer type");
    
    T value = 0;
    try
    { value = cstrtovCStyle<T>(ptr, end, ptr); }
    catch(const ParseException& ex)
    { throw ParseException(lineNo, ex.what()); }
    
    if (singleValue)
        skipSpacesToNextLine(ptr, end, lineNo);
    return value;
}

// parse YAML boolean value
static bool parseYAMLBoolValue(const char*& ptr, const char* end, size_t& lineNo,
        bool singleValue = false)
{
    skipSpacesToLineEnd(ptr, end);
    if (ptr == end || *ptr=='\n')
        throw ParseException(lineNo, "Expected boolean value");
    
    // skip !!bool
    YAMLValType valType = parseYAMLType(ptr, end, lineNo);
    if (valType == YAMLValType::BOOL)
    {   // if 
        skipSpacesToLineEnd(ptr, end);
        if (ptr == end || *ptr=='\n')
            throw ParseException(lineNo, "Expected boolean value");
    }
    else if (valType != YAMLValType::NONE)
        throw ParseException(lineNo, "Expected value of boolean type");
    
    const char* wordPtr = ptr;
    while(ptr != end && isAlnum(*ptr)) ptr++;
    CString word(wordPtr, ptr);
    
    bool value = false;
    bool isSet = false;
    for (const char* v: { "1", "true", "t", "on", "yes", "y"})
        if (::strcasecmp(word.c_str(), v) == 0)
        {
            isSet = true;
            value = true;
            break;
        }
    if (!isSet)
        for (const char* v: { "0", "false", "f", "off", "no", "n"})
            if (::strcasecmp(word.c_str(), v) == 0)
            {
                isSet = true;
                value = false;
                break;
            }
    if (!isSet)
        throw ParseException(lineNo, "This is not boolean value");
    
    if (singleValue)
        skipSpacesToNextLine(ptr, end, lineNo);
    return value;
}

// trim spaces (remove spaces from start and end)
static std::string trimStrSpaces(const std::string& str)
{
    size_t i = 0;
    const size_t sz = str.size();
    while (i!=sz && isSpace(str[i])) i++;
    if (i == sz) return "";
    size_t j = sz-1;
    while (j>i && isSpace(str[j])) j--;
    return str.substr(i, j-i+1);
}

static std::string parseYAMLString(const char*& linePtr, const char* end,
            size_t& lineNo)
{
    std::string strarray;
    if (linePtr == end || (*linePtr != '"' && *linePtr != '\''))
    {
        while (linePtr != end && !isSpace(*linePtr) && *linePtr != ',') linePtr++;
        throw ParseException(lineNo, "Expected string");
    }
    const char termChar = *linePtr;
    linePtr++;
    
    // main loop, where is character parsing
    while (linePtr != end && *linePtr != termChar)
    {
        if (*linePtr == '\\')
        {
            // escape
            linePtr++;
            uint16_t value;
            if (linePtr == end)
                throw ParseException(lineNo, "Unterminated character of string");
            if (*linePtr == 'x')
            {
                // hex literal
                linePtr++;
                if (linePtr == end)
                    throw ParseException(lineNo, "Unterminated character of string");
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
                    throw ParseException(lineNo, "Expected hexadecimal character code");
                value &= 0xff;
            }
            else if (isODigit(*linePtr))
            {
                // octal literal
                value = 0;
                for (cxuint i = 0; linePtr != end && i < 3; i++, linePtr++)
                {
                    if (!isODigit(*linePtr))
                        break;
                    value = (value<<3) + uint64_t(*linePtr-'0');
                    // checking range
                    if (value > 255)
                        throw ParseException(lineNo, "Octal code out of range");
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
        {
            if (*linePtr=='\n')
                lineNo++;
            strarray.push_back(*linePtr++);
        }
    }
    if (linePtr == end)
        throw ParseException(lineNo, "Unterminated string");
    linePtr++;
    return strarray;
}

static std::string parseYAMLStringValue(const char*& ptr, const char* end, size_t& lineNo,
                    cxuint prevIndent, bool singleValue = false, bool blockAccept = true)
{
    skipSpacesToLineEnd(ptr, end);
    if (ptr == end)
        return "";
    
    // skip !!str
    YAMLValType valType = parseYAMLType(ptr, end, lineNo);
    if (valType == YAMLValType::STRING)
    {   // if 
        skipSpacesToLineEnd(ptr, end);
        if (ptr == end)
            return "";
    }
    else if (valType != YAMLValType::NONE)
        throw ParseException(lineNo, "Expected value of string type");
    
    std::string buf;
    if (*ptr=='"' || *ptr== '\'')
        buf = parseYAMLString(ptr, end, lineNo);
    // otherwise parse stream
    else if (*ptr == '|' || *ptr == '>')
    {
        if (!blockAccept)
            throw ParseException(lineNo, "Illegal block string start");
        // multiline
        bool newLineFold = *ptr=='>';
        ptr++;
        skipSpacesToLineEnd(ptr, end);
        if (ptr!=end && *ptr!='\n')
            throw ParseException(lineNo, "Garbages at string block");
        if (ptr == end)
            return ""; // end
        lineNo++;
        ptr++; // skip newline
        const char* lineStart = ptr;
        skipSpacesToLineEnd(ptr, end);
        size_t indent = ptr - lineStart;
        if (indent <= prevIndent)
            throw ParseException(lineNo, "Unindented string block");
        
        std::string buf;
        while(ptr != end)
        {
            const char* strStart = ptr;
            while (ptr != end && *ptr!='\n') ptr++;
            buf.append(strStart, ptr);
            
            if (ptr != end) // if new line
            {
                lineNo++;
                ptr++;
            }
            else // end of stream
                break;
            
            const char* lineStart = ptr;
            skipSpacesToLineEnd(ptr, end);
            bool emptyLines = false;
            while (size_t(ptr - lineStart) <= indent)
            {
                if (ptr != end && *ptr=='\n')
                {
                    // empty line
                    buf.append("\n");
                    ptr++;
                    lineNo++;
                    lineStart = ptr;
                    skipSpacesToLineEnd(ptr, end);
                    emptyLines = true;
                    continue;
                }
                // if smaller indent
                if (size_t(ptr - lineStart) < indent)
                {
                    buf.append("\n"); // always add newline at last line
                    if (ptr != end)
                        ptr = lineStart;
                    return buf;
                }
                else // if this same and not end of line
                    break;
            }
            
            if (!emptyLines || !newLineFold)
                // add missing newline after line with text
                // only if no emptyLines or no newLineFold
                buf.append(newLineFold ? " " : "\n");
            // to indent
            ptr = lineStart + indent;
        }
        return buf;
    }
    else
    {
        // single line string (unquoted)
        const char* strStart = ptr;
        // automatically trim spaces at ends
        const char* strEnd = ptr;
        while (ptr != end && *ptr!='\n' && *ptr!='#')
        {
            if (!isSpace(*ptr))
                strEnd = ptr; // to trim at end
            ptr++;
        }
        if (strEnd != end && !isSpace(*strEnd))
            strEnd++;
        
        buf.assign(strStart, strEnd);
    }
    
    if (singleValue)
        skipSpacesToNextLine(ptr, end, lineNo);
    return buf;
}

/// element consumer class
class CLRX_INTERNAL YAMLElemConsumer
{
public:
    virtual void consume(const char*& ptr, const char* end, size_t& lineNo,
                cxuint prevIndent, bool singleValue, bool blockAccept) = 0;
};

static void parseYAMLValArray(const char*& ptr, const char* end, size_t& lineNo,
            size_t prevIndent, YAMLElemConsumer* elemConsumer, bool singleValue = false)
{
    skipSpacesToLineEnd(ptr, end);
    if (ptr == end)
        return;
    
    // skip !!int
    YAMLValType valType = parseYAMLType(ptr, end, lineNo);
    if (valType == YAMLValType::SEQ)
    {   // if 
        skipSpacesToLineEnd(ptr, end);
        if (ptr == end)
            return;
    }
    else if (valType != YAMLValType::NONE)
        throw ParseException(lineNo, "Expected value of sequence type");
    
    if (*ptr == '[')
    {
        // parse array []
        ptr++;
        skipSpacesAndComments(ptr, end, lineNo);
        while (ptr != end)
        {
            // parse in line
            elemConsumer->consume(ptr, end, lineNo, 0, false, false);
            skipSpacesAndComments(ptr, end, lineNo);
            if (ptr!=end && *ptr==']')
                // just end
                break;
            else if (ptr==end || *ptr!=',')
                throw ParseException(lineNo, "Expected ','");
            ptr++;
            skipSpacesAndComments(ptr, end, lineNo);
        }
        if (ptr == end)
            throw ParseException(lineNo, "Unterminated array");
        ptr++;
        
        if (singleValue)
            skipSpacesToNextLine(ptr, end, lineNo);
        return;
    }
    // parse sequence
    size_t oldLineNo = lineNo;
    size_t indent0 = skipSpacesAndComments(ptr, end, lineNo);
    if (ptr == end || lineNo == oldLineNo)
        throw ParseException(lineNo, "Expected sequence of values");
    
    if (indent0 < prevIndent)
        throw ParseException(lineNo, "Unindented sequence of objects");
    
    // main loop to parse sequence
    while (ptr != end)
    {
        if (*ptr != '-')
            throw ParseException(lineNo, "No '-' before element value");
        ptr++;
        const char* afterMinus = ptr;
        skipSpacesToLineEnd(ptr, end);
        if (afterMinus == ptr)
            throw ParseException(lineNo, "No spaces after '-'");
        elemConsumer->consume(ptr, end, lineNo, indent0, true, true);
        
        size_t indent = skipSpacesAndComments(ptr, end, lineNo);
        if (indent < indent0)
        {
            // if parent level
            ptr -= indent;
            break;
        }
        if (indent != indent0)
            throw ParseException(lineNo, "Wrong indentation of element");
    }
}

// integer element consumer
template<typename T>
class CLRX_INTERNAL YAMLIntArrayConsumer: public YAMLElemConsumer
{
private:
    size_t elemsNum;
    size_t requiredElemsNum;
public:
    T* array;
    
    YAMLIntArrayConsumer(size_t reqElemsNum, T* _array)
            : elemsNum(0), requiredElemsNum(reqElemsNum), array(_array)
    { }
    
    virtual void consume(const char*& ptr, const char* end, size_t& lineNo,
                cxuint prevIndent, bool singleValue, bool blockAccept)
    {
        if (elemsNum == requiredElemsNum)
            throw ParseException(lineNo, "Too many elements");
        try
        { array[elemsNum] = cstrtovCStyle<T>(ptr, end, ptr); }
        catch(const ParseException& ex)
        { throw ParseException(lineNo, ex.what()); }
        elemsNum++;
        if (singleValue)
            skipSpacesToNextLine(ptr, end, lineNo);
    }
};

// printf info string consumer
class CLRX_INTERNAL YAMLPrintfVectorConsumer: public YAMLElemConsumer
{
private:
    std::unordered_set<cxuint> printfIds;
public:
    std::vector<ROCmPrintfInfo>& printfInfos;
    
    YAMLPrintfVectorConsumer(std::vector<ROCmPrintfInfo>& _printInfos)
        : printfInfos(_printInfos)
    { }
    
    virtual void consume(const char*& ptr, const char* end, size_t& lineNo,
                cxuint prevIndent, bool singleValue, bool blockAccept)
    {
        const size_t oldLineNo = lineNo;
        std::string str = parseYAMLStringValue(ptr, end, lineNo, prevIndent,
                                singleValue, blockAccept);
        // parse printf string
        ROCmPrintfInfo printfInfo{};
        
        const char* ptr2 = str.c_str();
        const char* end2 = str.c_str() + str.size();
        skipSpacesToLineEnd(ptr2, end2);
        try
        { printfInfo.id = cstrtovCStyle<uint32_t>(ptr2, end2, ptr2); }
        catch(const ParseException& ex)
        { throw ParseException(oldLineNo, ex.what()); }
        
        // check printf id uniqueness
        if (!printfIds.insert(printfInfo.id).second)
            throw ParseException(oldLineNo, "Duplicate of printf id");
        
        skipSpacesToLineEnd(ptr2, end2);
        if (ptr2==end || *ptr2!=':')
            throw ParseException(oldLineNo, "No colon after printf callId");
        ptr2++;
        skipSpacesToLineEnd(ptr2, end2);
        uint32_t argsNum = cstrtovCStyle<uint32_t>(ptr2, end2, ptr2);
        skipSpacesToLineEnd(ptr2, end2);
        if (ptr2==end || *ptr2!=':')
            throw ParseException(oldLineNo, "No colon after printf argsNum");
        ptr2++;
        
        printfInfo.argSizes.resize(argsNum);
        
        // parse arg sizes
        for (size_t i = 0; i < argsNum; i++)
        {
            skipSpacesToLineEnd(ptr2, end2);
            printfInfo.argSizes[i] = cstrtovCStyle<uint32_t>(ptr2, end2, ptr2);
            skipSpacesToLineEnd(ptr2, end2);
            if (ptr2==end || *ptr2!=':')
                throw ParseException(lineNo, "No colon after printf argsNum");
            ptr2++;
        }
        // format
        printfInfo.format.assign(ptr2, end2);
        
        printfInfos.push_back(printfInfo);
    }
};

// skip YAML value after key
static void skipYAMLValue(const char*& ptr, const char* end, size_t& lineNo,
                cxuint prevIndent, bool singleValue = true)
{
    skipSpacesToLineEnd(ptr, end);
    if (ptr+2 >= end && ptr[0]=='!' && ptr[1]=='!')
    {   // skip !!xxxxx
        ptr+=2;
        while (ptr!=end && isAlpha(*ptr)) ptr++;
        skipSpacesToLineEnd(ptr, end);
    }
    
    if (ptr==end || (*ptr!='\'' && *ptr!='"' && *ptr!='|' && *ptr!='>' && *ptr !='[' &&
                *ptr!='#' && *ptr!='\n'))
    {
        while (ptr!=end && *ptr!='\n') ptr++;
        skipSpacesToNextLine(ptr, end, lineNo);
        return;
    }
    // string
    if (*ptr=='\'' || *ptr=='"')
    {
        const char delim = *ptr++;
        bool escape = false;
        while(ptr!=end && (escape || *ptr!=delim))
        {
            if (!escape && *ptr=='\\')
                escape = true;
            else if (escape)
                escape = false;
            if (*ptr=='\n') lineNo++;
            ptr++;
        }
        if (ptr==end)
            throw ParseException(lineNo, "Unterminated string");
        ptr++;
        if (singleValue)
            skipSpacesToNextLine(ptr, end, lineNo);
    }
    else if (*ptr=='[')
    {   // otherwise [array]
        ptr++;
        skipSpacesAndComments(ptr, end, lineNo);
        while (ptr != end)
        {
            // parse in line
            if (ptr!=end && (*ptr=='\'' || *ptr=='"'))
                // skip YAML string
                skipYAMLValue(ptr, end, lineNo, 0, false);
            else
                while (ptr!=end && *ptr!='\n' &&
                            *ptr!='#' && *ptr!=',' && *ptr!=']') ptr++;
            skipSpacesAndComments(ptr, end, lineNo);
            
            if (ptr!=end && *ptr==']')
                // just end
                break;
            else if (ptr!=end && *ptr!=',')
                throw ParseException(lineNo, "Expected ','");
            ptr++;
            skipSpacesAndComments(ptr, end, lineNo);
        }
        if (ptr == end)
            throw ParseException(lineNo, "Unterminated array");
        ptr++;
        skipSpacesToNextLine(ptr, end, lineNo);
    }
    else
    {   // block value
        bool blockValue = false;
        if (ptr!=end && (*ptr=='|' || *ptr=='>'))
        {
            ptr++; // skip '|' or '>'
            blockValue = true;
        }
        if (ptr!=end && *ptr=='#')
            while (ptr!=end && *ptr!='\n') ptr++;
        else
            skipSpacesToLineEnd(ptr, end);
        if (ptr!=end && *ptr!='\n')
            throw ParseException(lineNo, "Garbages before block or children");
        ptr++;
        lineNo++;
        // skip all lines indented beyound previous level
        while (ptr != end)
        {
            const char* lineStart = ptr;
            skipSpacesToLineEnd(ptr, end);
            if (ptr == end)
            {
                ptr++;
                lineNo++;
                continue;
            }
            if (size_t(ptr-lineStart) <= prevIndent && *ptr!='\n' &&
                (blockValue || *ptr!='#'))
                // if indent is short and not empty line (same spaces) or
                // or with only comment and not blockValue
            {
                ptr = lineStart;
                break;
            }
            
            while (ptr!=end && *ptr!='\n') ptr++;
            if (ptr!=end)
            {
                lineNo++;
                ptr++;
            }
        }
    }
}

enum {
    ROCMMT_MAIN_KERNELS = 0, ROCMMT_MAIN_PRINTF,  ROCMMT_MAIN_VERSION
};

static const char* mainMetadataKeywords[] =
{
    "Kernels", "Printf", "Version"
};

static const size_t mainMetadataKeywordsNum =
        sizeof(mainMetadataKeywords) / sizeof(const char*);

enum {
    ROCMMT_KERNEL_ARGS = 0, ROCMMT_KERNEL_ATTRS, ROCMMT_KERNEL_CODEPROPS,
    ROCMMT_KERNEL_LANGUAGE, ROCMMT_KERNEL_LANGUAGE_VERSION,
    ROCMMT_KERNEL_NAME, ROCMMT_KERNEL_SYMBOLNAME
};

static const char* kernelMetadataKeywords[] =
{
    "Args", "Attrs", "CodeProps", "Language", "LanguageVersion", "Name", "SymbolName"
};

static const size_t kernelMetadataKeywordsNum =
        sizeof(kernelMetadataKeywords) / sizeof(const char*);

enum {
    ROCMMT_ATTRS_REQD_WORK_GROUP_SIZE = 0, ROCMMT_ATTRS_RUNTIME_HANDLE,
    ROCMMT_ATTRS_VECTYPEHINT, ROCMMT_ATTRS_WORK_GROUP_SIZE_HINT
};

static const char* kernelAttrMetadataKeywords[] =
{
    "ReqdWorkGroupSize", "RuntimeHandle", "VecTypeHint", "WorkGroupSizeHint"
};

static const size_t kernelAttrMetadataKeywordsNum =
        sizeof(kernelAttrMetadataKeywords) / sizeof(const char*);

enum {
    ROCMMT_CODEPROPS_FIXED_WORK_GROUP_SIZE = 0, ROCMMT_CODEPROPS_GROUP_SEGMENT_FIXED_SIZE,
    ROCMMT_CODEPROPS_KERNARG_SEGMENT_ALIGN, ROCMMT_CODEPROPS_KERNARG_SEGMENT_SIZE,
    ROCMMT_CODEPROPS_MAX_FLAT_WORK_GROUP_SIZE, ROCMMT_CODEPROPS_NUM_SGPRS,
    ROCMMT_CODEPROPS_NUM_SPILLED_SGPRS, ROCMMT_CODEPROPS_NUM_SPILLED_VGPRS,
    ROCMMT_CODEPROPS_NUM_VGPRS, ROCMMT_CODEPROPS_PRIVATE_SEGMENT_FIXED_SIZE,
    ROCMMT_CODEPROPS_WAVEFRONT_SIZE
};

static const char* kernelCodePropsKeywords[] =
{
    "FixedWorkGroupSize", "GroupSegmentFixedSize", "KernargSegmentAlign",
    "KernargSegmentSize", "MaxFlatWorkGroupSize", "NumSGPRs",
    "NumSpilledSGPRs", "NumSpilledVGPRs", "NumVGPRs", "PrivateSegmentFixedSize",
    "WavefrontSize"
};

static const size_t kernelCodePropsKeywordsNum =
        sizeof(kernelCodePropsKeywords) / sizeof(const char*);

enum {
    ROCMMT_ARGS_ACCQUAL = 0, ROCMMT_ARGS_ACTUALACCQUAL, ROCMMT_ARGS_ADDRSPACEQUAL,
    ROCMMT_ARGS_ALIGN, ROCMMT_ARGS_ISCONST, ROCMMT_ARGS_ISPIPE, ROCMMT_ARGS_ISRESTRICT,
    ROCMMT_ARGS_ISVOLATILE, ROCMMT_ARGS_NAME, ROCMMT_ARGS_POINTEE_ALIGN,
    ROCMMT_ARGS_SIZE, ROCMMT_ARGS_TYPENAME, ROCMMT_ARGS_VALUEKIND,
    ROCMMT_ARGS_VALUETYPE
};

static const char* kernelArgInfosKeywords[] =
{
    "AccQual", "ActualAccQual", "AddrSpaceQual", "Align", "IsConst", "IsPipe",
    "IsRestrict", "IsVolatile", "Name", "PointeeAlign", "Size", "TypeName",
    "ValueKind", "ValueType"
};

static const size_t kernelArgInfosKeywordsNum =
        sizeof(kernelArgInfosKeywords) / sizeof(const char*);

static const std::pair<const char*, ROCmValueKind> rocmValueKindNamesMap[] =
{
    { "ByValue", ROCmValueKind::BY_VALUE },
    { "DynamicSharedPointer", ROCmValueKind::DYN_SHARED_PTR },
    { "GlobalBuffer", ROCmValueKind::GLOBAL_BUFFER },
    { "HiddenCompletionAction", ROCmValueKind::HIDDEN_COMPLETION_ACTION },
    { "HiddenDefaultQueue", ROCmValueKind::HIDDEN_DEFAULT_QUEUE },
    { "HiddenGlobalOffsetX", ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X },
    { "HiddenGlobalOffsetY", ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Y },
    { "HiddenGlobalOffsetZ", ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Z },
    { "HiddenNone", ROCmValueKind::HIDDEN_NONE },
    { "HiddenPrintfBuffer", ROCmValueKind::HIDDEN_PRINTF_BUFFER },
    { "Image", ROCmValueKind::IMAGE },
    { "Pipe", ROCmValueKind::PIPE },
    { "Queue", ROCmValueKind::QUEUE },
    { "Sampler", ROCmValueKind::SAMPLER }
};

static const size_t rocmValueKindNamesNum =
        sizeof(rocmValueKindNamesMap) / sizeof(std::pair<const char*, ROCmValueKind>);

static const std::pair<const char*, ROCmValueType> rocmValueTypeNamesMap[] =
{
    { "F16", ROCmValueType::FLOAT16 },
    { "F32", ROCmValueType::FLOAT32 },
    { "F64", ROCmValueType::FLOAT64 },
    { "I16", ROCmValueType::INT16 },
    { "I32", ROCmValueType::INT32 },
    { "I64", ROCmValueType::INT64 },
    { "I8", ROCmValueType::INT8 },
    { "Struct", ROCmValueType::STRUCTURE },
    { "U16", ROCmValueType::UINT16 },
    { "U32", ROCmValueType::UINT32 },
    { "U64", ROCmValueType::UINT64 },
    { "U8", ROCmValueType::UINT8 }
};

static const size_t rocmValueTypeNamesNum =
        sizeof(rocmValueTypeNamesMap) / sizeof(std::pair<const char*, ROCmValueType>);

static const char* rocmAddrSpaceTypesTbl[] =
{ "Private", "Global", "Constant", "Local", "Generic", "Region" };

static const char* rocmAccessQualifierTbl[] =
{ "Default", "ReadOnly", "WriteOnly", "ReadWrite" };

static void parseROCmMetadata(size_t metadataSize, const char* metadata,
                ROCmMetadata& metadataInfo)
{
    const char* ptr = metadata;
    const char* end = metadata + metadataSize;
    size_t lineNo = 1;
    // init metadata info object
    metadataInfo.kernels.clear();
    metadataInfo.printfInfos.clear();
    metadataInfo.version[0] = metadataInfo.version[1] = 0;
    
    std::vector<ROCmKernelMetadata>& kernels = metadataInfo.kernels;
    
    cxuint levels[6] = { UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX };
    cxuint curLevel = 0;
    bool inKernels = false;
    bool inKernel = false;
    bool inKernelArgs = false;
    bool inKernelArg = false;
    bool inKernelCodeProps = false;
    bool inKernelAttrs = false;
    bool canToNextLevel = false;
    
    size_t oldLineNo = 0;
    while (ptr != end)
    {
        cxuint level = skipSpacesAndComments(ptr, end, lineNo);
        if (ptr == end || lineNo == oldLineNo)
            throw ParseException(lineNo, "Expected new line");
        
        if (levels[curLevel] == UINT_MAX)
            levels[curLevel] = level;
        else if (levels[curLevel] < level)
        {
            if (canToNextLevel)
                // go to next nesting level
                levels[++curLevel] = level;
            else
                throw ParseException(lineNo, "Unexpected nesting level");
            canToNextLevel = false;
        }
        else if (levels[curLevel] > level)
        {
            while (curLevel != UINT_MAX && levels[curLevel] > level)
                curLevel--;
            if (curLevel == UINT_MAX)
                throw ParseException(lineNo, "Indentation smaller than in main level");
            
            // pop from previous level
            if (curLevel < 3)
            {
                if (inKernelArgs)
                {
                    // leave from kernel args
                    inKernelArgs = false;
                    inKernelArg = false;
                }
            
                inKernelCodeProps = false;
                inKernelAttrs = false;
            }
            if (curLevel < 1 && inKernels)
            {
                // leave from kernels
                inKernels = false;
                inKernel = false;
            }
            
            if (levels[curLevel] != level)
                throw ParseException(lineNo, "Unexpected nesting level");
        }
        
        oldLineNo = lineNo;
        if (curLevel == 0)
        {
            if (lineNo==1 && ptr+3 <= end && *ptr=='-' && ptr[1]=='-' && ptr[2]=='-' &&
                (ptr+3==end || (ptr+3 < end && ptr[3]=='\n')))
            {
                ptr += 3;
                if (ptr!=end)
                {
                    lineNo++;
                    ptr++; // to newline
                }
                continue; // skip document start
            }
            
            if (ptr+3 <= end && *ptr=='.' && ptr[1]=='.' && ptr[2]=='.' &&
                (ptr+3==end || (ptr+3 < end && ptr[3]=='\n')))
                break; // end of the document
            
            const size_t keyIndex = parseYAMLKey(ptr, end, lineNo,
                        mainMetadataKeywordsNum, mainMetadataKeywords);
            
            switch(keyIndex)
            {
                case ROCMMT_MAIN_KERNELS:
                    inKernels = true;
                    canToNextLevel = true;
                    break;
                case ROCMMT_MAIN_PRINTF:
                {
                    YAMLPrintfVectorConsumer consumer(metadataInfo.printfInfos);
                    parseYAMLValArray(ptr, end, lineNo, levels[curLevel], &consumer, true);
                    break;
                }
                case ROCMMT_MAIN_VERSION:
                {
                    YAMLIntArrayConsumer<uint32_t> consumer(2, metadataInfo.version);
                    parseYAMLValArray(ptr, end, lineNo, levels[curLevel], &consumer, true);
                    break;
                }
                default:
                    skipYAMLValue(ptr, end, lineNo, level);
                    break;
            }
        }
        
        if (curLevel==1 && inKernels)
        {
            // enter to kernel level
            if (ptr == end || *ptr != '-')
                throw ParseException(lineNo, "No '-' before kernel object");
            ptr++;
            const char* afterMinus = ptr;
            skipSpacesToLineEnd(ptr, end);
            levels[++curLevel] = level + 1 + ptr-afterMinus;
            level = levels[curLevel];
            inKernel = true;
            
            kernels.push_back(ROCmKernelMetadata());
            kernels.back().initialize();
        }
        
        if (curLevel==2 && inKernel)
        {
            // in kernel
            const size_t keyIndex = parseYAMLKey(ptr, end, lineNo,
                        kernelMetadataKeywordsNum, kernelMetadataKeywords);
            
            ROCmKernelMetadata& kernel = kernels.back();
            switch(keyIndex)
            {
                case ROCMMT_KERNEL_ARGS:
                    inKernelArgs = true;
                    canToNextLevel = true;
                    kernel.argInfos.clear();
                    break;
                case ROCMMT_KERNEL_ATTRS:
                    inKernelAttrs = true;
                    canToNextLevel = true;
                    // initialize kernel attributes values
                    kernel.reqdWorkGroupSize[0] = 0;
                    kernel.reqdWorkGroupSize[1] = 0;
                    kernel.reqdWorkGroupSize[2] = 0;
                    kernel.workGroupSizeHint[0] = 0;
                    kernel.workGroupSizeHint[1] = 0;
                    kernel.workGroupSizeHint[2] = 0;
                    kernel.runtimeHandle.clear();
                    kernel.vecTypeHint.clear();
                    break;
                case ROCMMT_KERNEL_CODEPROPS:
                    // initialize CodeProps values
                    kernel.kernargSegmentSize = BINGEN64_DEFAULT;
                    kernel.groupSegmentFixedSize = BINGEN64_DEFAULT;
                    kernel.privateSegmentFixedSize = BINGEN64_DEFAULT;
                    kernel.kernargSegmentAlign = BINGEN64_DEFAULT;
                    kernel.wavefrontSize = BINGEN_DEFAULT;
                    kernel.sgprsNum = BINGEN_DEFAULT;
                    kernel.vgprsNum = BINGEN_DEFAULT;
                    kernel.spilledSgprs = BINGEN_NOTSUPPLIED;
                    kernel.spilledVgprs = BINGEN_NOTSUPPLIED;
                    kernel.maxFlatWorkGroupSize = BINGEN64_DEFAULT;
                    kernel.fixedWorkGroupSize[0] = 0;
                    kernel.fixedWorkGroupSize[1] = 0;
                    kernel.fixedWorkGroupSize[2] = 0;
                    inKernelCodeProps = true;
                    canToNextLevel = true;
                    break;
                case ROCMMT_KERNEL_LANGUAGE:
                    kernel.language = parseYAMLStringValue(ptr, end, lineNo, level, true);
                    break;
                case ROCMMT_KERNEL_LANGUAGE_VERSION:
                {
                    YAMLIntArrayConsumer<uint32_t> consumer(2, kernel.langVersion);
                    parseYAMLValArray(ptr, end, lineNo, levels[curLevel], &consumer);
                    break;
                }
                case ROCMMT_KERNEL_NAME:
                    kernel.name = parseYAMLStringValue(ptr, end, lineNo, level, true);
                    break;
                case ROCMMT_KERNEL_SYMBOLNAME:
                    kernel.symbolName = parseYAMLStringValue(ptr, end, lineNo, level, true);
                    break;
                default:
                    skipYAMLValue(ptr, end, lineNo, level);
                    break;
            }
        }
        
        if (curLevel==3 && inKernelAttrs)
        {
            // in kernel attributes
            const size_t keyIndex = parseYAMLKey(ptr, end, lineNo,
                        kernelAttrMetadataKeywordsNum, kernelAttrMetadataKeywords);
            
            ROCmKernelMetadata& kernel = kernels.back();
            switch(keyIndex)
            {
                case ROCMMT_ATTRS_REQD_WORK_GROUP_SIZE:
                {
                    YAMLIntArrayConsumer<cxuint> consumer(3, kernel.reqdWorkGroupSize);
                    parseYAMLValArray(ptr, end, lineNo, level, &consumer);
                    break;
                }
                case ROCMMT_ATTRS_RUNTIME_HANDLE:
                    kernel.runtimeHandle = parseYAMLStringValue(
                                ptr, end, lineNo, level, true);
                    break;
                case ROCMMT_ATTRS_VECTYPEHINT:
                    kernel.vecTypeHint = parseYAMLStringValue(
                                ptr, end, lineNo, level, true);
                    break;
                case ROCMMT_ATTRS_WORK_GROUP_SIZE_HINT:
                {
                    YAMLIntArrayConsumer<cxuint> consumer(3, kernel.workGroupSizeHint);
                    parseYAMLValArray(ptr, end, lineNo, level, &consumer, true);
                    break;
                }
                default:
                    skipYAMLValue(ptr, end, lineNo, level);
                    break;
            }
        }
        
        if (curLevel==3 && inKernelCodeProps)
        {
            // in kernel codeProps
            const size_t keyIndex = parseYAMLKey(ptr, end, lineNo,
                        kernelCodePropsKeywordsNum, kernelCodePropsKeywords);
            
            ROCmKernelMetadata& kernel = kernels.back();
            switch(keyIndex)
            {
                case ROCMMT_CODEPROPS_FIXED_WORK_GROUP_SIZE:
                {
                    YAMLIntArrayConsumer<cxuint> consumer(3, kernel.fixedWorkGroupSize);
                    parseYAMLValArray(ptr, end, lineNo, level, &consumer);
                    break;
                }
                case ROCMMT_CODEPROPS_GROUP_SEGMENT_FIXED_SIZE:
                    kernel.groupSegmentFixedSize =
                                parseYAMLIntValue<cxuint>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_KERNARG_SEGMENT_ALIGN:
                    kernel.kernargSegmentAlign =
                                parseYAMLIntValue<uint64_t>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_KERNARG_SEGMENT_SIZE:
                    kernel.kernargSegmentSize =
                                parseYAMLIntValue<uint64_t>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_MAX_FLAT_WORK_GROUP_SIZE:
                    kernel.maxFlatWorkGroupSize =
                                parseYAMLIntValue<uint64_t>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_NUM_SGPRS:
                    kernel.sgprsNum = parseYAMLIntValue<cxuint>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_NUM_SPILLED_SGPRS:
                    kernel.spilledSgprs =
                            parseYAMLIntValue<cxuint>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_NUM_SPILLED_VGPRS:
                    kernel.spilledVgprs =
                            parseYAMLIntValue<cxuint>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_NUM_VGPRS:
                    kernel.vgprsNum = parseYAMLIntValue<cxuint>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_PRIVATE_SEGMENT_FIXED_SIZE:
                    kernel.privateSegmentFixedSize =
                                parseYAMLIntValue<uint64_t>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_CODEPROPS_WAVEFRONT_SIZE:
                    kernel.wavefrontSize =
                            parseYAMLIntValue<cxuint>(ptr, end, lineNo, true);
                    break;
                default:
                    skipYAMLValue(ptr, end, lineNo, level);
                    break;
            }
        }
        
        if (curLevel==3 && inKernelArgs)
        {
            // enter to kernel argument level
            if (ptr == end || *ptr != '-')
                throw ParseException(lineNo, "No '-' before argument object");
            ptr++;
            const char* afterMinus = ptr;
            skipSpacesToLineEnd(ptr, end);
            levels[++curLevel] = level + 1 + ptr-afterMinus;
            level = levels[curLevel];
            inKernelArg = true;
            
            kernels.back().argInfos.push_back(ROCmKernelArgInfo{});
        }
        
        if (curLevel==4 && inKernelArg)
        {
            // in kernel argument
            const size_t keyIndex = parseYAMLKey(ptr, end, lineNo,
                        kernelArgInfosKeywordsNum, kernelArgInfosKeywords);
            
            ROCmKernelArgInfo& kernelArg = kernels.back().argInfos.back();
            
            size_t valLineNo = lineNo;
            switch(keyIndex)
            {
                case ROCMMT_ARGS_ACCQUAL:
                case ROCMMT_ARGS_ACTUALACCQUAL:
                {
                    const std::string acc = trimStrSpaces(parseYAMLStringValue(
                                    ptr, end, lineNo, level, true));
                    size_t accIndex = 0;
                    for (; accIndex < 6; accIndex++)
                        if (::strcmp(rocmAccessQualifierTbl[accIndex], acc.c_str())==0)
                            break;
                    if (accIndex == 4)
                        throw ParseException(lineNo, "Wrong access qualifier");
                    if (keyIndex == ROCMMT_ARGS_ACCQUAL)
                        kernelArg.accessQual = ROCmAccessQual(accIndex);
                    else
                        kernelArg.actualAccessQual = ROCmAccessQual(accIndex);
                    break;
                }
                case ROCMMT_ARGS_ADDRSPACEQUAL:
                {
                    const std::string aspace = trimStrSpaces(parseYAMLStringValue(
                                    ptr, end, lineNo, level, true));
                    size_t aspaceIndex = 0;
                    for (; aspaceIndex < 6; aspaceIndex++)
                        if (::strcmp(rocmAddrSpaceTypesTbl[aspaceIndex],
                                    aspace.c_str())==0)
                            break;
                    if (aspaceIndex == 6)
                        throw ParseException(valLineNo, "Wrong address space");
                    kernelArg.addressSpace = ROCmAddressSpace(aspaceIndex+1);
                    break;
                }
                case ROCMMT_ARGS_ALIGN:
                    kernelArg.align = parseYAMLIntValue<uint64_t>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_ARGS_ISCONST:
                    kernelArg.isConst = parseYAMLBoolValue(ptr, end, lineNo, true);
                    break;
                case ROCMMT_ARGS_ISPIPE:
                    kernelArg.isPipe = parseYAMLBoolValue(ptr, end, lineNo, true);
                    break;
                case ROCMMT_ARGS_ISRESTRICT:
                    kernelArg.isRestrict = parseYAMLBoolValue(ptr, end, lineNo, true);
                    break;
                case ROCMMT_ARGS_ISVOLATILE:
                    kernelArg.isVolatile = parseYAMLBoolValue(ptr, end, lineNo, true);
                    break;
                case ROCMMT_ARGS_NAME:
                    kernelArg.name = parseYAMLStringValue(ptr, end, lineNo, level, true);
                    break;
                case ROCMMT_ARGS_POINTEE_ALIGN:
                    kernelArg.pointeeAlign =
                                parseYAMLIntValue<uint64_t>(ptr, end, lineNo, true);
                    break;
                case ROCMMT_ARGS_SIZE:
                    kernelArg.size = parseYAMLIntValue<uint64_t>(ptr, end, lineNo);
                    break;
                case ROCMMT_ARGS_TYPENAME:
                    kernelArg.typeName =
                                parseYAMLStringValue(ptr, end, lineNo, level, true);
                    break;
                case ROCMMT_ARGS_VALUEKIND:
                {
                    const std::string vkind = trimStrSpaces(parseYAMLStringValue(
                                ptr, end, lineNo, level, true));
                    const size_t vkindIndex = binaryMapFind(rocmValueKindNamesMap,
                            rocmValueKindNamesMap + rocmValueKindNamesNum, vkind.c_str(),
                            CStringLess()) - rocmValueKindNamesMap;
                    // if unknown kind
                    if (vkindIndex == rocmValueKindNamesNum)
                        throw ParseException(valLineNo, "Wrong argument value kind");
                    kernelArg.valueKind = rocmValueKindNamesMap[vkindIndex].second;
                    break;
                }
                case ROCMMT_ARGS_VALUETYPE:
                {
                    const std::string vtype = trimStrSpaces(parseYAMLStringValue(
                                    ptr, end, lineNo, level, true));
                    const size_t vtypeIndex = binaryMapFind(rocmValueTypeNamesMap,
                            rocmValueTypeNamesMap + rocmValueTypeNamesNum, vtype.c_str(),
                            CStringLess()) - rocmValueTypeNamesMap;
                    // if unknown type
                    if (vtypeIndex == rocmValueTypeNamesNum)
                        throw ParseException(valLineNo, "Wrong argument value type");
                    kernelArg.valueType = rocmValueTypeNamesMap[vtypeIndex].second;
                    break;
                }
                default:
                    skipYAMLValue(ptr, end, lineNo, level);
                    break;
            }
        }
    }
}

void ROCmMetadata::parse(size_t metadataSize, const char* metadata)
{
    parseROCmMetadata(metadataSize, metadata, *this);
}

/*
 * ROCm binary reader and generator
 */

/* TODO: add support for various kernel code offset (now only 256 is supported) */

ROCmBinary::ROCmBinary(size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags)
        : ElfBinary64(binaryCodeSize, binaryCode, creationFlags),
          regionsNum(0), codeSize(0), code(nullptr),
          globalDataSize(0), globalData(nullptr), metadataSize(0), metadata(nullptr),
          newBinFormat(false)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    uint64_t codeOffset = 0;
    // find '.text' section
    if (textIndex!=SHN_UNDEF)
    {
        code = getSectionContent(textIndex);
        const Elf64_Shdr& textShdr = getSectionHeader(textIndex);
        codeSize = ULEV(textShdr.sh_size);
        codeOffset = ULEV(textShdr.sh_offset);
    }
    
    cxuint rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { } // ignore failed
    // find '.text' section
    if (rodataIndex!=SHN_UNDEF)
    {
        globalData = getSectionContent(rodataIndex);
        const Elf64_Shdr& rodataShdr = getSectionHeader(rodataIndex);
        globalDataSize = ULEV(rodataShdr.sh_size);
    }
    
    cxuint gpuConfigIndex = SHN_UNDEF;
    try
    { gpuConfigIndex = getSectionIndex(".AMDGPU.config"); }
    catch(const Exception& ex)
    { } // ignore failed
    newBinFormat = (gpuConfigIndex == SHN_UNDEF);
    
    cxuint relaDynIndex = SHN_UNDEF;
    try
    { relaDynIndex = getSectionIndex(".rela.dyn"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    cxuint gotIndex = SHN_UNDEF;
    try
    { gotIndex = getSectionIndex(".got"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    // counts regions (symbol or kernel)
    regionsNum = 0;
    const size_t symbolsNum = getSymbolsNum();
    for (size_t i = 0; i < symbolsNum; i++)
    {
        // count regions number
        const Elf64_Sym& sym = getSymbol(i);
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        const cxbyte bind = ELF64_ST_BIND(sym.st_info);
        if (ULEV(sym.st_shndx)==textIndex &&
            (symType==STT_GNU_IFUNC || symType==STT_FUNC ||
                (bind==STB_GLOBAL && symType==STT_OBJECT)))
            regionsNum++;
    }
    if (code==nullptr && regionsNum!=0)
        throw BinException("No code if regions number is not zero");
    regions.reset(new ROCmRegion[regionsNum]);
    size_t j = 0;
    typedef std::pair<uint64_t, size_t> RegionOffsetEntry;
    std::unique_ptr<RegionOffsetEntry[]> symOffsets(new RegionOffsetEntry[regionsNum]);
    
    // get regions info
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const Elf64_Sym& sym = getSymbol(i);
        if (ULEV(sym.st_shndx)!=textIndex)
            continue;   // if not in '.text' section
        const size_t value = ULEV(sym.st_value);
        if (value < codeOffset)
            throw BinException("Region offset is too small!");
        const size_t size = ULEV(sym.st_size);
        
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        const cxbyte bind = ELF64_ST_BIND(sym.st_info);
        if (symType==STT_GNU_IFUNC || symType==STT_FUNC ||
                (bind==STB_GLOBAL && symType==STT_OBJECT))
        {
            ROCmRegionType type = ROCmRegionType::DATA;
            // if kernel
            if (symType==STT_GNU_IFUNC) 
                type = ROCmRegionType::KERNEL;
            // if function kernel
            else if (symType==STT_FUNC)
                type = ROCmRegionType::FKERNEL;
            symOffsets[j] = std::make_pair(value, j);
            if (type!=ROCmRegionType::DATA && value+0x100 > codeOffset+codeSize)
                throw BinException("Kernel or code offset is too big!");
            regions[j++] = { getSymbolName(i), size, value, type };
        }
    }
    // sort regions by offset
    std::sort(symOffsets.get(), symOffsets.get()+regionsNum,
            [](const RegionOffsetEntry& a, const RegionOffsetEntry& b)
            { return a.first < b.first; });
    // checking distance between regions
    for (size_t i = 1; i <= regionsNum; i++)
    {
        size_t end = (i<regionsNum) ? symOffsets[i].first : codeOffset+codeSize;
        ROCmRegion& region = regions[symOffsets[i-1].second];
        if (region.type==ROCmRegionType::KERNEL && symOffsets[i-1].first+0x100 > end)
            throw BinException("Kernel size is too small!");
        
        const size_t regSize = end - symOffsets[i-1].first;
        if (region.size==0)
            region.size = regSize;
        else
            region.size = std::min(regSize, region.size);
    }
    
    // load got symbols
    if (relaDynIndex != SHN_UNDEF && gotIndex != SHN_UNDEF)
    {
        const Elf64_Shdr& relaShdr = getSectionHeader(relaDynIndex);
        const Elf64_Shdr& gotShdr = getSectionHeader(gotIndex);
        
        size_t relaEntrySize = ULEV(relaShdr.sh_entsize);
        if (relaEntrySize==0)
            relaEntrySize = sizeof(Elf64_Rela);
        const size_t relaEntriesNum = ULEV(relaShdr.sh_size)/relaEntrySize;
        const size_t gotEntriesNum = ULEV(gotShdr.sh_size) >> 3;
        if (gotEntriesNum != relaEntriesNum)
            throw BinException("RelaDyn entries number and GOT entries "
                        "number doesn't match!");
        
        // initialize GOT symbols table
        gotSymbols.resize(gotEntriesNum);
        const cxbyte* relaDyn = getSectionContent(relaDynIndex);
        for (size_t i = 0; i < relaEntriesNum; i++)
        {
            const Elf64_Rela& rela = *reinterpret_cast<const Elf64_Rela*>(
                            relaDyn + relaEntrySize*i);
            // check rela entry fields
            if (ULEV(rela.r_offset) != ULEV(gotShdr.sh_offset) + i*8)
                throw BinException("Wrong dyn relocation offset");
            if (ULEV(rela.r_addend) != 0ULL)
                throw BinException("Wrong dyn relocation addend");
            size_t symIndex = ELF64_R_SYM(ULEV(rela.r_info));
            if (symIndex >= getDynSymbolsNum())
                throw BinException("Dyn relocation symbol index out of range");
            // just set in gotSymbols
            gotSymbols[i] = symIndex;
        }
    }
    
    // get metadata
    const size_t notesSize = getNotesSize();
    const cxbyte* noteContent = (const cxbyte*)getNotes();
    
    for (size_t offset = 0; offset < notesSize; )
    {
        const Elf64_Nhdr* nhdr = (const Elf64_Nhdr*)(noteContent + offset);
        size_t namesz = ULEV(nhdr->n_namesz);
        size_t descsz = ULEV(nhdr->n_descsz);
        if (usumGt(offset, namesz+descsz, notesSize))
            throw BinException("Note offset+size out of range");
        
        if (namesz==4 &&
            ::strcmp((const char*)noteContent+offset+ sizeof(Elf64_Nhdr), "AMD")==0)
        {
            const uint32_t noteType = ULEV(nhdr->n_type);
            if (noteType == 0xa)
            {
                metadata = (char*)(noteContent+offset+sizeof(Elf64_Nhdr) + 4);
                metadataSize = descsz;
            }
            else if (noteType == 0xb)
                target.assign((char*)(noteContent+offset+sizeof(Elf64_Nhdr) + 4), descsz);
        }
        size_t align = (((namesz+descsz)&3)!=0) ? 4-((namesz+descsz)&3) : 0;
        offset += sizeof(Elf64_Nhdr) + namesz + descsz + align;
    }
    
    if (hasRegionMap())
    {
        // create region map
        regionsMap.resize(regionsNum);
        for (size_t i = 0; i < regionsNum; i++)
            regionsMap[i] = std::make_pair(regions[i].regionName, i);
        // sort region map
        mapSort(regionsMap.begin(), regionsMap.end());
    }
    
    if ((creationFlags & ROCMBIN_CREATE_METADATAINFO) != 0 &&
        metadata != nullptr && metadataSize != 0)
    {
        metadataInfo.reset(new ROCmMetadata());
        parseROCmMetadata(metadataSize, metadata, *metadataInfo);
        
        if (hasKernelInfoMap())
        {
            const std::vector<ROCmKernelMetadata>& kernels = metadataInfo->kernels;
            kernelInfosMap.resize(kernels.size());
            for (size_t i = 0; i < kernelInfosMap.size(); i++)
                kernelInfosMap[i] = std::make_pair(kernels[i].name, i);
            // sort region map
            mapSort(kernelInfosMap.begin(), kernelInfosMap.end());
        }
    }
}

/// determint GPU device from ROCm notes
GPUDeviceType ROCmBinary::determineGPUDeviceType(uint32_t& outArchMinor,
                     uint32_t& outArchStepping) const
{
    uint32_t archMajor = 0;
    uint32_t archMinor = 0;
    uint32_t archStepping = 0;
    
    {
        const cxbyte* noteContent = (const cxbyte*)getNotes();
        if (noteContent==nullptr)
            throw BinException("Missing notes in inner binary!");
        size_t notesSize = getNotesSize();
        // find note about AMDGPU
        for (size_t offset = 0; offset < notesSize; )
        {
            const Elf64_Nhdr* nhdr = (const Elf64_Nhdr*)(noteContent + offset);
            size_t namesz = ULEV(nhdr->n_namesz);
            size_t descsz = ULEV(nhdr->n_descsz);
            if (usumGt(offset, namesz+descsz, notesSize))
                throw BinException("Note offset+size out of range");
            if (ULEV(nhdr->n_type) == 0x3 && namesz==4 && descsz>=0x1a &&
                ::strcmp((const char*)noteContent+offset+sizeof(Elf64_Nhdr), "AMD")==0)
            {    // AMDGPU type
                const uint32_t* content = (const uint32_t*)
                        (noteContent+offset+sizeof(Elf64_Nhdr) + 4);
                archMajor = ULEV(content[1]);
                archMinor = ULEV(content[2]);
                archStepping = ULEV(content[3]);
            }
            size_t align = (((namesz+descsz)&3)!=0) ? 4-((namesz+descsz)&3) : 0;
            offset += sizeof(Elf64_Nhdr) + namesz + descsz + align;
        }
    }
    // determine device type
    GPUDeviceType deviceType = getGPUDeviceTypeFromArchVersion(archMajor, archMinor,
                                    archStepping);
    outArchMinor = archMinor;
    outArchStepping = archStepping;
    return deviceType;
}

const ROCmRegion& ROCmBinary::getRegion(const char* name) const
{
    RegionMap::const_iterator it = binaryMapFind(regionsMap.begin(),
                             regionsMap.end(), name);
    if (it == regionsMap.end())
        throw BinException("Can't find region name");
    return regions[it->second];
}

const ROCmKernelMetadata& ROCmBinary::getKernelInfo(const char* name) const
{
    if (!hasMetadataInfo())
        throw BinException("Can't find kernel info name");
    RegionMap::const_iterator it = binaryMapFind(kernelInfosMap.begin(),
                             kernelInfosMap.end(), name);
    if (it == kernelInfosMap.end())
        throw BinException("Can't find kernel info name");
    return metadataInfo->kernels[it->second];
}

// if ROCm binary
bool CLRX::isROCmBinary(size_t binarySize, const cxbyte* binary)
{
    if (!isElfBinary(binarySize, binary))
        return false;
    if (binary[EI_CLASS] != ELFCLASS64)
        return false;
    const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binary);
    if (ULEV(ehdr->e_machine) != 0xe0)
        return false;
    return true;
}


void ROCmInput::addEmptyKernel(const char* kernelName)
{
    symbols.push_back({ kernelName, 0, 0, ROCmRegionType::KERNEL });
}

/*
 * ROCm YAML metadata generator
 */

static const char* rocmValueKindNames[] =
{
    "ByValue", "GlobalBuffer", "DynamicSharedPointer", "Sampler", "Image", "Pipe", "Queue",
    "HiddenGlobalOffsetX", "HiddenGlobalOffsetY", "HiddenGlobalOffsetZ", "HiddenNone",
    "HiddenPrintfBuffer", "HiddenDefaultQueue", "HiddenCompletionAction"
};

static const char* rocmValueTypeNames[] =
{
    "Struct", "I8", "U8", "I16", "U16", "F16", "I32", "U32", "F32", "I64", "U64", "F64"
};

static void genArrayValue(cxuint n, const cxuint* values, std::string& output)
{
    char numBuf[24];
    output += "[ ";
    for (cxuint i = 0; i < n; i++)
    {
        itocstrCStyle(values[i], numBuf, 24);
        output += numBuf;
        output += (i+1<n) ? ", " : " ]\n";
    }
}

// helper for checking whether value is supplied
static inline bool hasValue(cxuint value)
{ return value!=BINGEN_NOTSUPPLIED && value!=BINGEN_DEFAULT; }

static inline bool hasValue(uint64_t value)
{ return value!=BINGEN64_NOTSUPPLIED && value!=BINGEN64_DEFAULT; }

// get escaped YAML string if needed, otherwise get this same string
static std::string escapeYAMLString(const CString& input)
{
    bool toEscape = false;
    const char* s;
    for (s = input.c_str(); *s!=0; s++)
    {
        cxbyte c = *s;
        if (c < 0x20 || c >= 0x80 || c=='*' || c=='&' || c=='!' || c=='@' ||
            c=='\'' || c=='\"')
            toEscape = true;
    }
    // if spaces in begin and end
    if (isSpace(input[0]) || isDigit(input[0]) ||
        (!input.empty() && isSpace(s[-1])))
        toEscape = true;
    
    if (toEscape)
    {
        std::string out = "'";
        out += escapeStringCStyle(s-input.c_str(), input.c_str());
        out += "'";
        return out;
    }
    return input.c_str();
}

static std::string escapePrintfFormat(const std::string& fmt)
{
    std::string out;
    out.reserve(fmt.size());
    for (char c: fmt)
        if (c!=':')
            out.push_back(c);
        else
            out += "\\72";
    return out;
}

static void generateROCmMetadata(const ROCmMetadata& mdInfo,
                    const ROCmKernelConfig** kconfigs, std::string& output)
{
    output.clear();
    char numBuf[24];
    output += "---\n";
    // version
    output += "Version:         ";
    if (hasValue(mdInfo.version[0]))
        genArrayValue(2, mdInfo.version, output);
    else // default
        output += "[ 1, 0 ]\n";
    if (!mdInfo.printfInfos.empty())
        output += "Printf:          \n";
    // check print ids uniquness
    {
        std::unordered_set<cxuint> printfIds;
        for (const ROCmPrintfInfo& printfInfo: mdInfo.printfInfos)
            if (printfInfo.id!=BINGEN_DEFAULT)
                if (!printfIds.insert(printfInfo.id).second)
                    throw BinGenException("Duplicate of printf id");
        // printfs
        uint32_t freePrintfId = 1;
        for (const ROCmPrintfInfo& printfInfo: mdInfo.printfInfos)
        {
            // skip used printfids;
            uint32_t printfId = printfInfo.id;
            if (printfId == BINGEN_DEFAULT)
            {
                // skip used printfids
                for (; printfIds.find(freePrintfId) != printfIds.end(); ++freePrintfId);
                // just use this free printfid
                printfId = freePrintfId++;
            }
            
            output += "  - '";
            itocstrCStyle(printfId, numBuf, 24);
            output += numBuf;
            output += ':';
            itocstrCStyle(printfInfo.argSizes.size(), numBuf, 24);
            output += numBuf;
            output += ':';
            for (size_t argSize: printfInfo.argSizes)
            {
                itocstrCStyle(argSize, numBuf, 24);
                output += numBuf;
                output += ':';
            }
            // printf format
            std::string escapedFmt = escapeStringCStyle(printfInfo.format);
            escapedFmt = escapePrintfFormat(escapedFmt);
            output += escapedFmt;
            output += "'\n";
        }
    }
    
    if (!mdInfo.kernels.empty())
        output += "Kernels:         \n";
    // kernels
    for (size_t i = 0; i < mdInfo.kernels.size(); i++)
    {
        const ROCmKernelMetadata& kernel = mdInfo.kernels[i];
        output += "  - Name:            ";
        output.append(kernel.name.c_str(), kernel.name.size());
        output += "\n    SymbolName:      ";
        if (!kernel.symbolName.empty())
            output += escapeYAMLString(kernel.symbolName);
        else
        {
            // default is kernel name + '@kd'
            std::string symName = kernel.name.c_str();
            symName += "@kd";
            output += escapeYAMLString(symName);
        }
        output += "\n";
        if (!kernel.language.empty())
        {
            output += "    Language:        ";
            output += escapeYAMLString(kernel.language);
            output += "\n";
        }
        if (kernel.langVersion[0] != BINGEN_NOTSUPPLIED)
        {
            output += "    LanguageVersion: ";
            genArrayValue(2, kernel.langVersion, output);
        }
        // kernel attributes
        if (kernel.reqdWorkGroupSize[0] != 0 || kernel.reqdWorkGroupSize[1] != 0 ||
            kernel.reqdWorkGroupSize[2] != 0 ||
            kernel.workGroupSizeHint[0] != 0 || kernel.workGroupSizeHint[1] != 0 ||
            kernel.workGroupSizeHint[2] != 0 ||
            !kernel.vecTypeHint.empty() || !kernel.runtimeHandle.empty())
        {
            output += "    Attrs:           \n";
            if (kernel.workGroupSizeHint[0] != 0 || kernel.workGroupSizeHint[1] != 0 ||
                kernel.workGroupSizeHint[2] != 0)
            {
                output += "      WorkGroupSizeHint: ";
                genArrayValue(3, kernel.workGroupSizeHint, output);
            }
            if (kernel.reqdWorkGroupSize[0] != 0 || kernel.reqdWorkGroupSize[1] != 0 ||
                kernel.reqdWorkGroupSize[2] != 0)
            {
                output += "      ReqdWorkGroupSize: ";
                genArrayValue(3, kernel.reqdWorkGroupSize, output);
            }
            if (!kernel.vecTypeHint.empty())
            {
                output += "      VecTypeHint:     ";
                output += escapeYAMLString(kernel.vecTypeHint);
                output += "\n";
            }
            if (!kernel.runtimeHandle.empty())
            {
                output += "      RuntimeHandle:   ";
                output += escapeYAMLString(kernel.runtimeHandle);
                output += "\n";
            }
        }
        // kernel arguments
        if (!kernel.argInfos.empty())
            output += "    Args:            \n";
        for (const ROCmKernelArgInfo& argInfo: kernel.argInfos)
        {
            output += "      - ";
            if (!argInfo.name.empty())
            {
                output += "Name:            ";
                output += escapeYAMLString(argInfo.name);
                output += "\n        ";
            }
            if (!argInfo.typeName.empty())
            {
                output += "TypeName:        ";
                output += escapeYAMLString(argInfo.typeName);
                output += "\n        ";
            }
            output += "Size:            ";
            itocstrCStyle(argInfo.size, numBuf, 24);
            output += numBuf;
            output += "\n        Align:           ";
            itocstrCStyle(argInfo.align, numBuf, 24);
            output += numBuf;
            output += "\n        ValueKind:       ";
            
            if (argInfo.valueKind > ROCmValueKind::MAX_VALUE)
                throw BinGenException("Unknown ValueKind");
            output += rocmValueKindNames[cxuint(argInfo.valueKind)];
            
            if (argInfo.valueType > ROCmValueType::MAX_VALUE)
                throw BinGenException("Unknown ValueType");
            output += "\n        ValueType:       ";
            output += rocmValueTypeNames[cxuint(argInfo.valueType)];
            output += "\n";
            
            if (argInfo.valueKind == ROCmValueKind::DYN_SHARED_PTR)
            {
                output += "        PointeeAlign:    ";
                itocstrCStyle(argInfo.pointeeAlign, numBuf, 24);
                output += numBuf;
                output += "\n";
            }
            if (argInfo.valueKind == ROCmValueKind::DYN_SHARED_PTR ||
                argInfo.valueKind == ROCmValueKind::GLOBAL_BUFFER)
            {
                if (argInfo.addressSpace > ROCmAddressSpace::MAX_VALUE ||
                    argInfo.addressSpace == ROCmAddressSpace::NONE)
                    throw BinGenException("Unknown AddressSpace");
                output += "        AddrSpaceQual:   ";
                output += rocmAddrSpaceTypesTbl[cxuint(argInfo.addressSpace)-1];
                output += "\n";
            }
            if (argInfo.valueKind == ROCmValueKind::IMAGE ||
                argInfo.valueKind == ROCmValueKind::PIPE)
            {
                if (argInfo.accessQual> ROCmAccessQual::MAX_VALUE)
                    throw BinGenException("Unknown AccessQualifier");
                output += "        AccQual:         ";
                output += rocmAccessQualifierTbl[cxuint(argInfo.accessQual)];
                output += "\n";
            }
            if (argInfo.valueKind == ROCmValueKind::GLOBAL_BUFFER ||
                argInfo.valueKind == ROCmValueKind::IMAGE ||
                argInfo.valueKind == ROCmValueKind::PIPE)
            {
                if (argInfo.actualAccessQual> ROCmAccessQual::MAX_VALUE)
                    throw BinGenException("Unknown ActualAccessQualifier");
                output += "        ActualAccQual:   ";
                output += rocmAccessQualifierTbl[cxuint(argInfo.actualAccessQual)];
                output += "\n";
            }
            if (argInfo.isConst)
                output += "        IsConst:         true\n";
            if (argInfo.isRestrict)
                output += "        IsRestrict:      true\n";
            if (argInfo.isVolatile)
                output += "        IsVolatile:      true\n";
            if (argInfo.isPipe)
                output += "        IsPipe:          true\n";
        }
        
        // kernel code properties
        const ROCmKernelConfig& kconfig = *kconfigs[i];
        
        output += "    CodeProps:       \n";
        output += "      KernargSegmentSize: ";
        itocstrCStyle(hasValue(kernel.kernargSegmentSize) ?
                kernel.kernargSegmentSize : ULEV(kconfig.kernargSegmentSize),
                numBuf, 24);
        output += numBuf;
        output += "\n      GroupSegmentFixedSize: ";
        itocstrCStyle(hasValue(kernel.groupSegmentFixedSize) ?
                kernel.groupSegmentFixedSize :
                uint64_t(ULEV(kconfig.workgroupGroupSegmentSize)),
                numBuf, 24);
        output += numBuf;
        output += "\n      PrivateSegmentFixedSize: ";
        itocstrCStyle(hasValue(kernel.privateSegmentFixedSize) ?
                kernel.privateSegmentFixedSize :
                uint64_t(ULEV(kconfig.workitemPrivateSegmentSize)),
                numBuf, 24);
        output += numBuf;
        output += "\n      KernargSegmentAlign: ";
        itocstrCStyle(hasValue(kernel.kernargSegmentAlign) ?
                kernel.kernargSegmentAlign :
                uint64_t(1ULL<<kconfig.kernargSegmentAlignment),
                numBuf, 24);
        output += numBuf;
        output += "\n      WavefrontSize:   ";
        itocstrCStyle(hasValue(kernel.wavefrontSize) ? kernel.wavefrontSize :
                cxuint(1U<<kconfig.wavefrontSize), numBuf, 24);
        output += numBuf;
        output += "\n      NumSGPRs:        ";
        itocstrCStyle(hasValue(kernel.sgprsNum) ? kernel.sgprsNum :
                cxuint(ULEV(kconfig.wavefrontSgprCount)), numBuf, 24);
        output += numBuf;
        output += "\n      NumVGPRs:        ";
        itocstrCStyle(hasValue(kernel.vgprsNum) ? kernel.vgprsNum :
                cxuint(ULEV(kconfig.workitemVgprCount)), numBuf, 24);
        output += numBuf;
        // spilled registers
        if (hasValue(kernel.spilledSgprs))
        {
            output += "\n      NumSpilledSGPRs: ";
            itocstrCStyle(kernel.spilledSgprs, numBuf, 24);
            output += numBuf;
        }
        if (hasValue(kernel.spilledVgprs))
        {
            output += "\n      NumSpilledVGPRs: ";
            itocstrCStyle(kernel.spilledVgprs, numBuf, 24);
            output += numBuf;
        }
        output += "\n      MaxFlatWorkGroupSize: ";
        itocstrCStyle(hasValue(kernel.maxFlatWorkGroupSize) ?
                    kernel.maxFlatWorkGroupSize : uint64_t(256), numBuf, 24);
        output += numBuf;
        output += "\n";
        if (kernel.fixedWorkGroupSize[0] != 0 || kernel.fixedWorkGroupSize[1] != 0 ||
            kernel.fixedWorkGroupSize[2] != 0)
        {
            output += "      FixedWorkGroupSize:   ";
            genArrayValue(3, kernel.fixedWorkGroupSize, output);
        }
    }
    output += "...\n";
}

/* ROCm section generators */

class CLRX_INTERNAL ROCmGotGen: public ElfRegionContent
{
private:
    const ROCmInput* input;
public:
    explicit ROCmGotGen(const ROCmInput* _input) : input(_input)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        fob.fill(input->gotSymbols.size()*8, 0);
    }
};

class CLRX_INTERNAL ROCmRelaDynGen: public ElfRegionContent
{
private:
    size_t gotOffset;
    const ROCmInput* input;
public:
    explicit ROCmRelaDynGen(const ROCmInput* _input) : gotOffset(0), input(_input)
    { }
    
    void setGotOffset(size_t _gotOffset)
    { gotOffset = _gotOffset; }
    
    void operator()(FastOutputBuffer& fob) const
    {
        for (size_t i = 0; i < input->gotSymbols.size(); i++)
        {
            size_t symIndex = input->gotSymbols[i];
            Elf64_Rela rela{};
            SLEV(rela.r_offset, gotOffset + 8*i);
            SLEV(rela.r_info, ELF64_R_INFO(symIndex + 1, 3));
            rela.r_addend = 0;
            fob.writeObject(rela);
        }
    }
};

/*
 * ROCm Binary Generator
 */

ROCmBinGenerator::ROCmBinGenerator() : manageable(false), input(nullptr)
{ }

ROCmBinGenerator::ROCmBinGenerator(const ROCmInput* rocmInput)
        : manageable(false), input(rocmInput), rocmGotGen(nullptr), rocmRelaDynGen(nullptr)
{ }

ROCmBinGenerator::ROCmBinGenerator(GPUDeviceType deviceType,
        uint32_t archMinor, uint32_t archStepping, size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        const std::vector<ROCmSymbolInput>& symbols) :
        rocmGotGen(nullptr), rocmRelaDynGen(nullptr)
{
    std::unique_ptr<ROCmInput> _input(new ROCmInput{});
    _input->deviceType = deviceType;
    _input->archMinor = archMinor;
    _input->archStepping = archStepping;
    _input->eflags = 0;
    _input->newBinFormat = false;
    _input->globalDataSize = globalDataSize;
    _input->globalData = globalData;
    _input->symbols = symbols;
    _input->codeSize = codeSize;
    _input->code = code;
    _input->commentSize = 0;
    _input->comment = nullptr;
    _input->target = "";
    _input->targetTripple = "";
    _input->metadataSize = 0;
    _input->metadata = nullptr;
    _input->useMetadataInfo = false;
    _input->metadataInfo = ROCmMetadata{};
    input = _input.release();
}

ROCmBinGenerator::ROCmBinGenerator(GPUDeviceType deviceType,
        uint32_t archMinor, uint32_t archStepping, size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        std::vector<ROCmSymbolInput>&& symbols) :
        rocmGotGen(nullptr), rocmRelaDynGen(nullptr)
{
    std::unique_ptr<ROCmInput> _input(new ROCmInput{});
    _input->deviceType = deviceType;
    _input->archMinor = archMinor;
    _input->archStepping = archStepping;
    _input->eflags = 0;
    _input->newBinFormat = false;
    _input->globalDataSize = globalDataSize;
    _input->globalData = globalData;
    _input->symbols = std::move(symbols);
    _input->codeSize = codeSize;
    _input->code = code;
    _input->commentSize = 0;
    _input->comment = nullptr;
    _input->target = "";
    _input->targetTripple = "";
    _input->metadataSize = 0;
    _input->metadata = nullptr;
    _input->useMetadataInfo = false;
    _input->metadataInfo = ROCmMetadata{};
    input = _input.release();
}

ROCmBinGenerator::~ROCmBinGenerator()
{
    if (manageable)
        delete input;
    if (rocmGotGen!=nullptr)
        delete (ROCmGotGen*)rocmGotGen;
    if (rocmRelaDynGen!=nullptr)
        delete (ROCmRelaDynGen*)rocmRelaDynGen;
}

void ROCmBinGenerator::setInput(const ROCmInput* input)
{
    if (manageable)
        delete input;
    manageable = false;
    this->input = input;
}

// ELF notes contents
static const cxbyte noteDescType1[8] =
{ 2, 0, 0, 0, 1, 0, 0, 0 };

static const cxbyte noteDescType3[27] =
{ 4, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  'A', 'M', 'D', 0, 'A', 'M', 'D', 'G', 'P', 'U', 0 };

static inline void addMainSectionToTable(cxuint& sectionsNum, uint16_t* builtinTable,
                cxuint elfSectId)
{ builtinTable[elfSectId - ELFSECTID_START] = sectionsNum++; }

void ROCmBinGenerator::prepareBinaryGen()
{
    AMDGPUArchVersion amdGpuArchValues = getGPUArchVersion(input->deviceType,
                GPUArchVersionTable::ROCM);
    if (input->archMinor!=UINT32_MAX)
        amdGpuArchValues.minor = input->archMinor;
    if (input->archStepping!=UINT32_MAX)
        amdGpuArchValues.stepping = input->archStepping;
    
    comment = "CLRX ROCmBinGenerator " CLRX_VERSION;
    commentSize = ::strlen(comment);
    if (input->comment!=nullptr)
    {
        // if comment, store comment section
        comment = input->comment;
        commentSize = input->commentSize;
        if (commentSize==0)
            commentSize = ::strlen(comment);
    }
    
    uint32_t eflags = input->newBinFormat ? 2 : 0;
    if (input->eflags != BINGEN_DEFAULT)
        eflags = input->eflags;
    
    std::fill(mainBuiltinSectTable,
              mainBuiltinSectTable + ROCMSECTID_MAX-ELFSECTID_START+1, SHN_UNDEF);
    mainSectionsNum = 1;
    
    // generate main builtin section table (for section id translation)
    if (input->newBinFormat)
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_NOTE);
    if (input->globalData != nullptr)
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_RODATA);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_DYNSYM);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_HASH);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_DYNSTR);
    if (!input->gotSymbols.empty())
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_RELADYN);
    const cxuint execProgHeaderRegionIndex = mainSectionsNum;
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_TEXT);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_DYNAMIC);
    if (!input->gotSymbols.empty())
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_GOT);
    if (!input->newBinFormat)
    {
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_NOTE);
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_GPUCONFIG);
    }
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_COMMENT);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_SYMTAB);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_SHSTRTAB);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_STRTAB);
    
    elfBinGen64.reset(new ElfBinaryGen64({ 0U, 0U, 0x40, 0, ET_DYN, 0xe0, EV_CURRENT,
            cxuint(input->newBinFormat ? execProgHeaderRegionIndex : UINT_MAX), 0, eflags },
            true, true, true, PHREGION_FILESTART));
    
    static const int32_t dynTags[] = {
        DT_SYMTAB, DT_SYMENT, DT_STRTAB, DT_STRSZ, DT_HASH };
    elfBinGen64->addDynamics(sizeof(dynTags)/sizeof(int32_t), dynTags);
    
    // elf program headers
    elfBinGen64->addProgramHeader({ PT_PHDR, PF_R, 0, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64->addProgramHeader({ PT_LOAD, PF_R, PHREGION_FILESTART,
                    execProgHeaderRegionIndex,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 0x1000 });
    elfBinGen64->addProgramHeader({ PT_LOAD, PF_R|PF_X, execProgHeaderRegionIndex, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64->addProgramHeader({ PT_LOAD, PF_R|PF_W, execProgHeaderRegionIndex+1,
                    cxuint(1 + (!input->gotSymbols.empty())),
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64->addProgramHeader({ PT_DYNAMIC, PF_R|PF_W, execProgHeaderRegionIndex+1, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 8 });
    elfBinGen64->addProgramHeader({ PT_GNU_RELRO, PF_R, execProgHeaderRegionIndex+1,
                    cxuint(1 + (!input->gotSymbols.empty())),
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 1 });
    elfBinGen64->addProgramHeader({ PT_GNU_STACK, PF_R|PF_W, PHREGION_FILESTART, 0,
                    true, 0, 0, 0 });
    
    if (input->newBinFormat)
        // program header for note (new binary format)
        elfBinGen64->addProgramHeader({ PT_NOTE, PF_R, 1, 1, true,
                    Elf64Types::nobase, Elf64Types::nobase, 0, 4 });
    
    target = input->target.c_str();
    if (target.empty() && !input->targetTripple.empty())
    {
        target = input->targetTripple.c_str();
        char dbuf[20];
        snprintf(dbuf, 20, "-gfx%u%u%u", amdGpuArchValues.major, amdGpuArchValues.minor,
                 amdGpuArchValues.stepping);
        target += dbuf;
    }
    // elf notes
    elfBinGen64->addNote({"AMD", sizeof noteDescType1, noteDescType1, 1U});
    noteBuf.reset(new cxbyte[0x1b]);
    ::memcpy(noteBuf.get(), noteDescType3, 0x1b);
    SULEV(*(uint32_t*)(noteBuf.get()+4), amdGpuArchValues.major);
    SULEV(*(uint32_t*)(noteBuf.get()+8), amdGpuArchValues.minor);
    SULEV(*(uint32_t*)(noteBuf.get()+12), amdGpuArchValues.stepping);
    elfBinGen64->addNote({"AMD", 0x1b, noteBuf.get(), 3U});
    if (!target.empty())
        elfBinGen64->addNote({"AMD", target.size(), (const cxbyte*)target.c_str(), 0xbU});
    
    metadataSize = input->metadataSize;
    metadata = input->metadata;
    if (input->useMetadataInfo)
    {
        // generate ROCm metadata
        std::vector<std::pair<CString, size_t> > symbolIndices(input->symbols.size());
        // create sorted indices of symbols by its name
        for (size_t k = 0; k < input->symbols.size(); k++)
            symbolIndices[k] = std::make_pair(input->symbols[k].symbolName, k);
        mapSort(symbolIndices.begin(), symbolIndices.end());
        
        const size_t mdKernelsNum = input->metadataInfo.kernels.size();
        std::unique_ptr<const ROCmKernelConfig*[]> kernelConfigPtrs(
                new const ROCmKernelConfig*[mdKernelsNum]);
        // generate ROCm kernel config pointers
        for (size_t k = 0; k < mdKernelsNum; k++)
        {
            auto it = binaryMapFind(symbolIndices.begin(), symbolIndices.end(), 
                        input->metadataInfo.kernels[k].name);
            if (it == symbolIndices.end() ||
                (input->symbols[it->second].type != ROCmRegionType::FKERNEL &&
                 input->symbols[it->second].type != ROCmRegionType::KERNEL))
                throw BinGenException("Kernel in metadata doesn't exists in code");
            kernelConfigPtrs[k] = reinterpret_cast<const ROCmKernelConfig*>(
                        input->code + input->symbols[it->second].offset);
        }
        // just generate ROCm metadata from info
        generateROCmMetadata(input->metadataInfo, kernelConfigPtrs.get(), metadataStr);
        metadataSize = metadataStr.size();
        metadata = metadataStr.c_str();
    }
    
    if (metadataSize != 0)
        elfBinGen64->addNote({"AMD", metadataSize, (const cxbyte*)metadata, 0xaU});
    
    /// region and sections
    elfBinGen64->addRegion(ElfRegion64::programHeaderTable());
    if (input->newBinFormat)
        elfBinGen64->addRegion(ElfRegion64::noteSection());
    if (input->globalData != nullptr)
        elfBinGen64->addRegion(ElfRegion64(input->globalDataSize, input->globalData, 4,
                ".rodata", SHT_PROGBITS, SHF_ALLOC, 0, 0, Elf64Types::nobase));
    
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8,
                ".dynsym", SHT_DYNSYM, SHF_ALLOC, 0, BINGEN_DEFAULT, Elf64Types::nobase));
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 4,
                ".hash", SHT_HASH, SHF_ALLOC,
                mainBuiltinSectTable[ELFSECTID_DYNSYM-ELFSECTID_START], 0,
                Elf64Types::nobase));
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".dynstr", SHT_STRTAB,
                SHF_ALLOC, 0, 0, Elf64Types::nobase));
    if (!input->gotSymbols.empty())
    {
        ROCmRelaDynGen* sgen = new ROCmRelaDynGen(input);
        rocmRelaDynGen = (void*)sgen;
        elfBinGen64->addRegion(ElfRegion64(input->gotSymbols.size()*sizeof(Elf64_Rela),
                sgen, 8, ".rela.dyn", SHT_RELA, SHF_ALLOC,
                mainBuiltinSectTable[ELFSECTID_DYNSYM-ELFSECTID_START], 0,
                Elf64Types::nobase, sizeof(Elf64_Rela)));
    }
    // '.text' with alignment=4096
    elfBinGen64->addRegion(ElfRegion64(input->codeSize, (const cxbyte*)input->code, 
              0x1000, ".text", SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR, 0, 0,
              Elf64Types::nobase, 0, false, 256));
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 0x1000,
                ".dynamic", SHT_DYNAMIC, SHF_ALLOC|SHF_WRITE,
                mainBuiltinSectTable[ELFSECTID_DYNSTR-ELFSECTID_START], 0,
                Elf64Types::nobase, 0, false, 8));
    if (!input->gotSymbols.empty())
    {
        ROCmGotGen* sgen = new ROCmGotGen(input);
        rocmGotGen = (void*)sgen;
        elfBinGen64->addRegion(ElfRegion64(input->gotSymbols.size()*8, sgen,
                8, ".got", SHT_PROGBITS,
                SHF_ALLOC|SHF_WRITE, 0, 0, Elf64Types::nobase));
    }
    if (!input->newBinFormat)
    {
        elfBinGen64->addRegion(ElfRegion64::noteSection());
        elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1,
                    ".AMDGPU.config", SHT_PROGBITS, 0));
    }
    elfBinGen64->addRegion(ElfRegion64(commentSize, (const cxbyte*)comment, 1, ".comment",
              SHT_PROGBITS, SHF_MERGE|SHF_STRINGS, 0, 0, 0, 1));
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8,
                ".symtab", SHT_SYMTAB, 0, 0, BINGEN_DEFAULT));
    elfBinGen64->addRegion(ElfRegion64::shstrtabSection());
    elfBinGen64->addRegion(ElfRegion64::strtabSection());
    elfBinGen64->addRegion(ElfRegion64::sectionHeaderTable());
    
    /* extra sections */
    for (const BinSection& section: input->extraSections)
        elfBinGen64->addRegion(ElfRegion64(section, mainBuiltinSectTable,
                         ROCMSECTID_MAX, mainSectionsNum));
    updateSymbols();
    binarySize = elfBinGen64->countSize();
    
    if (rocmRelaDynGen != nullptr)
        ((ROCmRelaDynGen*)rocmRelaDynGen)->setGotOffset(
                elfBinGen64->getRegionOffset(
                        mainBuiltinSectTable[ROCMSECTID_GOT - ELFSECTID_START]));
}

void ROCmBinGenerator::updateSymbols()
{
    elfBinGen64->clearSymbols();
    elfBinGen64->clearDynSymbols();
    // add symbols (kernels, function kernels and data symbols)
    elfBinGen64->addSymbol(ElfSymbol64("_DYNAMIC",
                  mainBuiltinSectTable[ROCMSECTID_DYNAMIC-ELFSECTID_START],
                  ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE), STV_HIDDEN, true, 0, 0));
    const uint16_t textSectIndex = mainBuiltinSectTable[ELFSECTID_TEXT-ELFSECTID_START];
    for (const ROCmSymbolInput& symbol: input->symbols)
    {
        ElfSymbol64 elfsym;
        switch (symbol.type)
        {
            case ROCmRegionType::KERNEL:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), textSectIndex,
                      ELF64_ST_INFO(STB_GLOBAL, STT_GNU_IFUNC), 0, true,
                      symbol.offset, symbol.size);
                break;
            case ROCmRegionType::FKERNEL:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), textSectIndex,
                      ELF64_ST_INFO(STB_GLOBAL, STT_FUNC), 0, true,
                      symbol.offset, symbol.size);
                break;
            case ROCmRegionType::DATA:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), textSectIndex,
                      ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT), 0, true,
                      symbol.offset, symbol.size);
                break;
            default:
                break;
        }
        // add to symbols and dynamic symbols table
        elfBinGen64->addSymbol(elfsym);
        elfBinGen64->addDynSymbol(elfsym);
    }
    /* extra symbols */
    for (const BinSymbol& symbol: input->extraSymbols)
    {
        ElfSymbol64 sym(symbol, mainBuiltinSectTable,
                         ROCMSECTID_MAX, mainSectionsNum);
        elfBinGen64->addSymbol(sym);
        elfBinGen64->addDynSymbol(sym);
    }
}

void ROCmBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr)
{
    if (elfBinGen64 == nullptr)
        prepareBinaryGen();
    /****
     * prepare for write binary to output
     ****/
    std::unique_ptr<std::ostream> outStreamHolder;
    std::ostream* os = nullptr;
    if (aPtr != nullptr)
    {
        aPtr->resize(binarySize);
        outStreamHolder.reset(
                new ArrayOStream(binarySize, reinterpret_cast<char*>(aPtr->data())));
        os = outStreamHolder.get();
    }
    else if (vPtr != nullptr)
    {
        vPtr->resize(binarySize);
        outStreamHolder.reset(new VectorOStream(*vPtr));
        os = outStreamHolder.get();
    }
    else // from argument
        os = osPtr;
    
    const std::ios::iostate oldExceptions = os->exceptions();
    try
    {
    os->exceptions(std::ios::failbit | std::ios::badbit);
    /****
     * write binary to output
     ****/
    FastOutputBuffer bos(256, *os);
    elfBinGen64->generate(bos);
    assert(bos.getWritten() == binarySize);
    
    if (rocmGotGen != nullptr)
    {
        delete (ROCmGotGen*)rocmGotGen;
        rocmGotGen = nullptr;
    }
    if (rocmRelaDynGen != nullptr)
    {
        delete (ROCmGotGen*)rocmRelaDynGen;
        rocmRelaDynGen = nullptr;
    }
    }
    catch(...)
    {
        os->exceptions(oldExceptions);
        throw;
    }
    os->exceptions(oldExceptions);
}

void ROCmBinGenerator::generate(Array<cxbyte>& array)
{
    generateInternal(nullptr, nullptr, &array);
}

void ROCmBinGenerator::generate(std::ostream& os)
{
    generateInternal(&os, nullptr, nullptr);
}

void ROCmBinGenerator::generate(std::vector<char>& v)
{
    generateInternal(nullptr, &v, nullptr);
}
