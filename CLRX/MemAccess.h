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

/// main namespace
namespace CLRX
{

/// convert from/to big endian value
inline uint16_t BEV(uint16_t t);
/// convert from/to big endian value
inline int16_t BEV(int16_t t);

/// convert from/to big endian value
inline uint32_t BEV(uint32_t t);
/// convert from/to big endian value
inline int32_t BEV(int32_t t);

/// convert from/to big endian value
inline uint64_t BEV(uint64_t t);
/// convert from/to big endian value
inline int64_t BEV(int64_t t);

/// convert from/to big endian value from unaligned memory
inline uint16_t UBEV(const uint16_t& t);
/// convert from/to big endian value from unaligned memory
inline int16_t UBEV(const int16_t& t);

/// convert from/to big endian value from unaligned memory
inline uint32_t UBEV(const uint32_t& t);
/// convert from/to big endian value from unaligned memory
inline int32_t UBEV(const int32_t& t);

/// convert from/to big endian value from unaligned memory
inline uint64_t UBEV(const uint64_t& t);
/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t);

/// convert from/to little endian value
inline uint16_t LEV(uint16_t t);
/// convert from/to little endian value
inline int16_t LEV(int16_t t);

/// convert from/to little endian value
inline uint32_t LEV(uint32_t t);
/// convert from/to little endian value
inline int32_t LEV(int32_t t);

/// convert from/to little endian value
inline uint64_t LEV(uint64_t t);
/// convert from/to little endian value
inline int64_t LEV(int64_t t);

/// convert from/to little endian value from unaligned memory
inline uint16_t ULEV(const uint16_t& t);
/// convert from/to little endian value from unaligned memory
inline int16_t ULEV(const int16_t& t);
/// convert from/to little endian value from unaligned memory
inline uint32_t ULEV(const uint32_t& t);
/// convert from/to little endian value from unaligned memory
inline int32_t ULEV(const int32_t& t);
/// convert from/to little endian value from unaligned memory
inline uint64_t ULEV(const uint64_t& t);
/// convert from/to little endian value from unaligned memory
inline int64_t ULEV(const int64_t& t);

#ifdef HAVE_LITTLE_ENDIAN

#  ifdef __GNUC__
/// convert from/to big endian value
inline uint16_t BEV(uint16_t t)
{ return __builtin_bswap16(t); }
/// convert from/to big endian value
inline int16_t BEV(int16_t t)
{ return __builtin_bswap16(t); }

/// convert from/to big endian value
inline uint32_t BEV(uint32_t t)
{ return __builtin_bswap32(t); }
/// convert from/to big endian value
inline int32_t BEV(int32_t t)
{ return __builtin_bswap32(t); }

/// convert from/to big endian value
inline uint64_t BEV(uint64_t t)
{ return __builtin_bswap64(t); }
/// convert from/to big endian value
inline int64_t BEV(int64_t t)
{ return __builtin_bswap64(t); }

/// convert from/to big endian value from unaligned memory
inline uint16_t UBEV(const uint16_t& t)
{ return __builtin_bswap16(t); }
/// convert from/to big endian value from unaligned memory
inline int16_t UBEV(const int16_t& t)
{ return __builtin_bswap16(t); }

/// convert from/to big endian value from unaligned memory
inline uint32_t UBEV(const uint32_t& t)
{ return __builtin_bswap32(t); }
/// convert from/to big endian value from unaligned memory
inline int32_t UBEV(const int32_t& t)
{ return __builtin_bswap32(t); }

#  ifdef HAVE_ARCH_ARM32
/// convert from/to big endian value from unaligned memory
inline uint64_t UBEV(const uint64_t& t)
{ return __builtin_bswap64(
    ((uint64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1])); }

/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t)
{ return __builtin_bswap64(
    ((int64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1])); }
#  else
/// convert from/to big endian value from unaligned memory
inline uint64_t UBEV(const uint64_t& t)
{ return __builtin_bswap64(t); }
/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t)
{ return __builtin_bswap64(t); }
#  endif
#  else
#  error "Unsupported"
#  endif

/// convert from/to little endian value
inline uint16_t LEV(uint16_t t)
{ return t; }
/// convert from/to little endian value
inline int16_t LEV(int16_t t)
{ return t; }

/// convert from/to little endian value
inline uint32_t LEV(uint32_t t)
{ return t; }
/// convert from/to little endian value
inline int32_t LEV(int32_t t)
{ return t; }

/// convert from/to little endian value
inline uint64_t LEV(uint64_t t)
{ return t; }
/// convert from/to little endian value
inline int64_t LEV(int64_t t)
{ return t; }


/// convert from/to little endian value from unaligned memory
inline uint16_t ULEV(const uint16_t& t)
{ return t; }
/// convert from/to little endian value from unaligned memory
inline int16_t ULEV(const int16_t& t)
{ return t; }

