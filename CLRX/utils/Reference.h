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
/*! \file Reference.h
 * \brief reference and reference pointer
 */

#ifndef __CLRX_REFERENCE_H__
#define __CLRX_REFERENCE_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <atomic>

namespace CLRX
{

/// reference countable object
class RefCountable
{
private:
    std::atomic<size_t> refCount;
public:
    /// constructor
    RefCountable() : refCount(1)
    { }
    virtual ~RefCountable();
    
    /// reference object
    void reference()
    {
        refCount.fetch_add(1);
    }
    
    /// unreference object (and delete object when no references)
    void unreference()
    {
        if (refCount.fetch_sub(1) == 1)
            delete this; /* delete this object */
    }
};

/// reference pointer based on Glibmm refptr
template<typename T>
class RefPtr
{
private:
    T* ptr;
public:
    RefPtr(): ptr(nullptr)
    { }
    
    explicit RefPtr(T* inputPtr) : ptr(inputPtr)
    { }
    
    RefPtr(const RefPtr<T>& refPtr)
        : ptr(refPtr.ptr)
    {
        if (ptr != nullptr)
            ptr->reference();
    }
    
    RefPtr(RefPtr<T>&& refPtr)
        : ptr(refPtr.ptr)
    { refPtr.ptr = nullptr; }
    
    ~RefPtr()
    {
        if (ptr != nullptr)
            ptr->unreference();
    }
    
    RefPtr<T>& operator=(const RefPtr<T>& refPtr)
    {
        if (ptr != nullptr)
            ptr->unreference();
        if (refPtr.ptr != nullptr)
            refPtr.ptr->reference();
        ptr = refPtr.ptr;
    }
    RefPtr<T>& operator=(RefPtr<T>&& refPtr)
    {
        if (ptr != nullptr)
            ptr->unreference();
        ptr = refPtr.ptr;
        refPtr.ptr = nullptr;
    }
    
    bool operator==(const RefPtr<T>& refPtr) const
    { return ptr == refPtr.ptr; }
    bool operator!=(const RefPtr<T>& refPtr) const
    { return ptr != refPtr.ptr; }
    
    operator bool() const
    { return ptr!=nullptr; }
    
    T* operator->() const
    { return ptr; }
    
    void reset()
    {
        if (ptr != nullptr)
            ptr->unreference();
        ptr = nullptr;
    }
    
    void swap(RefPtr<T>& refPtr)
    {
        T* tmp = ptr;
        ptr = refPtr.ptr;
        refPtr.ptr = tmp;
    }
};

};

#endif
