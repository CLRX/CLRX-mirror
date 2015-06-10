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

/// save from/to big endian value
inline void SBEV(uint16_t& r, uint16_t v);
/// save from/to big endian value
inline void SBEV(int16_t& r, int16_t v);

/// save from/to big endian value
inline void SBEV(uint32_t& r, uint32_t v);
/// save from/to big endian value
inline void SBEV(int32_t& r, int32_t v);

/// save from/to big endian value
inline void SBEV(uint64_t& r, uint64_t v);
/// save from/to big endian value
inline void SBEV(int64_t& r, int64_t v);

/// save from/to big endian value
inline void SUBEV(uint16_t& r, uint16_t v);
/// save from/to big endian value
inline void SUBEV(int16_t& r, int16_t v);

/// save from/to big endian value
inline void SUBEV(uint32_t& r, uint32_t v);
/// save from/to big endian value
inline void SUBEV(int32_t& r, int32_t v);

/// save from/to big endian value
inline void SUBEV(uint64_t& r, uint64_t v);
/// save from/to big endian value
inline void SUBEV(int64_t& r, int64_t v);

/// save from/to little endian value
inline void SLEV(uint16_t& r, uint16_t v);
/// save from/to little endian value
inline void SLEV(int16_t& r, int16_t v);

/// save from/to little endian value
inline void SLEV(uint32_t& r, uint32_t v);
/// save from/to little endian value
inline void SLEV(int32_t& r, int32_t v);

/// save from/to little endian value
inline void SLEV(uint64_t& r, uint64_t v);
/// save from/to little endian value
inline void SLEV(int64_t& r, int64_t v);

/// save from/to little endian value
inline void SULEV(uint16_t& r, uint16_t v);
/// save from/to little endian value
inline void SULEV(int16_t& r, int16_t v);

/// save from/to little endian value
inline void SULEV(uint32_t& r, uint32_t v);
/// save from/to little endian value
inline void SULEV(int32_t& r, int32_t v);

/// save from/to little endian value
inline void SULEV(uint64_t& r, uint64_t v);
/// save from/to little endian value
inline void SULEV(int64_t& r, int64_t v);


#ifdef __GNUC__
/// BSWAP 16-bit
inline uint16_t BSWAP16(uint16_t v)
{ return __builtin_bswap16(v); }
/// BSWAP 32-bit
inline uint32_t BSWAP32(uint32_t v)
{ return __builtin_bswap32(v); }
/// BSWAP 64-bit
inline uint64_t BSWAP64(uint64_t v)
{ return __builtin_bswap64(v); }
#else
/// BSWAP 16-bit
inline uint16_t BSWAP16(uint16_t v)
{ return (v>>8)|(v<<8); }
/// BSWAP 32-bit
inline uint32_t BSWAP32(uint32_t v)
{ return (v>>24)|((v&0xff0000)>>8)|((v&0xff00)<<8)|(v<<24); }
/// BSWAP 64-bit
inline uint64_t BSWAP64(uint64_t v)
{ return (v>>56)|((v&0xff000000000000ULL)>>40)|((v&0xff0000000000ULL)>>24)|
    ((v&0xff00000000ULL)>>8)|((v&0xff000000ULL)<<8)|((v&0xff0000ULL)<<24)|
    ((v&0xff00ULL)<<40)|(v<<56); }
#endif

#ifdef HAVE_LITTLE_ENDIAN

/// convert from/to big endian value
inline uint16_t BEV(uint16_t t)
{ return BSWAP16(t); }
/// convert from/to big endian value
inline int16_t BEV(int16_t t)
{ return BSWAP16(t); }

/// convert from/to big endian value
inline uint32_t BEV(uint32_t t)
{ return BSWAP32(t); }
/// convert from/to big endian value
inline int32_t BEV(int32_t t)
{ return BSWAP32(t); }

/// convert from/to big endian value
inline uint64_t BEV(uint64_t t)
{ return BSWAP64(t); }
/// convert from/to big endian value
inline int64_t BEV(int64_t t)
{ return BSWAP64(t); }

/// convert from/to big endian value from unaligned memory
inline uint16_t UBEV(const uint16_t& t)
{ return BSWAP16(t); }
/// convert from/to big endian value from unaligned memory
inline int16_t UBEV(const int16_t& t)
{ return BSWAP16(t); }

/// convert from/to big endian value from unaligned memory
inline uint32_t UBEV(const uint32_t& t)
{ return BSWAP32(t); }
/// convert from/to big endian value from unaligned memory
inline int32_t UBEV(const int32_t& t)
{ return BSWAP32(t); }

#  ifdef HAVE_ARCH_ARM32
/// convert from/to big endian value from unaligned memory
inline uint64_t UBEV(const uint64_t& t)
{ return BSWAP64(
    ((uint64_t(((const uint32_t*)&t)[1]))<<32)|(((const uint32_t*)&t)[0])); }

/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t)
{ return BSWAP64(
    ((int64_t(((const uint32_t*)&t)[1]))<<32)|(((const uint32_t*)&t)[0])); }
