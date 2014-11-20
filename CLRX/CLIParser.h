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
/*! \file CLIParser.h
 * \brief command line parser
 */

#ifndef __CLRX_CLIPARSER_H__
#define __CLRX_CLIPARSER_H__

#include <CLRX/Config.h>
#include <exception>
#include <string>
#include <vector>
#include <ostream>
#include <cstring>
#include <CLRX/Utilities.h>

/// main namespace
namespace CLRX
{

/*
 * CommandLine Interface
 */

enum class CLIArgType: cxuint
{
    NONE = 0,
    BOOL,
    UINT,
    INT,
    UINT64,
    INT64,
    SIZE,
    FLOAT,
    DOUBLE,
    STRING,
    BOOL_ARRAY = 32,
    UINT_ARRAY,
    INT_ARRAY,
    UINT64_ARRAY,
    INT64_ARRAY,
    SIZE_ARRAY,
    FLOAT_ARRAY,
    DOUBLE_ARRAY,
    STRING_ARRAY
};

struct CLIOption
{
    const char* longName;
    char shortName;
    bool argIsOptional;
    CLIArgType argType;
    const char* argName;
    const char* description;
};

class CLIParser
{
private:
    template<typename T>
    struct OptTypeTrait { static const CLIArgType type; };
    
    struct OptionEntry
    {
        bool isSet;
        bool isArg;
        union {
            bool b;
            int32_t i32;
            uint32_t u32;
            int64_t i64;
            uint64_t u64;
            size_t size;
            float f;
            double d;
            const char* s;
            bool* bArr;
            int32_t* i32Arr;
            uint32_t* u32Arr;
            int64_t* i64Arr;
            uint64_t* u64Arr;
            size_t* sizeArr;
            float* fArr;
            double* dArr;
            const char** sArr;
            
            operator bool() const { return b; }
            operator int32_t() const { return i32; }
            operator uint32_t() const { return u32; }
            operator int64_t() const { return i64; }
            operator uint64_t() const { return u64; }
            operator size_t() const { return size; }
            operator float() const { return f; }
            operator double() const { return d; }
            operator const char*() const { return s; }
            
            operator const bool*() const { return bArr; }
            operator const int32_t*() const { return i32Arr; }
            operator const uint32_t*() const { return u32Arr; }
            operator const int64_t*() const { return i64Arr; }
            operator const uint64_t*() const { return u64Arr; }
            operator const size_t*() const { return sizeArr; }
            operator const float*() const { return fArr; }
            operator const double*() const { return dArr; }
            operator const char**() const { return sArr; }
        } v;
        size_t arrSize;
    };
    
    const CLIOption* options;
    const char* programName;
    cxuint argc;
    const char** argv;
    cxuint leftOverArgsNum;
    const char** leftOverArgs;
    
    std::vector<OptionEntry> optionEntries;
    
    void handleExceptionsForGetOptArg(cxuint optionId, CLIArgType argType);
    
    bool doExit;
public:
    CLIParser(const char* programName, const CLIOption* options,
            cxuint argc, const char** argv);
    ~CLIParser();
    
    void parse();
    
    bool isExit() const
    { return doExit; }
    
    template<typename T>
    T getOptArg(cxuint optionId) const
    {
        handleExceptionsGetOptArg(optionId, OptTypeTrait<T>::type);
        return static_cast<T>(optionEntries[optionId].v);
    }
    
    template<typename T>
    const T* getOptArgArray(cxuint optionId, size_t& length) const
    {
        handleExceptionsGetOptArg(optionId, OptTypeTrait<T>::type);
        const OptionEntry& optEntry = optionEntries[optionId];
        length = optEntry.arrSize;
        return static_cast<T*>(optEntry.v);
    }
    
    bool hasOptArg(cxuint optionId) const
    {
        if (optionId < optionEntries.size())
            throw Exception("No such command line option!");
        return optionEntries[optionId].isArg;
    }
    
    bool hasOption(cxuint optionId) const
    { 
        if (optionId < optionEntries.size())
            throw Exception("No such command line option!");
        return optionEntries[optionId].isSet;
    }
    
    cxuint getArgsNum() const
    { return leftOverArgsNum; }
    const char** getArgs() const
    { return leftOverArgs; }
    
    void printHelp(std::ostream& os) const;
    void printUsage(std::ostream& os) const;
};

template<>
struct CLIParser::OptTypeTrait<cxint> {
static const CLIArgType type = CLIArgType::INT; };

template<>
struct CLIParser::OptTypeTrait<cxulong> {
static const CLIArgType type = (sizeof(cxulong)==8)?CLIArgType::UINT64:CLIArgType::UINT; };

template<>
struct CLIParser::OptTypeTrait<cxlong> {
static const CLIArgType type = (sizeof(cxlong)==8)?CLIArgType::INT64:CLIArgType::INT; };

template<>
struct CLIParser::OptTypeTrait<cxullong> {
static const CLIArgType type = CLIArgType::UINT64; };

template<>
struct CLIParser::OptTypeTrait<cxllong> {
static const CLIArgType type = CLIArgType::INT64; };

template<>
struct CLIParser::OptTypeTrait<float> {
static const CLIArgType type = CLIArgType::FLOAT; };

template<>
struct CLIParser::OptTypeTrait<double> {
static const CLIArgType type = CLIArgType::DOUBLE; };

template<>
struct CLIParser::OptTypeTrait<const char*> {
static const CLIArgType type = CLIArgType::STRING; };

template<>
struct CLIParser::OptTypeTrait<cxint*> {
static const CLIArgType type = CLIArgType::INT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxulong*> {
static const CLIArgType type =
    (sizeof(cxulong)==8)?CLIArgType::UINT64_ARRAY:CLIArgType::UINT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxlong*> {
static const CLIArgType type =
    (sizeof(cxlong)==8)?CLIArgType::INT64_ARRAY:CLIArgType::INT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxullong*> {
static const CLIArgType type = CLIArgType::UINT64_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<cxllong*> {
static const CLIArgType type = CLIArgType::INT64_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<float*> {
static const CLIArgType type = CLIArgType::FLOAT_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<double*> {
static const CLIArgType type = CLIArgType::DOUBLE_ARRAY; };

template<>
struct CLIParser::OptTypeTrait<const char**> {
static const CLIArgType type = CLIArgType::STRING_ARRAY; };

};

#endif
