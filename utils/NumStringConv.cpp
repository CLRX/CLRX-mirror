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

//#define CSTRTOFX_DUMP_IRRESULTS 1

#include <CLRX/Config.h>
#include <algorithm>
#ifdef CSTRTOFX_DUMP_IRRESULTS
#include <iostream>
#include <sstream>
#include <iomanip>
#endif
#include <cstdint>
#include <vector>
#ifdef HAVE_LINUX
#include <alloca.h>
#elif defined(HAVE_MINGW)
#include <malloc.h>
#else
#include <cstdlib>
#endif
#include <climits>
#include <cstddef>
#include <CLRX/utils/Utilities.h>

using namespace CLRX;

// simple function to parse unsigned integer in decimal form
cxuint CLRX::cstrtoui(const char* str, const char* inend, const char*& outend)
{
    uint64_t out = 0;
    const char* p;
    if (inend == str)
    {
        outend = str;
        throw ParseException("No characters to parse");
    }
    
    for (p = str; p != inend && isDigit(*p); p++)
    {
        out = (out*10U) + *p-'0';
        if (out > UINT_MAX)
        {
            outend = p;
            throw ParseException("number out of range");
        }
    }
    if (p == str)
    {
        outend = p;
        throw ParseException("Missing number");
    }
    outend = p;
    return out;
}

uint64_t CLRX::cstrtouXCStyle(const char* str, const char* inend,
             const char*& outend, cxuint bits)
{
    uint64_t out = 0;
    const char* p = 0;
    if (inend == str)
    {
        outend = str;
        throw ParseException("No characters to parse");
    }
    
    if (*str == '0')
    {
        if (inend != str+1 && (str[1] == 'x' || str[1] == 'X'))
        {
            // hex
            if (inend == str+2)
            {
                outend = str+2;
                throw ParseException("Number is too short");
            }
            
            const uint64_t lastHex = (15ULL<<(bits-4));
            for (p = str+2; p != inend; p++)
            {
                cxuint digit;
                if (*p >= '0' && *p <= '9')
                    digit = *p-'0';
                else if (*p >= 'A' && *p <= 'F')
                    digit = *p-'A'+10;
                else if (*p >= 'a' && *p <= 'f')
                    digit = *p-'a'+10;
                else // stop in not hex digit
                    break;
                
                if ((out & lastHex) != 0)
                {
                    outend = p;
                    throw ParseException("Number out of range");
                }
                out = (out<<4) + digit;
            }
            if (p == str+2)
            {
                outend = p;
                throw ParseException("Missing number");
            }
        }
        else if (inend != str+1 && (str[1] == 'b' || str[1] == 'B'))
        {
            // binary
            if (inend == str+2)
            {
                outend = str+2;
                throw ParseException("Number is too short");
            }
            
            const uint64_t lastBit = (1ULL<<(bits-1));
            for (p = str+2; p != inend && (*p == '0' || *p == '1'); p++)
            {
                if ((out & lastBit) != 0)
                {
                    outend = p;
                    throw ParseException("Number out of range");
                }
                out = (out<<1) + (*p-'0');
            }
            if (p == str+2)
            {
                outend = p;
                throw ParseException("Missing number");
            }
        }
        else
        {
            // octal
            const uint64_t lastOct = (7ULL<<(bits-3));
            for (p = str+1; p != inend && isODigit(*p); p++)
            {
                if ((out & lastOct) != 0)
                {
                    outend = p;
                    throw ParseException("Number out of range");
                }
                out = (out<<3) + (*p-'0');
            }
            // if no octal parsed, then zero and correctly treated as decimal zero
        }
    }
    else
    {
        // decimal
        const uint64_t mask = (bits < 64) ? ((1ULL<<bits)-1) :
                    UINT64_MAX; /* fix for shift */
        const uint64_t lastDigitValue = mask/10;
        for (p = str; p != inend && isDigit(*p); p++)
        {
            if (out > lastDigitValue)
            {
                outend = p;
                throw ParseException("Number out of range");
            }
            const cxuint digit = (*p-'0');
            out = out * 10 + digit;
            if ((out&mask) < digit) // if carry
            {
                outend = p;
                throw ParseException("Number out of range");
            }
        }
        if (p == str)
        {
            outend = p;
            throw ParseException("Missing number");
        }
    }
    outend = p;
    return out;
}

// version for signed integers
int64_t CLRX::cstrtoiXCStyle(const char* str, const char* inend,
             const char*& outend, cxuint bits)
{
    const bool negative = (str != inend) && str[0] == '-';
    const bool sign = (str != inend) && (str[0] == '-' || str[0] == '+');
    const uint64_t out = cstrtouXCStyle(str + ((sign)?1:0), inend, outend, bits);
    if (!negative)
    {
        if (out >= (1ULL<<(bits-1)))
            throw ParseException("Number out of range");
        return out;
    }
    else
    {
        if (out > (1ULL<<(bits-1)))
            throw ParseException("Number out of range");
        return -out;
    }
}

/*
 * parses exponent of floating point value
 */
static int32_t parseFloatExponent(const char*& expstr, const char* inend)
{
    bool signOfExp = false; // positive exponent
    if (*expstr == '+' || *expstr == '-')
    {
        signOfExp = (*expstr == '-'); // set sign of exponent
        expstr++;
        if (expstr == inend)
            throw ParseException("End of floating point at exponent");
    }
    cxuint absExponent = 0;
    if (expstr == inend || !isDigit(*expstr))
        throw ParseException("Garbages at floating point exponent");
    for (;expstr != inend && isDigit(*expstr); expstr++)
    {
        if (absExponent > ((1U<<31)/10))
            throw ParseException("Exponent out of range");
        cxuint digit = (*expstr-'0');
        absExponent = absExponent * 10 + digit;
        if ((absExponent&((1U<<31)-1)) < digit && absExponent != (1U<<31))
            // check additional conditions for out of range (carry)
            throw ParseException("Exponent out of range");
    }
    
    if (!signOfExp && absExponent == (1U<<31))
        // if abs exponent with max negative value and not negative
        throw ParseException("Exponent out of range");
    return (signOfExp) ? -absExponent : absExponent;
}

/*
 * SimpleBigNum
 */

static inline void mul64Full(uint64_t a, uint64_t b, uint64_t* c)
{
#ifdef HAVE_INT128
    unsigned __int128 v = ((unsigned __int128)a)*b;
    c[0] = v;
    c[1] = v>>64;
#else
    const uint32_t a0 = a;
    const uint32_t a1 = a>>32;
    const uint32_t b0 = b;
    const uint32_t b1 = b>>32;
    c[0] = uint64_t(a0)*b0;
    const uint64_t m01 = uint64_t(a0)*b1;
    const uint64_t m10 = uint64_t(a1)*b0;
    c[1] = uint64_t(a1)*b1;
    const uint64_t mx = m01 + m10; // (a1*b0)+(a0*b1)
    c[0] += mx<<32;
    c[1] += (c[0] < (mx<<32)); // add carry from previous addition
    // (mx < m10) - carry from m01+m10
    c[1] += (mx>>32) + (uint64_t(mx < m10)<<32);
#endif
}

static inline bool bigAdd(cxuint aSize, uint64_t* biga, const uint64_t* bigb)
{
    bool carry = false;
    for (cxuint i = 0; i < aSize; i++)
    {
        biga[i] += bigb[i] + carry;
        carry = (biga[i] < bigb[i]) || ((biga[i] == bigb[i]) && carry);
    }
    return carry;
}

static inline bool bigAdd(cxuint aSize, uint64_t* biga, cxuint bSize, const uint64_t* bigb)
{
    cxuint carry = 0;
    cxuint minSize = std::min(aSize, bSize);
    cxuint i = 0;
    for (; i < minSize; i++)
    {
        biga[i] += bigb[i] + carry;
        carry = (biga[i] < bigb[i]) || ((biga[i] == bigb[i]) && carry);
    }
    for (; i < aSize; i++)
    {
        biga[i] += carry;
        carry = (biga[i] < carry);
    }
    return carry;
}

static inline bool bigSub(cxuint aSize, uint64_t* biga, cxuint bSize, const uint64_t* bigb)
{
    bool borrow = false;
    cxuint minSize = std::min(aSize, bSize);
    cxuint i = 0;
    for (; i < minSize; i++)
    {
        const uint64_t tmp = biga[i];
        biga[i] -= bigb[i] + borrow;
        borrow = (biga[i] > tmp) || (biga[i] == tmp && borrow);
    }
    for (; i < aSize; i++)
    {
        const uint64_t tmp = biga[i];
        biga[i] -= borrow;
        borrow = (biga[i] > tmp);
    }
    return borrow;
}

/*
 * bigMulSimple
 */
static void bigMulSimple(cxuint asize, const uint64_t* biga, cxuint bsize,
           const uint64_t* bigb, uint64_t* bigc)
{
    uint64_t* tmpMul = static_cast<uint64_t*>(::alloca((bsize+1)<<3));
    std::fill(bigc, bigc + asize + bsize, uint64_t(0));
    for (cxuint i = 0; i < asize; i++)
    {
        uint64_t t[2];
        tmpMul[0] = tmpMul[1] = 0;
        for (cxuint j = 0; j < bsize-1; j++)
        {
            mul64Full(biga[i], bigb[j], t);
            // add to tmp
            tmpMul[j] += t[0];
            bool carry = (tmpMul[j] < t[0]);
            tmpMul[j+1] += t[1] + carry;
            tmpMul[j+2] = (tmpMul[j+1] < t[1]) || (tmpMul[j+1] == t[1] && carry);
        }
        mul64Full(biga[i], bigb[bsize-1], t);
        tmpMul[bsize-1] += t[0];
        bool carry = (tmpMul[bsize-1] < t[0]);
        tmpMul[bsize] += t[1] + carry;
        // add to product
        carry = false;
        for (cxuint j = 0; j < bsize+1; j++)
        {
            bigc[i+j] += tmpMul[j] + carry;
            carry = (bigc[i+j] < tmpMul[j]) || ((bigc[i+j] == tmpMul[j]) && carry);
        }
        if (i+bsize+1 < asize+bsize)
            bigc[i+bsize+1] += carry;
    }
}

