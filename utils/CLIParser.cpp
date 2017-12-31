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
#include <algorithm>
#include <cstring>
#include <climits>
#include <cstddef>
#include <string>
#include <ostream>
#include <vector>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/CLIParser.h>

using namespace CLRX;

/* Command line exception */
CLIException::CLIException(const std::string& message) : Exception(message)
{ }

CLIException::CLIException(const std::string& message, char shortName)
{
    this->message = "Option '-";
    this->message += shortName;
    this->message += "': ";
    this->message += message;
}

CLIException::CLIException(const std::string& message,
               const std::string& longName)
{
    this->message = "Option '--";
    this->message += longName;
    this->message += "': ";
    this->message += message;
}

/// exception constructor designed to choosing between short or long option
CLIException::CLIException(const std::string& message, const CLIOption& option,
             bool chooseShortName)
{
    if (chooseShortName)
    {
        this->message = "Option '-";
        this->message += option.shortName;
    }
    else
    {
        this->message = "Option '--";
        this->message += option.longName;
    }
    this->message += "': ";
    this->message += message;
}

/* Command line parser class */

CLIParser::CLIParser(const char* _programName, const CLIOption* _options,
        cxuint _argc, const char** _argv) : options(_options), programName(_programName),
        packageName(nullptr), argc(_argc), argv(_argv)
{
    shortNameMap.reset(new cxuint[256]);
    std::fill(shortNameMap.get(), shortNameMap.get()+256, UINT_MAX); // fill as unused
    
    cxuint longNamesNum = 0;
    for (cxuint i = 0; options[i].longName != nullptr || options[i].shortName != 0; i++)
        if (options[i].longName != nullptr)
            longNamesNum++;
    
    longNameMap.resize(longNamesNum);
    cxuint i = 0;
    cxuint longNameCount = 0;
    for (; options[i].longName != nullptr || options[i].shortName != 0; i++)
    {
        if (options[i].shortName != 0)
        {
            if (options[i].shortName == '-' || cxuchar(options[i].shortName) < 0x20)
                throw CLIException("Illegal short name");
            if (shortNameMap[cxuchar(options[i].shortName)] != UINT_MAX)
                throw CLIException("Duplicate of option", options[i].shortName);
            shortNameMap[cxuchar(options[i].shortName)] = i;
        }
        // add long option to map
        if (options[i].longName != nullptr)
            longNameMap[longNameCount++] = std::make_pair(options[i].longName, i);
        // check conditions (illegal argument type, array occurrences only for arrays
        if ((options[i].argType > CLIArgType::SINGLE_MAX && 
             options[i].argType < CLIArgType::BOOL_ARRAY) ||
             options[i].argType > CLIArgType::ARRAY_MAX)
            throw CLIException("Illegal option argument type", options[i],
                       options[i].longName==nullptr);
        // checking whether arrayFromOccurrences defined for array argType
        if (options[i].arrayFromOccurrences &&
            options[i].argType <= CLIArgType::SINGLE_MAX)
            throw CLIException("Array of occurrences flag can be only for arrays");
    }
    optionEntries.resize(i); // resize to number of options
    
    mapSort(longNameMap.begin(), longNameMap.end(), CStringLess());
    for (cxuint i = 1; i < longNamesNum; i++)
        if (::strcmp(longNameMap[i].first, longNameMap[i-1].first) == 0)
            throw CLIException("Duplicate of option", longNameMap[i].first);
}

