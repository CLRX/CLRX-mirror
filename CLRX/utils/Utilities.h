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
/*! \file Utilities.h
 * \brief utilities for other libraries and programs
 */

#ifndef __CLRX_UTILITIES_H__
#define __CLRX_UTILITIES_H__

#include <CLRX/Config.h>
#include <exception>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <mutex>
#include <atomic>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/CString.h>

/// main namespace
namespace CLRX
{

/// non copyable and non movable base structure (class)
struct NonCopyableAndNonMovable
{
    /// constructor
    NonCopyableAndNonMovable() { }
    /// copy-constructor
    NonCopyableAndNonMovable(const NonCopyableAndNonMovable&) = delete;
    /// move-constructor
    NonCopyableAndNonMovable(NonCopyableAndNonMovable&&) = delete;
    /// copy-assignment
    NonCopyableAndNonMovable& operator=(const NonCopyableAndNonMovable&) = delete;
    /// move-asignment
    NonCopyableAndNonMovable& operator=(NonCopyableAndNonMovable&&) = delete;
};

/// exception class
class Exception: public std::exception
{
protected:
    std::string message;    ///< message
public:
    /// empty constructor
    Exception() = default;
    /// constructor with messasge
    explicit Exception(const std::string& message);
    /// destructor
    virtual ~Exception() noexcept = default;
    
    /// get exception message
    const char* what() const noexcept;
};

/// line number type
typedef uint64_t LineNo;

/// column number type
typedef size_t ColNo;

/// parse exception class
class ParseException: public Exception
{
public:
    /// empty constructor
    ParseException() = default;
    /// constructor with message
    explicit ParseException(const std::string& message);
    /// constructor with message and line number
    ParseException(LineNo lineNo, const std::string& message);
    /// constructor with message and line number and column number
    ParseException(LineNo lineNo, ColNo charNo, const std::string& message);
    /// destructor
    virtual ~ParseException() noexcept = default;
};

/// type for declaring various flags
typedef uint32_t Flags;

enum: Flags {
    FLAGS_ALL = 0xffffffffU
};

enum: Flags {
    DYNLIB_LOCAL = 0,   ///< treat symbols locally
    DYNLIB_LAZY = 1,    ///< resolve symbols when is needed
    DYNLIB_NOW = 2,     ///< resolve symbols now
    DYNLIB_MODE1_MASK = 7,  ///
    DYNLIB_GLOBAL = 8   ///< treats symbols globally
};

/// dynamic library class
class DynLibrary: public NonCopyableAndNonMovable
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
    DynLibrary(const char* filename, Flags flags = 0);
    ~DynLibrary();
    
    /** loads library
     * \param filename library filename
     * \param flags flags specifies way to load library and a resolving symbols
     */
    void load(const char* filename, Flags flags = 0);
    /// unload library
    void unload();
    
    /// get symbol
    void* getSymbol(const char* symbolName);
    
    /// returns true if  loaded
    operator bool() const
    { return handle!=nullptr; }
    
    /// returns true if not loaded
    bool operator!() const
    { return handle==nullptr; }
    
    // return true if loaded
    bool isLoaded() const
    { return handle!=nullptr; }
};

/* parse utilities */

/// check whether character is space
inline bool isSpace(unsigned char c);

inline bool isSpace(unsigned char c)
{ return (c == 32 || (c < 32 && (0x3e00U & (1U<<c)))); }

/// check whether character is digit
inline bool isODigit(unsigned char c);

inline bool isODigit(unsigned char c)
{ return c>='0' && c<= '7'; }

/// check whether character is digit
inline bool isDigit(unsigned char c);

inline bool isDigit(unsigned char c)
{ return c>='0' && c<= '9'; }

/// check whether character is hexadecimal digit
inline bool isXDigit(unsigned char c);

inline bool isXDigit(unsigned char c)
{ return (c>='0' && c<= '9') || (c>='a' && c<='f') || (c>='A' && c<='F'); }

/// check whether character is digit
inline bool isAlpha(unsigned char c);

inline bool isAlpha(unsigned char c)
{ return (c>='a' && c<='z') || (c>='A' && c<='Z'); }

/// check whether character is digit
inline bool isAlnum(unsigned char c);

inline bool isAlnum(unsigned char c)
{ return (c>='0' && c<= '9') || (c>='a' && c<='z') || (c>='A' && c<='Z'); }

