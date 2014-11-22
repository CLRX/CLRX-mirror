/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
#include <map>
#include <vector>
#include <CLRX/Utilities.h>
#include <CLRX/CLIParser.h>

using namespace CLRX;

/* Command line exception */
CLIException::CLIException(const std::string& message)
{
    this->message = message;
}

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

CLIException::CLIException(const std::string& message, char shortName,
           const std::string& longName, bool chooseShortName)
{
    if (chooseShortName)
    {
        this->message = "Option '-";
        this->message += shortName;
    }
    else
    {
        this->message = "Option '--";
        this->message += longName;
    }
    this->message += "': ";
    this->message += message;
}

/* Command line parser class */

CLIParser::CLIParser(const char* programName, const CLIOption* options,
        cxuint argc, const char** argv)
try
{
    shortNameMap = nullptr;
    this->programName = programName;
    this->options = options;
    this->argc = argc;
    this->argv = argv;
    
    shortNameMap = new cxuint[256];
    std::fill(shortNameMap, shortNameMap+256, UINT_MAX); // fill as unused
    
    cxuint i = 0;
    for (; options[i].longName != nullptr && options[i].shortName != 0; i++)
    {
        if (options[i].shortName != 0)
        {
            if (options[i].shortName == '-' || cxuchar(options[i].shortName) < 0x20)
                throw CLIException("Illegal short name");
            shortNameMap[cxuchar(options[i].shortName)] = i;
        }
        
        if (options[i].longName != nullptr)
        {
            if (::strchr(options[i].longName, '=') != nullptr)
                throw CLIException("Illegal long name");
            longNameMap.insert(std::make_pair(options[i].longName, i));
        }   
    }
    optionEntries.resize(i); // resize to number of options
}
catch(...)
{
    delete[] shortNameMap;
}

CLIParser::~CLIParser()
{
    for (cxuint i = 0; i < optionEntries.size(); i++)
    {   /* free optionEntry if it allocated some memory space */
        const OptionEntry& optEntry = optionEntries[i];
        if (!optEntry.isArg)
            continue;
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
                for (size_t e = 0; e < optEntry.arrSize; e++)
                    delete[] optEntry.v.sArr[e];
                delete[] optEntry.v.sArr;
                break;
            default:
                break;
        }
    }
            
    delete[] shortNameMap;
}