static void bigMul(cxuint asize, const uint64_t* biga, cxuint bsize,
           const uint64_t* bigb, uint64_t* bigc)
{
    if (asize == 1 || bsize == 1)
    {
        // if one number is single 64-bit
        uint64_t ae;
        cxuint size;
        const uint64_t* bignum;
        if (asize != 1)
        {
            ae = bigb[0];
            size = asize;
            bignum = biga;
        }
        else
        {
            ae = biga[0];
            size = bsize;
            bignum = bigb;
        }
        // main routine
        bigc[0] = bigc[1] = 0;
        uint64_t t[2];
        for (cxuint i = 0; i < size-1; i++)
        {
            mul64Full(ae, bignum[i], t);
            // add to bigc
            bigc[i] += t[0];
            bool carry = (bigc[i] < t[0]);
            bigc[i+1] += t[1] + carry;
            bigc[i+2] = (bigc[i+1] < t[1]) || (bigc[i+1] == t[1] && carry);
        }
        // last elem
        mul64Full(ae, bignum[size-1], t);
        bigc[size-1] += t[0];
        bool carry = (bigc[size-1] < t[0]);
        bigc[size] += t[1] + carry;
    }
    else
        bigMulSimple(asize, biga, bsize, bigb, bigc);
}

static inline void bigShift64Right(cxuint size, uint64_t* bigNum, cxuint shift64)
{
    const cxuint shift64n = 64-shift64;
    for (cxuint i = 0; i < size-1; i++)
        bigNum[i] = (bigNum[i]>>shift64) | (bigNum[i+1]<<shift64n);
    bigNum[size-1] >>= shift64;
}

static bool bigFPRoundToNearest(cxuint inSize, cxuint outSize, cxint& exponent,
            uint64_t* bigNum)
{
    const cxuint roundSize = inSize-outSize;
    cxuint carry = 0;
    if ((bigNum[roundSize-1] & (1ULL<<63)) != 0)
    {
        /* apply rounding */
        carry = true;
        for (cxuint i = 0; i < outSize; i++)
        {
            bigNum[i] = bigNum[roundSize+i] + carry;
            carry = (bigNum[i] < carry);
        }
    }
    else // if no rounding
        for (cxuint i = 0; i < outSize; i++)
            bigNum[i] = bigNum[roundSize+i];
    
    if (carry)
        exponent++;
    return carry;
}

struct CLRX_INTERNAL Pow5Num128TableEntry
{
    uint64_t value[2];
    int exponent;
};

/* this table used for speeding up computation of power of 5 for the most used cases.
 * in 128-bit format, with exponent */
static const Pow5Num128TableEntry pow5_128Table[129] =
{
    { { 0x7e4731ae8f66c448ULL, 0x50ffd44f4a73d34aULL }, -149 },
    { { 0x1dd8fe1a3340755aULL, 0xa53fc9631d10c81dULL }, -147 },
    { { 0x32a79ed060084959ULL, 0x0747ddddf22a7d12ULL }, -144 },
    { { 0xbf518684780a5bafULL, 0x4919d5556eb51c56ULL }, -142 },
    { { 0x6f25e825960cf29aULL, 0x9b604aaaca62636cULL }, -140 },
    { { 0xc577b1177dc817a0ULL, 0x011c2eaabe7d7e23ULL }, -137 },
    { { 0xb6d59d5d5d3a1d89ULL, 0x41633a556e1cddacULL }, -135 },
    { { 0xe48b04b4b488a4ebULL, 0x91bc08eac9a41517ULL }, -133 },
    { { 0xddadc5e1e1aace25ULL, 0xf62b0b257c0d1a5dULL }, -131 },
    { { 0xaa8c9bad2d0ac0d7ULL, 0x39dae6f76d88307aULL }, -128 },
    { { 0x552fc298784d710dULL, 0x8851a0b548ea3c99ULL }, -126 },
    { { 0xaa7bb33e9660cd50ULL, 0xea6608e29b24cbbfULL }, -124 },
    { { 0xca8d50071dfc8052ULL, 0x327fc58da0f6ff57ULL }, -121 },
    { { 0xbd30a408e57ba067ULL, 0x7f1fb6f10934bf2dULL }, -119 },
    { { 0x2c7ccd0b1eda8881ULL, 0xdee7a4ad4b81eef9ULL }, -117 },
    { { 0xbbce0026f3489550ULL, 0x2b50c6ec4f31355bULL }, -114 },
    { { 0xaac18030b01abaa4ULL, 0x7624f8a762fd82b2ULL }, -112 },
    { { 0x5571e03cdc21694eULL, 0xd3ae36d13bbce35fULL }, -110 },
    { { 0x95672c260994e1d0ULL, 0x244ce242c5560e1bULL }, -107 },
    { { 0x7ac0f72f8bfa1a45ULL, 0x6d601ad376ab91a2ULL }, -105 },
    { { 0x197134fb6ef8a0d6ULL, 0xc8b821885456760bULL }, -103 },
    { { 0xefe6c11d255b6486ULL, 0x1d7314f534b609c6ULL }, -100 },
    { { 0xabe071646eb23da7ULL, 0x64cfda3281e38c38ULL }, -98 },
    { { 0xd6d88dbd8a5ecd11ULL, 0xbe03d0bf225c6f46ULL }, -96 },
    { { 0x46475896767b402aULL, 0x16c262777579c58cULL }, -93 },
    { { 0x57d92ebc141a1035ULL, 0x5c72fb1552d836efULL }, -91 },
    { { 0x2dcf7a6b19209442ULL, 0xb38fb9daa78e44abULL }, -89 },
    { { 0xfca1ac82efb45ca9ULL, 0x1039d428a8b8eaeaULL }, -86 },
    { { 0xbbca17a3aba173d4ULL, 0x54484932d2e725a5ULL }, -84 },
    { { 0x2abc9d8c9689d0c9ULL, 0xa95a5b7f87a0ef0fULL }, -82 },
    { { 0x7ab5e277de16227dULL, 0x09d8792fb4c49569ULL }, -79 },
    { { 0xd9635b15d59bab1dULL, 0x4c4e977ba1f5bac3ULL }, -77 },
    { { 0xcfbc31db4b0295e4ULL, 0x9f623d5a8a732974ULL }, -75 },
    { { 0x01d59f290ee19dafULL, 0x039d66589687f9e9ULL }, -72 },
    { { 0x424b06f3529a051aULL, 0x4484bfeebc29f863ULL }, -70 },
    { { 0x12ddc8b027408661ULL, 0x95a5efea6b34767cULL }, -68 },
    { { 0x17953adc3110a7f9ULL, 0xfb0f6be50601941bULL }, -66 },
    { { 0xeebd44c99eaa68fcULL, 0x3ce9a36f23c0fc90ULL }, -63 },
    { { 0x2a6c95fc0655033aULL, 0x8c240c4aecb13bb5ULL }, -61 },
    { { 0x7507bb7b07ea4409ULL, 0xef2d0f5da7dd8aa2ULL }, -59 },
    { { 0x8924d52ce4f26a86ULL, 0x357c299a88ea76a5ULL }, -56 },
    { { 0xeb6e0a781e2f0527ULL, 0x82db34012b25144eULL }, -54 },
    { { 0xa6498d1625bac671ULL, 0xe392010175ee5962ULL }, -52 },
    { { 0xa7edf82dd794bc07ULL, 0x2e3b40a0e9b4f7ddULL }, -49 },
    { { 0x11e976394d79eb08ULL, 0x79ca10c9242235d5ULL }, -47 },
    { { 0x5663d3c7a0d865caULL, 0xd83c94fb6d2ac34aULL }, -45 },
    { { 0x75fe645cc4873f9eULL, 0x2725dd1d243aba0eULL }, -42 },
    { { 0x137dfd73f5a90f86ULL, 0x70ef54646d496892ULL }, -40 },
    { { 0x985d7cd0f3135367ULL, 0xcd2b297d889bc2b6ULL }, -38 },
    { { 0x1f3a6e0297ec1421ULL, 0x203af9ee756159b2ULL }, -35 },
    { { 0xa70909833de71929ULL, 0x6849b86a12b9b01eULL }, -33 },
    { { 0x50cb4be40d60df73ULL, 0xc25c268497681c26ULL }, -31 },
    { { 0xf27f0f6e885c8ba8ULL, 0x19799812dea11197ULL }, -28 },
    { { 0xef1ed34a2a73ae92ULL, 0x5fd7fe17964955fdULL }, -26 },
    { { 0x6ae6881cb5109a36ULL, 0xb7cdfd9d7bdbab7dULL }, -24 },
    { { 0x62d01511f12a6062ULL, 0x12e0be826d694b2eULL }, -21 },
    { { 0xfb841a566d74f87aULL, 0x5798ee2308c39df9ULL }, -19 },
    { { 0x7a6520ec08d23699ULL, 0xad7f29abcaf48578ULL }, -17 },
    { { 0x4c7f349385836220ULL, 0x0c6f7a0b5ed8d36bULL }, -14 },
    { { 0x1f9f01b866e43aa8ULL, 0x4f8b588e368f0846ULL }, -12 },
    { { 0xa786c226809d4952ULL, 0xa36e2eb1c432ca57ULL }, -10 },
    { { 0xc8b4395810624dd3ULL, 0x0624dd2f1a9fbe76ULL }, -7 },
    { { 0x7ae147ae147ae148ULL, 0x47ae147ae147ae14ULL }, -5 },
    { { 0x999999999999999aULL, 0x9999999999999999ULL }, -3 },
    { { 0x0000000000000000ULL, 0x0000000000000000ULL }, 0 },
    { { 0x0000000000000000ULL, 0x4000000000000000ULL }, 2 },
    { { 0x0000000000000000ULL, 0x9000000000000000ULL }, 4 },
    { { 0x0000000000000000ULL, 0xf400000000000000ULL }, 6 },
    { { 0x0000000000000000ULL, 0x3880000000000000ULL }, 9 },
    { { 0x0000000000000000ULL, 0x86a0000000000000ULL }, 11 },
    { { 0x0000000000000000ULL, 0xe848000000000000ULL }, 13 },
    { { 0x0000000000000000ULL, 0x312d000000000000ULL }, 16 },
    { { 0x0000000000000000ULL, 0x7d78400000000000ULL }, 18 },
    { { 0x0000000000000000ULL, 0xdcd6500000000000ULL }, 20 },
    { { 0x0000000000000000ULL, 0x2a05f20000000000ULL }, 23 },
    { { 0x0000000000000000ULL, 0x74876e8000000000ULL }, 25 },
    { { 0x0000000000000000ULL, 0xd1a94a2000000000ULL }, 27 },
    { { 0x0000000000000000ULL, 0x2309ce5400000000ULL }, 30 },
    { { 0x0000000000000000ULL, 0x6bcc41e900000000ULL }, 32 },
    { { 0x0000000000000000ULL, 0xc6bf526340000000ULL }, 34 },
    { { 0x0000000000000000ULL, 0x1c37937e08000000ULL }, 37 },
    { { 0x0000000000000000ULL, 0x6345785d8a000000ULL }, 39 },
    { { 0x0000000000000000ULL, 0xbc16d674ec800000ULL }, 41 },
    { { 0x0000000000000000ULL, 0x158e460913d00000ULL }, 44 },
    { { 0x0000000000000000ULL, 0x5af1d78b58c40000ULL }, 46 },
    { { 0x0000000000000000ULL, 0xb1ae4d6e2ef50000ULL }, 48 },
    { { 0x0000000000000000ULL, 0x0f0cf064dd592000ULL }, 51 },
    { { 0x0000000000000000ULL, 0x52d02c7e14af6800ULL }, 53 },
    { { 0x0000000000000000ULL, 0xa784379d99db4200ULL }, 55 },
    { { 0x0000000000000000ULL, 0x08b2a2c280290940ULL }, 58 },
    { { 0x0000000000000000ULL, 0x4adf4b7320334b90ULL }, 60 },
    { { 0x0000000000000000ULL, 0x9d971e4fe8401e74ULL }, 62 },
    { { 0x8000000000000000ULL, 0x027e72f1f1281308ULL }, 65 },
    { { 0xa000000000000000ULL, 0x431e0fae6d7217caULL }, 67 },
    { { 0x4800000000000000ULL, 0x93e5939a08ce9dbdULL }, 69 },
    { { 0x9a00000000000000ULL, 0xf8def8808b02452cULL }, 71 },
    { { 0xe040000000000000ULL, 0x3b8b5b5056e16b3bULL }, 74 },
    { { 0xd850000000000000ULL, 0x8a6e32246c99c60aULL }, 76 },
    { { 0x8e64000000000000ULL, 0xed09bead87c0378dULL }, 78 },
    { { 0x78fe800000000000ULL, 0x3426172c74d822b8ULL }, 81 },
    { { 0x973e200000000000ULL, 0x812f9cf7920e2b66ULL }, 83 },
    { { 0x3d0da80000000000ULL, 0xe17b84357691b640ULL }, 85 },
    { { 0x2628890000000000ULL, 0x2ced32a16a1b11e8ULL }, 88 },
    { { 0x2fb2ab4000000000ULL, 0x78287f49c4a1d662ULL }, 90 },
    { { 0xbb9f561000000000ULL, 0xd6329f1c35ca4bfaULL }, 92 },
    { { 0xb54395ca00000000ULL, 0x25dfa371a19e6f7cULL }, 95 },
    { { 0xe2947b3c80000000ULL, 0x6f578c4e0a060b5bULL }, 97 },
    { { 0xdb399a0ba0000000ULL, 0xcb2d6f618c878e32ULL }, 99 },
    { { 0xc904004744000000ULL, 0x1efc659cf7d4b8dfULL }, 102 },
    { { 0xbb45005915000000ULL, 0x66bb7f0435c9e717ULL }, 104 },
    { { 0xaa16406f5a400000ULL, 0xc06a5ec5433c60ddULL }, 106 },
    { { 0x8a4de84598680000ULL, 0x18427b3b4a05bc8aULL }, 109 },
    { { 0x2ce16256fe820000ULL, 0x5e531a0a1c872badULL }, 111 },
    { { 0x7819baecbe228000ULL, 0xb5e7e08ca3a8f698ULL }, 113 },
    { { 0x4b1014d3f6d59000ULL, 0x11b0ec57e6499a1fULL }, 116 },
    { { 0x1dd41a08f48af400ULL, 0x561d276ddfdc00a7ULL }, 118 },
    { { 0xe549208b31adb100ULL, 0xaba4714957d300d0ULL }, 120 },
    { { 0x8f4db456ff0c8ea0ULL, 0x0b46c6cdd6e3e082ULL }, 123 },
    { { 0x3321216cbecfb248ULL, 0x4e1878814c9cd8a3ULL }, 125 },
    { { 0xffe969c7ee839edaULL, 0xa19e96a19fc40ecbULL }, 127 },
    { { 0x7ff1e21cf5124348ULL, 0x05031e2503da893fULL }, 130 },
    { { 0x5fee5aa43256d41aULL, 0x4643e5ae44d12b8fULL }, 132 },
    { { 0x37e9f14d3eec8921ULL, 0x97d4df19d6057673ULL }, 134 },
    { { 0x05e46da08ea7ab69ULL, 0xfdca16e04b86d410ULL }, 136 },
    { { 0x03aec4845928cb22ULL, 0x3e9e4e4c2f34448aULL }, 139 },
    { { 0x849a75a56f72fdeaULL, 0x8e45e1df3b0155acULL }, 141 },
    { { 0xa5c1130ecb4fbd65ULL, 0xf1d75a5709c1ab17ULL }, 143 },
    { { 0xc798abe93f11d65fULL, 0x3726987666190aeeULL }, 146 },
    { { 0x797ed6e38ed64bf7ULL, 0x84f03e93ff9f4daaULL }, 148 }
};