/// skip spaces from cString
inline const char* skipSpaces(const char* s);

inline const char* skipSpaces(const char* s)
{
    while (isSpace(*s)) s++;
    return s;
}

/// skip spaces from cString
inline const char* skipSpacesAtEnd(const char* s, size_t length);

inline const char* skipSpacesAtEnd(const char* s, size_t length)
{
    const char* t = s+length;
    if (t == s) return s;
    for (t--; t != s-1 && isSpace(*t); t--);
    return t+1;
}

/// parses integer or float point formatted looks like C-style
/** parses integer or float point from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format, octal form (with prefix '0'), hexadecimal form
 * (prefix '0x' or '0X'), and binary form (prefix '0b' or '0B').
 * For floating points function accepts decimal format and binary format.
 * Result is rounded to nearest even
 * (if two values are equally close will be choosen a even value).
 * Currently only IEEE-754 format is supported.
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string (returned even if exception thrown)
 * \return parsed integer value
 */
template<typename T>
extern T cstrtovCStyle(const char* str, const char* inend, const char*& outend);

/// parse environment variable
/** parse environment variable
 * \param envVar - name of environment variable
 * \param defaultValue - value that will be returned if environment variable is not present
 * \return value
 */
template<typename T>
T parseEnvVariable(const char* envVar, const T& defaultValue = T())
{
    const char* var = getenv(envVar);
    if (var == nullptr)
        return defaultValue;
    var = skipSpaces(var);
    if (*var == 0)
        return defaultValue;
    const char* outend;
    try
    { return cstrtovCStyle<T>(var, nullptr, outend); }
    catch(const ParseException& ex)
    { return defaultValue; }
}

/// parse environment variable of cxuchar type
extern template
cxuchar parseEnvVariable<cxuchar>(const char* envVar, const cxuchar& defaultValue);

/// parse environment variable of cxchar type
extern template
cxchar parseEnvVariable<cxchar>(const char* envVar, const cxchar& defaultValue);

/// parse environment variable of cxuint type
extern template
cxuint parseEnvVariable<cxuint>(const char* envVar, const cxuint& defaultValue);

/// parse environment variable of cxint type
extern template
cxint parseEnvVariable<cxint>(const char* envVar, const cxint& defaultValue);

/// parse environment variable of cxushort type
extern template
cxushort parseEnvVariable<cxushort>(const char* envVar, const cxushort& defaultValue);

/// parse environment variable of cxshort type
extern template
cxshort parseEnvVariable<cxshort>(const char* envVar, const cxshort& defaultValue);

/// parse environment variable of cxulong type
extern template
cxulong parseEnvVariable<cxulong>(const char* envVar, const cxulong& defaultValue);

/// parse environment variable of cxlong type
extern template
cxlong parseEnvVariable<cxlong>(const char* envVar, const cxlong& defaultValue);

/// parse environment variable of cxullong type
extern template
cxullong parseEnvVariable<cxullong>(const char* envVar, const cxullong& defaultValue);

/// parse environment variable of cxllong type
extern template
cxllong parseEnvVariable<cxllong>(const char* envVar, const cxllong& defaultValue);

/// parse environment variable of float type
extern template
float parseEnvVariable<float>(const char* envVar, const float& defaultValue);

/// parse environment variable of double type
extern template
double parseEnvVariable<double>(const char* envVar, const double& defaultValue);

#ifndef __UTILITIES_MODULE__

/// parse environment variable of string type
extern template
std::string parseEnvVariable<std::string>(const char* envVar,
              const std::string& defaultValue);

/// parse environment variable of boolean type
extern template
bool parseEnvVariable<bool>(const char* envVar, const bool& defaultValue);

#endif

/// function class that returns true if first C string is less than second
struct CStringLess
{
    /// operator of call
    inline bool operator()(const char* c1, const char* c2) const
    { return ::strcmp(c1, c2)<0; }
};

/// function class that returns true if first C string is less than second (ignore case)
struct CStringCaseLess
{
    /// operator of call
    inline bool operator()(const char* c1, const char* c2) const
    { return ::strcasecmp(c1, c2)<0; }
};

/// function class that returns true if C strings are equal
struct CStringEqual
{
    /// operator of call
    inline bool operator()(const char* c1, const char* c2) const
    { return ::strcmp(c1, c2)==0; }
};