/// convert from/to little endian value from unaligned memory
inline uint32_t ULEV(const uint32_t& t)
{ return t; }
/// convert from/to little endian value from unaligned memory
inline int32_t ULEV(const int32_t& t)
{ return t; }

#  ifdef HAVE_ARCH_ARM32
/// convert from/to little endian value from unaligned memory
inline uint64_t ULEV(const uint64_t& t)
{ return ((uint64_t(((const uint32_t*)&t)[1]))<<32)|(((const uint32_t*)&t)[0]); }

/// convert from/to little endian value from unaligned memory
inline int64_t ULEV(const int64_t& t)
{ return ((int64_t(((const uint32_t*)&t)[1]))<<32)|(((const uint32_t*)&t)[0]); }
#  else
/// convert from/to little endian value from unaligned memory
inline uint64_t ULEV(const uint64_t& t)
{ return t; }
/// convert from/to little endian value from unaligned memory
inline int64_t ULEV(const int64_t& t)
{ return t; }
#  endif
#else // BIG ENDIAN

/// convert from/to big endian value
inline uint16_t BEV(uint16_t t)
{ return t; }
/// convert from/to big endian value
inline int16_t BEV(int16_t t)
{ return t; }

/// convert from/to big endian value
inline uint32_t BEV(uint32_t t)
{ return t; }
/// convert from/to big endian value
inline int32_t BEV(int32_t t)
{ return t; }

/// convert from/to big endian value
inline uint64_t BEV(uint64_t t)
{ return t; }
/// convert from/to big endian value
inline int64_t BEV(int64_t t)
{ return t; }


/// convert from/to big endian value from unaligned memory
inline uint16_t UBEV(const uint16_t& t)
{ return t; }
/// convert from/to big endian value from unaligned memory
inline int16_t UBEV(const int16_t& t)
{ return t; }

/// convert from/to big endian value from unaligned memory
inline uint32_t UBEV(const uint32_t& t)
{ return t; }
/// convert from/to big endian value from unaligned memory
inline int32_t UBEV(const int32_t& t)
{ return t; }

#  ifdef HAVE_ARCH_ARM32
/// convert from/to big endian value from unaligned memory
inline uint64_t UBEV(const uint64_t& t)
{ return ((uint64_t(((const uint32_t*)&t)[1]))<<32)|(((const uint32_t*)&t)[0]); }

/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t)
{ return ((int64_t(((const uint32_t*)&t)[1]))<<32)|(((const uint32_t*)&t)[0]); }
#  else
/// convert from/to big endian value from unaligned memory
inline uint64_t UBEV(const uint64_t& t)
{ return t; }
/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t)
{ return t; }
#  endif

#  ifdef __GNUC__
/// convert from/to little endian value
inline uint16_t LEV(uint16_t t)
{ return __builtin_bswap16(t); }
/// convert from/to little endian value
inline int16_t LEV(int16_t t)
{ return __builtin_bswap16(t); }

/// convert from/to little endian value
inline uint32_t LEV(uint32_t t)
{ return __builtin_bswap32(t); }
/// convert from/to little endian value
inline int32_t LEV(int32_t t)
{ return __builtin_bswap32(t); }

/// convert from/to little endian value
inline uint64_t LEV(uint64_t t)
{ return __builtin_bswap64(t); }
/// convert from/to little endian value
inline int64_t LEV(int64_t t)
{ return __builtin_bswap64(t); }

/// convert from/to little endian value from unaligned memory
inline uint16_t ULEV(const uint16_t& t)
{ return __builtin_bswap16(t); }
/// convert from/to little endian value from unaligned memory
inline int16_t ULEV(const int16_t& t)
{ return __builtin_bswap16(t); }

/// convert from/to little endian value from unaligned memory
inline uint32_t ULEV(const uint32_t& t)
{ return __builtin_bswap32(t); }
/// convert from/to little endian value from unaligned memory
inline int32_t ULEV(const int32_t& t)
{ return __builtin_bswap32(t); }

#  ifdef HAVE_ARCH_ARM32
/// convert from/to little endian value from unaligned memory
inline uint64_t ULEV(const uint64_t& t)
{ return __builtin_bswap64(
    ((uint64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1])); }

/// convert from/to little endian value from unaligned memory
inline int64_t ULEV(const int64_t& t)
{ return __builtin_bswap64(
    ((int64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1])); }
#  else
/// convert from/to little endian value from unaligned memory
inline uint64_t ULEV(const uint64_t& t)
{ return __builtin_bswap64(t); }
/// convert from/to little endian value from unaligned memory
inline int64_t ULEV(const int64_t& t)
{ return __builtin_bswap64(t); }
#  endif
#  else
#  error "Unsupported"
#  endif

#endif

};

#endif
