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
    Exception(const std::string& message);
    virtual ~Exception() throw() = default;
    
    const char* what() const throw();
};

class ParseException: public Exception
{
public:
    ParseException() = default;
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
cxuint cstrtoui(const char* s, const char*& end);

/// parse unsigned integer regardless locales
cxuint cstrtouiParse(const char* s, const char* inend, const char*& outend,
                     char delim, size_t lineNo);

/// parse 64-bit unsigned formatted looks like C-style
uint64_t cstrtou64CStyle(const char* s, const char* inend,
             const char*& outend, size_t lineNo, bool binaryFormat = false);

/// parse half float formatted looks like C-style
cxushort cstrtohCStyle(const char* s, const char* inend,
             const char*& outend, size_t lineNo, bool binaryFormat = false);

/// parse single float formatted looks like C-style
float cstrtofCStyle(const char* s, const char* inend,
             const char*& outend, size_t lineNo, bool binaryFormat = false);

/// parse double float formatted looks like C-style
double cstrtodCStyle(const char* s, const char* inend,
             const char*& outend, size_t lineNo, bool binaryFormat = false);

};

#endif