/* multiply two floating point values. xxxExp is binary exponent.
 * xxxxSize - number of dwords, xxxBits - number of used bits */
static void bigMulFP(cxuint maxSize,
        cxuint bigaSize, cxuint bigaBits, cxint bigaExp, const uint64_t* biga,
        cxuint bigbSize, cxuint bigbBits, cxint bigbExp, const uint64_t* bigb,
        cxuint& bigcSize, cxuint& bigcBits, cxint& bigcExp, uint64_t* bigc)
{
    /* A*B */
    bigMul(bigaSize, biga, bigbSize, bigb, bigc);
    /* realize AH*BH+AH*B+BH*A */
    cxuint carry = bigAdd(bigaSize, bigc + bigbSize, biga);
    carry += bigAdd(bigbSize, bigc + bigaSize, bigb);
    // compute size and bits of output
    bigcSize = bigaSize + bigbSize;
    bigcBits = bigaBits + bigbBits;
    bigcExp = bigaExp + bigbExp;
    if (carry != 0)
    {
        // carry, we shift right, and increment exponent
        const bool lsbit = bigc[0]&1;
        bigShift64Right(bigcSize, bigc, 1);
        bigc[bigcSize-1] |= (uint64_t(carry==2)<<63);
        bigcExp++;
        bigcBits++;
        if (bigcBits == (bigcSize<<6)+1 && bigcSize < maxSize)
        {
            // push least signicant bit to first elem
            /* this make sense only when number needs smaller space than maxSize */
            for (cxuint j = bigcSize; j > 0; j--)
                bigc[j] = bigc[j-1];
            bigc[0] = uint64_t(lsbit)<<63;
            bigcSize++;
        }
    }
    if (bigcSize > maxSize)
    {
        /* if out of maxSize, then round this number */
        bigcBits = maxSize<<6;
        bigFPRoundToNearest(bigcSize, maxSize, bigcExp, bigc);
        bigcSize = maxSize;
    }
    else if (bigcSize > ((bigcBits+63)>>6))
    {
        // fix bigc size if doesnt match with bigcBits
        for (cxuint i = 0; i < bigcSize; i++)
            bigc[i] = bigc[i+1];
        bigcSize--;
    }
}

/* generate bit Power of 5.
 * power - power, maxSize - max size of number
 * outSize - size of output number
 * exponent - output number exponent
 * outPow - output number
 * Number in format: (1 + outNumber/2**(dstsize*64)) * 2**exponent
 */
