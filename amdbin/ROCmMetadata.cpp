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
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <CLRX/utils/Utilities.h>
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
    { "HiddenMultiGridSyncArg", ROCmValueKind::HIDDEN_MULTIGRID_SYNC_ARG },
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

void CLRX::parseROCmMetadata(size_t metadataSize, const char* metadata,
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
                    for (; accIndex < 4; accIndex++)
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
                        if (::strcasecmp(rocmAddrSpaceTypesTbl[aspaceIndex],
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
 * ROCm metadata MsgPack parser
 */

static void parseMsgPackNil(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd || *dataPtr != 0xc0)
        throw ParseException("MsgPack: Can't parse nil value");
    dataPtr++;
}

static bool parseMsgPackBool(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd || ((*dataPtr)&0xfe) != 0xc2)
        throw ParseException("MsgPack: Can't parse bool value");
    const bool v = (*dataPtr==0xc3);
    dataPtr++;
    return v;
}

static uint64_t parseMsgPackInteger(const cxbyte*& dataPtr, const cxbyte* dataEnd,
                cxbyte signess = MSGPACK_WS_BOTH)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't parse integer value");
    uint64_t v = 0;
    if (*dataPtr < 0x80)
        v = *dataPtr++;
    else if (*dataPtr >= 0xe0)
    {
        v = uint64_t(-32) + ((*dataPtr++) & 0x1f);
        if (signess == MSGPACK_WS_UNSIGNED && v >= (1ULL<<63))
            throw ParseException("MsgPack: Negative value for unsigned integer");
    }
    else
    {
        const cxbyte code = *dataPtr++;
        switch(code)
        {
            case 0xcc:
            case 0xd0:
                if (dataPtr>=dataEnd)
                    throw ParseException("MsgPack: Can't parse integer value");
                if (code==0xcc)
                    v = *dataPtr++;
                else
                    v = int8_t(*dataPtr++);
                break;
            case 0xcd:
            case 0xd1:
                if (dataPtr+1>=dataEnd)
                    throw ParseException("MsgPack: Can't parse integer value");
                v = *dataPtr++;
                v |= uint16_t(*dataPtr++)<<8;
                if (code==0xd1 && (v&(1ULL<<15))!=0)
                    v |= (0xffffffffffffULL<<16);
                break;
            case 0xce:
            case 0xd2:
                if (dataPtr+3>=dataEnd)
                    throw ParseException("MsgPack: Can't parse integer value");
                for (cxuint i = 0; i < 32; i+=8)
                    v |= uint32_t(*dataPtr++)<<i;
                if (code==0xd2 && (v&(1ULL<<31))!=0)
                    v |= (0xffffffffULL<<32);
                break;
            case 0xcf:
            case 0xd3:
                if (dataPtr+7>=dataEnd)
                    throw ParseException("MsgPack: Can't parse integer value");
                for (cxuint i = 0; i < 64; i+=8)
                    v |= uint64_t(*dataPtr++)<<i;
                break;
            default:
                throw ParseException("MsgPack: Can't parse integer value");
        }
        
        if (signess == MSGPACK_WS_UNSIGNED && code >= 0xd0 && v >= (1ULL<<63))
            throw ParseException("MsgPack: Negative value for unsigned integer");
        if (signess == MSGPACK_WS_SIGNED && code < 0xd0 && v >= (1ULL<<63))
            throw ParseException("MsgPack: Positive value out of range for signed integer");
    }
    return v;
}

static double parseMsgPackFloat(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't parse float value");
    const cxbyte code = *dataPtr++;
    if (code == 0xca)
    {
        union {
            uint32_t v;
            float vf;
        } v;
        v.v = 0;
        if (dataPtr+3>=dataEnd)
            throw ParseException("MsgPack: Can't parse float value");
        for (cxuint i = 0; i < 32; i+=8)
            v.v |= uint32_t(*dataPtr++)<<i;
        return v.vf;
    }
    else if (code == 0xcb)
    {
        union {
            uint64_t v;
            double vf;
        } v;
        v.v = 0;
        if (dataPtr+7>=dataEnd)
            throw ParseException("MsgPack: Can't parse float value");
        for (cxuint i = 0; i < 64; i+=8)
            v.v |= uint64_t(*dataPtr++)<<i;
        return v.vf;
    }
    else
        throw ParseException("MsgPack: Can't parse float value");
}

