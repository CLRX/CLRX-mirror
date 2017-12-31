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
#if defined(HAVE_LINUX) || defined(HAVE_BSD)
#include <dlfcn.h>
#endif
#ifdef HAVE_WINDOWS
#include <direct.h>
#include <windows.h>
#include <shlobj.h>
#else
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
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

/* dynamic library routines can be non-reentrant and non-multithreaded,
 * then use global mutex   to secure these operations */
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
    // if already loaded
    if (handle != nullptr)
        throw Exception("DynLibrary already loaded");
#if defined(HAVE_LINUX) || defined(HAVE_BSD)
    // clear old errors
    dlerror();
    int outFlags = (flags & DYNLIB_GLOBAL) ? RTLD_GLOBAL : RTLD_LOCAL;
    if ((flags & DYNLIB_MODE1_MASK) == DYNLIB_LAZY)
        outFlags |= RTLD_LAZY;
    if ((flags & DYNLIB_MODE1_MASK) == DYNLIB_NOW)
        outFlags |= RTLD_NOW;
    handle = dlopen(filename, outFlags);
    if (handle == nullptr)
        throw Exception(dlerror());
#endif
#ifdef HAVE_WINDOWS
    handle = (void*)LoadLibrary(filename);
    if (handle == nullptr)
        throw Exception("DynLibrary::load failed");
#endif
}

void DynLibrary::unload()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (handle != nullptr)
    {
#if defined(HAVE_LINUX) || defined(HAVE_BSD)
        dlerror(); // clear old errors
        if (dlclose(handle)) // if closing failed
            throw Exception(dlerror());
#endif
#ifdef HAVE_WINDOWS
        if (!FreeLibrary((HMODULE)handle))
            throw Exception("DynLibrary::unload failed");
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
#if defined(HAVE_LINUX) || defined(HAVE_BSD)
    dlerror(); // clear old errors
    symbol = dlsym(handle, symbolName);
    const char* error = dlerror();
    if (symbol == nullptr && error != nullptr)
        throw Exception(error);
#endif
#ifdef HAVE_WINDOWS
    symbol = (void*)GetProcAddress((HMODULE)handle, symbolName);
    if (symbol==nullptr)
        throw Exception("DynLibrary::getSymbol failed");
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

/// escape char names without '\'
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
                // if next character can change octal escape
                notFullOctalEscape = ((c>>6) == 0);
            }
        }
        else  if (c == '\"')
        {
            // backslash
            out.push_back('\\');
            out.push_back('\"');
            notFullOctalEscape = false;
        }
        else  if (c == '\'')
        {
            // backslash
            out.push_back('\\');
            out.push_back('\'');
            notFullOctalEscape = false;
        }
        else  if (c == '\\')
        {
            // backslash
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
                // if next character can change octal escape
                notFullOctalEscape = ((c>>6) == 0);
            }
        }
        else  if (c == '\"')
        {
            // backslash
            if (d+2 >= outMaxSize)
                break; // end
            outStr[d++] = '\\';
            outStr[d++] = '\"';
            notFullOctalEscape = false;
        }
        else  if (c == '\'')
        {
            // backslash
            if (d+2 >= outMaxSize)
                break; // end
            outStr[d++] = '\\';
            outStr[d++] = '\'';
            notFullOctalEscape = false;
        }
        else  if (c == '\\')
        {
            // backslash
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
    if (::stat(path, &stBuf) != 0)
    {
        if (errno == ENOENT)
            throw Exception("File or directory doesn't exists");
        else if (errno == EACCES)
            throw Exception("Access to file or directory is not permitted");
        else
            throw Exception("Can't determine whether path refers to directory");
    }
#ifdef HAVE_WINDOWS
    return (_S_IFDIR&stBuf.st_mode)!=0;
#else
    return S_ISDIR(stBuf.st_mode);
#endif
}

