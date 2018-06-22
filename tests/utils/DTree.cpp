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
#include <numeric>
#include <limits>
#include <memory>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <utility>
#include <set>
#include <random>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/DTree.h>
#include "../TestUtils.h"

using namespace CLRX;

template<typename T>
static void verifyDTreeNodeV(const std::string& testName, const std::string& testCase,
                const typename DTreeSet<T>::NodeV& nv)
{
    char buf[16];
    for (cxuint i = 1; i < nv.size; i++)
    {
        snprintf(buf, sizeof buf, "<=e[%u]", i);
        assertTrue(testName, testCase + buf, nv[i-1] < nv[i]);
    }
    assertTrue(testName, testCase + ".nv.size<=maxSize",
                    nv.size <= DTree<T>::NodeVElemsNum);
}

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
    
    char buf[16];
    for (cxuint i = 1; i < n0.capacity; i++)
    {
        snprintf(buf, sizeof buf, "<=e[%u]", i);
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

enum {
    VERIFY_NODE_NO_MINTOTALSIZE = 1,
    VERIFY_NODE_NO_MAXTOTALSIZE = 2,
    VERIFY_NODE_NO_TOTALSIZE = VERIFY_NODE_NO_MINTOTALSIZE|VERIFY_NODE_NO_MAXTOTALSIZE,
};

template<typename T>
static void verifyDTreeNode1(const std::string& testName, const std::string& testCase,
                const typename DTreeSet<T>::Node1& n1, cxuint level, cxuint maxLevel,
                std::pair<T, bool>* prevValuePtr = nullptr, cxuint flags = 0)
{
    assertTrue(testName, testCase + ".n1.size<=n1.capacity",
                   n1.size <= n1.capacity);
    char buf[16];
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
            snprintf(buf, sizeof buf, "[%u]", i);
            verifyDTreeNode0<T>(testName, testCase + buf, n1.array[i], level+1, maxLevel,
                        prevValuePtr);
            assertValue(testName, testCase + buf + ".index", i, cxuint(n1.array[i].index));
        }
        // checking ordering
        for (cxuint i = 1; i < n1.size; i++)
        {
            snprintf(buf, sizeof buf, "<=f[%u]", i);
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
            snprintf(buf, sizeof buf, "[%u]", i);
            verifyDTreeNode1<T>(testName, testCase + buf, n1.array1[i], level+1, maxLevel,
                        prevValuePtr, flags);
            assertValue(testName, testCase + buf + ".index", i,
                        cxuint(n1.array1[i].index));
        }
        // checking ordering
        for (cxuint i = 1; i < n1.size; i++)
        {
            snprintf(buf, sizeof buf, "<=f[%u]", i);
            assertTrue(testName, testCase + buf, n1.array1[i-1].first <
                        n1.array1[i].first);
        }
    }
    
    assertTrue(testName, testCase + ".size<=n1.capacity", n1.size<=n1.capacity);
    assertTrue(testName, testCase + ".size<=maxNode1Size",
                n1.size <= DTree<T>::maxNode1Size);
    
    if (level != 0 && (flags & VERIFY_NODE_NO_MINTOTALSIZE) == 0)
        assertTrue(testName, testCase + ".totalSize>=minTotalSize",
                    n1.totalSize>=DTree<T>::minTotalSize(maxLevel-level));
    if ((flags & VERIFY_NODE_NO_MAXTOTALSIZE) == 0)
        assertTrue(testName, testCase + ".totalSize<=maxTotalSize",
                    n1.totalSize<=DTree<T>::maxTotalSize(maxLevel-level));
    assertValue(testName, testCase + ".totalSize==n1.totalSize",
                   totalSize, n1.totalSize);
    assertValue(testName, testCase + ".first==n1.first", firstKey, n1.first);
}

template<typename T>
static void verifyDTreeState(const std::string& testName, const std::string& testCase,
            const DTreeSet<T>& dt, cxuint flags = 0)
{
    if (dt.nv.type == DTree<T>::NODEV)
        verifyDTreeNodeV<T>(testName, testCase + "nvroot", dt.nv);
    else if (dt.n0.type == DTree<T>::NODE0)
        verifyDTreeNode0<T>(testName, testCase + "n0root", dt.n0, 0, 0);
    else
    {
        cxuint maxLevel = 1;
        for (const typename DTreeSet<T>::Node1* n1 = &dt.n1; n1->type==DTree<T>::NODE2;
                n1 = n1->array1, maxLevel++);
        
        verifyDTreeNode1<T>(testName, testCase + "n1root", dt.n1, 0, maxLevel,
                        nullptr, flags);
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
            bool erased = node0.erase(v, comp, kofval);
            assertTrue("DTreeNode0", "erase0.erased", erased);
            verifyDTreeNode0<cxuint>("DTreeNode0", "erase0", node0, 0, 0);
            checkContentDTreeNode0("DTreeNode0", "erase0.content1", node0,
                        dtreeNode0ValuesEraseNum-(i+1), dtreeNode0ValuesErase+i+1);
            checkNotFoundDTreeNode0("DTreeNode0", "erase0.content2", node0,
                        i+1, dtreeNode0ValuesErase);
            
            erased = node0.erase(v, comp, kofval);
            assertTrue("DTreeNode0", "erase0.erased2", !erased);
            verifyDTreeNode0<cxuint>("DTreeNode0", "erase0_2", node0, 0, 0);
            checkContentDTreeNode0("DTreeNode0", "erase0_2.content1", node0,
                        dtreeNode0ValuesEraseNum-(i+1), dtreeNode0ValuesErase+i+1);
            checkNotFoundDTreeNode0("DTreeNode0", "erase0_2.content2", node0,
                        i+1, dtreeNode0ValuesErase);
        }
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
                            const cxuint* input, const cxuint* node0Sizes = nullptr)
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    cxuint vnext = 100;
    for (cxuint i = 0; i < num; i++)
    {
        const cxuint v = (input!=nullptr) ? input[i] : vnext;
        DTreeSet<cxuint>::Node0 node0;
        const cxuint node0Size = (node0Sizes != nullptr) ? node0Sizes[i] : 20;
        for (cxuint x = v; x < v+node0Size; x++)
            node0.insert(x, comp, kofval);
        node1.insertNode0(std::move(node0), i, kofval);
        vnext += node0Size;
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
    {
        // test allocate0
        DTreeSet<cxuint>::Node1 node1;
        createNode1FromArray(node1, dtreeNode1FirstsNum, dtreeNode1Firsts);
        node1.allocate0(8);
        verifyDTreeNode1<cxuint>("DTreeNode1", "allocate(8)", node1, 0, 1);
        node1.allocate0(7);
        verifyDTreeNode1<cxuint>("DTreeNode1", "allocate(7)", node1, 0, 1);
        node1.allocate0(6);
        verifyDTreeNode1<cxuint>("DTreeNode1", "allocate(6)", node1, 0, 1);
        node1.allocate0(5);
        verifyDTreeNode1<cxuint>("DTreeNode1", "allocate(5)", node1, 0, 1);
        node1.allocate0(0);
        verifyDTreeNode1<cxuint>("DTreeNode1", "allocate(0)", node1, 0, 1);
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
            node1.insertNode0(std::move(node0), insIndex, kofval);
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
            node1.eraseNode0(idx, kofval);
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
    for (cxuint size = 1; size <= 8; size++)
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

/* testing DTree Node1 splittng */

struct DNode1SplitCase
{
    Array<cxuint> values;
    Array<cxuint> node0Sizes;
    cxuint expectedPoint;
};

static const DNode1SplitCase dtreeNode1SplitTbl[] =
{
    {   // 0 - first
        { 332, 425, 494, 568, 731 },
        { 23, 34, 18, 33, 39 }, 3
    },
    {   // 1 - first
        { 332, 425, 494, 568, 731 },
        { 32, 37, 18, 18, 18 }, 2
    },
    {   // 2 - first
        { 332, 425 },
        { 18, 56 }, 1
    },
    {   // 3 - first
        { 332, 425 },
        { 56, 18 }, 1
    },
    {   // 4 - first
        { 332, 425, 494, 568, 731, 797, 892, 971 },
        { 18, 18, 18, 56, 56, 56, 56, 56 }, 5
    },
};

static void testDTreeNode1Split(cxuint ti, const DNode1SplitCase& testCase)
{
    Identity<cxuint> kofval;
    std::ostringstream oss;
    oss << "Node1Split#" << ti;
    oss.flush();
    std::string caseName = oss.str();
    DTreeSet<cxuint>::Node1 node1, node2;
    createNode1FromArray(node1, testCase.values.size(), testCase.values.data(),
                    testCase.node0Sizes.data());
    node1.splitNode(node2, kofval);
    assertValue("DTreeNode1Split", caseName+".point",
                    testCase.expectedPoint, cxuint(node1.size));
    verifyDTreeNode1<cxuint>("DTreeNode1Split", caseName+"n1", node1, 0, 1);
    checkNode1Firsts0("DTreeNode1Split", caseName+"n1", node1,
                        testCase.expectedPoint, testCase.values.data());
    verifyDTreeNode1<cxuint>("DTreeNode1Split", caseName+"n2", node2, 0, 1);
    checkNode1Firsts0("DTreeNode1Split", caseName+"n2", node2,
                      testCase.values.size() - testCase.expectedPoint,
                      testCase.values.data() + testCase.expectedPoint);
}

/* testing Node1::reorganizeNode0s */

struct DNode1ReorgNode0sCase
{
    Array<cxuint> node0Sizes;
    cxuint start, end;
    bool removeOneNode0;
    Array<cxuint> expNode0Sizes;
};

static const DNode1ReorgNode0sCase dNode1ReorgNode0sCaseTbl[] =
{
    {   // 0 - first
        { 19, 53, 27, 48, 26 }, 1, 4, false,
        { 19, 43, 43, 42, 26 }
    },
    {   // 1 - second
        { 19, 53, 27, 48, 26, 51, 46, 21 }, 2, 7, false,
        { 19, 53, 40, 40, 40, 39, 39, 21 }
    },
    {   // 2 - third
        { 19, 18, 22, 18, 27, 19, 18, 23 }, 0, 8, false,
        { 21, 21, 21, 21, 20, 20, 20, 20 }
    },
    {   // 3 - with removing one node0
        { 19, 18, 22, 18, 27, 19, 18, 23 }, 0, 8, true,
        { 24, 24, 24, 23, 23, 23, 23 }
    },
    {   // 4 - with removing one node0
        { 19, 18, 22, 18, 27, 19, 18, 23 }, 1, 6, true,
        { 19, 26, 26, 26, 26, 18, 23 }
    }
};

static void testDNode1ReorganizeNode0s(cxuint ti, const DNode1ReorgNode0sCase& testCase)
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    
    std::ostringstream oss;
    oss << "Node1Reorg#" << ti;
    oss.flush();
    std::string caseName = oss.str();
    
    DTreeSet<cxuint>::Node1 node1;
    Array<cxuint> values(testCase.node0Sizes.size());
    values[0] = 100;
    for (cxuint i = 1; i < testCase.node0Sizes.size(); i++)
        values[i] = values[i-1] + testCase.node0Sizes[i-1];
    createNode1FromArray(node1, values.size(), values.data(),
                    testCase.node0Sizes.data());
    verifyDTreeNode1<cxuint>("DTreeNode1Reorg", caseName+"n1", node1, 0, 1,
                nullptr, VERIFY_NODE_NO_MAXTOTALSIZE);
    
    node1.reorganizeNode0s(testCase.start, testCase.end, testCase.removeOneNode0);
    verifyDTreeNode1<cxuint>("DTreeNode1Reorg", caseName+"n1_after", node1, 0, 1,
                nullptr, VERIFY_NODE_NO_MAXTOTALSIZE);
    
    // check reorganization0
    char buf[16];
    cxuint expValue = 100;
    for (cxuint i = 0; i < testCase.expNode0Sizes.size(); i++)
    {
        cxuint endExpValue = expValue + testCase.expNode0Sizes[i];
        assertValue("DTreeNode1Reorg", caseName+"n1 size", testCase.expNode0Sizes[i],
                    cxuint(node1.array[i].size));
        for (; expValue < endExpValue; expValue++)
        {
            snprintf(buf, sizeof buf, "%u", expValue);
            cxuint index = node1.array[i].find(expValue, comp, kofval);
            assertTrue("DTreeNode1Reorg", caseName+"n1 index:"+buf, 
                            index<node1.array[i].capacity);
            assertValue("DTreeNode1Reorg", caseName+"n1 value:"+buf, 
                            expValue, node1.array[i][index]);
        }
    }
}

/* testing DTree Node2s */

static void createNode1FromValue(DTreeSet<cxuint>::Node1& node1, cxuint value,
            cxuint size = 80)
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    if (size < 20)
    {
        // small amout
        DTreeSet<cxuint>::Node0 node0;
        for (cxuint y = value; y < + value+size; y++)
            node0.insert(y, comp, kofval);
        node1.insertNode0(std::move(node0), node1.size, kofval);
    }
    else
    {
        cxuint xend = ((size % 20)==0) ? value + size : value + (size/20 - 1)*20;
        cxuint x = value;
        for (x = value; x < xend; x += 20)
        {
            DTreeSet<cxuint>::Node0 node0;
            for (cxuint y = x; y < + x+20; y++)
                node0.insert(y, comp, kofval);
            node1.insertNode0(std::move(node0), node1.size, kofval);
        }
        if (xend < value+size)
        {
            DTreeSet<cxuint>::Node0 node0;
            for (cxuint y = x; y < value+size; y++)
                node0.insert(y, comp, kofval);
            node1.insertNode0(std::move(node0), node1.size, kofval);
        }
    }
}

