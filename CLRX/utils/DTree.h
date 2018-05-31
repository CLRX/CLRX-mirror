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
#include <initializer_list>
#include <climits>
#include <cstddef>
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

/// main D-Tree container (D-Tree is kind of the B-Tree
template<typename K, typename T = K, typename Comp = std::less<K>,
        typename KeyOfVal = Identity<K> >
class DTree: private Comp, KeyOfVal
{
public:
    /// node type
    enum : cxbyte
    {
        NODE0 = 0,  ///< Node0
        NODE1,  ///< Node1 that holds Node0's
        NODE2   ///< Node1 that holds Node1's
    };
    
    // number of elements in Node1
    static const cxuint maxNode1Size = 8;
    static const cxuint maxNode1Shift = 3;
    
    static const cxuint maxNode0Capacity = 64;
    static const cxuint normalNode0Capacity = maxNode0Capacity>>1;
    static const cxuint minNode0Capacity = 20;
    static const cxuint freePlacesShift = 1;
    static const cxuint minFreePlacesShift = 3;
    
    static const cxuint maxNode0Size = ((maxNode0Capacity*1000 /
            ((1<<minFreePlacesShift)+1))*(1<<minFreePlacesShift)) / 1000;
    static const cxuint normalNode0Size = ((normalNode0Capacity*1000 /
            ((1<<minFreePlacesShift)+1))*(1<<minFreePlacesShift)) / 1000;
    
    // get maximal total size for node in depth level
    static size_t maxTotalSize(cxuint level)
    {
        if (level == 0)
            return maxNode0Size;
        return size_t(normalNode0Size) << (maxNode1Shift * level);
    }
    
    // get normal total size for node in depth level
    static size_t normalTotalSize(cxuint level)
    {
        if (level == 0)
            return normalNode0Size;
        return size_t(normalNode0Size) << (maxNode1Shift * level - 1);
    }
    
    // get minimal total size for node in depth level
    static size_t minTotalSize(cxuint level)
    {
        if (level == 0)
            return maxNode0Size / 3;
        return (size_t(normalNode0Size) << (maxNode1Shift * level)) / 3;
    }
    
    // parent pointer part size of array (heap)
    static const int parentEntrySize = 8;
    
    struct NodeBase
    {
        cxbyte type;
        
        NodeBase(cxbyte _type) : type(_type)
        { }
    };
    
    struct Node1;
    static const int parentEntryIndex = -(parentEntrySize / parentEntrySize);
    
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
        T* array;   // array
        
        Node0(): NodeBase(NODE0), index(255), size(0), capacity(0),
                    firstPos(0), bitMask(0ULL), array(nullptr)
        { }
        