/// generate hash function for C string
struct CStringHash
{
    /// operator of call
    size_t operator()(const char* c) const
    {
        if (c == nullptr)
            return 0;
        size_t hash = 0;
        
        for (const char* p = c; *p != 0; p++)
            hash = ((hash<<8)^(cxbyte)*p)*size_t(0xbf146a3dU);
        return hash;
    }
};

/// counts leading zeroes for 32-bit unsigned integer. For zero behavior is undefined
inline cxuint CLZ32(uint32_t v);
/// counts leading zeroes for 64-bit unsigned integer. For zero behavior is undefined
inline cxuint CLZ64(uint64_t v);
/// counts trailing zeroes for 32-bit unsigned integer. For zero behavior is undefined
inline cxuint CTZ32(uint32_t v);
/// counts trailing zeroes for 64-bit unsigned integer. For zero behavior is undefined
inline cxuint CTZ64(uint64_t v);

inline cxuint CLZ32(uint32_t v)
{
#ifdef __GNUC__
    return __builtin_clz(v);
#else
#  ifdef _MSC_VER
    unsigned long index;
    _BitScanReverse(&index, v);
    return 31-index;
#  else
    cxuint count = 0;
    for (uint32_t t = 1U<<31; t > v; t>>=1, count++);
    return count;
#  endif
#endif
}

inline cxuint CLZ64(uint64_t v)
{
#ifdef __GNUC__
    return __builtin_clzll(v);
#else
#  ifdef _MSC_VER
#    ifdef HAVE_ARCH_X86
    unsigned long indexlo, indexhi;
    unsigned char nzhi;
    _BitScanReverse(&indexlo, uint32_t(v));
    nzhi = _BitScanReverse(&indexhi, uint32_t(v>>32));
    // final index
    indexlo = (nzhi ? indexhi+32 : indexlo);
    return 63-indexlo;
#    else
    unsigned long index;
    _BitScanReverse64(&index, v);
    return 63-index;
#    endif
#  else
    cxuint count = 0;
    for (uint64_t t = 1ULL<<63; t > v; t>>=1, count++);
    return count;
#  endif
#endif
}

inline cxuint CTZ32(uint32_t v)
{
#ifdef __GNUC__
    return __builtin_ctz(v);
#else
#  ifdef _MSC_VER
    unsigned long index;
    _BitScanForward(&index, v);
    return index;
#  else
    cxuint count = 0;
    for (uint32_t t = 1U; (t & v) == 0; t<<=1, count++);
    return count;
#  endif
#endif
}

inline cxuint CTZ64(uint64_t v)
{
#ifdef __GNUC__
    return __builtin_ctzll(v);
#else
#  ifdef _MSC_VER
#    ifdef HAVE_ARCH_X86
    unsigned long indexlo, indexhi;
    unsigned char nzlo;
    nzlo = _BitScanForward(&indexlo, uint32_t(v));
    _BitScanForward(&indexhi, uint32_t(v>>32));
    // final index
    indexlo = (nzlo ? indexlo : indexhi+32);
    return indexlo;
#    else
    unsigned long index;
    _BitScanForward64(&index, v);
    return index;
#    endif
#  else
    cxuint count = 0;
    for (uint64_t t = 1ULL; (t & v) == 0; t<<=1, count++);
    return count;
#  endif
#endif
}


/// safely compares sum of two unsigned integers with other unsigned integer
template<typename T, typename T2>
inline bool usumGt(T a, T b, T2 c)
{ return ((a+b)>c) || ((a+b)<a); }

/// safely compares sum of two unsigned integers with other unsigned integer
template<typename T, typename T2>
inline bool usumGe(T a, T b, T2 c)
{ return ((a+b)>=c) || ((a+b)<a); }

/// escapes string into C-style string
extern std::string escapeStringCStyle(size_t strSize, const char* str);

/// escape string into C-style string
inline std::string escapeStringCStyle(const std::string& str)
{ return escapeStringCStyle(str.size(), str.c_str()); }

/// escape string into C-style string
inline std::string escapeStringCStyle(const CString& str)
{ return escapeStringCStyle(str.size(), str.c_str()); }


/// escapes string into C-style string
/**
 * \param strSize string size
 * \param str string
 * \param outMaxSize output max size (including null-character)
 * \param outStr output string
 * \param outSize size of output string
 * \return number of processed input characters
 */
