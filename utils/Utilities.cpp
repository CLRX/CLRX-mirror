/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <mutex>
#include <cerrno>
#include <cstring>
#include <string>
#include <climits>
#define __UTILITIES_MODULE__ 1
#include <CLRX/utils/Utilities.h>

using namespace CLRX;

Exception::Exception(const std::string& _message) : message(_message)
{ }

const char* Exception::what() const noexcept
{
    return message.c_str();
}

ParseException::ParseException(const std::string& message) : Exception(message)
{ }

ParseException::ParseException(LineNo lineNo, const std::string& message)
{
    char buf[32];
    itocstrCStyle(lineNo, buf, 32);
    this->message = buf;
    this->message += ": ";
    this->message += message;
}

ParseException::ParseException(LineNo lineNo, ColNo charNo, const std::string& message)
{
    char buf[32];
    itocstrCStyle(lineNo, buf, 32);
    this->message = buf;
    this->message += ": ";
    itocstrCStyle(charNo, buf, 32);
    this->message += buf;
    this->message += ": ";
    this->message += message;
}

#ifndef HAVE_LINUX
#error "Other platforms than Linux not supported"
#endif

std::mutex CLRX::DynLibrary::mutex;

DynLibrary::DynLibrary() : handle(nullptr)
{ }

DynLibrary::DynLibrary(const char* filename, Flags flags) : handle(nullptr)
{
    load(filename, flags);
}

DynLibrary::~DynLibrary()
{
    unload();
}

void DynLibrary::load(const char* filename, Flags flags)
{
    std::lock_guard<std::mutex> lock(mutex);
#ifdef HAVE_LINUX
    dlerror(); // clear old errors
    int outFlags = (flags & DYNLIB_GLOBAL) ? RTLD_GLOBAL : RTLD_LOCAL;
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

void* DynLibrary::getSymbol(const char* symbolName)
{
    if (handle == nullptr)
        throw Exception("DynLibrary not loaded!");
    
    std::lock_guard<std::mutex> lock(mutex);
    void* symbol = nullptr;
#ifdef HAVE_LINUX
    dlerror(); // clear old errors
    symbol = dlsym(handle, symbolName);
    const char* error = dlerror();
    if (symbol == nullptr && error != nullptr)
        throw Exception(error);
#endif
    return symbol;
}

template
cxuchar CLRX::parseEnvVariable<cxuchar>(const char* envVar,
                const cxuchar& defaultValue);

template
cxchar CLRX::parseEnvVariable<cxchar>(const char* envVar,
              const cxchar& defaultValue);


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

template
float CLRX::parseEnvVariable<float>(const char* envVar,
            const float& defaultValue);

template
double CLRX::parseEnvVariable<double>(const char* envVar,
              const double& defaultValue);


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
    
    for (const char* v: { "1", "true", "t", "on", "yes", "y"})
        if (::strncasecmp(var, v, ::strlen(v)) == 0)
            return true;
    return false; // false
}
};

template
std::string CLRX::parseEnvVariable<std::string>(const char* envVar,
              const std::string& defaultValue);

template
bool CLRX::parseEnvVariable<bool>(const char* envVar, const bool& defaultValue);