CLIParser::~CLIParser()
{
    for (cxuint i = 0; i < optionEntries.size(); i++)
    {   /* free optionEntry if it allocated some memory space */
        const OptionEntry& optEntry = optionEntries[i];
        if (!optEntry.isArg)
            continue;
        // array and trimmed string are own allocation, string just come from argument
        switch (options[i].argType)
        {
            case CLIArgType::TRIMMED_STRING:
                delete[] optEntry.v.s;
                break;
            case CLIArgType::BOOL_ARRAY:
                delete[] optEntry.v.bArr;
                break;
            case CLIArgType::UINT_ARRAY:
                delete[] optEntry.v.u32Arr;
                break;
            case CLIArgType::INT_ARRAY:
                delete[] optEntry.v.i32Arr;
                break;
            case CLIArgType::UINT64_ARRAY:
                delete[] optEntry.v.u64Arr;
                break;
            case CLIArgType::INT64_ARRAY:
                delete[] optEntry.v.i64Arr;
                break;
            case CLIArgType::SIZE_ARRAY:
                delete[] optEntry.v.sizeArr;
                break;
            case CLIArgType::FLOAT_ARRAY:
                delete[] optEntry.v.fArr;
                break;
            case CLIArgType::DOUBLE_ARRAY:
                delete[] optEntry.v.dArr;
                break;
            case CLIArgType::STRING_ARRAY:
            case CLIArgType::TRIMMED_STRING_ARRAY:
                for (size_t e = 0; e < optEntry.arrSize; e++)
                    delete[] optEntry.v.sArr[e];
                delete[] optEntry.v.sArr;
                break;
            default:
                break;
        }
    }
}

/* special internal routine to handle exceptions while getting argument from option.
 * (if argument was given, and if target argument type match to option argument type) */
void CLIParser::handleExceptionsForGetOptArg(cxuint optionId, CLIArgType argType) const
{
    if (optionId >= optionEntries.size())
        throw CLIException("No such command line option!");
    if (!optionEntries[optionId].isArg)
        throw CLIException("Command line option doesn't have argument!");
    const CLIArgType optArgType = options[optionId].argType;
    /// checking argument match (if argument type is equivalent of option arg type)
    if (argType != optArgType && !(
        (argType == CLIArgType::TRIMMED_STRING &&
            optArgType == CLIArgType::STRING) ||
        (argType == CLIArgType::STRING &&
            optArgType == CLIArgType::TRIMMED_STRING) ||
        (argType == CLIArgType::TRIMMED_STRING_ARRAY &&
            optArgType == CLIArgType::STRING_ARRAY) ||
        (argType == CLIArgType::STRING_ARRAY &&
            optArgType == CLIArgType::TRIMMED_STRING_ARRAY) ||
       /* size_t and integer types */
       (sizeof(size_t) == 4 &&
       ((argType == CLIArgType::UINT && optArgType == CLIArgType::SIZE) ||
       (argType == CLIArgType::SIZE && optArgType == CLIArgType::UINT) ||
       (argType == CLIArgType::UINT_ARRAY && optArgType == CLIArgType::SIZE_ARRAY) ||
       (argType == CLIArgType::SIZE_ARRAY && optArgType == CLIArgType::UINT_ARRAY))) ||
       /* for 64-bit values and 64-bit sizes */
       (sizeof(size_t) == 8 &&
       ((argType == CLIArgType::UINT64 && optArgType == CLIArgType::SIZE) ||
       (argType == CLIArgType::SIZE && optArgType == CLIArgType::UINT64) ||
       (argType == CLIArgType::UINT64_ARRAY && optArgType == CLIArgType::SIZE_ARRAY) ||
       (argType == CLIArgType::SIZE_ARRAY && optArgType == CLIArgType::UINT64_ARRAY)))))
        throw CLIException("Argument type of option mismatch!");
}

template<typename T>
static T* parseOptArgList(const char* optArg, size_t& arrSize)
{
    arrSize = 0;
    std::vector<T> vec;
    while (*optArg != 0)
    {
        optArg = skipSpaces(optArg);
        if (*optArg == 0) break;
        
        vec.push_back(cstrtovCStyle<T>(optArg, nullptr, optArg));
        
        optArg = skipSpaces(optArg);
        if (*optArg == ',')
            optArg++;
        else if (*optArg != 0)
            throw ParseException("Garbages at end of argument");
    }
    
    arrSize = vec.size();
    T* out = new T[arrSize];
    std::copy(vec.begin(), vec.end(), out);
    return out;
}

