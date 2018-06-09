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
#include <algorithm>
#include <limits>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <utility>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/DTree.h>
#include "../TestUtils.h"

using namespace CLRX;

// prevValue - pointer to previous value and to set next last value (next previous)
// prevValue pair - first - previous value, second - is valid value
//                   (false if irst call of verifyDTreeNode0 and no previous value)

template<typename T>
static void verifyDTreeNode0(const std::string& testName, const std::string& testCase,
                const typename DTreeSet<T>::Node0& n0, cxuint level, cxuint maxLevel,
                std::pair<T, bool>* prevValuePtr = nullptr)
{
    assertTrue(testName, testCase + ".levelNode0", maxLevel==level);
    cxuint size = 0;
    cxuint firstPos = UINT_MAX;
    cxuint lastPos = UINT_MAX;
    for (cxuint i = 0; i < n0.capacity; i++)
    {
        if ((n0.bitMask & (1ULL<<i)) == 0)
        {
            if (firstPos == UINT_MAX)
                // set first pos
                firstPos = i;
            lastPos = i;
            size++;
        }
    }
    if (firstPos == UINT_MAX)
        firstPos = 0;
    
    if (prevValuePtr != nullptr && firstPos < n0.capacity && !prevValuePtr->second)
        // check ordering with previous value
        assertTrue(testName, testCase + "<e[0]", prevValuePtr->first < n0[firstPos]);
    
    char buf[10];
    for (cxuint i = 1; i < n0.capacity; i++)
    {
        snprintf(buf, 10, "<=e[%d]", i);
        if ((n0.bitMask & (3ULL<<(i-1))) != 0)
            // some places is unused (freed) in free space can be
            // same value as in used place
            assertTrue(testName, testCase + buf, n0[i-1] <= n0[i]);
        else
            assertTrue(testName, testCase + buf, n0[i-1] < n0[i]);
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
    
    if (prevValuePtr != nullptr && lastPos < n0.capacity)
        *prevValuePtr = std::make_pair(n0[lastPos], false);
}

template<typename T>
static void verifyDTreeNode1(const std::string& testName, const std::string& testCase,
                const typename DTreeSet<T>::Node1& n1, cxuint level, cxuint maxLevel,
                std::pair<T, bool>* prevValuePtr = nullptr)
{
    assertTrue(testName, testCase + ".n1.size<=n1.capacity",
                   n1.size <= n1.capacity);
    char buf[10];
    size_t totalSize = 0;
    T firstKey = T();
    std::pair<T, bool> prevValue = std::make_pair(T(), true);
    if (prevValuePtr == nullptr)
        // we do not have previous value (initial call) just use pointer to prevValue
        prevValuePtr = &prevValue; // by default is this value
    
    if (n1.type == DTree<T>::NODE1)
    {
        assertTrue(testName, testCase + ".levelNode1", maxLevel-1==level);
        
        if (n1.size != 0)
            firstKey = n1.array[0].array[n1.array[0].firstPos];
        for (cxuint i = 0; i < n1.size; i++)
        {
            totalSize += n1.array[i].size;
            snprintf(buf, sizeof buf, "[%d]", i);
            verifyDTreeNode0<T>(testName, testCase + buf, n1.array[i], level+1, maxLevel,
                        prevValuePtr);
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
        if (n1.size != 0)
            firstKey = n1.array1[0].first;
        for (cxuint i = 0; i < n1.size; i++)
        {
            totalSize += n1.array1[i].totalSize;
            snprintf(buf, sizeof buf, "[%d]", i);
            verifyDTreeNode1<T>(testName, testCase + buf, n1.array1[i], level+1, maxLevel,
                        prevValuePtr);
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
    
    assertTrue(testName, testCase + ".size<=n1.capacity", n1.size<=n1.capacity);
    assertTrue(testName, testCase + ".size<=maxNode1Size",
                n1.size <= DTree<T>::maxNode1Size);
    
    if (level != 0)
        assertTrue(testName, testCase + ".totalSize>=minTotalSize",
                    n1.totalSize>=DTree<T>::minTotalSize(maxLevel-level));
    assertTrue(testName, testCase + ".totalSize<=maxTotalSize",
                   n1.totalSize<=DTree<T>::maxTotalSize(maxLevel-level));
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

/* DTree Node0 tests */

static const cxuint dtreeNode0Values[] =
{
    532, 6421, 652, 31891, 78621, 61165, 1203, 1203, 41, 6629, 45811, 921, 2112, 31891
};
static const size_t dtreeNode0ValuesNum = sizeof dtreeNode0Values / sizeof(cxuint);

static const cxuint dtreeNode0ValuesErase[] =
{
    532, 6421, 652, 31891, 78621, 61165, 1203, 41, 6629, 45811, 921, 2112, 6521,
    971, 71, 41289, 769, 8921, 37912
};
static const size_t dtreeNode0ValuesEraseNum =
        sizeof dtreeNode0ValuesErase / sizeof(cxuint);

static const cxuint dtreeNode0ValuesSearch[] =
{
    42, 24, 52, 7, 17, 42, 37, 27, 4, 62, 34, 31, 9, 41, 49, 58, 53
};
static const size_t dtreeNode0ValuesSearchNum =
            sizeof dtreeNode0ValuesSearch / sizeof(cxuint);

template<typename T>
static void checkContentDTreeNode0(const std::string& testName,
            const std::string& testCase, const typename DTreeSet<T>::Node0& node0,
            size_t node0ValuesNum, const T* node0Values)
{
    std::less<T> comp;
    Identity<T> kofval;
    for (size_t i = 0; i < node0ValuesNum; i++)
    {
        const T& v = node0Values[i];
        const cxuint index = node0.find(v, comp, kofval);
        assertTrue(testName, testCase+".findindexBound", index!=node0.capacity);
        assertTrue(testName, testCase+".findindexAlloc",
                   (node0.bitMask & (1ULL<<index)) == 0);
        assertTrue(testName, testCase+".findindex", node0[index]==v);
    }
}

template<typename T>
static void checkNotFoundDTreeNode0(const std::string& testName,
            const std::string& testCase, const typename DTreeSet<T>::Node0& node0,
            size_t node0ValuesNum, const T* node0Values)
{
    std::less<T> comp;
    Identity<T> kofval;
    for (size_t i = 0; i < node0ValuesNum; i++)
    {
        const T& v = node0Values[i];
        const cxuint index = node0.find(v, comp, kofval);
        assertTrue(testName, testCase+".nofindindexBound", index==node0.capacity);
    }
}


static void testDTreeNode0()
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    {
        DTreeSet<cxuint>::Node0 empty;
        assertTrue("DTreeNode0", "empty.array", empty.array==nullptr);
        assertTrue("DTreeNode0", "empty.parent()", empty.parent()==nullptr);
        
        cxuint index = empty.lower_bound(432, comp, kofval);
        assertValue("DTreeNode0", "empty.lower_bound", cxuint(0), index);
        index = empty.lower_bound(432, comp, kofval);
        assertValue("DTreeNode0", "empty.upper_bound", cxuint(0), index);
        index = empty.lower_bound(432, comp, kofval);
        assertValue("DTreeNode0", "empty.find", cxuint(0), index);
        
        verifyDTreeNode0<cxuint>("DTreeNode0", "empty", empty, 0, 0);
        DTreeSet<cxuint>::Node0 empty1(empty);
        verifyDTreeNode0<cxuint>("DTreeNode0", "empty_copy", empty1, 0, 0);
        DTreeSet<cxuint>::Node0 empty2;
        empty2 = empty;
        verifyDTreeNode0<cxuint>("DTreeNode0", "empty_copy2", empty2, 0, 0);
        DTreeSet<cxuint>::Node0 empty3(std::move(empty));
        verifyDTreeNode0<cxuint>("DTreeNode0", "empty_move", empty3, 0, 0);
        DTreeSet<cxuint>::Node0 empty4;
        empty4 = std::move(empty1);
        verifyDTreeNode0<cxuint>("DTreeNode0", "empty_move2", empty4, 0, 0);
    }
    {
        // test insertion
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint i = 0; i < dtreeNode0ValuesEraseNum; i++)
        {
            const cxuint v = dtreeNode0ValuesErase[i];
            const cxuint index = node0.insert(v, comp, kofval).first;
            assertTrue("DTreeNode0", "node0_0x.indexBound", index!=node0.capacity);
            assertTrue("DTreeNode0", "node0_0x.index", node0[index]==v);
            verifyDTreeNode0<cxuint>("DTreeNode0", "node0_0x", node0, 0, 0);
            checkContentDTreeNode0("DTreeNode0", "node0_0xinsert", node0,
                    i+1, dtreeNode0ValuesErase);
            checkNotFoundDTreeNode0("DTreeNode0", "node0_0xinsert", node0,
                    dtreeNode0ValuesEraseNum-(i+1), dtreeNode0ValuesErase+i+1);
            assertValue("DTreeNode0", "node0_0x.size", i+1, cxuint(node0.size));
        }
    }
    {
        // test inserion and copying/moving
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint i = 0; i < dtreeNode0ValuesNum; i++)
        {
            const cxuint v = dtreeNode0Values[i];
            const cxuint index = node0.insert(v, comp, kofval).first;
            assertTrue("DTreeNode0", "node0_0.indexBound", index!=node0.capacity);
            assertTrue("DTreeNode0", "node0_0.index", node0[index]==v);
            verifyDTreeNode0<cxuint>("DTreeNode0", "node0_0", node0, 0, 0);
            checkContentDTreeNode0("DTreeNode0", "node0_0insert", node0,
                    i+1, dtreeNode0Values);
        }
        checkContentDTreeNode0("DTreeNode0", "node0_0", node0,
                    dtreeNode0ValuesNum, dtreeNode0Values);
        verifyDTreeNode0<cxuint>("DTreeNode0", "node0_0", node0, 0, 0);
        
        // copy node
        DTreeSet<cxuint>::Node0 node0_1(node0);
        checkContentDTreeNode0("DTreeNode0", "node0_0copy", node0_1,
                    dtreeNode0ValuesNum, dtreeNode0Values);
        verifyDTreeNode0<cxuint>("DTreeNode0", "node0_0copy", node0_1, 0, 0);
        
        DTreeSet<cxuint>::Node0 node0_2;
        node0_2 = node0;
        checkContentDTreeNode0("DTreeNode0", "node0_0copy2", node0_2,
                    dtreeNode0ValuesNum, dtreeNode0Values);
        verifyDTreeNode0<cxuint>("DTreeNode0", "node0_0copy2", node0_2, 0, 0);
        
        // move node
        DTreeSet<cxuint>::Node0 node0_3(std::move(node0));
        checkContentDTreeNode0("DTreeNode0", "node0_0move", node0_3,
                    dtreeNode0ValuesNum, dtreeNode0Values);
        verifyDTreeNode0<cxuint>("DTreeNode0", "node0_0move", node0_3, 0, 0);
        
        DTreeSet<cxuint>::Node0 node0_4;
        node0_4 = node0_1;
        checkContentDTreeNode0("DTreeNode0", "node0_0move2", node0_4,
                    dtreeNode0ValuesNum, dtreeNode0Values);
        verifyDTreeNode0<cxuint>("DTreeNode0", "node0_0move2", node0_4, 0, 0);
    }
    {
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint v: dtreeNode0ValuesSearch)
        {
            const cxuint index = node0.insert(v, comp, kofval).first;
            assertTrue("DTreeNode0", "node0_1.findindex", index!=node0.capacity);
            assertTrue("DTreeNode0", "node0_1.index", node0[index]==v);
            verifyDTreeNode0<cxuint>("DTreeNode0", "node0_1", node0, 0, 0);
        }
        const cxuint lowValue = 0;
        const cxuint highValue = *std::max_element(dtreeNode0ValuesSearch,
                    dtreeNode0ValuesSearch + dtreeNode0ValuesSearchNum) + 5;
        
        // lower bound
        for (cxuint v = lowValue; v < highValue; v++)
        {
            cxint index = node0.lower_bound(v, comp, kofval);
            if (index < node0.capacity)
            {
                assertTrue("DTReeNode0", "lower_bound found", v<=node0[index]);
                assertTrue("DTReeNode0", "lower_bound foundAlloc",
                            (node0.bitMask & (1ULL<<index)) == 0);
            }
            // go to previous element
            index--;
            while (index >= 0 && (node0.bitMask & (1ULL<<index)) != 0) index--;
            if (index >= 0)
                assertTrue("DTReeNode0", "lower_bound prev", v>node0[index]);
        }
        // upper bound
        for (cxuint v = lowValue; v < highValue; v++)
        {
            cxint index = node0.upper_bound(v, comp, kofval);
            if (index < node0.capacity)
            {
                assertTrue("DTReeNode0", "upper_bound found", v<node0[index]);
                assertTrue("DTReeNode0", "upper_bound foundAlloc",
                            (node0.bitMask & (1ULL<<index)) == 0);
            }
            // go to previous element
            index--;
            while (index >= 0 && (node0.bitMask & (1ULL<<index)) != 0) index--;
            if (index >= 0)
                assertTrue("DTReeNode0", "upper_bound prev", v>=node0[index]);
        }
        // find
        for (cxuint v = lowValue; v < highValue; v++)
        {
            cxint index = node0.find(v, comp, kofval);
            const bool found = std::find(dtreeNode0ValuesSearch,
                    dtreeNode0ValuesSearch + dtreeNode0ValuesSearchNum, v) !=
                    dtreeNode0ValuesSearch + dtreeNode0ValuesSearchNum;
            
            if (found)
            {
                assertTrue("DTReeNode0", "find foundAlloc",
                            (node0.bitMask & (1ULL<<index)) == 0);
                assertTrue("DTReeNode0", "find foundBound", index<node0.capacity);
                assertTrue("DTReeNode0", "find found2", v==node0[index]);
            }
            else
                assertTrue("DTReeNode0", "find notfound", index==node0.capacity);
        }
    }
    // resize checking
    {
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint v: dtreeNode0Values)
            node0.insert(v, comp, kofval);
        node0.resize(0);
        verifyDTreeNode0<cxuint>("DTreeNode0", "node0resize", node0, 0, 0);
    }
    // erase checking
    {
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint v: dtreeNode0ValuesErase)
            node0.insert(v, comp, kofval);
        for (cxuint i = 0; i < dtreeNode0ValuesEraseNum; i++)
        {
            cxuint v = dtreeNode0ValuesErase[i];
            node0.erase(v, comp, kofval);
            verifyDTreeNode0<cxuint>("DTreeNode0", "erase0", node0, 0, 0);
            checkContentDTreeNode0("DTreeNode0", "erase0", node0,
                        dtreeNode0ValuesEraseNum-(i+1), dtreeNode0ValuesErase+i+1);
            checkNotFoundDTreeNode0("DTreeNode0", "erase0", node0,
                        i+1, dtreeNode0ValuesErase);
        }
    }
    // test insertion with hint (good and wrong)
    for (cxuint diffIndex = 0; diffIndex <= 3; diffIndex++)
    {
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint i = 0; i < 20; i += 2)
            node0.insert(i, comp, kofval);
        
        verifyDTreeNode0<cxuint>("DTreeNode0", "node0insh", node0, 0, 0);
        cxuint index = node0.find(10, comp, kofval);
        node0.insert(9, comp, kofval, index-diffIndex);
        verifyDTreeNode0<cxuint>("DTreeNode0", "node0insh2", node0, 0, 0);
        index = node0.find(9, comp, kofval);
        assertTrue("DTreeNode0", "node0insh2find", index!=node0.capacity);
        assertValue("DTreeNode0", "node0insh2find2", cxuint(9), node0[index]);
        index = node0.find(10, comp, kofval);
        assertTrue("DTreeNode0", "node0insh2find", index!=node0.capacity);
        assertValue("DTreeNode0", "node0insh2find2", cxuint(10), node0[index]);
        index = node0.find(8, comp, kofval);
        assertTrue("DTreeNode0", "node0insh2find", index!=node0.capacity);
        assertValue("DTreeNode0", "node0insh2find2", cxuint(8), node0[index]);
    }
}