static const char cstyleEscapesTable[32] =
{
    0, 0, 0, 0, 0, 0, 0, 'a', 'b', 't', 'n', 'v', 'f', 'r', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

std::string CLRX::escapeStringCStyle(size_t strSize, const char* str)
{
    std::string out;
    out.reserve(strSize);
    bool notFullOctalEscape = false;
    for (size_t i = 0; i < strSize; i++)
    {
        const cxbyte c = str[i];
        if (c < 0x20 || c > 0x7e || (notFullOctalEscape && c >= 0x30 && c <= 0x37))
        {
            if (c < 0x20 && cstyleEscapesTable[c] != 0)
            {
                out.push_back('\\');
                out.push_back(cstyleEscapesTable[c]);
                notFullOctalEscape = false;
            }
            else // otherwise
            {
                out.push_back('\\');
                if ((c>>6) != 0)
                    out.push_back('0'+(c>>6));
                if ((c>>3) != 0)
                    out.push_back('0'+((c>>3)&7));
                out.push_back('0'+(c&7));
                if ((c>>6) == 0) // if next character can change octal escape
                    notFullOctalEscape = true;
            }
        }
        else  if (c == '\"')
        {   // backslash
            out.push_back('\\');
            out.push_back('\"');
            notFullOctalEscape = false;
        }
        else  if (c == '\'')
        {   // backslash
            out.push_back('\\');
            out.push_back('\'');
            notFullOctalEscape = false;
        }
        else  if (c == '\\')
        {   // backslash
            out.push_back('\\');
            out.push_back('\\');
            notFullOctalEscape = false;
        }
        else // otherwise normal character
        {
            out.push_back(c);
            notFullOctalEscape = false;
        }
    }
    return out;
}

size_t CLRX::escapeStringCStyle(size_t strSize, const char* str,
                 size_t outMaxSize, char* outStr, size_t& outSize)
{
    size_t i = 0, d = 0;
    bool notFullOctalEscape = false;
    for (i = 0; i < strSize; i++)
    {
        const cxbyte c = str[i];
        if (c < 0x20 || c > 0x7e || (notFullOctalEscape && c >= 0x30 && c <= 0x37))
        {
            if (c < 0x20 && cstyleEscapesTable[c] != 0)
            {
                if (d+2 >= outMaxSize)
                    break; // end
                outStr[d++] = '\\';
                outStr[d++] = cstyleEscapesTable[c];
                notFullOctalEscape = false;
            }
            else // otherwise
            {
                if (d + 2 + ((c>>3) != 0) + ((c>>6) != 0) >= outMaxSize)
                    break; // end
                outStr[d++] = '\\';
                if ((c>>6) != 0)
                    outStr[d++] = '0'+(c>>6);
                if ((c>>3) != 0)
                    outStr[d++] = '0'+((c>>3)&7);
                outStr[d++] = '0'+(c&7);
                if ((c>>6) == 0) // if next character can change octal escape
                    notFullOctalEscape = true;
            }
        }
        else  if (c == '\"')
        {   // backslash
            if (d+2 >= outMaxSize)
                break; // end
            outStr[d++] = '\\';
            outStr[d++] = '\"';
            notFullOctalEscape = false;
        }
        else  if (c == '\'')
        {   // backslash
            if (d+2 >= outMaxSize)
                break; // end
            outStr[d++] = '\\';
            outStr[d++] = '\'';
            notFullOctalEscape = false;
        }
        else  if (c == '\\')
        {   // backslash
            if (d+2 >= outMaxSize)
                break; // end
            outStr[d++] = '\\';
            outStr[d++] = '\\';
            notFullOctalEscape = false;
        }
        else // otherwise normal character
        {
            if (d+1 >= outMaxSize)
                break; // end
            outStr[d++] = c;
            notFullOctalEscape = false;
        }
    }
    outStr[d] = 0;
    outSize = d;
    return i;
}

bool CLRX::isDirectory(const char* path)
{
    struct stat stBuf;
    errno = 0;
    if (stat(path, &stBuf) != 0)
    {
        if (errno == ENOENT)
            throw Exception("File or directory doesn't exists");
        else if (errno == EACCES)
            throw Exception("Access to file or directory is not permitted");
        else
            throw Exception("Can't determine whether path refers to directory");
    }
    return S_ISDIR(stBuf.st_mode);
}

Array<cxbyte> CLRX::loadDataFromFile(const char* filename)
{
    uint64_t size;
    if (isDirectory(filename))
        throw Exception("This is directory!");
    
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        throw Exception("Can't open file");
    ifs.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    
    bool seekingIsWorking = true;
    try
    { ifs.seekg(0, std::ios::end); /* to end of file */ }
    catch(const std::exception& ex)
    {   /* oh, no! this is not regular file */
        seekingIsWorking = false;
        ifs.clear();
    }
    Array<cxbyte> buf;
    ifs.exceptions(std::ifstream::badbit); // ignore failbit for read
    if (seekingIsWorking)
    {
        size = ifs.tellg();
        if (size > SIZE_MAX)
            throw Exception("File is too big to load");
        ifs.seekg(0, std::ios::beg);
        buf.resize(size);
        ifs.read((char*)buf.data(), size);
        if (ifs.gcount() != std::streamsize(size))
            throw Exception("Can't read whole file");
    }
    else
    {   /* growing, growing... */
        size_t prevBufSize = 0;
        size_t readBufSize = 256;
        buf.resize(readBufSize);
        while(true)
        {
            ifs.read((char*)(buf.data()+prevBufSize), readBufSize-prevBufSize);
            const size_t readed = ifs.gcount();
            if (readed < readBufSize-prevBufSize)
            {   /* final */
                buf.resize(prevBufSize + readed);
                break;
            }
            prevBufSize = readBufSize;
            readBufSize = prevBufSize+(prevBufSize>>1);
            buf.resize(readBufSize);
        }
    }
    return buf;
}

void CLRX::filesystemPath(char* path)
{
    while (*path != 0)  // change to native dir separator
        if (*path == CLRX_ALT_DIR_SEP)
            *path = CLRX_NATIVE_DIR_SEP;
}

void CLRX::filesystemPath(std::string& path)
{
    for (char& c: path)
        if (c == CLRX_ALT_DIR_SEP)
            c = CLRX_NATIVE_DIR_SEP;
}

std::string CLRX::joinPaths(const std::string& path1, const std::string& path2)
{
    std::string outPath = path1;
    if (path1.back() == CLRX_NATIVE_DIR_SEP)
    {
        if (path2.size() >= 1 && path2.front() == CLRX_NATIVE_DIR_SEP)
            // skip first dir separator
            outPath.append(path2.begin()+1, path2.end());
        else
            outPath += path2;
    }
    else
    {
        if (path2.size() >= 1 && path2.front() != CLRX_NATIVE_DIR_SEP)
            outPath.push_back(CLRX_NATIVE_DIR_SEP); // add dir separator
        outPath += path2;
    }
    return outPath;
}

uint64_t CLRX::getFileTimestamp(const char* filename)
{
    struct stat stBuf;
    errno = 0;
    if (stat(filename, &stBuf) != 0)
    {
        if (errno == ENOENT)
            throw Exception("File or directory doesn't exists");
        else if (errno == EACCES)
            throw Exception("Access to file or directory is not permitted");
        else
            throw Exception("Can't determine whether path refers to directory");
    }
    return stBuf.st_mtim.tv_sec*1000000000ULL + stBuf.st_mtim.tv_nsec;
}