template<typename T>
static T parseOptArg(const char* optArg)
{
    optArg = skipSpaces(optArg);
    T value = cstrtovCStyle<T>(optArg, nullptr, optArg);
    optArg = skipSpaces(optArg);
    if (*optArg != 0)
        throw ParseException("Garbages at end of argument");
    return value;
}

// bool option arg parsing routine
static bool parseBoolOptArg(const char* optArg)
{
    optArg = skipSpaces(optArg);
    bool value = false;
    for (const char* v: { "1", "true", "t", "on", "yes", "y"})
    {
        const size_t vlen = ::strlen(v);
        if (::strncasecmp(optArg, v, vlen) == 0)
        {
            optArg += vlen;
            value = true;
            break;
        }
    }
    if (!value)
    {
        // if value true is not parsed, we try to parse false value
        bool isFalse = false;
        for (const char* v: { "0", "false", "f", "off", "no", "n"})
        {
            const size_t vlen = ::strlen(v);
            if (::strncasecmp(optArg, v, vlen) == 0)
            {
                optArg += vlen;
                isFalse = true;
                break;
            }
        }
        if (!isFalse)
            throw ParseException("Can't parse bool argument for option");
    }
    optArg = skipSpaces(optArg);
    if (*optArg != 0)
        throw ParseException("Garbages at end of argument");
    return value;
}

