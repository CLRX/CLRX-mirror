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

#include <CLRX/Config.h>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/ROCmBinaries.h>

namespace CLRX
{
void parsePrintfInfoString(const char* ptr2, const char* end2, size_t oldLineNo,
                size_t lineNo, ROCmPrintfInfo& printfInfo,
                std::unordered_set<cxuint>& printfIds);
};

using namespace CLRX;

// trim spaces (remove spaces from start and end)
static std::string trimStrSpaces(const std::string& str)
{
    size_t i = 0;
    const size_t sz = str.size();
    while (i!=sz && isSpace(str[i])) i++;
    if (i == sz) return "";
    size_t j = sz-1;
    while (j>i && isSpace(str[j])) j--;
    return str.substr(i, j-i+1);
}

/*
 * ROCm metadata MsgPack parser
 */

static void parseMsgPackNil(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd || *dataPtr != 0xc0)
        throw ParseException("MsgPack: Can't parse nil value");
    dataPtr++;
}

static bool parseMsgPackBool(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd || ((*dataPtr)&0xfe) != 0xc2)
        throw ParseException("MsgPack: Can't parse bool value");
    const bool v = (*dataPtr==0xc3);
    dataPtr++;
    return v;
}

static uint64_t parseMsgPackInteger(const cxbyte*& dataPtr, const cxbyte* dataEnd,
                cxbyte signess = MSGPACK_WS_BOTH)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't parse integer value");
    uint64_t v = 0;
    if (*dataPtr < 0x80)
        v = *dataPtr++;
    else if (*dataPtr >= 0xe0)
    {
        v = uint64_t(-32) + ((*dataPtr++) & 0x1f);
        if (signess == MSGPACK_WS_UNSIGNED && v >= (1ULL<<63))
            throw ParseException("MsgPack: Negative value for unsigned integer");
    }
    else
    {
        const cxbyte code = *dataPtr++;
        switch(code)
        {
            case 0xcc:
            case 0xd0:
                if (dataPtr>=dataEnd)
                    throw ParseException("MsgPack: Can't parse integer value");
                if (code==0xcc)
                    v = *dataPtr++;
                else
                    v = int8_t(*dataPtr++);
                break;
            case 0xcd:
            case 0xd1:
                if (dataPtr+1>=dataEnd)
                    throw ParseException("MsgPack: Can't parse integer value");
                v = uint16_t(*dataPtr++)<<8;
                v |= *dataPtr++;
                if (code==0xd1 && (v&(1ULL<<15))!=0)
                    v |= (0xffffffffffffULL<<16);
                break;
            case 0xce:
            case 0xd2:
                if (dataPtr+3>=dataEnd)
                    throw ParseException("MsgPack: Can't parse integer value");
                for (cxint i = 24; i >= 0; i-=8)
                    v |= uint32_t(*dataPtr++)<<i;
                if (code==0xd2 && (v&(1ULL<<31))!=0)
                    v |= (0xffffffffULL<<32);
                break;
            case 0xcf:
            case 0xd3:
                if (dataPtr+7>=dataEnd)
                    throw ParseException("MsgPack: Can't parse integer value");
                for (cxint i = 56; i >= 0; i-=8)
                    v |= uint64_t(*dataPtr++)<<i;
                break;
            default:
                throw ParseException("MsgPack: Can't parse integer value");
        }
        
        if (signess == MSGPACK_WS_UNSIGNED && code >= 0xd0 && v >= (1ULL<<63))
            throw ParseException("MsgPack: Negative value for unsigned integer");
        if (signess == MSGPACK_WS_SIGNED && code < 0xd0 && v >= (1ULL<<63))
            throw ParseException("MsgPack: Positive value out of range for signed integer");
    }
    return v;
}

static double parseMsgPackFloat(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't parse float value");
    const cxbyte code = *dataPtr++;
    if (code == 0xca)
    {
        union {
            uint32_t v;
            float vf;
        } v;
        v.v = 0;
        if (dataPtr+3>=dataEnd)
            throw ParseException("MsgPack: Can't parse float value");
        for (cxint i = 24; i >= 0; i-=8)
            v.v |= uint32_t(*dataPtr++)<<i;
        return v.vf;
    }
    else if (code == 0xcb)
    {
        union {
            uint64_t v;
            double vf;
        } v;
        v.v = 0;
        if (dataPtr+7>=dataEnd)
            throw ParseException("MsgPack: Can't parse float value");
        for (cxint i = 56; i >= 0; i-=8)
            v.v |= uint64_t(*dataPtr++)<<i;
        return v.vf;
    }
    else
        throw ParseException("MsgPack: Can't parse float value");
}

static std::string parseMsgPackString(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't parse string");
    size_t size = 0;
    
    if ((*dataPtr&0xe0) == 0xa0)
        size = (*dataPtr++) & 0x1f;
    else
    {
        const cxbyte code = *dataPtr++;
        switch (code)
        {
            case 0xd9:
                if (dataPtr>=dataEnd)
                    throw ParseException("MsgPack: Can't parse string size");
                size = *dataPtr++;
                break;
            case 0xda:
                if (dataPtr+1>=dataEnd)
                    throw ParseException("MsgPack: Can't parse string size");
                size = uint32_t(*dataPtr++)<<8;
                size |= *dataPtr++;
                break;
            case 0xdb:
                if (dataPtr+3>=dataEnd)
                    throw ParseException("MsgPack: Can't parse string size");
                for (cxint i = 24; i >= 0; i-=8)
                    size |= uint32_t(*dataPtr++)<<i;
                break;
            default:
                throw ParseException("MsgPack: Can't parse string");
        }
    }
    
    if (dataPtr+size > dataEnd)
        throw ParseException("MsgPack: Can't parse string");
    const char* strData = reinterpret_cast<const char*>(dataPtr);
    std::string out(strData, strData + size);
    dataPtr += size;
    return out;
}