        Node0(const Node0& node): NodeBase(NODE0), bitMask(node.bitMask),
                    index(node.index), size(node.size), capacity(node.capacity),
                    firstPos(node.firstPos), array(nullptr)
        {
            if (node.array != nullptr)
            {
                array = new T[capacity];
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
            delete[] array;
            array = nullptr;
            if (node.array != nullptr)
            {
                array = new T[capacity];
                std::copy(node.array, node.array+capacity, array);
            }
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
        
        const T& operator[](cxuint i) const
        { return array[i]; }
        
        T& operator[](cxuint i)
        { return array[i]; }
        
        /// get lower_bound (first index of element not less than value)
        cxuint lower_bound(const T& v, const Comp& comp, const KeyOfVal& kofval) const
        {
            cxuint index = std::lower_bound(array, array+capacity, v, 
                [&comp, &kofval](const T& v1, const T& v2)
                { return comp(kofval(v1), kofval(v2)); }) - array;
            for (; (bitMask & (1ULL<<index)) != 0; index++);
            return index;
        }
        
        /// get upper_bound (first index of element greater than value)
        cxuint upper_bound(const T& v, const Comp& comp, const KeyOfVal& kofval) const
        {
            cxuint index = std::upper_bound(array, array+capacity, v,
                [&comp, &kofval](const T& v1, const T& v2)
                { return comp(kofval(v1), kofval(v2)); }) - array;
            for (; (bitMask & (1ULL<<index)) != 0; index++);
            return index;
        }
        
        /// internal routine to organize array with empty holes
        static void organizeArray(T& toFill,
                cxuint& i, cxuint size, const T* array, uint64_t inBitMask,
                cxuint& k, cxuint newSize, T* out, uint64_t& outBitMask,
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
        
        /// merge this node with node2
        void merge(const Node0& node2)
        {
            cxuint newSize = size+node2.size;
            cxuint newCapacity = std::min(
                        cxbyte(newSize + (newSize>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            T* newArray = nullptr;
            if (newCapacity != 0)
                newArray = new T[newCapacity];
            
            uint64_t newBitMask = 0ULL;
            cxuint factor = 0;
            // finc - factor increment for empty holes
            const cxuint finc = newCapacity - newSize;
            T toFill = T();
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
            T* newArray0 = nullptr;
            if (newCapacity0 != 0)
                newArray0 = new T[newCapacity0];
            T* newArray1 = nullptr;
            if (newCapacity1 != 0)
                newArray1 = new T[newCapacity1];
            uint64_t newBitMask0 = 0ULL;
            uint64_t newBitMask1 = 0ULL;
            
            T toFill = T();
            cxuint i = 0, k = 0;
            cxuint factor = 0;
            // finc - factor increment for empty holes
            cxuint finc = newCapacity0 - newSize0;
            // store first part to newArray0
            organizeArray(toFill, i, newSize0, array, bitMask, k, newSize0, newArray0,
                        newBitMask0, factor, finc);
            
            // fill a remaining free elements
            if (k < newCapacity0)
            {
                newArray0[k] = toFill;
                newBitMask0 |= (1ULL<<k);
            }
            
            toFill = T();
            k = 0;
            factor = 0;
            // finc - factor increment for empty holes
            finc = newCapacity1 - newSize1;
            // store first part to newArray1
            organizeArray(toFill, i, newSize1, array, bitMask,
                        k, newSize1, newArray1, newBitMask1, factor, finc);
            
            // fill a remaining free elements
            if (k < newCapacity1)
            {
                newArray1[k] = toFill;
                newBitMask1 |= (1ULL<<k);
            }
            
            delete[] array;
            // store into this node (array0)
            array = newArray0;
            capacity = newCapacity0;
            size = newSize0;
            bitMask = newBitMask0;
            firstPos = 0;
            delete[] node2.array;
            // store into node2 (array1)
            node2.array = newArray1;
            node2.capacity = newCapacity1;
            node2.size = newSize1;
            node2.bitMask = newBitMask1;
            node2.firstPos = 0;
        }
        
        /// resize with index update (idx is index to update, used while inserting)
        void resizeWithIndexUpdate(cxint extraSize, cxuint& idx)
        {
            // reorganize array
            cxuint newCapacity = std::min(
                        cxbyte(size+extraSize + ((size+extraSize)>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            T* newArray = nullptr;
            if (newCapacity != 0)
                newArray = new T[newCapacity];
            
            uint64_t newBitMask = 0ULL;
            cxuint factor = 0;
            const cxuint finc = newCapacity - size;
            cxuint newIdx = 255; // new indx after reorganizing
            
            T toFill = T();
            cxuint i = 0, j = 0;
            
            while ((bitMask & (1ULL<<i)) != 0)
                i++; // skip free elem
            
            // fill newArray with skipping free spaces
            for (; i < capacity; j++)
            {
                toFill = newArray[j] = array[i];
                if (idx == i)
                    newIdx = j; // if is it this element
                
                factor += finc;
                if (factor >= size)
                {
                    // add additional (empty) element
                    factor -= size;
                    j++;
                    newArray[j] = array[i];
                    newBitMask |= (1ULL<<j);
                }
                
                i++;
                while ((bitMask & (1ULL<<i)) != 0)
                    i++; // skip free elem
            }
            // fill a remaining free elements
            if (j < newCapacity)
            {
                newArray[j] = toFill;
                newBitMask |= (1ULL<<j);
            }
            
            // determine new index if it is last
            if (newIdx == 255)
                newIdx = newCapacity -
                        ((newBitMask & (1ULL<<(newCapacity-1))) != 0 ? 1 : 0);
            idx = newIdx;
            delete[] array;
            array = newArray;
            capacity = newCapacity;
            bitMask = newBitMask;
            firstPos = 0;
        }
        
        /// simple resize
        void resize(cxint extraSize)
        {
            // reorganize array
            cxuint newCapacity = std::min(
                        cxbyte(size+extraSize + ((size+extraSize)>>freePlacesShift)),
                        cxbyte(maxNode0Capacity));
            T* newArray = nullptr;
            if (newCapacity != 0)
                newArray = new T[newCapacity];
            
            uint64_t newBitMask = 0ULL;
            cxuint factor = 0;
            const cxuint finc = newCapacity - size;
            
            T toFill = T();
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
                        const KeyOfVal& kofval, cxuint indexHint = 255)
        {
            cxuint idx = 255;
            if (indexHint != 255)
            {
                // handle index hint
                idx = indexHint;
                if ((bitMask & (1ULL<<indexHint)) == 0 &&
                    kofval(array[indexHint]) == kofval(v))
                    idx = indexHint;
                else if (indexHint>0 && (bitMask & (1ULL<<(indexHint-1))) == 0 &&
                    kofval(array[indexHint-1]) == kofval(v))
                    idx = indexHint - 1;
            }
            if (idx == 255)
                idx = lower_bound(v, comp, kofval);
            if (idx < capacity && !comp(kofval(v), kofval(array[idx])))
                // is equal, then skip insertion
                return std::make_pair(idx, false);
            
            cxuint minFreePlaces = ((size+1)>>minFreePlacesShift);
            if ((size+1) + minFreePlaces > capacity)
                resizeWithIndexUpdate(1, idx);
            
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
            return std::make_pair(idx, true);
        }
        
        /// erase element of value v
        bool erase(const T& v, const Comp& comp, const KeyOfVal& kofval)
        {
            cxuint index = lower_bound(v, comp, kofval);
            if (index >= capacity || comp(kofval(v), kofval(array[index])))
                return false;  // if not found
            return erase(index);
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
                    array(nullptr)
        { }
        
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
                    totalSize(node.totalSize), array(nullptr)
        {
            copyArray(node);
        }
        
        Node1(Node1&& node) noexcept: NodeBase(node.type), index(node.index),
                    size(node.size), capacity(node.capacity),
                    totalSize(node.totalSize), array(node.array)
        {
            reinterpret_cast<Node1**>(array)[parentEntryIndex] = this;
        }
        
        /// create from two Node0's
        Node1(Node0&& n0, Node0&& n1) : NodeBase(NODE1), index(255),
                size(2), capacity(2), totalSize(n0.size+n1.size), array(nullptr)
        {
            cxbyte* arrayData = new cxbyte[parentEntrySize + capacity*sizeof(Node0)];
            array = reinterpret_cast<Node0*>(arrayData + parentEntrySize);
            *reinterpret_cast<Node1**>(arrayData) = this;
            array[0] = std::move(n0);
            array[1] = std::move(n1);
        }
        /// create from two Node1's
        Node1(Node1&& n0, Node1&& n1) : NodeBase(NODE2), index(255),
                size(2), capacity(2), totalSize(n0.totalSize+n1.totalSize),
                array(nullptr)
        {
            cxbyte* arrayData = new cxbyte[parentEntrySize + capacity*sizeof(Node1)];
            array1 = reinterpret_cast<Node0*>(arrayData + parentEntrySize);
            *reinterpret_cast<Node1**>(arrayData) = this;
            array1[0] = std::move(n0);
            array1[1] = std::move(n1);
        }
        
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
        
        ~Node1()
        {
            freeArray();
        }
        
        Node1& operator=(const Node1& node)
        {
            freeArray();
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
            reinterpret_cast<Node1**>(array)[parentEntryIndex] = this;
            first = node.first;
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
            if (array != nullptr)
            {
                std::move(array, array + size, newArray);
                delete[] (reinterpret_cast<cxbyte*>(array) - parentEntrySize);
            }
            
            array = newArray;
            capacity = newCapacity;
        }
        
        /// reserve0 elements in Node0's array
        void reserve1(cxuint newCapacity)
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
                std::move(array1, array1 + size, newArray);
                delete[] (reinterpret_cast<cxbyte*>(array1) - parentEntrySize);
            }
            
            array1 = newArray;
            capacity = newCapacity;
        }
        
        /// insert Node0
        void insertNode0(const Node0& node)
        {
            NodeBase::type = NODE1;
            if (size+1 > capacity)
                reserve0(std::min(std::max(capacity + (capacity>>1), size+1),
                                  int(maxNode1Size)));
            array[size] = node;
            array[size].index = size;
            if (size == 0)
                first = array[0].array[array[0].firstPos];
            size++;
            totalSize += node.size;
        }
        
        /// insert Node0 - (move to this node)
        void insertNode0(Node0&& node)
        {
            NodeBase::type = NODE1;
            if (size+1 > capacity)
                reserve0(std::min(std::max(capacity + (capacity>>1), size+1),
                                  int(maxNode1Size)));
            array[size] = std::move(node);
            array[size].index = size;
            if (size == 0)
                first = array[0].array[array[0].firstPos];
            size++;
            totalSize += node.size;
        }
        
        /// insert Node1
        void insertNode1(const Node1& node)
        {
            NodeBase::type = NODE2;
            if (size+1 > capacity)
                reserve1(std::min(std::max(capacity + (capacity>>1), size+1),
                                  int(maxNode1Size)));
            array1[size] = node;
            array1[size].index = size;
            if (size == 0)
                first = array1[0].first;
            size++;
            totalSize += node.totalSize;
        }
        
        /// insert Node1 - (move to this node)
        void insertNode1(Node1&& node)
        {
            NodeBase::type = NODE2;
            if (size+1 > capacity)
                reserve1(std::min(std::max(capacity + (capacity>>1), size+1),
                                  int(maxNode1Size)));
            array1[size] = std::move(node);
            array1[size].index = size;
            if (size == 0)
                first = array1[0].first;
            size++;
            totalSize += node.totalSize;
        }
        
        /// remove node0 with index from this node
        void eraseNode0(cxuint index)
        {
            totalSize -= array[index].size;
            std::move(array+index+1, array+size, array+index);
            if (size == 1)
                array[0].~Node0();
            for (cxuint i = index; i < size-1U; i++)
                array[i].index = i;
            if (size > 1 && index == 0)
                first = array[0].array[array[0].firstPos];
            size--;
            if (size + (size>>1) < capacity)
                reserve0(size+1);
        }
        
        /// remove node1 with index from this node
        void eraseNode1(cxuint index)
        {
            totalSize -= array1[index].totalSize;
            std::move(array1+index+1, array1+size, array1+index);
            if (size == 1)
                array1[0].~Node1();
            for (cxuint i = index; i < size-1U; i++)
                array1[i].index = i;
            if (size > 1 && index == 0)
                first = array1[0].first;
            size--;
            if (size + (size>>1) < capacity)
                reserve1(size+1);
        }
        
        /// find node that hold first element not less than value
        cxuint lowerBoundN(const K& v, const Comp& comp, const KeyOfVal& kofval)
        {
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
                    if (comp(array[m].first, v))
                        l = m;
                    else
                        //  !(array[m] < v) -> v <= array[m]
                        r = m;
                }
                if (comp(array[l].first, v))
                    l++;
                return l;
            }
        }
        
        /// find node that hold first element greater than value
        cxuint upperBoundN(const K& v, const Comp& comp, const KeyOfVal& kofval)
        {
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
                    if (comp(v, array[m].first))
                        //  !(array[m] < v) -> v <= array[m]
                        r = m;
                    else
                        l = m;
                }
                if (!comp(v, array[l].first))
                    l++;
                return l;
            }
        }
        
        bool insert(const T& v, const Comp& comp);
        
        bool erase(const T& v, const Comp& comp);
        bool erase(cxbyte index);
    };
public:
    /// main iterator class
    struct IterBase {
        const Node0* n0;    //< node
        cxuint index;   ///< index in array
        
        /// go to inc next elements
        void next(size_t inc)
        {
            // first skip in elements this node0
            while (index < n0->capacity && inc != 0)
            {
                if ((n0->bitMask & (1ULL<<index)) == 0)
                    inc--;
                index++;
            }
            
            bool end = false;
            // we need to go to next node0
            if (index >= n0->capacity)
            {
                const Node1* n[20];
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
                        for (; i > 1; i--)
                            if (n[i+1] != nullptr)
                            {
                                if (end)
                                {
                                    /// fix for end position
                                    n[i-1] = n[i][-1].array1 + n[i][-1].size;
                                    continue;
                                }
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
                            if (end)
                                /// set last node0 for end
                                n0 = n[1][-1].array + n[1][-1].size-1;
                            else
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
                    }
                    if (!end)
                        index = 0;
                    else
                        // if it is end
                        index = n0->capacity;
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
        
        /// go to next element
        void next()
        {
            // skip empty space
            while (index < n0->capacity && (n0->bitMask & (1ULL<<index)) != 0)
                index++;
            index++;
            
            bool end = false;
            if (index >= n0->capacity)
            {
                const Node1* n[20];
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
                                end = true;
                                break;
                            }
                            n[i]++;
                            if (n[i] - n[i+1]->array1 < n[i+1]->size)
                                // if it is this level
                                break;
                        }
                        for (; i > 1; i--)
                            if (n[i+1] != nullptr && !end)
                                // set node for shallower level
                                n[i-1] = n[i]->array1;
                        if (n[2] != nullptr && !end)
                            /// set node0 for shallowest level
                            n0 = n[1]->array;
                    }
                    if (!end)
                        // do not set index if end of tree
                        index = 0;
                }
                else
                    end = true;
            }
            
            if (end)
                // revert if end of tree
                n0--;
            
            // skip empty space
            while (index < n0->capacity && (n0->bitMask & (1ULL<<index)) != 0)
                index++;
        }
        
        /// go to inc previous element
        void prev(size_t inc)
        {
            while (index != UINT_MAX && inc != 0)
            {
                if (index==n0->capacity || (n0->bitMask & (1ULL<<index)) == 0)
                    inc--;
                index--;
            }
            
            bool end = false;
            if (index == UINT_MAX)
            {
                const Node1* n[20];
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
            while (index != UINT_MAX && 
                (index != n0->capacity && (n0->bitMask & (1ULL<<index)) != 0))
                index--;
            
            index--;
            bool end = false;
            if (index == UINT_MAX)
            {
                const Node1* n[20];
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
        ssize_t diff(const IterBase& i2) const
        {
            ssize_t count = 0;
            if (n0 == i2.n0)
            {
                // if node0's are same
                cxuint index1 = std::min(index, i2.index);
                cxuint index2 = std::max(index, i2.index);
                // calculate element between indices
                for (cxuint i = index1; i < index2; i++)
                    if ((n0->bitMask & (1ULL<<i)) == 0)
                        count++;
                return index2 == index ? count : -count;
            }
            const Node1* n1[20];
            const Node1* n2[20];
            const Node0* xn0_1 = n0;
            const Node0* xn0_2 = i2.n0;
            cxuint index1 = index;
            cxuint index2 = i2.index;
            n1[0] = n0->parent();
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
        /// constructor
        Iter(Node0* n0 = nullptr, cxuint index = 0): IterBase{n0, index}
        { }
        
        /// pre-increment
        Iter& operator++()
        {
            IterBase::next();
            return *this;
        }
        // post-increment
        Iter operator++(int) const
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
        Iter operator--(int) const
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
        /// subtractor from iterator with assignment
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
        { return IterBase::n0->array[IterBase::index]; }
        /// get element
        T& operator->() const
        { return IterBase::n0->array[IterBase::index]; }
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
        /// constructor
        ConstIter(const Node0* n0 = nullptr, cxuint index = 0): IterBase{n0, index}
        { }
        
        /// pre-increment
        ConstIter& operator++()
        {
            IterBase::next();
            return *this;
        }
        /// post-increment
        ConstIter operator++(int) const
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
        ConstIter operator--(int) const
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
        { return IterBase::n0->array[IterBase::index]; }
        /// get element
        const T& operator->() const
        { return IterBase::n0->array[IterBase::index]; }
        /// equal to
        bool operator==(const IterBase& it) const
        { return IterBase::n0==it.n0 && IterBase::index==it.index; }
        /// not equal
        bool operator!=(const IterBase& it) const
        { return IterBase::n0!=it.n0 || IterBase::index!=it.index; }
    };
    
private:
    union {
        Node0 n0; // root Node0
        Node1 n1; // root Node1
    };    
public:
    ///
    typedef ConstIter const_iterator;
    typedef Iter iterator;
    typedef T value_type;
    typedef K key_type;
    
    /// default constructor
    DTree(const Comp& comp = Comp(), const KeyOfVal& kofval = KeyOfVal())
        : Comp(comp), KeyOfVal(kofval), n0()
    {
    }
    
    /// constructor with range assignment
    template<typename Iter>
    DTree(Iter first, Iter last, const Comp& comp = Comp(),
          const KeyOfVal& kofval = KeyOfVal()) : Comp(comp), KeyOfVal(kofval), n0()
    { }
    
    /// constructor with initializer list
    template<typename Iter>
    DTree(std::initializer_list<value_type> init, const Comp& comp = Comp(),
          const KeyOfVal& kofval = KeyOfVal()) : Comp(comp), KeyOfVal(kofval), n0()
    { }
    /// copy construcror
    DTree(const DTree& dt) 
    {
        if (dt.n0.type == NODE0)
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
    DTree(DTree&& dt)
    {
        if (dt.n0.type == NODE0)
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
        else
            n1.~Node1();
    }
    
    /// copy assignment
    DTree& operator=(const DTree& dt)
    { return *this; }
    /// move assignment
    DTree& operator=(DTree&& dt)
    { return *this; }
    /// assignment of initilizer list
    DTree& operator=(std::initializer_list<value_type> init)
    { return *this; }
    
    /// return true if empty
    bool empty() const
    { return false; }
    
    /// return size
    size_t size() const
    { return 0; }
    
    /// clear (remove all elements)
    void clear()
    { }
    
    /// insert new element
    std::pair<iterator, bool> insert(const value_type& value)
    { return {}; }
    /// insert new elemnt with iterator hint
    iterator insert(const_iterator hint, const value_type& value)
    { return {}; }
    void insert(std::initializer_list<value_type> ilist)
    { }
    /// put element (insert if doesn't exists or replace)
    std::pair<iterator, bool> put(const value_type& value)
    { return {}; }
    /// replace element with key
    void replace(iterator iter, const value_type& value)
    { }
    /// remove element in postion pointed by iterator
    iterator erase(const_iterator pos)
    { return {}; }
    /// remove elemnet by key
    size_t erase(const key_type& key)
    { return 1; }
    /// find element or return end iterator
    iterator find(const key_type& key)
    { return {}; }
    /// find element or return end iterator
    const_iterator find(const key_type& key) const
    { return {}; }
    
    /// return iterator to first element
    iterator begin()
    { return {}; }
    /// return iterator to first element
    const_iterator begin() const
    { return {}; }
    /// return iterator to first element
    const_iterator cbegin() const
    { return {}; }
    /// return iterator after last element
    iterator end()
    { return {}; }
    /// return iterator after last element
    const_iterator end() const
    { return {}; }
    /// return iterator after last element
    const_iterator cend() const
    { return {}; }
    
    /// first element that not less than key
    iterator lower_bound(const key_type& key)
    { return {}; }
    /// first element that not less than key
    const_iterator lower_bound(const key_type& key) const
    { return {}; }
    /// first element that greater than key
    iterator upper_bound(const key_type& key)
    { return {}; }
    /// first element that greater than key
    const_iterator upper_bound(const key_type& key) const
    { return {}; }
    
    /// lexicograhical equal to
    bool operator==(const DTree& dt) const
    { return false; }
    /// lexicograhical not equal
    bool operator!=(const DTree& dt) const
    { return false; }
    /// lexicograhical less
    bool operator<(const DTree& dt) const
    { return false; }
    /// lexicograhical less or equal
    bool operator<=(const DTree& dt) const
    { return false; }
    /// lexicograhical greater
    bool operator>(const DTree& dt) const
    { return false; }
    /// lexicograhical greater or equal
    bool operator>=(const DTree& dt) const
    { return false; }
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
    /// constructor iterator ranges
    template<typename Iter>
    DTreeSet(Iter first, Iter last, const Comp& comp = Comp()) :
            Impl(first, last, comp)
    { }
    /// constructor with element ranges
    template<typename Iter>
    DTreeSet(std::initializer_list<value_type> init, const Comp& comp = Comp()) 
            : Impl(init, comp)
    { }
};

/// DTree map
template<typename K, typename V, typename Comp = std::less<K> >
class DTreeMap: public DTree<K, std::pair<const K, V>, Comp, SelectFirst<K, V> >
{
private:
    typedef DTree<K, std::pair<const K, V>, Comp, SelectFirst<K, V> > Impl;
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
    template<typename Iter>
    DTreeMap(std::initializer_list<value_type> init, const Comp& comp = Comp()) 
            : Impl(init, comp)
    { }
    
    /// get reference to element pointed by key
    mapped_type& at(const key_type& key)
    { return mapped_type(); }
    /// get reference to element pointed by key
    const mapped_type& at(const key_type& key) const
    { return mapped_type(); }
    /// get reference to element pointed by key (add if key doesn't exists)
    mapped_type& operator[](const key_type& key)
    { return mapped_type(); }
};

};

#endif
