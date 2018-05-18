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
/*! \file Containers.h
 * \brief containers and other utils for other libraries and programs
 */

#ifndef __CLRX_CONTAINERS_H__
#define __CLRX_CONTAINERS_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include <vector>
#include <utility>
#include <unordered_map>
#include <initializer_list>

/// main namespace
namespace CLRX
{

/// an array class
template<typename T>
class Array
{
public:
    typedef T* iterator;    ///< type of iterator
    typedef const T* const_iterator;    ///< type of constant iterator
    typedef T element_type; ///< element type
private:
    T* ptr, *ptrEnd;
public:
    /// empty constructor
    Array(): ptr(nullptr), ptrEnd(nullptr)
    { }
    
    /// construct array of N elements
    explicit Array(size_t N)
    {
        ptr = nullptr;
        if (N != 0)
            ptr = new T[N];
        ptrEnd = ptr+N;
    }
    
    /// construct array of elements in begin and end
    template<typename It>
    Array(It b, It e)
    {
        try
        {
            ptr = nullptr;
            const size_t N = e-b;
            if (N != 0)
                ptr = new T[N];
            ptrEnd = ptr+N;
            std::copy(b, e, ptr);
        }
        catch(...)
        {
            delete[] ptr;
            throw;
        }
    }
    
    /// copy constructor
    Array(const Array& cp)
    {
        try
        {
            ptr = ptrEnd = nullptr;
            const size_t N = cp.size();
            if (N != 0)
                ptr = new T[N];
            ptrEnd = ptr+N;
            std::copy(cp.ptr, cp.ptrEnd, ptr);
        }
        catch(...)
        {
            delete[] ptr;
            throw;
        }
    }
    
    /// move constructor
    Array(Array&& cp) noexcept
    {
        ptr = cp.ptr;
        ptrEnd = cp.ptrEnd;
        cp.ptr = cp.ptrEnd = nullptr;
    }
    
    /// constructor with initializer list
    Array(std::initializer_list<T> list)
    {
        try
        {
            ptr = nullptr;
            const size_t N = list.size();
            if (N != 0)
                ptr = new T[N];
            ptrEnd = ptr+N;
            std::copy(list.begin(), list.end(), ptr);
        }
        catch(...)
        {
            delete[] ptr;
            throw;
        }
    }
    
    /// destructor
    ~Array()
    { delete[] ptr; }
    
    /// copy assignment
    Array& operator=(const Array& cp)
    {
        if (this == &cp)
            return *this;
        assign(cp.begin(), cp.end());
        return *this;
    }
    /// move assignment
    Array& operator=(Array&& cp) noexcept
    {
        if (this == &cp)
            return *this;
        delete[] ptr;
        ptr = cp.ptr;
        ptrEnd = cp.ptrEnd;
        cp.ptr = cp.ptrEnd = nullptr;
        return *this;
    }
    
    /// assignment from initializer list
    Array& operator=(std::initializer_list<T> list)
    {
        assign(list.begin(), list.end());
        return *this;
    }
    
    /// operator of indexing
    const T& operator[] (size_t i) const
    { return ptr[i]; }
    /// operator of indexing
    T& operator[] (size_t i)
    { return ptr[i]; }
    
    /// returns true if empty
    bool empty() const
    { return ptrEnd==ptr; }
    
    /// returns number of elements
    size_t size() const
    { return ptrEnd-ptr; }
    
    /// only allocating space without keeping previous content
    void allocate(size_t N)
    {
        if (N == size())
            return;
        delete[] ptr;
        ptr = nullptr;
        if (N != 0)
            ptr = new T[N];
        ptrEnd = ptr + N;
    }
    
    /// resize space with keeping old content
    void resize(size_t N)
    {
        if (N == size())
            return;
        T* newPtr = nullptr;
        if (N != 0)
            newPtr = new T[N];
        try
        {
            /* move only if move constructor doesn't throw exceptions */
            const size_t toMove = std::min(N, size());
            for (size_t k = 0; k < toMove; k++)
                newPtr[k] = std::move_if_noexcept(ptr[k]);
        }
        catch(...)
        {
            delete[] newPtr;
            throw;
        }
        delete[] ptr;
        ptr = newPtr;
        ptrEnd = ptr + N;
    }
    
    /// clear array
    void clear()
    {
        delete[] ptr;
        ptr = ptrEnd = nullptr;
    }
    
    /// assign from range of iterators
    template<typename It>
    Array& assign(It b, It e)
    {
        const size_t N = e-b;
        if (N != size())
        {
            T* newPtr = nullptr;
            if (N != 0)
                newPtr = new T[N];
            try
            { std::copy(b, e, newPtr); }
            catch(...)
            {
                delete[] newPtr;
                throw;
            }
            delete[] ptr;
            ptr = newPtr;
            ptrEnd = ptr+N;
        }
        else // no size changed only copy
            std::copy(b, e, ptr);
        return *this;
    }
    
    /// get data
    const T* data() const
    { return ptr; }
    /// get data
    T* data()
    { return ptr; }
    
    /// get iterator to first element
    const T* begin() const
    { return ptr; }
    /// get iterator to first element
    T* begin()
    { return ptr; }
    