static void bigPow5(cxint power, cxuint maxSize, cxuint& powSize,
            cxint& exponent, uint64_t* outPow)
{
    if ((power >= 0 && power < 28) ||
        (maxSize == 1 && power >= -64 && power <= 64))
    {
        /* get result from table */
        outPow[0] = pow5_128Table[power+64].value[1] +
                (pow5_128Table[power+64].value[0]>>63);
        powSize = 1;
        exponent = pow5_128Table[power+64].exponent;
        return;
    }
    if (maxSize == 2 && power >= -64 && power <= 64)
    {
        /* get result from table */
        outPow[0] = pow5_128Table[power+64].value[0];
        outPow[1] = pow5_128Table[power+64].value[1];
        powSize = 2;
        exponent = pow5_128Table[power+64].exponent;
        return;
    }
    
    maxSize++; // increase by 1 elem (64-bit) for accuracy
    const cxuint absPower = std::abs(power);
    Array<uint64_t> heap = Array<uint64_t>(maxSize<<3); // four (maxSize<<1)
    uint64_t* curPow2Pow = heap.data();
    uint64_t* prevPow2Pow = heap.data() + (maxSize<<1);
    uint64_t* curPow = heap.data() + (maxSize<<1)*2;
    uint64_t* prevPow = heap.data() + (maxSize<<1)*3;
    
    cxuint pow2PowSize, pow2PowBits, powBits;
    cxint pow2PowExp;
    
    cxuint p = 0;
    
    if (power >= 0)
    {
        // positive power
        powSize = 1;
        // no rounding because all power is in single value
        curPow[0] = pow5_128Table[64+(power&15)].value[1];
        exponent = pow5_128Table[64+(power&15)].exponent;
        powBits = exponent;
        pow2PowSize = 1;
        // no rounding because all power is in single value
        curPow2Pow[0] = pow5_128Table[64+16].value[1];
        pow2PowExp = pow5_128Table[64+16].exponent;
        pow2PowBits = pow2PowExp;
        p = 16;
    }
    else
    {
        // negative power (fractions)
        powSize = maxSize;
        // one in curPow
        std::fill(curPow, curPow + maxSize, uint64_t(0));
        exponent = 0;
        pow2PowSize = maxSize;
        powBits = maxSize<<6;
        // perfect 1/5 in pow2PowExp 1/5^1
        std::fill(curPow2Pow+1, curPow2Pow + maxSize, uint64_t(0x9999999999999999ULL));
        curPow2Pow[0] = 0x999999999999999aULL; // includes nearest rounding
        pow2PowExp = -3;
        pow2PowBits = maxSize<<6;
        p = 1;
    }
    // main loop to perform POW in form
    for (; p <= (absPower>>1); p<<=1)
    {
        if ((absPower&p)!=0)
        {
            /* POW = POW2*POW */
            bigMulFP(maxSize, pow2PowSize, pow2PowBits, pow2PowExp, curPow2Pow,
                 powSize, powBits, exponent, curPow,
                 powSize, powBits, exponent, prevPow);
            std::swap(curPow, prevPow);
        }
        
        /* POW2*POW2 */
        bigMulFP(maxSize, pow2PowSize, pow2PowBits, pow2PowExp, curPow2Pow,
                 pow2PowSize, pow2PowBits, pow2PowExp, curPow2Pow,
                 pow2PowSize, pow2PowBits, pow2PowExp, prevPow2Pow);
        std::swap(curPow2Pow, prevPow2Pow);
    }
    // last iteration
    if ((absPower&p)!=0)
    {
        /* POW = POW2*POW */
        bigMulFP(maxSize, pow2PowSize, pow2PowBits, pow2PowExp, curPow2Pow,
             powSize, powBits, exponent, curPow,
             powSize, powBits, exponent, prevPow);
        std::swap(curPow, prevPow);
    }
    // after computing we round to nearest
    if (powSize == maxSize)
    {
        /* round to nearest if result size is greater than original maxSize */
        bigFPRoundToNearest(maxSize, maxSize-1, exponent, curPow);
        powSize = maxSize-1;
    }
    // copy result to output
    std::copy(curPow, curPow + powSize, outPow);
}

/*
 * cstrtofXCStyle
 */

static const int64_t LOG2BYLOG10_32 = 1292913987LL;
static const int64_t LOG10BYLOG2_32 = 14267572528LL;

static inline cxint log2ByLog10Floor(cxint v)
{ return (int64_t(v)*LOG2BYLOG10_32)>>32; }

static inline cxint log2ByLog10Round(cxint v)
{ return (int64_t(v)*LOG2BYLOG10_32 + (1LL<<31))>>32; }

static inline cxint log2ByLog10Ceil(cxint v)
{ return (int64_t(v)*LOG2BYLOG10_32 + (1LL<<32)-1)>>32; }

static inline cxint log10ByLog2Floor(cxint v)
{ return (int64_t(v)*LOG10BYLOG2_32)>>32; }

static inline cxint log10ByLog2Ceil(cxint v)
{ return (int64_t(v)*LOG10BYLOG2_32 + (1LL<<32)-1)>>32; }

/// power of 10
static const uint64_t power10sTable[20] =
{
    1ULL,
    10ULL,
    100ULL,
    1000ULL,
    10000ULL,
    100000ULL,
    1000000ULL,
    10000000ULL,
    100000000ULL,
    1000000000ULL,
    10000000000ULL,
    100000000000ULL,
    1000000000000ULL,
    10000000000000ULL,
    100000000000000ULL,
    1000000000000000ULL,
    10000000000000000ULL,
    100000000000000000ULL,
    1000000000000000000ULL,
    10000000000000000000ULL
};

#ifdef CSTRTOFX_DUMP_IRRESULTS
static void dumpIntermediateResults(cxuint bigSize, const uint64_t* bigValue,
        const uint64_t* bigRescaled, cxint binaryExp, cxint powerof5, cxuint maxDigits,
        cxuint rescaledValueBits, cxuint processedDigits, cxint mantSignifBits)
{
    std::ostringstream oss;
    oss << "DEBUG: Dump of InterResults for cstrtof:\n";
    oss << "PowerOf5: " << powerof5 << ", binaryExp: " << binaryExp <<
            ", maxDigits: " << maxDigits << ", bigSize: " << bigSize <<
            "\nrvBits: " << rescaledValueBits <<
            ", processedDigits: " << processedDigits <<
            ", mantSignifBits: " << mantSignifBits << "\n";
    oss << "BigValue: ";
    for (cxuint i = 0; i < bigSize; i++)
        oss << std::hex << std::setw(16) << std::setfill('0') << bigValue[bigSize-i-1];
    oss << "\nBigRescaled: ";
    for (cxuint i = 0; i < bigSize; i++)
        oss << std::hex << std::setw(16) << std::setfill('0') << bigRescaled[bigSize-i-1];
    oss << "\n";
    oss.flush();
    std::cout << oss.str() << std::endl;
}
#endif