static const cxuint dtreeNode0ValuesM1[] =
{
    1, 13, 2, 5, 18, 8, 12, 3, 14, 20, 9, 10, 15, 7, 19, 17, 6, 11, 4, 16
};
static const size_t dtreeNode0ValuesM1Num = sizeof dtreeNode0ValuesM1 / sizeof(cxuint);

static const cxuint dtreeNode0ValuesM2[] =
{
    24, 38, 27, 28, 21, 35, 29, 25, 26, 39, 23, 31, 36, 32, 40, 34, 37, 33, 30, 22
};
static const size_t dtreeNode0ValuesM2Num = sizeof dtreeNode0ValuesM2 / sizeof(cxuint);

static const cxuint dtreeNode0ValuesS2[] =
{
    3, 52, 57, 281, 5, 86, 32, 67, 12, 54, 74, 89, 103, 156, 243, 209, 178, 196,
    84, 42, 47, 53, 275, 291, 115, 191, 83, 51, 34, 138, 58, 22, 162, 49, 185, 264
};
static const size_t dtreeNode0ValuesS2Num = sizeof dtreeNode0ValuesS2 / sizeof(cxuint);

static void testDTreeNode0SplitMerge()
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    char buf[20];
    
    {
        // merge
        DTreeSet<cxuint>::Node0 node0_1;
        for (cxuint i = 0; i < dtreeNode0ValuesM1Num; i++)
        {
            DTreeSet<cxuint>::Node0 node0_2;
            node0_1.insert(dtreeNode0ValuesM1[i], comp, kofval);
            for (cxuint j = 0; j < dtreeNode0ValuesM2Num; j++)
            {
                node0_2.insert(dtreeNode0ValuesM2[j], comp, kofval);
                DTreeSet<cxuint>::Node0 node0_1c = node0_1;
                DTreeSet<cxuint>::Node0 node0_2c = node0_2;
                node0_1c.merge(node0_2c);
                
                snprintf(buf, sizeof buf, "merge:%u:%u", i, j);
                verifyDTreeNode0<cxuint>("DTreeNode0SM", std::string(buf)+".verify",
                                node0_1c, 0, 0);
                assertValue("DTreeNode0SM", std::string(buf)+".size", i+j+2,
                                        cxuint(node0_1c.size));
                checkContentDTreeNode0("DTreeNode0SM", std::string(buf)+".content",
                        node0_1c, i+1, dtreeNode0ValuesM1);
                checkNotFoundDTreeNode0("DTreeNode0SM", std::string(buf)+".notfound",
                        node0_1c, dtreeNode0ValuesM1Num-(i+1), dtreeNode0ValuesM1+i+1);
                checkContentDTreeNode0("DTreeNode0SM", std::string(buf)+".content2",
                        node0_1c, j+1, dtreeNode0ValuesM2);
                checkNotFoundDTreeNode0("DTreeNode0SM",
                        std::string(buf)+".notfound2", node0_1c,
                        dtreeNode0ValuesM2Num-(j+1), dtreeNode0ValuesM2+j+1);
            }
        }
    }
    {
        // merge node0s with free spaces at start
        DTreeSet<cxuint>::Node0 node0_1, node0_2;
        for (cxuint i = 1; i <= 10; i++)
            node0_1.insert(i, comp, kofval);
        for (cxuint i = 11; i <= 20; i++)
            node0_2.insert(i, comp, kofval);
        // erase start
        node0_1.erase(1, comp, kofval);
        node0_1.erase(2, comp, kofval);
        node0_1.erase(10, comp, kofval);
        node0_2.erase(11, comp, kofval);
        node0_2.erase(12, comp, kofval);
        node0_2.erase(20, comp, kofval);
        // merge
        node0_1.merge(node0_2);
        verifyDTreeNode0<cxuint>("DTreeNode0SM2", "mergefreesstart", node0_1, 0, 0);
        for (cxuint i = 1; i <= 20; i++)
        {
            cxuint index = node0_1.find(i, comp, kofval);
            if (i==1 || i==2 || i==10 || i==11 || i==12 || i==20)
                assertTrue("DTreeNode0SM2", "mergefreesstartnf", index==node0_1.capacity);
            else
            {
                // if must be found
                assertTrue("DTreeNode0SM2", "mergefreesstart", index!=node0_1.capacity);
                assertValue("DTreeNode0SM2", "mergefreesstartval", i, node0_1[index]);
            }
        }
    }
    
    {
        // split testing
        for (cxuint i = 1; i < dtreeNode0ValuesS2Num; i++)
        {
            snprintf(buf, sizeof buf, "split:%u", i);
            
            DTreeSet<cxuint>::Node0 node0_1, node0_2;
            for (cxuint k = 0; k < i; k++)
                node0_1.insert(dtreeNode0ValuesS2[k], comp, kofval);
            
            node0_1.split(node0_2);
            
            verifyDTreeNode0<cxuint>("DTreeNode0Split", std::string(buf)+".verify1",
                                node0_1, 0, 0);
            verifyDTreeNode0<cxuint>("DTreeNode0Split", std::string(buf)+".verify2",
                                node0_2, 0, 0);
            assertValue("DTreeNode0Split", std::string(buf)+".size1", (i+1)>>1,
                                cxuint(node0_1.size));
            assertValue("DTreeNode0Split", std::string(buf)+".size2", i>>1,
                                cxuint(node0_2.size));
            assertValue("DTreeNode0Split", std::string(buf)+".sizesum", i,
                                cxuint(node0_1.size)+cxuint(node0_2.size));
            
            Array<cxuint> sorted(dtreeNode0ValuesS2, dtreeNode0ValuesS2+i);
            std::sort(sorted.begin(), sorted.end());
            checkContentDTreeNode0("DTreeNode0Split", std::string(buf)+".content",
                        node0_1, (i+1)>>1, sorted.data());
            checkContentDTreeNode0("DTreeNode0Split", std::string(buf)+".content",
                        node0_2, i>>1, sorted.data() + ((i+1)>>1));
        }
    }
}

