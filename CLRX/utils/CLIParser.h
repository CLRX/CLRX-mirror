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
#include <iostream>
#include <memory>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>

/// main namespace
namespace CLRX
{
/// helper for auto help (this macro should be place in CLI option table
#define CLRX_CLI_AUTOHELP \
    { "help", '?', CLIArgType::NONE, false, false, "print help", nullptr }, \
    { "usage", 0, CLIArgType::NONE, false, false, "print usage", nullptr }, \
    { "version", 0, CLIArgType::NONE, false, false, "print version", nullptr },

/*
 * CommandLine Interface
 */

/// type of argument of the option
enum class CLIArgType: cxuchar
{
    NONE = 0, ///< no argument
    BOOL, UINT, INT, UINT64, INT64, SIZE, FLOAT, DOUBLE, STRING,
    TRIMMED_STRING, ///< trimmed string (without spaces at begin and end)
    SINGLE_MAX = TRIMMED_STRING,
    BOOL_ARRAY = 32,
    UINT_ARRAY, INT_ARRAY, UINT64_ARRAY, INT64_ARRAY, SIZE_ARRAY, FLOAT_ARRAY,
    DOUBLE_ARRAY, STRING_ARRAY,
    TRIMMED_STRING_ARRAY, ///< trimmed string (without spaces at begin and end)
    ARRAY_MAX = TRIMMED_STRING_ARRAY
};

/// Command line option description
struct CLIOption
{
    const char* longName;   ///< long name of option
    char shortName;         ///< short name of option (single character)
    CLIArgType argType;     ///< type of argument of option (or none)
    bool argIsOptional;     ///< if true then option argument is optional
    
    /// construct argument array from any occurrence of option argument
    bool arrayFromOccurrences; 
    
    const char* description;    ///< description of option
    const char* argName;    ///< name of argument of option
};

/// CLI exception class
class CLIException: public Exception
{
public:
    /// default constructor
    CLIException() = default;
    /// constructor from message
    explicit CLIException(const std::string& message);
    /// constructor for short option
    CLIException(const std::string& message, char shortName);
    /// constructor for long option
    CLIException(const std::string& message, const std::string& longName);
    /// constructor for option
    CLIException(const std::string& message, const CLIOption& option,
             bool chooseShortName);
    /// destructor
    virtual ~CLIException() noexcept = default;
};

/// The Command Line Parser (parses options and their arguments)
/** This class implements a command-line parser which provides short options
 * and long options with/without arguments. Argument can be a string, number, boolean,
 * and list of previous. Conventions of the option's (and their arguments) syntax
 * was adapted from popt library.
 *
 * Option argument can be attached in the next argument or in the rest of the argument
 * (after '=' or not for short options). If argument option is optional then can be
 * attached in a next argument only when argument does not have option
 * (otherwise argument will be treated likes next option).
 * 
 * Elements of option argument array are separated by comma. Commas and backslashes can be
 * entered by using backslash escapes in a string array element. An element of string array
 * can also be empty.
 * 
 * Option identified by optionId which is order number. First optionId is 0. Option can have
 * short name, long name or both. Option without argument must have argType set likes
 * CLIArgType::NONE (no type, no argument). Argument name (argName)
 * is optional (can be null).
 * 
 * IMPORTANT NOTICE: Option's list and argument's list must be available and unmodified
 * while whole lifecycle of this CLIParser. For whole lifecycle of any option argument or
 * left over argument, a CLIParser must be alive.
 * (because it keeps values of these arguments). */
class CLIParser: public NonCopyableAndNonMovable
{
private:
    template<typename T>
    struct OptTypeTrait { static const CLIArgType type; };
    
    typedef Array<std::pair<const char*, cxuint> > LongNameMap;
    
    struct OptionEntry
    {
        bool isSet;     ///< true if entry set
        bool isArg;     ///< true if entry have set argument
        
        /// value holder
        union Value {
            bool b; ///< boolean
            cxint i;    ///< cxint
            cxuint u;   ///< cxuint
            cxlong li;  ///< cxlong
            cxulong lu; ///< cxulong
            cxllong lli;    ///< cxllong
            cxullong llu;   ///< cxullong
            int32_t i32;    ///< 32-bit int
            uint32_t u32;   ///< 32-bit uint
            int64_t i64;    ///< 64-bit int
            uint64_t u64;   ///< 64-bit uint
            size_t size;    ///< size
            float f;    ///< float
            double d;   ///< double
            const char* s;  ///< C-style string
            bool* bArr; ///< array of booleans
            cxint* iArr;     ///< array of cxints
            cxuint* uArr;   ///< array of cxuints
            cxlong* liArr;  ///< array of cxlongs
            cxulong* luArr; ///< array cxulongs
            cxllong* lliArr;    ///< array of cxllongs
            cxullong* lluArr;   ///< array of cxullongs
            int32_t* i32Arr;    ///< array of 32-bit ints
            uint32_t* u32Arr;   ///< array of 32-bit uints
            int64_t* i64Arr;    ///< array of 64-bit ints
            uint64_t* u64Arr;   ///< array of 64-bit uints
            size_t* sizeArr;    ///< array of sizes
            float* fArr;        ///< array of floats
            double* dArr;       ///< array of doubles
            const char** sArr;      ///< array of C-style strings
            