static std::string parseMsgPackString(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't parse string");
    size_t size = 0;
    
    if ((*dataPtr&0xe0) == 0xa0)
        size = (*dataPtr++) & 0x1f;
    else
    {
        const cxbyte code = *dataPtr++;
        switch (code)
        {
            case 0xd9:
                if (dataPtr>=dataEnd)
                    throw ParseException("MsgPack: Can't parse string size");
                size = *dataPtr++;
                break;
            case 0xda:
                if (dataPtr+1>=dataEnd)
                    throw ParseException("MsgPack: Can't parse string size");
                size = *dataPtr++;
                size |= uint32_t(*dataPtr++)<<8;
                break;
            case 0xdb:
                if (dataPtr+3>=dataEnd)
                    throw ParseException("MsgPack: Can't parse string size");
                for (cxuint i = 0; i < 32; i+=8)
                    size |= uint32_t(*dataPtr++)<<i;
                break;
            default:
                throw ParseException("MsgPack: Can't parse string");
        }
    }
    
    if (dataPtr+size > dataEnd)
        throw ParseException("MsgPack: Can't parse string");
    const char* strData = reinterpret_cast<const char*>(dataPtr);
    std::string out(strData, strData + size);
    dataPtr += size;
    return out;
}

static Array<cxbyte> parseMsgPackData(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't parse byte-array");
    const cxbyte code = *dataPtr++;
    size_t size = 0;
    switch (code)
    {
        case 0xc4:
            if (dataPtr>=dataEnd)
                throw ParseException("MsgPack: Can't parse byte-array size");
            size = *dataPtr++;
            break;
        case 0xc5:
            if (dataPtr+1>=dataEnd)
                throw ParseException("MsgPack: Can't parse byte-array size");
            size = *dataPtr++;
            size |= uint32_t(*dataPtr++)<<8;
            break;
        case 0xc6:
            if (dataPtr+3>=dataEnd)
                throw ParseException("MsgPack: Can't parse byte-array size");
            for (cxuint i = 0; i < 32; i+=8)
                size |= uint32_t(*dataPtr++)<<i;
            break;
        default:
            throw ParseException("MsgPack: Can't parse byte-array");
    }
    
    if (dataPtr+size > dataEnd)
        throw ParseException("MsgPack: Can't parse byte-array");
    Array<cxbyte> out(dataPtr, dataPtr + size);
    dataPtr += size;
    return out;
}