struct DTreeNode0OrgArrayCase
{
    cxuint size;
    cxuint newSize;
    cxuint factor;
    cxuint finc;
    uint64_t inBitMask;
    Array<cxuint> input;
    uint64_t expBitMask;
    Array<cxuint> expOutput;
};

static const DTreeNode0OrgArrayCase dtreeNode0OrgArrayTbl[] =
{
    {   // case 0
        7, 7, 0, 2, 0ULL,
        { 1, 2, 3, 4, 5, 6, 7 },
        0b100010000ULL,
        { 1, 2, 3, 4, 4, 5, 6, 7, 7 }
    },
    {   // case 1
        7, 7, 0, 3, 0b110ULL,
        { 1, 1, 1, 2, 3, 4, 5, 6, 7 },
        0b1001001000ULL,
        { 1, 2, 3, 3, 4, 5, 5, 6, 7, 7 }
    },
    {   // case 2
        7, 7, 6, 3, 0b110ULL,
        { 1, 1, 1, 2, 3, 4, 5, 6, 7 },
        0b10010010ULL,
        { 1, 1, 2, 3, 3, 4, 5, 5, 6, 7 }
    }
};

static void testDTreeOrganizeArray(cxuint id, const DTreeNode0OrgArrayCase& testCase)
{
    std::ostringstream oss;
    oss << "dtreeOrgArray#" << id;
    oss.flush();
    
    uint64_t outBitMask = 0;
    cxuint output[64];
    std::fill(output, output + 64, cxuint(0));
    
    cxuint toFill = 0;
    cxuint i = 0, k = 0;
    cxuint factor = testCase.factor;
    DTreeSet<cxuint>::Node0::organizeArray(toFill, i, testCase.size,
                testCase.input.data(), testCase.inBitMask, k, testCase.newSize,
                output, outBitMask, factor, testCase.finc);
    
    std::string testCaseName = oss.str();
    assertValue("DTreeOrgArray", testCaseName+".bitMask", testCase.expBitMask, outBitMask);
    assertArray("DTreeOrgArray", testCaseName+".array", testCase.expOutput, k, output);
}

