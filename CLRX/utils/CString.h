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
/*! \file CString.h
 * \brief C-style string class
 */

#ifndef __CLRX_CSTRING_H__
#define __CLRX_CSTRING_H__

#include <CLRX/Config.h>
#include <algorithm>
#include <cstddef>
#include <string>
#include <ostream>
#include <cstring>
#include <initializer_list>

namespace CLRX
{

/// simple C-string container
class CString
{
public:
    typedef char* iterator;    ///< type of iterator
    typedef const char* const_iterator;    ///< type of constant iterator
    typedef char element_type; ///< element type
    typedef std::string::size_type size_type; ///< size type
    static const size_type npos = -1;   ///< value to indicate no position
private:
    char* ptr;
public:
    /// constructor
    CString(): ptr(nullptr)
    { }
    
    /// constructor from C-style string pointer
    explicit CString(size_t n) : ptr(nullptr)
    {
        if (n == 0) return;
        ptr = new char[n+1];
        ptr[n] = 0;
    }
    
    /// constructor from C-style string pointer
    CString(const char* str) : ptr(nullptr)
    {
        if (str == nullptr)
            return;
        const size_t n = ::strlen(str);
        if (n == 0)
            return;
        ptr = new char[n+1];
        ::memcpy(ptr, str, n);
        ptr[n] = 0;
    }
    
    /// constructor from C++ std::string
    CString(const std::string& str) : ptr(nullptr)
    {
        const size_t n = str.size();
        if (n == 0)
            return;
        ptr = new char[n+1];
        ::memcpy(ptr, str.c_str(), n);
        ptr[n] = 0;
    }
    
    /// constructor
    CString(const char* str, size_t n) : ptr(nullptr)
    {
        if (n == 0)
            return;
        ptr = new char[n+1];
        ::memcpy(ptr, str, n);
        ptr[n] = 0;
    }
    
    /// constructor
    CString(const char* str, const char* end) : ptr(nullptr)
    {
        const size_t n = end-str;
        if (n == 0)
            return;
        ptr = new char[n+1];
        ::memcpy(ptr, str, n);
        ptr[n] = 0;
    }
    
    /// constructor
    CString(size_t n, char ch) : ptr(nullptr)
    {
        if (n == 0)
            return;
        ptr = new char[n+1];
        ::memset(ptr, ch, n);
        ptr[n] = 0;
    }
    
    /// copy-constructor
    CString(const CString& cstr) : ptr(nullptr)
    {
        const size_t n = cstr.size();
        if (n == 0)
            return;
        ptr = new char[n+1];
        ::memcpy(ptr, cstr.c_str(), n);
        ptr[n] = 0;
    }
    
    /// move-constructor
    CString(CString&& cstr) noexcept : ptr(cstr.ptr)
    { cstr.ptr = nullptr; }
    
    /// constructor
    CString(std::initializer_list<char> init) : ptr(nullptr)
    {
        const size_t n = init.size();
        if (n == 0)
            return;
        ptr = new char[n+1];
        std::copy(init.begin(), init.end(), ptr);
        ptr[n] = 0; // null char
    }
    
    /// destructor
    ~CString()
    { delete[] ptr; }
    
    /// copy-assignment
    CString& operator=(const CString& cstr)
    {
        if (this==&cstr)
            return *this;
        return assign(cstr.ptr);
    }
    
    /// assignment
    CString& operator=(const std::string& str)
    { return assign(str.c_str(), str.size()); }
    
    /// move-assignment
    CString& operator=(CString&& cstr) noexcept
    {
        if (this==&cstr)
            return *this;
        delete[] ptr;   // delete old
        ptr = cstr.ptr;
        cstr.ptr = nullptr;
        return *this;
    }
    
    /// assignment
    CString& operator=(const char* str)
    { return assign(str); }
    
    /// assignment
    CString& operator=(char ch)
    { return assign(1, ch); }
    
    /// assignment
    CString& operator=(std::initializer_list<char> init)
    { return assign(init); }
    
    /// assign string
    CString& assign(const char* str)
    {
        if (str==nullptr)
        {
            delete[] ptr;
            ptr = nullptr;
            return *this;
        }
        size_t length = ::strlen(str);
        return assign(str, length);
    }
    
