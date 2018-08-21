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
/*! \file DTree.h
 * \brief DTree container (kind of B-Tree)
 */

#ifndef __CLRX_DTREE_H__
#define __CLRX_DTREE_H__

#include <CLRX/Config.h>
#include <algorithm>
#include <iterator>
#include <initializer_list>
#include <climits>
#include <cstddef>
#include <memory>
#include <CLRX/utils/Utilities.h>

namespace CLRX
{

/// Select first element from pair
template<typename T1, typename T2>
struct SelectFirst
{
    const T1& operator()(const std::pair<T1, T2>& v) const
    { return v.first; }
    T1& operator()(std::pair<T1, T2>& v) const
    { return v.first; }
};

/// get element same as input
template<typename T>
struct Identity
{
    const T& operator()(const T& v) const
    { return v; }
    T& operator()(T& v) const
    { return v; }
};

/// main D-Tree container of the unique ordered elements (D-Tree is kind of the B-Tree)
/** The DTree is container very similar to the B+Tree (B-Tree that holds values in leafs).
 * This container holds unique values in sorted order (from smallest to greatest).
 * Arrays of values is held in Node0 nodes which can hold 18-56 elements.
 * Array of values holds empty spaces to improve insertion
 * (shorter moving of the elements).
 * Second type of Node is Node1 that holds Node0's or Node1's. It can hold 2-8 children.
 * DTree have static depth for all leafs
 * (from every leaf is this same step number to root).
 * 
 * About invalidation: every insertion or removal can invalidate pointers,
 * references and iterators to over 200 neighbors of the inserted/removed element.
 * Best way is assumption any insert/erase can invalidate any reference,
 * pointer or iterator.
 */
template<typename K, typename T = K, typename Comp = std::less<K>,
        typename KeyOfVal = Identity<K>, typename AT = T>
class DTree: private Comp, KeyOfVal
{
public:
    /// node type
    enum : cxbyte
    {
        NODE0 = 0,  ///< Node0
        NODE1,  ///< Node1 that holds Node0's
        NODE2,  ///< Node1 that holds Node1's
        NODEV   ///< NodeV - holds short array with elements its space
    };
    
    // number of elements in Node1
    static const cxuint maxNode1Size = 8;
    static const cxuint maxNode1Shift = 3;
    static const cxuint normalNode1Shift = 2;
    static const cxuint maxNode1Depth = (sizeof(size_t)*8)>>1;
    
    static const cxuint maxNode0Capacity = 63;
    static const cxuint normalNode0Capacity = maxNode0Capacity>>1;
    static const cxuint minNode0Capacity = 20;
    static const cxuint freePlacesShift = 1;
    static const cxuint minFreePlacesShift = 3;
    
    static const cxuint maxNode0Size = ((maxNode0Capacity*1000 /
            ((1<<minFreePlacesShift)+1))*(1<<minFreePlacesShift)) / 1000;
    static const cxuint normalNode0Size = ((normalNode0Capacity*1000 /
            ((1<<minFreePlacesShift)+1))*(1<<minFreePlacesShift)) / 1000;
    static const cxuint minNode0Size = maxNode0Size / 3;
    
    // get maximal total size for node in depth level
    static size_t maxTotalSize(cxuint level)
    {
        if (level == 0)
            return maxNode0Size;
        return size_t(maxNode0Size) << (normalNode1Shift * level);
    }
    
    // get normal total size for node in depth level
    static size_t normalTotalSize(cxuint level)
    {
        if (level == 0)
            return normalNode0Size;
        return size_t(normalNode0Size) << (normalNode1Shift * level - 1);
    }
    
    // get minimal total size for node in depth level
    static size_t minTotalSize(cxuint level)
    {
        if (level == 0)
            return minNode0Size;
        return (size_t(maxNode0Size) << (normalNode1Shift * level)) / 3;
    }
    
    // parent pointer part size of array (heap)
    static const int parentEntrySize = sizeof(void*) <= 8 ? 8 : sizeof(void*);
    
    struct NodeBase
    {
        cxbyte type;
        
        NodeBase(cxbyte _type) : type(_type)
        { }
    };
    
    struct Node1;
    static const int parentEntryIndex = -int(parentEntrySize / sizeof(void*));
    
    // main node0 - holds elements
    /* holds slighty greater array of the elements
     * organized in linear ways with empty holes that holds copies of later elements
     * array is ordered
     */
    struct Node0: NodeBase
    {
        cxbyte index;       // index in Node1
        cxbyte size;        // size (number of elements)
        cxbyte capacity;    // capacity of array
        cxbyte firstPos;    // first position with element
        uint64_t bitMask;   // bitmask: 0 - hold element, 1 - free space
        union {
            AT* array;   // array (internal)
            T* arrayOut;  // array out for iterator
        };
        
        Node0(): NodeBase(NODE0), index(255), size(0), capacity(0),
                    firstPos(0), bitMask(0ULL), array(nullptr)
        { }
        
        Node0(const Node0& node): NodeBase(NODE0),
                    index(node.index), size(node.size), capacity(node.capacity),
                    firstPos(node.firstPos), bitMask(node.bitMask), array(nullptr)
        {
            if (node.array != nullptr)
            {
                array = new AT[capacity];
                std::copy(node.array, node.array+capacity, array);
            }
        }
        Node0(Node0&& node) noexcept: NodeBase(NODE0),
                    index(node.index), size(node.size), capacity(node.capacity),
                    firstPos(node.firstPos), bitMask(node.bitMask), array(node.array)
        {
            node.array = nullptr;
        }
        ~Node0()
        {
            delete[] array;
        }
        
        /// copying assignment
        Node0& operator=(const Node0& node)
        {
            NodeBase::type = node.NodeBase::type;
            index = node.index;
            size = node.size;
            capacity = node.capacity;
            firstPos = node.firstPos;
            bitMask = node.bitMask;
            AT* newArray = nullptr;
            if (node.array != nullptr)
            {
                newArray = new AT[capacity];
                std::copy(node.array, node.array+capacity, newArray);
            }
            delete[] array;
            array = newArray;
            return *this;
        }
        
        /// moving assigment
        Node0& operator=(Node0&& node) noexcept
        {
            NodeBase::type = node.NodeBase::type;
            index = node.index;
            size = node.size;
            capacity = node.capacity;
            firstPos = node.firstPos;
            bitMask = node.bitMask;
            delete[] array;
            array = node.array;
            node.array = nullptr;
            return *this;
        }
        
        /// get parent node
        const Node1* parent() const
        { return index!=255U ?
            reinterpret_cast<Node1* const *>(this-index)[parentEntryIndex] : nullptr; }
        
        /// get parent node
        Node1* parent()
        { return index!=255U ?
            reinterpret_cast<Node1**>(this-index)[parentEntryIndex] : nullptr; }
        
        const AT& operator[](cxuint i) const
        { return array[i]; }
        
        AT& operator[](cxuint i)
        { return array[i]; }
        
        /// get lower_bound (first index of element not less than value)
        cxuint lower_boundFree(const K& k, const Comp& comp, const KeyOfVal& kofval) const
        {
            AT kt;
            kofval(kt) = k;
            cxuint index = std::lower_bound(array, array+capacity, kt,
                [&comp, &kofval](const AT& v1, const AT& v2)
                { return comp(kofval(v1), kofval(v2)); }) - array;
            return index;
        }
        
        /// get lower_bound (first index of element not less than value)
        cxuint lower_bound(const K& k, const Comp& comp, const KeyOfVal& kofval) const
        {
            AT kt;
            kofval(kt) = k;
            cxuint index = std::lower_bound(array, array+capacity, kt,
                [&comp, &kofval](const AT& v1, const AT& v2)
                { return comp(kofval(v1), kofval(v2)); }) - array;
            for (; (bitMask & (1ULL<<index)) != 0; index++);
            return index;
        }
        
        /// get upper_bound (first index of element greater than value)
        cxuint upper_bound(const K& k, const Comp& comp, const KeyOfVal& kofval) const
        {
            AT kt;
            kofval(kt) = k;
            cxuint index = std::upper_bound(array, array+capacity, kt,
                [&comp, &kofval](const T& v1, const T& v2)
                { return comp(kofval(v1), kofval(v2)); }) - array;
            for (; (bitMask & (1ULL<<index)) != 0; index++);
            return index;
        }
        
        /// get lower_bound (first index of element not less than value)
        cxuint find(const K& k, const Comp& comp, const KeyOfVal& kofval) const
        {
            cxuint index = lower_bound(k, comp, kofval);
            if (index == capacity ||
                // if not equal
                comp(k, kofval(array[index])) || comp(kofval(array[index]), k))
                return capacity; // not found
            return index;
        }
        
        /// internal routine to organize array with empty holes
        static void organizeArray(AT& toFill,
                cxuint& i, cxuint size, const AT* array, uint64_t inBitMask,
                cxuint& k, cxuint newSize, AT* out, uint64_t& outBitMask,
                cxuint& factor, cxuint finc)
        {
            while ((inBitMask & (1ULL<<i)) != 0)
                i++; // skip free elem
            
            cxuint p0 = 0;
            for (; p0 < size; k++, p0++)
            {
                toFill = out[k] = array[i];
                
                factor += finc;
                if (factor >= newSize)
                {
                    // add additional (empty) element
                    factor -= newSize;
                    k++;
                    out[k] = array[i];
                    outBitMask |= (1ULL<<k);
                }
                
                i++;
                while ((inBitMask & (1ULL<<i)) != 0)
                    i++; // skip free elem
            }
        }
        
