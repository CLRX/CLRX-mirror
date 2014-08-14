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

//#define CSTRTOFX_DUMP_IRRESULTS 1

#include <CLRX/Config.h>
#include <algorithm>
#ifdef CSTRTOFX_DUMP_IRRESULTS
#include <iostream>
#include <sstream>
#include <iomanip>
#endif
#include <cstdint>
#include <alloca.h>
#include <climits>
#include <CLRX/Utilities.h>

using namespace CLRX;

cxuint CLRX::cstrtoui(const char* str, const char* inend, const char*& outend)
{
    uint64_t out = 0;
    const char* p;
    if (inend == str)
        throw ParseException("No characters to parse");
    
    for (p = str; p != inend && *p >= '0' && *p <= '9'; p++)
    {
        out = (out*10U) + *p-'0';
        if (out > UINT_MAX)
            throw ParseException("number out of range");
    }
    if (p == str)
        throw ParseException("A missing number");
    outend = p;
    return out;
}

static uint64_t cstrtouXCStyle(const char* str, const char* inend,
             const char*& outend, cxuint bits)
{
    uint64_t out = 0;
    const char* p = 0;
    if (inend == str)
        throw ParseException("No characters to parse");
    
    if (*str == '0')
    {
        if (inend != str+1 && (str[1] == 'x' || str[1] == 'X'))
        {   // hex
            if (inend == str+2)
                throw ParseException("Number is too short");
            
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
                else //
                    break;
                
                if ((out & lastHex) != 0)
                    throw ParseException("Number out of range");
                out = (out<<4) + digit;
            }
            if (p == str+2)
                throw ParseException("A missing number");
        }
        else if (inend != str+1 && (str[1] == 'b' || str[1] == 'B'))
        {   // binary
            if (inend == str+2)
                throw ParseException("Number is too short");
            
            const uint64_t lastBit = (1ULL<<(bits-1));
            for (p = str+2; p != inend && (*p == '0' || *p == '1'); p++)
            {
                if ((out & lastBit) != 0)
                    throw ParseException("Number out of range");
                out = (out<<1) + (*p-'0');
            }
            if (p == str+2)
                throw ParseException("A missing number");
        }
        else
        {   // octal
            const uint64_t lastOct = (7ULL<<(bits-3));
            for (p = str+1; p != inend && *p >= '0' && *p <= '7'; p++)
            {
                if ((out & lastOct) != 0)
                    throw ParseException("Number out of range");
                out = (out<<3) + (*p-'0');
            }
            // if no octal parsed, then zero and correctly treated as decimal zero
        }
    }
    else
    {   // decimal
        const uint64_t mask = (bits < 64) ? ((1ULL<<bits)-1) :
                    UINT64_MAX; /* fix for shift */
        const uint64_t lastDigitValue = mask/10;
        for (p = str; p != inend && *p >= '0' && *p <= '9'; p++)
        {
            if (out > lastDigitValue)
                throw ParseException("Number out of range");
            cxuint digit = (*p-'0');
            out = out * 10 + digit;
            if ((out&mask) < digit) // if carry
                throw ParseException("Number out of range");
        }
        if (p == str)
            throw ParseException("A missing number");
    }
    outend = p;
    return out;
}

uint8_t CLRX::cstrtou8CStyle(const char* str, const char* inend, const char*& outend)
{
    return cstrtouXCStyle(str, inend, outend, 8);
}

uint16_t CLRX::cstrtou16CStyle(const char* str, const char* inend, const char*& outend)
{
    return cstrtouXCStyle(str, inend, outend, 16);
}

uint32_t CLRX::cstrtou32CStyle(const char* str, const char* inend, const char*& outend)
{
    return cstrtouXCStyle(str, inend, outend, 32);
}

uint64_t CLRX::cstrtou64CStyle(const char* str, const char* inend, const char*& outend)
{
    return cstrtouXCStyle(str, inend, outend, 64);
}