// parses option's argument, choosenShortName - option was called from short name
void CLIParser::parseOptionArg(cxuint optionId, const char* optArg, bool chooseShortName)
{
    const CLIOption& option = options[optionId];
    OptionEntry& optEntry = optionEntries[optionId];
    
    switch(option.argType)
    {
        case CLIArgType::BOOL:
            try
            { optEntry.v.b = parseBoolOptArg(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option, chooseShortName); }
            break;
        case CLIArgType::UINT:
            try
            { optEntry.v.u32 = parseOptArg<uint32_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option, chooseShortName); }
            break;
        case CLIArgType::INT:
            try
            { optEntry.v.i32 = parseOptArg<int32_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option, chooseShortName); }
            break;
        case CLIArgType::UINT64:
            try
            { optEntry.v.u64 = parseOptArg<uint64_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option, chooseShortName); }
            break;
        case CLIArgType::INT64:
            try
            { optEntry.v.i64 = parseOptArg<int64_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option, chooseShortName); }
            break;
        case CLIArgType::SIZE:
            try
            { optEntry.v.size = parseOptArg<size_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option, chooseShortName); }
            break;
        case CLIArgType::FLOAT:
            try
            { optEntry.v.f = parseOptArg<float>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option, chooseShortName); }
            break;
        case CLIArgType::DOUBLE:
            try
            { optEntry.v.d = parseOptArg<double>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option, chooseShortName); }
            break;
        case CLIArgType::TRIMMED_STRING:
        {
            optArg = skipSpaces(optArg);
            const char* end;
            if (*optArg != 0)
                end = skipSpacesAtEnd(optArg, ::strlen(optArg));
            else
                end = optArg;
            char* newStr = new char[end-optArg+1];
            std::copy(optArg, end, newStr);
            newStr[end-optArg] = 0;
            delete[] optEntry.v.s;
            optEntry.v.s = newStr;
            break;
        }
        case CLIArgType::STRING:
            optEntry.v.s = optArg;
            break;
        case CLIArgType::BOOL_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                // parse array from single argument
                if (optEntry.v.bArr != nullptr)
                optEntry.arrSize = 0;
                { delete[] optEntry.v.bArr; optEntry.v.bArr = nullptr; }
                std::vector<bool> bVec;
                while (*optArg != 0)
                {
                    optArg = skipSpaces(optArg);
                    if (*optArg == 0) break;
                    
                    const char* oldOptArg = optArg;
                    bool entryVal = false;
                    for (const char* v: { "1", "true", "t", "on", "yes", "y"})
                    {
                        const size_t vlen = ::strlen(v);
                        if (::strncasecmp(optArg, v, vlen) == 0)
                        {
                            optArg += vlen;
                            entryVal = true;
                            break;
                        }
                    }
                    // if value true is not parsed, we try to parse false value
                    if (!entryVal)
                    {
                        bool isFalse = false;
                        for (const char* v: { "0", "false", "f", "off", "no", "n"})
                        {
                            const size_t vlen = ::strlen(v);
                            if (::strncasecmp(optArg, v, vlen) == 0)
                            {
                                optArg += vlen;
                                isFalse = true;
                                break;
                            }
                        }
                        
                        if (!isFalse)
                            throw CLIException(
                                "Can't parse array of boolean argument for option",
                                       option, chooseShortName);
                    }
                    bVec.push_back(entryVal);
                    
                    const char* afterVal = optArg;
                    /* is not single character */
                    optArg = skipSpaces(optArg);
                    if (*optArg == ',')
                        optArg++;
                    else if (*optArg != 0 && oldOptArg+1 != afterVal)
                        throw CLIException("Garbages at end of argument",
                               option, chooseShortName);
                }
                // copy to option entry
                optEntry.v.bArr = new bool[bVec.size()];
                optEntry.arrSize = bVec.size();
                std::copy(bVec.begin(), bVec.end(), optEntry.v.bArr);
            }
            else
            {
                bool value;
                try
                { value = parseBoolOptArg(optArg); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const bool* oldArr = optEntry.v.bArr;
                    optEntry.v.bArr = new bool[optEntry.arrSize+8];
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.bArr);
                    delete[] oldArr;
                }
                optEntry.v.bArr[optEntry.arrSize++] = value;
            }
            break;
        case CLIArgType::UINT_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                // parse array from single argument
                if (optEntry.v.u32Arr != nullptr)
                { delete[] optEntry.v.u32Arr; optEntry.v.u32Arr = nullptr; }
                try
                {
                    optEntry.v.u32Arr = parseOptArgList<uint32_t>(
                            optArg, optEntry.arrSize);
                }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
            }
            else
            {
                uint32_t value;
                try
                { value = parseOptArg<uint32_t>(optArg); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const uint32_t* oldArr = optEntry.v.u32Arr;
                    optEntry.v.u32Arr = new uint32_t[optEntry.arrSize+8];
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.u32Arr);
                    delete[] oldArr;
                }
                optEntry.v.u32Arr[optEntry.arrSize++] = value;
            }
            break;
        case CLIArgType::INT_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                if (optEntry.v.i32Arr != nullptr)
                { delete[] optEntry.v.i32Arr; optEntry.v.i32Arr = nullptr; }
                try
                {
                    optEntry.v.i32Arr = parseOptArgList<int32_t>(
                            optArg, optEntry.arrSize);
                }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
            }
            else
            {
                int32_t value;
                try
                { value = parseOptArg<int32_t>(optArg); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const int32_t* oldArr = optEntry.v.i32Arr;
                    optEntry.v.i32Arr = new int32_t[optEntry.arrSize+8];
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.i32Arr);
                    delete[] oldArr;
                }
                optEntry.v.i32Arr[optEntry.arrSize++] = value;
            }
            break;
        case CLIArgType::UINT64_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                if (optEntry.v.u64Arr != nullptr)
                { delete[] optEntry.v.u64Arr; optEntry.v.u64Arr = nullptr; }
                try
                {
                    optEntry.v.u64Arr = parseOptArgList<uint64_t>(
                                optArg, optEntry.arrSize);
                }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
            }
            else
            {
                uint64_t value;
                try
                { value = parseOptArg<uint64_t>(optArg); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const uint64_t* oldArr = optEntry.v.u64Arr;
                    optEntry.v.u64Arr = new uint64_t[optEntry.arrSize+8];
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.u64Arr);
                    delete[] oldArr;
                }
                optEntry.v.u64Arr[optEntry.arrSize++] = value;
            }
            break;
        case CLIArgType::INT64_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                if (optEntry.v.i64Arr != nullptr)
                { delete[] optEntry.v.i64Arr; optEntry.v.i64Arr = nullptr; }
                try
                {
                    optEntry.v.i64Arr = parseOptArgList<int64_t>(
                                optArg, optEntry.arrSize);
                }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
            }
            else
            {
                int64_t value;
                try
                { value = parseOptArg<int64_t>(optArg); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const int64_t* oldArr = optEntry.v.i64Arr;
                    optEntry.v.i64Arr = new int64_t[optEntry.arrSize+8];
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.i64Arr);
                    delete[] oldArr;
                }
                optEntry.v.i64Arr[optEntry.arrSize++] = value;
            }
            break;
        case CLIArgType::SIZE_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                if (optEntry.v.sizeArr != nullptr)
                { delete[] optEntry.v.sizeArr; optEntry.v.sizeArr = nullptr; }
                try
                {
                    optEntry.v.sizeArr = parseOptArgList<size_t>(
                            optArg, optEntry.arrSize);
                }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
            }
            else
            {
                size_t value;
                try
                { value = parseOptArg<size_t>(optArg); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const size_t* oldArr = optEntry.v.sizeArr;
                    optEntry.v.sizeArr = new size_t[optEntry.arrSize+8];
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.sizeArr);
                    delete[] oldArr;
                }
                optEntry.v.sizeArr[optEntry.arrSize++] = value;
            }
            break;
        case CLIArgType::FLOAT_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                if (optEntry.v.fArr != nullptr)
                { delete[] optEntry.v.fArr; optEntry.v.fArr = nullptr; }
                try
                {
                    optEntry.v.fArr = parseOptArgList<float>(
                            optArg, optEntry.arrSize);
                }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
            }
            else
            {
                float value;
                try
                { value = parseOptArg<float>(optArg); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const float* oldArr = optEntry.v.fArr;
                    optEntry.v.fArr = new float[optEntry.arrSize+8];
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.fArr);
                    delete[] oldArr;
                }
                optEntry.v.fArr[optEntry.arrSize++] = value;
            }
            break;
        case CLIArgType::DOUBLE_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                if (optEntry.v.dArr != nullptr)
                { delete[] optEntry.v.dArr; optEntry.v.dArr = nullptr; }
                try
                { optEntry.v.dArr = parseOptArgList<double>(optArg, optEntry.arrSize); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
            }
            else
            {
                double value;
                try
                { value = parseOptArg<double>(optArg); }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const double* oldArr = optEntry.v.dArr;
                    optEntry.v.dArr = new double[optEntry.arrSize+8];
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.dArr);
                    delete[] oldArr;
                }
                optEntry.v.dArr[optEntry.arrSize++] = value;
            }
            break;
        case CLIArgType::STRING_ARRAY:
        case CLIArgType::TRIMMED_STRING_ARRAY:
            if (!option.arrayFromOccurrences)
            {
                // parse array from single argument
                if (optEntry.v.sArr != nullptr)
                {
                    for (size_t k = 0; k < optEntry.arrSize; k++)
                        delete[] optEntry.v.sArr[k];
                    delete[] optEntry.v.sArr;
                    optEntry.v.sArr = nullptr;
                }
                
                optEntry.arrSize = 0;
                std::vector<char*> sVec;
                const bool doTrim = (option.argType == CLIArgType::TRIMMED_STRING_ARRAY);
                
                try
                {
                while (true)
                {
                    if (doTrim)
                        optArg = skipSpaces(optArg);
                    
                    size_t elemLength = 0;
                    const char* elemEnd;
                    for (elemEnd = optArg, elemLength = 0;
                         *elemEnd != ',' && *elemEnd != 0; ++elemEnd, ++elemLength)
                         if (*elemEnd == '\\')
                         {
                             if (elemEnd[1] == ',' || elemEnd[1] == '\\')
                                 elemEnd++; // escape of ','
                             else // bad escape (accepted at ',' and '\\'
                                 throw CLIException("Incorrect escape of string",
                                            option, chooseShortName);
                         }
                    if (doTrim)
                    {
                        const char* trimmedEnd = skipSpacesAtEnd(optArg, elemEnd-optArg);
                        elemLength += trimmedEnd-elemEnd; // subtract length to trimmed
                    }
                    
                    char* strElem = new char[elemLength+1];
                    // copy element with escaping ',' and '\\'
                    for (size_t l = 0; l < elemLength; ++optArg, ++l)
                        if (*optArg == '\\' && (optArg[1] == ',' || optArg[1] == '\\'))
                        {
                            ++optArg;
                            strElem[l] = *optArg;
                        }
                        else
                            strElem[l] = *optArg;
                    
                    optArg = elemEnd;
                    strElem[elemLength] = 0;
                    sVec.push_back(strElem);
                    
                    if (*optArg == ',')
                        optArg++;
                    else if (*optArg != 0)
                        throw CLIException("Garbages at end of argument", option,
                                   chooseShortName);
                    else
                        break;
                }
                optEntry.v.sArr = new const char*[sVec.size()];
                optEntry.arrSize = sVec.size();
                std::copy(sVec.begin(), sVec.end(), optEntry.v.sArr);
                }
                catch(...)
                {
                    // delete previously parsed strings
                    for (const char* v: sVec)
                        delete[] v;
                    throw;
                }
            }
            else
            {
                char* value = nullptr;
                try
                {
                    if (option.argType == CLIArgType::TRIMMED_STRING_ARRAY)
                    {
                        // trimmed string
                        optArg = skipSpaces(optArg);
                        const char* end;
                        if (*optArg != 0)
                            end = skipSpacesAtEnd(optArg, ::strlen(optArg));
                        else
                            end = optArg;
                        value = new char[end-optArg+1];
                        std::copy(optArg, end, value);
                        value[end-optArg] = 0;
                    }
                    else
                    {
                        // normal string
                        size_t length = ::strlen(optArg)+1;
                        value = new char[length+1];
                        ::std::copy(optArg, optArg + length+1, value);
                    }
                }
                catch(const ParseException& ex)
                { throw CLIException(ex.what(), option, chooseShortName); }
                
                // resize array for every 8th element
                if ((optEntry.arrSize&7) == 0)
                {
                    const char** oldArr = optEntry.v.sArr;
                    try
                    { optEntry.v.sArr = new const char*[optEntry.arrSize+8]; }
                    catch(...)
                    {
                        delete[] value;
                        throw;
                    }
                    std::copy(oldArr, oldArr + optEntry.arrSize, optEntry.v.sArr);
                    delete[] oldArr;
                }
                optEntry.v.sArr[optEntry.arrSize++] = value;
            }
            break;
        default:
            throw CLIException("Unknown option argument type", option, chooseShortName);
            break;
    }
    optEntry.isArg = true;
}