uint64_t CLRX::cstrtofXCStyle(const char* str, const char* inend,
             const char*& outend, cxuint expBits, cxuint mantisaBits)
{
    uint64_t out = 0;
    const char* p = 0;
    bool signOfValue = false;
    
    if (inend == str)
    {
        outend = str;
        throw ParseException("No characters to parse");
    }
    
    p = str;
    if (p+1 != inend && (*p == '+' || *p == '-'))
    {
        signOfValue = (*p == '-'); // true if negative
        p++;
    }
    
    // check for nan or inf
    if (signOfValue) // sign
        out = (1ULL<<(expBits + mantisaBits));
    if (p != inend && p+1 != inend && p+2 != inend)
    {
        if ((p[0] == 'n' || p[0] == 'N') && (p[1] == 'a' || p[1] == 'A') &&
            (p[2] == 'n' || p[2] == 'N'))
        {
            // create positive nan
            out = (((1ULL<<expBits)-1ULL)<<mantisaBits) | (1ULL<<(mantisaBits-1));
            outend = p+3;
            return out;
        }
        else if ((p[0] == 'i' || p[0] == 'I') && (p[1] == 'n' || p[1] == 'N') &&
            (p[2] == 'f' || p[2] == 'F'))
        {
            // create +/- infinity
            out |= ((1ULL<<expBits)-1ULL)<<mantisaBits;
            outend = p+3;
            return out;
        }
        // if not we parse again 
    }
    
    const int minExpNonDenorm = -((1U<<(expBits-1))-2);
    const int minExpDenorm = (minExpNonDenorm-mantisaBits);
    const int maxExp = (1U<<(expBits-1))-1;
    
    if (p != inend && p+1 != inend && *p == '0' && (p[1] == 'x' || p[1] == 'X'))
    {
        // in hex format
        p+=2;
        cxint binaryExp = 0;
        const char* expstr = p;
        bool comma = false;
        for (;expstr != inend && (*expstr == '.' || (*expstr >= '0' && *expstr <= '9') ||
                (*expstr >= 'a' && *expstr <= 'f') ||
                (*expstr >= 'A' && *expstr <= 'F')); expstr++)
            if (*expstr == '.')
            {
                // if comma found and we found next we break skipping
                if (comma) break;
                comma = true;
            }
            
        if (p == expstr || (p+1 == expstr && *p == '.'))
        {
            outend = expstr;
            throw ParseException("Floating point doesn't have value part!");
        }
        // value end in string
        const char* valEnd = expstr;
        
        if (expstr != inend && (*expstr == 'p' || *expstr == 'P')) // we found exponent
        {
            expstr++;
            outend = expstr;
            if (expstr == inend)
                throw ParseException("End of floating point at exponent");
            binaryExp = parseFloatExponent(outend, inend);
        }
        else
            outend = expstr;
        
        // determine real exponent (exponent of value)
        cxint expOfValue = 0;
        while (p != valEnd && *p == '0') p++; // skip zeroes
        
        const char* vs = nullptr;
        cxuint firstDigitBits = 0;
        if (p != valEnd)
        {
            if (*p != '.') // if integer part
            {
                firstDigitBits = (*p >= '8') ? 4 : (*p >= '4') ? 3 : (*p >= '2') ? 2 : 1;
                expOfValue = firstDigitBits-1;
                // count exponent of integer part
                vs = p; // set pointer to real value
                for (p++; p != valEnd && *p != '.'; p++)
                    expOfValue += 4;
            }
            else // if fraction
            {
                // count exponent of fractional part
                for (p++; p != valEnd && *p == '0'; p++)
                    expOfValue -= 4; // skip zeroes
                if (p != valEnd)
                {
                    firstDigitBits = (*p >= '8') ? 4 : (*p >= '4') ? 3 :
                            (*p >= '2') ? 2 : 1;
                    expOfValue += (firstDigitBits-1) - 4;
                    vs = p; // set pointer to real value
                }
            }
        }
        
        if (vs == nullptr || vs == valEnd)
            return out;   // return zero
        
        const int64_t tempExp = int64_t(expOfValue)+int64_t(binaryExp);
        // handling exponent range
        if (tempExp > maxExp) // out of max exponent
            throw ParseException("Absolute value of number is too big");
        if (tempExp < minExpDenorm-1)
            return out; // return zero
        
        // get significant bits. if number normalized adds one-integer (1.) bit.
        // also add rounding bit
        const cxuint significantBits = (tempExp >= minExpNonDenorm) ? mantisaBits+2 :
            tempExp-minExpDenorm+2;
        uint64_t fvalue = 0;
        // parse mantisa
        cxuint parsedBits = 0;
        for (; vs != valEnd && fvalue < (1ULL<<significantBits); vs++)
        {
            cxuint digit = 0;
            if (*vs >= '0' && *vs <= '9')
                digit = *vs - '0';
            else if (*vs >= 'a' && *vs <= 'f')
                digit = *vs - 'a' + 10;
            else if (*vs >= 'A' && *vs <= 'F')
                digit = *vs - 'A' + 10;
            else //if (*vs == '.')
                continue; // skip comma
            fvalue = (fvalue<<4) + digit;
            parsedBits += 4;
        }
        /* parsedBits - bits of parsed value. parsedDigits*4 - 4 + firstDigitBits+1 */
        parsedBits = parsedBits - 4 + firstDigitBits;
        // compute required bits for fvalue
        cxuint requiredBits = significantBits;
        if (requiredBits > parsedBits)
            fvalue <<= requiredBits-parsedBits;
        else // parsed more bits than required
            requiredBits = parsedBits;
        
        // check for exact half (rounding)
        const cxuint nativeFPShift = (requiredBits-significantBits+1);
        const uint64_t roundBit = (1ULL<<(nativeFPShift-1));
        
        // convert to native FP mantisa and native FP exponent
        uint64_t fpMantisa = (fvalue >> nativeFPShift) & ((1ULL<<mantisaBits)-1ULL);
        cxuint fpExponent = (tempExp >= minExpNonDenorm) ? tempExp+(1U<<(expBits-1))-1 : 0;
        bool addRoundings = false;
        
        if ((fvalue & roundBit) != 0)
        {
            // rounding to nearest even
            bool isSecondHalf = true;
            if ((fvalue & (roundBit-1ULL)) != 0)
                isSecondHalf = false;
            
            if (isSecondHalf && (fpMantisa&1)==0)
            {
                // check further digits if still is half and value is even
                for (; vs != valEnd; vs++)
                {
                    // digits character has been already checked, we check that is not zero
                    if (*vs == '.')
                        continue; // skip comma
                    else if (*vs > '0')
                    {
                        // if not zero (is half of value)
                        isSecondHalf = false;
                        break;
                    }
                }
            }
            
            /* is greater than half or value is odd */
            addRoundings = (!isSecondHalf || (fpMantisa&1)!=0);
        }
        
        if (addRoundings)
        {
            // add roundings
            fpMantisa++;
            // check promotion to next exponent
            if (fpMantisa >= (1ULL<<mantisaBits))
            {
                fpExponent++;
                if (fpExponent == ((1U<<expBits)-1)) // overflow!!!
                    throw ParseException("Absolute value of number is too big");
                fpMantisa = 0; // zeroing value
            }
        }
        out |= fpMantisa | (uint64_t(fpExponent)<<mantisaBits);
    }
    else
    {
        // in decimal format
        cxint decimalExp = 0;
        const char* expstr = p;
        bool comma = false;
        for (;expstr != inend && (*expstr == '.' || isDigit(*expstr));
             expstr++)
            if (*expstr == '.')
            {
                // if comma found and we found next we break skipping
                if (comma) break;
                comma = true;
            }
        
        if (p == expstr || (p+1 == expstr && *p == '.'))
        {
            outend = expstr;
            throw ParseException("Floating point doesn't have value part!");
        }
        // value end in string
        const char* valEnd = expstr;
        
        if (expstr != inend && (*expstr == 'e' || *expstr == 'E')) // we found exponent
        {
            expstr++;
            outend = expstr; // set out end
            if (expstr == inend)
                throw ParseException("End of floating point at exponent");
            decimalExp = parseFloatExponent(outend, inend);
        }
        else
            outend = expstr;
        
        // determine real exponent
        cxint decExpOfValue = 0;
        const char* vs = nullptr;
        
        while (p != valEnd && *p == '0') p++; // skip zeroes
        
        if (p != valEnd)
        {
            if (*p != '.') // if integer part
            {
                // count exponent of integer part
                vs = p; // set pointer to real value
                for (p++; p != valEnd && *p != '.'; p++)
                    decExpOfValue++;
            }
            else
            {
                // count exponent of fractional part
                decExpOfValue = -1;
                for (p++; p != valEnd && *p == '0'; p++)
                    decExpOfValue--; // skip zeroes
                vs = p;
            }
        }
        
        if (vs == nullptr || vs == valEnd)
            return out;   // return zero
        
        const int64_t decTempExp = int64_t(decExpOfValue)+int64_t(decimalExp);
        // handling exponent range
        if (decTempExp > log2ByLog10Ceil(maxExp)) // out of max exponent
            throw ParseException("Absolute value of number is too big");
        if (decTempExp < log2ByLog10Floor(minExpDenorm-1))
            return out; // return zero
        /*
         * first trial with 64-bit precision
         */
        uint64_t value = 0;
        uint64_t rescaledValue;
        cxuint processedDigits = 0;
        for (processedDigits = 0; vs != valEnd && processedDigits < 19; vs++)
        {
            if (isDigit(*vs))
                value = value*10 + *vs-'0';
            else // if (*vs == '.')
                continue;
            processedDigits++;
        }
        if (processedDigits < 19)
        {
            /* align to 19 digits */
            value *= power10sTable[19-processedDigits];
            processedDigits = 19;
        }
        
        uint64_t decFactor;
        cxint decFacBinExp;
        cxuint powSize;
        cxuint rescaledValueBits;
        cxint powerof5 = decTempExp-processedDigits+1;
        bigPow5(powerof5, 1, powSize, decFacBinExp, &decFactor);
        
        {
            /* rescale value to binary exponent */
            uint64_t rescaled[2];
            if (decFacBinExp != 0)
                mul64Full(decFactor, value, rescaled);
            else //
                rescaled[0] = rescaled[1] = 0;
            // addition for integer part of decFactor
            rescaled[1] += value;
            bool rvCarry = (rescaled[1] < value);
            // rounding
            if ((rescaled[0] & (1ULL<<63)) != 0)
            {
                rescaled[1]++;
                rvCarry = rvCarry || (rescaled[1] == 0);
            }
            rescaledValue = rescaled[1];
            if (!rvCarry)
            {
                /* compute rescaledValueBits */
                rescaledValueBits = 63 - CLZ64(rescaledValue);
                // remove integer part (lastbit) from rescaled value
                rescaledValue &= (1ULL<<rescaledValueBits)-1ULL;
            }
            else
                rescaledValueBits = 64;
        }
        
        // compute binary exponent
        cxint binaryExp = decFacBinExp + powerof5 + rescaledValueBits;
        if (binaryExp > maxExp) // out of max exponent
            throw ParseException("Absolute value of number is too big");
        if (binaryExp < minExpDenorm-2)
            return out; // return zero
                
        cxint mantSignifBits = (binaryExp >= minExpNonDenorm) ? mantisaBits :
                binaryExp-minExpDenorm;
        bool isNotTooExact = false;
        //std::cout << "mantSignifBits: " << mantSignifBits << std::endl;
        const cxuint subValueShift = rescaledValueBits - mantSignifBits;
        const uint64_t subValue = (subValueShift<64)?
                rescaledValue&((1ULL<<(subValueShift))-1ULL):rescaledValue;
        const uint64_t half = (subValueShift<65)?(1ULL<<(subValueShift-1)):0;
#ifdef CSTRTOFX_DUMP_IRRESULTS
        {
            std::ostringstream oss;
            oss << "SubValue: " << std::hex << subValue << ", Half: " <<
                std::hex << half << ", rvBits: " << std::dec << rescaledValueBits <<
                ", pow5: " << powerof5 << ", mantSignBits: " << mantSignifBits;
            oss.flush();
            std::cout << oss.str() << std::endl;
        }
#endif
        
        /* check if value is too close to half of value, if yes we going to next trials
         * too close value between HALF-3 and HALF+1 (3 because we expects value from
         * next digit */
        if (mantSignifBits >= 0)
            isNotTooExact = (subValue >= half-3ULL && subValue <= half+1ULL);
        else if (mantSignifBits == -1) // if half of smallest denormal
            isNotTooExact = (subValue <= 1ULL);
        else if (mantSignifBits == -2) // if half of smallest denormal
            isNotTooExact = (subValue >= (half>>1)-3ULL);
        // otherwise isNotExact is false if value too small
        
        bool addRoundings = false;
        uint64_t fpMantisa;
        cxuint fpExponent = (binaryExp >= minExpNonDenorm) ?
                binaryExp+(1U<<(expBits-1))-1 : 0;
        
        if (!isNotTooExact)
        {
            // value is exact (not too close half
            if (mantSignifBits>=0)
            {
                addRoundings = (subValue >= half);
                fpMantisa = (rescaledValue>>subValueShift)&((1ULL<<mantisaBits)-1ULL);
                if (fpExponent == 0) // add one for denormalized value
                    fpMantisa |= 1ULL<<mantSignifBits;
            }
            else // if half of smallest denormal
            {
                addRoundings = (mantSignifBits == -1);
                fpMantisa = 0;
            }
        }
        else
        {
            // max digits to compute value
            cxuint maxDigits;
            if (binaryExp-cxint(mantisaBits)-1 >= 0)
                maxDigits = decTempExp+1; // when rounding bit of mantisa is not fraction
            else if (binaryExp >= 0)
                // if rounding bit is fraction but value have integer part
                maxDigits = std::max(1,log2ByLog10Ceil(binaryExp)) -
                         // negative power of rounding bit
                        (binaryExp-mantSignifBits-1)+1;
            else // if all value is fractional value
                maxDigits = -(binaryExp-mantSignifBits-1) -
                    log2ByLog10Floor(-binaryExp)+1;
            maxDigits = std::max(maxDigits, processedDigits);
            
            const cxuint maxBigSize = (log10ByLog2Ceil(maxDigits+3)+63)>>6;
            // using vector for prevents memory leaks (function can throw exception)
            Array<uint64_t> heap = Array<uint64_t>(maxBigSize*5 + 5);
            uint64_t* bigDecFactor = heap.data();
            uint64_t* curBigValue = heap.data()+maxBigSize;
            uint64_t* prevBigValue = heap.data()+maxBigSize*2 + 2;
            uint64_t* bigRescaled = heap.data()+maxBigSize*3 + 4;
            
            curBigValue[0] = value;
            bigDecFactor[0] = decFactor;
            bigRescaled[0] = 0;
            bigRescaled[1] = rescaledValue;
            
            cxuint bigSize = 2;
            cxuint bigValueSize = 1;
            bool isSecondHalf = false; // initialize for compiler warning
            bool isHalfEqual = false; // initialize for compiler warning
            
            /* next trials with higher precision */
            /* parsing is divided by 5-dword packets, to speeding up.
             * already parsed packed will be added to main value */
            while (isNotTooExact && processedDigits < maxDigits+3)
            {
                /* parse digits and put to bigValue */
                const cxuint digitsToProcess = std::min(int(maxDigits),
                        log2ByLog10Floor(bigSize<<6));
                
                /// loop that parses packets and adds to main value
                while (processedDigits < digitsToProcess)
                {
                    // digit pack is 4-dword packet of part of decimal value
                    uint64_t digitPack[4];
                    // packTens holds temporary powers of 10 for current digitPack
                    uint64_t packTens[4];
                    digitPack[0] = digitPack[1] = digitPack[2] = digitPack[3] = 0;
                    packTens[0] = packTens[1] = packTens[2] = packTens[3] = 1;
                    
                    cxuint digitPacksNum = 0;
                    cxuint digitsOfPack = 0;
                    while (digitPacksNum < 4 && processedDigits < digitsToProcess)
                    {
                        cxuint digitsOfPart = 0; // number of parsed digits for dword
                        uint64_t curValue = 0;
                        for (; vs != valEnd && digitsOfPart < 19 &&
                            processedDigits < digitsToProcess; vs++)
                        {
                            if (isDigit(*vs))
                                curValue = curValue*10 + *vs-'0';
                            else // if (*vs == '.')
                                continue;
                            processedDigits++;
                            digitsOfPart++;
                        }
                        if (vs == valEnd)
                        {
                            // fill first digits with zeroes, for align to dword value
                            const cxuint pow10 = std::min(digitsToProcess-processedDigits,
                                        19-digitsOfPart);
                            curValue = curValue*power10sTable[pow10];
                            processedDigits += pow10;
                            digitsOfPart += pow10;
                        }
                        uint64_t tmpPack[5];
                        // put to digitPack: make place for next value, and value
                        bigMul(1, &power10sTable[digitsOfPart], 4, digitPack, tmpPack);
                        digitPack[0] = tmpPack[0];
                        digitPack[1] = tmpPack[1];
                        digitPack[2] = tmpPack[2];
                        digitPack[3] = tmpPack[3];
                        bigAdd(4, digitPack, 1, &curValue);
                        digitsOfPack += digitsOfPart;
                        packTens[digitPacksNum++] = power10sTable[digitsOfPart];
                    }
                    // generate power of tens for digitPack from packTens
                    uint64_t packPowerOfTen[4];
                    if (digitPacksNum > 2)
                    {
                        uint64_t tmpProd[4];
                        mul64Full(packTens[0], packTens[1], tmpProd);
                        mul64Full(packTens[2], packTens[3], tmpProd+2);
                        bigMulSimple(2, tmpProd, 2, tmpProd+2, packPowerOfTen);
                    }
                    else
                    {
                        mul64Full(packTens[0], packTens[1], packPowerOfTen);
                        packPowerOfTen[2] = packPowerOfTen[3] = 0;
                    }
                    cxuint digitPackSize = 4;
                    if (packPowerOfTen[3] == 0)
                    {
                        if (packPowerOfTen[2] == 0)
                            digitPackSize = (packPowerOfTen[1] != 0)?2:1;
                        else //
                            digitPackSize = 3;
                    }
                    // put to bigValue, multiply by power of 10 by processed digits number
                    bigMul(digitPackSize, packPowerOfTen, bigValueSize,
                           curBigValue, prevBigValue);
                    bigValueSize += digitPackSize;
                    prevBigValue[bigValueSize] = 0; // zeroing after addition
                    // finally add value
                    bigAdd(bigValueSize+1, prevBigValue, digitPackSize, digitPack);
                    if (prevBigValue[bigValueSize] == 0)
                    {
                        if (prevBigValue[bigValueSize-1] == 0)
                            bigValueSize--; // if last elem is zero
                    }
                    else // if carry!!!
                        bigValueSize++;
                    std::swap(prevBigValue, curBigValue);
                }
                
                if (digitsToProcess == maxDigits)
                {
                    // adds three digits extra in last iteration
                    // because threshold is needed for half equality detection
                    bigMul(1, power10sTable+3, bigValueSize, curBigValue, prevBigValue);
                    if (prevBigValue[bigValueSize]!=0)
                        bigValueSize++;
                    processedDigits += 3;
                    std::swap(prevBigValue, curBigValue);
                }
                
                // compute power of 5
                powerof5 = decTempExp-processedDigits+1;
                bigPow5(powerof5, bigValueSize, powSize, decFacBinExp, bigDecFactor);
                
                // rescale value
                if (decFacBinExp != 0) // if not 1 in bigDecFactor
                    bigMul(bigValueSize, curBigValue, powSize, bigDecFactor, bigRescaled);
                else // if one in bigDecFactor
                    std::fill(bigRescaled, bigRescaled + bigValueSize+powSize, uint64_t(0));
                
                bool rvCarry = bigAdd(bigValueSize, bigRescaled + powSize, curBigValue);
                /* round rescaled value */
                if ((bigRescaled[powSize-1]&(1ULL<<63)) != 0)
                {
                    cxuint carry = 1;
                    for (cxuint k = powSize; k < powSize+bigValueSize; k++)
                    {
                        bigRescaled[k] += carry;
                        carry = (bigRescaled[k] < carry);
                    }
                    rvCarry = rvCarry || carry;
                }
                if (!rvCarry)
                {
                    /* compute rescaledValueBits */
                    rescaledValueBits = (bigValueSize<<6) - 1 - CLZ64(
                        bigRescaled[powSize+bigValueSize-1]);
                    // remove integer part (lastbit) from rescaled value
                    bigRescaled[powSize+bigValueSize-1] &=
                            (1ULL<<(rescaledValueBits&63))-1ULL;
                }
                else
                    rescaledValueBits = bigValueSize<<6;
                
                // compute binary exponent
                binaryExp = decFacBinExp + powerof5 + rescaledValueBits;
                if (binaryExp < minExpDenorm-2)
                    return out; // return zero
                
                mantSignifBits = (binaryExp >= minExpNonDenorm) ? mantisaBits :
                        binaryExp-minExpDenorm;
#ifdef CSTRTOFX_DUMP_IRRESULTS
                dumpIntermediateResults(bigValueSize, curBigValue, bigRescaled+powSize,
                        binaryExp, powerof5, maxDigits, rescaledValueBits, processedDigits,
                        mantSignifBits);
#endif
                
                // determine whether is nearly half
                const cxuint halfValueShift = (rescaledValueBits - mantSignifBits-1)&63;
                const cxuint halfValuePos = (rescaledValueBits - mantSignifBits-1)>>6;
                const uint64_t halfValue = (1ULL<<halfValueShift);
                
                if (mantSignifBits < 0)
                {
                    // smaller than smallest denormal, add half
                    bigRescaled[powSize+bigValueSize] = 0;
                    // add one to value
                    if (rescaledValueBits < (bigValueSize<<6))
                        bigRescaled[powSize+bigValueSize-1] |=
                                (1ULL<<(rescaledValueBits&63));
                    else // add carry if rescaledValueBits==bigValueSize<<6
                        bigRescaled[powSize+bigValueSize] |= 1;
                }
                
                /* check if value is too close to half of value, if yes we going to next
                 * trials. too close value between HALF-3 and HALF+1
                 * (3 because we expects value from next digit) */
                isHalfEqual = false;
                isNotTooExact = false;
                isSecondHalf = ((bigRescaled[powSize+halfValuePos]&halfValue) != 0);
                if (isSecondHalf)
                {
                    if (halfValuePos == 0 && ((bigRescaled[powSize] & (halfValue-1)) <= 1))
                        isHalfEqual = isNotTooExact = true;
                    else if (halfValuePos != 0 &&
                        (bigRescaled[powSize+halfValuePos] & (halfValue-1)) == 0)
                    {
                        bool zeroes = true;
                        for (cxuint k = 1; k < halfValuePos; k++)
                            if (bigRescaled[powSize+k] != 0)
                            { zeroes = false; break; }
                        isNotTooExact = isHalfEqual =
                            (zeroes && (bigRescaled[powSize] <= 1));
                    }
                }
                else
                {
                    // is first half
                    if (halfValuePos == 0)
                    {
                        isHalfEqual = ((bigRescaled[powSize] & (halfValue-1)) >=
                                halfValue-1);
                        isNotTooExact = ((bigRescaled[powSize] & (halfValue-1)) >=
                                halfValue-3);
                    }
                    else if (halfValuePos != 0 &&
                            (bigRescaled[powSize+halfValuePos] & (halfValue-1)) ==
                            (halfValue-1))
                    {
                        bool ones = true;
                        for (cxuint k = 1; k < halfValuePos; k++)
                            if (bigRescaled[powSize+k] != UINT64_MAX)
                            { ones = false; break; }
                        if (ones)
                        {
                            isNotTooExact = (bigRescaled[powSize] >= UINT64_MAX-2);
                            isHalfEqual = (bigRescaled[powSize] == UINT64_MAX);
                        }
                    }
                }
                
                // next big size
                if (bigSize != 2)
                    bigSize = (bigSize<<1)+1;
                else
                    bigSize += 1;
                bigSize = std::min(bigSize, maxBigSize);
            }
            /* after last trial we checks isNotTooExact, if yes we checking parity
             * of next value and scans further digits for a determining what is half */
            // final value
            const cxuint subValueShift = (rescaledValueBits - mantSignifBits)&63;
            const cxuint subValuePos = (rescaledValueBits - mantSignifBits)>>6;
            fpExponent = (binaryExp >= minExpNonDenorm) ?
                binaryExp+(1U<<(expBits-1))-1 : 0;
            if (mantSignifBits >= 0)
            {
                if (subValueShift != 0)
                {
                    fpMantisa = (bigRescaled[powSize+subValuePos]>>subValueShift);
                    if (subValuePos+1 < bigValueSize)
                        fpMantisa |=
                            (bigRescaled[powSize+subValuePos+1]<<(64-subValueShift));
                }
                else // shift = 0
                    fpMantisa = bigRescaled[powSize+subValuePos];
                if (fpExponent == 0) // add one for denormalized value
                    fpMantisa |= 1ULL<<mantSignifBits;
            }
            else
            {   //
                fpMantisa = 0;
                addRoundings = false;
            }
            
            if (isHalfEqual) // isHalfEqual implies isNotTooExact
            {
                // if even value
                if ((fpMantisa&1) == 0)
                {
                    /* parse rest of the numbers */
                    bool onlyZeros = true;
                    for (; vs != valEnd; vs++)
                    {
                        if (*vs == '.')
                            continue;
                        if (*vs != '0')
                        {
                            onlyZeros = false;
                            break;
                        }
                    }
                    addRoundings = !onlyZeros;
                }
                else  // otherwise force rounding
                    addRoundings = true;
            }
            else // otherwise
                addRoundings = isSecondHalf;
        }
        
        // add rounding if needed
        if (addRoundings)
        {
            // add roundings
            fpMantisa++;
            // check promotion to next exponent
            if (fpMantisa >= (1ULL<<mantisaBits))
            {
                fpExponent++;
                fpMantisa = 0; // zeroing value
            }
        }
        if (fpExponent >= ((1U<<expBits)-1))
            throw ParseException("Absolute value of number is too big");
        out |= fpMantisa | (uint64_t(fpExponent)<<mantisaBits);
    }
    return out;
}

