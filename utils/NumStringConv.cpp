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

#include <CLRX/Config.h>
#include <algorithm>
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

static bool bigAdd(cxuint aSize, const uint64_t* biga, const uint64_t* bigb, uint64_t* bigc)
{
    bool carry = false;
    for (cxuint i = 0; i < aSize; i++)
    {
        bigc[i] = biga[i] + bigb[i] + carry;
        carry = (bigc[i] < biga[i]) || ((bigc[i] == biga[i]) && carry);
    }
    return carry;
}

static bool bigAdd(cxuint aSize, uint64_t* biga, const uint64_t* bigb)
{
    bool carry = false;
    for (cxuint i = 0; i < aSize; i++)
    {
        biga[i] += bigb[i] + carry;
        carry = (biga[i] < bigb[i]) || ((biga[i] == bigb[i]) && carry);
    }
    return carry;
}

static bool bigAdd(cxuint aSize, uint64_t* biga, cxuint bSize, const uint64_t* bigb)
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

static bool bigAdd(cxuint aSize, const uint64_t* biga, cxuint bSize, const uint64_t* bigb,
       uint64_t* bigc)
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

static bool bigSub(cxuint aSize, uint64_t* biga, const uint64_t* bigb)
{
    bool borrow = false;
    for (cxuint i = 0; i < aSize; i++)
    {
        const uint64_t tmp = biga[i];
        biga[i] -= bigb[i] + borrow;
        borrow = (biga[i] > tmp) || (biga[i] == tmp && borrow);
    }
    return borrow;
}

static bool bigSub(cxuint aSize, uint64_t* biga, cxuint bSize, const uint64_t* bigb)
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

static void bigMul(cxuint size, const uint64_t* biga, const uint64_t* bigb,
                   uint64_t* bigc)
{
    if (size == 1)
        mul64Full(biga[0], bigb[0], bigc);
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
    else
    {   // higher level (use Karatsuba algorithm)
        const cxuint halfSize = size>>1;
        uint64_t* mx = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*(size+1)));
        bigMul(halfSize, biga, bigb, bigc);
        bigMul(halfSize, biga+halfSize, bigb+halfSize, bigc+size);
        uint64_t* suma = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*(halfSize)));
        uint64_t* sumb = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*(halfSize)));
        bool sumaLast = bigAdd(halfSize, biga, biga+halfSize, suma);
        bool sumbLast = bigAdd(halfSize, bigb, bigb+halfSize, sumb);
        mx[size] = sumaLast&sumbLast;
        bigMul(halfSize, suma, sumb, mx); /* (a0+a1)*(b0+b1) */
        if (sumaLast) // last bit in a0+a1 is set add (1<<64)*sumb
            mx[size] += bigAdd(halfSize, mx+halfSize, sumb);
        if (sumbLast) // last bit in b0+b1 is set add (1<<64)*suma
            mx[size] += bigAdd(halfSize, mx+halfSize, suma);
        // mx-bigL
        mx[size] -= bigSub(size, mx, bigc);
        mx[size] -= bigSub(size, mx, bigc+size);
        // add to bigc
        bigAdd(size+halfSize, bigc+halfSize, size+1, mx);
    }
}