extern size_t escapeStringCStyle(size_t strSize, const char* str,
                 size_t outMaxSize, char* outStr, size_t& outSize);

/// parses unsigned integer regardless locales
/** parses unsigned integer in decimal form from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string (returned even if exception thrown)
 * \return parsed integer value
 */
extern cxuint cstrtoui(const char* str, const char* inend, const char*& outend);

/// parse 64-bit signed integer
extern int64_t cstrtoiXCStyle(const char* str, const char* inend,
             const char*& outend, cxuint bits);

/// parse 64-bit unsigned integer
extern uint64_t cstrtouXCStyle(const char* str, const char* inend,
             const char*& outend, cxuint bits);

/// Unsigned 128-bit integer
struct UInt128
{
    uint64_t lo; ///< low part
    uint64_t hi; ///< high part
};

/// parse 64-bit float value
extern uint64_t cstrtofXCStyle(const char* str, const char* inend,
             const char*& outend, cxuint expBits, cxuint mantisaBits);

/// parse 128-bit unsigned integer
extern UInt128 cstrtou128CStyle(const char* str, const char* inend, const char*& outend);

/* cstrtovcstyle impls */

/// parse cxuchar value from string
template<> inline
cxuchar cstrtovCStyle<cxuchar>(const char* str, const char* inend, const char*& outend)
{ return cstrtouXCStyle(str, inend, outend, sizeof(cxuchar)<<3); }

/// parse cxchar value from string
template<> inline
cxchar cstrtovCStyle<cxchar>(const char* str, const char* inend, const char*& outend)
{ return cstrtoiXCStyle(str, inend, outend, sizeof(cxchar)<<3); }

/// parse cxuint value from string
template<> inline
cxuint cstrtovCStyle<cxuint>(const char* str, const char* inend, const char*& outend)
{ return cstrtouXCStyle(str, inend, outend, sizeof(cxuint)<<3); }

/// parse cxint value from string
template<> inline
cxint cstrtovCStyle<cxint>(const char* str, const char* inend, const char*& outend)
{ return cstrtoiXCStyle(str, inend, outend, sizeof(cxint)<<3); }

/// parse cxushort value from string
template<> inline
cxushort cstrtovCStyle<cxushort>(const char* str, const char* inend, const char*& outend)
{ return cstrtouXCStyle(str, inend, outend, sizeof(cxushort)<<3); }

/// parse cxshort value from string
template<> inline
cxshort cstrtovCStyle<cxshort>(const char* str, const char* inend, const char*& outend)
{ return cstrtoiXCStyle(str, inend, outend, sizeof(cxshort)<<3); }

/// parse cxulong value from string
template<> inline
cxulong cstrtovCStyle<cxulong>(const char* str, const char* inend, const char*& outend)
{ return cstrtouXCStyle(str, inend, outend, sizeof(cxulong)<<3); }

/// parse cxlong value from string
template<> inline
cxlong cstrtovCStyle<cxlong>(const char* str, const char* inend, const char*& outend)
{ return cstrtoiXCStyle(str, inend, outend, sizeof(cxlong)<<3); }

/// parse cxullong value from string
template<> inline
cxullong cstrtovCStyle<cxullong>(const char* str, const char* inend, const char*& outend)
{ return cstrtouXCStyle(str, inend, outend, sizeof(cxullong)<<3); }

/// parse cxllong value from string
template<> inline
cxllong cstrtovCStyle<cxllong>(const char* str, const char* inend, const char*& outend)
{ return cstrtoiXCStyle(str, inend, outend, sizeof(cxllong)<<3); }

/// parse UInt128 value from string
template<> inline
UInt128 cstrtovCStyle<UInt128>(const char* str, const char* inend, const char*& outend)
{ return cstrtou128CStyle(str, inend, outend); }

/// parse float value from string
template<> inline
float cstrtovCStyle<float>(const char* str, const char* inend, const char*& outend)
{
    union {
        float f;
        uint32_t u;
    } v;
    v.u = cstrtofXCStyle(str, inend, outend, 8, 23);
    return v.f;
}

/// parse double value from string
template<> inline
double cstrtovCStyle<double>(const char* str, const char* inend, const char*& outend)
{
    union {
        double d;
        uint64_t u;
    } v;
    v.u = cstrtofXCStyle(str, inend, outend, 11, 52);
    return v.d;
}