void CLIParser::handleExceptionsForGetOptArg(cxuint optionId, CLIArgType argType)
{
    if (optionId < optionEntries.size())
        throw CLIException("No such command line option!");
    if (!optionEntries[optionId].isArg)
        throw CLIException("Command line option doesn't have argument!");
    if (argType != options[optionId].argType ||
        (argType == CLIArgType::TRIMMED_STRING &&
            options[optionId].argType == CLIArgType::STRING) ||
        (argType == CLIArgType::STRING &&
            options[optionId].argType == CLIArgType::TRIMMED_STRING) ||
        (argType == CLIArgType::TRIMMED_STRING_ARRAY &&
            options[optionId].argType == CLIArgType::STRING_ARRAY) ||
        (argType == CLIArgType::STRING_ARRAY &&
            options[optionId].argType == CLIArgType::TRIMMED_STRING_ARRAY))
        throw CLIException("Argument type of command line option mismatch!");
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

void CLIParser::parseOptionArg(cxuint optionId, const char* optArg, bool chooseShortName)
{
    const CLIOption& option = options[optionId];
    OptionEntry& optEntry = optionEntries[optionId];
    
    switch(option.argType)
    {
        case CLIArgType::BOOL:
            optArg = skipSpaces(optArg);
            optEntry.v.b = false;
            for (const char* v: { "1", "true", "t", "on", "yes", "y"})
                if (::strncasecmp(optArg, v, ::strlen(optArg)) == 0)
                {
                    optArg += ::strlen(v);
                    optEntry.v.b = true;
                    break;
                }
            if (!optEntry.v.b)
            {
                bool isFalse = false;
                for (const char* v: { "0", "false", "f", "off", "no", "n"})
                    if (::strncasecmp(optArg, v, ::strlen(optArg)) == 0)
                    {
                        optArg += ::strlen(v);
                        isFalse = true;
                        break;
                    }
                if (!isFalse)
                    throw CLIException("Can't parse bool argument for option",
                               option.shortName, option.longName, chooseShortName);
            }
            optArg = skipSpaces(optArg);
            if (*optArg != 0)
                throw CLIException("Garbages at end of argument",
                               option.shortName, option.longName, chooseShortName);
            break;
        case CLIArgType::UINT:
            try
            { optEntry.v.u32 = parseOptArg<uint32_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::INT:
            try
            { optEntry.v.i32 = parseOptArg<int32_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::UINT64:
            try
            { optEntry.v.u64 = parseOptArg<uint64_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::INT64:
            try
            { optEntry.v.i64 = parseOptArg<int64_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::SIZE:
            try
            { optEntry.v.size = parseOptArg<size_t>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::FLOAT:
            try
            { optEntry.v.f = parseOptArg<float>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::DOUBLE:
            try
            { optEntry.v.d = parseOptArg<double>(optArg); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
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
            optEntry.v.s = newStr;
            break;
        }
        case CLIArgType::STRING:
            optEntry.v.s = optArg;
            break;
        case CLIArgType::BOOL_ARRAY:
        {
            optEntry.arrSize = 0;
            std::vector<bool> bVec;
            while (*optArg != 0)
            {
                optArg = skipSpaces(optArg);
                if (*optArg == 0) break;
                
                const char* oldOptArg = optArg;
                bool entryVal = false;
                for (const char* v: { "1", "true", "t", "on", "yes", "y"})
                    if (::strncasecmp(optArg, v, ::strlen(optArg)) == 0)
                    {
                        optArg += ::strlen(v);
                        entryVal = true;
                        break;
                    }
                bool isFalse = false;
                for (const char* v: { "0", "false", "f", "off", "no", "n"})
                    if (::strncasecmp(optArg, v, ::strlen(optArg)) == 0)
                    {
                        optArg += ::strlen(v);
                        isFalse = true;
                        break;
                    }
                    
                if (!isFalse)
                    throw CLIException("Can't parse array of boolean argument for option",
                               option.shortName, option.longName, chooseShortName);
                bVec.push_back(entryVal);
                
                if (oldOptArg+1 != optArg)
                {   /* is not single character */
                    optArg = skipSpaces(optArg);
                    if (*optArg == ',')
                        optArg++;
                    else if (*optArg != 0)
                        throw CLIException("Garbages at end of argument",
                               option.shortName, option.longName, chooseShortName);
                }
            }
            // copy to option entry
            optEntry.v.bArr = new bool[bVec.size()];
            optEntry.arrSize = bVec.size();
            std::copy(bVec.begin(), bVec.end(), optEntry.v.bArr);
            break;
        }
        case CLIArgType::UINT_ARRAY:
            try
            { optEntry.v.u32Arr = parseOptArgList<uint32_t>(optArg, optEntry.arrSize); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::INT_ARRAY:
            try
            { optEntry.v.i32Arr = parseOptArgList<int32_t>(optArg, optEntry.arrSize); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::UINT64_ARRAY:
            try
            { optEntry.v.u64Arr = parseOptArgList<uint64_t>(optArg, optEntry.arrSize); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::INT64_ARRAY:
            try
            { optEntry.v.i64Arr = parseOptArgList<int64_t>(optArg, optEntry.arrSize); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::SIZE_ARRAY:
            try
            { optEntry.v.sizeArr = parseOptArgList<size_t>(optArg, optEntry.arrSize); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::FLOAT_ARRAY:
            try
            { optEntry.v.fArr = parseOptArgList<float>(optArg, optEntry.arrSize); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::DOUBLE_ARRAY:
            try
            { optEntry.v.dArr = parseOptArgList<double>(optArg, optEntry.arrSize); }
            catch(const ParseException& ex)
            { throw CLIException(ex.what(), option.shortName, option.longName,
                        chooseShortName); }
            break;
        case CLIArgType::STRING_ARRAY:
        case CLIArgType::TRIMMED_STRING_ARRAY:
        {
            optEntry.arrSize = 0;
            std::vector<char*> sVec;
            const bool doTrim = (option.argType == CLIArgType::TRIMMED_STRING_ARRAY);
            
            try
            {
            while (*optArg != 0)
            {
                if (doTrim)
                    optArg = skipSpaces(optArg);
                
                size_t elemLength = 0;
                const char* elemEnd;
                for (elemEnd = optArg, elemLength = 0;
                     *elemEnd == ',' || *elemEnd == 0; ++elemEnd, ++elemLength)
                     if (*elemEnd == '\\' && *elemEnd == ',')
                         elemEnd++; // escape of ','
                if (doTrim)
                {
                    const char* oldElemEnd = elemEnd;
                    elemEnd = skipSpacesAtEnd(optArg, elemEnd-optArg);
                    elemLength += oldElemEnd-elemEnd; // subtract length to trimmed
                }
                
                char* strElem = new char[elemLength+1];
                // copy
                for (size_t l = 0; *optArg == ',' || *optArg == 0; ++optArg, ++l)
                    if (*optArg == '\\' && *optArg == ',')
                    {
                        ++optArg;
                        strElem[l] = *optArg;
                    }
                strElem[elemLength] = 0;
                sVec.push_back(strElem);
                
                if (*optArg == ',')
                    optArg++;
                else if (*optArg != 0)
                    throw CLIException("Garbages at end of argument",
                           option.shortName, option.longName, chooseShortName);
            }
            optEntry.v.sArr = new const char*[sVec.size()];
            optEntry.arrSize = sVec.size();
            std::copy(sVec.begin(), sVec.end(), optEntry.v.sArr);
            }
            catch(...)
            {   // delete previously parsed strings
                for (const char* v: sVec)
                    delete[] v;
                throw;
            }
            break;
        }
        default:
            throw CLIException("Unknown option argument type", option.shortName,
                       option.longName, chooseShortName);
            break;
    }
    optEntry.isArg = true;
}

void CLIParser::parse()
{   /* parse args */
    bool isLeftOver = false;
    for (cxuint i = 1; i < argc; i++)
    {
        if (argv[i] == nullptr)
            throw CLIException("Null argument!");
        const char* arg = argv[i];
        if (!isLeftOver && arg[0] == '-' && arg[1] != 0)
        {
            
            if (arg[1] == '-')
            {   // longNames
                if (arg[2] == 0)
                {   // if '--'
                    isLeftOver = true;
                    continue;
                }
                /* find option from long name */
                auto it = longNameMap.lower_bound(arg+2);
                const char* optLongName = nullptr;
                bool found = false;
                size_t optLongNameLen;
                if (it != longNameMap.end())
                {   /* check this same or greater key */
                    optLongName = options[it->second].longName;
                    optLongNameLen = ::strlen(optLongName);
                    found = (::strncmp(arg+2, optLongName, optLongNameLen) == 0 &&
                        (arg[2+optLongNameLen] == 0 || arg[2+optLongNameLen] == '='));
                }
                
                if (!found && it != longNameMap.begin())
                {   /* check previous (lower) entry in longNameMap */
                    --it;
                    optLongName = options[it->second].longName;
                    optLongNameLen = ::strlen(optLongName);
                    found = (::strncmp(arg+2, optLongName, optLongNameLen) == 0 &&
                        (arg[2+optLongNameLen] == 0 || arg[2+optLongNameLen] == '='));
                }
                
                if (!found) // unknown option
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
                        if (argv[i+1][0] != '-' || argv[i+1][1] == 0 ||
                            !option.argIsOptional)
                        {
                            i++; // next elem
                            optArg = argv[i];
                        }
                    }
                    else if (!option.argIsOptional)
                        throw CLIException("Missing argument", option.longName);
                    
                    if (optArg != nullptr) /* parse option argument */
                        parseOptionArg(optionId, optArg, false);
                }
            }
            else
            {   // short names
                arg++;
                for (; *arg != 0; arg++)
                {
                    const cxuint optionId = shortNameMap[cxuchar(*arg)];
                    if (optionId != UINT_MAX)
                    {
                        const CLIOption& option = options[optionId];
                        OptionEntry& optionEntry =  optionEntries[optionId];
                        optionEntry.isSet = true;
                        
                        if (option.argType != CLIArgType::NONE)
                            break;
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
                                if (argv[i+1][0] != '-' || argv[i+1][1] == 0 ||
                                    !option.argIsOptional)
                                {
                                    i++; // next elem
                                    optArg = argv[i];
                                }
                            }
                            else if (!option.argIsOptional)
                                throw CLIException("Missing argument", option.shortName);
                            
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
}

void CLIParser::printHelp(std::ostream& os) const
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
            if (option.argName!=nullptr)
                colLength += ::strlen(option.argName);
            else
                colLength += 3; // ARG
            if (option.argIsOptional)
                colLength += 2; // quadratic braces
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
        if (option.shortName == 0)
        {
            optColumn += "  -";
            optColumn += option.shortName;
        }
        else
            optColumn += "    ";
        
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
        optColumn.clear();
        
        if (option.description != nullptr)
            os << option.description;
        os << '\n';
    }
    
    os.flush();
}

void CLIParser::printUsage(std::ostream& os) const
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
            if (option.argType == CLIArgType::NONE)
            {
                if (option.argIsOptional)
                    os << '[';
                os << '=' << ((option.argName!=nullptr) ? option.argName : "ARG");
                if (option.argIsOptional)
                    os << ']';
            }
        }
        os << ']';
    }
    os << " ..." << std::endl;
}