    /// assign string
    CString& assign(const char* str, size_t n)
    {
        if (n == 0)
        {
            // just clear
            delete[] ptr;
            ptr = nullptr;
            return *this;
        }
        char* newPtr = new char[n+1];
        ::memcpy(newPtr, str, n);
        newPtr[n] = 0;
        delete[] ptr;
        ptr = newPtr;
        return *this;
    }
    
    /// assign string
    CString& assign(const char* str, const char* end)
    { return assign(str, end-str); }
    
    /// assign string
    CString& assign (size_t n, char ch)
    {
        if (n == 0)
        {
            // just clear
            delete[] ptr;
            ptr = nullptr;
            return *this;
        }
        char* newPtr = new char[n+1];
        ::memset(newPtr, ch, n);
        newPtr[n] = 0; // null-char
        delete[] ptr;
        ptr = newPtr;
        return *this;
    }
    
    /// assign string
    CString& assign(std::initializer_list<char> init)
    {
        const size_t n = init.size();
        if (n == 0)
        {
            // just clear
            delete[] ptr;
            ptr = nullptr;
            return *this;
        }
        char* newPtr = new char[n+1];
        std::copy(init.begin(), init.end(), newPtr);
        newPtr[n] = 0; // null char
        delete[] ptr;
        ptr = newPtr;
        return *this;
    }
    
    /// return C-style string pointer
    const char* c_str() const
    { return ptr!=nullptr ? ptr : ""; }
    
    /// return C-style string pointer
    const char* begin() const
    { return ptr!=nullptr ? ptr : ""; }
    
    /// get ith character (use only if string is not empty)
    const char& operator[](size_t i) const
    { return ptr[i]; }
    
    /// get ith character (use only if string is not empty)
    char& operator[](size_t i)
    { return ptr[i]; }
    
    /// return C-style string pointer
    char* begin()
    { return ptr; }
    
    /// compute size
    size_t size() const
    {
        if (ptr==nullptr) return 0;
        return ::strlen(ptr);
    }
    /// compute size
    size_t length() const
    {
        if (ptr==nullptr) return 0;
        return ::strlen(ptr);
    }
    
    /// clear this string
    void clear()
    {
        delete[] ptr;
        ptr = nullptr;
    }
    
    /// return true if string is empty
    bool empty() const
    { return ptr==nullptr; }
    
    /// first character (use only if string is not empty)
    const char& front() const
    { return ptr[0]; }
    
    /// first character (use only if string is not empty)
    char& front()
    { return ptr[0]; }
    
    /// compare with string
    int compare(const CString& cstr) const
    { return ::strcmp(c_str(), cstr.c_str()); }
    
    /// compare with string
    int compare(const char* str) const
    { return ::strcmp(c_str(), str); }
    
    /// compare with string
    int compare(size_t pos, size_t n, const char* str) const
    { return ::strncmp(c_str()+pos, str, n); }

    /// find character in string
    size_type find(char ch, size_t pos = 0) const
    {
        const char* th = c_str();
        const char* p = ::strchr(th+pos, ch);
        return (p!=nullptr) ? p-th : npos;
    }
    
    /// find string in string
    size_type find(const CString& str, size_t pos = 0) const
    {
        const char* th = c_str();
        const char* p = ::strstr(th+pos, str.c_str());
        return (p!=nullptr) ? p-th : npos;
    }
    
    /// find string in string
    size_type find(const char* str, size_t pos = 0) const
    {
        const char* th = c_str();
        const char* p = ::strstr(th+pos, str);
        return (p!=nullptr) ? p-th : npos;
    }
    
    /// make substring from string
    CString substr(size_t pos, size_t n) const
    { return CString(c_str()+pos, n); }
    
    /// swap this string with another
    void swap(CString& s2) noexcept
    { std::swap(ptr, s2.ptr); }
};

/// equal operator
inline bool operator==(const CLRX::CString& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())==0; }

/// not-equal operator
inline bool operator!=(const CLRX::CString& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())!=0; }

/// less operator
inline bool operator<(const CLRX::CString& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())<0; }

