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
#include <cstring>
#include <string>
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
        const uint64_t mask = (bits < 64) ? ((1ULL<<bits)-1) : (-1ULL); /* fix for shift */
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
    if (inend != nullptr && p+3 <= inend)
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
    
    if (p+1 != inend && *p == '0' && (p[1] == 'x' || p[1] == 'X'))
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
        // value end in string
        const char* valEnd = expstr;
        
        if (expstr != inend && (*expstr == 'p' || *expstr == 'P')) // we found exponential
        {
            expstr++;
            if (expstr  == inend)
                throw ParseException("End of floating point at exponent");
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
                if ((absExponent&((1U<<31)-1)) < digit) // if carry
                    throw ParseException("Exponent out of range");
            }
            
            if (!signOfExp && absExponent == (1U<<31))
                // if abs exponent with max negative value and not negative
                throw ParseException("Exponent out of range");
            binaryExp = (signOfExp) ? -absExponent : absExponent;
            
        }
        outend = expstr; // set out end
        
        if (p == inend)
            throw ParseException("Floating point doesnt have value part!");
        
        // determine real exponent
        cxint expOfValue = 0;
        const char* pstart = p;
        while (p != inend && *p == '0') p++; // skip zeroes
        
        const bool haveIntegerPart = (p != pstart);
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
                const char* pfract = ++p;
                // count exponent of fractional part
                for (; p != inend && *p == '0'; p++)
                    expOfValue -= 4; // skip zeroes
                if (p != inend && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
                        (*p >= 'A' && *p <= 'F')))
                {
                    firstDigitBits = (*p >= '8') ? 4 : (*p >= '4') ? 3 :
                            (*p >= '2') ? 2 : 1;
                    expOfValue += (firstDigitBits-1) - 4;
                    vs = p; // set pointer to real value
                    p++;
                }
                if (pfract == p && !haveIntegerPart) // if no any value part
                    throw ParseException("No integer and fraction in number");
            }
            else if (!haveIntegerPart) // if no any value part
                throw ParseException("No integer and fraction in number");
        }
        
        if (vs == inend || ((*vs < '0' || *vs > '9') && (*vs < 'a' || *vs > 'f') &&
                (*vs < 'A' || *vs > 'F')))
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