/// parses half float formatted looks like C-style
/** parses half floating point from str string. inend can points
 * to end of string or can be null. Function throws ParseException when number in string
 * is out of range, when string does not have number or inend points to string.
 * Function accepts decimal format and binary format. Result is rounded to nearest even
 * (if two values are equally close will be choosen a even value).
 * Currently only IEEE-754 format is supported.
 * \param str input string pointer
 * \param inend pointer points to end of string or null if not end specified
 * \param outend returns end of number in string (returned even if exception thrown)
 * \return parsed floating point value
 */
cxushort cstrtohCStyle(const char* str, const char* inend, const char*& outend);

/// parse half value from string
inline cxushort cstrtohCStyle(const char* str, const char* inend, const char*& outend)
{ return cstrtofXCStyle(str, inend, outend, 5, 10); }

/// format unsigned value to string
extern size_t uXtocstrCStyle(uint64_t value, char* str, size_t maxSize, cxuint radix,
            cxuint width, bool prefix);

/// formast signed value to string
extern size_t iXtocstrCStyle(int64_t value, char* str, size_t maxSize, cxuint radix,
            cxuint width, bool prefix);

/// formats an integer
/** formats an integer in C-style formatting.
 * \param value integer value
 * \param str output string
 * \param maxSize max size of string (including null-character)
 * \param radix radix of digits (2, 8, 10, 16)
 * \param width max number of digits in number
 * \param prefix adds required prefix if true
 * \return length of output string (excluding null-character)
 */
template<typename T>
extern size_t itocstrCStyle(T value, char* str, size_t maxSize, cxuint radix = 10,
       cxuint width = 0, bool prefix = true);