#  else
/// convert from/to big endian value from unaligned memory
inline uint64_t UBEV(const uint64_t& t)
{ return BSWAP64(t); }
/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t)
{ return BSWAP64(t); }
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

/// save from/to big endian value
inline void SBEV(uint16_t& r, uint16_t v)
{ r = BSWAP16(v); }
/// save from/to big endian value
inline void SBEV(int16_t& r, int16_t v)
{ r = BSWAP16(v); }

/// save from/to big endian value
inline void SBEV(uint32_t& r, uint32_t v)
{ r = BSWAP32(v); }
/// save from/to big endian value
inline void SBEV(int32_t& r, int32_t v)
{ r = BSWAP32(v); }

/// save from/to big endian value
inline void SBEV(uint64_t& r, uint64_t v)
{ r = BSWAP64(v); }
/// save from/to big endian value
inline void SBEV(int64_t& r, int64_t v)
{ r = BSWAP64(v); }

/// save from/to big endian value
inline void SUBEV(uint16_t& r, uint16_t v)
{ r = BSWAP16(v); }
/// save from/to big endian value
inline void SUBEV(int16_t& r, int16_t v)
{ r = BSWAP16(v); }

/// save from/to big endian value
inline void SUBEV(uint32_t& r, uint32_t v)
{ r = BSWAP32(v); }
/// save from/to big endian value
inline void SUBEV(int32_t& r, int32_t v)
{ r = BSWAP32(v); }

#  ifdef HAVE_ARCH_ARM32
/// save from/to little endian value
inline void SUBEV(uint64_t& r, uint64_t v)
{ ((uint32_t*)&r)[0] = BSWAP32(v>>32); ((uint32_t*)&r)[1] = BSWAP32(v); }
/// save from/to little endian value
inline void SUBEV(int64_t& r, int64_t v)
{ ((uint32_t*)&r)[0] = BSWAP32(v>>32); ((uint32_t*)&r)[1] = BSWAP32(v); }
#  else
/// save from/to little endian value
inline void SUBEV(uint64_t& r, uint64_t v)
{ r = BSWAP64(v); }
/// save from/to little endian value
inline void SUBEV(int64_t& r, int64_t v)
{ r = BSWAP64(v); }
#  endif


/// save from/to little endian value
inline void SLEV(uint16_t& r, uint16_t v)
{ r = v; }
/// save from/to little endian value
inline void SLEV(int16_t& r, int16_t v)
{ r = v; }

/// save from/to little endian value
inline void SLEV(uint32_t& r, uint32_t v)
{ r = v; }
/// save from/to little endian value
inline void SLEV(int32_t& r, int32_t v)
{ r = v; }

/// save from/to little endian value
inline void SLEV(uint64_t& r, uint64_t v)
{ r = v; }
/// save from/to little endian value
inline void SLEV(int64_t& r, int64_t v)
{ r = v; }

/// save from/to little endian value
inline void SULEV(uint16_t& r, uint16_t v)
{ r = v; }
/// save from/to little endian value
inline void SULEV(int16_t& r, int16_t v)
{ r = v; }

/// save from/to little endian value
inline void SULEV(uint32_t& r, uint32_t v)
{ r = v; }
/// save from/to little endian value
inline void SULEV(int32_t& r, int32_t v)
{ r = v; }

#  ifdef HAVE_ARCH_ARM32
/// save from/to little endian value
inline void SULEV(uint64_t& r, uint64_t v)
{ ((uint32_t*)&r)[0] = (uint32_t)v; ((uint32_t*)&r)[1] = v>>32; }
/// save from/to little endian value
inline void SULEV(int64_t& r, int64_t v)
{ ((uint32_t*)&r)[0] = (uint32_t)v; ((uint32_t*)&r)[1] = v>>32; }
#  else
/// save from/to little endian value
inline void SULEV(uint64_t& r, uint64_t v)
{ r = v; }
/// save from/to little endian value
inline void SULEV(int64_t& r, int64_t v)
{ r = v; }
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
{ return ((uint64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1]); }

/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t)
{ return ((int64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1]); }
#  else
/// convert from/to big endian value from unaligned memory
inline uint64_t UBEV(const uint64_t& t)
{ return t; }
/// convert from/to big endian value from unaligned memory
inline int64_t UBEV(const int64_t& t)
{ return t; }
#  endif

/// convert from/to little endian value
inline uint16_t LEV(uint16_t t)
{ return BSWAP16(t); }
/// convert from/to little endian value
inline int16_t LEV(int16_t t)
{ return BSWAP16(t); }

/// convert from/to little endian value
inline uint32_t LEV(uint32_t t)
{ return BSWAP32(t); }
/// convert from/to little endian value
inline int32_t LEV(int32_t t)
{ return BSWAP32(t); }

/// convert from/to little endian value
inline uint64_t LEV(uint64_t t)
{ return BSWAP64(t); }
/// convert from/to little endian value
inline int64_t LEV(int64_t t)
{ return BSWAP64(t); }