/*
 * parseExponent
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
    for (;expstr != inend && *expstr >= '0' && *expstr <= '9'; expstr++)
    {
        if (absExponent > ((1U<<31)/10))
            throw ParseException("Exponent out of range");
        cxuint digit = (*expstr-'0');
        absExponent = absExponent * 10 + digit;
        if ((absExponent&((1U<<31)-1)) < digit && absExponent != (1U<<31))
            // if carry
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

static inline bool bigAdd(cxuint aSize, const uint64_t* biga, const uint64_t* bigb,
              uint64_t* bigc)
{
    bool carry = false;
    for (cxuint i = 0; i < aSize; i++)
    {
        bigc[i] = biga[i] + bigb[i] + carry;
        carry = (bigc[i] < biga[i]) || ((bigc[i] == biga[i]) && carry);
    }
    return carry;
}

static bool inline bigAdd(cxuint aSize, uint64_t* biga, const uint64_t* bigb)
{
    bool carry = false;
    for (cxuint i = 0; i < aSize; i++)
    {
        biga[i] += bigb[i] + carry;
        carry = (biga[i] < bigb[i]) || ((biga[i] == bigb[i]) && carry);
    }
    return carry;
}

static bool inline bigAdd(cxuint aSize, uint64_t* biga, cxuint bSize, const uint64_t* bigb)
{
    bool carry = false;
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

static bool inline bigAdd(cxuint aSize, const uint64_t* biga, cxuint bSize,
          const uint64_t* bigb, uint64_t* bigc)
{
    bool carry = false;
    cxuint minSize = std::min(aSize, bSize);
    cxuint i = 0;
    for (; i < minSize; i++)
    {
        bigc[i] = biga[i]+ bigb[i] + carry;
        carry = (bigc[i] < biga[i]) || ((bigc[i] == biga[i]) && carry);
    }
    for (; i < aSize; i++)
    {
        bigc[i] = biga[i] + carry;
        carry = (bigc[i] < carry);
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

#ifdef HAVE_INT128
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
            // add to bigc
            tmpMul[j] += t[0];
            bool carry = (tmpMul[j] < t[0]);
            tmpMul[j+1] += t[1] + carry;
            tmpMul[j+2] = (tmpMul[j+1] < t[1]) || (tmpMul[j+1] == t[1] && carry);
        }
        mul64Full(biga[i], bigb[bsize-1], t);
        tmpMul[bsize-1] += t[0];
        bool carry = (tmpMul[bsize-1] < t[0]);
        tmpMul[bsize] += t[1] + carry;
        // ad to product
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
#endif

static void bigMulPow2(cxuint size, const uint64_t* biga, const uint64_t* bigb,
                   uint64_t* bigc)
{
    if (size == 1)
        mul64Full(biga[0], bigb[0], bigc);
#ifdef HAVE_INT128
    else if (size <= 8)
        bigMulSimple(size, biga, size, bigb, bigc);
#else
    else if (size == 2)
    {   /* in this level we are using Karatsuba algorithm */
        uint64_t mx[3];
        mul64Full(biga[0], bigb[0], bigc);
        mul64Full(biga[1], bigb[1], bigc+2);
        const uint64_t suma = biga[0]+biga[1];
        const uint64_t sumb = bigb[0]+bigb[1];
        mul64Full(suma, sumb, mx); /* (a0+a1)*(b0+b1)  */
        bool sumaLast = suma < biga[0]; // last bit of suma
        bool sumbLast = sumb < bigb[0]; // last bit of sumb
        mx[2] = sumaLast&sumbLast; // zeroing last elem
        if (sumaLast) // last bit in a0+a1 is set
        {   // add (1<<64)*sumb
            mx[1] += sumb;
            mx[2] += (mx[1] < sumb); // carry from add
        }
        if (sumbLast) // last bit in b0+b1 is set
        {   // add (1<<64)*suma
            mx[1] += suma;
            mx[2] += (mx[1] < suma); // carry from add
        }
        {   // mx-bigcL
            bool borrow;
            uint64_t old = mx[0];
            mx[0] -= bigc[0];
            borrow = (mx[0]>old);
            old = mx[1];
            mx[1] -= bigc[1] + borrow;
            borrow = (mx[1]>old) || (mx[1] == old && borrow);
            mx[2] -= borrow;
            // mx-bigcH
            old = mx[0];
            mx[0] -= bigc[2];
            borrow = (mx[0]>old);
            old = mx[1];
            mx[1] -= bigc[3] + borrow;
            borrow = (mx[1]>old) || (mx[1] == old && borrow);
            mx[2] -= borrow;
        }
        // add to bigc
        bigc[1] += mx[0];
        const uint64_t oldBigc2 = bigc[2]; // old for carry
        bigc[2] += (bigc[1] < mx[0]); // carry from bigc[1]+mx[0]
        bigc[3] += (bigc[2] < oldBigc2); // propagate previous carry
        bigc[2] += mx[1]; // next add
        bigc[3] += (bigc[2] < mx[1]); // carry from bigc[2]+mx[1]
        bigc[3] += mx[2];   // last addition
    }
#endif
    else
    {   // higher level (use Karatsuba algorithm)
        const cxuint halfSize = size>>1;
        uint64_t* mx = static_cast<uint64_t*>(::alloca((size+1)<<3));
        bigMulPow2(halfSize, biga, bigb, bigc);
        bigMulPow2(halfSize, biga+halfSize, bigb+halfSize, bigc+size);
        uint64_t* suma = static_cast<uint64_t*>(::alloca(halfSize<<3));
        uint64_t* sumb = static_cast<uint64_t*>(::alloca(halfSize<<3));
        bool sumaLast = bigAdd(halfSize, biga, biga+halfSize, suma);
        bool sumbLast = bigAdd(halfSize, bigb, bigb+halfSize, sumb);
        mx[size] = sumaLast&sumbLast;
        bigMulPow2(halfSize, suma, sumb, mx); /* (a0+a1)*(b0+b1) */
        if (sumaLast) // last bit in a0+a1 is set add (1<<64)*sumb
            bigAdd(halfSize+1, mx+halfSize, halfSize, sumb);
        if (sumbLast) // last bit in b0+b1 is set add (1<<64)*suma
            bigAdd(halfSize+1, mx+halfSize, halfSize, suma);
        // mx-bigL
        bigSub(size+1, mx, size, bigc);
        bigSub(size+1, mx, size, bigc+size);
        // add to bigc
        bigAdd(size+halfSize, bigc+halfSize, size+1, mx);
    }
}