    /// get iterator to after last element
    const T* end() const
    { return ptrEnd; }
    /// get iterator to after last element
    T* end()
    { return ptrEnd; }
    
    /// get first element
    const T& front() const
    { return *ptr; }
    /// get first element
    T& front()
    { return *ptr; }
    /// get last element
    const T& back() const
    { return ptrEnd[-1]; }
    /// get last element
    T& back()
    { return ptrEnd[-1]; }
    
    /// swap two arrays
    void swap(Array& array)
    {
        std::swap(ptr, array.ptr);
        std::swap(ptrEnd, array.ptrEnd);
    }
};

/// VectorSet
template<typename T>
class VectorSet: public std::vector<T>
{
public:
    /// constructor
    VectorSet()
    { }
    
    /// constructor
    explicit VectorSet(size_t n) : std::vector<T>(n)
    { }
    
    /// constructor
    VectorSet(size_t n, const T& v) : std::vector<T>(n, v)
    { }
    
    /// constructor
    template<typename It>
    VectorSet(It first, It last) : std::vector<T>(first, last)
    { }
    
    /// constructor
    VectorSet(std::initializer_list<T> l): std::vector<T>(l)
    { }
    
    /// insert new value if doesn't exists
    void insertValue(const T& v)
    {
        auto fit = std::find(std::vector<T>::begin(), std::vector<T>::end(), v);
        if (fit == std::vector<T>::end())
            std::vector<T>::push_back(v);
    }
    
    /// erase value if exists
    void eraseValue(const T& v)
    {
        auto fit = std::find(std::vector<T>::begin(), std::vector<T>::end(), v);
        if (fit != std::vector<T>::end())
            std::vector<T>::erase(fit);
    }
    
    /// return true if value present in vector set
    bool hasValue(const T& v) const
    {
        return std::find(std::vector<T>::begin(), std::vector<T>::end(), v) !=
                std::vector<T>::end();
    }
};


/// binary find helper
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 * \param v value to find
 * \return iterator to value or end
 */
template<typename Iter>
Iter binaryFind(Iter begin, Iter end, const
            typename std::iterator_traits<Iter>::value_type& v);

/// binary find helper
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 * \param v value to find
 * \param comp comparator
 * \return iterator to value or end
 */
template<typename Iter, typename Comp =
        std::less<typename std::iterator_traits<Iter>::value_type> >
Iter binaryFind(Iter begin, Iter end, const
        typename std::iterator_traits<Iter>::value_type& v, Comp comp);

/// binary find helper for array-map
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 * \param k key to find
 * \return iterator to value or end
 */
template<typename Iter>
Iter binaryMapFind(Iter begin, Iter end, const 
        typename std::iterator_traits<Iter>::value_type::first_type& k);

/// binary find helper for array-map
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 * \param k key to find
 * \param comp comparator
 * \return iterator to value or end
 */
template<typename Iter, typename Comp =
    std::less<typename std::iterator_traits<Iter>::value_type::first_type> >
Iter binaryMapFind(Iter begin, Iter end, const
        typename std::iterator_traits<Iter>::value_type::first_type& k, Comp comp);

/// map range of iterators
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 */
template<typename Iter>
void mapSort(Iter begin, Iter end);

/// map range of iterators
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 */
template<typename Iter, typename Comp =
    std::less<typename std::iterator_traits<Iter>::value_type::first_type> >
void mapSort(Iter begin, Iter end, Comp comp);

/// binary find
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 * \param v value to find
 * \return iterator to value or end
 */
template<typename Iter>
Iter binaryFind(Iter begin, Iter end,
        const typename std::iterator_traits<Iter>::value_type& v)
{
    auto it = std::lower_bound(begin, end, v);
    if (it == end || v < *it)
        return end;
    return it;
}

/// binary find
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 * \param v value to find
 * \param comp comparator
 * \return iterator to value or end
 */
template<typename Iter, typename Comp>
Iter binaryFind(Iter begin, Iter end,
        const typename std::iterator_traits<Iter>::value_type& v, Comp comp)
{
    auto it = std::lower_bound(begin, end, v, comp);
    if (it == end || comp(v, *it))
        return end;
    return it;
}

/// binary find of map (array)
template<typename Iter>
Iter binaryMapFind(Iter begin, Iter end, const
            typename std::iterator_traits<Iter>::value_type::first_type& k)
{
    typedef typename std::iterator_traits<Iter>::value_type::first_type K;
    typedef typename std::iterator_traits<Iter>::value_type::second_type V;
    auto it = std::lower_bound(begin, end, std::make_pair(k, V()),
               [](const std::pair<K,V>& e1, const std::pair<K,V>& e2) {
        return e1.first < e2.first; });
    if (it == end || k < it->first)
        return end;
    return it;
}

/// binary find of map (array)
template<typename Iter, typename Comp>
Iter binaryMapFind(Iter begin, Iter end, const
        typename std::iterator_traits<Iter>::value_type::first_type& k, Comp comp)
{
    typedef typename std::iterator_traits<Iter>::value_type::first_type K;
    typedef typename std::iterator_traits<Iter>::value_type::second_type V;
    auto it = std::lower_bound(begin, end, std::make_pair(k, V()),
               [&comp](const std::pair<K,V>& e1, const std::pair<K,V>& e2) {
        return comp(e1.first, e2.first); });
    if (it == end || comp(k, it->first))
        return end;
    return it;
}

/// sort map (array)
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 */
template<typename Iter>
void mapSort(Iter begin, Iter end)
{
    typedef typename std::iterator_traits<Iter>::value_type::first_type K;
    typedef typename std::iterator_traits<Iter>::value_type::second_type V;
    std::sort(begin, end, [](const std::pair<K,V>& e1, const std::pair<K,V>& e2) {
        return e1.first < e2.first; });
}

/// sort map (array)
/**
 * \param begin iterator to first element
 * \param end iterator to after last element
 * \param comp comparator
 */
template<typename Iter, typename Comp>
void mapSort(Iter begin, Iter end, Comp comp)
{
    typedef typename std::iterator_traits<Iter>::value_type::first_type K;
    typedef typename std::iterator_traits<Iter>::value_type::second_type V;
    std::sort(begin, end, [&comp](const std::pair<K,V>& e1, const std::pair<K,V>& e2) {
       return comp(e1.first, e2.first); });
}

/** Simple cache **/

/// Simple cache for object. object class should have a weight method
template<typename K, typename V>
class SimpleCache
{
private:
    struct Entry
    {
        size_t sortedPos;
        size_t usage;
        V value;
    };
    