static void bigMul(cxuint asize, const uint64_t* biga, cxuint bsize,
           const uint64_t* bigb, uint64_t* bigc)
{
    cxuint asizeRound;
    for (asizeRound = 1; asizeRound < asize; asizeRound <<= 1);
    cxuint bsizeRound;
    for (bsizeRound = 1; bsizeRound < bsize; bsizeRound <<= 1);
    
    asizeRound = (asizeRound != asize)?asizeRound>>1:asizeRound;
    bsizeRound = (bsizeRound != bsize)?bsizeRound>>1:bsizeRound;
    
    const cxuint smallerRound = std::min(asizeRound, bsizeRound);
    
    if (asize == bsize && asize == asizeRound && bsize == bsizeRound)
        // if this same sizes and is powers of 2
        bigMul(asize, biga, bigb, bigc);
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
        uint64_t* tmpMul = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*gsize));
        bigMul(lsize, bigl, bigg, bigc);
        bigMul(lsize, bigl, gsize-lsize, bigg+lsize, tmpMul);
        // zeroing before addition
        std::fill(bigc+(lsize<<1), bigc+lsize+gsize, uint64_t(0));
        // adds two subproducts
        bigAdd(gsize, bigc+lsize, tmpMul);
    }
    else if (smallerRound < asize && smallerRound < bsize &&
             (smallerRound<<1) >= asize && (smallerRound<<1) >= bsize)
    {   // Karatsuba for various size
        const cxuint halfSize = (asize < bsize) ? asizeRound : bsizeRound;
        const cxuint ahalfSize2 = asize-halfSize;
        const cxuint bhalfSize2 = bsize-halfSize;
        cxuint size = halfSize<<1;
        uint64_t* mx = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*(size+1)));
        bigMul(halfSize, biga, bigb, bigc);
        bigMul(ahalfSize2, biga+halfSize, bhalfSize2, bigb+halfSize, bigc+size);
        if (halfSize*halfSize < (ahalfSize2+bhalfSize2)*halfSize)
        {   // if karatsuba better
            uint64_t* suma = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*(halfSize)));
            uint64_t* sumb = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*(halfSize)));
            bool sumaLast = bigAdd(halfSize, biga, ahalfSize2, biga+halfSize, suma);
            bool sumbLast = bigAdd(halfSize, bigb, bhalfSize2, bigb+halfSize, sumb);
            mx[size] = sumaLast&sumbLast;
            bigMul(halfSize, suma, sumb, mx); /* (a0+a1)*(b0+b1) */
            if (sumaLast) // last bit in a0+a1 is set add (1<<64)*sumb
                mx[size] += bigAdd(halfSize, mx+halfSize, sumb);
            if (sumbLast) // last bit in b0+b1 is set add (1<<64)*suma
                mx[size] += bigAdd(halfSize, mx+halfSize, suma);
            // mx-bigL
            mx[size] -= bigSub(size, mx, bigc);
            mx[size] -= bigSub(size, mx, ahalfSize2+bhalfSize2, bigc+size);
        }
        else
        {   // if normal product is better than Karatsuba
            uint64_t* mx2 = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*size));
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
        if (asize < bsize)
        {
            lsizeRound = asizeRound;
            lsize = asize;
            bigl = biga;
            gsize = bsize;
            bigg = bigb;
        }
        else
        {
            lsizeRound = bsizeRound;
            lsize = bsize;
            bigl = bigb;
            gsize = asize;
            bigg = biga;
        }
        /* main routine */
        const cxuint stepsNum = gsize/lsizeRound;
        std::fill(bigc, bigc + gsize+lsize, uint64_t(0));
        const cxuint lsizeRound2 = (lsizeRound<<1);
        
        const cxuint glastSize = (gsize&(lsizeRound-1));
        if (lsizeRound == asize)
        {   /* lsize is power of two */
            uint64_t* tmpMul = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*
                    lsizeRound2));
            cxuint i;
            for (i = 0; i < stepsNum-1; i++)
            {
                bigMul(lsizeRound, bigl, bigg + i*lsizeRound, tmpMul);
                bigAdd(lsizeRound2+1, bigc + i*lsizeRound, lsizeRound2, tmpMul);
            }
            bigMul(lsizeRound, bigl, bigg + i*lsizeRound, tmpMul);
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
            uint64_t* tmpMul = static_cast<uint64_t*>(::alloca(sizeof(uint64_t)*
                    (lsizeRound2+lsize)));
            
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

/*
 * cstrtofXCStyle
 */

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
        while (p != inend && *p == '0') p++; // skip zeroes
        
        const char* vs = nullptr;
        cxuint firstDigitBits = 0;
        if (p != inend)
        {
            if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
                (*p >= 'A' && *p <= 'F')) // if integer part
            {
                firstDigitBits = (*p >= '8') ? 4 : (*p >= '4') ? 3 : (*p >= '2') ? 2 : 1;
                expOfValue = firstDigitBits-1;
                // count exponent of integer part
                vs = p; // set pointer to real value
                for (p++; p != inend &&
                    ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
                    (*p >= 'A' && *p <= 'F')); p++)
                    expOfValue += 4;
            }
            else if (*p == '.')
            {
                // count exponent of fractional part
                for (p++; p != inend && *p == '0'; p++)
                    expOfValue -= 4; // skip zeroes
                if (p != inend && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
                        (*p >= 'A' && *p <= 'F')))
                {
                    firstDigitBits = (*p >= '8') ? 4 : (*p >= '4') ? 3 :
                            (*p >= '2') ? 2 : 1;
                    expOfValue += (firstDigitBits-1) - 4;
                    vs = p; // set pointer to real value
                }
            }
        }
        
        if (vs == nullptr || vs == inend || ((*vs < '0' || *vs > '9') &&
                (*vs < 'a' || *vs > 'f') && (*vs < 'A' || *vs > 'F')))
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
            else if (*vs == '.')
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
                {
                    // digits character has been already checked, we check that is not zero
                    if (*vs == '.')
                        continue; // skip comma
                    else if (*vs > '0')
                    {   // if not zero (is half of value)
                        isHalf = false;
                        break;
                    }
                }
            }
            
            /* is greater than half or value is odd or is half of
             * smallest denormalized value */
            addRoundings = (!isHalf || (fpMantisa&1)!=0 ||
                    (fpExponent == 0 && fpMantisa==0));
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
        throw ParseException("Unsupported");
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