size_t CLRX::fXtocstrCStyle(uint64_t value, char* str, size_t maxSize,
        bool scientific, cxuint expBits, cxuint mantisaBits)
{
    char* p = str;
    bool signOfValue = ((value>>(expBits+mantisaBits))!=0);
    const cxuint expMask = ((1U<<expBits)-1U);
    const uint64_t mantisaMask = (1ULL<<mantisaBits)-1ULL;
    
    if (((value >> mantisaBits)&expMask) == expMask)
    {
        // infinity or nans
        if (signOfValue)
        {
            if (maxSize < 5)
                throw Exception("Max size is too small");
            *p++ = '-';
        }
        else if (maxSize < 4)
            throw Exception("Max size is too small");
        
        if ((value&mantisaMask) != 0)
        {
            /* nan */
            *p++ = 'n';
            *p++ = 'a';
            *p++ = 'n';
        }
        else
        {
            /* infinity */
            *p++ = 'i';
            *p++ = 'n';
            *p++ = 'f';
        }
        *p = 0;
        return p-str;
    }
    
    uint64_t mantisa = value&mantisaMask;
    const int minExpNonDenorm = -((1U<<(expBits-1))-2);
    const int binaryExp = ((value>>mantisaBits)&expMask) - (expMask>>1);
    
    if (mantisa == 0 && binaryExp == minExpNonDenorm-1)
    {
        /* if zero */
        if (signOfValue)
        {
            if (maxSize < 3)
                throw Exception("Max size is too small");
            *p++ = '-';
        }
        else if (maxSize < 2)
            throw Exception("Max size is too small");
        *p++ = '0';
        if (scientific)
        {
            if (p+3 >= str + maxSize)
                throw Exception("Max size is too small");
            *p++='e';
            *p++='+';
            *p++='0';
        }   
        *p = 0;
        return p-str;
    }
    
    cxuint significantBits = 0;
    if (binaryExp >= minExpNonDenorm)
    {
        /* normalized value */
        significantBits = mantisaBits;
        mantisa |= 1ULL<<mantisaBits;
    }
    else /* if value is denormalized */
        significantBits = 63 - CLZ64(mantisa);
    
    /* binExpOfValue - exponent for binaryValue (in integer form).
     * binaryExp - mantisaBits - for normalized values
     * binaryExp - mantisaBits + 1 - for denormalized form. adds 1 because integer of
     *   denormalized value is in fraction part (mantisa) binaryExp is -expMask/2.
     */
    const int binExpOfValue = binaryExp-mantisaBits+(binaryExp < minExpNonDenorm);
    // decimal exponent for value plus one for extraneous digit
    const int decExpOfValue = log2ByLog10Round(binExpOfValue)-1;
    // binary exponent for
    const int reqBinExpOfValue = log10ByLog2Floor(decExpOfValue);
    const cxuint mantisaShift = (62-significantBits);
    mantisa = mantisa<<mantisaShift;
    
    cxuint oneBitPos = mantisaShift - (binExpOfValue-reqBinExpOfValue);
    
    uint64_t pow5[2];
    uint64_t rescaled[4];
    cxuint powSize;
    cxint pow5Exp;
    // generate rescaled value
    if (decExpOfValue != 0)
    {
        const uint64_t inMantisa[2] = { 0, mantisa };
        bigPow5(-decExpOfValue, 2, powSize, pow5Exp, pow5);
        bigMul(powSize, pow5, 2, inMantisa, rescaled);
        rescaled[powSize+1] += mantisa;
        oneBitPos++;
    }
    else
    {
        /* if one */
        powSize = 1;
        pow5[0] = 0;
        rescaled[0] = rescaled[1] = 0;
        rescaled[2] = mantisa;
    }
    
    const uint64_t oneValue = 1ULL<<oneBitPos;
    uint64_t decValue = rescaled[powSize+1]>>oneBitPos;
    char buffer[20];
    cxuint digitsNum = 0;
    
    bool roundingFix = false;
    /* fix for rounding to ten */
    const cxuint mod = (decValue) % 100;
    /* check rounding digits */
    /* for latest two values >= 82 or <= 18 and not zeros, and binary mantisa is not zero
     * then try to apply rounding. do not if binary mantisa is zero, because
     * next exponent have different value threshold and that can makes trip of rounding */
    if ((mod >= 82 || (mod <= 18 && mod != 0)) && (value&mantisaMask)!=0)
    {
        // check higher round
        uint64_t rescaledHalf[4];
        // rescaled half (ULP) minus rescaled max error + 1
        rescaledHalf[0] = 0;
        rescaledHalf[1] = pow5[0]<<(mantisaShift-1);
        rescaledHalf[2] = pow5[0]>>(64-mantisaShift+1);
        if (powSize == 2)
        {
            rescaledHalf[2] |= pow5[1]<<(mantisaShift-1);
            rescaledHalf[3] = pow5[1]>>(64-mantisaShift+1);
        }
        rescaledHalf[powSize+1] |= (1ULL<<(mantisaShift-1));
        
        if (-decExpOfValue < 0 || -decExpOfValue > 55)
        {
            // if not exact pow5 (rescaled value is not exact value)
            // compute max error of rescaled value ((0.5 + 2**-29)*mantisa)
            const uint64_t maxRescaledError[2] = 
            { mantisa<<(64-30), (mantisa>>1) + (mantisa>>30) };
            bigSub(powSize+2, rescaledHalf, 2, maxRescaledError);
        }
        const uint64_t one64 = 1; // preserve rounding for exact half case
        bigSub(powSize+2, rescaledHalf, 1, &one64);
        
        if (mod >= 82)
        {
            // we must add some bits
            uint64_t toAdd[4] = { 0, 0, 0, 0 };
            // (100-mod) - (rest from value beginning one) - value to add
            toAdd[powSize+1] = ((100ULL-uint64_t(mod))<<oneBitPos) +
                    (rescaled[powSize+1] & ~(oneValue-1));
            bigSub(2+powSize, toAdd, 2+powSize, rescaled);
            // check if half changed
            if (!bigSub(2+powSize, rescaledHalf, 2+powSize, toAdd))
            {
                // if toAdd is smaller than rescaledHalf
                decValue += (100-mod);
                roundingFix = true;
            }
        }
        else if (mod <= 18)
        {
            // we must subtract some bits
            uint64_t toSub[4];
            // value to subtract
            toSub[powSize+1] = (uint64_t(mod)<<oneBitPos) |
                    (rescaled[powSize+1]&(oneValue-1));
            toSub[0] = rescaled[0];
            if (powSize == 2)
                toSub[1] = rescaled[1];
            toSub[powSize] = rescaled[powSize];
            // check if half changed
            if (!bigSub(2+powSize, rescaledHalf, 2+powSize, toSub))
            {
                // if toSub is smaller than rescaledHalf
                decValue -= mod;
                roundingFix = true;
            }
        }
    }
    
    /* fix for rounding to one (to nearest decimal value) (if not rounding zeros)
     * if needed apply rounding to nearest (to decimal value) if lowest 2 two digits
     * is not zero or mantisa is zero (just print in highest precision) */
    if (!roundingFix && (mod != 0 || (value&mantisaMask)==0) &&
        ((rescaled[powSize+1] & (oneValue-1)) >= (oneValue>>1)))
        decValue++;
    
    for (uint64_t tmpVal = decValue; tmpVal != 0; )
    {
        const uint64_t tmp = tmpVal/10u;
        const cxuint digit = tmpVal - tmp*10U;
        buffer[digitsNum++] = '0'+digit;
        tmpVal = tmp;
    }
    
    cxuint roundPos = 0;
    // count roundPos
    for (roundPos = 0; buffer[roundPos] == '0' && roundPos < digitsNum; roundPos++);
    
    if (digitsNum-roundPos > maxSize)
        throw Exception("Max size is too small");
    
    const char* strend = str + maxSize-1;
    
    if (signOfValue)
    {
        if (p >= strend)
            throw Exception("Max size is too small");
        *p++ = '-';
    }
    
    cxuint commaPos = digitsNum-1;
    cxint decExponent = decExpOfValue + digitsNum - 1;
    if (!scientific)
    {
        if (decExponent >= -5 && decExponent < 0)
        {
            // over same number
            if (p+2 > strend)
                throw Exception("Max size is too small");
            *p++ = '0';
            *p++ = '.';
            for (cxint dx = -1; dx > decExponent; dx--)
            {
                if (p >= strend)
                    throw Exception("Max size is too small");
                *p++ = '0';
            }
            commaPos = digitsNum;
        }
        else if (decExponent >= 0 && decExponent <= int(digitsNum-1))
        {
            commaPos = digitsNum - decExponent - 1;
            roundPos = std::min(commaPos, roundPos);
        }
    }
    /* put to string */
    if (p + digitsNum-roundPos > strend) // out of string
        throw Exception("Max size is too small");
    
    for (cxuint pos = digitsNum; pos > roundPos; pos--)
    {
        *p++ = buffer[pos-1];
        if (pos-1 == commaPos && pos-1 > roundPos)
            *p++ = '.';
    }
    
    if (p > strend) // out of string
        throw Exception("Max size is too small");
        
    if (scientific || decExponent < -5 || decExponent > int(digitsNum-1))
    {
        /* print exponent */
        if (p >= strend) // out of string
            throw Exception("Max size is too small");
        *p++ = 'e';
        if (decExponent < 0)
        {
            *p++ = '-';
            decExponent = -decExponent;
        }
        else
            *p++ = '+';
        
        cxuint decExpDigitsNum = 0;
        if (decExponent != 0)
            while (decExponent != 0)
            {
                const cxint tmp = decExponent/10;
                const cxint digit = decExponent - tmp*10;
                buffer[decExpDigitsNum++] = '0' + digit;
                decExponent = tmp;
            }
        else
        {
            buffer[0] = '0';
            decExpDigitsNum = 1;
        }
        
        if ((p + decExpDigitsNum) > strend)
            throw Exception("Max size is too small");
        
        for (cxuint pos = decExpDigitsNum; pos > 0; pos--)
            *p++ = buffer[pos-1];
    }
    
    *p = 0;
    return p-str;
}