            /// cast to boolean
            operator bool() const { return b; }
            /// cast to cxint
            operator cxint() const { return i; }
            /// cast to cxuint
            operator cxuint() const { return u; }
            /// cast to cxlong
            operator cxlong() const { return li; }
            /// cast to cxulong
            operator cxulong() const { return lu; }
            /// cast to cxllong
            operator cxllong() const { return lli; }
            /// cast to cxullong
            operator cxullong() const { return llu; }
            /// cast to float
            operator float() const { return f; }
            /// cast to double
            operator double() const { return d; }
            /// cast to C-style string
            operator const char*() const { return s; }
            
            /// cast to array of booleans
            operator const bool*() const { return bArr; }
            /// cast to array of cxints
            operator const cxint*() const { return iArr; }
            /// cast to array of cxuints
            operator const cxuint*() const { return uArr; }
            /// cast to array of cxlongs
            operator const cxlong*() const { return liArr; }
            /// cast to array of cxulongs
            operator const cxulong*() const { return luArr; }
            /// cast to array of cxllongs
            operator const cxllong*() const { return lliArr; }
            /// cast to array of cxullongs
            operator const cxullong*() const { return lluArr; }
            /// cast to array of floats
            operator const float*() const { return fArr; }
            /// cast to array of doubles
            operator const double*() const { return dArr; }
            /// cast to array of C-style strings
            operator const char**() const { return sArr; }
        };
        Value v;    ///< value
        size_t arrSize;
        OptionEntry() : isSet(false), isArg(false), arrSize(0)
        {  ::memset(&v, 0, sizeof(v)); /* use memset, workaround for CLang++ */ }
    };
    
    const CLIOption* options;
    const char* programName;
    const char* packageName;
    cxuint argc;
    const char** argv;
    std::vector<const char*> leftOverArgs;
    
    Array<OptionEntry> optionEntries;
    LongNameMap longNameMap;
    std::unique_ptr<cxuint[]> shortNameMap;
    
    void handleExceptionsForGetOptArg(cxuint optionId, CLIArgType argType) const;
    void parseOptionArg(cxuint optionId, const char* optArg, bool chooseShortName);
    
public:
    /// constructor
    /**
     * \param programName name of program
     * \param options null-terminated (shortName==0, longName==NULL) options list
     * \param argc argc
     * \param argv argv
     */
    CLIParser(const char* programName, const CLIOption* options,
            cxuint argc, const char** argv);
    ~CLIParser();
    
    /// set package name
    void setPackageName(const char* pkgName)
    { packageName = pkgName; }
    
    /// parse options from arguments
    void parse();
    
    /// handle printing of help or usage. returns false when help or usage not enabled
    bool handleHelpOrUsage(std::ostream& os = std::cout) const;
    
    /// find option by shortName, returns optionId
    cxuint findOption(char shortName) const;
    
    /// find option by longName, returns optionId
    cxuint findOption(const char* longName) const;
    
    /// get option argument if it provided
    template<typename T>
    T getOptArg(cxuint optionId) const
    {
        handleExceptionsForGetOptArg(optionId, OptTypeTrait<T>::type);
        return static_cast<T>(optionEntries[optionId].v);
    }
    
    /// get option argument if it provided
    template<typename T>
    T getShortOptArg(char shortName) const
    { return getOptArg<T>(findOption(shortName)); }
    
    /// get option argument if it provided
    template<typename T>
    T getLongOptArg(const char* longName) const
    { return getOptArg<T>(findOption(longName)); }
    
    /// get option argument array  if it provided
    /**
     * \param optionId id of option
     * \param length length of array
     * \return argument array
     */
    template<typename T>
    const T* getOptArgArray(cxuint optionId, size_t& length) const
    {
        handleExceptionsForGetOptArg(optionId, OptTypeTrait<T*>::type);
        const OptionEntry& optEntry = optionEntries[optionId];
        length = optEntry.arrSize;
        return static_cast<const T*>(optEntry.v);
    }
    
    /// get option argument array  if it provided
    /**
     * \param shortName short name of option
     * \param length length of array
     * \return argument array
     */
    template<typename T>
    const T* getShortOptArgArray(char shortName, size_t& length) const
    { return getOptArgArray<T>(findOption(shortName), length); }
    
    /// get option argument array  if it provided
    /**
     * \param longName long name of option
     * \param length length of array
     * \return argument value
     */
    template<typename T>
    const T* getLongOptArgArray(const char* longName, size_t& length) const
    { return getOptArgArray<T>(findOption(longName), length); }
    
    /// returns true when argument provided for specified option
    bool hasOptArg(cxuint optionId) const
    {
        if (optionId >= optionEntries.size())
            throw CLIException("No such command line option!");
        return optionEntries[optionId].isArg;
    }
    
    /// returns true when argument provided for specified option
    bool hasShortOptArg(char shortName) const
    { return hasOptArg(findOption(shortName)); }
    