static void createNode2FromArray(DTreeSet<cxuint>::Node1& node2, cxuint num,
                    const cxuint* input, const cxuint* node1Sizes = nullptr)
{
    cxuint xnext = 100;
    for (cxuint i = 0; i < num; i++)
    {
        DTreeSet<cxuint>::Node1 node1;
        cxuint v = input!=nullptr ? input[i] : xnext;
        if (node1Sizes == nullptr)
            createNode1FromValue(node1, v);
        else
            createNode1FromValue(node1, v, node1Sizes[i]);
        node2.insertNode1(std::move(node1), i);
        v += node2.array1[node2.size-1].totalSize;
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
    {
        // test allocate1
        DTreeSet<cxuint>::Node1 node2;
        createNode2FromArray(node2, dtreeNode2FirstsNum, dtreeNode2Firsts);
        node2.allocate1(8);
        verifyDTreeNode1<cxuint>("DTreeNode2", "allocate(8)", node2, 0, 2);
        node2.allocate1(7);
        verifyDTreeNode1<cxuint>("DTreeNode2", "allocate(7)", node2, 0, 2);
        node2.allocate1(6);
        verifyDTreeNode1<cxuint>("DTreeNode2", "allocate(6)", node2, 0, 2);
        node2.allocate1(5);
        verifyDTreeNode1<cxuint>("DTreeNode2", "allocate(5)", node2, 0, 2);
        node2.allocate1(0);
        verifyDTreeNode1<cxuint>("DTreeNode2", "allocate(0)", node2, 0, 2);
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
    for (cxuint size = 1; size <= 8; size++)
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

/* testing DTree Node2 splittng */

static const DNode1SplitCase dtreeNode2SplitTbl[] =
{
    {   // 0 - first
        { 33200, 42500, 49400, 56800, 73100 },
        { 90, 130, 70, 82, 156 }, 3
    },
    {   // 1 - first
        { 33200, 42500 },
        { 40, 159 }, 1
    },
    {   // 2 - first
        { 33200, 42500 },
        { 159, 40 }, 1
    }
};

static void testDTreeNode2Split(cxuint ti, const DNode1SplitCase& testCase)
{
    Identity<cxuint> kofval;
    std::ostringstream oss;
    oss << "Node2Split#" << ti;
    oss.flush();
    std::string caseName = oss.str();
    DTreeSet<cxuint>::Node1 node1, node2;
    createNode2FromArray(node1, testCase.values.size(), testCase.values.data(),
                    testCase.node0Sizes.data());
    node1.splitNode(node2, kofval);
    assertValue("DTreeNode2Split", caseName+".point",
                    testCase.expectedPoint, cxuint(node1.size));
    verifyDTreeNode1<cxuint>("DTreeNode2Split", caseName+"n1", node1, 0, 2, nullptr,
            VERIFY_NODE_NO_MINTOTALSIZE);
    checkNode2Firsts0("DTreeNode2Split", caseName+"n1", node1,
                        testCase.expectedPoint, testCase.values.data());
    verifyDTreeNode1<cxuint>("DTreeNode2Split", caseName+"n2", node2, 0, 2, nullptr,
            VERIFY_NODE_NO_MINTOTALSIZE);
    checkNode2Firsts0("DTreeNod21Split", caseName+"n2", node2,
                      testCase.values.size() - testCase.expectedPoint,
                      testCase.values.data() + testCase.expectedPoint);
}

/* testing Node1::reorganizeNode1s */

struct DNode1ReorgNode1sCase
{
    Array<Array<cxuint> > node0Sizes;
    cxuint start, end;
    bool removeOneNode1;
    Array<cxuint> expNode1Sizes;
};

static const DNode1ReorgNode1sCase dNode1ReorgNode1sCaseTbl[] =
{
    {   // 0 - first
        { { 34, 18, 42, 31 }, { 35, 51, 18 }, { 19, 21, 18 } },
        0, 3, false,
        { 3, 3, 4 }
    },
    {   // 1 - second
        { { 18, 19, 19, 21 }, { 23, 19, 38 }, { 49, 51, 47 } },
        0, 3, false,
        { 5, 3, 2 }
    },
    {   // 2 - third
        { { 18, 19, 19, 21 }, { 21, 19, 19 }, { 53, 51, 52 } },
        0, 3, false,
        { 5, 3, 2 }
    },
    {   // 4 - longer
        { { 19, 21, 21, 23 }, { 55, 55, 54, 53 }, { 18, 18, 19 }, { 18, 20, 21 },
          { 21, 23, 24 }, { 18, 23 }, { 19, 23 }, { 24, 21, 18 } },
        1, 7, false,
        { 4, 2, 2, 4, 3, 3, 3, 3 }
    },
    {   // 5 - longer
        { { 19, 21, 21, 23 }, { 25, 25, 54, 33 }, { 18, 18, 19 }, { 18, 20, 21 },
          { 21, 23, 24 }, { 18, 23 }, { 19, 23 }, { 24, 21, 18 } },
        1, 6, false,
        { 4, 2, 2, 4, 4, 3, 2, 3 }
    },
    {   // 6 - with high node children number
        { { 51, 56 }, { 18, 18, 19, 20, 18, 20, 19, 18 },
          { 18, 18, 19, 20, 18, 20, 19, 18 },
          { 18, 18, 19, 20, 18, 20, 19, 18 }, { 56, 56 },
          { 18, 18, 19, 20, 18, 20, 19, 18 }, { 30, 39, 53 },
          { 18, 18, 19, 20, 18, 20, 19, 18 } },
        0, 8, false,
        { 4, 7, 7, 7, 3, 7, 4, 8 }
    },
    {   // 7 - with high node children number
        { { 51, 56 }, { 18, 18, 19, 20, 18, 20, 19, 18 },
          { 18, 18, 19, 20, 18, 20, 19, 18 },
          { 18, 18, 19, 20, 18, 20, 19, 18 }, { 56, 18, 18, 18, 18 },
          { 18, 18, 19, 20, 18, 20, 19, 18 }, { 18, 18, 18, 18, 18, 18, 18 },
          { 18, 18, 19, 20, 18, 20, 19, 18 } },
        0, 8, false,
        { 4, 7, 7, 7, 6, 8, 8, 7 }
    },
    {   // 7 - try to provoke higher children number than maxNode1Size
        { { 56, 56, 56, 56, 18, 18 }, { 18, 18, 19, 20, 18, 20, 19, 18 }, },
        0, 2, false,
        { 6, 8 }
    },
    {   // 8 - with removeNode1
        { { 34, 18, 42, 31 }, { 35, 51, 18 }, { 19, 21, 18 } },
        0, 3, true,
        { 5, 5 }
    },
    {   // 9 - with removeNode1
        { { 19, 21, 21, 23 }, { 25, 25, 54, 33 }, { 18, 18, 19 }, { 18, 20, 21 },
          { 21, 23, 24 }, { 18, 23 }, { 19, 23 }, { 24, 21, 18 } },
        1, 6, true,
        { 4, 3, 4, 4, 4, 2, 3 }
    }
};

static void testDNode1ReorganizeNode1s(cxuint ti, const DNode1ReorgNode1sCase& testCase)
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    
    std::ostringstream oss;
    oss << "Node2Reorg#" << ti;
    oss.flush();
    std::string caseName = oss.str();
    
    DTreeSet<cxuint>::Node1 node2;
    cxuint flatNodeEntriesNum = 0;
    for (const Array<cxuint>& n0Sizes: testCase.node0Sizes)
        flatNodeEntriesNum += n0Sizes.size();
    Array<cxuint> flatNode0Sizes(flatNodeEntriesNum);
    
    cxuint value = 100;
    cxuint flatN0Index = 0;
    cxuint expNode1Children = 0;
    for (cxuint i = 0; i < testCase.node0Sizes.size(); i++)
    {
        const Array<cxuint>& n0Sizes = testCase.node0Sizes[i];
        expNode1Children += n0Sizes.size();
        DTreeSet<cxuint>::Node1 node1;
        Array<cxuint> values(n0Sizes.size());
        values[0] = value;
        for (cxuint j = 0; j < n0Sizes.size(); j++)
        {
            values[j] = value;
            value += n0Sizes[j];
            flatNode0Sizes[flatN0Index++] = n0Sizes[j];
        }
        createNode1FromArray(node1, values.size(), values.data(), n0Sizes.data());
        node2.insertNode1(std::move(node1), node2.size);
    }
    verifyDTreeNode1<cxuint>("DTreeNode2Reorg", caseName+"n1", node2, 0, 2,
                nullptr, VERIFY_NODE_NO_TOTALSIZE);
    
    node2.reorganizeNode1s(testCase.start, testCase.end, kofval, testCase.removeOneNode1);
    verifyDTreeNode1<cxuint>("DTreeNode2Reorg", caseName+"n1_after", node2, 0, 2,
                nullptr, VERIFY_NODE_NO_TOTALSIZE);
    
    assertValue("DTreeNode2Reorg", caseName+"n1 children",
                testCase.node0Sizes.size() - testCase.removeOneNode1,
                testCase.expNode1Sizes.size());
    
    // check reorganization0
    char buf[16];
    cxuint expValue = 100;
    flatN0Index = 0;
    cxuint resNode1Children = 0;
    for (cxuint i = 0; i < testCase.expNode1Sizes.size(); i++)
    {
        resNode1Children += testCase.expNode1Sizes[i];
        snprintf(buf, sizeof buf, "[%u]", i);
        assertValue("DTreeNode2Reorg", caseName+"n2 size"+buf,
                    testCase.expNode1Sizes[i], cxuint(node2.array1[i].size));
        const DTreeSet<cxuint>::Node1& node1 = node2.array1[i];
        for (cxuint j = 0; j < testCase.expNode1Sizes[i]; j++)
        {
            assertValue("DTreeNode2Reorg", caseName+"n2n1 size",
                    flatNode0Sizes[flatN0Index+j], cxuint(node1.array[j].size));
            
            cxuint endExpValue = expValue + flatNode0Sizes[flatN0Index+j];
            for (; expValue < endExpValue; expValue++)
            {
                snprintf(buf, sizeof buf, "%u:%u", j, expValue);
                cxuint index = node1.array[j].find(expValue, comp, kofval);
                assertTrue("DTreeNode2Reorg", caseName+"n2n1 index:"+buf,
                                index<node1.array[j].capacity);
                assertValue("DTreeNode2Reorg", caseName+"n2n1 value:"+buf,
                                expValue, node1.array[j][index]);
            }
        }
        flatN0Index += testCase.expNode1Sizes[i];
    }
    assertValue("DTreeNode2Reorg", caseName+"n1n1 children", expNode1Children,
                resNode1Children);
}

/* DTree IterBase tests */

struct DIterBaseCase
{
    Array<Array<cxuint> > treeNodeSizes;
    Array<cxuint> traverseCases;
};

static const DIterBaseCase diterBaseCaseTbl[] =
{
    {   // 0 - first
        {
            { 4 },
            { 3, 2, 4, 5 },
            { 27, 19, 20,  41, 39,  31, 33, 18, 21,  51, 54, 12, 19, 16 }
        },
        { 0, 26, 29, 36, 55, 56, 58, 93, 98, 111, 399, 400, 401 }
    },
    {   // 1 - second
        {
            { 5 },
            { 27, 19, 20, 27, 31 }
        },
        { 0, 26, 36, 55, 56 }
    },
    {   // 2 - greater
        {
            { 5 },
            { 3, 3, 5, 2, 4 },
            { 6, 4, 2,  7, 8, 6, 3, 4, 2, 6, 2,  7, 8,  5, 4, 4, 6 },
            { 21, 28, 41, 51, 46, 34,  31, 19, 23, 44,  55, 54,
              32, 21, 27, 27, 34, 35, 39,  19, 21, 36, 18, 25, 29, 41, 42,
                46, 47, 31, 19, 29, 33,
              38, 31, 21,  51, 17, 51, 19,  41, 29,  35, 31, 21, 26, 53, 44,  19, 20,
              19 ,20, 21, 22, 23, 24, 25,  41, 40, 39, 38, 37, 36, 35, 34,
              43, 45, 41, 24, 31,  43, 29, 21, 55,  23, 29, 28, 45,
                40, 30, 28, 21, 49, 50 }
        },
        { 21, 234, 465, 1001, 1421, 1769, 2421, 2697, 2757 }
    }
};

static DTreeSet<cxuint>::Node1 createDTreeFromNodeSizes(const std::string& testName,
            const std::string& caseName, const Array<Array<cxuint> >& treeNodeSizes,
            cxuint& elemsNum, cxuint valueStep = 1)
{
    std::less<cxuint> comp;
    Identity<cxuint> kofval;
    // construct DTree from nodeSizes
    Array<DTreeSet<cxuint>::Node1> parents;
    Array<DTreeSet<cxuint>::Node1> children;
    Array<DTreeSet<cxuint>::Node0> children0;
    
    cxint level = treeNodeSizes.size()-1;
    elemsNum = 0;
    {
        const Array<cxuint>& nodeSizes = treeNodeSizes[level--];
        cxuint value = 100;
        children0.resize(nodeSizes.size());
        for (cxuint i = 0; i < nodeSizes.size(); i++)
        {
            for (cxuint k = 0; k < nodeSizes[i]; k++, value += valueStep, elemsNum++)
                children0[i].insert(value, comp, kofval);
        }
    }
    
    // last node1s level
    if (level >= 0)
    {
        const Array<cxuint>& nodeSizes = treeNodeSizes[level--];
        parents.resize(nodeSizes.size());
        
        cxuint j = 0;
        for (cxuint i = 0; i < nodeSizes.size(); i++)
        {
            DTreeSet<cxuint>::Node1& p = parents[i];
            for (cxuint k = 0; k < nodeSizes[i]; k++, j++)
                p.insertNode0(std::move(children0[j]), p.size, kofval);
        }
        assertValue(testName, caseName+".nodeLevelSize", j, cxuint(children0.size()));
        children = parents;
        parents.clear();
    }
    for (; level >= 0; level--)
    {
        const Array<cxuint>& nodeSizes = treeNodeSizes[level];
        parents.resize(nodeSizes.size());
        
        cxuint j = 0;
        for (cxuint i = 0; i < nodeSizes.size(); i++)
        {
            DTreeSet<cxuint>::Node1& p = parents[i];
            for (cxuint k = 0; k < nodeSizes[i]; k++, j++)
                p.insertNode1(std::move(children[j]), p.size);
        }
        assertValue(testName, caseName+".nodeLevelSize", j, cxuint(children.size()));
        children = parents;
        parents.clear();
    }
    return children[0];
}

static void testDTreeIterBase(cxuint ti, const DIterBaseCase& testCase)
{
    std::ostringstream oss;
    oss << "DIterBase" << ti;
    oss.flush();
    std::string caseName = oss.str();
    cxuint elemsNum = 0;
    // construct DTree from nodeSizes
    DTreeSet<cxuint>::Node1 root = createDTreeFromNodeSizes("DTreeIterBase", caseName,
                testCase.treeNodeSizes, elemsNum);
    
    // IterBase testing
    DTreeSet<cxuint>::IterBase iterStart(root.getFirstNode0(), 0);
    iterStart.index = iterStart.n0->firstPos;
    DTreeSet<cxuint>::IterBase iter = iterStart;
    
    char buf[16];
    for (cxuint i = 1; i < elemsNum; i++)
    {
        iter.next();
        snprintf(buf, sizeof buf, "it%u", i);
        assertValue("DTreeIterBase", caseName+".nextValue."+buf,
                    i+100, (*iter.n0)[iter.index]);
    }
    iter.next();
    DTreeSet<cxuint>::IterBase iterEnd(iter);
    assertValue("DTreeIterBase", caseName+".diff(end,end)", ssize_t(0),
                iterEnd.diff(iterEnd));
    
    for (cxuint i = elemsNum; i > 0; i--)
    {
        iter.prev();
        snprintf(buf, sizeof buf, "it%u", i);
        assertValue("DTreeIterBase", caseName+".prevValue."+buf,
                    i-1+100, (*iter.n0)[iter.index]);
    }
    
    iter = DTreeSet<cxuint>::IterBase(root.getFirstNode0(), 0);
    for (cxuint i = 1; i < elemsNum; i++)
    {
        DTreeSet<cxuint>::IterBase thisIter = iter;
        thisIter.next(i);
        snprintf(buf, sizeof buf, "it%u", i);
        assertValue("DTreeIterBase", caseName+".nextNValue."+buf,
                    i+100, (*thisIter.n0)[thisIter.index]);
        assertValue("DTreeIterBase", caseName+".diff(end-"+buf+")",
                ssize_t(elemsNum)-ssize_t(i), iterEnd.diff(thisIter));
        assertValue("DTreeIterBase", caseName+".diff("+buf+"-end)",
                ssize_t(i)-ssize_t(elemsNum), thisIter.diff(iterEnd));
        assertValue("DTreeIterBase", caseName+".diff("+buf+"-"+buf+")", ssize_t(0),
                thisIter.diff(thisIter));
        thisIter.prev(i+1);
    }
    for (cxuint i = 1; i < elemsNum; i++)
    {
        DTreeSet<cxuint>::IterBase thisIter = iterEnd;
        thisIter.prev(i);
        snprintf(buf, sizeof buf, "it%u", i);
        assertValue("DTreeIterBase", caseName+".prevNValue."+buf,
                    (elemsNum-i)+100, (*thisIter.n0)[thisIter.index]);
        assertValue("DTreeIterBase", caseName+".diff(start-"+buf+")",
                ssize_t(i)-ssize_t(elemsNum), iterStart.diff(thisIter));
        assertValue("DTreeIterBase", caseName+".diff("+buf+"-start)",
                ssize_t(elemsNum)-ssize_t(i), thisIter.diff(iterStart));
    }
    
    assertValue("DTreeIterBase", caseName+".diff(end-start)", ssize_t(elemsNum),
                iterEnd.diff(iterStart));
    assertValue("DTreeIterBase", caseName+".diff(start-end)", -ssize_t(elemsNum),
                iterStart.diff(iterEnd));
    
    iter = iterStart;
    iter.prev();
    
    for (cxuint i = 0; i < testCase.traverseCases.size(); i++)
    {
        const cxuint tPos = testCase.traverseCases[i];
        DTreeSet<cxuint>::IterBase posIter = iterStart;
        posIter.next(tPos);
        
        for (cxuint j = 0; j < elemsNum; j++)
        {
            if (tPos == j)
                continue;
            DTreeSet<cxuint>::IterBase thisIter = posIter;
            thisIter.step(ssize_t(j)-ssize_t(tPos));
            snprintf(buf, sizeof buf, "%u:%u", tPos, j);
            assertValue("DTreeIterBase", caseName+".iterstep:"+buf,
                        cxuint(100+j), (*thisIter.n0)[thisIter.index]);
            assertValue("DTreeIterBase", caseName+".iterdiff:"+buf,
                        ssize_t(j)-ssize_t(tPos), thisIter.diff(posIter));
            assertValue("DTreeIterBase", caseName+".iterdiffn:"+buf,
                        ssize_t(tPos)-ssize_t(j), posIter.diff(thisIter));
        }
    }
}

template<typename Iter>
static void testDTreeIterTempl(const std::string& testName,
            DTreeSet<cxuint>::Node1& root)
{
    Iter iterStart(root.getFirstNode0(), 0);
    Iter iter = iterStart;
    // test behaviour
    Iter prevIter = iter++;
    assertTrue(testName, "postInc", prevIter == iterStart);
    iter = iterStart;
    Iter nextIter = ++iter;
    assertTrue(testName, "preInc", nextIter != iterStart);
    assertTrue(testName, "preInc2", nextIter == iter);
    iter = iterStart;
    ++iter; // next
    ++iter; // yet next
    Iter iterSecond = iter;
    prevIter = iter--;
    assertTrue(testName, "postDec", prevIter == iterSecond);
    iter = iterSecond;
    nextIter = --iter;
    assertTrue(testName, "preDec", nextIter != iterSecond);
    assertTrue(testName, "preDec2", nextIter == iter);
    // check addition and subtract
    iterSecond = iter = iterStart;
    for (cxuint i = 0; i < 4; i++)
        iterSecond++;
    iter += 4;
    assertTrue(testName, "add0", iter == iterSecond);
    iter -= 4;
    assertTrue(testName, "sub0", iter == iterStart);
    nextIter = iter + 4;
    assertTrue(testName, "add1it", iter == iterStart);
    assertTrue(testName, "add1", nextIter == iterSecond);
    prevIter = nextIter - 4;
    assertTrue(testName, "sub1it", nextIter == iterSecond);
    assertTrue(testName, "sub1", prevIter == iterStart);
    assertValue(testName, "diff", ssize_t(4), iterSecond-iterStart);
    assertValue(testName, "diff", -ssize_t(4), iterStart-iterSecond);
    assertValue(testName, "get", cxuint(100+4), *iterSecond);
}

static void testDTreeIter()
{
    cxuint elemsNum = 0;
    DTreeSet<cxuint>::Node1 root = createDTreeFromNodeSizes("DTreeXIter", "create",
                diterBaseCaseTbl[0].treeNodeSizes, elemsNum);
    testDTreeIterTempl<DTreeSet<cxuint>::Iter>("DTreeIter", root);
    testDTreeIterTempl<DTreeSet<cxuint>::ConstIter>("DTreeConstIter", root);
    
    DTreeSet<cxuint>::Iter iter(root.getFirstNode0(), 0);
    DTreeSet<cxuint>::ConstIter constIter(root.getFirstNode0(), 0);
    assertTrue("DTreeIter", "iter==constIter", iter == constIter);
    assertTrue("DTreeIter", "iter!=constIter", !(iter != constIter));
    assertValue("DTreeIter", "iter-constIter", ssize_t(0), iter - constIter);
    assertTrue("DTreeIter", "constIter==iter", constIter == iter);
    assertTrue("DTreeIter", "constIter!=iter", !(constIter != iter));
    assertValue("DTreeIter", "constIter-iter", ssize_t(0), constIter - iter);
}

/* DTree findReorgBounds0 */

struct DTreeFindReorgBounds0Case
{
    Array<cxuint> node0Sizes;
    cxuint n0Index;
    cxuint n0Size;
    cxint expLeft, expRight;
    bool expRemoveNode;
};

static const DTreeFindReorgBounds0Case dtreeFindReorgBounds0Tbl[] =
{
    {   // 0
        { 18, 22, 45, 56, 41, 48 },
        3, 56,
        1, 5, true
    },
    {   // 1
        { 47, 52, 49, 56, 55, 52 },
        3, 56,
        0, 5, false
    },
    {   // 2
        { 47, 56, 49, 48, 42 },
        1, 56,
        0, 4, false
    },
    {   // 3
        { 25, 18, 56, 18, 18, 18, 23 },
        2, 56,
        1, 3, true
    },
    {   // 4
        { 21, 24, 42, 41, 56, 47, 27, 21 },
        4, 56,
        2, 6, true
    }
};

static void testDTreeFindReorgBounds0(cxuint ti, const DTreeFindReorgBounds0Case& testCase)
{
    std::ostringstream oss;
    oss << "DFindReorgBounds0_" << ti;
    oss.flush();
    std::string caseName = oss.str();
    
    DTreeSet<cxuint>::Node1 node1;
    createNode1FromArray(node1, testCase.node0Sizes.size(), nullptr,
                    testCase.node0Sizes.data());
    cxint resLeft = UINT_MAX, resRight = UINT_MAX;
    bool resRemoveNode = false;
    DTreeSet<cxuint>::findReorgBounds0(testCase.n0Index, &node1, testCase.n0Size,
                            resLeft, resRight, &resRemoveNode);
    
    assertValue("DTree", caseName+".left", testCase.expLeft, resLeft);
    assertValue("DTree", caseName+".right", testCase.expRight, resRight);
    assertValue("DTree", caseName+".removeNode0", cxuint(testCase.expRemoveNode),
                cxuint(resRemoveNode));
}

/* DTree findReorgBounds1 */

static const DTreeFindReorgBounds0Case dtreeFindReorgBounds1Tbl[] =
{
    {   // 0
        { 75, 101, 83, 160, 135, 129 },
        3, 160,
        2, 4, false
    },
    {   // 1
        { 160, 135, 129, 75, 101, 83 },
        0, 160,
        0, 3, true
    },
    {   // 2
        { 75, 101, 135, 129, 83, 160 },
        5, 160,
        3, 5, false
    }
};

static void testDTreeFindReorgBounds1(cxuint ti, const DTreeFindReorgBounds0Case& testCase)
{
    std::ostringstream oss;
    oss << "DFindReorgBounds1_" << ti;
    oss.flush();
    std::string caseName = oss.str();
    
    DTreeSet<cxuint>::Node1 node1;
    createNode2FromArray(node1, testCase.node0Sizes.size(), nullptr,
                    testCase.node0Sizes.data());
    
    cxint resLeft = UINT_MAX, resRight = UINT_MAX;
    bool resRemoveNode = false;
    DTreeSet<cxuint>::findReorgBounds1(node1.array1 + testCase.n0Index,
                    &node1, testCase.n0Size, 224, resLeft, resRight, &resRemoveNode);
    
    assertValue("DTree", caseName+".left", testCase.expLeft, resLeft);
    assertValue("DTree", caseName+".right", testCase.expRight, resRight);
    assertValue("DTree", caseName+".removeNode1", cxuint(testCase.expRemoveNode),
                cxuint(resRemoveNode));
}

/* DTreeSet tests */

static const Array<Array<cxuint> > dtreeFindCaseTbl[] =
{
    {   // 0 - first
        { 4 },
        { 3, 2, 4, 5 },
        { 27, 19, 20,  41, 39,  31, 33, 18, 21,  51, 54, 12, 19, 16 }
    },
    {   // 1 - second
        { 5 },
        { 27, 19, 20, 27, 31 }
    },
    {   // 2 - greater
        { 5 },
        { 3, 3, 5, 2, 4 },
        { 6, 4, 2,  7, 8, 6, 3, 4, 2, 6, 2,  7, 8,  5, 4, 4, 6 },
        { 21, 28, 41, 51, 46, 34,  31, 19, 23, 44,  55, 54,
            32, 21, 27, 27, 34, 35, 39,  19, 21, 36, 18, 25, 29, 41, 42,
            46, 47, 31, 19, 29, 33,
            38, 31, 21,  51, 17, 51, 19,  41, 29,  35, 31, 21, 26, 53, 44,  19, 20,
            19 ,20, 21, 22, 23, 24, 25,  41, 40, 39, 38, 37, 36, 35, 34,
            43, 45, 41, 24, 31,  43, 29, 21, 55,  23, 29, 28, 45,
            40, 30, 28, 21, 49, 50 }
    }
};

static void testDTreeFind(cxuint ti, const Array<Array<cxuint> >& treeNodeSizes)
{
    std::ostringstream oss;
    oss << "DTreeFind" << ti;
    oss.flush();
    std::string caseName = oss.str();
    
    cxuint elemsNum = 0;
    // construct DTree from nodeSizes
    DTreeSet<cxuint>::Node1 root = createDTreeFromNodeSizes("DTreeIterBase", caseName,
                treeNodeSizes, elemsNum, 3);
    DTreeSet<cxuint> set(root);
    // checking lower_bound
    char buf[16];
    for (cxuint v = 97; v < 100 + elemsNum*3+3; v++)
    {
        snprintf(buf, sizeof buf, "v=%u", v);
        auto it = set.lower_bound(v);
        if (it != set.end())
            assertTrue("DTree", caseName+".lower_bound "+buf+" <=", v<=*it);
        if (it != set.begin())
        {
            --it;
            assertTrue("DTree", caseName+".lower_bound "+buf+" >", v>*it);
        }
    }
    // checking upper_bound
    for (cxuint v = 97; v < 100 + elemsNum*3+3; v++)
    {
        snprintf(buf, sizeof buf, "v=%u", v);
        auto it = set.upper_bound(v);
        if (it != set.end())
            assertTrue("DTree", caseName+".upper_bound "+buf+" <", v<*it);
        if (it != set.begin())
        {
            --it;
            assertTrue("DTree", caseName+".upper_bound "+buf+" >=", v>=*it);
        }
    }
    // checking find
    for (cxuint v = 97; v < 100 + elemsNum*3+3; v++)
    {
        snprintf(buf, sizeof buf, "v=%u", v);
        auto it = set.find(v);
        if (v < 100 || v >= 100 + elemsNum*3 || (v-100)%3 != 0)
            assertTrue("DTree", caseName+".find "+buf+" not found", it==set.end());
        else
            assertTrue("DTree", caseName+".find "+buf+" found", it!=set.end());
    }
    
    const DTreeSet<cxuint>& cset = set;
    cset.lower_bound(100);
    cset.upper_bound(100);
    cset.find(100);
}

static const Array<cxuint> dtreeInsertCaseTbl[] =
{
    { 1, 3, 8, 2, 7 },
    { 3, 1, 8, 2, 7, 9, 0 },
    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
      21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
      41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 58 }
};

static void checkDTreeContent(const std::string& testName, const std::string& caseName,
            const DTreeSet<cxuint>& set, const size_t entriesNum, const cxuint* inValues)
{
    assertValue(testName, caseName+".size", entriesNum, set.size());
    
    std::unique_ptr<cxuint[]> sortedValues(new cxuint[entriesNum]);
    std::copy(inValues, inValues + entriesNum, sortedValues.get());
    std::sort(sortedValues.get(), sortedValues.get() + entriesNum);
    char buf[16];
    auto it = set.begin();
    for (cxuint i = 0; i < entriesNum; i++, ++it)
    {
        snprintf(buf, sizeof buf, "[%u]", i);
        assertValue(testName, caseName+".value"+buf, sortedValues[i], *it);
    }
}

static void testDTreeInsert(cxuint ti, const Array<cxuint>& valuesToInsert)
{
    std::ostringstream oss;
    oss << "DTreeInsert" << ti;
    oss.flush();
    std::string caseName = oss.str();
    
    char buf[16];
    DTreeSet<cxuint> set;
    for (cxuint i = 0; i < valuesToInsert.size(); i++)
    {
        auto res = set.insert(valuesToInsert[i]);
        snprintf(buf, sizeof buf, "[%u]", i);
        assertTrue("DTreeInsert", caseName+buf+".inserted", res.second);
        assertValue("DTreeInsert", caseName+buf+".value", valuesToInsert[i], *res.first);
        
        verifyDTreeState("DTreeInsert", caseName+buf+".test", set);
        checkDTreeContent("DTreeInsert", caseName+buf+".content",
                        set, i+1, valuesToInsert.data());
        
        res = set.insert(valuesToInsert[i]);
        assertTrue("DTreeInsert", caseName+buf+".notinserted", !res.second);
        assertValue("DTreeInsert", caseName+buf+".value2", valuesToInsert[i], *res.first);
    }
}

static const cxuint node0MaxSize = DTreeSet<cxuint>::maxNode0Size;

static void testDTreeInsert2()
{
    char buf[16];
    DTreeSet<cxuint> set;
    std::vector<cxuint> values;
    for (cxuint v = 100; v < 100 + (node0MaxSize+1)*10; v += 10)
    {
        auto res = set.insert(v);
        values.push_back(v);
        snprintf(buf, sizeof buf, "[%u]", v);
        assertTrue("DTreeInsert", std::string("insert0")+buf+".inserted", res.second);
        assertValue("DTreeInsert", std::string("insert0")+buf+".value", v, *res.first);
        verifyDTreeState("DTreeInsert", std::string("insert0")+buf+".test", set);
    }
    
    for (cxuint x = 0; x < 2; x++)
    {
        for (cxuint v = 122+x; v < 122+x + (node0MaxSize+1)*10; v += 10)
        {
            auto res = set.insert(v);
            values.push_back(v);
            snprintf(buf, sizeof buf, "[%u]", v);
            assertTrue("DTreeInsert", std::string("insert1")+buf+".inserted", res.second);
            assertValue("DTreeInsert", std::string("insert1")+buf+".value", v, *res.first);
            verifyDTreeState("DTreeInsert", std::string("insert1")+buf+".test", set);
        }
        // check content
        std::sort(values.begin(), values.end());
        checkDTreeContent("DTreeInsert", "insert01:content",
                            set, values.size(), values.data());
    }
    
    /// to level depth 2
    set.clear();
    values.clear();
    
    cxuint v = 100;
    for (v = 100; v < 100 + (node0MaxSize)*10*4 + 10; v += 10)
    {
        auto res = set.insert(v);
        values.push_back(v);
        snprintf(buf, sizeof buf, "[%u]", v);
        assertTrue("DTreeInsert", std::string("insert2")+buf+".inserted", res.second);
        assertValue("DTreeInsert", std::string("insert2")+buf+".value", v, *res.first);
    }
    verifyDTreeState("DTreeInsert", "insert2.test", set);
    checkDTreeContent("DTreeInsert", "insert2:content", set, values.size(), values.data());
    for (; v < 100 + (node0MaxSize)*10*6; v += 10)
    {
        auto res = set.insert(v);
        values.push_back(v);
        snprintf(buf, sizeof buf, "[%u]", v);
        assertTrue("DTreeInsert", std::string("insert2")+buf+".inserted2", res.second);
        assertValue("DTreeInsert", std::string("insert2")+buf+".value2", v, *res.first);
    }
    verifyDTreeState("DTreeInsert", "insert2.test", set);
    checkDTreeContent("DTreeInsert", "insert2:content", set, values.size(), values.data());
}

static void testDTreeInsertInitList()
{
    DTreeSet<cxuint> set;
    std::vector<cxuint> values;
    set.insert({ 34, 67, 11, 99 });
    values.insert(values.begin(), { 34, 67, 11, 99 });
    verifyDTreeState("DTreeInsert", "insertInitList.test", set);
    checkDTreeContent("DTreeInsert", "insertInitList:content", set,
                      values.size(), values.data());
    
    DTreeSet<cxuint> set2({ 34, 67, 11, 99 });
    verifyDTreeState("DTreeInsert", "constrInitList.test", set2);
    checkDTreeContent("DTreeInsert", "constrInitList:content", set2,
                      values.size(), values.data());
}

static void testDTreeInsertIters()
{
    std::vector<cxuint> values;
    values.insert(values.begin(), { 34, 67, 11, 99 });
    DTreeSet<cxuint> set;
    set.insert(values.begin(), values.end());
    verifyDTreeState("DTreeInsert", "insertIters.test", set);
    checkDTreeContent("DTreeInsert", "insertIters:content", set,
                      values.size(), values.data());
    
    DTreeSet<cxuint> set2(values.begin(), values.end());
    verifyDTreeState("DTreeInsert", "constrIters.test", set2);
    checkDTreeContent("DTreeInsert", "constrIters:content", set2,
                      values.size(), values.data());
}

struct DTreeForceBehCase
{
    Array<Array<cxuint> > treeNodeSizes;
    cxuint step;
    cxuint valueToProcess;
};

static const DTreeForceBehCase dtreeInsertBehCaseTbl[] =
{
    {   // 0 - first - force reorganizeNode0s in level 0
        {
            { 8 },
            { 18, 18, 21, 28, 56, 23, 18, 18 }
        },
        3, 359
    },
    {   // 1 - second - force reorganizeNode1s in level 1
        {
            { 8 },
            { 4, 5, 2, 8, 3, 5, 6, 5 },
            { 18, 18, 20, 22,
              22, 21, 26, 19, 31,
              25, 31,
              31, 23, 32, 41, 23, 27, 28, 19,
              23, 23, 28,
              22, 19, 18, 24, 26,
              18, 18, 18, 19, 18, 18,
              23, 22, 18, 22, 31 }
        },
        3, 1022
    },
    {   // 2 - third - force reorganizeNode0s 0 and reorganizeNode0s 1
        {
            { 8 },
            { 4, 5, 2, 8, 3, 5, 6, 5 },
            { 18, 18, 20, 22,
              22, 21, 26, 19, 31,
              25, 31,
              26, 23, 27, 56, 23, 26, 24, 19,
              23, 23, 28,
              22, 19, 18, 24, 26,
              18, 18, 18, 19, 18, 18,
              23, 22, 18, 22, 31 }
        },
        3, 1119
    },
    {   // 3 - fourth - reorganize and split to level 3
        {
            { 8 },
            { 4, 5, 2, 8, 3, 5, 6, 5 },
            { 18, 18, 20, 22,
              22, 21, 26, 19, 31,
              25, 35,
              31, 23, 32, 41, 23, 27, 28, 19,
              23, 23, 28,
              22, 19, 18, 24, 26,
              18, 18, 21, 19, 18, 19,
              23, 25, 18, 22, 31 }
        },
        3, 1287
    },
    {   // 4 - split in level 0, reorganize in level 1
        {
            { 8 },
            { 4, 5, 2, 7, 3, 5, 6, 5 },
            { 18, 18, 20, 22,
              22, 21, 26, 19, 31,
              25, 31,
              26, 24, 37, 56, 36, 24, 21,
              23, 23, 28,
              22, 19, 18, 22, 26,
              18, 18, 18, 19, 18, 18,
              22, 22, 18, 22, 31 }
        },
        3, 1157
    },
    {   // 5 - split in level 0 and in level 1
        {
            { 7 },
            { 4, 5, 2, 7, 3, 5, 6 },
            { 18, 18, 20, 22,
              22, 21, 26, 19, 31,
              25, 31,
              26, 24, 37, 56, 36, 24, 21,
              23, 23, 28,
              22, 19, 18, 22, 26,
              18, 18, 18, 19, 18, 18 }
        },
        3, 1157
    },
    {   // 6 - split in level 0, reorganize in level 1, level 2 split
        {
            { 8 },
            { 4, 5, 2, 7, 3, 5, 6, 5 },
            { 18, 18, 24, 22,
              22, 21, 26, 19, 31,
              25, 32,
              26, 24, 37, 56, 36, 24, 21,
              23, 23, 32,
              22, 19, 18, 22, 26,
              18, 18, 18, 19, 18, 18,
              22, 22, 18, 27, 31 }
        },
        3, 1178
    },
    {   // 7 - reorganize in 0, reorganize in level 1, split in level 2
        {
            { 2 },
            { 5, 8 },
            { 3, 2, 4, 5, 2,
              4, 5, 2, 8, 3, 5, 6, 5 },
            { 32, 43, 31,
              37, 41,
              23, 21, 41, 19,
              19, 51, 21, 41, 20,
              51, 52,
              //----------
              18, 18, 24, 22,
              22, 21, 26, 19, 31,
              25, 32,
              21, 22, 31, 56, 31, 24, 18, 21,
              23, 23, 32,
              22, 19, 18, 22, 26,
              18, 18, 18, 19, 18, 18,
              22, 22, 18, 27, 31 }
        },
        3, 2789
    },
    {   // 8 - split in 0, reorganize in level 1, split in level 2
        {
            { 2 },
            { 5, 8 },
            { 3, 2, 4, 5, 2,
              4, 5, 2, 7, 3, 5, 6, 5 },
            { 32, 43, 31,
              37, 41,
              23, 21, 41, 19,
              19, 51, 21, 41, 20,
              51, 52,
              //----------
              18, 18, 24, 22,
              22, 21, 26, 19, 31,
              25, 32,
              21, 29, 34, 56, 31, 24, 29,
              23, 23, 32,
              22, 19, 18, 22, 26,
              18, 18, 18, 19, 18, 18,
              22, 22, 18, 27, 31 }
        },
        3, 2789
    },
    {   // 9 - split in 0, split in level 2
        {
            { 2 },
            { 5, 8 },
            { 3, 2, 4, 5, 2,
              4, 5, 2, 7, 3, 5, 6, 5 },
            { 32, 43, 31,
              37, 41,
              23, 21, 41, 19,
              19, 51, 21, 41, 20,
              51, 52,
              //----------
              18, 18, 24, 22,
              22, 24, 26, 19, 31,
              25, 33,
              21, 21, 34, 56, 30, 24, 29,
              23, 23, 32,
              22, 19, 18, 22, 26,
              22, 18, 19, 19, 18, 18,
              22, 22, 18, 27, 31 }
        },
        3, 2789
    },
    {   // 10 - force split in level 0 (right)
        {
            { 7 },
            { 18, 18, 21, 56, 23, 18, 18 }
        },
        3, 414
    },
    {   // 11 - force split in level 0 (left)
        {
            { 7 },
            { 18, 18, 21, 56, 23, 18, 18 }
        },
        3, 303
    },
    {   // 12 - force reorganizeNode0s in level 0
        {
            { 8 },
            { 26, 25, 21, 26, 53, 23, 32, 18 }
        },
        3, 359
    },
    {   // 13 - force reorganizeNode0s in level 0
        {
            { 8 },
            { 26, 25, 21, 26, 53, 23, 32, 18 }
        },
        3, 600
    },
    {   // 14 - add after first elem in node0
        {
            { 8 },
            { 26, 24, 21, 26, 33, 23, 30, 18 }
        },
        3, 251
    },
    {   // 15 - add before last elem in node0
        {
            { 8 },
            { 26, 24, 21, 26, 33, 23, 30, 18 }
        },
        3, 249
    },
    {   // 16 - add before last elem in node0
        {
            { 8 },
            { 26, 24, 21, 26, 33, 23, 30, 18 }
        },
        3, 246
    },
    {   // 17 - force reorganizeNode1s in level 1 (before first elem in node0)
        {
            { 8 },
            { 4, 5, 2, 8, 3, 5, 6, 5 },
            { 18, 18, 20, 22,
              22, 21, 26, 19, 31,
              25, 31,
              31, 23, 32, 41, 23, 27, 28, 19,
              23, 23, 28,
              22, 19, 18, 24, 26,
              18, 18, 18, 19, 18, 18,
              23, 22, 18, 22, 31 }
        },
        3, 1020
    },
    {   // 18 - force reorganizeNode1s in level 1 (before last elem in node0)
        {
            { 8 },
            { 4, 5, 2, 8, 3, 5, 6, 5 },
            { 18, 18, 20, 22,
              22, 21, 26, 19, 31,
              25, 31,
              31, 23, 32, 41, 23, 27, 28, 19,
              23, 23, 28,
              22, 19, 18, 24, 26,
              18, 18, 18, 19, 18, 18,
              23, 22, 18, 22, 31 }
        },
        3, 1113
    }
};

static void testDTreeInsertBehaviour(cxuint ti, const DTreeForceBehCase& testCase)
{
    std::ostringstream oss;
    oss << "DTreeInsertBeh" << ti;
    oss.flush();
    std::string caseName = oss.str();
    
    std::vector<cxuint> values;
    
    cxuint elemsNum = 0;
    // construct DTree from nodeSizes
    const DTreeSet<cxuint>::Node1 node1 = createDTreeFromNodeSizes("DTree", caseName,
                testCase.treeNodeSizes, elemsNum, testCase.step);
    DTreeSet<cxuint> set(node1);
    for (cxuint v: set)
        values.push_back(v);
    verifyDTreeState("DTree", caseName+".test", set, VERIFY_NODE_NO_MINTOTALSIZE);
    
    auto res = set.insert(testCase.valueToProcess);
    
    verifyDTreeState("DTree", caseName+".test", set, VERIFY_NODE_NO_MINTOTALSIZE);
    values.insert(std::lower_bound(values.begin(), values.end(), testCase.valueToProcess),
                    testCase.valueToProcess);
    checkDTreeContent("DTree", caseName+".content", set, values.size(), values.data());
    
    assertTrue("DTree", caseName+".inserted", res.second);
    assertValue("DTree", caseName+".value", cxuint(testCase.valueToProcess), *res.first);
}

static void testDTreeInsertRandom()
{
    std::minstd_rand0 ranen(13453312);
    DTreeSet<cxuint> set;
    std::set<cxuint> values;
    
    char buf[16];
    for (cxuint i = 0; i < 3000; i++)
    {
        //std::cout << "dt" << i << std::endl;
        cxuint value = ranen();
        values.insert(value);
        set.insert(value);
        
        if ((i%10) == 9)
        {
            snprintf(buf, sizeof buf, "%u", i);
            verifyDTreeState("DTree", std::string("InsertRan")+buf+".test", set);
            // check content
            auto vit = values.begin();
            auto sit = set.begin();
            for (; vit != values.end() && sit != set.end(); ++vit, ++sit)
                assertValue("DTree", std::string("InsertRan")+buf+".content", *vit, *sit);
            assertTrue("DTree", std::string("InsertRan")+buf+"contentend",
                    vit==values.end() && sit==set.end());
        }
    }
}

/* DTreeSet erase */

static void testDTreeErase0()
{
    DTreeSet<cxuint> set;
    for (cxuint i = 0; i < 40; i++)
        set.insert(i);
    verifyDTreeState("DTree", "erase0.test", set);
    
    char buf[16];
    for (cxuint i = 0; i < 40; i++)
    {
        snprintf(buf, sizeof buf, "[%u]", i);
        auto it = set.erase(set.find(i));
        if (i == 39)
            assertTrue("DTree", std::string("erase0")+buf+".it", it==set.end());
        else
            assertValue("DTree", std::string("erase0")+buf+".it", cxuint(i+1), *it);
        
        verifyDTreeState("DTree", std::string("erase0")+buf+".test", set);
    }
    
    set.clear();
    
    for (cxuint i = 0; i < 40; i++)
        set.insert(i);
    
    auto it = set.erase(set.find(27));
    assertValue("DTree", "erase0x[0].it", cxuint(28), *it);
    it = set.erase(set.find(39));
    assertTrue("DTree", "erase0x[1].it", it==set.end());
    it = set.erase(set.find(0));
    assertValue("DTree", "erase0x[2].it", cxuint(1), *it);
    
    for (cxuint i = 10; i < 20; i++)
    {
        snprintf(buf, sizeof buf, "[%u]", i);
        auto it = set.erase(set.find(i));
        assertValue("DTree", std::string("erase0x")+buf+".it", cxuint(i+1), *it);
        verifyDTreeState("DTree", std::string("erase0x")+buf+".test", set);
    }
}

static const DTreeForceBehCase dtreeEraseBehCaseTbl[] =
{
    {   // 0 - first - in level 1 - it causes nothing
        {
            { 8 },
            { 18, 18, 21, 28, 56, 23, 18, 18 }
        },
        3, 358
    },
    {   // 1 - second - force merge
        {
            { 8 },
            { 18, 18, 21, 28, 56, 23, 18, 18 }
        },
        3, 187
    },
    {   // 2 - force merge in righ side
        {
            { 8 },
            { 24, 18, 21, 28, 56, 23, 18, 18 }
        },
        3, 196
    },
    {   // 3 - force merge in right side (first node0)
        {
            { 8 },
            { 18, 27, 21, 28, 34, 23, 18, 18 }
        },
        3, 121
    },
    {   // 4 - force merge in left side (last node0)
        {
            { 8 },
            { 18, 27, 21, 28, 34, 23, 27, 18 }
        },
        3, 649
    },
    {   // 5 - force reogranizeNode0
        {
            { 8 },
            { 20, 47, 18, 41, 23, 18, 22, 21 }
        },
        3, 334
    },
    {   // 6 - force reogranizeNode0
        {
            { 8 },
            { 20, 52, 18, 38, 23, 18, 22, 21 }
        },
        3, 334
    },
    {   // 7 - force merge in level 1 (left side)
        {
            { 3 },
            { 3, 3, 5 },
            { 41, 22, 34,
              27, 22, 25,
              32, 21, 25, 23, 32 }
        },
        3, 406
    },
    {   // 8 - force merge in level 1 (right side)
        {
            { 3 },
            { 5, 3, 3 },
            { 32, 21, 25, 23, 32,
              27, 22, 25,
              41, 22, 34 }
        },
        3, 589
    },
    {   // 9 - force reorganize in level 1
        {
            { 5 },
            { 2, 5, 3, 6, 4 },
            { 39, 53,
              42, 51, 35, 43, 42,
              27, 22, 25,
              42, 21, 53, 34, 41, 22,
              23, 43, 21, 27 }
        },
        3, 1126
    },
    {   // 10 - force reorganize in level 1 (forced by too many children in neighbors)
        {
            { 5 },
            { 2, 6, 3, 6, 4 },
            { 39, 53,
              18, 19, 20, 22, 23, 24,
              27, 22, 25,
              19, 24, 24, 19, 21, 22,
              23, 43, 21, 27 }
        },
        3, 934
    },
    {   // 11 - force remove root
        {
            { 2 },
            { 18, 24 }
        },
        3, 130
    },
    {   // 12 - force remove root 2
        {
            { 2 },
            { 3, 3 },
            { 37, 35, 40,
              21, 24, 29 }
        },
        3, 529
    },
    {   // 13 - force reorganizeNodeXs in level 0 and merge left in level 1
        {
            { 3 },
            { 3, 2, 5 },
            { 41, 22, 34,
              18, 56,
              32, 21, 25, 23, 32 }
        },
        3, 409
    },
    {   // 14 - force reorganizeNodeXs in level 0 and 1
        {
            { 3 },
            { 3, 2, 5 },
            { 48, 50, 53,
              18, 56,
              47, 37, 38, 40, 33 }
        },
        3, 601
    },
    {   // 15 - force merge in level 0 and 1
        {
            { 3 },
            { 3, 3, 4 },
            { 48, 50, 53,
              18, 20, 36,
              36, 32, 28, 40 }
        },
        3, 601
    },
    {   // 16 - force merge in level 0 and 1 (right, right)
        {
            { 3 },
            { 3, 3, 4 },
            { 48, 50, 53,
              36, 18, 20,
              36, 32, 28, 40 }
        },
        3, 697
    },
    {   // 17 - force merge in level 0 and 1 (left,left)
        {
            { 3 },
            { 3, 3, 4 },
            { 43, 42, 43,
              20, 18, 36,
              36, 32, 28, 40 }
        },
        3, 586
    },
    {   // 18 - force merge in level 0 and 1 (left,left)
        {
            { 2 },
            { 4, 3 },
            { 23, 26, 25, 23,
              20, 18, 36 }
        },
        3, 472
    },
    {   // 19 - force merge - iterator beyond right side
        {
            { 8 },
            { 18, 18, 21, 28, 56, 23, 18, 18 }
        },
        3, 205 // last elem in second node0
    },
    {   // 20 - force merge in level 0 and 1 (left,left) (last elem in right merge)
        {
            { 3 },
            { 3, 3, 4 },
            { 43, 42, 43,
              20, 36, 18,
              36, 34, 28, 40 }
        },
        3, 703
    },
    {   // 21 - force merge in level 0 (last elem in right merge)
        {
            { 3 },
            { 3, 3, 4 },
            { 43, 42, 43,
              30, 36, 18,
              36, 34, 28, 40 }
        },
        3, 733
    },
    {   // 22 - in level 1 - it causes nothing (last elem in node0)
        {
            { 8 },
            { 18, 18, 21, 28, 56, 23, 18, 18 }
        },
        3, 352
    },
    {   // 23 - in level 1 - it causes nothing (last elem in node0)
        {
            { 8 },
            { 18, 18, 21, 28, 56, 23, 18, 18 }
        },
        3, 208
    },
    {   // 24 - force merge in level 0 and 1 (left,left) (first elem in left merge)
        {
            { 3 },
            { 3, 3, 4 },
            { 43, 42, 43,
              18, 20, 36,
              36, 34, 28, 40 }
        },
        3, 484
    },
    {   // 25 - force merge in level 0 and 1 (left,left) (second elem in left merge)
        {
            { 3 },
            { 3, 3, 4 },
            { 43, 42, 43,
              18, 20, 36,
              36, 34, 28, 40 }
        },
        3, 487
    },
    {   // 26 - merge 0 , reorganize 1, merge 2
        {
            { 3 },
            { 3, 2, 4 },
            { 3, 3, 4,  4, 3,  4, 2, 3, 4 },
            { 34, 32, 47,
              35, 21, 19,
              31, 18, 25, 37,
              // -------
              56, 56, 56, 56,
              18, 20, 36,
              // -------
              21, 28, 22, 33,
              37, 39,
              32, 24, 19,
              24, 38, 29, 31
            }
        },
        3, 1711
    },
    {   // 27 - merge 0, merge 2
        {
            { 3 },
            { 3, 2, 4 },
            { 3, 3, 4,  4, 3,  4, 2, 3, 4 },
            { 34, 32, 47,
              35, 21, 19,
              31, 18, 25, 37,
              // -------
              55, 56, 56, 56,
              18, 20, 37,
              // -------
              21, 28, 22, 33,
              37, 39,
              32, 24, 19,
              24, 38, 29, 31
            }
        },
        3, 1711
    },
    {   // 28 - merge 0 , reorganize 1, merge 2, replace root
        {
            { 2 },
            { 3, 2 },
            { 3, 3, 4,  4, 3 },
            { 34, 32, 47,
              35, 21, 19,
              31, 18, 25, 37,
              // -------
              56, 56, 56, 56,
              18, 20, 36,
            }
        },
        3, 1711
    },
    {   // 29 - in level 1 - it causes nothing (last elem in node0)
        {
            { 8 },
            { 18, 18, 21, 28, 56, 23, 18, 18 }
        },
        3, 208
    },
    {   // 30 - update parent first
        {
            { 3 },
            { 3, 2, 4 },
            { 3, 3, 4,  4, 3,  4, 2, 3, 4 },
            { 34, 32, 47,
              35, 21, 19,
              31, 18, 25, 37,
              // -------
              56, 56, 56, 56,
              22, 24, 36,
              // -------
              21, 28, 22, 33,
              37, 39,
              32, 24, 19,
              24, 38, 29, 31
            }
        },
        3, 1915
    },
    {   // 31 - erase last item
        {
            { 8 },
            { 18, 18, 21, 28, 56, 23, 22, 21 }
        },
        3, 718
    }
};

static void testDTreeEraseBehaviour(cxuint ti, const DTreeForceBehCase& testCase)
{
    std::ostringstream oss;
    oss << "DTreeEraseBeh" << ti;
    oss.flush();
    std::string caseName = oss.str();
    
    std::vector<cxuint> values;
    
    cxuint elemsNum = 0;
    // construct DTree from nodeSizes
    const DTreeSet<cxuint>::Node1 node1 = createDTreeFromNodeSizes("DTree", caseName,
                testCase.treeNodeSizes, elemsNum, testCase.step);
    DTreeSet<cxuint> set(node1);
    for (cxuint v: set)
        if (v != testCase.valueToProcess)
            values.push_back(v);
    verifyDTreeState("DTree", caseName+".test", set);
    auto lastIt = set.end();
    --lastIt;
    const bool isLast = (*lastIt == testCase.valueToProcess);
    
    auto toErase = set.find(testCase.valueToProcess);
    assertValue("DTree", caseName+".toErase", cxuint(testCase.valueToProcess), *toErase);
    auto it = set.erase(toErase);
    
    verifyDTreeState("DTree", caseName+".testafter", set);
    checkDTreeContent("DTree", caseName+".content", set, values.size(), values.data());
    
    if (!isLast)
        assertValue("DTree", caseName+".value",
                        cxuint(testCase.valueToProcess+testCase.step), *it);
    else
        assertTrue("DTree", caseName+".end", it==set.end());
}

static void testDTreeEraseRandom()
{
    std::minstd_rand0 ranen(753561124);
    DTreeSet<cxuint> set;
    std::vector<cxuint> values;
    
    char buf[16];
    for (cxuint i = 0; i < 1500; i++)
    {
        //std::cout << "dt" << i << std::endl;
        cxuint value = ranen();
        values.push_back(value);
        set.insert(value);
    }
    // sort values
    std::sort(values.begin(), values.end());
    {
        verifyDTreeState("DTree", "EraseIRan.test", set);
        // check content
        auto vit = values.begin();
        auto sit = set.begin();
        for (; vit != values.end() && sit != set.end(); ++vit, ++sit)
            assertValue("DTree", "EraseIRan.content", *vit, *sit);
        assertTrue("DTree", "EraseIRan.contentend", vit==values.end() && sit==set.end());
    }
    
    /// erasing
    for (cxuint i = 0; i < 1500; i++)
    {
        const cxuint index = ranen() % values.size();
        set.erase(set.begin() + index);
        values.erase(values.begin() + index);
        
        if ((i%10) == 9)
        {
            snprintf(buf, sizeof buf, "%u", i);
            verifyDTreeState("DTree", std::string("EraseRan")+buf+".test", set);
            // check content
            auto vit = values.begin();
            auto sit = set.begin();
            for (; vit != values.end() && sit != set.end(); ++vit, ++sit)
                assertValue("DTree", std::string("EraseRan")+buf+".content", *vit, *sit);
            assertTrue("DTree", std::string("EraseRan")+buf+"contentend",
                    vit==values.end() && sit==set.end());
        }
    }
}

static const cxuint eraseValueCaseValues[] = { 33, 473 };

static void testEraseValue()
{
    DTreeSet<cxuint> set;
    set.insert(33);
    set.insert(473);
    verifyDTreeState("DTree", "eraseValue.test", set);
    
    size_t count = set.erase(34);
    assertValue("DTree", "eraseValue(34)", size_t(0), count);
    checkDTreeContent("DTree", "eraseValue(34).content", set, 2, eraseValueCaseValues);
    
    count = set.erase(33);
    assertValue("DTree", "eraseValue(33)", size_t(1), count);
    verifyDTreeState("DTree", "eraseValue.test2", set);
    checkDTreeContent("DTree", "eraseValue(33).content", set, 1, eraseValueCaseValues+1);
    
    count = set.erase(33);
    assertValue("DTree", "eraseValue(33)2", size_t(0), count);
    checkDTreeContent("DTree", "eraseValue(33)2.content", set, 1, eraseValueCaseValues+1);
    
    count = set.erase(475);
    assertValue("DTree", "eraseValue(475)", size_t(0), count);
    checkDTreeContent("DTree", "eraseValue(475).content", set, 1, eraseValueCaseValues+1);
    
    count = set.erase(473);
    assertValue("DTree", "eraseValue(473)", size_t(1), count);
    verifyDTreeState("DTree", "eraseValue.test3", set);
    checkDTreeContent("DTree", "eraseValue(473).content", set, 0, eraseValueCaseValues);
    
    count = set.erase(473);
    assertValue("DTree", "eraseValue(473)2", size_t(0), count);
    checkDTreeContent("DTree", "eraseValue(473).content", set, 0, eraseValueCaseValues);
}

static void testDTreeConstructors()
{
    DTreeSet<cxuint> setx;
    setx.insert(48);
    setx.insert(557);
    {
        DTreeSet<cxuint> set;
        set.insert(33);
        set.insert(473);
        
        verifyDTreeState("DTree", "construct.test", set);
        checkDTreeContent("DTree", "construct.content", set, 2, eraseValueCaseValues);
        
        DTreeSet<cxuint> set2(set);
        verifyDTreeState("DTree", "construct2.test", set2);
        checkDTreeContent("DTree", "construct2.content", set2, 2, eraseValueCaseValues);
        
        DTreeSet<cxuint> set2a(setx);
        set2a = set;
        verifyDTreeState("DTree", "assign.test", set2a);
        checkDTreeContent("DTree", "assign.content", set2a, 2, eraseValueCaseValues);
        
        DTreeSet<cxuint> set3(std::move(set));
        verifyDTreeState("DTree", "construct3.test", set3);
        checkDTreeContent("DTree", "construct3.content", set3, 2, eraseValueCaseValues);
        
        DTreeSet<cxuint> set3a(setx);
        set3a = std::move(set2);
        verifyDTreeState("DTree", "assign3a.test", set3a);
        checkDTreeContent("DTree", "assign3a.content", set3a, 2, eraseValueCaseValues);
    }
    DTreeSet<cxuint> setx2;
    {
        DTreeSet<cxuint> set;
        std::vector<cxuint> values;
        for (cxuint v = 10; v < 200; v++)
        {
            set.insert(v);
            values.push_back(v);
        }
        
        verifyDTreeState("DTree", "2construct.test", set);
        checkDTreeContent("DTree", "2construct.content", set,
                          values.size(), values.data());
        
        DTreeSet<cxuint> set2(set);
        verifyDTreeState("DTree", "2construct2.test", set2);
        checkDTreeContent("DTree", "2construct2.content", set2,
                          values.size(), values.data());
        
        DTreeSet<cxuint> set2a(setx);
        set2a = set;
        verifyDTreeState("DTree", "2assign.test", set2a);
        checkDTreeContent("DTree", "2assign.content", set2a,
                        values.size(), values.data());
        
        DTreeSet<cxuint> set3(std::move(set));
        verifyDTreeState("DTree", "2construct3.test", set3);
        checkDTreeContent("DTree", "2construct3.content", set3,
                        values.size(), values.data());
        
        DTreeSet<cxuint> set3a(setx);
        set3a = std::move(set2);
        verifyDTreeState("DTree", "2assign3a.test", set3a);
        checkDTreeContent("DTree", "2assign3a.content", set3a,
                        values.size(), values.data());
        
        setx2 = set3a;
    }
    {
        DTreeSet<cxuint> set;
        set.insert(33);
        set.insert(473);
        
        verifyDTreeState("DTree", "3construct.test", set);
        checkDTreeContent("DTree", "3construct.content", set, 2, eraseValueCaseValues);
        
        DTreeSet<cxuint> set2a(setx2);
        set2a = set;
        verifyDTreeState("DTree", "3assign.test", set2a);
        checkDTreeContent("DTree", "3assign.content", set2a, 2, eraseValueCaseValues);
        
        DTreeSet<cxuint> set3a(setx2);
        set3a = std::move(set2a);
        verifyDTreeState("DTree", "3assign3a.test", set3a);
        checkDTreeContent("DTree", "3assign3a.content", set3a, 2, eraseValueCaseValues);
    }
    
    {
        DTreeSet<cxuint> set;
        std::vector<cxuint> values;
        for (cxuint v = 10; v < 1100; v++)
        {
            set.insert(v);
            values.push_back(v);
        }
        
        verifyDTreeState("DTree", "4construct.test", set);
        checkDTreeContent("DTree", "4construct.content", set,
                          values.size(), values.data());
        
        DTreeSet<cxuint> set2(set);
        verifyDTreeState("DTree", "4construct2.test", set2);
        checkDTreeContent("DTree", "4construct2.content", set2,
                          values.size(), values.data());
        
        DTreeSet<cxuint> set2a(setx);
        set2a = set;
        verifyDTreeState("DTree", "4assign.test", set2a);
        checkDTreeContent("DTree", "4assign.content", set2a,
                        values.size(), values.data());
        
        DTreeSet<cxuint> set3(std::move(set));
        verifyDTreeState("DTree", "4construct3.test", set3);
        checkDTreeContent("DTree", "4construct3.content", set3,
                          values.size(), values.data());
        
        DTreeSet<cxuint> set4(setx);
        set4 = std::move(set2);
        verifyDTreeState("DTree", "4assign4.test", set4);
        checkDTreeContent("DTree", "4assign4.content", set4,
                        values.size(), values.data());
    }
}

static void testDTreeEmptySizeClear()
{
    DTreeSet<cxuint> setEmpty;
    DTreeSet<cxuint> set1;
    set1.insert(48);
    set1.insert(557);
    DTreeSet<cxuint> set1a(set1);
    
    DTreeSet<cxuint> set2;
    for (cxuint v = 10; v < 200; v++)
        set2.insert(v);
    DTreeSet<cxuint> set2a(set2);
    
    assertTrue("DTree", "setEmpty.empty()", setEmpty.empty());
    assertTrue("DTree", "set1.empty()", !set1.empty());
    assertTrue("DTree", "set2.empty()", !set2.empty());
    
    assertValue("DTree", "setEmpty.size()", size_t(0), setEmpty.size());
    assertValue("DTree", "set1.size()", size_t(2), set1.size());
    assertValue("DTree", "set2.size()", size_t(190), set2.size());
    
    setEmpty.clear();
    set1.clear();
    set2.clear();
    
    assertTrue("DTree", "setEmpty.empty()2", setEmpty.empty());
    assertTrue("DTree", "set1.empty()2", set1.empty());
    assertTrue("DTree", "set2.empty()2", set2.empty());
    
    assertValue("DTree", "setEmpty.size()2", size_t(0), setEmpty.size());
    assertValue("DTree", "set1.size()2", size_t(0), set1.size());
    assertValue("DTree", "set2.size()2", size_t(0), set2.size());
    
    setEmpty = setEmpty;
    set1 = set1a;
    set2 = set2a;
    
    assertTrue("DTree", "setEmpty.empty()3", setEmpty.empty());
    assertTrue("DTree", "set1.empty()3", !set1.empty());
    assertTrue("DTree", "set2.empty()3", !set2.empty());
    
    assertValue("DTree", "setEmpty.size()3", size_t(0), setEmpty.size());
    assertValue("DTree", "set1.size()3", size_t(2), set1.size());
    assertValue("DTree", "set2.size()3", size_t(190), set2.size());
    
    set1 = set1;
    set2 = set2;
    
    assertTrue("DTree", "set1.empty()4", !set1.empty());
    assertTrue("DTree", "set2.empty()4", !set2.empty());
    
    assertValue("DTree", "set1.size()4", size_t(2), set1.size());
    assertValue("DTree", "set2.size()4", size_t(190), set2.size());
    
    set1.erase(48);
    verifyDTreeState("DTree", "3assign3a.test", set1);
    set1.erase(557);
    verifyDTreeState("DTree", "3assign3a.test", set1);
    for (cxuint v = 10; v < 200; v++)
    {
        set2.erase(v);
        verifyDTreeState("DTree", "3assign3a.test", set2);
    }
    
    assertTrue("DTree", "set1.empty()5", set1.empty());
    assertTrue("DTree", "set2.empty()5", set2.empty());
    
    assertValue("DTree", "set1.size()5", size_t(0), set1.size());
    assertValue("DTree", "set2.size()5", size_t(0), set2.size());
}

static void testDTreeSwap()
{
    DTreeSet<cxuint> set0;
    DTreeSet<cxuint> set1;
    set1.insert(48);
    set1.insert(557);
    DTreeSet<cxuint> set2;
    for (cxuint v = 10; v < 200; v++)
        set2.insert(v);
    
    std::swap(set1, set2); // node0<->node1
    std::swap(set1, set0); // node1<->empty
    std::swap(set1, set2); // empty<->node0
}

static void testDTreeCompare()
{
    DTreeSet<cxuint> set, set2, set3;
    for (cxuint v = 10; v < 200; v++)
    {
        set.insert(v);
        set3.insert(v+1);
    }
    set2 = set;
    assertTrue("DTree", "set==set2", set==set2);
    assertTrue("DTree", "set!=set2", !(set!=set2));
    assertTrue("DTree", "set==set3", !(set==set3));
    assertTrue("DTree", "set!=set3", set!=set3);
    
    assertTrue("DTree", "set<set3", set<set3);
    assertTrue("DTree", "set<set2", !(set<set2));
    assertTrue("DTree", "set<=set3", set<=set3);
    assertTrue("DTree", "set<=set2", set<=set2);
    assertTrue("DTree", "set3>set", set3>set);
    assertTrue("DTree", "set2>set", !(set2>set));
    assertTrue("DTree", "set3>=set", set3>=set);
    assertTrue("DTree", "set2>=set", set2>=set);
}

static void testDTreeBeginEnd()
{
    DTreeSet<cxuint> set, set2, set3;
    for (cxuint v = 10; v < 30; v++)
        set.insert(v);
    for (cxuint v = 10; v < 200; v++)
        set2.insert(v);
    for (cxuint v = 10; v < 1500; v++)
        set3.insert(v);
    
    assertValue("DTree", "set1.begin()", cxuint(10), *set.begin());
    assertValue("DTree", "set2.begin()", cxuint(10), *set2.begin());
    assertValue("DTree", "set3.begin()", cxuint(10), *set3.begin());
    assertValue("DTree", "set1.cbegin()", cxuint(10), *set.cbegin());
    assertValue("DTree", "set2.cbegin()", cxuint(10), *set2.cbegin());
    assertValue("DTree", "set3.cbegin()", cxuint(10), *set3.cbegin());
    assertValue("DTree", "set1.end()", cxuint(29), *--set.end());
    assertValue("DTree", "set2.end()", cxuint(199), *--set2.end());
    assertValue("DTree", "set3.end()", cxuint(1499), *--set3.end());
    set.erase(10);
    set2.erase(10);
    set3.erase(10);
    set.erase(29);
    set2.erase(199);
    set3.erase(1499);
    assertValue("DTree", "set1.begin()2", cxuint(11), *set.begin());
    assertValue("DTree", "set2.begin()2", cxuint(11), *set2.begin());
    assertValue("DTree", "set3.begin()2", cxuint(11), *set3.begin());
    assertValue("DTree", "set1.cbegin()2", cxuint(11), *set.cbegin());
    assertValue("DTree", "set2.cbegin()2", cxuint(11), *set2.cbegin());
    assertValue("DTree", "set3.cbegin()2", cxuint(11), *set3.cbegin());
    assertValue("DTree", "set1.end()2", cxuint(28), *--set.end());
    assertValue("DTree", "set2.end()2", cxuint(198), *--set2.end());
    assertValue("DTree", "set3.end()2", cxuint(1498), *--set3.end());
    
    const DTree<cxuint>& cset = set;
    assertValue("DTree", "const set1.begin()2", cxuint(11), *cset.begin());
    assertValue("DTree", "const set1.begin()2", cxuint(28), *--cset.end());
    
    set.clear();
    assertTrue("DTree", "set1.begin()==set1.end()", set.begin()==set.end());
}

static void testDTreeMapUsage()
{
    DTreeMap<cxuint, uint64_t> map;
    map.insert(std::make_pair(55, uint64_t(256)));
    assertValue("DTreeMap", "find(55)", uint64_t(256), map.find(55)->second);
    map.at(55);
    map[55] = 567;
    assertValue("DTreeMap", "[55]", uint64_t(567), map[55]);
    assertValue("DTreeMap", "at(55)", uint64_t(567), map.at(55));
    assertValue("DTreeMap", "begin()", uint64_t(567), map.begin()->second);
    assertValue("DTreeMap", "cbegin()", uint64_t(567), map.cbegin()->second);
    assertValue("DTreeMap", "find(55)", uint64_t(567), map.find(55)->second);
    assertValue("DTreeMap", "lower_bound(55)", uint64_t(567), map.lower_bound(55)->second);
    assertTrue("DTreeMap", "upper_bound(55)", map.upper_bound(55)==map.end());
    map.erase(55);
    assertCLRXException("DTreeMap", "at(55) except", "DTreeMap key not found",
                        [&map](cxuint v) { map.at(v); } , 55);
    const DTreeMap<cxuint, uint64_t>& cmap = map;
    map.find(55);
    map.begin();
    map.cbegin();
    map.end();
    map.cend();
    map.size();
    map.empty();
    map[55] = 1567;
    assertValue("DTreeMap", "find(55)", uint64_t(1567), map.find(55)->second);
    cmap.lower_bound(55);
    cmap.upper_bound(55);
    assertValue("DTreeMap", "cfind(55)", uint64_t(1567), cmap.find(55)->second);
    assertValue("DTreeMap", "clower_bound(55)", uint64_t(1567),
                cmap.lower_bound(55)->second);
    assertTrue("DTreeMap", "cupper_bound(55)", cmap.upper_bound(55)==cmap.end());
    cmap.find(55);
    cmap.begin();
    cmap.cbegin();
    cmap.end();
    cmap.cend();
    cmap.size();
    cmap.empty();
    assertValue("DTreeMap", "c at(55)", uint64_t(1567), map.at(55));
    assertCLRXException("DTreeMap", "c at(55) except", "DTreeMap key not found",
                        [&map](cxuint v) { map.at(v); } , 57);
    //
    map.put(std::make_pair(46, 5123));
    assertValue("DTreeMap", "put(55)", uint64_t(5123), map.find(46)->second);
    map.put(std::make_pair(46, 6123));
    assertValue("DTreeMap", "put(55)2", uint64_t(6123), map.find(46)->second);
    auto it = map.find(46);
    map.replace(it, std::make_pair(47, 4212));
    assertTrue("DTreeMap", "replace(it, 47).old", map.find(46)==map.end());
    assertValue("DTreeMap", "replace(it, {47, 4212})",
                uint64_t(4212), map.find(47)->second);
}

static void testDTreeMapReplace()
{
    DTreeMap<cxuint, uint64_t> map;
    map.insert(std::make_pair(45, uint64_t(276)));
    map.insert(std::make_pair(67, uint64_t(276)));
    map.insert(std::make_pair(87, uint64_t(872)));
    auto it = map.find(67);
    map.replace(it, std::make_pair(57, 87992));
    assertTrue("DTreeMapRep", "replace(it, 57).old", map.find(67)==map.end());
    assertValue("DTreeMapRep", "replace(it, {57, xxx})",
                uint64_t(87992), map.find(57)->second);
    
    map.replace(it, std::make_pair(86, 1212));
    assertTrue("DTreeMapRep", "replace(it, 86).old", map.find(57)==map.end());
    assertValue("DTreeMapRep", "replace(it, {86, xxx})",
                uint64_t(1212), map.find(86)->second);
    
    assertCLRXException("DTreeMapRep", "replace(it, 87)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(87, 8621)); });
    assertCLRXException("DTreeMapRep", "replace(it, 88)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(88, 8621)); });
    assertCLRXException("DTreeMapRep", "replace(it, 45)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(45, 8621)); });
    assertCLRXException("DTreeMapRep", "replace(it, 44)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(45, 8621)); });
    
    it = map.find(45);
    map.replace(it, std::make_pair(11, 87662));
    assertTrue("DTreeMapRep", "replace(it2, 11).old", map.find(45)==map.end());
    assertValue("DTreeMapRep", "replace(it2, {11, xxx})",
                uint64_t(87662), map.find(11)->second);
    
    map.replace(it, std::make_pair(85, 18311));
    assertTrue("DTreeMapRep", "replace(it2, 85).old", map.find(11)==map.end());
    assertValue("DTreeMapRep", "replace(it2, {85, xxx})",
                uint64_t(18311), map.find(85)->second);
    
    assertCLRXException("DTreeMapRep", "replace(it2, 86)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(86, 8621)); });
    assertCLRXException("DTreeMapRep", "replace(it2, 111)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(111, 8621)); });
    
    it = map.find(87);
    map.replace(it, std::make_pair(98, 112233));
    assertTrue("DTreeMapRep", "replace(it3, 98).old", map.find(87)==map.end());
    assertValue("DTreeMapRep", "replace(it3, {98, xxx})",
                uint64_t(112233), map.find(98)->second);
    
    assertCLRXException("DTreeMapRep", "replace(it3, 86)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(86, 8621)); });
    assertCLRXException("DTreeMapRep", "replace(it3, 85)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(85, 8621)); });
    assertCLRXException("DTreeMapRep", "replace(it3, 23)", "Key out of range",
                    [&map, &it]() { map.replace(it, std::make_pair(23, 8621)); });
}

static void testDTreeInsertEraseRandom(cxuint initialElemsNum)
{
    std::minstd_rand0 ranen(753561124);
    DTreeSet<cxuint> set;
    std::set<cxuint> values;
    
    for (cxuint i = 0; i < initialElemsNum; i++)
    {
        cxuint value = ranen();
        values.insert(value);
        set.insert(value);
    }
    if (initialElemsNum != 0)
    {
        verifyDTreeState("DTree", "InErRanInit.test", set);
        // check content
        auto vit = values.begin();
        auto sit = set.begin();
        for (; vit != values.end() && sit != set.end(); ++vit, ++sit)
            assertValue("DTree", "InErRanInit.content", *vit, *sit);
        assertTrue("DTree", "InErRanInitcontentend", vit==values.end() && sit==set.end());
    }
    
    char buf[16];
    for (cxuint i = 0; i < 3000; i++)
    {
        //std::cout << "dt" << i << std::endl;
        snprintf(buf, sizeof buf, "%u", i);
        if ((ranen() & 1) == 0 || values.empty())
        {
            //std::cout << "Insert " << i << std::endl;
            cxuint value = ranen();
            values.insert(value);
            set.insert(value);
        }
        else
        {
            //std::cout << "Erase " << i << std::endl;
            cxuint index = ranen()%set.size();
            auto it = set.begin() + index;
            values.erase(*it);
            set.erase(it);
        }
        
        {
            verifyDTreeState("DTree", std::string("InErRan")+buf+".test", set);
            // check content
            auto vit = values.begin();
            auto sit = set.begin();
            for (; vit != values.end() && sit != set.end(); ++vit, ++sit)
                assertValue("DTree", std::string("InErRan")+buf+".content", *vit, *sit);
            assertTrue("DTree", std::string("InErRan")+buf+"contentend",
                    vit==values.end() && sit==set.end());
        }
    }
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
    
    for (cxuint i = 0; i < sizeof(dtreeNode1SplitTbl) / sizeof(DNode1SplitCase); i++)
        retVal |= callTest(testDTreeNode1Split, i, dtreeNode1SplitTbl[i]);
    
    for (cxuint i = 0; i < sizeof(dNode1ReorgNode0sCaseTbl) /
                        sizeof(DNode1ReorgNode0sCase); i++)
        retVal |= callTest(testDNode1ReorganizeNode0s, i, dNode1ReorgNode0sCaseTbl[i]);
    
    retVal |= callTest(testDTreeNode2);
    
    for (cxuint i = 0; i < sizeof(dtreeNode2SplitTbl) / sizeof(DNode1SplitCase); i++)
        retVal |= callTest(testDTreeNode2Split, i, dtreeNode2SplitTbl[i]);
    
    for (cxuint i = 0; i < sizeof(dNode1ReorgNode1sCaseTbl) /
                        sizeof(DNode1ReorgNode1sCase); i++)
        retVal |= callTest(testDNode1ReorganizeNode1s, i, dNode1ReorgNode1sCaseTbl[i]);
    
    for (cxuint i = 0; i < sizeof(diterBaseCaseTbl) / sizeof(DIterBaseCase); i++)
        retVal |= callTest(testDTreeIterBase, i, diterBaseCaseTbl[i]);
    
    retVal |= callTest(testDTreeIter);
    
    for (cxuint i = 0; i < sizeof(dtreeFindReorgBounds0Tbl) /
                    sizeof(DTreeFindReorgBounds0Case); i++)
        retVal |= callTest(testDTreeFindReorgBounds0, i, dtreeFindReorgBounds0Tbl[i]);
    
    for (cxuint i = 0; i < sizeof(dtreeFindReorgBounds1Tbl) /
                    sizeof(DTreeFindReorgBounds0Case); i++)
        retVal |= callTest(testDTreeFindReorgBounds1, i, dtreeFindReorgBounds1Tbl[i]);
    
    for (cxuint i = 0; i < sizeof(dtreeFindCaseTbl) / sizeof(Array<Array<cxuint> >); i++)
        retVal |= callTest(testDTreeFind, i, dtreeFindCaseTbl[i]);
    
    for (cxuint i = 0; i < sizeof(dtreeInsertCaseTbl) / sizeof(Array<cxuint>); i++)
        retVal |= callTest(testDTreeInsert, i, dtreeInsertCaseTbl[i]);
    retVal |= callTest(testDTreeInsert2);
    retVal |= callTest(testDTreeInsertInitList);
    retVal |= callTest(testDTreeInsertIters);
    
    for (cxuint i = 0; i < sizeof(dtreeInsertBehCaseTbl) / sizeof(DTreeForceBehCase); i++)
        retVal |= callTest(testDTreeInsertBehaviour, i, dtreeInsertBehCaseTbl[i]);
    retVal |= callTest(testDTreeInsertRandom);
    
    retVal |= callTest(testDTreeErase0);
    for (cxuint i = 0; i < sizeof(dtreeEraseBehCaseTbl) / sizeof(DTreeForceBehCase); i++)
        retVal |= callTest(testDTreeEraseBehaviour, i, dtreeEraseBehCaseTbl[i]);
    retVal |= callTest(testDTreeEraseRandom);
    retVal |= callTest(testEraseValue);
    retVal |= callTest(testDTreeConstructors);
    retVal |= callTest(testDTreeEmptySizeClear);
    retVal |= callTest(testDTreeCompare);
    retVal |= callTest(testDTreeBeginEnd);
    retVal |= callTest(testDTreeSwap);
    retVal |= callTest(testDTreeMapUsage);
    retVal |= callTest(testDTreeMapReplace);
    
    retVal |= callTest(testDTreeInsertEraseRandom, 0);
    retVal |= callTest(testDTreeInsertEraseRandom, 100);
    //retVal |= callTest(testDTreeInsertEraseRandom, 500);
    return retVal;
}
