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

#include <CLRX/Config.h>
//#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"
#include "GCNInternals.h"

using namespace CLRX;

static std::once_flag clrxGCNAssemblerOnceFlag;
static Array<GCNAsmInstruction> gcnInstrSortedTable;

static void initializeGCNAssembler()
{
    size_t tableSize = 0;
    while (gcnInstrsTable[tableSize].mnemonic!=nullptr)
        tableSize++;
    gcnInstrSortedTable.resize(tableSize);
    for (cxuint i = 0; i < tableSize; i++)
    {
        const GCNInstruction& insn = gcnInstrsTable[i];
        gcnInstrSortedTable[i] = {insn.mnemonic, insn.encoding, GCNENC_NONE, insn.mode,
                    insn.code, UINT16_MAX, insn.archMask};
    }
    
    std::sort(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
            [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
            {   // compare mnemonic and if mnemonic
                int r = ::strcmp(instr1.mnemonic, instr2.mnemonic);
                return (r < 0) || (r==0 && instr1.encoding1 < instr2.encoding1) ||
                            (r == 0 && instr1.encoding1 == instr2.encoding1 &&
                             instr1.archMask < instr2.archMask);
            });
    
    cxuint j = 0;
    /* join VOP3A instr with VOP2/VOPC/VOP1 instr together to faster encoding. */
    for (cxuint i = 0; i < tableSize; i++)
    {   
        GCNAsmInstruction insn = gcnInstrSortedTable[i];
        if ((insn.encoding1 == GCNENC_VOP3A || insn.encoding1 == GCNENC_VOP3B))
        {   // check duplicates
            cxuint k = j-1;
            while (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                    (gcnInstrSortedTable[k].archMask & insn.archMask)!=insn.archMask) k--;
            
            if (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                (gcnInstrSortedTable[k].archMask & insn.archMask)==insn.archMask)
            {   // we found duplicate, we apply
                if (gcnInstrSortedTable[k].code2==UINT16_MAX)
                {   // if second slot for opcode is not filled
                    gcnInstrSortedTable[k].code2 = insn.code1;
                    gcnInstrSortedTable[k].encoding2 = insn.encoding1;
                    gcnInstrSortedTable[k].archMask &= insn.archMask;
                }
                else
                {   // if filled we create new entry
                    gcnInstrSortedTable[j] = gcnInstrSortedTable[k];
                    gcnInstrSortedTable[j].archMask &= insn.archMask;
                    gcnInstrSortedTable[j].encoding2 = insn.encoding1;
                    gcnInstrSortedTable[j++].code2 = insn.code1;
                }
            }
            else // not found
                gcnInstrSortedTable[j++] = insn;
        }
        else // normal instruction
            gcnInstrSortedTable[j++] = insn;
    }
    gcnInstrSortedTable.resize(j); // final size
    /* for (const GCNAsmInstruction& instr: gcnInstrSortedTable)
        std::cout << "{ " << instr.mnemonic << ", " << cxuint(instr.encoding1) << ", " <<
                cxuint(instr.encoding2) <<
                std::hex << ", 0x" << instr.mode << ", 0x" << instr.code1 << ", 0x" <<
                instr.code2 << std::dec << ", " << instr.archMask << " }" << std::endl;*/
}

GCNAssembler::GCNAssembler(Assembler& assembler): ISAAssembler(assembler),
        sgprsNum(0), vgprsNum(0), curArchMask(
                    1U<<cxuint(getGPUArchitectureFromDeviceType(assembler.getDeviceType())))
{
    std::call_once(clrxGCNAssemblerOnceFlag, initializeGCNAssembler);
}

GCNAssembler::~GCNAssembler()
{ }

namespace CLRX
{
    
std::pair<uint16_t, uint16_t> GCNAsmUtils::parseVRegRange(Assembler& asmr,
              const char*& linePtr, bool required)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* vgprRangePlace = linePtr;
    if (linePtr == end)
    {
        if (required)
            asmr.printError(vgprRangePlace, "VRegister range is required");
        return std::make_pair(0, 0);
    }
    if (toLower(*linePtr) != 'v') // if
    {
        if (required)
            asmr.printError(vgprRangePlace, "VRegister range is required");
        return std::make_pair(0, 0);
    }
    if (++linePtr == end)
    {
        if (required)
            asmr.printError(vgprRangePlace, "VRegister range is required");
        return std::make_pair(0, 0);
    }
    
    try /* for handling parse exception */
    {
    if (isDigit(*vgprRangePlace))
    {   // if single register
        cxuint value = cstrtovCStyle<cxuint>(linePtr, end, linePtr);
        if (value >= 256)
        {
            asmr.printError(vgprRangePlace, "VRegister number of out range (0-255)");
            std::make_pair(0, 0);
        }
        return std::make_pair(256+value, 256+value+1);
    }
    else if (*linePtr == '[')
    {   // many registers
        ++linePtr;
        cxuint value1, value2;
        skipSpacesToEnd(linePtr, end);
        value1 = cstrtovCStyle<cxuint>(linePtr, end, linePtr);
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ':')
        {   // error
            asmr.printError(vgprRangePlace, "Unterminated VRegister range");
            return std::make_pair(0, 0);
        }
        ++linePtr;
        skipSpacesToEnd(linePtr, end);
        value2 = cstrtovCStyle<cxuint>(linePtr, end, linePtr);
        if (value2 <= value1 || value2 >= 256 || value1 >= 256)
        {   // error (illegal register range)
            asmr.printError(vgprRangePlace, "Illegal VRegister range");
            return std::make_pair(0, 0);
        }
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ']')
        {   // error
            asmr.printError(vgprRangePlace, "Unterminated VRegister range");
            return std::make_pair(0, 0);
        }
        return std::make_pair(256+value1, 256+value2+1);
    }
    } catch(const ParseException& ex)
    { asmr.printError(linePtr, ex.what()); }
    
    return std::make_pair(0, 0);
}