    /// returns true when argument provided for specified option
    bool hasLongOptArg(const char* longName) const
    { return hasOptArg(findOption(longName)); }
    
    /// returns true if option included in command line
    bool hasOption(cxuint optionId) const
    { 
        if (optionId >= optionEntries.size())
            throw CLIException("No such command line option!");
        return optionEntries[optionId].isSet;
    }
    
    /// returns true if option included in command line
    bool hasShortOption(char shortName) const
    { return hasOption(findOption(shortName)); }
    
    /// returns true if option included in command line
    bool hasLongOption(const char* longName) const
    { return hasOption(findOption(longName)); }
    
    /// get left over arguments number
    cxuint getArgsNum() const
    { return leftOverArgs.size()-1; }
    /// get left over arguments (null-terminated)
    const char* const* getArgs() const
    { return leftOverArgs.data(); }
    
    /// print help for program (lists options)
    void printHelp(std::ostream& os = std::cout) const;
    /// print usage
    void printUsage(std::ostream& os = std::cout) const;
    /// print version
    void printVersion(std::ostream& os = std::cout) const;
};

/// Option type trait for boolean type
template<>
struct CLIParser::OptTypeTrait<bool> {
static const CLIArgType type = CLIArgType::BOOL; ///< type
};

/// Option type trait for cxuint type
template<>
struct CLIParser::OptTypeTrait<cxuint> {
static const CLIArgType type = CLIArgType::UINT; ///< type
};

/// Option type trait for cxint type
template<>
struct CLIParser::OptTypeTrait<cxint> {
static const CLIArgType type = CLIArgType::INT; ///< type
};

/// Option type trait for cxulong type
template<>
struct CLIParser::OptTypeTrait<cxulong> {
/// type
static const CLIArgType type = (sizeof(cxulong)==8)?CLIArgType::UINT64:CLIArgType::UINT; };

/// Option type trait for cxlong type
template<>
struct CLIParser::OptTypeTrait<cxlong> {
/// type
static const CLIArgType type = (sizeof(cxlong)==8)?CLIArgType::INT64:CLIArgType::INT; };

/// Option type trait for cxullong type
template<>
struct CLIParser::OptTypeTrait<cxullong> {
static const CLIArgType type = CLIArgType::UINT64; ///< type
};

/// Option type trait for cxllong type
template<>
struct CLIParser::OptTypeTrait<cxllong> {
static const CLIArgType type = CLIArgType::INT64; ///< type
};

/// Option type trait for float type
template<>
struct CLIParser::OptTypeTrait<float> {
static const CLIArgType type = CLIArgType::FLOAT; ///< type
};

/// Option type trait for double type
template<>
struct CLIParser::OptTypeTrait<double> {
static const CLIArgType type = CLIArgType::DOUBLE; ///< type
};

/// Option type trait for const char* type
template<>
struct CLIParser::OptTypeTrait<const char*> {
static const CLIArgType type = CLIArgType::STRING; ///< type
};

/// Option type trait for bool* type
template<>
struct CLIParser::OptTypeTrait<bool*> {
static const CLIArgType type = CLIArgType::BOOL_ARRAY; ///< type
};

/// Option type trait for cxuint* type
template<>
struct CLIParser::OptTypeTrait<cxuint*> {
static const CLIArgType type = CLIArgType::UINT_ARRAY; ///< type
};

/// Option type trait for cxint* type
template<>
struct CLIParser::OptTypeTrait<cxint*> {
static const CLIArgType type = CLIArgType::INT_ARRAY; ///< type
};

/// Option type trait for cxulong* type
template<>
struct CLIParser::OptTypeTrait<cxulong*> {
/// type
static const CLIArgType type =
    (sizeof(cxulong)==8)?CLIArgType::UINT64_ARRAY:CLIArgType::UINT_ARRAY; };

/// Option type trait for cxlong* type
template<>
struct CLIParser::OptTypeTrait<cxlong*> {
/// type
static const CLIArgType type =
    (sizeof(cxlong)==8)?CLIArgType::INT64_ARRAY:CLIArgType::INT_ARRAY; };

/// Option type trait for cxullong* type
template<>
struct CLIParser::OptTypeTrait<cxullong*> {
static const CLIArgType type = CLIArgType::UINT64_ARRAY; ///< type
};

/// Option type trait for cxllong* type
template<>
struct CLIParser::OptTypeTrait<cxllong*> {
static const CLIArgType type = CLIArgType::INT64_ARRAY; ///< type
};

/// Option type trait for float* type
template<>
struct CLIParser::OptTypeTrait<float*> {
static const CLIArgType type = CLIArgType::FLOAT_ARRAY; ///< type
};

/// Option type trait for double* type
template<>
struct CLIParser::OptTypeTrait<double*> {
static const CLIArgType type = CLIArgType::DOUBLE_ARRAY; ///< type
};

/// Option type trait for const char** type
template<>
struct CLIParser::OptTypeTrait<const char**> {
static const CLIArgType type = CLIArgType::STRING_ARRAY; ///< type
};

};

#endif