/// greater operator
inline bool operator>(const CLRX::CString& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())>0; }

/// less or equal operator
inline bool operator<=(const CLRX::CString& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())<=0; }

/// greater or equal operator
inline bool operator>=(const CLRX::CString& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())>=0; }

}

/// equal operator
inline bool operator==(const CLRX::CString& s1, const std::string& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())==0; }

/// not-equal operator
inline bool operator!=(const CLRX::CString& s1, const std::string& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())!=0; }

/// less operator
inline bool operator<(const CLRX::CString& s1, const std::string& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())<0; }

/// greater operator
inline bool operator>(const CLRX::CString& s1, const std::string& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())>0; }

/// less or equal operator
inline bool operator<=(const CLRX::CString& s1, const std::string& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())<=0; }

/// greater or equal operator
inline bool operator>=(const CLRX::CString& s1, const std::string& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())>=0; }

/// equal operator
inline bool operator==(const std::string& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())==0; }

/// not-equal operator
inline bool operator!=(const std::string& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())!=0; }

/// less operator
inline bool operator<(const std::string& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())<0; }

/// greater operator
inline bool operator>(const std::string& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())>0; }

/// less or equal operator
inline bool operator<=(const std::string& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())<=0; }

/// greater or equal operator
inline bool operator>=(const std::string& s1, const CLRX::CString& s2)
{ return ::strcmp(s1.c_str(), s2.c_str())>=0; }

/// equal operator
inline bool operator==(const CLRX::CString& s1, const char* s2)
{ return ::strcmp(s1.c_str(), s2)==0; }

/// not-equal operator
inline bool operator!=(const CLRX::CString& s1, const char* s2)
{ return ::strcmp(s1.c_str(), s2)!=0; }

/// less operator
inline bool operator<(const CLRX::CString& s1, const char* s2)
{ return ::strcmp(s1.c_str(), s2)<0; }

/// greater operator
inline bool operator>(const CLRX::CString& s1, const char* s2)
{ return ::strcmp(s1.c_str(), s2)>0; }

/// less or equal operator
inline bool operator<=(const CLRX::CString& s1, const char* s2)
{ return ::strcmp(s1.c_str(), s2)<=0; }

/// greater or equal operator
inline bool operator>=(const CLRX::CString& s1, const char* s2)
{ return ::strcmp(s1.c_str(), s2)>=0; }

/// equal operator
inline bool operator==(const char* s1, const CLRX::CString& s2)
{ return ::strcmp(s1, s2.c_str())==0; }

/// not-equal operator
inline bool operator!=(const char* s1, const CLRX::CString& s2)
{ return ::strcmp(s1, s2.c_str())!=0; }

/// less operator
inline bool operator<(const char* s1, const CLRX::CString& s2)
{ return ::strcmp(s1, s2.c_str())<0; }

/// greater operator
inline bool operator>(const char* s1, const CLRX::CString& s2)
{ return ::strcmp(s1, s2.c_str())>0; }

/// less or equal operator
inline bool operator<=(const char* s1, const CLRX::CString& s2)
{ return ::strcmp(s1, s2.c_str())<=0; }

/// greater or equal operator
inline bool operator>=(const char* s1, const CLRX::CString& s2)
{ return ::strcmp(s1, s2.c_str())>=0; }

/// push to output stream as string
inline std::ostream& operator<<(std::ostream& os, const CLRX::CString& cstr)
{ return os<<cstr.c_str(); }

namespace std
{

/// std::swap specialization CLRX CString
inline void swap(CLRX::CString& s1, CLRX::CString& s2)
{ s1.swap(s2); }

/// std::hash specialization for CLRX CString
template<>
struct hash<CLRX::CString>
{
    typedef CLRX::CString argument_type;    ///< argument type
    typedef std::size_t result_type;    ///< result type
    
    /// a calling operator
    size_t operator()(const CLRX::CString& s1) const
    {
        size_t hash = 0;
        for (const char* p = s1.c_str(); *p != 0; p++)
            hash = ((hash<<8)^(cxbyte)*p)*size_t(0xbf146a3dU);
        return hash;
    }
};

}

#endif