/* DTree Node1 tests */

static void createNode1FromArray(DTreeSet<cxuint>::Node1& node1, cxuint num,
                            const cxuint* input)
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    for (cxuint i = 0; i < num; i++)
    {
        const cxuint v = input[i];
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint x = v; x < v+20; x++)
            node0.insert(x, comp, kofval);
        node1.insertNode0(std::move(node0), i);
    }
}

static const cxuint dtreeNode1Firsts[] = { 32, 135, 192, 243, 393, 541 };
static const cxuint dtreeNode1FirstsNum = sizeof dtreeNode1Firsts / sizeof(cxuint);

static const cxuint dtreeNode1Firsts2[] = { 32, 135, 192, 243, 393, 541, 593, 678 };

static const cxuint dtreeNode1Order[] = { 4, 3, 5, 2, 6, 7, 0, 1 };
static const cxuint dtreeNode1EraseOrder[] = { 1, 4, 5, 0, 1, 2, 0, 0 };

static void checkNode1Firsts0(const std::string& testName, const std::string& testCase,
            const DTreeSet<cxuint>::Node1& node1, cxuint num, const cxuint* input)
{
    assertValue(testName, testCase+".size", num, cxuint(node1.size));
    char buf[20];
    for (cxuint i = 0; i < num; i++)
    {
        snprintf(buf, sizeof buf, "[%u]", i);
        assertValue(testName, testCase+buf, input[i],
                    node1.array[i].array[node1.array[i].firstPos]);
    }
}