static void skipMsgPackObject(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't skip object");
    if (*dataPtr==0xc0 || *dataPtr==0xc2 || *dataPtr==0xc3 ||
        *dataPtr < 0x80 || *dataPtr >= 0xe0)
        dataPtr++;
    else if (*dataPtr==0xcc || *dataPtr==0xd0)
    {
        if (dataPtr+1>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += 2;
    }
    else if (*dataPtr==0xcd || *dataPtr==0xd1)
    {
        if (dataPtr+2>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += 3;
    }
    else if (*dataPtr==0xce || *dataPtr==0xd2 || *dataPtr==0xca)
    {
        if (dataPtr+4>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += 5;
    }
    else if (*dataPtr==0xcf || *dataPtr==0xd3 || *dataPtr==0xcb)
    {
        if (dataPtr+8>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += 9;
    }
    else if(((*dataPtr)&0xe0)==0xa0)
    {
        const size_t size = *dataPtr&0x1f;
        if (dataPtr+size>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += size+1;
    }
    else if (*dataPtr == 0xc4 || *dataPtr == 0xd9)
    {
        dataPtr++;
        if (dataPtr>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        const size_t size = *dataPtr++;
        if (dataPtr+size>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += size;
    }
    else if (*dataPtr == 0xc5 || *dataPtr == 0xda)
    {
        dataPtr++;
        if (dataPtr+1>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        size_t size = *dataPtr++;
        size |= (*dataPtr++)<<8;
        if (dataPtr+size>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += size;
    }
    else if (*dataPtr == 0xc6 || *dataPtr == 0xdb)
    {
        dataPtr++;
        if (dataPtr+1>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        size_t size = 0;
        for (cxuint i = 0; i < 32; i+=8)
            size |= (*dataPtr++)<<i;
        if (dataPtr+size>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += size;
    }
    else if ((*dataPtr&0xf0) == 0x90 || (*dataPtr&0xf0) == 0x80)
    {
        const bool isMap = (*dataPtr<0x90);
        size_t size = (*dataPtr++)&15;
        if (isMap)
            size <<= 1;
        for (size_t i = 0; i < size; i++)
            skipMsgPackObject(dataPtr, dataEnd);
    }
    else if (*dataPtr == 0xdc || *dataPtr==0xde)
    {
        const bool isMap = (*dataPtr==0xde);
        dataPtr++;
        if (dataPtr>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        size_t size = *dataPtr++;
        size |= (*dataPtr++)<<8;
        if (dataPtr+size>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        if (isMap)
            size<<=1;
        for (size_t i = 0; i < size; i++)
            skipMsgPackObject(dataPtr, dataEnd);
    }
    else if (*dataPtr == 0xdd || *dataPtr==0xdf)
    {
        const bool isMap = (*dataPtr==0xdf);
        dataPtr++;
        if (dataPtr>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        size_t size = 0;
        for (cxuint i = 0; i < 32; i+=8)
            size |= (*dataPtr++)<<i;
        if (dataPtr+size>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        if (isMap)
            size<<=1;
        for (size_t i = 0; i < size; i++)
            skipMsgPackObject(dataPtr, dataEnd);
    }
}

//////////////////
MsgPackArrayParser::MsgPackArrayParser(const cxbyte*& _dataPtr, const cxbyte* _dataEnd)
        : dataPtr(_dataPtr), dataEnd(_dataEnd), count(0)
{
    if (dataPtr==dataEnd)
        throw ParseException("MsgPack: Can't parse array of elements");
    
    if (((*dataPtr) & 0xf0) == 0x90)
        count = (*dataPtr++) & 15;
    else
    {
        const cxbyte code = *dataPtr++;
        if (code == 0xdc)
        {
            if (dataPtr+1 >= dataEnd)
                throw ParseException("MsgPack: Can't parse array size");
            count = *dataPtr++;
            count |= (*dataPtr++)<<8;
        }
        else if (code == 0xdd)
        {
            if (dataPtr+3 >= dataEnd)
                throw ParseException("MsgPack: Can't parse array size");
            for (cxuint i = 0; i < 32; i+=8)
                count |= uint32_t(*dataPtr++)<<i;
        }
        else
            throw ParseException("MsgPack: Can't parse array of elements");
    }
}

void MsgPackArrayParser::handleErrors()
{
    if (count == 0)
        throw ParseException("MsgPack: No left element to parse");
}

void MsgPackArrayParser::parseNil()
{
    handleErrors();
    parseMsgPackNil(dataPtr, dataEnd);
    count--;
}

bool MsgPackArrayParser::parseBool()
{
    handleErrors();
    auto v = parseMsgPackBool(dataPtr, dataEnd);
    count--;
    return v;
}

uint64_t MsgPackArrayParser::parseInteger(cxbyte signess)
{
    handleErrors();
    auto v = parseMsgPackInteger(dataPtr, dataEnd, signess);
    count--;
    return v;
}

double MsgPackArrayParser::parseFloat()
{
    handleErrors();
    auto v = parseMsgPackFloat(dataPtr, dataEnd);
    count--;
    return v;
}

std::string MsgPackArrayParser::parseString()
{
    handleErrors();
    auto v = parseMsgPackString(dataPtr, dataEnd);
    count--;
    return v;
}

Array<cxbyte> MsgPackArrayParser::parseData()
{
    handleErrors();
    auto v = parseMsgPackData(dataPtr, dataEnd);
    count--;
    return v;
}

MsgPackArrayParser MsgPackArrayParser::parseArray()
{
    handleErrors();
    auto v = MsgPackArrayParser(dataPtr, dataEnd);
    count--;
    return v;
}

MsgPackMapParser MsgPackArrayParser::parseMap()
{
    handleErrors();
    auto v = MsgPackMapParser(dataPtr, dataEnd);
    count--;
    return v;
}

size_t MsgPackArrayParser::end()
{
    for (size_t i = 0; i < count; i++)
        skipMsgPackObject(dataPtr, dataEnd);
    return count;
}

//////////////////
MsgPackMapParser::MsgPackMapParser(const cxbyte*& _dataPtr, const cxbyte* _dataEnd)
        : dataPtr(_dataPtr), dataEnd(_dataEnd), count(0), keyLeft(true)
{
    if (dataPtr==dataEnd)
        throw ParseException("MsgPack: Can't parse map");
    
    if (((*dataPtr) & 0xf0) == 0x80)
        count = (*dataPtr++) & 15;
    else
    {
        const cxbyte code = *dataPtr++;
        if (code == 0xde)
        {
            if (dataPtr+1 >= dataEnd)
                throw ParseException("MsgPack: Can't parse map size");
            count = *dataPtr++;
            count |= (*dataPtr++)<<8;
        }
        else if (code == 0xdf)
        {
            if (dataPtr+3 >= dataEnd)
                throw ParseException("MsgPack: Can't parse map size");
            for (cxuint i = 0; i < 32; i+=8)
                count |= uint32_t(*dataPtr++)<<i;
        }
        else
            throw ParseException("MsgPack: Can't parse map");
    }
}

void MsgPackMapParser::handleErrors(bool key)
{
    if (count == 0)
        throw ParseException("MsgPack: No left element to parse");
    if (key && !keyLeft)
        throw ParseException("MsgPack: Key already parsed");
    if (!key && keyLeft)
        throw ParseException("MsgPack: This is not a value");
}

void MsgPackMapParser::parseKeyNil()
{
    handleErrors(true);
    parseMsgPackNil(dataPtr, dataEnd);
    keyLeft = false;
}

bool MsgPackMapParser::parseKeyBool()
{
    handleErrors(true);
    auto v = parseMsgPackBool(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

uint64_t MsgPackMapParser::parseKeyInteger(cxbyte signess)
{
    handleErrors(true);
    auto v = parseMsgPackInteger(dataPtr, dataEnd, signess);
    keyLeft = false;
    return v;
}

std::string MsgPackMapParser::parseKeyString()
{
    handleErrors(true);
    auto v = parseMsgPackString(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

Array<cxbyte> MsgPackMapParser::parseKeyData()
{
    handleErrors(true);
    auto v = parseMsgPackData(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

MsgPackArrayParser MsgPackMapParser::parseKeyArray()
{
    handleErrors(true);
    auto v = MsgPackArrayParser(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

MsgPackMapParser MsgPackMapParser::parseKeyMap()
{
    handleErrors(true);
    auto v = MsgPackMapParser(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

void MsgPackMapParser::parseValueNil()
{
    handleErrors(false);
    parseMsgPackNil(dataPtr, dataEnd);
    keyLeft = true;
    count--;
}

bool MsgPackMapParser::parseValueBool()
{
    handleErrors(false);
    auto v = parseMsgPackBool(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

uint64_t MsgPackMapParser::parseValueInteger(cxbyte signess)
{
    handleErrors(false);
    auto v = parseMsgPackInteger(dataPtr, dataEnd, signess);
    keyLeft = true;
    count--;
    return v;
}

std::string MsgPackMapParser::parseValueString()
{
    handleErrors(false);
    auto v = parseMsgPackString(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

Array<cxbyte> MsgPackMapParser::parseValueData()
{
    handleErrors(false);
    auto v = parseMsgPackData(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

MsgPackArrayParser MsgPackMapParser::parseValueArray()
{
    handleErrors(false);
    auto v = MsgPackArrayParser(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

MsgPackMapParser MsgPackMapParser::parseValueMap()
{
    handleErrors(false);
    auto v = MsgPackMapParser(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

void MsgPackMapParser::skipValue()
{
    handleErrors(false);
    skipMsgPackObject(dataPtr, dataEnd);
    keyLeft = true;
    count--;
}

size_t MsgPackMapParser::end()
{
    if (!keyLeft)
        skipMsgPackObject(dataPtr, dataEnd);
    for (size_t i = 0; i < count; i++)
    {
        skipMsgPackObject(dataPtr, dataEnd);
        skipMsgPackObject(dataPtr, dataEnd);
    }
    return count;
}

template<typename T>
static void parseMsgPackValueTypedArrayForMap(MsgPackMapParser& map, T* out,
                                    size_t elemsNum, cxbyte signess)
{
    MsgPackArrayParser arrParser = map.parseValueArray();
    for (size_t i = 0; i < elemsNum; i++)
        out[i] = arrParser.parseInteger(signess);
    if (arrParser.haveElements())
        throw ParseException("Typed Array has too many elements");
}

enum {
    ROCMMP_ARG_ACCESS = 0, ROCMMP_ARG_ACTUAL_ACCESS, ROCMMP_ARG_ADDRESS_SPACE,
    ROCMMP_ARG_IS_CONST, ROCMMP_ARG_IS_PIPE, ROCMMP_ARG_IS_RESTRICT,
    ROCMMP_ARG_IS_VOLATILE, ROCMMP_ARG_NAME, ROCMMP_ARG_OFFSET, ROCMMP_ARG_POINTEE_ALIGN,
    ROCMMP_ARG_SIZE, ROCMMP_ARG_TYPE_NAME, ROCMMP_ARG_VALUE_KIND, ROCMMP_ARG_VALUE_TYPE
};

static const char* rocmMetadataMPKernelArgNames[] =
{
    ".access", ".actual_access", ".address_space", ".is_const", ".is_pipe", ".is_restrict",
    ".is_volatile", ".name", ".offset", ".pointee_align", ".size", ".type_name",
    ".value_kind", ".value_type"
};

static const size_t rocmMetadataMPKernelArgNamesSize =
                sizeof(rocmMetadataMPKernelArgNames) / sizeof(const char*);

static const char* rocmMPAccessQualifierTbl[] =
{ "read_only", "write_only", "read_write" };

static void parseROCmMetadataKernelArgMsgPack(MsgPackArrayParser& argsParser,
                        ROCmKernelArgInfo& argInfo)
{
    MsgPackMapParser aParser = argsParser.parseMap();
    while (aParser.haveElements())
    {
        const std::string name = aParser.parseKeyString();
        const size_t index = binaryFind(rocmMetadataMPKernelArgNames,
                    rocmMetadataMPKernelArgNames + rocmMetadataMPKernelArgNamesSize,
                    name.c_str()) - rocmMetadataMPKernelArgNames;
        switch(index)
        {
            case ROCMMP_ARG_ACCESS:
            case ROCMMP_ARG_ACTUAL_ACCESS:
            {
                const std::string acc = trimStrSpaces(aParser.parseValueString());
                size_t accIndex = 0;
                for (; accIndex < 3; accIndex++)
                    if (::strcmp(rocmMPAccessQualifierTbl[accIndex], acc.c_str())==0)
                        break;
                if (accIndex == 3)
                    throw ParseException("Wrong access qualifier");
                if (index == ROCMMP_ARG_ACCESS)
                    argInfo.accessQual = ROCmAccessQual(accIndex+1);
                else
                    argInfo.actualAccessQual = ROCmAccessQual(accIndex+1);
                break;
            }
            case ROCMMP_ARG_ADDRESS_SPACE:
            {
                const std::string aspace = trimStrSpaces(aParser.parseValueString());
                size_t aspaceIndex = 0;
                for (; aspaceIndex < 6; aspaceIndex++)
                    if (::strcasecmp(rocmAddrSpaceTypesTbl[aspaceIndex],
                                aspace.c_str())==0)
                        break;
                if (aspaceIndex == 6)
                    throw ParseException("Wrong address space");
                argInfo.addressSpace = ROCmAddressSpace(aspaceIndex+1);
                break;
            }
            case ROCMMP_ARG_IS_CONST:
                argInfo.isConst = aParser.parseValueBool();
                break;
            case ROCMMP_ARG_IS_PIPE:
                argInfo.isPipe = aParser.parseValueBool();
                break;
            case ROCMMP_ARG_IS_RESTRICT:
                argInfo.isRestrict = aParser.parseValueBool();
                break;
            case ROCMMP_ARG_IS_VOLATILE:
                argInfo.isVolatile = aParser.parseValueBool();
                break;
            case ROCMMP_ARG_NAME:
                argInfo.name = aParser.parseValueString();
                break;
            case ROCMMP_ARG_OFFSET:
                argInfo.offset = aParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_ARG_POINTEE_ALIGN:
                argInfo.pointeeAlign = aParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_ARG_SIZE:
                argInfo.size = aParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_ARG_TYPE_NAME:
                argInfo.typeName = aParser.parseValueString();
                break;
            case ROCMMP_ARG_VALUE_KIND:
            {
                const std::string vkind = trimStrSpaces(aParser.parseValueString());
                const size_t vkindIndex = binaryMapFind(rocmValueKindNamesMap,
                            rocmValueKindNamesMap + rocmValueKindNamesNum, vkind.c_str(),
                            CStringLess()) - rocmValueKindNamesMap;
                    // if unknown kind
                    if (vkindIndex == rocmValueKindNamesNum)
                        throw ParseException("Wrong argument value kind");
                    argInfo.valueKind = rocmValueKindNamesMap[vkindIndex].second;
                break;
            }
            case ROCMMP_ARG_VALUE_TYPE:
            {
                const std::string vtype = trimStrSpaces(aParser.parseValueString());
                const size_t vtypeIndex = binaryMapFind(rocmValueTypeNamesMap,
                        rocmValueTypeNamesMap + rocmValueTypeNamesNum, vtype.c_str(),
                        CStringLess()) - rocmValueTypeNamesMap;
                // if unknown type
                if (vtypeIndex == rocmValueTypeNamesNum)
                    throw ParseException("Wrong argument value type");
                argInfo.valueType = rocmValueTypeNamesMap[vtypeIndex].second;
                break;
            }
            default:
                aParser.skipValue();
                break;
        }
    }
};

enum {
    ROCMMP_KERNEL_ARGS = 0, ROCMMP_KERNEL_DEVICE_ENQUEUE_SYMBOL,
    ROCMMP_KERNEL_GROUP_SEGMENT_FIXED_SIZE, ROCMMP_KERNEL_KERNARG_SEGMENT_ALIGN,
    ROCMMP_KERNEL_KERNARG_SEGMENT_SIZE, ROCMMP_KERNEL_LANGUAGE,
    ROCMMP_KERNEL_LANGUAGE_VERSION, ROCMMP_KERNEL_MAX_FLAT_WORKGROUP_SIZE,
    ROCMMP_KERNEL_NAME, ROCMMP_KERNEL_PRIVATE_SEGMENT_FIXED_SIZE,
    ROCMMP_KERNEL_REQD_WORKGROUP_SIZE, ROCMMP_KERNEL_SGPR_COUNT,
    ROCMMP_KERNEL_SGPR_SPILL_COUNT, ROCMMP_KERNEL_SYMBOL,
    ROCMMP_KERNEL_VEC_TYPE_HINT, ROCMMP_KERNEL_VGPR_COUNT,
    ROCMMP_KERNEL_VGPR_SPILL_COUNT, ROCMMP_KERNEL_WAVEFRONT_SIZE,
    ROCMMP_KERNEL_WORKGROUP_SIZE_HINT
};

static const char* rocmMetadataMPKernelNames[] =
{
    ".args", ".device_enqueue_symbol", ".group_segment_fixed_size", ".kernarg_segment_align",
    ".kernarg_segment_size", ".language", ".language_version", ".max_flat_workgroup_size",
    ".name", ".private_segment_fixed_size", ".reqd_workgroup_size", ".sgpr_count",
    ".sgpr_spill_count", ".symbol", ".vec_type_hint", ".vgpr_count", ".vgpr_spill_count",
    ".wavefront_size", ".workgroup_size_hint"
};

static const size_t rocmMetadataMPKernelNamesSize = sizeof(rocmMetadataMPKernelNames) /
                    sizeof(const char*);

static void parseROCmMetadataKernelMsgPack(MsgPackArrayParser& kernelsParser,
                        ROCmKernelMetadata& kernel)
{
    MsgPackMapParser kParser = kernelsParser.parseMap();
    while (kParser.haveElements())
    {
        const std::string name = kParser.parseKeyString();
        const size_t index = binaryFind(rocmMetadataMPKernelNames,
                    rocmMetadataMPKernelNames + rocmMetadataMPKernelNamesSize,
                    name.c_str()) - rocmMetadataMPKernelNames;
        
        switch(index)
        {
            case ROCMMP_KERNEL_ARGS:
            {
                MsgPackArrayParser argsParser = kParser.parseValueArray();
                while (argsParser.haveElements())
                {
                    ROCmKernelArgInfo arg{};
                    parseROCmMetadataKernelArgMsgPack(kernelsParser, arg);
                    kernel.argInfos.push_back(arg);
                }
                break;
            }
            case ROCMMP_KERNEL_DEVICE_ENQUEUE_SYMBOL:
                kernel.deviceEnqueueSymbol = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_GROUP_SEGMENT_FIXED_SIZE:
                kernel.groupSegmentFixedSize = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_KERNARG_SEGMENT_ALIGN:
                kernel.kernargSegmentAlign = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_KERNARG_SEGMENT_SIZE:
                kernel.kernargSegmentSize = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_LANGUAGE:
                kernel.language = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_LANGUAGE_VERSION:
                parseMsgPackValueTypedArrayForMap(kParser, kernel.langVersion,
                                        2, MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_MAX_FLAT_WORKGROUP_SIZE:
                kernel.maxFlatWorkGroupSize = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_NAME:
                kernel.name = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_PRIVATE_SEGMENT_FIXED_SIZE:
                kernel.privateSegmentFixedSize = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_REQD_WORKGROUP_SIZE:
                parseMsgPackValueTypedArrayForMap(kParser, kernel.reqdWorkGroupSize,
                                        3, MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_SGPR_COUNT:
                kernel.sgprsNum = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_SGPR_SPILL_COUNT:
                kernel.spilledSgprs = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_SYMBOL:
                kernel.symbolName = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_VEC_TYPE_HINT:
                kernel.vecTypeHint = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_VGPR_COUNT:
                kernel.vgprsNum = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_VGPR_SPILL_COUNT:
                kernel.spilledVgprs = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_WAVEFRONT_SIZE:
                kernel.wavefrontSize = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_WORKGROUP_SIZE_HINT:
                parseMsgPackValueTypedArrayForMap(kParser, kernel.workGroupSizeHint,
                                        3, MSGPACK_WS_UNSIGNED);
                break;
            default:
                kParser.skipValue();
                break;
        }
    }
}

void CLRX::parseROCmMetadataMsgPack(size_t metadataSize, const cxbyte* metadata,
                ROCmMetadata& metadataInfo)
{
    // init metadata info object
    metadataInfo.kernels.clear();
    metadataInfo.printfInfos.clear();
    metadataInfo.version[0] = metadataInfo.version[1] = 0;
    
    std::vector<ROCmKernelMetadata>& kernels = metadataInfo.kernels;
    
    MsgPackMapParser mainMap(metadata, metadata+metadataSize);
    while (mainMap.haveElements())
    {
        const CString name = mainMap.parseKeyString();
        if (name == "amdhsa.version")
            parseMsgPackValueTypedArrayForMap(mainMap, metadataInfo.version,
                                        2, MSGPACK_WS_UNSIGNED);
        else if (name == "amdhsa.kernels")
        {
            MsgPackArrayParser kernelsParser = mainMap.parseValueArray();
            while (kernelsParser.haveElements())
            {
                ROCmKernelMetadata kernel{};
                parseROCmMetadataKernelMsgPack(kernelsParser, kernel);
                kernels.push_back(kernel);
            }
        }
        else
            mainMap.skipValue();
    }
}

void ROCmMetadata::parseMsgPack(size_t metadataSize, const cxbyte* metadata)
{
    parseROCmMetadataMsgPack(metadataSize, metadata, *this);
}

/*
 * ROCm YAML metadata generator
 */

static const char* rocmValueKindNames[] =
{
    "ByValue", "GlobalBuffer", "DynamicSharedPointer", "Sampler", "Image", "Pipe", "Queue",
    "HiddenGlobalOffsetX", "HiddenGlobalOffsetY", "HiddenGlobalOffsetZ", "HiddenNone",
    "HiddenPrintfBuffer", "HiddenDefaultQueue", "HiddenCompletionAction",
    "HiddenMultiGridSyncArg"
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

void CLRX::generateROCmMetadata(const ROCmMetadata& mdInfo,
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
