/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
/*! \file MemAccess.h
 * \brief inlines for accessing memory words in LittleEndian and unaligned
 */

#ifndef __CLRX_MEMACCESS_H__
#define __CLRX_MEMACCESS_H__

#include <CLRX/Config.h>
#include <cstdint>

namespace CLRX
{

#ifdef HAVE_LITTLE_ENDIAN
static inline uint16_t ULEV(const uint16_t& t)
{ return t; }
static inline int16_t ULEV(const int16_t& t)
{ return t; }

static inline uint32_t ULEV(const uint32_t& t)
{ return t; }
static inline int32_t ULEV(const int32_t& t)
{ return t; }

#  ifdef HAVE_ARCH_ARM32
static inline uint64_t ULEV(const uint64_t& t)
{ return ((uint64_t(((const uint32_t*)&t)[1]))<<32)|(((const uint32_t*)&t)[0]); }

static inline int64_t ULEV(const int64_t& t)
{ return ((int64_t(((const uint32_t*)&t)[1]))<<32)|(((const uint32_t*)&t)[0]); }
#  else
static inline uint64_t ULEV(const uint64_t& t)
{ return t; }
static inline int64_t ULEV(const int64_t& t)
{ return t; }
#  endif
#else // BIG ENDIAN

#  ifdef __GNUC__
static inline uint16_t ULEV(const uint16_t& t)
{ return __builtin_bswap16(t); }
static inline int16_t ULEV(const int16_t& t)
{ return __builtin_bswap16(t); }
static inline uint32_t ULEV(const uint32_t& t)
{ return __builtin_bswap32(t); }
static inline int32_t ULEV(const int32_t& t)
{ return __builtin_bswap32(t); }
static inline uint64_t ULEV(const uint64_t& t)
{ return __builtin_bswap64(t); }
static inline int64_t ULEV(const int64_t& t)
{ return __builtin_bswap64(t); }

#  ifdef HAVE_ARCH_ARM32
static inline uint64_t ULEV(const uint64_t& t)
{ return __builtin_bswap64(
    ((uint64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1])); }

static inline int64_t ULEV(const int64_t& t)
{ return __builtin_bswap64(
    ((int64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1])); }
#  else
static inline uint64_t ULEV(const uint64_t& t)
{ return __builtin_bswap64(t); }
static inline int64_t ULEV(const int64_t& t)
{ return __builtin_bswap64(t); }
#  endif
#  else
#  error "Unsupported"
#  endif

#endif

};

#endif