void CLIParser::parse()
{
    /* parse args */
    bool isLeftOver = false;
    for (cxuint i = 1; i < argc; i++)
    {
        if (argv[i] == nullptr)
            throw CLIException("Null argument!");
        const char* arg = argv[i];
        if (!isLeftOver && arg[0] == '-' && arg[1] != 0)
        {
            if (arg[1] == '-')
            {
                // longNames
                if (arg[2] == 0)
                {
                    // if '--'
                    isLeftOver = true;
                    continue;
                }
                /* find option from long name */
                LongNameMap::const_iterator it = longNameMap.end();
                
                const char* lastEq = arg+::strlen(arg);
                size_t optLongNameLen = lastEq-arg-2;
                CString curArgStr;
                /// we try to match longest option name that end from last '='
                /// in next iterations, we check option name that ends from previous '='
                while(lastEq != arg+1)
                {
                    optLongNameLen = lastEq-arg-2;
                    curArgStr.assign(arg+2, optLongNameLen);
                    it = binaryMapFind(longNameMap.begin(), longNameMap.end(),
                            curArgStr.c_str(), CStringLess());
                    if (it != longNameMap.end())
                        break;
                    
                    lastEq--;
                    while (lastEq != arg+1 && *lastEq!='=') lastEq--;
                }
                
                if (it == longNameMap.end()) // unknown option
                    throw CLIException("Unknown command line option", arg+2);
                
                const cxuint optionId = it->second;
                const CLIOption& option = options[optionId];
                OptionEntry& optionEntry =  optionEntries[optionId];
                optionEntry.isSet = true;
                if (option.argType != CLIArgType::NONE)
                {
                    const char* optArg = nullptr;
                    if (arg[optLongNameLen+2] == '=')
                        // if option arg in this same arg elem
                        optArg = arg+optLongNameLen+3;
                    else if (i+1 < argc)
                    {
                        if (argv[i+1] == nullptr)
                            throw CLIException("Null argument!");
                        // if next argument is not begin from '-' or only is '-' or
                        // option argument is not optional we get optarg from next arg
                        if (argv[i+1][0] != '-' || argv[i+1][1] == 0 ||
                            !option.argIsOptional)
                            optArg = argv[++i]; // next elem as argument
                    }
                    else if (!option.argIsOptional)
                        throw CLIException("A missing argument", option.longName);
                    
                    if (optArg != nullptr) /* parse option argument */
                        parseOptionArg(optionId, optArg, false);
                }
            }
            else
            {
                // short names
                for (arg++; *arg != 0; arg++)
                {
                    const cxuint optionId = shortNameMap[cxuchar(*arg)];
                    if (optionId != UINT_MAX)
                    {
                        const CLIOption& option = options[optionId];
                        OptionEntry& optionEntry =  optionEntries[optionId];
                        optionEntry.isSet = true;
                        
                        if (option.argType != CLIArgType::NONE)
                        {
                            const char* optArg = nullptr;
                            if (arg[1] == '=')
                                optArg = arg+2; // if after '='
                            else if (arg[1] != 0) // if option arg in this same arg elem
                                optArg = arg+1;
                            else if (i+1 < argc)
                            {
                                if (argv[i+1] == nullptr)
                                    throw CLIException("Null argument!");
                                // refer to long names parsing
                                if (argv[i+1][0] != '-' || argv[i+1][1] == 0 ||
                                    !option.argIsOptional)
                                    optArg = argv[++i]; // next elem as argument
                            }
                            else if (!option.argIsOptional)
                                throw CLIException("A missing argument", option.shortName);
                            
                            if (optArg != nullptr) /* parse option argument */
                                parseOptionArg(optionId, optArg, true);
                            break; // end break parsing
                        }
                    }
                    else // if option is not known
                        throw CLIException("Unknown command line option", *arg);
                }
            }
        }
        else // left over args
            leftOverArgs.push_back(arg);
    }
    leftOverArgs.push_back(nullptr);
}

