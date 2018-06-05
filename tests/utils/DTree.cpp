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

#define DTREE_TESTING 1

#include <CLRX/Config.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <utility>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/DTree.h>
#include "../TestUtils.h"

using namespace CLRX;

template<typename T>
static void verifyDTreeNode0(const std::string& testName, const std::string& testCase,
                const typename DTreeSet<T>::Node0& n0, cxuint level, cxuint maxLevel)
{
    assertTrue(testName, testCase + ".levelNode0", maxLevel==level);
    cxuint size = 0;
    cxuint firstPos = UINT_MAX;
    for (cxuint i = 0; i < n0.capacity; i++)
    {
        if ((n0.bitMask & (1ULL<<i)) == 0)
        {
            if (firstPos == UINT_MAX)
                // set first pos
                firstPos = i;
            size++;
        }
    }
    char buf[10];
    for (cxuint i = 1; i < n0.capacity; i++)
    {
        snprintf(buf, 10, "<=e[%d]", i);
        if ((n0.bitMask & (3ULL<<(i-1))) != 0)
            // some places is unused (freed) in free space can be
            // same value as in used place
            assertTrue(testName, testCase + buf, n0.array[i-1] <= n0.array[i]);
        else
            assertTrue(testName, testCase + buf, n0.array[i-1] < n0.array[i]);
    }
    
    assertValue(testName, testCase + ".n0.firstPos==firstPos", firstPos,
                cxuint(n0.firstPos));
    assertValue(testName, testCase + ".n0.size==size", size, cxuint(n0.size));
    assertTrue(testName, testCase + ".n0.size<=n0.capacity", n0.size <= n0.capacity);
    
    if (level > 0)
        assertTrue(testName, testCase + ".n0.size>=minSize",
                    n0.size >= DTree<T>::minNode0Size);
    
    assertTrue(testName, testCase + ".n0.size<=maxSize",
                    n0.size <= DTree<T>::maxNode0Size);
    assertTrue(testName, testCase + ".n0.capacity<=maxCapacity",
                    n0.capacity <= DTree<T>::maxNode0Capacity);
}

template<typename T>
static void verifyDTreeNode1(const std::string& testName, const std::string& testCase,
                const typename DTreeSet<T>::Node1& n1, cxuint level, cxuint maxLevel)
{
    assertTrue(testName, testCase + ".n1.size<=n1.capacity",
                   n1.size <= n1.capacity);
    char buf[10];
    size_t totalSize = 0;
    T firstKey = T();
    if (n1.type == DTree<T>::NODE1)
    {
        assertTrue(testName, testCase + ".levelNode1", maxLevel-1==level);
        
        firstKey = n1.array[0].array[n1.array[0].firstPos];
        for (cxuint i = 0; i < n1.size; i++)
        {
            totalSize += n1.array[i].size;
            snprintf(buf, sizeof buf, "[%d]", i);
            verifyDTreeNode0<T>(testName, testCase + buf, n1.array[i], level+1, maxLevel);
            assertValue(testName, testCase + buf + ".index", i, cxuint(n1.array[i].index));
        }
        // checking ordering
        for (cxuint i = 1; i < n1.size; i++)
        {
            snprintf(buf, sizeof buf, "<=f[%d]", i);
            assertTrue(testName, testCase + buf,
                        n1.array[i-1].array[n1.array[i-1].firstPos] <
                        n1.array[i].array[n1.array[i].firstPos]);
        }
    }
    else
    {
        // Node1 with Node1's
        firstKey = n1.array1[0].first;
        for (cxuint i = 0; i < n1.size; i++)
        {
            totalSize += n1.array1[i].totalSize;
            snprintf(buf, sizeof buf, "[%d]", i);
            verifyDTreeNode1<T>(testName, testCase + buf, n1.array1[i], level+1, maxLevel);
            assertValue(testName, testCase + buf + ".index", i,
                        cxuint(n1.array1[i].index));
        }
        // checking ordering
        for (cxuint i = 1; i < n1.size; i++)
        {
            snprintf(buf, sizeof buf, "<=f[%d]", i);
            assertTrue(testName, testCase + buf, n1.array1[i-1].first <
                        n1.array1[i].first);
        }
    }
    assertValue(testName, testCase + ".totalSize==n1.totalSize",
                   totalSize, n1.totalSize);
    assertValue(testName, testCase + ".first==n1.first", firstKey, n1.first);
}

template<typename T>
static void verifyDTreeState(const std::string& testName, const std::string& testCase,
            const DTreeSet<T>& dt)
{
    if (dt.n0.type == DTree<T>::NODE0)
    {
        verifyDTreeNode0<T>(testName, testCase + "n0root", dt.n0, 0, 0);
        assertTrue(testName, testCase + ".first", (&dt.n0)==dt.first);
        assertTrue(testName, testCase + ".last", (&dt.n0)==dt.last);
    }
    else
    {
        cxuint maxLevel = 1;
        for (const typename DTreeSet<T>::Node1* n1 = &dt.n1; n1->type==DTree<T>::NODE2;
                n1 = n1->array1, maxLevel++);
        
        verifyDTreeNode1<T>(testName, testCase + "n1root", dt.n1, 0, 0);
        const typename DTreeSet<T>::Node1* n1;
        for (n1 = &dt.n1; n1->type == DTree<T>::NODE2; n1 = n1->array1);
        assertTrue(testName, testCase + ".first", n1->array == dt.first);
        for (n1 = &dt.n1; n1->type == DTree<T>::NODE2; n1 = n1->array1 + n1->size-1);
        assertTrue(testName, testCase + ".last", (n1->array + n1->size-1) == dt.last);
    }
}

struct testDTreeInsert
{
    
};

static void testDTreeInsert(cxuint i, const Array<cxuint>& valuesToInsert)
{
    DTreeSet<cxuint> set;
    for (size_t j = 0; j < valuesToInsert.size(); i++)
    {
        auto it = set.insert(valuesToInsert[i]);
        verifyDTreeState("test", "test", set);
    }
    set.erase(11);
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    return retVal;
}