size_t CLRX::uXtocstrCStyle(uint64_t value, char* str, size_t maxSize, cxuint radix,
            cxuint width, bool prefix)
{
    cxuint digitsNum = 0;
    char buffer[64];
    
    char* strend = str + maxSize-1;
    char* p = str;
    switch(radix)
    {
        case 2:
            if (prefix)
            {
                if (p+2 >= strend)
                    throw Exception("Max size is too small");
                *p++ = '0';
                *p++ = 'b';
            }
            for (uint64_t tval = value; tval != 0; digitsNum++)
            {
                const cxuint digit = tval&1;
                buffer[digitsNum] = '0'+digit;
                tval = tval>>1;
            }
            break;
        case 8:
            if (prefix)
            {
                if (p+1 >= strend)
                    throw Exception("Max size is too small");
                *p++ = '0';
            }
            for (uint64_t tval = value; tval != 0; digitsNum++)
            {
                const cxuint digit = tval&7;
                buffer[digitsNum] = '0'+digit;
                tval = tval>>3;
            }
            break;
        case 10:
            if (value > UINT32_MAX)
                for (uint64_t tval = value; tval != 0; digitsNum++)
                {
                    const uint64_t tmp = tval/10U;
                    const cxuint digit = tval - tmp*10U;
                    buffer[digitsNum] = '0'+digit;
                    tval = tmp;
                }
            else // for speed
                for (uint32_t tval = value; tval != 0; digitsNum++)
                {
                    const uint32_t tmp = tval/10U;
                    const cxuint digit = tval - tmp*10U;
                    buffer[digitsNum] = '0'+digit;
                    tval = tmp;
                }
            break;
        case 16:
            if (prefix)
            {
                if (p+2 >= strend)
                    throw Exception("Max size is too small");
                *p++ = '0';
                *p++ = 'x';
            }
            for (uint64_t tval = value; tval != 0; digitsNum++)
            {
                const cxuint digit = tval&15;
                buffer[digitsNum] = (digit < 10) ? ('0'+digit) : ('a'+digit-10);
                tval = tval>>4;
            }
            break;
        default:
            throw Exception("Unknown radix");
            break;
    }
    
    // if zero
    if (value == 0)
        buffer[digitsNum++] = '0';
    
    if (p+digitsNum > strend || p+width > strend)
        throw Exception("Max size is too small");
    
    if (digitsNum < width)
    {
        const char fillchar = (radix == 10)?' ':'0';
        for (cxuint pos = 0; pos < width-digitsNum; pos++)
            *p++ = fillchar;
    }
    
    for (cxuint pos = digitsNum; pos > 0; pos--)
        *p++ = buffer[pos-1];
    
    *p = 0;
    return p-str;
}