std::pair<uint16_t, uint16_t> GCNAsmUtils::parseSRegRange(Assembler& asmr,
              const char*& linePtr, uint16_t arch, bool required)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* sgprRangePlace = linePtr;
    if (linePtr == end)
    {
        if (required)
            asmr.printError(sgprRangePlace, "SRegister range is required");
        return std::make_pair(0, 0);
    }
    
    try
    {
    if (toLower(*linePtr) != 's') // if
    {
        char regName[20];
        if (!getNameArg(asmr, 20, regName, linePtr, "register name", required))
            return std::make_pair(0, 0);
        toLowerString(regName);
        
        if (regName[0] == 'v' && regName[1] == 'c' && regName[2] == 'c')
        {   /// vcc
            if (regName[3] == '_')
            {
                if (regName[4] == 'l' && regName[5] == 'o' && regName[6] == 0)
                    return std::make_pair(106, 107);
                if (regName[4] == 'h' && regName[5] == 'i' && regName[6] == 0)
                    return std::make_pair(107, 108);
            }
            else if (regName[3] == 0)
                return std::make_pair(106, 108);
        }
        else if (regName[0] == 'e' && regName[1] == 'x' && regName[2] == 'e' &&
            regName[3] == 'c')
        {   /* exec* */
            if (regName[4] == '_')
            {
                if (regName[5] == 'l' && regName[6] == 'o' && regName[7] == 0)
                    return std::make_pair(126, 127);
                if (regName[5] == 'h' && regName[6] == 'i' && regName[7] == 0)
                    return std::make_pair(127, 128);
            }
            else if (regName[4] == 0)
                return std::make_pair(126, 128);
        }
        else if (regName[0]=='t')
        {   /* tma,tba, ttmpX */
            if (regName[1] == 'b' && regName[2] == 'a')
            {
                if (regName[3] == '_')
                {
                    if (regName[4] == 'l' && regName[5] == 'o' && regName[6] == 0)
                        return std::make_pair(108, 109);
                    if (regName[4] == 'h' && regName[5] == 'i' && regName[6] == 0)
                        return std::make_pair(109, 110);
                }
                else if (regName[3] == 0)
                    return std::make_pair(108, 110);
            }
            else if (regName[1] == 'm' && regName[2] == 'a')
            {
                if (regName[3] == '_')
                {
                    if (regName[4] == 'l' && regName[5] == 'o' && regName[6] == 0)
                        return std::make_pair(110, 111);
                    if (regName[4] == 'h' && regName[5] == 'i' && regName[6] == 0)
                        return std::make_pair(111, 112);
                }
                else if (regName[4] == 0)
                    return std::make_pair(110, 112);
            }
            else if (regName[1] == 't' && regName[2] == 'm' && regName[3] == 'p')
            {
                cxbyte value = cstrtovCStyle<cxbyte>(linePtr, end, linePtr);
                if (value > 11)
                {
                    asmr.printError(linePtr, "TTMP register number out of range");
                    return std::make_pair(0, 0);
                }
            }
        }
        else if (regName[0] == 'm' || regName[1] == '0' || regName[2] == 0)
            return std::make_pair(124, 125);
        else if (arch&ARCH_GCN_1_1_2)
        {
            if (::memcmp(regName, "flat_scratch", 12)==0)
            {   // flat
            }
            //if ((arch&ARCH_RX3X0) && ::memcmp(regName, "flat_scratch", 12)==0
        }
        /*if (required)
            asmr.printError(sgprRangePlace, "SRegister range is required");
        return std::make_pair(0, 0);*/
        /* check other codes */
        
    }
    if (++linePtr == end)
    {
        if (required)
            asmr.printError(sgprRangePlace, "SRegister range is required");
        return std::make_pair(0, 0);
    }
    
    if (isDigit(*linePtr))
    {   // if single register
        cxuint value = cstrtovCStyle<cxuint>(linePtr, end, linePtr);
        return std::make_pair(value, value+1);
    }
    else if (*linePtr == '[')
    {   // many registers
        ++linePtr;
        cxuint value1, value2;
        skipSpacesToEnd(linePtr, end);
        value1 = cstrtovCStyle<cxbyte>(linePtr, end, linePtr);
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ':')
        {   // error
            asmr.printError(sgprRangePlace, "Unterminated SRegister range");
            return std::make_pair(0, 0);
        }
        ++linePtr;
        skipSpacesToEnd(linePtr, end);
        value2 = cstrtovCStyle<cxuint>(linePtr, end, linePtr);
        
        const cxuint maxSGPRsNum = (arch&ARCH_RX3X0) ? 102 : 104;
        if (value2 <= value1 || value1 >= maxSGPRsNum || value2 >= maxSGPRsNum)
        {   // error (illegal register range)
            asmr.printError(sgprRangePlace, "Illegal SRegister range");
            return std::make_pair(0, 0);
        }
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ']')
        {   // error
            asmr.printError(sgprRangePlace, "Unterminated SRegister range");
            return std::make_pair(0, 0);
        }
        /// check alignment
        if ((value2-value1==1 && (value1&1)!=0) ||
            (value2-value1>1 && (value1&3)!=0))
        {
            asmr.printError(sgprRangePlace, "Unaligned SRegister range");
            return std::make_pair(0, 0);
        }
        return std::make_pair(value1, uint16_t(value2)+1);
    }
    } catch(const ParseException& ex)
    { asmr.printError(linePtr, ex.what()); }
    
    return std::make_pair(0, 0);
}