/// convert from/to little endian value from unaligned memory
inline uint16_t ULEV(const uint16_t& t)
{ return BSWAP16(t); }
/// convert from/to little endian value from unaligned memory
inline int16_t ULEV(const int16_t& t)
{ return BSWAP16(t); }

/// convert from/to little endian value from unaligned memory
inline uint32_t ULEV(const uint32_t& t)
{ return BSWAP32(t); }
/// convert from/to little endian value from unaligned memory
inline int32_t ULEV(const int32_t& t)
{ return BSWAP32(t); }

#  ifdef HAVE_ARCH_ARM32
/// convert from/to little endian value from unaligned memory
inline uint64_t ULEV(const uint64_t& t)
{ return BSWAP64(
    ((uint64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1])); }

/// convert from/to little endian value from unaligned memory
inline int64_t ULEV(const int64_t& t)
{ return BSWAP64(
    ((int64_t(((const uint32_t*)&t)[0]))<<32)|(((const uint32_t*)&t)[1])); }
#  else
/// convert from/to little endian value from unaligned memory
inline uint64_t ULEV(const uint64_t& t)
{ return BSWAP64(t); }
/// convert from/to little endian value from unaligned memory
inline int64_t ULEV(const int64_t& t)
{ return BSWAP64(t); }
#  endif


/// save from/to big endian value
inline void SBEV(uint16_t& r, uint16_t v)
{ r = v; }
/// save from/to big endian value
inline void SBEV(int16_t& r, int16_t v)
{ r = v; }

/// save from/to big endian value
inline void SBEV(uint32_t& r, uint32_t v)
{ r = v; }
/// save from/to big endian value
inline void SBEV(int32_t& r, int32_t v)
{ r = v; }

/// save from/to big endian value
inline void SBEV(uint64_t& r, uint64_t v)
{ r = v; }
/// save from/to big endian value
inline void SBEV(int64_t& r, int64_t v)
{ r = v; }

/// save from/to big endian value
inline void SUBEV(uint16_t& r, uint16_t v)
{ r = v; }
/// save from/to big endian value
inline void SUBEV(int16_t& r, int16_t v)
{ r = v; }

/// save from/to big endian value
inline void SUBEV(uint32_t& r, uint32_t v)
{ r = v; }
/// save from/to big endian value
inline void SUBEV(int32_t& r, int32_t v)
{ r = v; }

#  ifdef HAVE_ARCH_ARM32
/// save from/to big endian value
inline void SUBEV(uint64_t& r, uint64_t v)
{ ((uint32_t*)&r)[0] = v>>32; ((uint32_t*)&r)[1] = (uint32_t)v; }
/// save from/to big endian value
inline void SUBEV(int64_t& r, int64_t v)
{ ((uint32_t*)&r)[0] = v>>32; ((uint32_t*)&r)[1] = (uint32_t)v; }
#  else
/// save from/to big endian value
inline void SUBEV(uint64_t& r, uint64_t v)
{ r = v; }
/// save from/to big endian value
inline void SUBEV(int64_t& r, int64_t v)
{ r = v; }
#  endif


/// save from/to little endian value
inline void SLEV(uint16_t& r, uint16_t v)
{ r = BSWAP16(v); }
/// save from/to little endian value
inline void SLEV(int16_t& r, int16_t v)
{ r = BSWAP16(v); }

/// save from/to little endian value
inline void SLEV(uint32_t& r, uint32_t v)
{ r = BSWAP32(v); }
/// save from/to little endian value
inline void SLEV(int32_t& r, int32_t v)
{ r = BSWAP32(v); }

/// save from/to little endian value
inline void SLEV(uint64_t& r, uint64_t v)
{ r = BSWAP64(v); }
/// save from/to little endian value
inline void SLEV(int64_t& r, int64_t v)
{ r = BSWAP64(v); }

/// save from/to little endian value
inline void SULEV(uint16_t& r, uint16_t v)
{ r = BSWAP16(v); }
/// save from/to little endian value
inline void SULEV(int16_t& r, int16_t v)
{ r = BSWAP16(v); }

/// save from/to little endian value
inline void SULEV(uint32_t& r, uint32_t v)
{ r = BSWAP32(v); }
/// save from/to little endian value
inline void SULEV(int32_t& r, int32_t v)
{ r = BSWAP32(v); }

#  ifdef HAVE_ARCH_ARM32
/// save from/to little endian value
inline void SULEV(uint64_t& r, uint64_t v)
{ ((uint32_t*)&r)[0] = BSWAP32((uint32_t)v); ((uint32_t*)&r)[1] = BSWAP32(v>>32); }
/// save from/to little endian value
inline void SULEV(int64_t& r, int64_t v)
{ ((uint32_t*)&r)[0] = BSWAP32((uint32_t)v); ((uint32_t*)&r)[1] = BSWAP32(v>>32); }
#  else
/// save from/to little endian value
inline void SULEV(uint64_t& r, uint64_t v)
{ r = BSWAP64(v); }
/// save from/to little endian value
inline void SULEV(int64_t& r, int64_t v)
{ r = BSWAP64(v); }
#  endif

#endif

};

#endif