/// format cxuchar value to string
template<> inline
size_t itocstrCStyle<cxuchar>(cxuchar value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return uXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxchar value to string
template<> inline
size_t itocstrCStyle<cxchar>(cxchar value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return iXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxushort value to string
template<> inline
size_t itocstrCStyle<cxushort>(cxushort value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return uXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxshort value to string
template<> inline
size_t itocstrCStyle<cxshort>(cxshort value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return iXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxuint value to string
template<> inline
size_t itocstrCStyle<cxuint>(cxuint value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return uXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxint value to string
template<> inline
size_t itocstrCStyle<cxint>(cxint value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return iXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxulong value to string
template<> inline
size_t itocstrCStyle<cxulong>(cxulong value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return uXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxlong value to string
template<> inline
size_t itocstrCStyle<cxlong>(cxlong value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return iXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxullong value to string
template<> inline
size_t itocstrCStyle<cxullong>(cxullong value, char* str, size_t maxSize, cxuint radix,
       cxuint width , bool prefix)
{ return uXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format cxllong value to string
template<> inline
size_t itocstrCStyle<cxllong>(cxllong value, char* str, size_t maxSize, cxuint radix,
       cxuint width, bool prefix)
{ return iXtocstrCStyle(value, str, maxSize, radix, width, prefix); }

/// format float value to string
extern size_t fXtocstrCStyle(uint64_t value, char* str, size_t maxSize,
        bool scientific, cxuint expBits, cxuint mantisaBits);

/// formats half float in C-style
/** formats to string the half float in C-style formatting. This function handles 2 modes
 * of printing value: human readable and scientific. Scientific mode forces form with
 * decimal exponent.
 * Currently only IEEE-754 format is supported.
 * \param value float value
 * \param str output string
 * \param maxSize max size of string (including null-character)
 * \param scientific enable scientific mode
 * \return length of output string (excluding null-character)
 */
size_t htocstrCStyle(cxushort value, char* str, size_t maxSize,
                            bool scientific = false);

inline size_t htocstrCStyle(cxushort value, char* str, size_t maxSize, bool scientific)
{ return fXtocstrCStyle(value, str, maxSize, scientific, 5, 10); }

/// formats single float in C-style
/** formats to string the single float in C-style formatting. This function handles 2 modes
 * of printing value: human readable and scientific. Scientific mode forces form with
 * decimal exponent.
 * Currently only IEEE-754 format is supported.
 * \param value float value
 * \param str output string
 * \param maxSize max size of string (including null-character)
 * \param scientific enable scientific mode
 * \return length of output string (excluding null-character)
 */
size_t ftocstrCStyle(float value, char* str, size_t maxSize,
                            bool scientific = false);

inline size_t ftocstrCStyle(float value, char* str, size_t maxSize, bool scientific)
{
    union {
        float f;
        uint32_t u;
    } v;
    v.f = value;
    return fXtocstrCStyle(v.u, str, maxSize, scientific, 8, 23);
}

/// formats double float in C-style
/** formats to string the double float in C-style formatting. This function handles 2 modes
 * of printing value: human readable and scientific. Scientific mode forces form with
 * decimal exponent.
 * Currently only IEEE-754 format is supported.
 * \param value float value
 * \param str output string
 * \param maxSize max size of string (including null-character)
 * \param scientific enable scientific mode
 * \return length of output string (excluding null-character)
 */
size_t dtocstrCStyle(double value, char* str, size_t maxSize,
                            bool scientific = false);

inline size_t dtocstrCStyle(double value, char* str, size_t maxSize, bool scientific)
{
    union {
        double d;
        uint64_t u;
    } v;
    v.d = value;
    return fXtocstrCStyle(v.u, str, maxSize, scientific, 11, 52);
}

/* file system utilities */

/// returns true if path refers to directory
extern bool isDirectory(const char* path);
/// returns true if file exists
extern bool isFileExists(const char* path);

/// load data from file (any regular or pipe or device)
/**
 * \param filename filename
 * \return array of data (can be greater than size of data from file)
 */
extern Array<cxbyte> loadDataFromFile(const char* filename);

/// convert to filesystem from unified path (with slashes)
extern void filesystemPath(char* path);
/// convert to filesystem from unified path (with slashes)
extern void filesystemPath(std::string& path);

/// join two paths
extern std::string joinPaths(const std::string& path1, const std::string& path2);

/// get file timestamp in nanosecond since Unix epoch
extern uint64_t getFileTimestamp(const char* filename);

/// get user's home directory
extern std::string getHomeDir();
/// create directory
extern void makeDir(const char* dirname);
/// run executable with output, returns array of output
extern Array<cxbyte> runExecWithOutput(const char* program, const char** argv);

/// find amdocl library, returns path if found, otherwise returns empty string
extern std::string findAmdOCL();

/// find Mesa OpenCL library, returns path if found, otherwise returns empty string
extern std::string findMesaOCL();

/// find LLVM config, returns path if found, otherwise returns empty string
extern std::string findLLVMConfig();

/*
 * Reference support
 */

/// reference countable object
class RefCountable
{
private:
    mutable std::atomic<size_t> refCount;
public:
    /// constructor
    RefCountable() : refCount(1)
    { }
    
    /// reference object
    void reference() const
    {
        refCount.fetch_add(1);
    }
    
    /// unreference object (returns true if no reference count)
    bool unreference() const
    {
        return (refCount.fetch_sub(1) == 1);
    }
};

/// reference countable object (only for single threading usage)
class FastRefCountable
{
private:
    mutable size_t refCount;
public:
    /// constructor
    FastRefCountable() : refCount(1)
    { }
    
    /// reference object
    void reference() const
    {
        refCount++;
    }
    
    /// unreference object (returns true if no reference count)
    bool unreference() const
    {
        return (--refCount == 0);
    }
};

/// reference pointer based on Glibmm refptr
template<typename T>
class RefPtr
{
private:
    T* ptr;
public:
    /// empty constructor
    RefPtr(): ptr(nullptr)
    { }
    
    /// constructor from pointer
    explicit RefPtr(T* inputPtr) : ptr(inputPtr)
    { }
    
    /// copy constructor
    RefPtr(const RefPtr<T>& refPtr) : ptr(refPtr.ptr)
    {
        if (ptr != nullptr)
            ptr->reference();
    }
    
    /// move constructor
    RefPtr(RefPtr<T>&& refPtr) : ptr(refPtr.ptr)
    { refPtr.ptr = nullptr; }
    
    /// destructor
    ~RefPtr()
    {
        if (ptr != nullptr)
            if (ptr->unreference())
                delete ptr;
    }
    
    /// copy constructor
    RefPtr<T>& operator=(const RefPtr<T>& refPtr)
    {
        if (ptr != nullptr)
            if (ptr->unreference())
                delete ptr;
        if (refPtr.ptr != nullptr)
            refPtr.ptr->reference();
        ptr = refPtr.ptr;
        return *this;
    }
    /// move constructor
    RefPtr<T>& operator=(RefPtr<T>&& refPtr)
    {
        if (ptr != nullptr)
            if (ptr->unreference())
                delete ptr;
        ptr = refPtr.ptr;
        refPtr.ptr = nullptr;
        return *this;
    }
    
    /// equality operator
    bool operator==(const RefPtr<T>& refPtr) const
    { return ptr == refPtr.ptr; }
    /// unequality operator
    bool operator!=(const RefPtr<T>& refPtr) const
    { return ptr != refPtr.ptr; }
    
    /// return true if not null
    operator bool() const
    { return ptr!=nullptr; }
    
    /// return true if null
    bool operator!() const
    { return ptr==nullptr; }
    
    /// get elem from pointer
    T* operator->() const
    { return ptr; }
    
    /// get elem
    T* get() const
    { return ptr; }
    
    /// reset refpointer
    void reset()
    {
        if (ptr != nullptr)
            if (ptr->unreference())
                delete ptr;
        ptr = nullptr;
    }
    
    /// swap between refpointers
    void swap(RefPtr<T>& refPtr)
    {
        T* tmp = ptr;
        ptr = refPtr.ptr;
        refPtr.ptr = tmp;
    }
    
    /// const cast
    template<typename DestType>
    RefPtr<DestType> constCast() const
    {
        DestType* p = const_cast<DestType*>(ptr);
        if (p != nullptr)
            p->reference();
        return RefPtr<DestType>(p);
    }
    /// static cast
    template<typename DestType>
    RefPtr<DestType> staticCast() const
    {
        DestType* p = static_cast<DestType*>(ptr);
        if (p != nullptr)
            p->reference();
        return RefPtr<DestType>(p);
    }
    /// dynamic cast
    template<typename DestType>
    RefPtr<DestType> dynamicCast() const
    {
        DestType* p = dynamic_cast<DestType*>(ptr);
        if (p != nullptr)
            p->reference();
        return RefPtr<DestType>(p);
    }
};

/// convert character to lowercase
inline char toLower(char c);

inline char toLower(char c)
{ return  (c >= 'A' &&  c <= 'Z') ? c - 'A' + 'a' : c; }

/// convert string to lowercase
inline void toLowerString(std::string& string);

inline void toLowerString(std::string& string)
{ std::transform(string.begin(), string.end(), string.begin(), toLower); }

/// convert string to lowercase
inline void toLowerString(char* cstr);

inline void toLowerString(char* cstr)
{
    for (; *cstr!=0; cstr++)
        *cstr = toLower(*cstr);
}

/// convert string to lowercase
inline void toLowerString(CString& string);

inline void toLowerString(CString& string)
{ if (!string.empty())
    toLowerString(string.begin()); }

/// convert character to uppercase
inline char toUpper(char c);

inline char toUpper(char c)
{ return  (c >= 'a' &&  c <= 'z') ? c - 'a' + 'A' : c; }

/// convert string to uppercase
inline void toUpperString(std::string& string);

inline void toUpperString(std::string& string)
{ std::transform(string.begin(), string.end(), string.begin(), toUpper); }

/// convert string to uppercase
inline void toUpperString(char* cstr);

inline void toUpperString(char* cstr)
{
    for (; *cstr!=0; cstr++)
        *cstr = toUpper(*cstr);
}

/// convert string to uppercase
inline void toUpperString(CString& string);

inline void toUpperString(CString& string)
{ if (!string.empty())
    toUpperString(string.begin()); }

/* CALL once */

#ifdef HAVE_CALL_ONCE
/// Once flag type (wrapper for std::once_flag)
typedef std::once_flag OnceFlag;

/// callOnce - portable wrapper for std::call_once
template<class Callable, class... Args>
inline void callOnce(std::once_flag& flag, Callable&& f, Args&&... args)
{ std::call_once(flag, f, args...); }
#else
struct OnceFlag: std::atomic<int>
{
    // force zero initialization
    OnceFlag(): std::atomic<int>(0)
    { }
};

template<class Callable, class... Args>
inline void callOnce(OnceFlag& flag, Callable&& f, Args&&... args)
{
    if (flag.exchange(1) == 0)
        f(args...);
}
#endif

};

#endif
