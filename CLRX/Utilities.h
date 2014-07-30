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
/*! \file Utilities.h
 * \brief utilities for other libraries and programs
 */

#ifndef __CLRX_UTILITIES_H__
#define __CLRX_UTILITIES_H__

#include <CLRX/Config.h>
#include <exception>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <locale>
#include <mutex>
#include <sstream>

/// main namespace
namespace CLRX
{

/// exception class
class Exception: public std::exception
{
protected:
    std::string message;
public:
    Exception() = default;
    explicit Exception(const std::string& message);
    virtual ~Exception() throw() = default;
    
    const char* what() const throw();
};

class ParseException: public Exception
{
public:
    ParseException() = default;
    explicit ParseException(const std::string& message);
    ParseException(size_t lineNo, const std::string& message);
    virtual ~ParseException() throw() = default;
};


enum {
    DYNLIB_LOCAL = 0,   ///< treat symbols locally
    DYNLIB_LAZY = 1,    ///< resolve symbols when is needed
    DYNLIB_NOW = 2,     ///< resolve symbols now
    DYNLIB_MODE1_MASK = 7,  ///
    DYNLIB_GLOBAL = 8   ///< treats symbols globally
};

/// dynamic library class
class DynLibrary
{
private:
    void* handle;
    static std::mutex mutex;
public:
    DynLibrary();
    /** constructor - loads library
     * \param filename library filename
     * \param flags flags specifies way to load library and a resolving symbols
     */
    DynLibrary(const char* filename, cxuint flags = 0);
    ~DynLibrary();
    
    DynLibrary(const DynLibrary&) = delete;
    DynLibrary(DynLibrary&&) = delete;
    DynLibrary& operator=(const DynLibrary&) = delete;
    DynLibrary& operator=(DynLibrary&&) = delete;
    
    /** loads library
     * \param filename library filename
     * \param flags flags specifies way to load library and a resolving symbols
     */
    void load(const char* filename, cxuint flags = 0);
    /// unload library
    void unload();
    
    /// get symbol
    void* getSymbol(const std::string& symbolName);
};

/* parse utilities */

/// parse environment variable
/**parse environment variable
 * \param envVar - name of environment variable
 * \param defaultValue - value that will be returned if environment variable is not present
 */
template<typename T>
T parseEnvVariable(const char* envVar, const T& defaultValue = T())
{
    const char* var = getenv(envVar);
    if (var == nullptr)
        return defaultValue;
    std::istringstream iss(var);
    iss.imbue(std::locale::classic()); // force use classic locale
    T value;
    iss >> value;
    if (iss.fail() || iss.bad())
        return defaultValue;
    return value;
}

extern template
cxuint parseEnvVariable<cxuint>(const char* envVar, const cxuint& defaultValue);

extern template
cxint parseEnvVariable<cxint>(const char* envVar, const cxint& defaultValue);

extern template
cxushort parseEnvVariable<cxushort>(const char* envVar, const cxushort& defaultValue);

extern template
cxshort parseEnvVariable<cxshort>(const char* envVar, const cxshort& defaultValue);

extern template
cxulong parseEnvVariable<cxulong>(const char* envVar, const cxulong& defaultValue);

extern template
cxlong parseEnvVariable<cxlong>(const char* envVar, const cxlong& defaultValue);

extern template
cxullong parseEnvVariable<cxullong>(const char* envVar, const cxullong& defaultValue);

extern template
cxllong parseEnvVariable<cxllong>(const char* envVar, const cxllong& defaultValue);

#ifndef __UTILITIES_MODULE__

extern template
std::string parseEnvVariable<std::string>(const char* envVar,
              const std::string& defaultValue);

extern template
bool parseEnvVariable<bool>(const char* envVar, const bool& defaultValue);

#endif

/// function class that returns true if first C string is less than second
struct CStringLess
{
    inline bool operator()(const char* c1, const char* c2) const
    { return ::strcmp(c1, c2)<0; }
};

/// function class that returns true if C strings are equal
struct CStringEqual
{
    inline bool operator()(const char* c1, const char* c2) const
    { return ::strcmp(c1, c2)==0; }
};

/// generate hash function for C string
struct CStringHash
{
    size_t operator()(const char* c) const;
};

/// parse unsigned integer regardless locales
/** parses unsigned integer in decimal form from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string
 * \return parsed integer value
 */
cxuint cstrtoui(const char* str, const char* inend, const char*& outend);

/// parse 8-bit unsigned formatted looks like C-style
/** parses 8-bit unsigned integer from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format, octal form (with prefix '0'), hexadecimal form
 * (prefix '0x' or '0X'), and binary form (prefix '0b' or '0B').
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string
 * \return parsed integer value
 */
uint8_t cstrtou8CStyle(const char* str, const char* inend, const char*& outend);

/// parse 16-bit unsigned formatted looks like C-style
/** parses 16-bit unsigned integer from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format, octal form (with prefix '0'), hexadecimal form
 * (prefix '0x' or '0X'), and binary form (prefix '0b' or '0B').
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string
 * \return parsed integer value
 */
uint16_t cstrtou16CStyle(const char* str, const char* inend, const char*& outend);

/// parse 32-bit unsigned formatted looks like C-style
/** parses 32-bit unsigned integer from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format, octal form (with prefix '0'), hexadecimal form
 * (prefix '0x' or '0X'), and binary form (prefix '0b' or '0B').
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string
 * \return parsed integer value
 */
uint32_t cstrtou32CStyle(const char* str, const char* inend, const char*& outend);

/// parse 64-bit unsigned formatted looks like C-style
/** parses 64-bit unsigned integerfrom str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format, octal form (with prefix '0'), hexadecimal form
 * (prefix '0x' or '0X'), and binary form (prefix '0b' or '0B').
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string
 * \return parsed integer value
 */
uint64_t cstrtou64CStyle(const char* str, const char* inend, const char*& outend);

/// parse half float formatted looks like C-style
/** parses half floating point from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format and binary format. Result is rounded to nearest even
 * (if two values are equally close will be choosen a even value).
 * Currently only IEEE-754 format is supported.
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string
 * \return parsed floating point value
 */
cxushort cstrtohCStyle(const char* str, const char* inend, const char*& outend);

/// parse single float formatted looks like C-style
/** parses single floating point from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format and binary format. Result is rounded to nearest even
 * (if two values are equally close will be choosen a even value).
 * Currently only IEEE-754 format is supported.
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string
 * \return parsed floating point value
 */
float cstrtofCStyle(const char* str, const char* inend, const char*& outend);

/// parse double float formatted looks like C-style
/** parses double floating point from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format and binary format. Result is rounded to nearest even
 * (if two values are equally close will be choosen a even value).
 * Currently only IEEE-754 format is supported.
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string
 * \return parsed floating point value
 */
double cstrtodCStyle(const char* str, const char* inend, const char*& outend);

};

#endif