        void setFromArray(cxuint size, const AT* input)
        {
            cxuint newCapacity = std::min(cxbyte(size + (size>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            AT* newArray = new AT[newCapacity];
            
            cxuint factor = 0;
            // finc - factor increment for empty holes
            const cxuint finc = newCapacity - size;
            AT toFill = AT();
            cxuint i = 0, k = 0;
            uint64_t newBitMask = 0;
            organizeArray(toFill, i, size, input, 0ULL, k, size, newArray,
                        newBitMask, factor, finc);
            if (k < newCapacity)
            {
                newArray[k] = toFill;
                newBitMask |= (1ULL<<k);
            }
            
            delete[] array;
            array = newArray;
            this->size = size;
            bitMask = newBitMask;
            capacity = newCapacity;
            firstPos = 0;
        }
        
        void allocate(cxuint size)
        {
            capacity = std::min(cxbyte(size + (size>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            AT* newArray = new AT[capacity];
            delete[] array;
            array = newArray;
            firstPos = 0;
            bitMask = 0;
            this->size = 0;
        }
        
        void assignArray(AT& toFill, cxuint inSize, cxuint& index, cxuint& pos,
                const AT* array, uint64_t inBitMask, size_t newSize,
                cxuint& k, cxuint& factor)
        {
            cxuint finc = capacity - newSize;
            cxuint remainingSize = std::min(cxuint(newSize-size), cxuint(inSize-pos));
            organizeArray(toFill, index, remainingSize, array,
                          inBitMask, k, newSize, this->array, bitMask, factor, finc);
            size += remainingSize;
            pos += remainingSize;
        }
        
        /// merge this node with node2
        void merge(const Node0& node2)
        {
            cxuint newSize = size+node2.size;
            cxuint newCapacity = std::min(
                        cxbyte(newSize + (newSize>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            AT* newArray = nullptr;
            if (newCapacity != 0)
                newArray = new AT[newCapacity];
            
            uint64_t newBitMask = 0ULL;
            cxuint factor = 0;
            // finc - factor increment for empty holes
            const cxuint finc = newCapacity - newSize;
            AT toFill = AT();
            cxuint i = 0, j = 0, k = 0;
            
            organizeArray(toFill, i, size, array, bitMask, k, newSize, newArray,
                        newBitMask, factor, finc);
            
            organizeArray(toFill, j, node2.size, node2.array, node2.bitMask,
                        k, newSize, newArray, newBitMask, factor, finc);
            
            // fill a remaining free elements
            if (k < newCapacity)
            {
                newArray[k] = toFill;
                newBitMask |= (1ULL<<k);
            }
            
            delete[] array;
            array = newArray;
            capacity = newCapacity;
            size = newSize;
            bitMask = newBitMask;
            firstPos = 0;
        }
        
        /// split this node and store in this node and node2
        void split(Node0& node2)
        {
            cxuint newSize0 = (size+1)>>1;
            cxuint newSize1 = size-newSize0;
            cxuint newCapacity0 = std::min(
                        cxbyte(newSize0 + (newSize0>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            cxuint newCapacity1 = std::min(
                        cxbyte(newSize1 + (newSize1>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            std::unique_ptr<AT[]> newArray0 = nullptr;
            std::unique_ptr<AT[]> newArray1 = nullptr;
            if (newCapacity0 != 0)
                newArray0.reset(new AT[newCapacity0]);
            if (newCapacity1 != 0)
                newArray1.reset(new AT[newCapacity1]);
            uint64_t newBitMask0 = 0ULL;
            uint64_t newBitMask1 = 0ULL;
            
            AT toFill = AT();
            cxuint i = 0, k = 0;
            cxuint factor = 0;
            // finc - factor increment for empty holes
            cxuint finc = newCapacity0 - newSize0;
            // store first part to newArray0
            organizeArray(toFill, i, newSize0, array, bitMask, k, newSize0,
                        newArray0.get(), newBitMask0, factor, finc);
            
            // fill a remaining free elements
            if (k < newCapacity0)
            {
                newArray0[k] = toFill;
                newBitMask0 |= (1ULL<<k);
            }
            
            toFill = AT();
            k = 0;
            factor = 0;
            // finc - factor increment for empty holes
            finc = newCapacity1 - newSize1;
            // store first part to newArray1
            organizeArray(toFill, i, newSize1, array, bitMask,
                        k, newSize1, newArray1.get(), newBitMask1, factor, finc);
            
            // fill a remaining free elements
            if (k < newCapacity1)
            {
                newArray1[k] = toFill;
                newBitMask1 |= (1ULL<<k);
            }
            
            delete[] array;
            // store into this node (array0)
            array = newArray0.release();
            capacity = newCapacity0;
            size = newSize0;
            bitMask = newBitMask0;
            firstPos = 0;
            delete[] node2.array;
            // store into node2 (array1)
            node2.array = newArray1.release();
            node2.capacity = newCapacity1;
            node2.size = newSize1;
            node2.bitMask = newBitMask1;
            node2.firstPos = 0;
        }
        
        /// simple resize
        void resize(cxint extraSize)
        {
            // reorganize array
            cxuint newCapacity = std::min(
                        cxbyte(size+extraSize + ((size+extraSize)>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            AT* newArray = nullptr;
            if (newCapacity != 0)
                newArray = new AT[newCapacity];
            
            uint64_t newBitMask = 0ULL;
            cxuint factor = 0;
            const cxuint finc = newCapacity - size;
            
            AT toFill = AT();
            cxuint i = 0, j = 0;
            
            organizeArray(toFill, i, size, array, bitMask, j, size, newArray,
                        newBitMask, factor, finc);
            // fill a remaining free elements
            if (j < newCapacity)
            {
                newArray[j] = toFill;
                newBitMask |= (1ULL<<j);
            }
            
            delete[] array;
            array = newArray;
            capacity = newCapacity;
            bitMask = newBitMask;
            firstPos = 0;
        }
        
        /// insert element
        std::pair<cxuint, bool> insert(const T& v, const Comp& comp,
                        const KeyOfVal& kofval)
        {
            cxuint idx = 255;
            idx = lower_boundFree(kofval(v), comp, kofval);
            
            if (idx < capacity && (bitMask & (1ULL<<idx))==0 &&
                        !comp(kofval(v), kofval(array[idx])))
                // is equal, then skip insertion
                return std::make_pair(idx, false);
            
            cxuint minFreePlaces = ((size+1)>>minFreePlacesShift);
            if ((size+1) + minFreePlaces > capacity)
            {
                resize(1);
                idx = lower_boundFree(kofval(v), comp, kofval);
            }
            
            if ((bitMask & (1ULL<<idx)) == 0)
            {
                // if this is not free element
                const uint64_t leftMask = bitMask & ((1ULL<<idx)-1U);
                const uint64_t rightMask = bitMask & ~((2ULL<<idx)-1U);
                
                // leftLen - number of elemnts in left side to first empty hole
                // rightLen - number of elemnts in right side to first empty hole
                cxuint leftLen = 255, rightLen = 255;
                if (leftMask != 0)
                    leftLen = idx-(63-CLZ64(leftMask));
                if (rightMask != 0)
                    rightLen = CTZ64(rightMask)-idx;
                
                if (leftLen >= rightLen)
                {
                    // move right side (shorter)
                    cxuint k = idx + rightLen;
                    bitMask &= ~(1ULL<<k);
                    for (; k > idx; k--)
                        array[k] = array[k-1];
                }
                else
                {
                    // move left side (shorter)
                    cxuint k = idx - leftLen;
                    bitMask &= ~(1ULL<<k);
                    for (; k < idx-1; k++)
                        array[k] = array[k+1];
                    idx--; // before element
                    
                    firstPos = 0;
                    while ((bitMask & (1U<<firstPos)) != 0)
                        firstPos++; // skip free places
                }
                array[idx] = v;
            }
            else
            {
                // if this is free place
                array[idx] = v;
                bitMask &= ~(1ULL<<idx);
            }
            size++;
            firstPos = std::min(cxuint(firstPos), idx);
            return std::make_pair(idx, true);
        }
        
        /// erase element in index
        bool erase(cxuint index)
        {
            if ((bitMask & (1ULL<<index)) != 0)
                return false;
            bitMask |= (1ULL<<index);
            size--;
            
            cxuint maxFreePlaces = ((size+1)>>freePlacesShift);
            if (size + maxFreePlaces < capacity)
                resize(0);
            else if (index == firstPos)
                while ((bitMask & (1U<<firstPos)) != 0)
                    firstPos++; // skip free places
            return true;
        }
        
        /// erase element of value v
        bool erase(const K& k, const Comp& comp, const KeyOfVal& kofval)
        {
            cxuint index = lower_bound(k, comp, kofval);
            if (index >= capacity || comp(k, kofval(array[index])))
                return false;  // if not found
            return erase(index);
        }
    };
    
    /// Node1 - main node that holds Node0's or Node1's
    struct Node1 : NodeBase
    {
        cxbyte index;       ///< index in array
        cxbyte size;    ///< number of the nodes
        cxbyte capacity;    ///< capacity
        size_t totalSize;   ///< total size in this node (count recursively)
        K first;    /// first element in this node (recursively)
        /// array are allocated by bytes, preceding part is heap that hold parent node
        union {
            Node0* array;   /// array hold Node0's
            Node1* array1;  /// array holds Node1's
        };
        
        Node1(): NodeBase(NODE1), index(255), size(0), capacity(0), totalSize(0),
                    first(), array(nullptr)
        { }
        
        /// helper for freeing array
        void freeArray()
        {
            if (array != nullptr)
            {
                if (NodeBase::type == NODE1)
                {
                    for (cxuint i = 0; i < capacity; i++)
                        array[i].~Node0();
                    delete[] (reinterpret_cast<cxbyte*>(array) - parentEntrySize);
                }
                else
                {
                    for (cxuint i = 0; i < capacity; i++)
                        array1[i].~Node1();
                    delete[] (reinterpret_cast<cxbyte*>(array1) - parentEntrySize);
                }
                array1 = nullptr;
            }
        }
        
        // copy array helper - copy array from Node1 to this node
        void copyArray(const Node1& node)
        {
            if (NodeBase::type == NODE1)
            {
                // if Node1 holds Node0's
                if (node.array != nullptr)
                {
                    cxbyte* arrayData = new cxbyte[parentEntrySize +
                                capacity*sizeof(Node0)];
                    freeArray();
                    array = reinterpret_cast<Node0*>(arrayData + parentEntrySize);
                    /// set parent for this array
                    *reinterpret_cast<Node1**>(arrayData) = this;
                    for (cxuint i = 0; i < capacity; i++)
                        new (array+i)Node0();
                    std::copy(node.array, node.array+size, array);
                }
            }
            else
            {
                // if Node1 holds Node1's
                if (node.array1 != nullptr)
                {
                    cxbyte* arrayData = new cxbyte[parentEntrySize +
                                capacity*sizeof(Node1)];
                    freeArray();
                    array1 = reinterpret_cast<Node1*>(arrayData + parentEntrySize);
                    /// set parent for this array
                    *reinterpret_cast<Node1**>(arrayData) = this;
                    for (cxuint i = 0; i < capacity; i++)
                        new (array1+i)Node1();
                    std::copy(node.array1, node.array1+size, array1);
                }
            }
        }
        
        Node1(const Node1& node): NodeBase(node.type), index(node.index),
                    size(node.size), capacity(node.capacity),
                    totalSize(node.totalSize), first(node.first), array(nullptr)
        {
            copyArray(node);
        }
        
        Node1(Node1&& node) noexcept: NodeBase(node.type), index(node.index),
                    size(node.size), capacity(node.capacity),
                    totalSize(node.totalSize), first(node.first), array(node.array)
        {
            if (array != nullptr)
                reinterpret_cast<Node1**>(array)[parentEntryIndex] = this;
            node.array = nullptr;
        }
        
        /// create from two Node0's
        Node1(Node0&& n0, Node0&& n1, const KeyOfVal& kofval)
                : NodeBase(NODE1), index(255), size(2), capacity(2),
                totalSize(n0.size+n1.size), first(kofval(n0.array[n0.firstPos])),
                array(nullptr)
        {
            cxbyte* arrayData = new cxbyte[parentEntrySize + capacity*sizeof(Node0)];
            array = reinterpret_cast<Node0*>(arrayData + parentEntrySize);
            new (array+0)Node0();
            new (array+1)Node0();
            *reinterpret_cast<Node1**>(arrayData) = this;
            array[0] = std::move(n0);
            array[1] = std::move(n1);
            array[0].index = 0;
            array[1].index = 1;
        }
        /// create from two Node1's
        Node1(Node1&& n0, Node1&& n1) : NodeBase(NODE2), index(255),
                size(2), capacity(2), totalSize(n0.totalSize+n1.totalSize),
                first(n0.first), array(nullptr)
        {
            cxbyte* arrayData = new cxbyte[parentEntrySize + capacity*sizeof(Node1)];
            array1 = reinterpret_cast<Node1*>(arrayData + parentEntrySize);
            new (array1+0)Node1();
            new (array1+1)Node1();
            *reinterpret_cast<Node1**>(arrayData) = this;
            array1[0] = std::move(n0);
            array1[1] = std::move(n1);
            array1[0].index = 0;
            array1[1].index = 1;
        }
        
        ~Node1()
        {
            freeArray();
        }
        
        Node1& operator=(const Node1& node)
        {
            NodeBase::type = node.NodeBase::type;
            size = node.size;
            index = node.index;
            capacity = node.capacity;
            totalSize = node.totalSize;
            copyArray(node);
            first = node.first;
            return *this;
        }
        
        Node1& operator=(Node1&& node) noexcept
        {
            freeArray();
            NodeBase::type = node.NodeBase::type;
            size = node.size;
            index = node.index;
            capacity = node.capacity;
            totalSize = node.totalSize;
            array = node.array;
            if (array != nullptr)
                reinterpret_cast<Node1**>(array)[parentEntryIndex] = this;
            first = node.first;
            node.array = nullptr;
            return *this;
        }
        
        Node0* getFirstNode0()
        {
            Node1* cur = this;
            while (cur->NodeBase::type == NODE2)
                cur = cur->array1;
            return cur->array;
        }
        
        const Node0* getFirstNode0() const
        {
            const Node1* cur = this;
            while (cur->NodeBase::type == NODE2)
                cur = cur->array1;
            return cur->array;
        }
        
        Node0* getLastNode0()
        {
            Node1* cur = this;
            while (cur->NodeBase::type == NODE2)
                cur = cur->array1 + cur->size - 1;
            return cur->array + cur->size - 1;
        }
        
        const Node0* getLastNode0() const
        {
            const Node1* cur = this;
            while (cur->NodeBase::type == NODE2)
                cur = cur->array1 + cur->size - 1;
            return cur->array + cur->size - 1;
        }
        
        /// get parent node
        const Node1* parent() const
        { return index!=255U ?
            reinterpret_cast<Node1* const *>(this-index)[parentEntryIndex] : nullptr; }
        
        /// get parent node
        Node1* parent()
        { return index!=255U ?
            reinterpret_cast<Node1**>(this-index)[parentEntryIndex] : nullptr; }
        
        void allocate0(cxuint newCapacity)
        {
            cxbyte* newData = new cxbyte[newCapacity*sizeof(Node0) + parentEntrySize];
            // set parent node
            *reinterpret_cast<Node1**>(newData) = this;
            Node0* newArray = reinterpret_cast<Node0*>(newData + parentEntrySize);
            /// construct Node1's
            for (cxuint i = 0; i < newCapacity; i++)
                new (newArray+i)Node0();
            if (array != nullptr)
            {
                for (cxuint i = 0; i < size; i++)
                    array[i].~Node0();
                delete[] (reinterpret_cast<cxbyte*>(array) - parentEntrySize);
            }
            
            totalSize = 0;
            array = newArray;
            capacity = newCapacity;
            size = 0;
            first = K();
        }
        
        void allocate1(cxuint newCapacity)
        {
            cxbyte* newData = new cxbyte[newCapacity*sizeof(Node1) + parentEntrySize];
            // set parent node
            *reinterpret_cast<Node1**>(newData) = this;
            Node1* newArray = reinterpret_cast<Node1*>(newData + parentEntrySize);
            /// construct Node1's
            for (cxuint i = 0; i < newCapacity; i++)
                new (newArray+i)Node1();
            if (array != nullptr)
            {
                for (cxuint i = 0; i < size; i++)
                    array1[i].~Node1();
                delete[] (reinterpret_cast<cxbyte*>(array1) - parentEntrySize);
            }
            
            totalSize = 0;
            array1 = newArray;
            capacity = newCapacity;
            size = 0;
            first = K();
        }
        
        /// reserve0 elements in Node0's array
        void reserve0(cxuint newCapacity)
        {
            cxbyte* newData = new cxbyte[newCapacity*sizeof(Node0) + parentEntrySize];
            // set parent node
            *reinterpret_cast<Node1**>(newData) = this;
            Node0* newArray = reinterpret_cast<Node0*>(newData + parentEntrySize);
            /// construct Node1's
            for (cxuint i = 0; i < newCapacity; i++)
                new (newArray+i)Node0();
            cxuint newSize = std::min(cxuint(size), newCapacity);
            if (array != nullptr)
            {
                for (cxuint i = newSize; i < size; i++)
                {
                    totalSize -= array[i].size;
                    array[i].~Node0();
                }
                std::move(array, array + newSize, newArray);
                delete[] (reinterpret_cast<cxbyte*>(array) - parentEntrySize);
            }
            
            array = newArray;
            capacity = newCapacity;
            size = newSize;
            if (size == 0)
                first = K();
        }
        
        /// reserve1 elements in Node0's array
        void reserve1(cxuint newCapacity)
        {
            cxbyte* newData = new cxbyte[newCapacity*sizeof(Node1) + parentEntrySize];
            // set parent node
            *reinterpret_cast<Node1**>(newData) = this;
            Node1* newArray = reinterpret_cast<Node1*>(newData + parentEntrySize);
            /// construct Node1's
            for (cxuint i = 0; i < newCapacity; i++)
                new (newArray+i)Node1();
            cxuint newSize = std::min(cxuint(size), newCapacity);
            if (array != nullptr)
            {
                for (cxuint i = newSize; i < size; i++)
                {
                    totalSize -= array1[i].totalSize;
                    array1[i].~Node1();
                }
                std::move(array1, array1 + newSize, newArray);
                delete[] (reinterpret_cast<cxbyte*>(array1) - parentEntrySize);
            }
            
            array1 = newArray;
            capacity = newCapacity;
            size = newSize;
            if (size == 0)
                first = K();
        }
        
        
        /// find node that hold first element not less than value
        cxuint lowerBoundN(const K& v, const Comp& comp, const KeyOfVal& kofval) const
        {
            if (size == 0)
                return 0;
            if (NodeBase::type == NODE1)
            {
                cxuint l = 0, r = size;
                cxuint m;
                while (l+1<r)
                {
                    m = (l+r)>>1;
                    if (comp(kofval(array[m].array[array[m].firstPos]), v))
                        l = m;
                    else
                        //  !(array[m] < v) -> v <= array[m]
                        r = m;
                }
                if (comp(kofval(array[l].array[array[l].firstPos]), v))
                    l++;
                return l;
            }
            else
            {
                if (size == 1)
                    //  !(array[m] < v) -> v <= array[0]
                    return !comp(array1[0].first, v) ? 0 : 1;
                cxuint l = 0, r = size;
                cxuint m;
                while (l+1 < r)
                {
                    m = (l+r)>>1;
                    if (comp(array1[m].first, v))
                        l = m;
                    else
                        //  !(array[m] < v) -> v <= array[m]
                        r = m;
                }
                if (comp(array1[l].first, v))
                    l++;
                return l;
            }
        }
        
        /// find node that hold first element greater than value
        cxuint upperBoundN(const K& v, const Comp& comp, const KeyOfVal& kofval) const
        {
            if (size == 0)
                return 0;
            if (NodeBase::type == NODE1)
            {
                cxuint l = 0, r = size;
                cxuint m;
                while (l+1 < r)
                {
                    m = (l+r)>>1;
                    if (comp(v, kofval(array[m].array[array[m].firstPos])))
                        //  !(array[m] < v) -> v <= array[m]
                        r = m;
                    else
                        l = m;
                }
                if (!comp(v, kofval(array[l].array[array[l].firstPos])))
                    l++;
                return l;
            }
            else
            {
                cxuint l = 0, r = size;
                cxuint m;
                while (l+1 < r)
                {
                    m = (l+r)>>1;
                    if (comp(v, array1[m].first))
                        //  !(array[m] < v) -> v <= array[m]
                        r = m;
                    else
                        l = m;
                }
                if (!comp(v, array1[l].first))
                    l++;
                return l;
            }
        }
        
        /// insert Node0 - (move to this node)
        void insertNode0(Node0&& node, cxuint index, const KeyOfVal& kofval,
                         bool updateTotSize = true)
        {
            NodeBase::type = NODE1;
            if (size+1 > capacity)
                reserve0(std::min(std::max(capacity + (capacity>>1), size+1),
                                  int(maxNode1Size)));
            // move to next position
            for (size_t i = size; i > index; i--)
            {
                array[i] = std::move(array[i-1]);
                array[i].index = i;
            }
            
            array[index] = std::move(node);
            array[index].index = index;
            if (index == 0 && array[0].array!=nullptr)
                first = kofval(array[0].array[array[0].firstPos]);
            size++;
            if (updateTotSize)
                totalSize += node.size;
        }
        
        /// insert Node1 - (move to this node)
        void insertNode1(Node1&& node, cxuint index, bool updateTotSize = true)
        {
            NodeBase::type = NODE2;
            if (size+1 > capacity)
                reserve1(std::min(std::max(capacity + (capacity>>1), size+1),
                                  int(maxNode1Size)));
            // move to next position
            for (size_t i = size; i > index; i--)
            {
                array1[i] = std::move(array1[i-1]);
                array1[i].index = i;
            }
            
            array1[index] = std::move(node);
            array1[index].index = index;
            if (index == 0)
                first = array1[0].first;
            size++;
            if (updateTotSize)
                totalSize += node.totalSize;
        }
        
        /// remove node0 with index from this node
        void eraseNode0(cxuint index, const KeyOfVal& kofval, bool updateTotSize = true)
        {
            if (updateTotSize)
                totalSize -= array[index].size;
            std::move(array+index+1, array+size, array+index);
            if (size == index+1)
            {
                array[size-1].~Node0();
                array[size-1].array = nullptr;
            }
            for (cxuint i = index; i < size-1U; i++)
                array[i].index = i;
            if (size > 1 && index == 0)
                first = kofval(array[0].array[array[0].firstPos]);
            size--;
            if (size + (size>>1) < capacity)
                reserve0(size+1);
            if (size == 0)
                first = K();
        }
        
        /// remove node1 with index from this node
        void eraseNode1(cxuint index, bool updateTotSize = true)
        {
            if (updateTotSize)
                totalSize -= array1[index].totalSize;
            std::move(array1+index+1, array1+size, array1+index);
            if (size == index+1)
            {
                array1[size-1].~Node1();
                array1[size-1].array = nullptr;
            }
            for (cxuint i = index; i < size-1U; i++)
                array1[i].index = i;
            if (size > 1 && index == 0)
                first = array1[0].first;
            size--;
            if (size + (size>>1) < capacity)
                reserve1(size+1);
            if (size == 0)
                first = K();
        }
        
        void reorganizeNode0s(cxuint start, cxuint end, bool removeOneNode0 = false)
        {
            Node0 temps[maxNode1Size];
            cxuint nodesSize = 0;
            for (cxuint i = start; i < end; i++)
                nodesSize += array[i].size;
            
            cxuint newNodeSize = nodesSize / (end-start - removeOneNode0);
            cxuint withExtraElem = nodesSize - newNodeSize*(end-start - removeOneNode0);
            cxuint ni = 0; // new item start
            AT toFill = AT();
            cxuint inIndex, inPos;
            cxuint k = 0;
            cxuint factor = 0;
            cxuint newSize = newNodeSize + (ni < withExtraElem);
            temps[ni].allocate(newSize);
            temps[ni].index = start + ni;
            // main loop to fill up new node0's
            for (cxuint i = start; i < end; i++)
            {
                inPos = inIndex = 0;
                while(inIndex < array[i].capacity)
                {
                    temps[ni].assignArray(toFill, array[i].size, inIndex, inPos,
                            array[i].array, array[i].bitMask, newSize, k, factor);
                    if (inPos < array[i].size)
                    {
                        // fill up end of new node0
                        if (k < temps[ni].capacity)
                        {
                            temps[ni].array[k] = toFill;
                            temps[ni].bitMask |= (1ULL<<k);
                        }
                        
                        factor = k = 0;
                        ni++;
                        newSize = newNodeSize + (ni < withExtraElem);
                        if (ni < end-start)
                        {
                            temps[ni].allocate(newSize);
                            temps[ni].index = start + ni;
                        }
                    }
                }
            }
            // final move to this array
            std::move(temps, temps + end-start - removeOneNode0, array+start);
            if (removeOneNode0)
            {
                std::move(array + end, array + size, array + end - 1);
                for (cxuint i = end-1; i < cxuint(size - 1); i++)
                    array[i].index = i;
                size--;
            }
        }
        
        void merge(Node1&& n2)
        {
            cxuint oldSize = size;
            if ((n2.size != 0 && n2.NodeBase::type == NODE1) ||
                (size != 0 && NodeBase::type == NODE1))
            {
                reserve0(std::max(cxuint(maxNode1Size), cxuint(capacity + n2.capacity)));
                std::move(n2.array, n2.array + n2.size, array + size);
                for (cxuint i = size; i < size+n2.size; i++)
                    array[i].index = i;
            }
            else
            {
                NodeBase::type = NODE2;
                reserve1(std::max(cxuint(maxNode1Size), cxuint(capacity + n2.capacity)));
                std::move(n2.array1, n2.array1 + n2.size, array1 + size);
                for (cxuint i = size; i < size+n2.size; i++)
                    array1[i].index = i;
            }
            totalSize += n2.totalSize;
            size += n2.size;
            if (oldSize == 0)
                first = n2.first;
        }
        
        void splitNode(Node1& n2, const KeyOfVal& kofval)
        {
            if (NodeBase::type == NODE1)
            {
                cxuint halfPos = 0;
                size_t halfTotSize = 0;
                for (; halfPos < size && halfTotSize < (totalSize>>1); halfPos++)
                    halfTotSize += array[halfPos].size;
                if ((halfTotSize - (totalSize>>1)) > size_t(array[halfPos-1].size>>1))
                {
                    halfPos--; // 
                    halfTotSize -= array[halfPos].size;
                }
                
                cxuint newSize2 = size - halfPos;
                size_t secHalfTotSize = totalSize - halfTotSize;
                n2.allocate0(std::min(newSize2, cxuint(maxNode1Size)));
                std::move(array + halfPos, array + size, n2.array);
                n2.totalSize = secHalfTotSize;
                n2.first = kofval(n2.array[0].array[n2.array[0].firstPos]);
                for (cxuint i = 0; i < newSize2; i++)
                    n2.array[i].index = i;
                reserve0(halfPos);
                n2.size = newSize2;
            }
            else
            {
                cxuint halfPos = 0;
                size_t halfTotSize = 0;
                for (; halfPos < size && halfTotSize < (totalSize>>1); halfPos++)
                    halfTotSize += array1[halfPos].totalSize;
                if ((halfTotSize - (totalSize>>1)) > (array1[halfPos-1].totalSize>>1))
                {
                    halfPos--; // 
                    halfTotSize -= array1[halfPos].totalSize;
                }
                
                n2.NodeBase::type = NODE2;
                cxuint newSize2 = size - halfPos;
                size_t secHalfTotSize = totalSize - halfTotSize;
                n2.allocate1(std::min(newSize2, cxuint(maxNode1Size)));
                std::move(array1 + halfPos, array1 + size, n2.array1);
                n2.totalSize = secHalfTotSize;
                n2.first = n2.array1[0].first;
                for (cxuint i = 0; i < newSize2; i++)
                    n2.array1[i].index = i;
                reserve1(halfPos);
                n2.size = newSize2;
            }
        }
        
        void reorganizeNode1s(cxuint start, cxuint end, const KeyOfVal& kofval,
                        bool removeOneNode1 = false)
        {
            Node1 temps[maxNode1Size];
            cxuint node0sNum = 0;
            size_t nodesTotSize = 0;
            for (cxuint i = start; i < end; i++)
            {
                node0sNum += array1[i].size;
                nodesTotSize += array1[i].totalSize;
            }
            
            const cxuint end2 = end - removeOneNode1;
            cxuint node0Count = 0;
            cxuint j = start; // input node index
            cxuint k = 0; // input child node index
            cxuint remaining = end2-start-1;
            for (cxuint i = 0; i < end2-start; i++, remaining--)
            {
                temps[i].index = start+i;
                cxuint newNodeSize = nodesTotSize / (end2-start-i);
                for (; j < end; j++)
                {
                    const Node1& child = array1[j];
                    if (child.type == NODE2)
                        for (; k < child.size && temps[i].size < maxNode1Size &&
                            (temps[i].size < 2 ||
                            (node0sNum-node0Count > remaining*maxNode1Size) ||
                            // if no node0s in parent node
                            temps[i].totalSize+(child.array1[k].totalSize>>1) <
                                        newNodeSize); k++, node0Count++)
                        {
                            if (node0sNum-node0Count <= (remaining<<1))
                                // prevent too small node0s number for rest node1s
                                break;
                            temps[i].insertNode1(std::move(child.array1[k]),
                                                temps[i].size);
                        }
                    else
                        for (; k < child.size && temps[i].size < maxNode1Size &&
                            (temps[i].size < 2 ||
                            (node0sNum-node0Count > remaining*maxNode1Size) ||
                            // if no node0s in parent node
                            temps[i].totalSize+(child.array[k].size>>1) < newNodeSize);
                                    k++, node0Count++)
                        {
                            if (node0sNum-node0Count <= (remaining<<1))
                                // prevent too small node0s number for rest node1s and
                                break;
                            temps[i].insertNode0(std::move(child.array[k]),
                                                temps[i].size, kofval);
                        }
                    
                    if (k >= child.size)
                        k = 0; // if end of input node
                    else
                        break;
                }
                
                nodesTotSize -= temps[i].totalSize;
            }
            
            // final move to this array
            std::move(temps, temps + end-start - removeOneNode1, array1+start);
            if (removeOneNode1)
            {
                std::move(array1 + end, array1 + size, array1 + end - 1);
                for (cxuint i = end-1; i < cxuint(size - 1); i++)
                    array1[i].index = i;
                size--;
            }
        }
    };
    
    static const size_t MaxNode01Size = sizeof(Node0) < sizeof(Node1) ?
                sizeof(Node1) : sizeof(Node0);
    
    static const size_t NodeVElemsNum = (MaxNode01Size -
            // calculate space took by NodeV fields
            (2 < alignof(T) ? alignof(T) : 2)) / sizeof(T);
    
    struct NodeV: NodeBase
    {
        cxbyte size;
        union {
            AT array[NodeVElemsNum];
            T arrayOut[NodeVElemsNum];
        };
        
        NodeV() : NodeBase(NODEV), size(0)
        { }
        
        NodeV(const NodeV& nv) : NodeBase(nv.NodeBase::type), size(nv.size)
        { std::copy(nv.array, nv.array + nv.size, array); }
        NodeV(NodeV&& nv) :  NodeBase(nv.NodeBase::type), size(nv.size)
        { std::copy(nv.array, nv.array + nv.size, array); }
        
        NodeV& operator=(const NodeV& nv)
        {
            NodeBase::type = nv.NodeBase::type;
            size = nv.size;
            std::copy(nv.array, nv.array + nv.size, array);
            return *this;
        }
        NodeV& operator=(NodeV&& nv)
        {
            NodeBase::type = nv.NodeBase::type;
            size = nv.size;
            std::copy(nv.array, nv.array + nv.size, array);
            return *this;
        }
        
        void assignNode0(const Node0& n0)
        {
            size = n0.size;
            cxuint j = 0;
            for (cxuint i = 0; j < size; i++)
                if ((n0.bitMask & (1ULL<<i)) == 0)
                    array[j++] = n0.array[i];
        }
        
        bool isFull() const
        { return size >= NodeVElemsNum; }
        
        cxuint lower_bound(const K& key, const Comp& comp, const KeyOfVal& kofval) const
        {
            cxuint index = 0;
            for (; index < size && comp(kofval(array[index]), key); index++);
            return index;
        }
        
        cxuint upper_bound(const K& key, const Comp& comp, const KeyOfVal& kofval) const
        {
            cxuint index = 0;
            for (; index < size && !comp(key, kofval(array[index])); index++);
            return index;
        }
        
        cxuint find(const K& key, const Comp& comp, const KeyOfVal& kofval) const
        {
            cxuint index = 0;
            for (; index < size && comp(kofval(array[index]), key); index++);
            if (index < size && !comp(key, kofval(array[index])))
                // if elem already found
                return index;
            return size;
        }
        
        std::pair<cxuint, bool> insert(const T& v,
                    const Comp& comp, const KeyOfVal& kofval)
        {
            const K key = kofval(v);
            cxuint index = lower_bound(key, comp, kofval);
            if (index < size && !comp(key, kofval(array[index])))
                // if elem already found
                return std::make_pair(index, false);
            
            std::copy_backward(array + index, array + size, array + size + 1);
            array[index] = v;
            size++;
            return std::make_pair(index, true);
        }
        
        void erase(cxuint index)
        {
            std::copy(array + index + 1, array + size, array + index);
            size--;
        }
        
        const AT& operator[](cxuint i) const
        { return array[i]; }
        
        AT& operator[](cxuint i)
        { return array[i]; }
    };
    
public:
    /// main iterator class
    struct IterBase {
        typedef T value_type;
        
        union {
            Node0* n0;    //< node
            const Node0* cn0;    //< node
            NodeV* nv;
            const NodeV* cnv;
        };
        cxuint index;   ///< index in array
        
        IterBase() : cn0(nullptr), index(0)
        { }
        IterBase(const Node0* _n0, cxuint _index) : cn0(_n0), index(_index)
        { }
        IterBase(Node0* _n0, cxuint _index) : n0(_n0), index(_index)
        { }
        
        IterBase& operator=(const IterBase& it)
        {
            n0 = it.n0;
            index = it.index;
            return *this;
        }
        
        void toNode0()
        {
            if (n0->type != NODE0)
            {
                // from root to last Node0
                n0 = reinterpret_cast<Node1*>(n0)->getLastNode0();
                index = n0->capacity;
            }
        }
        
        /// go to inc next elements
        void next(size_t inc)
        {
            if (n0->type == NODEV)
            {
                index += inc;
                return;
            }
            toNode0();
            
            // first skip in elements this node0
            while (index < n0->capacity && inc != 0)
            {
                if ((n0->bitMask & (1ULL<<index)) == 0)
                    inc--;
                index++;
            }
            while (index < n0->capacity && (n0->bitMask & (1ULL<<index)) != 0)
                index++;
            
            bool end = false;
            // we need to go to next node0
            if (index >= n0->capacity)
            {
                const Node1* n[maxNode1Depth];
                //Node1* n1 = n0->parent();
                n[1] = n0->parent();
                if (n[1] != nullptr)
                {
                    n0++;
                    // skipping node0's
                    for (; n0 < n[1]->array+n[1]->size; n0++)
                        if (n0->size <= inc)
                            inc -= n0->size;
                        else
                            break;
                    
                    if (n0 - n[1]->array >= n[1]->size)
                    {
                        cxuint i = 1;
                        for (; i < 20 ; i++)
                        {
                            n[i+1] = n[i]->parent();
                            if (n[i+1] == nullptr)
                            {
                                cn0 = reinterpret_cast<const Node0*>(n[i]);
                                index = n[i]->size;
                                end = true;
                                break;
                            }
                            n[i]++;
                            /// skipping node1's in deeper level
                            for (; n[i] < n[i+1]->array1+n[i+1]->size; n[i]++)
                                if (n[i]->totalSize <= inc)
                                    inc -= n[i]->totalSize;
                                else
                                    break;
                            
                            // if it is this level
                            if (n[i] - n[i+1]->array1 < n[i+1]->size)
                                break;
                        }
                        if (!end)
                        {
                            for (; i > 1; i--)
                                if (n[i+1] != nullptr)
                                {
                                    // set this node1 for this level
                                    n[i-1] = n[i]->array1;
                                    /// and skip further node1's in shallow level
                                    for (; n[i-1] < n[i]->array1+n[i]->size; n[i-1]++)
                                        if (n[i-1]->totalSize <= inc)
                                            inc -= n[i-1]->totalSize;
                                        else
                                            break;
                                }
                            if (n[2] != nullptr)
                            {
                                n0 = n[1]->array;
                                /// skip further node0's for shallowest level
                                for (; n0 < n[1]->array+n[1]->size; n0++)
                                    if (n0->size <= inc)
                                        inc -= n0->size;
                                    else
                                        break;
                            }
                        }
                        else
                            // set end
                            index = n[i]->size;
                    }
                    if (!end)
                        index = n0->firstPos;
                }
                else
                    end = true;
            }
            
            if (!end)
            {
                // skip last elements in count
                while (index < n0->capacity && inc != 0)
                {
                    if ((n0->bitMask & (1ULL<<index)) == 0)
                        inc--; // if it not free space
                    index++;
                }
                // and skip empty space
                while (index < n0->capacity && (n0->bitMask & (1ULL<<index)) != 0)
                    index++;
            }
        }
        
        void toNextNode0()
        {
            bool end = false;
            if (index >= n0->capacity)
            {
                const Node1* n[maxNode1Depth];
                //Node1* n1 = n0->parent();
                n[1] = n0->parent();
                if (n[1] != nullptr)
                {
                    n0++;
                    if (n0 - n[1]->array >= n[1]->size)
                    {
                        // deeper level is needed to be visited
                        cxuint i = 1;
                        for (; i < 20 ; i++)
                        {
                            n[i+1] = n[i]->parent();
                            if (n[i+1] == nullptr)
                            {
                                // if end of tree
                                cn0 = reinterpret_cast<const Node0*>(n[i]);
                                index = n[i]->size;
                                end = true;
                                break;
                            }
                            n[i]++;
                            if (n[i] - n[i+1]->array1 < n[i+1]->size)
                                // if it is this level
                                break;
                        }
                        if (!end)
                        {
                            for (; i > 1; i--)
                                if (n[i+1] != nullptr)
                                    // set node for shallower level
                                    n[i-1] = n[i]->array1;
                            if (n[2] != nullptr)
                                /// set node0 for shallowest level
                                n0 = n[1]->array;
                        }
                        else
                            // set end
                            index = n[i]->size;
                    }
                    if (!end)
                        // do not set index if end of tree
                        index = n0->firstPos;
                }
                else
                    end = true;
            }
        }
        
        /// go to next element
        void next()
        {
            if (n0->type == NODEV)
            {
                index++;
                return;
            }
            
            toNode0();
            
            // skip empty space
            index++;
            while (index < n0->capacity && (n0->bitMask & (1ULL<<index)) != 0)
                index++;
            
            toNextNode0();
        }
        
        /// go to inc previous element
        void prev(size_t inc)
        {
            if (n0->type == NODEV)
            {
                index -= inc;
                return;
            }
            toNode0();
            
            while (index != UINT_MAX && inc != 0)
            {
                if (index==n0->capacity || (n0->bitMask & (1ULL<<index)) == 0)
                    inc--;
                index--;
            }
            while (index != UINT_MAX &&
                (index != n0->capacity && (n0->bitMask & (1ULL<<index)) != 0))
                index--;
            
            bool end = false;
            if (index == UINT_MAX)
            {
                const Node1* n[maxNode1Depth];
                //Node1* n1 = n0->parent();
                n[1] = n0->parent();
                if (n[1] != nullptr)
                {
                    n0--;
                    // skipping node0's
                    for (; n0 >= n[1]->array; n0--)
                        if (n0->size <= inc)
                            inc -= n0->size;
                        else
                            break;
                    if (n0 - n[1]->array < 0)
                    {
                        cxuint i = 1;
                        for (; i < 20 ; i++)
                        {
                            n[i+1] = n[i]->parent();
                            if (n[i+1] == nullptr)
                            {
                                end = true;
                                break;
                            }
                            n[i]--;
                            // skipping node1's
                            for (; n[i] >= n[i+1]->array1; n[i]--)
                                if (n[i]->totalSize <= inc)
                                    inc -= n[i]->totalSize;
                                else
                                    break;
                            if (n[i] - n[i+1]->array1 >= 0)
                                break;
                        }
                        for (; i > 1; i--)
                            if (n[i+1] != nullptr)
                            {
                                if (end)
                                {
                                    // if end, then set to first node
                                    n[i-1] = n[i][1].array1;
                                    continue;
                                }
                                n[i-1] = n[i]->array1 + n[i]->size-1;
                                // further skipping for shallower level
                                for (; n[i-1] >= n[i]->array1; n[i-1]--)
                                    if (n[i-1]->totalSize <= inc)
                                        inc -= n[i-1]->totalSize;
                                    else
                                        break;
                            }
                        if (n[2] != nullptr)
                        {
                            if (end)
                                n0 = n[1][0].array;
                            else
                            {
                                n0 = n[1]->array + n[1]->size-1;
                                for (; n0 >= n[1]->array; n0--)
                                    if (n0->size <= inc)
                                        inc -= n0->size;
                                    else
                                        break;
                            }
                        }
                    }
                    if (!end)
                        // set index for this node2 (from end of array)
                        index = n0->capacity-1;
                    else
                        // set index for before begin (end)
                        index = UINT_MAX;
                }
                else
                    end = true;
            }
            
            if (!end)
            {
                // skipping last elements in array of node0
                while (index != UINT_MAX && inc != 0)
                {
                    if ((n0->bitMask & (1ULL<<index)) == 0)
                        inc--;
                    index--;
                }
                // and skip empty space
                while (index != UINT_MAX && (n0->bitMask & (1ULL<<index)) != 0)
                    index--;
            }
        }
        
        /// go to previous element
        void prev()
        {
            if (n0->type == NODEV)
            {
                index--;
                return;
            }
            toNode0();
            
            index--;
            while (index != UINT_MAX &&
                (index != n0->capacity && (n0->bitMask & (1ULL<<index)) != 0))
                index--;
            
            bool end = false;
            if (index == UINT_MAX)
            {
                const Node1* n[maxNode1Depth];
                //Node1* n1 = n0->parent();
                n[1] = n0->parent();
                if (n[1] != nullptr)
                {
                    n0--;
                    if (n0 - n[1]->array < 0)
                    {
                        cxuint i = 1;
                        for (; i < 20 ; i++)
                        {
                            n[i+1] = n[i]->parent();
                            if (n[i+1] == nullptr)
                            {
                                end = true;
                                break;
                            }
                            n[i]--;
                            if (n[i] - n[i+1]->array1 >= 0)
                                // if it is this level
                                break;
                        }
                        for (; i > 1; i--)
                            if (n[i+1] != nullptr && !end)
                                // set node for shallower level
                                n[i-1] = n[i]->array1 + n[i]->size-1;
                        if (n[2] != nullptr && !end)
                            // set node for shallowest level
                            n0 = n[1]->array + n[1]->size-1;
                    }
                    if (!end)
                        index = n0->capacity-1;
                }
                else
                    end = true;
            }
            
            if (end)
                // revert if end (before begin)
                n0++;
            
            /// skip last empty space
            while (index != UINT_MAX && (n0->bitMask & (1ULL<<index)) != 0)
                index--;
        }
        
        /// go to i element from current position
        void step(ssize_t i)
        {
            if (i > 0)
                next(i);
            else if (i < 0)
                prev(-i);
        }
        
        /// calculate distance between iterators
        ssize_t diff(const IterBase& k2) const
        {
            if (n0->type == NODEV)
                return index - k2.index;
            
            ssize_t count = 0;
            IterBase i1 = *this;
            IterBase i2 = k2;
            i1.toNode0();
            i2.toNode0();
            if (i1.n0 == i2.n0)
            {
                // if node0's are same
                cxuint index1 = std::min(i1.index, i2.index);
                cxuint index2 = std::max(i1.index, i2.index);
                // calculate element between indices
                for (cxuint i = index1; i < index2; i++)
                    if ((i1.n0->bitMask & (1ULL<<i)) == 0)
                        count++;
                return index2 == i1.index ? count : -count;
            }
            const Node1* n1[maxNode1Depth];
            const Node1* n2[maxNode1Depth];
            const Node0* xn0_1 = i1.n0;
            const Node0* xn0_2 = i2.n0;
            cxuint index1 = i1.index;
            cxuint index2 = i2.index;
            n1[0] = i1.n0->parent();
            n2[0] = i2.n0->parent();
            
            cxuint i = 0;
            /// penetrate to level where node are same
            while (n1[i] != n2[i])
            {
                i++;
                n1[i] = n1[i-1]->parent();
                n2[i] = n2[i-1]->parent();
            };
            
            bool negate = false;
            // sumarizing
            if ((i==0 && xn0_2->index < xn0_1->index) || 
                (i>0 && n2[i-1]->index < n1[i-1]->index))
            {
                // if this position are beyond i2 position, no negation, but swapping
                std::swap_ranges(n1, n1+i+1, n2);
                std::swap(xn0_1, xn0_2);
                std::swap(index1, index2);
            }
            else
                negate = true;
            
            if (i==0) // first level
                for (cxuint j = xn0_1->index+1; j < xn0_2->index; j++)
                    count += n1[i]->array[j].size;
            else
            {
                // couting elements for nodes between this node and i2's node
                for (cxuint j = n1[i-1]->index+1; j < n2[i-1]->index; j++)
                    count += n1[i]->array1[j].totalSize;
                
                for (i--; i >= 1; i--)
                {
                    // in shallower level, count left nodes after n1 node
                    for (cxuint j = n1[i-1]->index+1; j < n1[i]->size; j++)
                        count += n1[i]->array1[j].totalSize;
                    /// count for right nodes before n2 node
                    for (cxuint j = 0; j < n2[i-1]->index; j++)
                        count += n2[i]->array1[j].totalSize;
                }
                // in shallower level, count left nodes after n1 node
                for (cxuint j = xn0_1->index+1; j < n1[0]->size; j++)
                    count += n1[0]->array[j].size;
                /// count for right nodes before n2 node
                for (cxuint j = 0; j < xn0_2->index; j++)
                    count += n2[0]->array[j].size;
            }
            
            // last distances in left
            for (cxuint j = index1; j < xn0_1->capacity; j++)
                if ((xn0_1->bitMask & (1ULL<<j)) == 0)
                    count++;
            // right side
            for (cxuint j = 0; j < index2; j++)
                if ((xn0_2->bitMask & (1ULL<<j)) == 0)
                    count++;
            
            return negate ? -count : count;
        }
    };
    
    /// iterator which allow to modify underlying element
    struct Iter: IterBase
    {
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef ssize_t difference_type;
        typedef std::bidirectional_iterator_tag iterator_category;
        
        /// constructor
        Iter(Node0* n0 = nullptr, cxuint index = 0): IterBase{n0, index}
        { }
        
        Iter(const IterBase& it) : IterBase(it)
        { }
        
        /// pre-increment
        Iter& operator++()
        {
            IterBase::next();
            return *this;
        }
        // post-increment
        Iter operator++(int)
        {
            Iter tmp = *this;
            IterBase::next();
            return tmp;
        }
        /// add to iterator
        Iter operator+(ssize_t i) const
        {
            Iter tmp = *this;
            tmp.IterBase::step(i);
            return tmp;
        }
        /// add to iterator with assignment
        Iter& operator+=(ssize_t i)
        {
            IterBase::step(i);
            return *this;
        }
        /// pre-decrement
        Iter& operator--()
        {
            IterBase::prev();
            return *this;
        }
        /// post-decrement
        Iter operator--(int)
        {
            Iter tmp = *this;
            IterBase::prev();
            return tmp;
        }
        /// subtract from iterator
        Iter operator-(ssize_t i) const
        {
            Iter tmp = *this;
            tmp.IterBase::step(-i);
            return tmp;
        }
        /// subtract from iterator with assignment
        Iter& operator-=(ssize_t i)
        {
            IterBase::step(-i);
            return *this;
        }
        // distance between iterators
        ssize_t operator-(const IterBase& i2) const
        { return IterBase::diff(i2); }
        /// get element
        T& operator*() const
        {
            return (IterBase::n0->type == NODE0) ?
                    IterBase::n0->arrayOut[IterBase::index] :
                    IterBase::nv->arrayOut[IterBase::index];
        }
        /// get element
        T* operator->() const
        {
            return (IterBase::n0->type == NODE0) ?
                    IterBase::n0->arrayOut + IterBase::index :
                    IterBase::nv->arrayOut + IterBase::index;
        }
        /// equal to
        bool operator==(const IterBase& it) const
        { return IterBase::n0==it.n0 && IterBase::index==it.index; }
        /// not equal
        bool operator!=(const IterBase& it) const
        { return IterBase::n0!=it.n0 || IterBase::index!=it.index; }
    };
    
    /// iterator that allow only to read element
    struct ConstIter: IterBase
    {
        typedef T value_type;
        typedef const T& reference;
        typedef const T* pointer;
        typedef ssize_t difference_type;
        typedef std::bidirectional_iterator_tag iterator_category;
        
        /// constructor
        ConstIter(const Node0* n0 = nullptr, cxuint index = 0): IterBase{n0, index}
        { }
        
        ConstIter(const IterBase& it) : IterBase(it)
        { }
        
        /// pre-increment
        ConstIter& operator++()
        {
            IterBase::next();
            return *this;
        }
        /// post-increment
        ConstIter operator++(int)
        {
            ConstIter tmp = *this;
            IterBase::next();
            return tmp;
        }
        /// add to iterator
        ConstIter operator+(ssize_t i) const
        {
            ConstIter tmp = *this;
            tmp.IterBase::step(i);
            return tmp;
        }
        /// add to iterator with assignment
        ConstIter& operator+=(ssize_t i)
        {
            IterBase::step(i);
            return *this;
        }
        /// pre-decrement
        ConstIter& operator--()
        {
            IterBase::prev();
            return *this;
        }
        /// post-decrement
        ConstIter operator--(int)
        {
            ConstIter tmp = *this;
            IterBase::prev();
            return tmp;
        }
        /// subtract from iterator
        ConstIter operator-(ssize_t i) const
        {
            ConstIter tmp = *this;
            tmp.IterBase::step(-i);
            return tmp;
        }
        /// subtract from iterator with assignment
        ConstIter& operator-=(ssize_t i)
        {
            IterBase::step(-i);
            return *this;
        }
        
        // distance between iterators
        ssize_t operator-(const IterBase& i2) const
        { return IterBase::diff(i2); }
        /// get element
        const T& operator*() const
        {
            return (IterBase::n0->type == NODE0) ?
                    IterBase::n0->arrayOut[IterBase::index] :
                    IterBase::nv->arrayOut[IterBase::index];
        }
        /// get element
        const T* operator->() const
        {
            return (IterBase::n0->type == NODE0) ?
                    IterBase::n0->arrayOut + IterBase::index :
                    IterBase::nv->arrayOut + IterBase::index;
        }
        /// equal to
        bool operator==(const IterBase& it) const
        { return IterBase::n0==it.n0 && IterBase::index==it.index; }
        /// not equal
        bool operator!=(const IterBase& it) const
        { return IterBase::n0!=it.n0 || IterBase::index!=it.index; }
    };
#ifdef DTREE_TESTING
public:
#else
protected:
#endif
    union {
        Node0 n0; // root Node0
        Node1 n1; // root Node1
        NodeV nv; // root NodeV
    };
public:
    ///
    typedef ConstIter const_iterator;
    typedef Iter iterator;
    typedef T value_type;
    typedef K key_type;
    
    /// default constructor
    DTree(const Comp& comp = Comp(), const KeyOfVal& kofval = KeyOfVal())
        : Comp(comp), KeyOfVal(kofval), nv()
    { }
    
#if DTREE_TESTING
    DTree(const Node0& node0, const Comp& comp = Comp(),
          const KeyOfVal& kofval = KeyOfVal()) : Comp(comp), KeyOfVal(kofval), n0(node0)
    { }
    
    DTree(const Node1& node1, const Comp& comp = Comp(),
          const KeyOfVal& kofval = KeyOfVal()) : Comp(comp), KeyOfVal(kofval), n1(node1)
    { }
#endif
    /// constructor with range assignment
    template<typename Iter>
    DTree(Iter first, Iter last, const Comp& comp = Comp(),
          const KeyOfVal& kofval = KeyOfVal()) : Comp(comp), KeyOfVal(kofval), nv()
    {
        for (Iter it = first; it != last; ++it)
            insert(*it);
    }
    
    /// constructor with initializer list
    DTree(std::initializer_list<value_type> init, const Comp& comp = Comp(),
          const KeyOfVal& kofval = KeyOfVal()) : Comp(comp), KeyOfVal(kofval), nv()
    {
        for (const value_type& v: init)
            insert(v);
    }
    /// copy construcror
    DTree(const DTree& dt) 
    {
        if (dt.nv.type == NODEV)
            nv = dt.nv;
        else if (dt.n0.type == NODE0)
        {
            n0.array = nullptr;
            n0 = dt.n0;
        }
        else
        {
            n1.array = nullptr;
            n1 = dt.n1;
        }
    }
    /// move constructor
    DTree(DTree&& dt) noexcept
    {
        if (dt.nv.type == NODEV)
            nv = dt.nv;
        else if (dt.n0.type == NODE0)
        {
            n0.array = nullptr;
            n0 = std::move(dt.n0);
        }
        else
        {
            n1.array = nullptr;
            n1 = std::move(dt.n1);
        }
    }
    /// destructor
    ~DTree()
    {
        if (n0.type == NODE0)
            n0.~Node0();
        else if (n0.type != NODEV)
            n1.~Node1();
    }
    
    /// copy assignment
    DTree& operator=(const DTree& dt)
    {
        if (this == &dt)
            return *this;
        if (n0.type == NODE0)
            n0.~Node0();
        else if (n0.type != NODEV)
            n1.~Node1();
        
        if (dt.n0.type == NODEV)
            nv = dt.nv;
        else if (dt.n0.type == NODE0)
        {
            n0.array = nullptr;
            n0 = dt.n0;
        }
        else
        {
            n1.array = nullptr;
            n1 = dt.n1;
        }
        return *this;
    }
    /// move assignment
    DTree& operator=(DTree&& dt) noexcept
    {
        if (this == &dt)
            return *this;
        if (n0.type == NODE0)
            n0.~Node0();
        else if (n0.type != NODEV)
            n1.~Node1();
        
        if (dt.n0.type == NODEV)
            nv = dt.nv;
        else if (dt.n0.type == NODE0)
        {
            n0.array = nullptr;
            n0 = std::move(dt.n0);
        }
        else
        {
            n1.array = nullptr;
            n1 = std::move(dt.n1);
        }
        return *this;
    }
    /// assignment of initilizer list
    DTree& operator=(std::initializer_list<value_type> init)
    { return *this; }
    
    /// return true if empty
    bool empty() const
    { return (n0.type==NODE0 && n0.size==0) || (nv.type==NODEV && nv.size==0); }
    
    /// return size
    size_t size() const
    {
        if (nv.type==NODEV)
            return nv.size;
        return n0.type==NODE0 ? n0.size : n1.totalSize;
    }
    
    /// clear (remove all elements)
    void clear()
    {
        if (n0.type == NODE0)
            n0.~Node0();
        else if (n0.type != NODEV)
            n1.~Node1();
        nv = NodeV();
    }
    
private:
    IterBase findInt(const key_type& key) const
    {
        if (n0.type == NODEV)
            return IterBase{&n0, nv.find(key, *this, *this)};
        
        if (n0.type == NODE0)
            return IterBase{&n0, n0.find(key, *this, *this)};
        const Node1* curn1 = &n1;
        cxuint index = 0;
        while (curn1->type == NODE2)
        {
            index = curn1->upperBoundN(key, *this, *this);
            if (index == 0)
                return end();
            curn1 = curn1->array1 + index - 1;
        }
        // node1
        index = curn1->upperBoundN(key, *this, *this);
        if (index == 0)
            return end();
        const Node0* curn0 = curn1->array + index - 1;
        // node0
        IterBase it{curn0, curn0->find(key, *this, *this)};
        if (it.index == curn0->capacity)
            return end();
        return it;
    }
    
    IterBase lower_boundInt(const key_type& key) const
    {
        if (n0.type == NODEV)
            return IterBase{&n0, nv.lower_bound(key, *this, *this)};
        
        if (n0.type == NODE0)
            return IterBase{&n0, n0.lower_bound(key, *this, *this)};
        const Node1* curn1 = &n1;
        cxuint index = 0;
        while (curn1->type == NODE2)
        {
            index = curn1->upperBoundN(key, *this, *this);
            if (index == 0)
                return begin();
            curn1 = curn1->array1 + index - 1;
        }
        // node1
        index = curn1->upperBoundN(key, *this, *this);
        if (index == 0)
            return begin();
        const Node0* curn0 = curn1->array + index - 1;
        // node0
        IterBase it{curn0, curn0->lower_bound(key, *this, *this)};
        if (it.index == curn0->capacity)
            it.toNextNode0();
        return it;
    }
    
    IterBase upper_boundInt(const key_type& key) const
    {
        if (n0.type == NODEV)
            return IterBase{&n0, nv.upper_bound(key, *this, *this)};
        
        if (n0.type == NODE0)
            return IterBase{&n0, n0.upper_bound(key, *this, *this)};
        const Node1* curn1 = &n1;
        cxuint index = 0;
        while (curn1->type == NODE2)
        {
            index = curn1->upperBoundN(key, *this, *this);
            if (index == 0)
                return begin();
            curn1 = curn1->array1 + index - 1;
        }
        index = curn1->upperBoundN(key, *this, *this);
        if (index == 0)
            return begin();
        const Node0* curn0 = curn1->array + index - 1;
        // node0
        IterBase it{curn0, curn0->upper_bound(key, *this, *this)};
        if (it.index == curn0->capacity)
            it.toNextNode0();
        return it;
    }
    
public:
    /// find element or return end iterator
    iterator find(const key_type& key)
    { return findInt(key); }
    /// find element or return end iterator
    const_iterator find(const key_type& key) const
    { return findInt(key); }
    
    /// return iterator to first element
    iterator begin()
    {
        if (nv.type == NODEV)
            return iterator(&n0, 0);
        if (n0.type == NODE0)
            return iterator(&n0, n0.firstPos);
        else
        {
            iterator it(n1.getFirstNode0());
            it.index = it.n0->firstPos;
            return it;
        }
    }
    /// return iterator to first element
    const_iterator cbegin() const
    {
        if (nv.type == NODEV)
            return const_iterator(&n0, 0);
        if (n0.type == NODE0)
            return const_iterator(&n0, n0.firstPos);
        else
        {
            const_iterator it(n1.getFirstNode0());
            it.index = it.n0->firstPos;
            return it;
        }
    }
    /// return iterator to first element
    const_iterator begin() const
    { return cbegin(); }
    /// return iterator after last element
    iterator end()
    {
        if (nv.type == NODEV)
            return iterator(&n0, nv.size);
        return iterator(&n0, n0.type==NODE0 ? n0.capacity : n1.size);
    }
    /// return iterator after last element
    const_iterator end() const
    {
        if (nv.type == NODEV)
            return const_iterator(&n0, nv.size);
        return const_iterator(&n0, n0.type==NODE0 ? n0.capacity : n1.size);
    }
    /// return iterator after last element
    const_iterator cend() const
    {
        if (nv.type == NODEV)
            return const_iterator(&n0, nv.size);
        return const_iterator(&n0, n0.type==NODE0 ? n0.capacity : n1.size);
    }
    
    /// first element that not less than key
    iterator lower_bound(const key_type& key)
    { return lower_boundInt(key); }
    /// first element that not less than key
    const_iterator lower_bound(const key_type& key) const
    { return lower_boundInt(key); }
    /// first element that greater than key
    iterator upper_bound(const key_type& key)
    { return upper_boundInt(key); }
    /// first element that greater than key
    const_iterator upper_bound(const key_type& key) const
    { return upper_boundInt(key); }
    
#ifdef DTREE_TESTING
public:
#else
private:
#endif
    static void findReorgBounds0(cxuint n0Index, const Node1* curn1, cxuint neededN0Size,
                          cxint& left, cxint& right, bool* removeOneNode = nullptr)
    {
        cxuint freeSpace = 0;
        left = right = n0Index;
        cxuint nodeCount = 0;
        do {
            left--;
            right++;
            if (left >= 0)
            {
                freeSpace += maxNode0Size - curn1->array[left].size;
                nodeCount++;
            }
            if (right < curn1->size)
            {
                freeSpace += maxNode0Size - curn1->array[right].size;
                nodeCount++;
            }
        } while (freeSpace < neededN0Size && (left >= 0 || right < curn1->size));
        
        left = std::max(0, left);
        right = std::min(curn1->size-1, right);
        if (removeOneNode != nullptr)
            *removeOneNode = freeSpace >= neededN0Size;
    }
    
    static void findReorgBounds1(const Node1* prevn1, const Node1* curn1,
            size_t neededN1Size, size_t maxN1Size, cxint& left, cxint& right,
            bool* removeOneNode = nullptr)
    {
        cxuint n1Index = prevn1->index;
        size_t freeSpace = 0;
        left = right = n1Index;
        cxuint children = prevn1->size;
        const size_t needed = neededN1Size + (neededN1Size>>2);
        
        do {
            left--;
            right++;
            if (left >= 0)
            {
                freeSpace += maxN1Size -
                        std::min(curn1->array1[left].totalSize, maxN1Size);
                children += curn1->array1[left].size;
            }
            if (right < curn1->size)
            {
                freeSpace += maxN1Size -
                        std::min(curn1->array1[right].totalSize, maxN1Size);
                children += curn1->array1[right].size;
            }
        } while (freeSpace < needed && (left >= 0 || right < curn1->size));
        
        left = std::max(0, left);
        right = std::min(curn1->size-1, right);
        if (removeOneNode != nullptr)
            *removeOneNode = (freeSpace >= needed &&
                    children < maxNode1Size*(right-left));
    }
public:
    
    /// insert new element
    std::pair<iterator, bool> insert(const value_type& value)
    {
        if (nv.type == NODEV)
        {
            if (!nv.isFull())
            {
                auto res = nv.insert(value, *this, *this);
                return std::make_pair(iterator(&n0, res.first), res.second);
            }
            else
            {
                // if full
                NodeV tmpnv = nv;
                n0.array = nullptr;
                n0 = Node0();
                n0.setFromArray(tmpnv.size, tmpnv.array);
            }
        }
        const key_type key = KeyOfVal::operator()(value);
        iterator it = lower_bound(key);
        if (it!=end())
        {
            const key_type itkey = KeyOfVal::operator()(*it);
            if (!Comp::operator()(key, itkey))
                return std::make_pair(it, false);
        }
        
        if (it.n0->type != NODE0)
            it.toNode0();
        
        cxuint index = it.n0->insert(value, *this, *this).first;
        Node1* curn1 = it.n0->parent();
        iterator newit(it.n0, index);
        // reorganize/merge/split needed
        if (it.n0->size > maxNode0Size)
        {
            // simple split to first level
            cxuint n0Index = it.n0->index;
            if (curn1 == nullptr || curn1->size < maxNode1Size)
            {
                // put new node0 in node1 or create new node1 with two nodes
                Node0 node0_2;
                it.n0->split(node0_2);
                cxuint index = 0;
                bool secondNode = false;
                if (Comp::operator()(key,
                        KeyOfVal::operator()(node0_2.array[it.n0->firstPos])))
                    // key < first key in second node0
                    index = it.n0->lower_bound(key, *this, *this);
                else
                {   // put to node0 2
                    secondNode = true;
                    index = node0_2.lower_bound(key, *this, *this);
                }
                if (curn1 == nullptr)
                {
                    Node1 node1(std::move(*it.n0), std::move(node0_2), *this);
                    new(&n1) Node1();
                    n1 = std::move(node1);
                    
                    return std::make_pair(iterator(n1.array + secondNode, index), true);
                }
                curn1->insertNode0(std::move(node0_2), n0Index+1, *this, false);
                newit = iterator(curn1->array + n0Index + secondNode, index);
            }
            else
            {
                cxint left, right;
                // reorganize in this level
                findReorgBounds0(it.n0->index, curn1, it.n0->size, left, right);
                
                // reorganize array from left to right
                curn1->reorganizeNode0s(left, right+1);
                // find newit for inserted value
                newit.n0 = curn1->array + curn1->upperBoundN(key, *this, *this) - 1;
                newit.index = newit.n0->lower_bound(key, *this, *this);
            }
        }
        
        cxuint level = 1;
        Node1* prevn1 = nullptr;
        while (curn1 != nullptr)
        {
            prevn1 = curn1;
            curn1 = prevn1->parent();
            prevn1->first = prevn1->NodeBase::type==NODE1 ?
                    KeyOfVal::operator()(
                        prevn1->array[0].array[prevn1->array[0].firstPos]) :
                    prevn1->array1[0].first;
            
            prevn1->totalSize++; // increase
            cxuint maxN1Size = maxTotalSize(level);
            
            if (prevn1->totalSize > maxN1Size)
            {
                cxuint n0Index = newit.n0->index;
                cxuint n1Index = prevn1->index;
                if (curn1 == nullptr || curn1->size < maxNode1Size)
                {
                    // simple split
                    Node1 node1_2;
                    prevn1->splitNode(node1_2, *this);
                    
                    if (level==1)
                    {
                        if (n0Index < prevn1->size)
                            newit.n0 = prevn1->array + n0Index;
                        else
                            newit.n0 = node1_2.array + n0Index - prevn1->size;
                    }
                    // node0 has been moved (no array/array1 pointer change/reallocate)
                    if (curn1 == nullptr)
                    {
                        Node1 node1(std::move(*prevn1), std::move(node1_2));
                        n1 = std::move(node1);
                    }
                    else
                        curn1->insertNode1(std::move(node1_2), n1Index+1, false);
                }
                else
                {
                    // reorganize nodes
                    cxint left, right;
                    findReorgBounds1(prevn1, curn1, prevn1->totalSize,
                            maxN1Size, left, right);
                    // reorganize array from left to right
                    curn1->reorganizeNode1s(left, right+1, *this);
                    if (level == 1)
                    {
                        const Node1* pn1 = curn1->array1 +
                                curn1->upperBoundN(key, *this, *this) - 1;
                        newit.n0 = pn1->array + pn1->upperBoundN(key, *this, *this) - 1;
                    }
                }
            }
            level++;
        }
        
        return std::make_pair(newit, true);
    }
    /// insert new elements from initializer list
    void insert(std::initializer_list<value_type> ilist)
    {
        for (const value_type& v: ilist)
            insert(v);
    }
    // insert new elements from iterators
    template<typename Iter>
    void insert(Iter first, Iter last)
    {
        for (Iter it = first; it != last; ++it)
            insert(*it);
    }
    /// remove element in postion pointed by iterator
    iterator erase(const_iterator it)
    {
        
        if (nv.type == NODEV)
        {
            if (nv.size == 0)
                return it;
            nv.erase(it.index);
            return it;
        }
        K key = KeyOfVal::operator()(*it);
        
        const_iterator newit(it);
        if (it == end())
            return newit; // do nothing (if end or free space)
        if (!it.n0->erase(it.index))
            // not finished
            return newit;
        
        newit.index = newit.n0->lower_bound(key, *this, *this);
        if (newit.n0->size != 0 && newit.index == newit.n0->firstPos)
            // update key because will be removed and first will be greater than it
            key = KeyOfVal::operator()(*newit);
        if (it.n0 == &n0)
        {
            if (n0.size <= NodeVElemsNum)
            {
                // to NodeV
                Node0 tmpn0 = std::move(n0);
                n0.array = nullptr;
                nv.type = NODEV;
                nv.assignNode0(tmpn0);
            }
            return newit;
        }
        
        Node1* curn1 = it.n0->parent();
        if (it.n0->size < minNode0Size)
        {
            Node1* curn1 = it.n0->parent();
            const cxuint n0Index = it.n0->index;
            cxuint n0Left1 = n0Index > 0 ? curn1->array[n0Index-1].size : UINT_MAX;
            cxuint n0Right1 = n0Index+1 < curn1->size ? curn1->array[n0Index+1].size :
                    UINT_MAX;
            cxuint mergedN0Index = UINT_MAX;
            if (n0Left1 < n0Right1)
            {
                if (n0Left1+minNode0Size-1 <= maxNode0Size)
                {
                    curn1->array[n0Index-1].merge(*it.n0);
                    curn1->eraseNode0(n0Index, *this, false);
                    mergedN0Index = n0Index-1;
                    newit.n0 = curn1->array + n0Index-1;
                    newit.index = newit.n0->lower_bound(key, *this, *this);
                }
            }
            else if (n0Right1 != UINT_MAX)
            {
                if (n0Right1+minNode0Size-1 <= maxNode0Size)
                {
                    it.n0->merge(curn1->array[n0Index+1]);
                    curn1->eraseNode0(n0Index+1, *this, false);
                    mergedN0Index = n0Index;
                    newit.n0 = curn1->array + n0Index;
                    newit.index = newit.n0->lower_bound(key, *this, *this);
                }
            }
            if (mergedN0Index == UINT_MAX)
            {
                // reorganization needed before inserting
                cxint left, right;
                bool removeNode0 = false;
                // reorganize in this level
                findReorgBounds0(n0Index, curn1, curn1->array[n0Index].size,
                                left, right, &removeNode0);
                curn1->reorganizeNode0s(left, right+1, removeNode0);
                // find newit for inserted value
                newit.n0 = curn1->array + curn1->upperBoundN(key, *this, *this) - 1;
                newit.index = newit.n0->lower_bound(key, *this, *this);
            }
        }
        if (newit.index == newit.n0->capacity)
        {
            newit.toNextNode0();
            if (newit != end())
                key = KeyOfVal::operator()(newit.n0->array[newit.n0->firstPos]);
        }
        
        cxuint level = 1;
        Node1* prevn1 = nullptr;
        const Node1* newItN1 = newit.n0->parent();
        while (curn1 != nullptr)
        {
            curn1->totalSize--;
            prevn1 = curn1;
            curn1 = prevn1->parent();
            prevn1->first = prevn1->NodeBase::type==NODE1 ?
                    KeyOfVal::operator()(
                        prevn1->array[0].array[prevn1->array[0].firstPos]) :
                    prevn1->array1[0].first;
            
            cxuint maxN1Size = maxTotalSize(level);
            cxuint minN1Size = minTotalSize(level);
            if (curn1 == nullptr)
            {
                if (prevn1->size == 1)
                {
                    // remove old root
                    if (prevn1->type == NODE1)
                    {
                        Node0 tempN0 = std::move(prevn1->array[0]);
                        n1.~Node1();
                        n0.array = nullptr;
                        n0 = std::move(tempN0);
                        n0.index = 255;
                        newit.n0 = &n0;
                    }
                    else
                    {
                        Node1 tempN1 = std::move(prevn1->array1[0]);
                        n1.~Node1();
                        n1.array = nullptr;
                        n1 = std::move(tempN1);
                        n1.index = 255;
                    }
                }
                break;
            }
            
            const cxuint n1Index = prevn1->index;
            size_t n1Left1 = n1Index > 0 ? curn1->array1[n1Index-1].totalSize : SIZE_MAX;
            size_t n1Right1 = n1Index+1 < curn1->size ?
                    curn1->array1[n1Index+1].totalSize : SIZE_MAX;
            
            if (prevn1->totalSize >= minN1Size && prevn1->size > 1)
            {
                level++;
                continue;
            }
            
            // check number of total children after merging
            if (n1Left1!=SIZE_MAX &&
                prevn1->size + curn1->array1[n1Index-1].size > maxNode1Size)
                n1Left1 = SIZE_MAX;
            if (n1Right1!=SIZE_MAX &&
                prevn1->size + curn1->array1[n1Index+1].size > maxNode1Size)
                n1Right1 = SIZE_MAX;
                
            cxuint mergedN1Index = UINT_MAX;
            if (n1Left1 < n1Right1 && n1Left1 != SIZE_MAX)
            {
                if (n1Left1+minN1Size-1 <= maxN1Size)
                {
                    curn1->array1[n1Index-1].merge(std::move(*prevn1));
                    curn1->eraseNode1(n1Index, false);
                    mergedN1Index = n1Index-1;
                    if (level == 1 && newItN1 == prevn1)
                        newit.n0 = curn1->array1[n1Index-1].array +
                            curn1->array1[n1Index-1].upperBoundN(key, *this, *this) - 1;
                }
            }
            else if (n1Right1 != SIZE_MAX)
            {
                if (n1Right1+minN1Size-1 <= maxN1Size)
                {
                    prevn1->merge(std::move(curn1->array1[n1Index+1]));
                    curn1->eraseNode1(n1Index+1, false);
                    mergedN1Index = n1Index;
                    if (level == 1 && newItN1 == prevn1)
                        newit.n0 = curn1->array1[n1Index].array +
                            curn1->array1[n1Index].upperBoundN(key, *this, *this) - 1;
                }
            }
            if (mergedN1Index == UINT_MAX)
            {
                // reorganization needed before inserting
                cxint left, right;
                bool removeNode1 = false;
                // reorganize in this level
                findReorgBounds1(prevn1, curn1, prevn1->totalSize, maxN1Size,
                                left, right, &removeNode1);
                curn1->reorganizeNode1s(left, right+1, *this, removeNode1);
                if (level == 1 && newItN1 == prevn1)
                {
                    const Node1* pn1 = curn1->array1 +
                            curn1->upperBoundN(key, *this, *this) - 1;
                    newit.n0 = pn1->array + pn1->upperBoundN(key, *this, *this) - 1;
                }
            }
            level++;
        }
        return newit;
    }
    /// remove element by key
    size_t erase(const key_type& key)
    {
        iterator it = find(key);
        if (it == end())
            return 0;
        erase(it);
        return 1;
    }
    
    /// replace element with checking range
    void replace(iterator iter, const value_type& value)
    {
        if (iter == end())
            return;
        if (iter != begin())
        {
            iterator prevIt = iter;
            --prevIt;
            if (*prevIt >= value)
                throw std::out_of_range("Value out of range");
        }
        iterator nextIt = iter;
        ++nextIt;
        if (nextIt != end() && value >= *nextIt)
            throw std::out_of_range("Value out of range");
        
        if (iter.n0->type == NODE0)
        {
            Node0* n0 = iter.n0;
            n0->array[iter.index] = value;
            // fill up surrounding freespace for correct order
            for (cxint i = iter.index-1; i >= 0 && (n0->bitMask & (1ULL<<i)) != 0; i--)
                n0->array[i] = value;
            for (cxint i = iter.index+1; i < n0->capacity &&
                                    (n0->bitMask & (1ULL<<i)) != 0; i++)
                n0->array[i] = value;
        }
        else if (iter.n0->type == NODEV)
            nv.array[iter.index] = value;
    }
    
    /// lexicograhical equal to
    bool operator==(const DTree& dt) const
    { return size()==dt.size() && std::equal(begin(), end(), dt.begin()); }
    /// lexicograhical not equal
    bool operator!=(const DTree& dt) const
    { return size()!=dt.size() || !std::equal(begin(), end(), dt.begin()); }
    /// lexicograhical less
    bool operator<(const DTree& dt) const
    { return std::lexicographical_compare(begin(), end(), dt.begin(), dt.end()); }
    /// lexicograhical less or equal
    bool operator<=(const DTree& dt) const
    { return !(dt < *this); }
    /// lexicograhical greater
    bool operator>(const DTree& dt) const
    { return (dt < *this); }
    /// lexicograhical greater or equal
    bool operator>=(const DTree& dt) const
    { return !(*this < dt); }
};


/// DTree set
template<typename T, typename Comp = std::less<T> >
class DTreeSet: public DTree<T, T, Comp, Identity<T> >
{
private:
    typedef DTree<T, T, Comp, Identity<T> > Impl;
public:
    typedef typename Impl::const_iterator const_iterator;
    typedef typename Impl::iterator iterator;
    typedef typename Impl::value_type value_type;
    typedef typename Impl::key_type key_type;
    
    /// default constructor
    DTreeSet(const Comp& comp = Comp()) : Impl(comp)
    { }
#ifdef DTREE_TESTING
    DTreeSet(const typename Impl::Node0& node0, const Comp& comp = Comp())
        : Impl(node0, comp)
    { }
    DTreeSet(const typename Impl::Node1& node1, const Comp& comp = Comp())
        : Impl(node1, comp)
    { }
#endif
    /// constructor iterator ranges
    template<typename Iter>
    DTreeSet(Iter first, Iter last, const Comp& comp = Comp()) :
            Impl(first, last, comp)
    { }
    /// constructor with element ranges
    DTreeSet(std::initializer_list<value_type> init, const Comp& comp = Comp()) 
            : Impl(init, comp)
    { }
};

/// DTree map
template<typename K, typename V, typename Comp = std::less<K> >
class DTreeMap: public DTree<K, std::pair<const K, V>, Comp, SelectFirst<K, V>,
            std::pair<K, V> >
{
private:
    typedef DTree<K, std::pair<const K, V>, Comp, SelectFirst<K, V>,
                std::pair<K, V> > Impl;
public:
    typedef typename Impl::const_iterator const_iterator;
    typedef typename Impl::iterator iterator;
    typedef typename Impl::value_type value_type;
    typedef typename Impl::key_type key_type;
    typedef V mapped_type;
    
    /// default constructor
    DTreeMap(const Comp& comp = Comp()) : Impl(comp)
    { }
    /// constructor iterator ranges
    template<typename Iter>
    DTreeMap(Iter first, Iter last, const Comp& comp = Comp()) :
            Impl(first, last, comp)
    { }
    /// constructor with element ranges
    DTreeMap(std::initializer_list<value_type> init, const Comp& comp = Comp()) 
            : Impl(init, comp)
    { }
    
    /// replace element with checking range of the key
    void replace(iterator iter, const value_type& value)
    {
        if (iter == Impl::end())
            return;
        if (iter != Impl::begin())
        {
            iterator prevIt = iter;
            --prevIt;
            if (prevIt->first >= value.first)
                throw std::out_of_range("Key out of range");
        }
        iterator nextIt = iter;
        ++nextIt;
        if (nextIt != Impl::end() && value.first >= nextIt->first)
            throw std::out_of_range("Key out of range");
        if (iter.n0->type == Impl::NODE0)
        {
            typename Impl::Node0* n0 = iter.n0;
            n0->array[iter.index].first = value.first;
            n0->array[iter.index].second = value.second;
            // fill up surrounding freespace for correct order
            for (cxint i = iter.index-1; i >= 0 && (n0->bitMask & (1ULL<<i)) != 0; i--)
                n0->array[i].first = value.first;
            for (cxint i = iter.index+1; i < n0->capacity &&
                                    (n0->bitMask & (1ULL<<i)) != 0; i++)
                n0->array[i].first = value.first;
        }
        else if (iter.n0->type == Impl::NODEV)
        {
            Impl::nv.array[iter.index].first = value.first;
            Impl::nv.array[iter.index].second = value.second;
        }
    }
    
    /// put element, insert or replace if found
    std::pair<iterator, bool> put(const value_type& value)
    {
        auto res = Impl::insert(value);
        if (!res.second)
            res.first->second = value.second;
        return res;
    }
    
    /// get reference to element pointed by key
    mapped_type& at(const key_type& key)
    {
        iterator it = Impl::find(key);
        if (it == Impl::end())
            throw std::out_of_range("DTreeMap key not found");
        return it->second;
    }
    /// get reference to element pointed by key
    const mapped_type& at(const key_type& key) const
    {
        const_iterator it = Impl::find(key);
        if (it == Impl::end())
            throw std::out_of_range("DTreeMap key not found");
        return it->second;
    }
    /// get reference to element pointed by key (add if key doesn't exists)
    mapped_type& operator[](const key_type& key)
    {
        iterator it = Impl::insert(std::pair<const K, V>(key, V())).first;
        return it->second;
    }
};

};

#endif