static void testDTreeNode1()
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    {
        DTreeSet<cxuint>::Node1 empty;
        cxuint index = empty.lowerBoundN(53, comp, kofval);
        assertValue("DTreeNode1", "empty.lowerBoundN", cxuint(0), index);
        index = empty.upperBoundN(53, comp, kofval);
        assertValue("DTreeNode1", "empty.upperBoundN", cxuint(0), index);
        
        verifyDTreeNode1<cxuint>("DTreeNode1", "empty", empty, 0, 1);
        DTreeSet<cxuint>::Node1 empty1(empty);
        verifyDTreeNode1<cxuint>("DTreeNode1", "empty_copy", empty1, 0, 1);
        DTreeSet<cxuint>::Node1 empty2;
        empty2 = empty;
        verifyDTreeNode1<cxuint>("DTreeNode1", "empty_copy2", empty2, 0, 1);
        DTreeSet<cxuint>::Node1 empty3(std::move(empty));
        verifyDTreeNode1<cxuint>("DTreeNode1", "empty_move", empty3, 0, 1);
        DTreeSet<cxuint>::Node1 empty4;
        empty4 = std::move(empty1);
        verifyDTreeNode1<cxuint>("DTreeNode1", "empty_move2", empty4, 0, 1);
    }
    {
        DTreeSet<cxuint>::Node1 node1;
        createNode1FromArray(node1, dtreeNode1FirstsNum, dtreeNode1Firsts);
        verifyDTreeNode1<cxuint>("DTreeNode1", "node1_0", node1, 0, 1);
        checkNode1Firsts0("DTreeNode1", "node1_0", node1,
                          dtreeNode1FirstsNum, dtreeNode1Firsts);
        // getFirstNode0
        const DTreeSet<cxuint>::Node0* node0f = node1.getFirstNode0();
        assertTrue("DTreeNode1", "node1First", node0f==node1.array);
        const DTreeSet<cxuint>::Node0* node0l = node1.getLastNode0();
        assertTrue("DTreeNode1", "node1Last", node0l==(node1.array + node1.size-1));
        
        // checking copy/move constructor/assignment
        DTreeSet<cxuint>::Node1 node1_1(node1);
        verifyDTreeNode1<cxuint>("DTreeNode1", "node1_copy", node1_1, 0, 1);
        checkNode1Firsts0("DTreeNode1", "node1_copy", node1_1,
                          dtreeNode1FirstsNum, dtreeNode1Firsts);
        DTreeSet<cxuint>::Node1 node1_2;
        node1_2 = node1;
        verifyDTreeNode1<cxuint>("DTreeNode1", "node1_copy2", node1_2, 0, 1);
        checkNode1Firsts0("DTreeNode1", "node1_copy2", node1_2,
                          dtreeNode1FirstsNum, dtreeNode1Firsts);
        DTreeSet<cxuint>::Node1 node1_3(std::move(node1));
        verifyDTreeNode1<cxuint>("DTreeNode1", "node1_move", node1_3, 0, 1);
        checkNode1Firsts0("DTreeNode1", "node1_move", node1_3,
                          dtreeNode1FirstsNum, dtreeNode1Firsts);
        DTreeSet<cxuint>::Node1 node1_4;
        node1_4 = std::move(node1_1);
        verifyDTreeNode1<cxuint>("DTreeNode1", "node1_move2", node1_4, 0, 1);
        checkNode1Firsts0("DTreeNode1", "node1_move2", node1_4,
                          dtreeNode1FirstsNum, dtreeNode1Firsts);
    }
    {
        DTreeSet<cxuint>::Node0 n01, n02;
        for (cxuint x = 0; x < 20; x++)
        {
            n01.insert(x+10, comp, kofval);
            n02.insert(x+40, comp, kofval);
        }
        // constructor with two Node0's
        DTreeSet<cxuint>::Node1 node1(std::move(n01), std::move(n02), kofval);
        verifyDTreeNode1<cxuint>("DTreeNode1", "node1_2n0s", node1, 0, 1);
        for (cxuint x = 0; x < 20; x++)
        {
            cxuint index = node1.array[0].find(x+10, comp, kofval);
            assertTrue("DTreeNode1", "node1_2n0sContent0find",
                       index != node1.array[0].capacity);
            assertValue("DTreeNode1", "node1_2n0sContent0", x+10, node1.array[0][index]);
            
            index = node1.array[1].find(x+40, comp, kofval);
            assertTrue("DTreeNode1", "node1_2n0sContent1find",
                       index != node1.array[1].capacity);
            assertValue("DTreeNode1", "node1_2n0sContent1", x+40, node1.array[1][index]);
        }
    }
    {
        // test reserve0
        DTreeSet<cxuint>::Node1 node1;
        createNode1FromArray(node1, dtreeNode1FirstsNum, dtreeNode1Firsts);
        node1.reserve0(8);
        verifyDTreeNode1<cxuint>("DTreeNode1", "reserve(8)", node1, 0, 1);
        checkNode1Firsts0("DTreeNode1", "reserve(8)", node1,
                          std::min(dtreeNode1FirstsNum, cxuint(8)), dtreeNode1Firsts);
        node1.reserve0(7);
        verifyDTreeNode1<cxuint>("DTreeNode1", "reserve(7)", node1, 0, 1);
        checkNode1Firsts0("DTreeNode1", "reserve(7)", node1,
                          std::min(dtreeNode1FirstsNum, cxuint(7)), dtreeNode1Firsts);
        node1.reserve0(6);
        verifyDTreeNode1<cxuint>("DTreeNode1", "reserve(6)", node1, 0, 1);
        checkNode1Firsts0("DTreeNode1", "reserve(6)", node1,
                          std::min(dtreeNode1FirstsNum, cxuint(6)), dtreeNode1Firsts);
        node1.reserve0(5);
        verifyDTreeNode1<cxuint>("DTreeNode1", "reserve(5)", node1, 0, 1);
        checkNode1Firsts0("DTreeNode1", "reserve(5)", node1,
                          std::min(dtreeNode1FirstsNum, cxuint(5)), dtreeNode1Firsts);
        node1.reserve0(0);
        verifyDTreeNode1<cxuint>("DTreeNode1", "reserve(0)", node1, 0, 1);
    }
    // test lowerBoundN and upperBoundN
    char buf[16];
    for (cxuint size = 1; size <= 8; size++)
    {
        DTreeSet<cxuint>::Node1 node1;
        createNode1FromArray(node1, size, dtreeNode1Firsts2);
        for (cxuint i = 0; i < size; i++)
            for (int diff = -2; diff <= 2; diff += 2)
            {
                const cxuint value = dtreeNode1Firsts2[i] + diff;
                snprintf(buf, sizeof buf, "sz:%u,val:%u", size, value);
                cxuint index = node1.lowerBoundN(value, comp, kofval);
                cxuint expIndex = std::lower_bound(dtreeNode1Firsts2,
                        dtreeNode1Firsts2 + size, value) - dtreeNode1Firsts2;
                assertValue("DTreeNode1", std::string("lowerBoundN:")+buf,
                            expIndex, index);
                index = node1.upperBoundN(value, comp, kofval);
                expIndex = std::upper_bound(dtreeNode1Firsts2,
                        dtreeNode1Firsts2 + size, value) - dtreeNode1Firsts2;
                assertValue("DTreeNode1", std::string("upperBoundN:")+buf,
                            expIndex, index);
            }
    }
    // test insertion
    {
        DTreeSet<cxuint>::Node1 node1;
        for (cxuint idx: dtreeNode1Order)
        {
            const cxuint v = dtreeNode1Firsts2[idx];
            DTreeSet<cxuint>::Node0 node0;
            for (cxuint x = v; x < v+20; x++)
                node0.insert(x, comp, kofval);
            cxuint insIndex = node1.lowerBoundN(v, comp, kofval);
            node1.insertNode0(std::move(node0), insIndex);
            verifyDTreeNode1<cxuint>("DTreeNode1", "instest", node1, 0, 1);
        }
        checkNode1Firsts0("DTreeNode1", "instest", node1, 8, dtreeNode1Firsts2);
    }
    // test erasing
    {
        DTreeSet<cxuint>::Node1 node1;
        createNode1FromArray(node1, 8, dtreeNode1Firsts2);
        std::vector<cxuint> contentIndices;
        for (cxuint i = 0; i < 8; i++)
            contentIndices.push_back(i);
        
        cxuint size = 8;
        for (cxuint idx: dtreeNode1EraseOrder)
        {
            node1.eraseNode0(idx);
            verifyDTreeNode1<cxuint>("DTreeNode1", "erase0", node1, 0, 1);
            contentIndices.erase(contentIndices.begin()+idx);
            size--;
            assertValue("DTreeNode1", "erase0.size", size, cxuint(node1.size));
            // check content
            for (cxuint i = 0; i < size; i++)
                assertValue("DTreeNode1", "erase0.size",
                            dtreeNode1Firsts2[contentIndices[i]],
                            node1.array[i].array[node1.array[i].firstPos]);
        }
    }
    // test merge
    for (cxuint size = 2; size < 8; size++)
        for (cxuint m = 0; m <= size; m++)
        {
            snprintf(buf, sizeof buf, "%u:%u", size, m);
            DTreeSet<cxuint>::Node1 node1, node2;
            createNode1FromArray(node1, m, dtreeNode1Firsts2);
            createNode1FromArray(node2, size-m, dtreeNode1Firsts2+m);
            node1.merge(std::move(node2));
            verifyDTreeNode1<cxuint>("DTreeNode1",
                        std::string("merge") + buf, node1, 0, 1);
            checkNode1Firsts0("DTreeNode1", std::string("merge") + buf, node1,
                          size, dtreeNode1Firsts2);
        }
}

