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

#ifndef __CLRXTEST_TESTUTILS_H__
#define __CLRXTEST_TESTUTILS_H__

#include <CLRX/Config.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <initializer_list>
#include <CLRX/utils/Utilities.h>

using namespace CLRX;

// fail when value is false
static inline void assertTrue(const std::string& testName, const std::string& caseName,
           bool value)
{
    if (!value)
    {
        std::ostringstream oss;
        oss << "Failed " << testName << ":" << caseName;
        oss.flush();
        throw Exception(oss.str());
    }
}

// fail when value!=result
template<typename T>
static void assertValue(const std::string& testName, const std::string& caseName,
            const T& expected, const T& result)
{
    if (expected != result)
    {
        std::ostringstream oss;
        oss << "Failed " << testName << ":" << caseName << "\n" <<
            expected << "!=" << result;
        oss.flush();
        throw Exception(oss.str());
    }
}

// assertion for equality of strings
static inline void assertString(const std::string& testName, const std::string& caseName,
             const char* expected, const char* result)
{
    if (expected == nullptr)
    {
        // if expected is null then fail if result is non null
        if (result != nullptr)
        {
            std::ostringstream oss;
            oss << "Failed " << testName << ":" << caseName << "\n" << "null!=\"" <<
                        result << '\"';
            oss.flush();
            throw Exception(oss.str());
        }
    }
    else if (result == nullptr)
    {
        // if result is null then fail if expected is non null
        if (expected != nullptr)
        {
            std::ostringstream oss;
            oss << "Failed " << testName << ":" << caseName << "\n\"" <<
                    expected << "\"!=null";
            oss.flush();
            throw Exception(oss.str());
        }
    }
    else if (::strcmp(expected, result) != 0)
    {
        std::ostringstream oss;
        oss << "Failed " << testName << ":" << caseName << "\n" <<
            "\"" << expected << "\"!=\"" << result << "\"";
        oss.flush();
        throw Exception(oss.str());
    }
}

static inline void assertString(const std::string& testName, const std::string& caseName,
             const char* expected, const std::string& result)
{ assertString(testName, caseName, expected, result.c_str()); }

static inline void assertString(const std::string& testName, const std::string& caseName,
             const char* expected, const CString& result)
{ assertString(testName, caseName, expected, result.c_str()); }

template<typename T>
static inline const T& printForTests(const T& v)
{ return v; }

// special cases for printing byte and character as integer
static inline const int printForTests(const cxbyte v)
{ return int(v); }

static inline const int printForTests(const cxchar v)
{ return int(v); }

// assertion for equality of arrays
template<typename T>
static void assertArray(const std::string& testName, const std::string& caseName,
            const Array<T>& expected, size_t resultSize, const T* result)
{
    if (result == nullptr)
    {
        if (!expected.empty())
        {
            std::ostringstream oss;
            oss << "Failed " << testName << ":" << caseName << " \n" <<
                    "Array is nullptr";
            oss.flush();
            throw Exception(oss.str());
        }
    }
    // check size of array (must be equal)
    if (expected.size() != resultSize)
    {
        std::ostringstream oss;
        oss << "Failed " << testName << ":" << caseName << " \n" <<
            "Size of Array: " << expected.size() << "!=" << resultSize;
        oss.flush();
        throw Exception(oss.str());
    }
    auto it = expected.begin();
    for (size_t i = 0; i < resultSize ; i++, ++it)
        if (*it != result[i])
        {
            std::ostringstream oss;
            oss << "Failed " << testName << ":" << caseName << " \n" <<
                "Elem #" << i << ": " << printForTests(*it) << "!=" <<
                    printForTests(result[i]);
            oss.flush();
            throw Exception(oss.str());
        }
}

template<typename T>
static void assertArray(const std::string& testName, const std::string& caseName,
            const Array<T>& expected, const Array<T>& result)
{ assertArray<T>(testName, caseName, expected, result.size(), result.data()); }

template<typename T>
static void assertArray(const std::string& testName, const std::string& caseName,
            const Array<T>& expected, const std::vector<T>& result)
{ assertArray<T>(testName, caseName, expected, result.size(), result.data()); }

// assertion for euqality of array of strings
static inline void assertStrArray(const std::string& testName,
      const std::string& caseName, const std::initializer_list<const char*>& expected,
      size_t resultSize, const char** result)
{
    if (expected.size() != resultSize)
    {
        std::ostringstream oss;
        oss << "Failed " << testName << ":" << caseName << " \n" <<
            "Size of Array: " << expected.size() << "!=" << resultSize;
        oss.flush();
        throw Exception(oss.str());
    }
    auto it = expected.begin();
    for (size_t i = 0; i < resultSize ; i++, ++it)
        if (::strcmp(*it, result[i]) != 0)
        {
            std::ostringstream oss;
            oss << "Failed " << testName << ":" << caseName << " \n" <<
                "Elem #" << i << ": " << *it << "!=" << result[i];
            oss.flush();
            throw Exception(oss.str());
        }
}

// fail when no exception occurs
template<typename Call, typename... T>
static inline void assertCLRXException(const std::string& testName,
           const std::string& caseName, const char* expectedWhat,
           const Call& call, T&& ...args)
{
    bool failed = false;
    std::string resultWhat;
    try
    { call(args...); }
    catch(const std::exception& ex)
    {
        failed = true;
        resultWhat = ex.what();
    }
    if (!failed)
    {
        std::ostringstream oss;
        oss << "No exception occurred for " << testName << ":" << caseName << " \n";
        oss.flush();
        throw Exception(oss.str());
    }
    else if (resultWhat != expectedWhat)
    {
        std::ostringstream oss;
        oss << "Exception string mismatch for " << testName << ":" << caseName << " \n" <<
                expectedWhat << "!=" << resultWhat << "\n";
        oss.flush();
        throw Exception(oss.str());
    }
}


template<typename Call, typename... T>
static int callTest(const Call& call, T&& ...args)
{
    try
    { call(args...); }
    catch(const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    return 0;
}

#endif