cxuint CLIParser::findOption(char shortName) const
{
    cxuint optionId = shortNameMap[cxuchar(shortName)];
    if (optionId == UINT_MAX)
        throw CLIException("Option not found", shortName);
    return optionId;
}

cxuint CLIParser::findOption(const char* longName) const
{
    const auto it = binaryMapFind(longNameMap.begin(), longNameMap.end(), longName,
                   CStringLess());
    if (it == longNameMap.end())
        throw CLIException("Option not found", longName);
    return it->second;
}

bool CLIParser::handleHelpOrUsage(std::ostream& os) const
{
    if (hasOption(findOption("help")))
    {
        printHelp(os);
        return true;
    }
    if (hasOption(findOption("usage")))
    {
        printUsage(os);
        return true;
    }
    if (hasOption(findOption("version")))
    {
        printVersion();
        return true;
    }
    return false;
}

void CLIParser::printHelp(std::ostream& os) const
{
    const std::ios::iostate oldExceptions = os.exceptions();
    os.exceptions(std::ios::failbit | std::ios::badbit);
    try
    {
    size_t maxLen = 0;
    for (cxuint i = 0; i < optionEntries.size(); i++)
    {
        const CLIOption& option = options[i];
        size_t colLength = 4;
        if (option.longName != nullptr)
            colLength += ::strlen(option.longName)+4;
        
        if (option.argType != CLIArgType::NONE)
        {
            colLength++; // '='
            if (option.argName!=nullptr)
                colLength += ::strlen(option.argName);
            else
                colLength += 3; // ARG
            if (option.argIsOptional)
                colLength += 2; // quadratic brackets
        }
        maxLen = std::max(maxLen, colLength);
    }
    
    os << "Usage: " << programName << " [OPTION...]\n";
    /* print all options */
    std::string optColumn;
    optColumn.reserve(maxLen+3);
    
    for (cxuint i = 0; i < optionEntries.size(); i++)
    {
        const CLIOption& option = options[i];
        if (option.shortName != 0)
        {
            optColumn = "  -";
            optColumn += option.shortName;
        }
        else
            optColumn = "    ";
        
        if (option.longName != nullptr)
        {
            optColumn += ((option.shortName!=0)?", --":"  --");
            optColumn += option.longName;
        }
        
        if (option.argType != CLIArgType::NONE)
        {
            optColumn += '=';
            if (option.argName!=nullptr)
            {
                if (option.argIsOptional)
                    optColumn += '[';
                optColumn += option.argName;
                if (option.argIsOptional)
                    optColumn += ']';
            }
            else
                optColumn += ((option.argIsOptional)?"[ARG]":"ARG");
        }
        
        // align to next column (description)
        for (size_t k = optColumn.size(); k < maxLen+2; k++)
            optColumn += ' ';
        
        os << optColumn;
        
        if (option.description != nullptr)
            os << option.description;
        os << '\n';
    }
    
    os.flush();
    } /* try catch */
    catch(...)
    {
        os.exceptions(oldExceptions);
        throw;
    }
    os.exceptions(oldExceptions);
}