    size_t totalWeight;
    size_t maxWeight;
    
    typedef typename std::unordered_map<K, Entry>::iterator EntryMapIt;
    // sorted entries - sorted by usage
    std::vector<EntryMapIt> sortedEntries;
    std::unordered_map<K, Entry> entryMap;
    
    void updateInSortedEntries(EntryMapIt it)
    {
        const size_t curPos = it->second.sortedPos;
        if (curPos == 0)
            return; // first position
        if (sortedEntries[curPos-1]->second.usage < it->second.usage &&
            (curPos==1 || sortedEntries[curPos-2]->second.usage >= it->second.usage))
        {
            //std::cout << "fast path" << std::endl;
            std::swap(sortedEntries[curPos-1]->second.sortedPos, it->second.sortedPos);
            std::swap(sortedEntries[curPos-1], sortedEntries[curPos]);
            return;
        }
        //std::cout << "slow path" << std::endl;
        auto fit = std::upper_bound(sortedEntries.begin(),
            sortedEntries.begin()+it->second.sortedPos, it,
            [](EntryMapIt it1, EntryMapIt it2)
            { return it1->second.usage > it2->second.usage; });
        if (fit != sortedEntries.begin()+it->second.sortedPos)
        {
            const size_t curPos = it->second.sortedPos;
            std::swap((*fit)->second.sortedPos, it->second.sortedPos);
            std::swap(*fit, sortedEntries[curPos]);
        }
    }
    
    void insertToSortedEntries(EntryMapIt it)
    {
        it->second.sortedPos = sortedEntries.size();
        sortedEntries.push_back(it);
    }
    
    void removeFromSortedEntries(size_t pos)
    {
        // update later element positioning
        for (size_t i = pos+1; i < sortedEntries.size(); i++)
            (sortedEntries[i]->second.sortedPos)--;
        sortedEntries.erase(sortedEntries.begin() + pos);
    }
    
public:
    /// constructor
    explicit SimpleCache(size_t _maxWeight) : totalWeight(0), maxWeight(_maxWeight)
    { }
    
    /// use key - get value
    V* use(const K& key)
    {
        auto it = entryMap.find(key);
        if (it != entryMap.end())
        {
            it->second.usage++;
            updateInSortedEntries(it);
            return &(it->second.value);
        }
        return nullptr;
    }
    
    /// return true if key exists
    bool hasKey(const K& key)
    { return entryMap.find(key) != entryMap.end(); }
    
    /// put value
    void put(const K& key, const V& value)
    {
        auto res = entryMap.insert({ key, Entry{ 0, 0, value } });
        if (!res.second)
        {
            removeFromSortedEntries(res.first->second.sortedPos); // remove old value
            // update value
            totalWeight -= res.first->second.value.weight();
            res.first->second = Entry{ 0, 0, value };
        }
        const size_t elemWeight = value.weight();
        
        // correct max weight if element have greater weight
        if (elemWeight > maxWeight)
            maxWeight = elemWeight<<1;
        
        while (totalWeight+elemWeight > maxWeight)
        {
            // remove min usage element
            auto minUsageIt = sortedEntries.back();
            sortedEntries.pop_back();
            totalWeight -= minUsageIt->second.value.weight();
            entryMap.erase(minUsageIt);
        }
        
        insertToSortedEntries(res.first); // new entry in sorted entries
        
        totalWeight += elemWeight;
    }
};

};

namespace std
{

/// std::swap specialization for CLRX::Array
template<typename T>
void swap(CLRX::Array<T>& a1, CLRX::Array<T>& a2)
{ a1.swap(a2); }

}

#endif