/// returns true if file exists
bool CLRX::isFileExists(const char* path)
{
#ifdef HAVE_WINDOWS
    return GetFileAttributes(path)!=INVALID_FILE_ATTRIBUTES ||
            GetLastError()!=ERROR_FILE_NOT_FOUND;
#else
    return ::access(path, F_OK)==0;
#endif
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
    {
        /* oh, no! this is not regular file */
        seekingIsWorking = false;
        ifs.clear();
    }
    Array<cxbyte> buf;
    ifs.exceptions(std::ifstream::badbit); // ignore failbit for read
    if (seekingIsWorking)
    {
        // just read whole file to memory
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
    {
        /* growing, growing... */
        size_t prevBufSize = 0;
        size_t readBufSize = 256;
        buf.resize(readBufSize);
        while(true)
        {
            ifs.read((char*)(buf.data()+prevBufSize), readBufSize-prevBufSize);
            const size_t readed = ifs.gcount();
            if (readed < readBufSize-prevBufSize)
            {
                /* final resize */
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
    if (::stat(filename, &stBuf) != 0)
    {
        if (errno == ENOENT)
            throw Exception("File or directory doesn't exists");
        else if (errno == EACCES)
            throw Exception("Access to file or directory is not permitted");
        else
            throw Exception("Can't determine whether path refers to directory");
    }
#if _POSIX_C_SOURCE>=200800L && !defined(HAVE_MINGW)
    return stBuf.st_mtim.tv_sec*1000000000ULL + stBuf.st_mtim.tv_nsec;
#else
    return stBuf.st_mtime*1000000000ULL;
#endif
}

std::string CLRX::getHomeDir()
{
#ifndef HAVE_WINDOWS
    struct passwd pw;
    long pwbufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (pwbufSize != -1)
    {
        struct passwd* pwres;
        Array<char> pwbuf(pwbufSize);
        if (getpwuid_r(getuid(), &pw, pwbuf.data(), pwbufSize, &pwres)==0 &&
                pwres!=nullptr)
            return std::string(pwres->pw_dir);
    }
#else
    // requires Windows XP or Windows 2000
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_PROFILE, nullptr, 0, path)))
        return std::string(path);
#endif
    return "";
}

void CLRX::makeDir(const char* dirname)
{
    errno = 0;
#ifdef HAVE_WINDOWS
    int ret = _mkdir(dirname);
#else
    // warning this is not thread safe code! (umask)
    int um = umask(0);
    umask(um);
    int ret = ::mkdir(dirname, 0777&~um);
#endif
    if (ret!=0)
    {
        if (errno == EEXIST)
            throw Exception("Directory already exists");
        else if (errno == ENOENT)
            throw Exception("No such parent directory");
        else if (errno == EACCES)
            throw Exception("Access to parent directory is not permitted");
        else
            throw Exception("Can't create directory");
    }
}

CLRX::Array<cxbyte> CLRX::runExecWithOutput(const char* program, const char** argv)
{
#ifndef HAVE_WINDOWS
    Array<cxbyte> output;
    errno = 0;
    int pipefds[2];
    if (::pipe(pipefds) < 0)
        throw Exception("Can't create pipe");
    int fret = fork();
    if (fret == 0)
    {
        // if children
        if (::close(1)<0)
            ::exit(-1);
        if (::dup2(pipefds[1], 1)<0) // redirection to pipe
            ::exit(-1);
        ::close(pipefds[0]);
        ::execvp(program, (char**)argv);
        ::exit(-1); // not
    }
    else if (fret > 0)
    {
        // if parent (this process)
        ::close(pipefds[1]);
        /* growing, growing... */
        size_t prevBufSize = 0;
        size_t readBufSize = 256;
        output.resize(readBufSize);
        try
        {
        // reading output from children
        while(true)
        {
            while (prevBufSize < readBufSize)
            {
                ssize_t ret = ::read(pipefds[0], output.data()+prevBufSize,
                            readBufSize-prevBufSize);
                if (ret < 0)
                    throw Exception("Can't read output of running process");
                prevBufSize += ret;
                if (ret == 0)
                    break;
            }
            if (prevBufSize < readBufSize)
            {
                /* final resize */
                output.resize(prevBufSize);
                break;
            }
            prevBufSize = readBufSize;
            readBufSize = prevBufSize+(prevBufSize>>1);
            output.resize(readBufSize);
        }
        }
        catch(...)
        {
            // close on error
            ::waitpid(fret, nullptr, 0);
            ::close(pipefds[0]);
            throw;
        }
        int status = 0;
        ::waitpid(fret, &status, 0);
        ::close(pipefds[0]);
        // if children exited unsuccessfully (terminated or error code returned)
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            throw Exception("Process exited abnormally");
    }
    else
        throw Exception("Can't create new process");
    return output;
#else
    throw Exception("Unsupported function");
#endif
}

static std::string findFileByEnvPaths(const char* envName, const char* fileName)
{
    // find by path
    const std::string paths = parseEnvVariable<std::string>(envName);
    for (size_t i = 0; i != std::string::npos; )
    {
#ifndef HAVE_WINDOWS
        size_t nexti = paths.find(':', i);
#else
        size_t nexti = paths.find(';', i);
#endif
        std::string path;
        if (nexti != std::string::npos)
        {
            // next
            path = paths.substr(i, nexti-i);
            i = nexti+1;
        }
        else
        {
            // last
            path = paths.substr(i);
            i = std::string::npos;
        }
        if (path.empty())
            continue; // skip empty path
        std::string filePath = joinPaths(path, fileName);
        if (isFileExists(filePath.c_str()))
            return filePath;
    }
    return "";
}

static const char* libAmdOCLPaths[] =
{
#ifdef HAVE_LINUX
#  ifdef HAVE_32BIT
     "/usr/lib/i386-linux-gnu/amdgpu-pro",
     "/opt/amdgpu-pro/lib/i386-linux-gnu",
     "/opt/amdgpu-pro/lib32",
     "/opt/amdgpu-pro/lib",
     "/usr/lib/i386-linux-gnu",
     "/usr/lib32",
     "/usr/lib"
#  else
     "/usr/lib/x86_64-linux-gnu/amdgpu-pro",
     "/opt/amdgpu-pro/lib/x86_64-linux-gnu",
     "/usr/lib/x86_64-linux-gnu",
     "/opt/amdgpu-pro/lib64",
     "/opt/amdgpu-pro/lib",
     "/usr/lib64",
     "/usr/lib"
#  endif
#elif defined(HAVE_WINDOWS)
     "c:\\Windows\\System32"
#endif
};

std::string CLRX::findAmdOCL()
{
    std::string amdOclPath = parseEnvVariable<std::string>("CLRX_AMDOCL_PATH", "");
    if (!amdOclPath.empty())
    {
        // AMDOCL library path given in envvar (check only that place)
        if (isFileExists(amdOclPath.c_str()))
            return amdOclPath;
    }
    else
    {
#ifndef HAVE_WINDOWS
        // check from paths in LD_LIBRARY_PATH
        amdOclPath = findFileByEnvPaths("LD_LIBRARY_PATH", DEFAULT_AMDOCLNAME);
        if (!amdOclPath.empty())
            return amdOclPath;
#endif
        for (const char* libPath: libAmdOCLPaths)
        {
            amdOclPath = joinPaths(libPath, DEFAULT_AMDOCLNAME);
            if (isFileExists(amdOclPath.c_str()))
                return amdOclPath;
        }
#ifdef HAVE_WINDOWS
        return findFileByEnvPaths("PATH", DEFAULT_AMDOCLNAME);
#endif
    }
    return "";
}

#ifndef HAVE_WINDOWS
static const char* libMesaOCLPaths[] =
{
#  ifdef HAVE_32BIT
     "/usr/lib/i386-linux-gnu",
     "/usr/lib32",
     "/usr/lib"
#  else
     "/usr/lib/x86_64-linux-gnu",
     "/usr/lib64",
     "/usr/lib"
#  endif
};
#endif

std::string CLRX::findMesaOCL()
{
#ifndef HAVE_WINDOWS
    std::string mesaOclPath = parseEnvVariable<std::string>("CLRX_MESAOCL_PATH", "");
    if (!mesaOclPath.empty())
    {
        // MesaOpenCL library path given in envvar (check only that place)
        if (isFileExists(mesaOclPath.c_str()))
            return mesaOclPath;
    }
    else
    {
        // otherwise find in paths given in LD_LIBRARY_PATH
        mesaOclPath = findFileByEnvPaths("LD_LIBRARY_PATH", "libMesaOpenCL.so.1");
        if (!mesaOclPath.empty())
            return mesaOclPath;
        // otherwise in hardcoded paths
        for (const char* libPath: libMesaOCLPaths)
        {
            mesaOclPath = joinPaths(libPath, "libMesaOpenCL.so.1");
            if (isFileExists(mesaOclPath.c_str()))
                return mesaOclPath;
        }
#ifdef HAVE_64BIT
        if (isFileExists("/usr/lib64/OpenCL/vendors/mesa/libOpenCL.so"))
            return "/usr/lib64/OpenCL/vendors/mesa/libOpenCL.so";
        if (isFileExists("/usr/lib/OpenCL/vendors/mesa/libOpenCL.so"))
            return "/usr/lib/OpenCL/vendors/mesa/libOpenCL.so";
#else
        if (isFileExists("/usr/lib32/OpenCL/vendors/mesa/libOpenCL.so"))
            return "/usr/lib32/OpenCL/vendors/mesa/libOpenCL.so";
        if (isFileExists("/usr/lib/OpenCL/vendors/mesa/libOpenCL.so"))
            return "/usr/lib/OpenCL/vendors/mesa/libOpenCL.so";
#endif
    }
#endif
    return "";
}

std::string CLRX::findLLVMConfig()
{
    std::string llvmConfigPath = parseEnvVariable<std::string>("CLRX_LLVMCONFIG_PATH", "");
    if (!llvmConfigPath.empty())
    {
        // LLVM-config path given in envvar (check only that place)
        if (isFileExists(llvmConfigPath.c_str()))
            return llvmConfigPath;
        return "";
    }
    else
    {
        // find by PATH
        llvmConfigPath = findFileByEnvPaths("PATH", "llvm-config");
        if (!llvmConfigPath.empty())
            return llvmConfigPath;
#ifndef HAVE_WINDOWS
        if (isFileExists("/usr/bin/llvm-config"))
            return "/usr/bin/llvm-config";
#endif
    }
    return "";
}