static void createNode1FromValue(DTreeSet<cxuint>::Node1& node1, cxuint value)
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    for (cxuint x = value; x < value+80; x += 20)
    {
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint y = x; y < + x+20; y++)
            node0.insert(y, comp, kofval);
        node1.insertNode0(std::move(node0), node1.size);
    }
}

static void createNode2FromArray(DTreeSet<cxuint>::Node1& node2, cxuint num,
                            const cxuint* input)
{
    for (cxuint i = 0; i < num; i++)
    {
        DTreeSet<cxuint>::Node1 node1;
        createNode1FromValue(node1, input[i]);
        node2.insertNode1(std::move(node1), i);
    }
}

static void checkNode2Firsts0(const std::string& testName, const std::string& testCase,
            const DTreeSet<cxuint>::Node1& node1, cxuint num, const cxuint* input)
{
    assertValue(testName, testCase+".size", num, cxuint(node1.size));
    char buf[20];
    for (cxuint i = 0; i < num; i++)
    {
        snprintf(buf, sizeof buf, "[%u]", i);
        assertValue(testName, testCase+buf, input[i], node1.array1[i].first);
    }
}

static const cxuint dtreeNode2Firsts[] = { 3204, 13506, 19207, 24308, 39309, 54121 };
static const cxuint dtreeNode2FirstsNum = sizeof dtreeNode2Firsts / sizeof(cxuint);