size_t CLRX::iXtocstrCStyle(int64_t value, char* str, size_t maxSize, cxuint radix,
            cxuint width, bool prefix)
{
    if (value >= 0)
        return uXtocstrCStyle(value, str, maxSize, radix, width, prefix);
    if (maxSize < 3)
        throw Exception("Max size is too small");
    str[0] = '-';
    return uXtocstrCStyle(-value, str+1, maxSize-1, radix, width, prefix)+1;
}

/* support for 128-bit integers */

UInt128 CLRX::cstrtou128CStyle(const char* str, const char* inend, const char*& outend)
{
    UInt128 out = { 0, 0 };
    const char* p = 0;
    if (inend == str)
    {
        outend = str;
        throw ParseException("No characters to parse");
    }
    
    if (*str == '0')
    {
        if (inend != str+1 && (str[1] == 'x' || str[1] == 'X'))
        {
            // hex
            if (inend == str+2)
            {
                outend = str+2;
                throw ParseException("Number is too short");
            }
            
            const uint64_t lastHex = (15ULL<<60);
            for (p = str+2; p != inend; p++)
            {
                cxuint digit;
                if (*p >= '0' && *p <= '9')
                    digit = *p-'0';
                else if (*p >= 'A' && *p <= 'F')
                    digit = *p-'A'+10;
                else if (*p >= 'a' && *p <= 'f')
                    digit = *p-'a'+10;
                else //
                    break;
                
                if ((out.hi & lastHex) != 0)
                {
                    outend = p;
                    throw ParseException("Number out of range");
                }
                out.hi = (out.hi<<4) | (out.lo>>60);
                out.lo = (out.lo<<4) + digit;
            }
            if (p == str+2)
            {
                outend = p;
                throw ParseException("Missing number");
            }
        }
        else if (inend != str+1 && (str[1] == 'b' || str[1] == 'B'))
        {
            // binary
            if (inend == str+2)
            {
                outend = str+2;
                throw ParseException("Number is too short");
            }
            
            const uint64_t lastBit = (1ULL<<63);
            for (p = str+2; p != inend && (*p == '0' || *p == '1'); p++)
            {
                if ((out.hi & lastBit) != 0)
                {
                    outend = p;
                    throw ParseException("Number out of range");
                }
                out.hi = (out.hi<<1) | (out.lo>>63);
                out.lo = (out.lo<<1) + (*p-'0');
            }
            if (p == str+2)
            {
                outend = p;
                throw ParseException("Missing number");
            }
        }
        else
        {
            // octal
            const uint64_t lastOct = (7ULL<<61);
            for (p = str+1; p != inend && isODigit(*p); p++)
            {
                if ((out.hi & lastOct) != 0)
                {
                    outend = p;
                    throw ParseException("Number out of range");
                }
                out.hi = (out.hi<<3) | (out.lo>>61);
                out.lo = (out.lo<<3) + (*p-'0');
            }
            // if no octal parsed, then zero and correctly treated as decimal zero
        }
    }
    else
    {
        // decimal
        const UInt128 lastDigitValue = { 11068046444225730969ULL, 1844674407370955161ULL };
        for (p = str; p != inend && isDigit(*p); p++)
        {
            if (out.hi > lastDigitValue.hi ||
                (out.hi == lastDigitValue.hi && out.lo > lastDigitValue.lo))
            {
                outend = p;
                throw ParseException("Number out of range");
            }
            const cxuint digit = (*p-'0');
            UInt128 temp = out;
            // multiply by 10 by shifting right by 3 and 1 bits with adding them
            out.hi = (temp.hi<<3) | (temp.lo>>61);
            out.lo = (temp.lo<<3);
            out.hi += (temp.hi<<1) | (temp.lo>>63);
            out.lo += (temp.lo<<1);
            if (out.lo < (temp.lo<<1)) // carry
                out.hi++;
            // add digit
            out.lo += digit;
            if (out.lo < digit) // carry
                out.hi++;
            if (out.lo < digit && out.hi == 0) // if carry
            {
                outend = p;
                throw ParseException("Number out of range");
            }
        }
        if (p == str)
        {
            outend = p;
            throw ParseException("Missing number");
        }
    }
    outend = p;
    return out;
}