static void bigMul(cxuint asize, const uint64_t* biga, cxuint bsize,
           const uint64_t* bigb, uint64_t* bigc)
{
    cxuint asizeRound, bsizeRound;
    cxuint asizeBit, bsizeBit;
#ifdef __GNUC__
    asizeBit = sizeof(cxuint)*8-__builtin_clz(asize);
    bsizeBit = sizeof(cxuint)*8-__builtin_clz(bsize);
    asizeRound = 1U<<asizeBit;
    bsizeRound = 1U<<bsizeBit;
#else
    for (asizeBit = 0, asizeRound = 1; asizeRound < asize; asizeRound <<= 1, asizeBit++);
    for (bsizeBit = 0, bsizeRound = 1; bsizeRound < bsize; bsizeRound <<= 1, bsizeBit++);
#endif
    
    if (asizeRound != asize)
    {
        asizeBit--;
        asizeRound = asizeRound>>1;
    }
    if (bsizeRound != bsize)
    {
        bsizeBit--;
        bsizeRound = bsizeRound>>1;
    }
    
    const cxuint smallerRound = std::min(asizeRound, bsizeRound);
    
    if (asize == bsize && asize == asizeRound && bsize == bsizeRound)
        // if this same sizes and is powers of 2
        bigMulPow2(asize, biga, bigb, bigc);
    else if (asize == 1 || bsize == 1)
    {   // if one number is single 64-bit
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
#ifdef HAVE_INT128
    else if (asize*bsize <= 128)
        bigMulSimple(asize, biga, bsize, bigb, bigc);
#endif
    else if ((smallerRound == asize || smallerRound == bsize) &&
        (smallerRound<<1) >= asize && (smallerRound<<1) >= bsize)
    {   // [Ah,AL]*[BL] or [AL]*[Bh,BL] where sizeof Bh or Ah is smaller than BL
        cxuint lsize, gsize;
        const uint64_t* bigl, *bigg;
        if (asize < bsize)
        {
            lsize = asize;
            gsize = bsize;
            bigl = biga;
            bigg = bigb;
        }
        else
        {
            lsize = bsize;
            gsize = asize;
            bigl = bigb;
            bigg = biga;
        }
        uint64_t* tmpMul = static_cast<uint64_t*>(::alloca(gsize<<3));
        bigMulPow2(lsize, bigl, bigg, bigc);
        bigMul(lsize, bigl, gsize-lsize, bigg+lsize, tmpMul);
        // zeroing before addition
        std::fill(bigc+(lsize<<1), bigc+lsize+gsize, uint64_t(0));
        // adds two subproducts
        bigAdd(gsize, bigc+lsize, tmpMul);
    }
    else if (smallerRound < asize && smallerRound < bsize &&
             (smallerRound<<1) >= asize && (smallerRound<<1) >= bsize)
    {   // Karatsuba for various size
        const cxuint halfSize = smallerRound;
        const cxuint ahalfSize2 = asize-halfSize;
        const cxuint bhalfSize2 = bsize-halfSize;
        cxuint size = halfSize<<1;
        uint64_t* mx = static_cast<uint64_t*>(::alloca((size+1)<<3));
        bigMulPow2(halfSize, biga, bigb, bigc);
        bigMul(ahalfSize2, biga+halfSize, bhalfSize2, bigb+halfSize, bigc+size);
        if (halfSize*halfSize < (ahalfSize2+bhalfSize2)*halfSize)
        {   // if karatsuba better
            uint64_t* suma = static_cast<uint64_t*>(::alloca(halfSize<<3));
            uint64_t* sumb = static_cast<uint64_t*>(::alloca(halfSize<<3));
            bool sumaLast = bigAdd(halfSize, biga, ahalfSize2, biga+halfSize, suma);
            bool sumbLast = bigAdd(halfSize, bigb, bhalfSize2, bigb+halfSize, sumb);
            mx[size] = sumaLast&sumbLast;
            bigMulPow2(halfSize, suma, sumb, mx); /* (a0+a1)*(b0+b1) */
            if (sumaLast) // last bit in a0+a1 is set add (1<<64)*sumb
                bigAdd(halfSize+1, mx+halfSize, halfSize, sumb);
            if (sumbLast) // last bit in b0+b1 is set add (1<<64)*suma
                bigAdd(halfSize+1, mx+halfSize, halfSize, suma);
            // mx-bigL-bigH
            bigSub(size+1, mx, size, bigc);
            bigSub(size+1, mx, ahalfSize2+bhalfSize2, bigc+size);
        }
        else
        {   // if normal product is better than Karatsuba
            uint64_t* mx2 = static_cast<uint64_t*>(::alloca(size<<3));
            std::fill(mx+asize, mx+size+1, uint64_t(0));
            bigMul(ahalfSize2, biga+halfSize, halfSize, bigb, mx);
            bigMul(halfSize, biga, bhalfSize2, bigb+halfSize, mx2);
            bigAdd(size+1, mx, bsize, mx2);
        }
        // add to bigc
        bigAdd(asize+bsize-halfSize, bigc+halfSize, size+1, mx);
    }
    else
    {   /* otherwise we reuses bigMul to multiply numbers with various sizes */
        cxuint lsizeRound;
        cxuint lsize, gsize;
        const uint64_t* bigl, *bigg;
        cxuint lsizeBit;
        if (asize < bsize)
        {
            lsizeRound = asizeRound;
            lsize = asize;
            bigl = biga;
            gsize = bsize;
            bigg = bigb;
            lsizeBit = asizeBit;
        }
        else
        {
            lsizeRound = bsizeRound;
            lsize = bsize;
            bigl = bigb;
            gsize = asize;
            bigg = biga;
            lsizeBit = bsizeBit;
        }
        /* main routine */
        const cxuint stepsNum = gsize>>lsizeBit;
        std::fill(bigc, bigc + gsize+lsize, uint64_t(0));
        const cxuint lsizeRound2 = (lsizeRound<<1);
        
        const cxuint glastSize = (gsize&(lsizeRound-1));
        if (lsizeRound == lsize)
        {   /* lsize is power of two */
            uint64_t* tmpMul = static_cast<uint64_t*>(::alloca(lsizeRound2<<3));
            cxuint i;
            for (i = 0; i < stepsNum-1; i++)
            {
                bigMulPow2(lsizeRound, bigl, bigg + i*lsizeRound, tmpMul);
                bigAdd(lsizeRound2+1, bigc + i*lsizeRound, lsizeRound2, tmpMul);
            }
            bigMulPow2(lsizeRound, bigl, bigg + i*lsizeRound, tmpMul);
            // lsizeRound2+(glastSize!=0) - includes carry only when required
            bigAdd(lsizeRound2 + (glastSize!=0), bigc + i*lsizeRound, lsizeRound2, tmpMul);
            
            if (glastSize != 0)
            {   // last part of H
                const cxuint glastPos = stepsNum*lsizeRound;
                bigMul(lsizeRound, bigl, glastSize, bigg + glastPos, tmpMul);
                bigAdd(lsizeRound + glastSize, bigc + glastPos,
                       lsizeRound+glastSize, tmpMul);
            }
        }
        else
        {   /* lsize is not power of two */
            uint64_t* tmpMul = static_cast<uint64_t*>(::alloca((lsizeRound2+lsize)<<3));
            
            const cxuint newStepsNum = stepsNum&~1;
            cxuint i;
            for (i = 0; i < newStepsNum-2; i+=2)
            {
                bigMul(lsize, bigl, lsizeRound2, bigg + i*lsizeRound, tmpMul);
                bigAdd(lsizeRound2+lsize+1, bigc + i*lsizeRound,
                       lsizeRound2+lsize, tmpMul);
            }
            bigMul(lsize, bigl, lsizeRound2, bigg + i*lsizeRound, tmpMul);
            // include carry only when required
            bigAdd(lsizeRound2+lsize + ((gsize&(lsizeRound2-1)) != 0),
                   bigc + i*lsizeRound, lsizeRound2+lsize, tmpMul);
            
            const cxuint glastPos = stepsNum*lsizeRound;
            if ((stepsNum&1) != 0)
            {
                bigMul(lsize, bigl, lsizeRound, bigg + glastPos-lsizeRound, tmpMul);
                // include carry only when required (lsize+lsizeRound + (glastSize != 0))
                bigAdd(lsize+lsizeRound + (glastSize != 0), bigc + glastPos-lsizeRound,
                       lsize+lsizeRound, tmpMul);
            }
            if (glastSize != 0)
            {
                bigMul(lsize, bigl, glastSize, bigg + glastPos, tmpMul);
                bigAdd(lsize+glastSize, bigc + glastPos, lsize+glastSize, tmpMul);
            }
        }
    }
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
    bool carry = false;
    if ((bigNum[roundSize-1] & (1ULL<<63)) != 0)
    {   /* apply rounding */
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

struct Pow5NumTableEntry
{
    uint64_t value;
    cxuint exponent;
};

static const Pow5NumTableEntry pow5Table[28] =
{
    { 0x0000000000000000ULL, 0 },
    { 0x4000000000000000ULL, 2 },
    { 0x9000000000000000ULL, 4 },
    { 0xf400000000000000ULL, 6 },
    { 0x3880000000000000ULL, 9 },
    { 0x86a0000000000000ULL, 11 },
    { 0xe848000000000000ULL, 13 },
    { 0x312d000000000000ULL, 16 },
    { 0x7d78400000000000ULL, 18 },
    { 0xdcd6500000000000ULL, 20 },
    { 0x2a05f20000000000ULL, 23 },
    { 0x74876e8000000000ULL, 25 },
    { 0xd1a94a2000000000ULL, 27 },
    { 0x2309ce5400000000ULL, 30 },
    { 0x6bcc41e900000000ULL, 32 },
    { 0xc6bf526340000000ULL, 34 },
    { 0x1c37937e08000000ULL, 37 },
    { 0x6345785d8a000000ULL, 39 },
    { 0xbc16d674ec800000ULL, 41 },
    { 0x158e460913d00000ULL, 44 },
    { 0x5af1d78b58c40000ULL, 46 },
    { 0xb1ae4d6e2ef50000ULL, 48 },
    { 0x0f0cf064dd592000ULL, 51 },
    { 0x52d02c7e14af6800ULL, 53 },
    { 0xa784379d99db4200ULL, 55 },
    { 0x08b2a2c280290940ULL, 58 },
    { 0x4adf4b7320334b90ULL, 60 },
    { 0x9d971e4fe8401e74ULL, 62 }
};

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
    bigcSize = bigaSize + bigbSize;
    bigcBits = bigaBits + bigbBits;
    bigcExp = bigaExp + bigbExp;
    if (carry != 0)
    {   // carry, we shift right, and increment exponent
        const bool lsbit = bigc[0]&1;
        bigShift64Right(bigcSize, bigc, 1);
        bigc[bigcSize-1] |= (uint64_t(carry==2)<<63);
        bigcExp++;
        bigcBits++;
        if (bigcBits == (bigcSize<<6)+1 && bigcSize < maxSize)
        {   // push least signicant bit to first elem
            /* this make sense only when number needs smaller space than maxSize */
            for (cxuint j = bigcSize; j > 0; j--)
                bigc[j] = bigc[j-1];
            bigc[0] = uint64_t(lsbit)<<63;
            bigcSize++;
        }
    }
    if (bigcSize > maxSize)
    {   /* if out of maxSize, then round this number */
        bigcBits = maxSize<<6;
        bigFPRoundToNearest(bigcSize, maxSize, bigcExp, bigc);
        bigcSize = maxSize;
    }
    else if (bigcSize > ((bigcBits+63)>>6))
    {   // fix bigc size if doesnt match with bigcBits
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
    if (power >= 0 && power < 28)
    {   /* get result from table */
        outPow[0] = pow5Table[power].value;
        powSize = 1;
        exponent = pow5Table[power].exponent;
        return;
    }
    maxSize++; // increase by 1 elem (64-bit) for accuracy
    
    uint64_t* heap = new uint64_t[maxSize<<3]; // four (maxSize<<1)
    uint64_t* curPow2Pow = heap;
    uint64_t* prevPow2Pow = heap + (maxSize<<1);
    uint64_t* curPow = heap + (maxSize<<1)*2;
    uint64_t* prevPow = heap + (maxSize<<1)*3;
    
    cxuint pow2PowSize, pow2PowBits, powBits;
    cxint pow2PowExp;
    const cxuint absPower = std::abs(power);
    cxuint p = 0;
    
    if (power >= 0)
    {   // positive power
        powSize = 1;
        curPow[0] = pow5Table[power&15].value;
        exponent = pow5Table[power&15].exponent;
        powBits = exponent;
        pow2PowSize = 1;
        curPow2Pow[0] = pow5Table[16].value;
        pow2PowExp = pow5Table[16].exponent;
        pow2PowBits = pow2PowExp;
        p = 16;
    }
    else
    {   // negative power (fractions)
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
    
    for (; p <= (absPower>>1); p<<=1)
    {
        if ((absPower&p)!=0)
        {   /* POW = POW2*POW */
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
    {   /* POW = POW2*POW */
        bigMulFP(maxSize, pow2PowSize, pow2PowBits, pow2PowExp, curPow2Pow,
             powSize, powBits, exponent, curPow,
             powSize, powBits, exponent, prevPow);
        std::swap(curPow, prevPow);
    }
    // after computing
    if (powSize == maxSize)
    {   /* round to nearest if result size is greater than original maxSize */
        bigFPRoundToNearest(maxSize, maxSize-1, exponent, curPow);
        powSize = maxSize-1;
    }
    // copy result to output
    std::copy(curPow, curPow + powSize, outPow);
    
    delete[] heap;
}

/*
 * cstrtofXCStyle
 */

static const int64_t LOG2BYLOG10_32 = 1292913987LL;
static const int64_t LOG10BYLOG2_32 = 14267572528LL;

static inline cxint log2ByLog10Floor(cxint v)
{ return (int64_t(v)*LOG2BYLOG10_32)>>32; }

static inline cxint log2ByLog10Ceil(cxint v)
{ return (int64_t(v)*LOG2BYLOG10_32 + (1ULL<<32)-1)>>32; }

static inline cxint log10ByLog2Floor(cxint v)
{ return (int64_t(v)*LOG10BYLOG2_32)>>32; }

static inline cxint log10ByLog2Ceil(cxint v)
{ return (int64_t(v)*LOG10BYLOG2_32 + (1ULL<<32)-1)>>32; }

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

static uint64_t cstrtofXCStyle(const char* str, const char* inend,
             const char*& outend, cxuint expBits, cxuint mantisaBits)
{
    uint64_t out = 0;
    const char* p = 0;
    bool signOfValue = false;
    
    if (inend == str)
        throw ParseException("No characters to parse");
    
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
        {   // create positive nan
            out = (((1ULL<<expBits)-1ULL)<<mantisaBits) | (1ULL<<(mantisaBits-1));
            outend = p+3;
            return out;
        }
        else if ((p[0] == 'i' || p[0] == 'I') && (p[1] == 'n' || p[1] == 'N') &&
            (p[2] == 'f' || p[2] == 'F'))
        {   // create +/- infinity
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
    {   // in hex format
        p+=2;
        cxint binaryExp = 0;
        const char* expstr = p;
        bool comma = false;
        for (;expstr != inend && (*expstr == '.' || (*expstr >= '0' && *expstr <= '9') ||
                (*expstr >= 'a' && *expstr <= 'f') ||
                (*expstr >= 'A' && *expstr <= 'F')); expstr++)
            if (*expstr == '.')
            {   // if comma found and we found next we break skipping
                if (comma) break;
                comma = true;
            }
            
        if (p == expstr || (p+1 == expstr && *p == '.'))
            throw ParseException("Floating point doesn't have value part!");
        // value end in string
        const char* valEnd = expstr;
        
        if (expstr != inend && (*expstr == 'p' || *expstr == 'P')) // we found exponent
        {
            expstr++;
            if (expstr == inend)
                throw ParseException("End of floating point at exponent");
            binaryExp = parseFloatExponent(expstr, inend);
        }
        outend = expstr; // set out end
        
        // determine real exponent
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
            {   // count exponent of fractional part
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
        // get exponent data from
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
        { // rounding to nearest even
            bool isHalf = true;
            if ((fvalue & (roundBit-1ULL)) != 0)
                isHalf = false;
            
            if (isHalf && (fpMantisa&1)==0)
            {   // check further digits if still is half and value is even
                for (; vs != valEnd; vs++)
                {   // digits character has been already checked, we check that is not zero
                    if (*vs == '.')
                        continue; // skip comma
                    else if (*vs > '0')
                    {   // if not zero (is half of value)
                        isHalf = false;
                        break;
                    }
                }
            }
            
            /* is greater than half or value is odd */
            addRoundings = (!isHalf || (fpMantisa&1)!=0);
        }
        
        if (addRoundings)
        {   // add roundings
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
    {   // in decimal format
        cxint decimalExp = 0;
        const char* expstr = p;
        bool comma = false;
        for (;expstr != inend && (*expstr == '.' || (*expstr >= '0' && *expstr <= '9'));
             expstr++)
            if (*expstr == '.')
            {   // if comma found and we found next we break skipping
                if (comma) break;
                comma = true;
            }
            
        if (p == expstr || (p+1 == expstr && *p == '.'))
            throw ParseException("Floating point doesn't have value part!");
        // value end in string
        const char* valEnd = expstr;
        
        if (expstr != inend && (*expstr == 'e' || *expstr == 'E')) // we found exponent
        {
            expstr++;
            if (expstr == inend)
                throw ParseException("End of floating point at exponent");
            decimalExp = parseFloatExponent(expstr, inend);
        }
        outend = expstr; // set out end
        
        // determine real exponent
        cxint decExpOfValue = 0;
        const char* vs = nullptr;
        
        while (p != valEnd && *p == '0') p++; // skip zeroes
        
        if (p != valEnd)
        {
            if (*p != '.') // if integer part
            {   // count exponent of integer part
                vs = p; // set pointer to real value
                for (p++; p != valEnd && *p != '.'; p++)
                    decExpOfValue++;
            }
            else
            {   // count exponent of fractional part
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
            if (*vs >= '0' && *vs <= '9')
                value = value*10 + *vs-'0';
            else // if (*vs == '.')
                continue;
            processedDigits++;
        }
        if (processedDigits < 19)
        {   /* align to 19 digits */
            value *= power10sTable[19-processedDigits];
            processedDigits = 19;
        }
        
        uint64_t decFactor;
        cxint decFacBinExp;
        cxuint powSize;
        cxuint rescaledValueBits;
        cxint powerof5 = decTempExp-processedDigits+1;
        bigPow5(powerof5, 1, powSize, decFacBinExp, &decFactor);
        
        {   /* rescale value to binary exponent */
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
            {   /* compute rescaledValueBits */
#ifdef __GNUC__
                rescaledValueBits = 63-__builtin_clzll(rescaledValue);
#else
                rescaledValueBits = 63;
                for (uint64_t t = 1ULL<<63; rescaledValue < t;
                     t>>=1, rescaledValueBits--);
#endif
                // remove integer part (lastbit) from rescaled value
                rescaledValue &= (1ULL<<rescaledValueBits)-1ULL;
            }
            else
                rescaledValueBits = 64;
        }
        
        // compute binary exponent
        cxint binaryExp = decFacBinExp + powerof5 + rescaledValueBits;
        if (binaryExp > maxExp+1) // out of max exponent+1
            throw ParseException("Absolute value of number is too big");
        if (binaryExp < minExpDenorm-2)
            return out; // return zero
                
        cxint mantSignifBits = (binaryExp >= minExpNonDenorm) ? mantisaBits :
                binaryExp-minExpDenorm;
        bool isNotTooExact = false;
        //std::cout << "mantSignifBits: " << mantSignifBits << std::endl;
        
        const cxuint subValueShift = rescaledValueBits - mantSignifBits;
        const uint64_t subValue = (subValueShift<64)?
                rescaledValue&((1ULL<<(subValueShift))-1ULL):UINT64_MAX;
        const uint64_t half = (subValueShift<65)?(1ULL<<(subValueShift-1)):0;
        
        /* check if value is too close to half of value, if yes we going to next trials
         * too close value between HALF-3 and HALF+1 (3 because we expects value from
         * next digit */
        if (mantSignifBits >= 0)
            isNotTooExact = (subValue >= half-3ULL && subValue <= half+1ULL);
        else if (mantSignifBits == -1) // if half of smallest denormal
            isNotTooExact = (subValue <= 1ULL);
        else if (mantSignifBits == -2) // if half of smallest denormal
            isNotTooExact = (subValue >= (half>>1)-3ULL);
        bool addRoundings = false;
        uint64_t fpMantisa;
        cxuint fpExponent = (binaryExp >= minExpNonDenorm) ?
                binaryExp+(1U<<(expBits-1))-1 : 0;
        
        if (!isNotTooExact)
        {   // value is exact (not too close half
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
        {   // max digits to compute value
            cxuint maxDigits;
            if (binaryExp-cxint(mantisaBits)-1 >= 0)
                maxDigits = decTempExp+1; // when rounding bit of mantisa is not fraction
            else if (binaryExp >= 0)
                // if rounding bit is fraction but value have integer part
                maxDigits = std::max(1,log2ByLog10Ceil(binaryExp)) -
                         // negative power of rounding bit
                        (binaryExp-cxint(mantSignifBits)-1);
            else // if all value is fractional value
                maxDigits = -(binaryExp-cxint(mantSignifBits)-1) -
                    log2ByLog10Floor(-binaryExp);
            maxDigits = std::max(maxDigits, processedDigits);
            
            const cxuint maxBigSize = (log10ByLog2Ceil(maxDigits+3)+63)>>6;
            uint64_t* heap = new uint64_t[maxBigSize*5 + 5];
            uint64_t* bigDecFactor = heap;
            uint64_t* curBigValue = heap+maxBigSize;
            uint64_t* prevBigValue = heap+maxBigSize*2 + 2;
            uint64_t* bigRescaled = heap+maxBigSize*3 + 4;
            
            curBigValue[0] = value;
            bigDecFactor[0] = decFactor;
            bigRescaled[0] = 0;
            bigRescaled[1] = rescaledValue;
            
            cxuint bigSize = 2;
            cxuint bigValueSize = 1;
            bool isHalf = (subValue >= half);    // if second half
            // if value is nearly equal to half of value
            bool isHalfEqual = (subValue >= half-1ULL && subValue <= half+1ULL);
            /* next trials with higher precision */
            while (isNotTooExact && processedDigits < maxDigits+3)
            {   /* parse digits and put to bigValue */
                const cxuint digitsToProcess = std::min(int(maxDigits),
                        log2ByLog10Floor(bigSize<<6));
                
                while (processedDigits < digitsToProcess)
                {
                    uint64_t digitPack[4];
                    uint64_t packTens[4];
                    digitPack[0] = digitPack[1] = digitPack[2] = digitPack[3] = 0;
                    packTens[0] = packTens[1] = packTens[2] = packTens[3] = 1;
                    
                    cxuint digitPacksNum = 0;
                    cxuint digitsOfPack = 0;
                    while (digitPacksNum < 4 && processedDigits < digitsToProcess)
                    {
                        cxuint digitsOfPart = 0;
                        uint64_t curValue = 0;
                        for (; vs != valEnd && digitsOfPart < 19 &&
                            processedDigits < digitsToProcess; vs++)
                        {
                            if (*vs >= '0' && *vs <= '9')
                                curValue = curValue*10 + *vs-'0';
                            else if (*vs == '.')
                                continue;
                            processedDigits++;
                            digitsOfPart++;
                        }
                        if (vs == valEnd)
                        {   // fill first digits with zeroes
                            const cxuint pow10 = std::min(digitsToProcess-processedDigits,
                                        19-digitsOfPart);
                            curValue = curValue*power10sTable[pow10];
                            processedDigits += pow10;
                            digitsOfPart += pow10;
                        }
                        uint64_t tmpPack[4];
                        // put to digitPack
                        bigMul(1, &power10sTable[digitsOfPart], 4, digitPack, tmpPack);
                        digitPack[0] = tmpPack[0];
                        digitPack[1] = tmpPack[1];
                        digitPack[2] = tmpPack[2];
                        digitPack[3] = tmpPack[3];
                        bigAdd(4, digitPack, 1, &curValue);
                        digitsOfPack += digitsOfPart;
                        packTens[digitPacksNum++] = power10sTable[digitsOfPart];
                    }
                    // generate power of tens for digitPack
                    uint64_t packPowerOfTen[4];
                    uint64_t tmpProd[4];
                    if (digitPacksNum > 2)
                    {
                        mul64Full(packTens[0], packTens[1], tmpProd);
                        mul64Full(packTens[2], packTens[3], tmpProd+2);
                        bigMulPow2(2, tmpProd, tmpProd+2, packPowerOfTen);
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
                    // put to bigValue
                    bigMul(digitPackSize, packPowerOfTen, bigValueSize,
                           curBigValue, prevBigValue);
                    bigValueSize += digitPackSize;
                    prevBigValue[bigValueSize] = 0; // zeroing after addition
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
                {   // adds three digits extra in last iteration
                    // because threshold is needed for half equality detection
                    bigMul(1, power10sTable+3, bigValueSize, curBigValue, prevBigValue);
                    if (prevBigValue[bigValueSize]!=0)
                        bigValueSize++;
                    processedDigits += 3;
                    std::swap(prevBigValue, curBigValue);
                }
                
                // compute power of 5
                powerof5 = decTempExp-processedDigits+1;
                bigPow5(powerof5, bigSize, powSize, decFacBinExp, bigDecFactor);
                
                // rescale value
                if (decFacBinExp != 0) // if not 1 in bigDecFactor
                    bigMul(bigValueSize, curBigValue, powSize, bigDecFactor, bigRescaled);
                else // if one in bigDecFactor
                    std::fill(bigRescaled, bigRescaled + bigValueSize+powSize, uint64_t(0));
                
                bool rvCarry = bigAdd(bigValueSize, bigRescaled + powSize, curBigValue);
                /* round rescaled value */
                if ((bigRescaled[powSize-1]&((1ULL<<63)-1ULL)) != 0)
                {
                    bool carry = true;
                    for (cxuint k = powSize; k < powSize+bigValueSize; k++)
                    {
                        bigRescaled[k] += carry;
                        carry = (bigRescaled[k] < carry);
                    }
                    rvCarry = rvCarry || carry;
                }
                if (!rvCarry)
                {   /* compute rescaledValueBits */
#ifdef __GNUC__
                    rescaledValueBits = (bigValueSize<<6) - 1 -__builtin_clzll(
                        bigRescaled[powSize+bigValueSize-1]);
#else
                    rescaledValueBits = (bigValueSize<<6)-1;
                    for (uint64_t t = 1ULL<<63; bigRescaled[powSize+bigValueSize-1] < t;
                         t>>=1, rescaledValueBits--);
#endif
                    // remove integer part (lastbit) from rescaled value
                    bigRescaled[powSize+bigValueSize-1] &=
                            (1ULL<<(rescaledValueBits&63))-1ULL;
                }
                else
                    rescaledValueBits = bigValueSize<<6;
                
                // compute binary exponent
                binaryExp = decFacBinExp + powerof5 + rescaledValueBits;
                if (binaryExp > maxExp + 1) // out of max exponent+1
                    throw ParseException("Absolute value of number is too big");
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
                {   // smaller than smallest denormal, add half
                    if (((rescaledValueBits - mantSignifBits-1)>>6) == bigValueSize)
                        // zeroing before OR'ing
                        bigRescaled[powSize+halfValuePos] = 0;
                    // add one to value
                    bigRescaled[powSize+bigValueSize-1] |=
                            (1ULL<<(rescaledValueBits&63));
                }
                
                /* check if value is too close to half of value, if yes we going to next
                 * trials. too close value between HALF-3 and HALF+1
                 * (3 because we expects value from next digit) */
                isHalfEqual = false;
                isNotTooExact = false;
                isHalf = ((bigRescaled[powSize+halfValuePos]&halfValue) != 0);
                if (isHalf)
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
                addRoundings = (mantSignifBits == -1);
            }
            
            if (isHalfEqual) // isHalfEqual implies isNotTooExact
            {   // if even value
                if ((fpMantisa&1) == 0)
                {   /* parse rest of the numbers */
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
                addRoundings = isHalf;
            delete[] heap;
        }
        
        // add rounding if needed
        if (addRoundings)
        {   // add roundings
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
        if (fpExponent >= ((1U<<expBits)-1))
            throw ParseException("Absolute value of number is too big");
        out |= fpMantisa | (uint64_t(fpExponent)<<mantisaBits);
    }
    return out;
}

cxushort CLRX::cstrtohCStyle(const char* str, const char* inend, const char*& outend)
{
    return cstrtofXCStyle(str, inend, outend, 5, 10);
}

union FloatUnion
{
    float f;
    uint32_t u;
};

float CLRX::cstrtofCStyle(const char* str, const char* inend, const char*& outend)
{
    FloatUnion v;
    v.u = cstrtofXCStyle(str, inend, outend, 8, 23);
    return v.f;
}

union DoubleUnion
{
    double d;
    uint64_t u;
};

double CLRX::cstrtodCStyle(const char* str, const char* inend, const char*& outend)
{
    DoubleUnion v;
    v.u = cstrtofXCStyle(str, inend, outend, 11, 52);
    return v.d;
}