void CLIParser::printUsage(std::ostream& os) const
{
    const std::ios::iostate oldExceptions = os.exceptions();
    os.exceptions(std::ios::failbit | std::ios::badbit);
    try
    {
    os << "Usage: " << programName;
    for (cxuint i = 0; i < optionEntries.size(); i++)
    {
        const CLIOption& option = options[i];
        os << " [";
        if (option.shortName != 0)
            os << '-' << option.shortName;
        if (option.longName != nullptr)
        {
            if (option.shortName != 0)
                os << '|';
            os << "--" << option.longName;
        }
        if (option.argType != CLIArgType::NONE)
        {
            if (option.argIsOptional)
                os << '[';
            os << '=' << ((option.argName!=nullptr) ? option.argName : "ARG");
            if (option.argIsOptional)
                os << ']';
        }
        os << ']';
    }
    os << " ..." << std::endl;
    }
    catch(...)
    {
        os.exceptions(oldExceptions);
        throw;
    }
    os.exceptions(oldExceptions);
}

void CLIParser::printVersion(std::ostream& os) const
{
    const std::ios::iostate oldExceptions = os.exceptions();
    os.exceptions(std::ios::failbit | std::ios::badbit);
    try
    {
        const char* pkgName = (packageName!=nullptr) ? packageName : "CLRadeonExtender";
        os << programName << " (" << pkgName << ") " << CLRX_VERSION << "\n";
    }
    catch(...)
    {
        os.exceptions(oldExceptions);
        throw;
    }
    os.exceptions(oldExceptions);
}