std::pair<uint16_t, uint16_t> GCNAsmUtils::parseOperand(Assembler& asmr,
            const char*& linePtr, Flags instrOpMask)
{
    return std::make_pair(0, 0);
}

void GCNAsmUtils::parseSOP2Encoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseSOPKEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseSOPPEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseSMRDEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseMXBUFEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, std::vector<cxbyte>& output)
{
}

};

void GCNAssembler::assemble(const CString& mnemonic, const char* mnemPlace,
            const char* linePtr, const char* lineEnd, std::vector<cxbyte>& output)
{
    auto it = binaryFind(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
               GCNAsmInstruction{mnemonic.c_str()},
               [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
               { return ::strcmp(instr1.mnemonic, instr2.mnemonic)<0; });
    
    // find matched entry
    if (it != gcnInstrSortedTable.end() && (it->archMask & curArchMask)==0)
        // if not match current arch mask
        for (++it ;it != gcnInstrSortedTable.end() &&
               ::strcmp(it->mnemonic, mnemonic.c_str())==0 &&
               (it->archMask & curArchMask)==0; ++it);

    if (it == gcnInstrSortedTable.end() || ::strcmp(it->mnemonic, mnemonic.c_str())!=0)
    {   // unrecognized mnemonic
        printError(mnemPlace, "Unrecognized instruction");
        return;
    }
    
    /* decode instruction line */
    switch(it->encoding1)
    {
        case GCNENC_SOPC:
            GCNAsmUtils::parseSOPCEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_SOPP:
            GCNAsmUtils::parseSOPPEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_SOP1:
            GCNAsmUtils::parseSOP1Encoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_SOP2:
            GCNAsmUtils::parseSOP2Encoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_SOPK:
            GCNAsmUtils::parseSOPKEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_SMRD:
            GCNAsmUtils::parseSMRDEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_VOPC:
            GCNAsmUtils::parseVOPCEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_VOP1:
            GCNAsmUtils::parseVOP1Encoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_VOP2:
            GCNAsmUtils::parseVOP2Encoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_VOP3A:
        case GCNENC_VOP3B:
            GCNAsmUtils::parseVOP3Encoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_VINTRP:
            GCNAsmUtils::parseVINTRPEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_DS:
            GCNAsmUtils::parseDSEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_MUBUF:
        case GCNENC_MTBUF:
            GCNAsmUtils::parseMXBUFEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_MIMG:
            GCNAsmUtils::parseMIMGEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_EXP:
            GCNAsmUtils::parseEXPEncoding(assembler, *it, linePtr, output);
            break;
        case GCNENC_FLAT:
            GCNAsmUtils::parseFLATEncoding(assembler, *it, linePtr, output);
            break;
        default:
            break;
    }
}

bool GCNAssembler::resolveCode(cxbyte* location, cxbyte targetType, uint64_t value)
{
    return false;
}

bool GCNAssembler::checkMnemonic(const CString& mnemonic) const
{
    return std::binary_search(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
               GCNAsmInstruction{mnemonic.c_str()},
               [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
               { return ::strcmp(instr1.mnemonic, instr2.mnemonic)<0; });
}

const cxuint* GCNAssembler::getAllocatedRegisters(size_t& regTypesNum) const
{
    regTypesNum = 2;
    return regTable;
}