static const cxuint dtreeNode2Firsts2[] = { 3204, 13506, 19207, 24308, 39309, 54121,
                59232, 64212 };
static const cxuint dtreeNode2Firsts2Num = sizeof dtreeNode2Firsts / sizeof(cxuint);

static void testDTreeNode2()
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    {
        DTreeSet<cxuint>::Node1 node2;
        createNode2FromArray(node2, dtreeNode2FirstsNum, dtreeNode2Firsts);
        verifyDTreeNode1<cxuint>("DTreeNode2", "node2_0", node2, 0, 2);
        checkNode2Firsts0("DTreeNode2", "node2_0", node2,
                          dtreeNode2FirstsNum, dtreeNode2Firsts);
        
        // checking copy/move constructor/assignment
        DTreeSet<cxuint>::Node1 node2_1(node2);
        verifyDTreeNode1<cxuint>("DTreeNode2", "node2_copy", node2_1, 0, 2);
        checkNode2Firsts0("DTreeNode2", "node2_copy", node2_1,
                          dtreeNode2FirstsNum, dtreeNode2Firsts);
        DTreeSet<cxuint>::Node1 node2_2;
        node2_2 = node2;
        verifyDTreeNode1<cxuint>("DTreeNode2", "node2_copy2", node2_2, 0, 2);
        checkNode2Firsts0("DTreeNode2", "node2_copy2", node2_2,
                          dtreeNode2FirstsNum, dtreeNode2Firsts);
        DTreeSet<cxuint>::Node1 node2_3(std::move(node2));
        verifyDTreeNode1<cxuint>("DTreeNode1", "node1_move", node2_3, 0, 2);
        checkNode2Firsts0("DTreeNode2", "node2_move", node2_3,
                          dtreeNode2FirstsNum, dtreeNode2Firsts);
        DTreeSet<cxuint>::Node1 node2_4;
        node2_4 = std::move(node2_1);
        verifyDTreeNode1<cxuint>("DTreeNode2", "node2_move2", node2_4, 0, 2);
        checkNode2Firsts0("DTreeNode2", "node2_move2", node2_4,
                          dtreeNode2FirstsNum, dtreeNode2Firsts);
    }
    {
        DTreeSet<cxuint>::Node1 n11, n12;
        createNode1FromValue(n11, 100);
        createNode1FromValue(n12, 400);
        // constructor with two Node1's
        DTreeSet<cxuint>::Node1 node2(std::move(n11), std::move(n12));
        verifyDTreeNode1<cxuint>("DTreeNode2", "node2_2n0s", node2, 0, 2);
        for (cxuint x = 0; x < 4; x++)
        {
            assertValue("DTreeNode2", "node2_2n0sContent0", x*20+100,
                        node2.array1[0].array[x][node2.array1[0].array[x].firstPos]);
            assertValue("DTreeNode2", "node2_2n0sContent1", x*20+400,
                        node2.array1[1].array[x][node2.array1[1].array[x].firstPos]);
        }
    }
    {
        // test reserve1
        DTreeSet<cxuint>::Node1 node2;
        createNode2FromArray(node2, dtreeNode2FirstsNum, dtreeNode2Firsts);
        node2.reserve1(8);
        verifyDTreeNode1<cxuint>("DTreeNode2", "reserve(8)", node2, 0, 2);
        checkNode2Firsts0("DTreeNode2", "reserve(8)", node2,
                          std::min(dtreeNode2FirstsNum, cxuint(8)), dtreeNode2Firsts);
        node2.reserve1(7);
        verifyDTreeNode1<cxuint>("DTreeNode2", "reserve(7)", node2, 0, 2);
        checkNode2Firsts0("DTreeNode2", "reserve(7)", node2,
                          std::min(dtreeNode2FirstsNum, cxuint(7)), dtreeNode2Firsts);
        node2.reserve1(6);
        verifyDTreeNode1<cxuint>("DTreeNode2", "reserve(6)", node2, 0, 2);
        checkNode2Firsts0("DTreeNode2", "reserve(6)", node2,
                          std::min(dtreeNode2FirstsNum, cxuint(6)), dtreeNode2Firsts);
        node2.reserve1(5);
        verifyDTreeNode1<cxuint>("DTreeNode2", "reserve(5)", node2, 0, 2);
        checkNode2Firsts0("DTreeNode2", "reserve(5)", node2,
                          std::min(dtreeNode2FirstsNum, cxuint(5)), dtreeNode2Firsts);
        node2.reserve1(0);
        verifyDTreeNode1<cxuint>("DTreeNode1", "reserve(0)", node2, 0, 2);
    }
    // test lowerBoundN and upperBoundN
    char buf[16];
    for (cxuint size = 1; size <= 8; size++)
    {
        DTreeSet<cxuint>::Node1 node2;
        createNode2FromArray(node2, size, dtreeNode2Firsts2);
        for (cxuint i = 0; i < size; i++)
            for (int diff = -2; diff <= 2; diff += 2)
            {
                const cxuint value = dtreeNode2Firsts2[i] + diff;
                snprintf(buf, sizeof buf, "sz:%u,val:%u", size, value);
                cxuint index = node2.lowerBoundN(value, comp, kofval);
                cxuint expIndex = std::lower_bound(dtreeNode2Firsts2,
                        dtreeNode2Firsts2 + size, value) - dtreeNode2Firsts2;
                assertValue("DTreeNode2", std::string("lowerBoundN:")+buf,
                            expIndex, index);
                index = node2.upperBoundN(value, comp, kofval);
                expIndex = std::upper_bound(dtreeNode2Firsts2,
                        dtreeNode2Firsts2 + size, value) - dtreeNode2Firsts2;
                assertValue("DTreeNode2", std::string("upperBoundN:")+buf,
                            expIndex, index);
            }
    }
    // test insertion
    {
        DTreeSet<cxuint>::Node1 node2;
        for (cxuint idx: dtreeNode1Order)
        {
            const cxuint v = dtreeNode2Firsts2[idx];
            DTreeSet<cxuint>::Node1 node1;
            createNode1FromValue(node1, v);
            cxuint insIndex = node2.lowerBoundN(v, comp, kofval);
            node2.insertNode1(std::move(node1), insIndex);
            verifyDTreeNode1<cxuint>("DTreeNode2", "instest", node2, 0, 2);
        }
        checkNode2Firsts0("DTreeNode2", "instest", node2, 8, dtreeNode2Firsts2);
    }
    // test erasing
    {
        DTreeSet<cxuint>::Node1 node2;
        createNode2FromArray(node2, 8, dtreeNode2Firsts2);
        std::vector<cxuint> contentIndices;
        for (cxuint i = 0; i < 8; i++)
            contentIndices.push_back(i);
        
        cxuint size = 8;
        for (cxuint idx: dtreeNode1EraseOrder)
        {
            node2.eraseNode1(idx);
            verifyDTreeNode1<cxuint>("DTreeNode2", "erase0", node2, 0, 2);
            contentIndices.erase(contentIndices.begin()+idx);
            size--;
            assertValue("DTreeNode2", "erase0.size", size, cxuint(node2.size));
            // check content
            for (cxuint i = 0; i < size; i++)
                assertValue("DTreeNode2", "erase0.size",
                        dtreeNode2Firsts2[contentIndices[i]], node2.array1[i].first);
        }
    }
    // test merge
    for (cxuint size = 2; size < 8; size++)
        for (cxuint m = 0; m <= size; m++)
        {
            snprintf(buf, sizeof buf, "%u:%u", size, m);
            DTreeSet<cxuint>::Node1 node1, node2;
            createNode2FromArray(node1, m, dtreeNode2Firsts2);
            createNode2FromArray(node2, size-m, dtreeNode2Firsts2+m);
            node1.merge(std::move(node2));
            verifyDTreeNode1<cxuint>("DTreeNode2",
                        std::string("merge") + buf, node1, 0, 2);
            checkNode2Firsts0("DTreeNode2", std::string("merge") + buf, node1,
                          size, dtreeNode2Firsts2);
        }
}

/* DTreeSet tests */

struct testDTreeInsert
{ };

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
    retVal |= callTest(testDTreeNode0);
    for (cxuint i = 0; i < sizeof(dtreeNode0OrgArrayTbl) /
                            sizeof(DTreeNode0OrgArrayCase); i++)
        retVal |= callTest(testDTreeOrganizeArray, i, dtreeNode0OrgArrayTbl[i]);
    retVal |= callTest(testDTreeNode0SplitMerge);
    retVal |= callTest(testDTreeNode1);
    retVal |= callTest(testDTreeNode2);
    return retVal;
}
