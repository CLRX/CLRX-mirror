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
#ifdef HAVE_LINUX
#include <dlfcn.h>
#endif
#include <mutex>
#include <cstring>
#include <climits>
#define __UTILITIES_MODULE__ 1
#include <CLRX/Utilities.h>

using namespace CLRX;

Exception::Exception(const std::string& message)
{
    this->message = message;
}

const char* Exception::what() const throw()
{
    return message.c_str();
}

ParseException::ParseException(size_t lineNo, const std::string& message)
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << lineNo << ": " << message;
    oss.flush();
    this->message = oss.str();
}

#ifndef HAVE_LINUX
#error "Other platforms than Linux not supported"
#endif

std::mutex CLRX::DynLibrary::mutex;

DynLibrary::DynLibrary()
{
    handle = nullptr;
}

DynLibrary::DynLibrary(const char* filename, cxuint flags)
{
    handle = nullptr;
    load(filename, flags);
}

DynLibrary::~DynLibrary()
{
    unload();
}

void DynLibrary::load(const char* filename, cxuint flags)
{
    std::lock_guard<std::mutex> lock(mutex);
#ifdef HAVE_LINUX
    dlerror(); // clear old errors
    cxuint outFlags = (flags & DYNLIB_GLOBAL) ? RTLD_GLOBAL : RTLD_LOCAL;
    if ((flags & DYNLIB_MODE1_MASK) == DYNLIB_LAZY)
        outFlags |= RTLD_LAZY;
    if ((flags & DYNLIB_MODE1_MASK) == DYNLIB_NOW)
        outFlags |= RTLD_NOW;
    handle = dlopen(filename, outFlags);
    if (handle == nullptr)
        throw Exception(dlerror());
#endif
}

void DynLibrary::unload()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (handle != nullptr)
    {
#ifdef HAVE_LINUX
        dlerror(); // clear old errors
        if (dlclose(handle)) // if closing failed
            throw Exception(dlerror());
#endif
    }
    handle = nullptr;
}

void* DynLibrary::getSymbol(const std::string& symbolName)
{
    if (handle == nullptr)
        throw Exception("DynLibrary not loaded!");
    
    std::lock_guard<std::mutex> lock(mutex);
    void* symbol = nullptr;
#ifdef HAVE_LINUX
    dlerror(); // clear old errors
    symbol = dlsym(handle, symbolName.c_str());
    const char* error = dlerror();
    if (symbol == nullptr && error != nullptr)
        throw Exception(error);
#endif
    return symbol;
}

template
cxuint CLRX::parseEnvVariable<cxuint>(const char* envVar,
               const cxuint& defaultValue);

template
cxint CLRX::parseEnvVariable<cxint>(const char* envVar,
               const cxint& defaultValue);

template
cxushort CLRX::parseEnvVariable<cxushort>(const char* envVar,
               const cxushort& defaultValue);

template
cxshort CLRX::parseEnvVariable<cxshort>(const char* envVar,
               const cxshort& defaultValue);

template
cxulong CLRX::parseEnvVariable<cxulong>(const char* envVar,
               const cxulong& defaultValue);

template
cxlong CLRX::parseEnvVariable<cxlong>(const char* envVar,
               const cxlong& defaultValue);

template
cxullong CLRX::parseEnvVariable<cxullong>(const char* envVar,
               const cxullong& defaultValue);

template
cxllong CLRX::parseEnvVariable<cxllong>(const char* envVar,
               const cxllong& defaultValue);

namespace CLRX
{
template<>
std::string parseEnvVariable<std::string>(const char* envVar,
              const std::string& defaultValue)
{
    const char* var = getenv(envVar);
    if (var == nullptr)
        return defaultValue;
    return var;
}

template<>
bool parseEnvVariable<bool>(const char* envVar, const bool& defaultValue)
{
    const char* var = getenv(envVar);
    if (var == nullptr)
        return defaultValue;
    
    // skip spaces
    for (; *var == ' ' || *var == '\t' || *var == '\r'; var++);
    
    for (const char* v: { "1", "true", "TRUE", "ON", "on", "YES", "yes"})
        if (strncmp(var, v, strlen(v)) == 0)
            return true;
    return false; // false
}
};

template
std::string CLRX::parseEnvVariable<std::string>(const char* envVar,
              const std::string& defaultValue);

template
bool CLRX::parseEnvVariable<bool>(const char* envVar, const bool& defaultValue);


cxuint CLRX::cstrtoui(const char* s, const char*& end)
{
    uint64_t out = 0;
    const char* p;
    for (p = s; *p >= '0' && *p <= '9'; p++)
    {
        out = (out*10U) + *p-'0';
        if (out > UINT_MAX)
            throw Exception("UInt out of range");
    }
    end = p;
    return out;
}

cxuint CLRX::cstrtouiParse(const char* s, const char* inend, const char*& outend,
                     char delim, size_t lineNo)
{
    uint64_t out = 0;
    const char* p;
    for (p = s; p != inend && *p >= '0' && *p <= '9'; p++)
    {
        out = (out*10U) + *p-'0';
        if (out > UINT_MAX)
            throw ParseException(lineNo, "UInt out of range");
    }
    if (*p == 0)
        throw ParseException(lineNo, "No delimiter");
    if (*p != delim)
        throw ParseException(lineNo, "Wrong delimiter");
    outend = p;
    return out;
}

size_t CStringHash::operator()(const char* c) const
{
    if (c == nullptr)
        return 0;
    size_t hash = 0;
    
    for (const char* p = c; *p != 0; p++)
        hash = ((hash<<8)^(uint8_t)*p)*size_t(0x93cda145bf146a3dU);
    return hash;
}