static Array<cxbyte> parseMsgPackData(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't parse byte-array");
    const cxbyte code = *dataPtr++;
    size_t size = 0;
    switch (code)
    {
        case 0xc4:
            if (dataPtr>=dataEnd)
                throw ParseException("MsgPack: Can't parse byte-array size");
            size = *dataPtr++;
            break;
        case 0xc5:
            if (dataPtr+1>=dataEnd)
                throw ParseException("MsgPack: Can't parse byte-array size");
            size = uint32_t(*dataPtr++)<<8;
            size |= *dataPtr++;
            break;
        case 0xc6:
            if (dataPtr+3>=dataEnd)
                throw ParseException("MsgPack: Can't parse byte-array size");
            for (cxint i = 24; i >= 0; i-=8)
                size |= uint32_t(*dataPtr++)<<i;
            break;
        default:
            throw ParseException("MsgPack: Can't parse byte-array");
    }
    
    if (dataPtr+size > dataEnd)
        throw ParseException("MsgPack: Can't parse byte-array");
    Array<cxbyte> out(dataPtr, dataPtr + size);
    dataPtr += size;
    return out;
}

static void skipMsgPackObject(const cxbyte*& dataPtr, const cxbyte* dataEnd)
{
    if (dataPtr>=dataEnd)
        throw ParseException("MsgPack: Can't skip object");
    if (*dataPtr==0xc0 || *dataPtr==0xc2 || *dataPtr==0xc3 ||
        *dataPtr < 0x80 || *dataPtr >= 0xe0)
        dataPtr++;
    else if (*dataPtr==0xcc || *dataPtr==0xd0)
    {
        if (dataPtr+1>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += 2;
    }
    else if (*dataPtr==0xcd || *dataPtr==0xd1)
    {
        if (dataPtr+2>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += 3;
    }
    else if (*dataPtr==0xce || *dataPtr==0xd2 || *dataPtr==0xca)
    {
        if (dataPtr+4>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += 5;
    }
    else if (*dataPtr==0xcf || *dataPtr==0xd3 || *dataPtr==0xcb)
    {
        if (dataPtr+8>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += 9;
    }
    else if(((*dataPtr)&0xe0)==0xa0)
    {
        const size_t size = *dataPtr&0x1f;
        if (dataPtr+size>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += size+1;
    }
    else if (*dataPtr == 0xc4 || *dataPtr == 0xd9)
    {
        dataPtr++;
        if (dataPtr>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        const size_t size = *dataPtr++;
        if (dataPtr+size>dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += size;
    }
    else if (*dataPtr == 0xc5 || *dataPtr == 0xda)
    {
        dataPtr++;
        if (dataPtr+1>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        size_t size = uint16_t(*dataPtr++)<<8;
        size |= *dataPtr++;
        if (dataPtr+size>dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += size;
    }
    else if (*dataPtr == 0xc6 || *dataPtr == 0xdb)
    {
        dataPtr++;
        if (dataPtr+1>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        size_t size = 0;
        for (cxint i = 24; i >= 0; i-=8)
            size |= uint32_t(*dataPtr++)<<i;
        if (dataPtr+size>dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        dataPtr += size;
    }
    else if ((*dataPtr&0xf0) == 0x90 || (*dataPtr&0xf0) == 0x80)
    {
        const bool isMap = (*dataPtr<0x90);
        size_t size = (*dataPtr++)&15;
        if (isMap)
            size <<= 1;
        for (size_t i = 0; i < size; i++)
            skipMsgPackObject(dataPtr, dataEnd);
    }
    else if (*dataPtr == 0xdc || *dataPtr==0xde)
    {
        const bool isMap = (*dataPtr==0xde);
        dataPtr++;
        if (dataPtr>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        size_t size = uint16_t(*dataPtr++)<<8;
        size |= *dataPtr++;
        if (isMap)
            size<<=1;
        for (size_t i = 0; i < size; i++)
            skipMsgPackObject(dataPtr, dataEnd);
    }
    else if (*dataPtr == 0xdd || *dataPtr==0xdf)
    {
        const bool isMap = (*dataPtr==0xdf);
        dataPtr++;
        if (dataPtr>=dataEnd)
            throw ParseException("MsgPack: Can't skip object");
        size_t size = 0;
        for (cxint i = 24; i >= 0; i-=8)
            size |= (*dataPtr++)<<i;
        if (isMap)
            size<<=1;
        for (size_t i = 0; i < size; i++)
            skipMsgPackObject(dataPtr, dataEnd);
    }
}

//////////////////
MsgPackArrayParser::MsgPackArrayParser(const cxbyte*& _dataPtr, const cxbyte* _dataEnd)
        : dataPtr(_dataPtr), dataEnd(_dataEnd), count(0)
{
    if (dataPtr==dataEnd)
        throw ParseException("MsgPack: Can't parse array of elements");
    
    if (((*dataPtr) & 0xf0) == 0x90)
        count = (*dataPtr++) & 15;
    else
    {
        const cxbyte code = *dataPtr++;
        if (code == 0xdc)
        {
            if (dataPtr+1 >= dataEnd)
                throw ParseException("MsgPack: Can't parse array size");
            count = uint16_t(*dataPtr++)<<8;
            count |= *dataPtr++;
        }
        else if (code == 0xdd)
        {
            if (dataPtr+3 >= dataEnd)
                throw ParseException("MsgPack: Can't parse array size");
            for (cxint i = 24; i >= 0; i-=8)
                count |= uint32_t(*dataPtr++)<<i;
        }
        else
            throw ParseException("MsgPack: Can't parse array of elements");
    }
}

void MsgPackArrayParser::handleErrors()
{
    if (count == 0)
        throw ParseException("MsgPack: No left element to parse");
}

void MsgPackArrayParser::parseNil()
{
    handleErrors();
    parseMsgPackNil(dataPtr, dataEnd);
    count--;
}

bool MsgPackArrayParser::parseBool()
{
    handleErrors();
    auto v = parseMsgPackBool(dataPtr, dataEnd);
    count--;
    return v;
}

uint64_t MsgPackArrayParser::parseInteger(cxbyte signess)
{
    handleErrors();
    auto v = parseMsgPackInteger(dataPtr, dataEnd, signess);
    count--;
    return v;
}

double MsgPackArrayParser::parseFloat()
{
    handleErrors();
    auto v = parseMsgPackFloat(dataPtr, dataEnd);
    count--;
    return v;
}

std::string MsgPackArrayParser::parseString()
{
    handleErrors();
    auto v = parseMsgPackString(dataPtr, dataEnd);
    count--;
    return v;
}

Array<cxbyte> MsgPackArrayParser::parseData()
{
    handleErrors();
    auto v = parseMsgPackData(dataPtr, dataEnd);
    count--;
    return v;
}

MsgPackArrayParser MsgPackArrayParser::parseArray()
{
    handleErrors();
    auto v = MsgPackArrayParser(dataPtr, dataEnd);
    count--;
    return v;
}

MsgPackMapParser MsgPackArrayParser::parseMap()
{
    handleErrors();
    auto v = MsgPackMapParser(dataPtr, dataEnd);
    count--;
    return v;
}

size_t MsgPackArrayParser::end()
{
    for (size_t i = 0; i < count; i++)
        skipMsgPackObject(dataPtr, dataEnd);
    return count;
}

//////////////////
MsgPackMapParser::MsgPackMapParser(const cxbyte*& _dataPtr, const cxbyte* _dataEnd)
        : dataPtr(_dataPtr), dataEnd(_dataEnd), count(0), keyLeft(true)
{
    if (dataPtr==dataEnd)
        throw ParseException("MsgPack: Can't parse map");
    
    if (((*dataPtr) & 0xf0) == 0x80)
        count = (*dataPtr++) & 15;
    else
    {
        const cxbyte code = *dataPtr++;
        if (code == 0xde)
        {
            if (dataPtr+1 >= dataEnd)
                throw ParseException("MsgPack: Can't parse map size");
            count = uint16_t(*dataPtr++)<<8;
            count |= *dataPtr++;
        }
        else if (code == 0xdf)
        {
            if (dataPtr+3 >= dataEnd)
                throw ParseException("MsgPack: Can't parse map size");
            for (cxint i = 24; i >= 0; i-=8)
                count |= uint32_t(*dataPtr++)<<i;
        }
        else
            throw ParseException("MsgPack: Can't parse map");
    }
}

void MsgPackMapParser::handleErrors(bool key)
{
    if (count == 0)
        throw ParseException("MsgPack: No left element to parse");
    if (key && !keyLeft)
        throw ParseException("MsgPack: Key already parsed");
    if (!key && keyLeft)
        throw ParseException("MsgPack: This is not a value");
}

void MsgPackMapParser::parseKeyNil()
{
    handleErrors(true);
    parseMsgPackNil(dataPtr, dataEnd);
    keyLeft = false;
}

bool MsgPackMapParser::parseKeyBool()
{
    handleErrors(true);
    auto v = parseMsgPackBool(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

uint64_t MsgPackMapParser::parseKeyInteger(cxbyte signess)
{
    handleErrors(true);
    auto v = parseMsgPackInteger(dataPtr, dataEnd, signess);
    keyLeft = false;
    return v;
}

std::string MsgPackMapParser::parseKeyString()
{
    handleErrors(true);
    auto v = parseMsgPackString(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

Array<cxbyte> MsgPackMapParser::parseKeyData()
{
    handleErrors(true);
    auto v = parseMsgPackData(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

MsgPackArrayParser MsgPackMapParser::parseKeyArray()
{
    handleErrors(true);
    auto v = MsgPackArrayParser(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

MsgPackMapParser MsgPackMapParser::parseKeyMap()
{
    handleErrors(true);
    auto v = MsgPackMapParser(dataPtr, dataEnd);
    keyLeft = false;
    return v;
}

void MsgPackMapParser::parseValueNil()
{
    handleErrors(false);
    parseMsgPackNil(dataPtr, dataEnd);
    keyLeft = true;
    count--;
}

bool MsgPackMapParser::parseValueBool()
{
    handleErrors(false);
    auto v = parseMsgPackBool(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

uint64_t MsgPackMapParser::parseValueInteger(cxbyte signess)
{
    handleErrors(false);
    auto v = parseMsgPackInteger(dataPtr, dataEnd, signess);
    keyLeft = true;
    count--;
    return v;
}

std::string MsgPackMapParser::parseValueString()
{
    handleErrors(false);
    auto v = parseMsgPackString(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

Array<cxbyte> MsgPackMapParser::parseValueData()
{
    handleErrors(false);
    auto v = parseMsgPackData(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

MsgPackArrayParser MsgPackMapParser::parseValueArray()
{
    handleErrors(false);
    auto v = MsgPackArrayParser(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

MsgPackMapParser MsgPackMapParser::parseValueMap()
{
    handleErrors(false);
    auto v = MsgPackMapParser(dataPtr, dataEnd);
    keyLeft = true;
    count--;
    return v;
}

void MsgPackMapParser::skipValue()
{
    handleErrors(false);
    skipMsgPackObject(dataPtr, dataEnd);
    keyLeft = true;
    count--;
}

size_t MsgPackMapParser::end()
{
    if (!keyLeft)
        skipMsgPackObject(dataPtr, dataEnd);
    for (size_t i = 0; i < count; i++)
    {
        skipMsgPackObject(dataPtr, dataEnd);
        skipMsgPackObject(dataPtr, dataEnd);
    }
    return count;
}

template<typename T>
static void parseMsgPackValueTypedArrayForMap(MsgPackMapParser& map, T* out,
                                    size_t elemsNum, cxbyte signess)
{
    MsgPackArrayParser arrParser = map.parseValueArray();
    for (size_t i = 0; i < elemsNum; i++)
        out[i] = arrParser.parseInteger(signess);
    if (arrParser.haveElements())
        throw ParseException("Typed Array has too many elements");
}

enum {
    ROCMMP_ARG_ACCESS = 0, ROCMMP_ARG_ACTUAL_ACCESS, ROCMMP_ARG_ADDRESS_SPACE,
    ROCMMP_ARG_IS_CONST, ROCMMP_ARG_IS_PIPE, ROCMMP_ARG_IS_RESTRICT,
    ROCMMP_ARG_IS_VOLATILE, ROCMMP_ARG_NAME, ROCMMP_ARG_OFFSET, ROCMMP_ARG_POINTEE_ALIGN,
    ROCMMP_ARG_SIZE, ROCMMP_ARG_TYPE_NAME, ROCMMP_ARG_VALUE_KIND, ROCMMP_ARG_VALUE_TYPE
};

static const char* rocmMetadataMPKernelArgNames[] =
{
    ".access", ".actual_access", ".address_space", ".is_const", ".is_pipe", ".is_restrict",
    ".is_volatile", ".name", ".offset", ".pointee_align", ".size", ".type_name",
    ".value_kind", ".value_type"
};

static const size_t rocmMetadataMPKernelArgNamesSize =
                sizeof(rocmMetadataMPKernelArgNames) / sizeof(const char*);

static const char* rocmMPAccessQualifierTbl[] =
{ "read_only", "write_only", "read_write" };

static const std::pair<const char*, ROCmValueKind> rocmMPValueKindNamesMap[] =
{
    { "by_value", ROCmValueKind::BY_VALUE },
    { "dynamic_shared_pointer", ROCmValueKind::DYN_SHARED_PTR },
    { "global_buffer", ROCmValueKind::GLOBAL_BUFFER },
    { "hidden_completion_action", ROCmValueKind::HIDDEN_COMPLETION_ACTION },
    { "hidden_default_queue", ROCmValueKind::HIDDEN_DEFAULT_QUEUE },
    { "hidden_global_offset_x", ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X },
    { "hidden_global_offset_y", ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Y },
    { "hidden_global_offset_z", ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Z },
    { "hidden_multigrid_sync_arg", ROCmValueKind::HIDDEN_MULTIGRID_SYNC_ARG },
    { "hidden_none", ROCmValueKind::HIDDEN_NONE },
    { "hidden_printf_buffer", ROCmValueKind::HIDDEN_PRINTF_BUFFER },
    { "image", ROCmValueKind::IMAGE },
    { "pipe", ROCmValueKind::PIPE },
    { "queue", ROCmValueKind::QUEUE },
    { "sampler", ROCmValueKind::SAMPLER }
};

static const size_t rocmMPValueKindNamesNum =
        sizeof(rocmMPValueKindNamesMap) / sizeof(std::pair<const char*, ROCmValueKind>);

static const std::pair<const char*, ROCmValueType> rocmValueTypeNamesMap[] =
{
    { "F16", ROCmValueType::FLOAT16 },
    { "F32", ROCmValueType::FLOAT32 },
    { "F64", ROCmValueType::FLOAT64 },
    { "I16", ROCmValueType::INT16 },
    { "I32", ROCmValueType::INT32 },
    { "I64", ROCmValueType::INT64 },
    { "I8", ROCmValueType::INT8 },
    { "Struct", ROCmValueType::STRUCTURE },
    { "U16", ROCmValueType::UINT16 },
    { "U32", ROCmValueType::UINT32 },
    { "U64", ROCmValueType::UINT64 },
    { "U8", ROCmValueType::UINT8 }
};

static const size_t rocmValueTypeNamesNum =
        sizeof(rocmValueTypeNamesMap) / sizeof(std::pair<const char*, ROCmValueType>);

static const char* rocmMPAddrSpaceTypesTbl[] =
{ "private", "global", "constant", "local", "generic", "region" };

static void parseROCmMetadataKernelArgMsgPack(MsgPackArrayParser& argsParser,
                        ROCmKernelArgInfo& argInfo)
{
    MsgPackMapParser aParser = argsParser.parseMap();
    while (aParser.haveElements())
    {
        const std::string name = aParser.parseKeyString();
        const size_t index = binaryFind(rocmMetadataMPKernelArgNames,
                    rocmMetadataMPKernelArgNames + rocmMetadataMPKernelArgNamesSize,
                    name.c_str(), CStringLess()) - rocmMetadataMPKernelArgNames;
        switch(index)
        {
            case ROCMMP_ARG_ACCESS:
            case ROCMMP_ARG_ACTUAL_ACCESS:
            {
                const std::string acc = trimStrSpaces(aParser.parseValueString());
                size_t accIndex = 0;
                for (; accIndex < 3; accIndex++)
                    if (::strcmp(rocmMPAccessQualifierTbl[accIndex], acc.c_str())==0)
                        break;
                if (accIndex == 3)
                    throw ParseException("Wrong access qualifier");
                if (index == ROCMMP_ARG_ACCESS)
                    argInfo.accessQual = ROCmAccessQual(accIndex+1);
                else
                    argInfo.actualAccessQual = ROCmAccessQual(accIndex+1);
                break;
            }
            case ROCMMP_ARG_ADDRESS_SPACE:
            {
                const std::string aspace = trimStrSpaces(aParser.parseValueString());
                size_t aspaceIndex = 0;
                for (; aspaceIndex < 6; aspaceIndex++)
                    if (::strcasecmp(rocmMPAddrSpaceTypesTbl[aspaceIndex],
                                aspace.c_str())==0)
                        break;
                if (aspaceIndex == 6)
                    throw ParseException("Wrong address space");
                argInfo.addressSpace = ROCmAddressSpace(aspaceIndex+1);
                break;
            }
            case ROCMMP_ARG_IS_CONST:
                argInfo.isConst = aParser.parseValueBool();
                break;
            case ROCMMP_ARG_IS_PIPE:
                argInfo.isPipe = aParser.parseValueBool();
                break;
            case ROCMMP_ARG_IS_RESTRICT:
                argInfo.isRestrict = aParser.parseValueBool();
                break;
            case ROCMMP_ARG_IS_VOLATILE:
                argInfo.isVolatile = aParser.parseValueBool();
                break;
            case ROCMMP_ARG_NAME:
                argInfo.name = aParser.parseValueString();
                break;
            case ROCMMP_ARG_OFFSET:
                argInfo.offset = aParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_ARG_POINTEE_ALIGN:
                argInfo.pointeeAlign = aParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_ARG_SIZE:
                argInfo.size = aParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_ARG_TYPE_NAME:
                argInfo.typeName = aParser.parseValueString();
                break;
            case ROCMMP_ARG_VALUE_KIND:
            {
                const std::string vkind = trimStrSpaces(aParser.parseValueString());
                const size_t vkindIndex = binaryMapFind(rocmMPValueKindNamesMap,
                            rocmMPValueKindNamesMap + rocmMPValueKindNamesNum, vkind.c_str(),
                            CStringLess()) - rocmMPValueKindNamesMap;
                    // if unknown kind
                    if (vkindIndex == rocmMPValueKindNamesNum)
                        throw ParseException("Wrong argument value kind");
                    argInfo.valueKind = rocmMPValueKindNamesMap[vkindIndex].second;
                break;
            }
            case ROCMMP_ARG_VALUE_TYPE:
            {
                const std::string vtype = trimStrSpaces(aParser.parseValueString());
                const size_t vtypeIndex = binaryMapFind(rocmValueTypeNamesMap,
                        rocmValueTypeNamesMap + rocmValueTypeNamesNum, vtype.c_str(),
                        CStringCaseLess()) - rocmValueTypeNamesMap;
                // if unknown type
                if (vtypeIndex == rocmValueTypeNamesNum)
                    throw ParseException("Wrong argument value type");
                argInfo.valueType = rocmValueTypeNamesMap[vtypeIndex].second;
                break;
            }
            default:
                aParser.skipValue();
                break;
        }
    }
};

enum {
    ROCMMP_KERNEL_ARGS = 0, ROCMMP_KERNEL_DEVICE_ENQUEUE_SYMBOL,
    ROCMMP_KERNEL_GROUP_SEGMENT_FIXED_SIZE, ROCMMP_KERNEL_KERNARG_SEGMENT_ALIGN,
    ROCMMP_KERNEL_KERNARG_SEGMENT_SIZE, ROCMMP_KERNEL_LANGUAGE,
    ROCMMP_KERNEL_LANGUAGE_VERSION, ROCMMP_KERNEL_MAX_FLAT_WORKGROUP_SIZE,
    ROCMMP_KERNEL_NAME, ROCMMP_KERNEL_PRIVATE_SEGMENT_FIXED_SIZE,
    ROCMMP_KERNEL_REQD_WORKGROUP_SIZE, ROCMMP_KERNEL_SGPR_COUNT,
    ROCMMP_KERNEL_SGPR_SPILL_COUNT, ROCMMP_KERNEL_SYMBOL,
    ROCMMP_KERNEL_VEC_TYPE_HINT, ROCMMP_KERNEL_VGPR_COUNT,
    ROCMMP_KERNEL_VGPR_SPILL_COUNT, ROCMMP_KERNEL_WAVEFRONT_SIZE,
    ROCMMP_KERNEL_WORKGROUP_SIZE_HINT
};

static const char* rocmMetadataMPKernelNames[] =
{
    ".args", ".device_enqueue_symbol", ".group_segment_fixed_size", ".kernarg_segment_align",
    ".kernarg_segment_size", ".language", ".language_version", ".max_flat_workgroup_size",
    ".name", ".private_segment_fixed_size", ".reqd_workgroup_size", ".sgpr_count",
    ".sgpr_spill_count", ".symbol", ".vec_type_hint", ".vgpr_count", ".vgpr_spill_count",
    ".wavefront_size", ".workgroup_size_hint"
};

static const size_t rocmMetadataMPKernelNamesSize = sizeof(rocmMetadataMPKernelNames) /
                    sizeof(const char*);

static void parseROCmMetadataKernelMsgPack(MsgPackArrayParser& kernelsParser,
                        ROCmKernelMetadata& kernel)
{
    MsgPackMapParser kParser = kernelsParser.parseMap();
    while (kParser.haveElements())
    {
        const std::string name = kParser.parseKeyString();
        const size_t index = binaryFind(rocmMetadataMPKernelNames,
                    rocmMetadataMPKernelNames + rocmMetadataMPKernelNamesSize,
                    name.c_str(), CStringLess()) - rocmMetadataMPKernelNames;
        
        switch(index)
        {
            case ROCMMP_KERNEL_ARGS:
            {
                MsgPackArrayParser argsParser = kParser.parseValueArray();
                while (argsParser.haveElements())
                {
                    ROCmKernelArgInfo arg{};
                    parseROCmMetadataKernelArgMsgPack(argsParser, arg);
                    kernel.argInfos.push_back(arg);
                }
                break;
            }
            case ROCMMP_KERNEL_DEVICE_ENQUEUE_SYMBOL:
                kernel.deviceEnqueueSymbol = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_GROUP_SEGMENT_FIXED_SIZE:
                kernel.groupSegmentFixedSize = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_KERNARG_SEGMENT_ALIGN:
                kernel.kernargSegmentAlign = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_KERNARG_SEGMENT_SIZE:
                kernel.kernargSegmentSize = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_LANGUAGE:
                kernel.language = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_LANGUAGE_VERSION:
                parseMsgPackValueTypedArrayForMap(kParser, kernel.langVersion,
                                        2, MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_MAX_FLAT_WORKGROUP_SIZE:
                kernel.maxFlatWorkGroupSize = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_NAME:
                kernel.name = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_PRIVATE_SEGMENT_FIXED_SIZE:
                kernel.privateSegmentFixedSize = kParser.
                                    parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_REQD_WORKGROUP_SIZE:
                parseMsgPackValueTypedArrayForMap(kParser, kernel.reqdWorkGroupSize,
                                        3, MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_SGPR_COUNT:
                kernel.sgprsNum = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_SGPR_SPILL_COUNT:
                kernel.spilledSgprs = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_SYMBOL:
                kernel.symbolName = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_VEC_TYPE_HINT:
                kernel.vecTypeHint = kParser.parseValueString();
                break;
            case ROCMMP_KERNEL_VGPR_COUNT:
                kernel.vgprsNum = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_VGPR_SPILL_COUNT:
                kernel.spilledVgprs = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_WAVEFRONT_SIZE:
                kernel.wavefrontSize = kParser.parseValueInteger(MSGPACK_WS_UNSIGNED);
                break;
            case ROCMMP_KERNEL_WORKGROUP_SIZE_HINT:
                parseMsgPackValueTypedArrayForMap(kParser, kernel.workGroupSizeHint,
                                        3, MSGPACK_WS_UNSIGNED);
                break;
            default:
                kParser.skipValue();
                break;
        }
    }
}

void CLRX::parseROCmMetadataMsgPack(size_t metadataSize, const cxbyte* metadata,
                ROCmMetadata& metadataInfo)
{
    // init metadata info object
    metadataInfo.kernels.clear();
    metadataInfo.printfInfos.clear();
    metadataInfo.version[0] = metadataInfo.version[1] = 0;
    
    std::vector<ROCmKernelMetadata>& kernels = metadataInfo.kernels;
    
    MsgPackMapParser mainMap(metadata, metadata+metadataSize);
    while (mainMap.haveElements())
    {
        const CString name = mainMap.parseKeyString();
        if (name == "amdhsa.version")
            parseMsgPackValueTypedArrayForMap(mainMap, metadataInfo.version,
                                        2, MSGPACK_WS_UNSIGNED);
        else if (name == "amdhsa.kernels")
        {
            MsgPackArrayParser kernelsParser = mainMap.parseValueArray();
            while (kernelsParser.haveElements())
            {
                ROCmKernelMetadata kernel{};
                kernel.initialize();
                parseROCmMetadataKernelMsgPack(kernelsParser, kernel);
                kernels.push_back(kernel);
            }
        }
        else if (name == "amdhsa.printf")
        {
            std::unordered_set<cxuint> printfIds;
            MsgPackArrayParser printfsParser = mainMap.parseValueArray();
            while (printfsParser.haveElements())
            {
                ROCmPrintfInfo printfInfo{};
                std::string pistr = printfsParser.parseString();
                parsePrintfInfoString(pistr.c_str(), pistr.c_str() + pistr.size(),
                                0, 0, printfInfo, printfIds);
                metadataInfo.printfInfos.push_back(printfInfo);
            }
        }
        else
            mainMap.skipValue();
    }
}

void ROCmMetadata::parseMsgPack(size_t metadataSize, const cxbyte* metadata)
{
    parseROCmMetadataMsgPack(metadataSize, metadata, *this);
}

static void msgPackWriteString(const char* str, std::vector<cxbyte>& output)
{
    const size_t len = ::strlen(str);
    if (len < 32)
        output.push_back(0xa0 + len);
    else if (len < 256)
    {
        cxbyte v[2];
        v[0] = 0xd9;
        v[1] = len;
        output.insert(output.end(), v, v+2);
    }
    else if (len < 0x10000U)
    {
        cxbyte v[3];
        v[0] = 0xda;
        v[1] = len>>8;
        v[2] = len&0xff;
        output.insert(output.end(), v, v+3);
    }
    else
    {
        cxbyte v[5];
        v[0] = 0xdb;
        v[1] = len>>24;
        v[2] = (len>>16)&0xff;
        v[3] = (len>>8)&0xff;
        v[4] = len&0xff;
        output.insert(output.end(), v, v+5);
    }
    output.insert(output.end(), reinterpret_cast<const cxbyte*>(str),
                  reinterpret_cast<const cxbyte*>(str+len));
}

static inline void msgPackWriteBool(bool b, std::vector<cxbyte>& output)
{
    output.push_back(b ? 0xc3 : 0xc2);
}

static void msgPackWriteUInt(uint64_t v, std::vector<cxbyte>& output)
{
    if (v < 128)
        output.push_back(cxbyte(v));
    else if (v < 256)
    {
        cxbyte d[2];
        d[0] = 0xcc;
        d[1] = cxbyte(v);
        output.insert(output.end(), d, d+2);
    }
    else if (v < 0x10000U)
    {
        cxbyte d[3];
        d[0] = 0xcd;
        d[1] = v>>8;
        d[2] = v&0xff;
        output.insert(output.end(), d, d+3);
    }
    else if (v < 0x100000000ULL)
    {
        cxbyte d[5];
        d[0] = 0xce;
        uint64_t v2 = v;
        for (cxuint i=5; i >= 0; i--, v2>>=8)
            d[i] = v2&0xff;
        output.insert(output.end(), d, d+5);
    }
    else
    {
        cxbyte d[9];
        d[0] = 0xcf;
        uint64_t v2 = v;
        for (cxuint i=9; i >= 0; i--, v2>>=8)
            d[i] = v2&0xff;
        output.insert(output.end(), d, d+9);
    }
}

MsgPackArrayWriter::MsgPackArrayWriter(size_t _elemsNum,
            std::vector<cxbyte>& _output) : output(_output), elemsNum(_elemsNum), count(0)
{
    if (elemsNum < 16)
        output.push_back(0x90 + elemsNum);
    else if (elemsNum < 0x10000U)
    {
        cxbyte d[3];
        d[0] = 0xdc;
        d[1] = elemsNum>>8;
        d[2] = elemsNum&0xff;
        output.insert(output.end(), d, d+3);
    }
    else
    {
        cxbyte d[5];
        d[0] = 0xdd;
        uint32_t v2 = elemsNum;
        for (cxint i=4; i > 0; i--, v2>>=8)
            d[i] = v2&0xff;
        output.insert(output.end(), d, d+5);
    }
}

void MsgPackArrayWriter::putBool(bool b)
{
    if (count == elemsNum)
        throw BinException("MsgPack: Too many array elements");
    count++;
    msgPackWriteBool(b, output);
}

void MsgPackArrayWriter::putString(const char* str)
{
    if (count == elemsNum)
        throw BinException("MsgPack: Too many array elements");
    count++;
    msgPackWriteString(str, output);
}

void MsgPackArrayWriter::putUInt(uint64_t v)
{
    if (count == elemsNum)
        throw BinException("MsgPack: Too many array elements");
    count++;
    msgPackWriteUInt(v, output);
}

MsgPackArrayWriter MsgPackArrayWriter::putArray(size_t aelemsNum)
{
    if (count == elemsNum)
        throw BinException("MsgPack: Too many array elements");
    count++;
    return MsgPackArrayWriter(aelemsNum, output);
}

MsgPackMapWriter MsgPackArrayWriter::putMap(size_t melemsNum)
{
    if (count == elemsNum)
        throw BinException("MsgPack: Too many array elements");
    count++;
    return MsgPackMapWriter(melemsNum, output);
}

MsgPackMapWriter::MsgPackMapWriter(size_t _elemsNum, std::vector<cxbyte>& _output)
        : output(_output), elemsNum(_elemsNum), count(0), inKey(true)
{
    if (elemsNum < 16)
        output.push_back(0x80 + elemsNum);
    else if (elemsNum < 0x10000U)
    {
        cxbyte d[3];
        d[0] = 0xde;
        d[1] = elemsNum>>8;
        d[2] = elemsNum&0xff;
        output.insert(output.end(), d, d+3);
    }
    else
    {
        cxbyte d[5];
        d[0] = 0xdf;
        uint32_t v2 = elemsNum;
        for (cxint i=4; i > 0; i--, v2>>=8)
            d[i] = v2&0xff;
        output.insert(output.end(), d, d+5);
    }
}

void MsgPackMapWriter::putKeyString(const char* str)
{
    if (!inKey)
        throw BinException("MsgPack: Not in key value");
    inKey = false;
    if (count == elemsNum)
        throw BinException("MsgPack: Too many map elements");
    count++;
    msgPackWriteString(str, output);
}

void MsgPackMapWriter::putValueBool(bool b)
{
    if (inKey)
        throw BinException("MsgPack: Not in value value");
    inKey = true;
    msgPackWriteBool(b, output);
}

void MsgPackMapWriter::putValueString(const char* str)
{
    if (inKey)
        throw BinException("MsgPack: Not in value value");
    inKey = true;
    msgPackWriteString(str, output);
}

void MsgPackMapWriter::putValueUInt(uint64_t v)
{
    if (inKey)
        throw BinException("MsgPack: Not in value value");
    inKey = true;
    msgPackWriteUInt(v, output);
}

MsgPackArrayWriter MsgPackMapWriter::putValueArray(size_t aelemsNum)
{
    if (inKey)
        throw BinException("MsgPack: Not in value value");
    inKey = true;
    return MsgPackArrayWriter(aelemsNum, output);
}

std::vector<cxbyte>& MsgPackMapWriter::putValueElement()
{
    if (inKey)
        throw BinException("MsgPack: Not in value value");
    inKey = true;
    return output;
}

MsgPackMapWriter MsgPackMapWriter::putValueMap(size_t  melemsNum)
{
    if (inKey)
        throw BinException("MsgPack: Not in value value");
    inKey = true;
    return MsgPackMapWriter(melemsNum, output);
}

static const char* rocmMPValueKindNames[] =
{
    "by_value", "global_buffer", "dynamic_shared_pointer", "sampler", "image", "pipe",
    "queue", "hidden_Global_offset_x", "hidden_global_offset_y",
    "hidden_global_offset_z", "hidden_none", "hidden_printf_buffer",
    "hidden_default_queue", "hidden_completion_action", "hidden_multigrid_sync_arg"
};

static const char* rocmMPValueTypeNames[] =
{
    "struct", "i8", "u8", "i16", "u16", "f16", "i32", "u32", "f32", "i64", "u64", "f64"
};

// helper for checking whether value is supplied
static inline bool hasValue(cxuint value)
{ return value!=BINGEN_NOTSUPPLIED && value!=BINGEN_DEFAULT; }

static inline bool hasValue(uint64_t value)
{ return value!=BINGEN64_NOTSUPPLIED && value!=BINGEN64_DEFAULT; }

void CLRX::generateROCmMetadataMsgPack(const ROCmMetadata& mdInfo,
                    const ROCmKernelDescriptor** kdescs, std::vector<cxbyte>& output)
{
    output.clear();
    MsgPackArrayWriter kernelsWriter(mdInfo.kernels.size(), output);
    for (size_t i = 0; i < mdInfo.kernels.size(); i++)
    {
        const ROCmKernelMetadata& kernelMD = mdInfo.kernels[i];
        const size_t mapSize = 13 + (!kernelMD.deviceEnqueueSymbol.empty()) +
                (kernelMD.reqdWorkGroupSize[0]!=0 ||
                 kernelMD.reqdWorkGroupSize[1]!=0 ||
                 kernelMD.reqdWorkGroupSize[2]!=0) +
                (!kernelMD.vecTypeHint.empty()) +
                (kernelMD.workGroupSizeHint[0]!=0 ||
                 kernelMD.workGroupSizeHint[1]!=0 ||
                 kernelMD.workGroupSizeHint[2]!=0) +
                (!kernelMD.language.empty()) +
                (kernelMD.langVersion[0]!=BINGEN_NOTSUPPLIED);
        MsgPackMapWriter kwriter = kernelsWriter.putMap(mapSize);
        kwriter.putKeyString(".args");
        // kernel arguments
        {
        MsgPackArrayWriter kargsWriter = kwriter.putValueArray(kernelMD.argInfos.size());
        for (const ROCmKernelArgInfo& arg: kernelMD.argInfos)
        {
            const bool hasAccess = (arg.accessQual != ROCmAccessQual::DEFAULT &&
                     (arg.valueKind==ROCmValueKind::IMAGE ||
                      arg.valueKind==ROCmValueKind::PIPE));
            const bool hasActualAccess = (
                     arg.actualAccessQual != ROCmAccessQual::DEFAULT &&
                     (arg.valueKind==ROCmValueKind::GLOBAL_BUFFER ||
                      arg.valueKind==ROCmValueKind::IMAGE ||
                      arg.valueKind==ROCmValueKind::PIPE));
            const bool hasAddrSpace = (arg.addressSpace != ROCmAddressSpace::NONE &&
                     (arg.valueKind==ROCmValueKind::GLOBAL_BUFFER ||
                      arg.valueKind==ROCmValueKind::DYN_SHARED_PTR));
            const size_t amapSize = 4 + (arg.isConst) + (arg.isPipe) +
                    (arg.isRestrict) + (arg.isVolatile) +
                    (!arg.name.empty()) + (!arg.typeName.empty()) +
                     hasAddrSpace + hasAccess + hasActualAccess + (arg.pointeeAlign!=0);
            MsgPackMapWriter argWriter = kargsWriter.putMap(amapSize);
            if (hasAccess)
            {
                if (arg.accessQual > ROCmAccessQual::MAX_VALUE)
                    throw BinGenException("Unknown AccessQualifier");
                argWriter.putKeyString(".access");
                argWriter.putValueString(
                        rocmMPAccessQualifierTbl[cxuint(arg.accessQual)-1]);
            }
            if (hasActualAccess)
            {
                if (arg.actualAccessQual > ROCmAccessQual::MAX_VALUE)
                    throw BinGenException("Unknown ActualAccessQualifier");
                argWriter.putKeyString(".actual_access");
                argWriter.putValueString(
                        rocmMPAccessQualifierTbl[cxuint(arg.actualAccessQual)-1]);
            }
            if (hasAddrSpace)
            {
                if (arg.addressSpace > ROCmAddressSpace::MAX_VALUE ||
                    arg.addressSpace == ROCmAddressSpace::NONE)
                    throw BinGenException("Unknown AddressSpace");
                argWriter.putKeyString(".address_space");
                argWriter.putValueString(
                        rocmMPAddrSpaceTypesTbl[cxuint(arg.addressSpace)-1]);
            }
            if (arg.isConst)
            {
                argWriter.putKeyString(".is_const");
                argWriter.putValueBool(true);
            }
            if (arg.isPipe)
            {
                argWriter.putKeyString(".is_pipe");
                argWriter.putValueBool(true);
            }
            if (arg.isRestrict)
            {
                argWriter.putKeyString(".is_restrict");
                argWriter.putValueBool(true);
            }
            if (arg.isVolatile)
            {
                argWriter.putKeyString(".is_volatile");
                argWriter.putValueBool(true);
            }
            if (!arg.name.empty())
            {
                argWriter.putKeyString(".name");
                argWriter.putValueString(arg.name.c_str());
            }
            argWriter.putKeyString(".offset");
            argWriter.putValueUInt(arg.offset);
            if (arg.pointeeAlign!=0)
            {
                argWriter.putKeyString(".pointee_align");
                argWriter.putValueUInt(arg.pointeeAlign);
            }
            argWriter.putKeyString(".size");
            argWriter.putValueUInt(arg.size);
            if (!arg.typeName.empty())
            {
                argWriter.putKeyString(".type_name");
                argWriter.putValueString(arg.typeName.c_str());
            }
            
            if (arg.valueKind > ROCmValueKind::MAX_VALUE)
                throw BinGenException("Unknown ValueKind");
            argWriter.putKeyString(".value_kind");
            argWriter.putValueString(rocmMPValueKindNames[cxuint(arg.valueKind)]);
            
            if (arg.valueType > ROCmValueType::MAX_VALUE)
                throw BinGenException("Unknown ValueType");
            argWriter.putKeyString(".value_type");
            argWriter.putValueString(rocmMPValueTypeNames[cxuint(arg.valueType)]);
        }
        } //
        if (!kernelMD.deviceEnqueueSymbol.empty())
        {
            kwriter.putKeyString(".device_enqueue_symbol");
            kwriter.putValueString(kernelMD.deviceEnqueueSymbol.c_str());
        }
        
        const ROCmKernelDescriptor& kdesc = *(kdescs[i]);
        
        kwriter.putKeyString(".group_segment_fixed_size");
        kwriter.putValueUInt(hasValue(kernelMD.groupSegmentFixedSize) ?
                kernelMD.groupSegmentFixedSize : ULEV(kdesc.groupSegmentFixedSize));
        kwriter.putKeyString(".kernarg_segment_align");
        kwriter.putValueUInt(kernelMD.kernargSegmentAlign);
        kwriter.putKeyString(".kernarg_segment_size");
        kwriter.putValueUInt(kernelMD.kernargSegmentSize);
        
        if (!kernelMD.language.empty())
        {
            kwriter.putKeyString(".language");
            kwriter.putValueString(kernelMD.language.c_str());
        }
        if (kernelMD.langVersion[0]!=BINGEN_NOTSUPPLIED)
        {
            kwriter.putKeyString(".language_version");
            MsgPackArrayWriter verWriter = kwriter.putValueArray(2);
            verWriter.putUInt(kernelMD.langVersion[0]);
            verWriter.putUInt(kernelMD.langVersion[1]);
        }
        
        kwriter.putKeyString(".max_flat_workgroup_size");
        kwriter.putValueUInt(kernelMD.maxFlatWorkGroupSize);
        kwriter.putKeyString(".name");
        kwriter.putValueString(kernelMD.name.c_str());
        kwriter.putKeyString(".private_segment_fixed_size");
        kwriter.putValueUInt(hasValue(kernelMD.privateSegmentFixedSize) ?
                kernelMD.privateSegmentFixedSize : ULEV(kdesc.privateSegmentFixedSize));
        
        if (kernelMD.reqdWorkGroupSize[0] != 0 || kernelMD.reqdWorkGroupSize[1] != 0 ||
            kernelMD.reqdWorkGroupSize[2] != 0)
        {
            kwriter.putKeyString(".reqd_workgroup_size");
            MsgPackArrayWriter rwriter = kwriter.putValueArray(3);
            for (cxuint i = 0; i < 3; i++)
                rwriter.putUInt(kernelMD.reqdWorkGroupSize[i]);
        }
        
        kwriter.putKeyString(".sgpr_count");
        kwriter.putValueUInt(kernelMD.sgprsNum);
        kwriter.putKeyString(".sgpr_spill_count");
        kwriter.putValueUInt(kernelMD.spilledSgprs);
        if (!kernelMD.vecTypeHint.empty())
        {
            kwriter.putKeyString(".vec_type_hint");
            kwriter.putValueString(kernelMD.vecTypeHint.c_str());
        }
        kwriter.putKeyString(".vgpr_count");
        kwriter.putValueUInt(kernelMD.vgprsNum);
        kwriter.putKeyString(".vgpr_spill_count");
        kwriter.putValueUInt(kernelMD.spilledVgprs);
        kwriter.putKeyString(".wavefront_size");
        kwriter.putValueUInt(kernelMD.wavefrontSize);
        
        if (kernelMD.workGroupSizeHint[0] != 0 || kernelMD.workGroupSizeHint[1] != 0 ||
            kernelMD.workGroupSizeHint[2] != 0)
        {
            kwriter.putKeyString(".workgroup_size_hint");
            MsgPackArrayWriter rwriter = kwriter.putValueArray(3);
            for (cxuint i = 0; i < 3; i++)
                rwriter.putUInt(kernelMD.workGroupSizeHint[i]);
        }
    }
}
