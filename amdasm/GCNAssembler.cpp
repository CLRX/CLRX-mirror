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
#include <cstdio>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/GCNDefs.h>
#include "GCNAsmInternals.h"

using namespace CLRX;

static OnceFlag clrxGCNAssemblerOnceFlag;
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
        gcnInstrSortedTable[i] = {insn.mnemonic, insn.encoding, insn.mode,
                    insn.code, UINT16_MAX, insn.archMask};
    }
    
    // sort GCN instruction table by mnemonic, encoding and architecture
    std::sort(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
            [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
            {
                // compare mnemonic and if mnemonic
                int r = ::strcmp(instr1.mnemonic, instr2.mnemonic);
                return (r < 0) || (r==0 && instr1.encoding < instr2.encoding) ||
                            (r == 0 && instr1.encoding == instr2.encoding &&
                             instr1.archMask < instr2.archMask);
            });
    
    cxuint j = 0;
    std::unique_ptr<uint16_t[]> oldArchMasks(new uint16_t[tableSize]);
    /* join VOP3A instr with VOP2/VOPC/VOP1 instr together to faster encoding. */
    for (cxuint i = 0; i < tableSize; i++)
    {
        GCNAsmInstruction insn = gcnInstrSortedTable[i];
        if (insn.encoding == GCNENC_VOP3A || insn.encoding == GCNENC_VOP3B)
        {
            // check duplicates
            cxuint k = j-1;
            while (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                    (oldArchMasks[k] & insn.archMask)!=insn.archMask) k--;
            
            if (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                (oldArchMasks[k] & insn.archMask)==insn.archMask)
            {
                // we found duplicate, we apply
                if (gcnInstrSortedTable[k].code2==UINT16_MAX)
                {
                    // if second slot for opcode is not filled
                    gcnInstrSortedTable[k].code2 = insn.code1;
                    gcnInstrSortedTable[k].archMask = oldArchMasks[k] & insn.archMask;
                }
                else
                {
                    // if filled we create new entry
                    oldArchMasks[j] = gcnInstrSortedTable[j].archMask;
                    gcnInstrSortedTable[j] = gcnInstrSortedTable[k];
                    gcnInstrSortedTable[j].archMask = oldArchMasks[k] & insn.archMask;
                    gcnInstrSortedTable[j++].code2 = insn.code1;
                }
            }
            else // not found
            {
                oldArchMasks[j] = insn.archMask;
                gcnInstrSortedTable[j++] = insn;
            }
        }
        else if (insn.encoding == GCNENC_VINTRP)
        {
            // check duplicates
            cxuint k = j-1;
            oldArchMasks[j] = insn.archMask;
            gcnInstrSortedTable[j++] = insn;
            while (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                    gcnInstrSortedTable[k].encoding!=GCNENC_VOP3A) k--;
            if (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                gcnInstrSortedTable[k].encoding==GCNENC_VOP3A)
                // we found VINTRP duplicate, set up second code (VINTRP)
                gcnInstrSortedTable[k].code2 = insn.code1;
        }
        else // normal instruction
        {
            oldArchMasks[j] = insn.archMask;
            gcnInstrSortedTable[j++] = insn;
        }
    }
    gcnInstrSortedTable.resize(j); // final size
}

// GCN Usage handler

GCNUsageHandler::GCNUsageHandler() : ISAUsageHandler()
{ }
GCNUsageHandler::~GCNUsageHandler()
{ }

ISAUsageHandler* GCNUsageHandler::copy() const
{
    return new GCNUsageHandler(*this);
}

/// get usage dependencies
/* linearDeps - lists of linked register fields (linked fields) */
void GCNUsageHandler::getUsageDependencies(cxuint rvusNum, const AsmRegVarUsage* rvus,
                cxbyte* linearDeps) const
{
    cxuint count = 0;
    // linear dependencies (join fields)
    count = 0;
    for (cxuint i = 0; i < rvusNum; i++)
    {
        if (rvus[i].regVar == nullptr)
            continue;
        const AsmRegField rf = rvus[i].regField;
        if (rf == GCNFIELD_M_VDATA || rf == GCNFIELD_M_VDATAH ||
            rf == GCNFIELD_M_VDATALAST ||
            rf == GCNFIELD_FLAT_VDST || rf == GCNFIELD_FLAT_VDSTLAST)
            linearDeps[2 + count++] = i;
    }
    linearDeps[1] = (count >= 2) ? count : 0;
    linearDeps[0] = (linearDeps[1] != 0);
}

/*
 * GCN Assembler
 */

GCNAssembler::GCNAssembler(Assembler& assembler): ISAAssembler(assembler),
        regs({0, 0}), curArchMask(1U<<cxuint(
                    getGPUArchitectureFromDeviceType(assembler.getDeviceType())))
{
    callOnce(clrxGCNAssemblerOnceFlag, initializeGCNAssembler);
    std::fill(instrRVUs, instrRVUs + sizeof(instrRVUs)/sizeof(AsmRegVarUsage),
            AsmRegVarUsage{});
}

GCNAssembler::~GCNAssembler()
{ }

void GCNAssembler::setRegVarUsage(const AsmRegVarUsage& rvu)
{
    const cxuint maxSGPRsNum = getGPUMaxRegsNumByArchMask(curArchMask, REGTYPE_SGPR);
    if (rvu.regVar != nullptr || rvu.rstart < maxSGPRsNum || rvu.rstart >= 256 ||
        isSpecialSGPRRegister(curArchMask, rvu.rstart))
        instrRVUs[currentRVUIndex] = rvu;
}

ISAUsageHandler* GCNAssembler::createUsageHandler() const
{
    return new GCNUsageHandler();
}

void GCNAssembler::assemble(const CString& inMnemonic, const char* mnemPlace,
            const char* linePtr, const char* lineEnd, std::vector<cxbyte>& output,
            ISAUsageHandler* usageHandler, ISAWaitHandler* waitHandler)
{
    CString mnemonic;
    size_t inMnemLen = inMnemonic.size();
    GCNEncSize gcnEncSize = GCNEncSize::UNKNOWN;
    GCNVOPEnc vopEnc = GCNVOPEnc::NORMAL;
    // checking encoding suffixes (_e64, _e32,_dpp, _sdwa)
    if (inMnemLen>4 && ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_e64")==0)
    {
        gcnEncSize = GCNEncSize::BIT64;
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    }
    else if (inMnemLen>4 && ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_e32")==0)
    {
        gcnEncSize = GCNEncSize::BIT32;
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    }
    else if (inMnemLen>6 && toLower(inMnemonic[0])=='v' && inMnemonic[1]=='_' &&
        ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_dpp")==0)
    {
        vopEnc = GCNVOPEnc::DPP;
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    }
    else if (inMnemLen>7 && toLower(inMnemonic[0])=='v' && inMnemonic[1]=='_' &&
        ::strcasecmp(inMnemonic.c_str()+inMnemLen-5, "_sdwa")==0)
    {
        vopEnc = GCNVOPEnc::SDWA;
        mnemonic = inMnemonic.substr(0, inMnemLen-5);
    }
    else
        mnemonic = inMnemonic;
    
    // find instruction by mnemonic
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
    {
        // unrecognized mnemonic
        printError(mnemPlace, "Unknown instruction");
        return;
    }
    
    resetInstrRVUs();
    resetWaitInstrs();
    setCurrentRVU(0);
    /* decode instruction line */
    bool good = false;
    switch(it->encoding)
    {
        case GCNENC_SOPC:
            good = GCNAsmUtils::parseSOPCEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SOPP:
            good = GCNAsmUtils::parseSOPPEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SOP1:
            good = GCNAsmUtils::parseSOP1Encoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SOP2:
            good = GCNAsmUtils::parseSOP2Encoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SOPK:
            good = GCNAsmUtils::parseSOPKEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SMRD:
            if (curArchMask & ARCH_GCN_1_2_4)
                good = GCNAsmUtils::parseSMEMEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            else
                good = GCNAsmUtils::parseSMRDEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_VOPC:
            good = GCNAsmUtils::parseVOPCEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_VOP1:
            good = GCNAsmUtils::parseVOP1Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_VOP2:
            good = GCNAsmUtils::parseVOP2Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_VOP3A:
        case GCNENC_VOP3B:
            good = GCNAsmUtils::parseVOP3Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_VINTRP:
            good = GCNAsmUtils::parseVINTRPEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_DS:
            good = GCNAsmUtils::parseDSEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_MUBUF:
        case GCNENC_MTBUF:
            good = GCNAsmUtils::parseMUBUFEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_MIMG:
            good = GCNAsmUtils::parseMIMGEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_EXP:
            good = GCNAsmUtils::parseEXPEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_FLAT:
            good = GCNAsmUtils::parseFLATEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        default:
            break;
    }
    // register RegVarUsage in tests, do not apply normal usage
    if (good && (assembler.getFlags() & ASM_TESTRUN) != 0)
    {
        flushInstrRVUs(usageHandler);
        flushWaitInstrs(waitHandler);
    }
}

#define GCN_FAIL_BY_ERROR(PLACE, STRING) \
    { \
        printError(PLACE, STRING); \
        return false; \
    }

// method to resolve expressions in code (in instruction in instruction field)
bool GCNAssembler::resolveCode(const AsmSourcePos& sourcePos, cxuint targetSectionId,
             cxbyte* sectionData, size_t offset, AsmExprTargetType targetType,
             cxuint sectionId, uint64_t value)
{
    switch(targetType)
    {
        case GCNTGT_LITIMM:
            // literal in instruction
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                        "Relative value is illegal in literal expressions")
            SULEV(*reinterpret_cast<uint32_t*>(sectionData+offset+4), value);
            printWarningForRange(32, value, sourcePos);
            return true;
        case GCNTGT_SOPKSIMM16:
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                        "Relative value is illegal in immediate expressions")
            SULEV(*reinterpret_cast<uint16_t*>(sectionData+offset), value);
            printWarningForRange(16, value, sourcePos);
            return true;
        case GCNTGT_SOPJMP:
        {
            if (sectionId != targetSectionId)
                // if jump outside current section (.text)
                GCN_FAIL_BY_ERROR(sourcePos, "Jump over current section!")
            int64_t outOffset = (int64_t(value)-int64_t(offset)-4);
            if (outOffset & 3)
                GCN_FAIL_BY_ERROR(sourcePos, "Jump is not aligned to word!")
            outOffset >>= 2;
            if (outOffset > INT16_MAX || outOffset < INT16_MIN)
                GCN_FAIL_BY_ERROR(sourcePos, "Jump out of range!")
            SULEV(*reinterpret_cast<uint16_t*>(sectionData+offset), outOffset);
            uint16_t insnCode = ULEV(*reinterpret_cast<uint16_t*>(sectionData+offset+2));
            // add codeflow entry
            addCodeFlowEntry(sectionId, { size_t(offset), size_t(value),
                    insnCode==0xbf82U ? AsmCodeFlowType::JUMP :
                    // CALL from S_CALL_B64
                    (((insnCode&0xff80)==0xba80 &&
                            (curArchMask&ARCH_GCN_1_4)!=0) ? AsmCodeFlowType::CALL :
                                AsmCodeFlowType::CJUMP) });
            return true;
        }
        case GCNTGT_SMRDOFFSET:
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                            "Relative value is illegal in offset expressions")
            sectionData[offset] = value;
            printWarningForRange(8, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_DSOFFSET16:
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                            "Relative value is illegal in offset expressions")
            SULEV(*reinterpret_cast<uint16_t*>(sectionData+offset), value);
            printWarningForRange(16, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_DSOFFSET8_0:
        case GCNTGT_DSOFFSET8_1:
        case GCNTGT_SOPCIMM8:
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos, (targetType != GCNTGT_SOPCIMM8) ?
                        "Relative value is illegal in offset expressions" :
                        "Relative value is illegal in immediate expressions")
            if (targetType==GCNTGT_DSOFFSET8_0)
                sectionData[offset] = value;
            else
                sectionData[offset+1] = value;
            printWarningForRange(8, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_MXBUFOFFSET:
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                            "Relative value is illegal in offset expressions")
            sectionData[offset] = value&0xff;
            sectionData[offset+1] = (sectionData[offset+1]&0xf0) | ((value>>8)&0xf);
            printWarningForRange(12, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_SMEMOFFSET:
        case GCNTGT_SMEMOFFSETVEGA:
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                            "Relative value is illegal in offset expressions")
            if (targetType==GCNTGT_SMEMOFFSETVEGA)
            {
                uint32_t oldV = ULEV(*reinterpret_cast<uint32_t*>(sectionData+offset+4));
                SULEV(*reinterpret_cast<uint32_t*>(sectionData+offset+4),
                            (oldV & 0xffe00000U) | (value&0x1fffffU));
            }
            else
                SULEV(*reinterpret_cast<uint32_t*>(sectionData+offset+4), value&0xfffffU);
            printWarningForRange(targetType==GCNTGT_SMEMOFFSETVEGA ? 21 : 20,
                            value, sourcePos,
                            targetType==GCNTGT_SMEMOFFSETVEGA ? WS_BOTH : WS_UNSIGNED);
            return true;
        case GCNTGT_SMEMIMM:
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                        "Relative value is illegal in immediate expressions")
            sectionData[offset] = (sectionData[offset]&0x3f) | ((value<<6)&0xff);
            sectionData[offset+1] = (sectionData[offset+1]&0xe0) | ((value>>2)&0x1f);
            printWarningForRange(7, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_INSTOFFSET:
            // FLAT unsigned inst_offset
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                        "Relative value is illegal in offset expressions")
            sectionData[offset] = value;
            sectionData[offset+1] = (sectionData[offset+1]&0xf0) | ((value&0xf00)>>8);
            printWarningForRange(12, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_INSTOFFSET_S:
            // FLAT signed inst_offset
            if (sectionId != ASMSECT_ABS)
                GCN_FAIL_BY_ERROR(sourcePos,
                        "Relative value is illegal in offset expressions")
            sectionData[offset] = value;
            sectionData[offset+1] = (sectionData[offset+1]&0xe0) |
                    ((value&0x1f00)>>8);
            printWarningForRange(13, value, sourcePos, WS_BOTH);
            return true;
        default:
            return false;
    }
}

// check whether name is mnemonic (currently unused anywhere)
bool GCNAssembler::checkMnemonic(const CString& inMnemonic) const
{
    CString mnemonic;
    size_t inMnemLen = inMnemonic.size();
    // checking for encoding suffixes
    if (inMnemLen>4 &&
        (::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_e64")==0 ||
            ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_e32")==0))
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    else if (inMnemLen>6 && toLower(inMnemonic[0])=='v' && inMnemonic[1]=='_' &&
        ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_dpp")==0)
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    else if (inMnemLen>7 && toLower(inMnemonic[0])=='v' && inMnemonic[1]=='_' &&
        ::strcasecmp(inMnemonic.c_str()+inMnemLen-5, "_sdwa")==0)
        mnemonic = inMnemonic.substr(0, inMnemLen-5);
    else
        mnemonic = inMnemonic;
    
    return std::binary_search(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
               GCNAsmInstruction{mnemonic.c_str()},
               [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
               { return ::strcmp(instr1.mnemonic, instr2.mnemonic)<0; });
}

void GCNAssembler::setAllocatedRegisters(const cxuint* inRegs, Flags inRegFlags)
{
    if (inRegs==nullptr)
        regs.sgprsNum = regs.vgprsNum = 0;
    else // if not null, just copy
        std::copy(inRegs, inRegs+2, regTable);
    regs.regFlags = inRegFlags;
}

const cxuint* GCNAssembler::getAllocatedRegisters(size_t& regTypesNum,
              Flags& outRegFlags) const
{
    regTypesNum = 2;
    outRegFlags = regs.regFlags;
    return regTable;
}

void GCNAssembler::getMaxRegistersNum(size_t& regTypesNum, cxuint* maxRegs) const
{
    maxRegs[0] = getGPUMaxRegsNumByArchMask(curArchMask, 0);
    maxRegs[1] = getGPUMaxRegsNumByArchMask(curArchMask, 1);
    regTypesNum = 2;
}

void GCNAssembler::getRegisterRanges(size_t& regTypesNum, cxuint* regRanges) const
{
    regRanges[0] = 0;
    regRanges[1] = 108; // to extra SGPR register number
    regRanges[2] = 256; // vgpr
    regRanges[3] = 256+getGPUMaxRegsNumByArchMask(curArchMask, 1);
    regTypesNum = 2;
}

// method that filling code to alignment (used by alignment pseudo-ops on code section)
void GCNAssembler::fillAlignment(size_t size, cxbyte* output)
{
    uint32_t value = LEV(0xbf800000U); // fill with s_nop's
    if ((size&3)!=0)
    {
        // first, we fill zeros
        const size_t toAlign4 = 4-(size&3);
        ::memset(output, 0, toAlign4);
        output += toAlign4;
    }
    std::fill((uint32_t*)output, ((uint32_t*)output) + (size>>2), value);
}

bool GCNAssembler::parseRegisterRange(const char*& linePtr, cxuint& regStart,
          cxuint& regEnd, const AsmRegVar*& regVar)
{
    GCNOperand operand;
    regVar = nullptr;
    if (!GCNAsmUtils::parseOperand(assembler, linePtr, operand, nullptr, curArchMask, 0,
                INSTROP_SREGS|INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_UNALIGNED|
                INSTROP_ONLYINLINECONSTS,
                ASMFIELD_NONE))
        return false;
    regStart = operand.range.start;
    regEnd = operand.range.end;
    regVar = operand.range.regVar;
    return true;
}

bool GCNAssembler::relocationIsFit(cxuint bits, AsmExprTargetType tgtType)
{
    if (bits==32)
        return tgtType==GCNTGT_SOPJMP || tgtType==GCNTGT_LITIMM;
    return false;
}

bool GCNAssembler::parseRegisterType(const char*& linePtr, const char* end, cxuint& type)
{
    skipSpacesToEnd(linePtr, end);
    if (linePtr!=end)
    {
        const char c = toLower(*linePtr);
        if (c=='v' || c=='s')
        {
            type = c=='v' ? REGTYPE_VGPR : REGTYPE_SGPR;
            linePtr++;
            return true;
        }
        return false;
    }
    return false;
}

static const bool gcnSize11Table[16] =
{
    false, // GCNENC_SMRD, // 0000
    false, // GCNENC_SMRD, // 0001
    false, // GCNENC_VINTRP, // 0010
    false, // GCNENC_NONE, // 0011 - illegal
    true,  // GCNENC_VOP3A, // 0100
    false, // GCNENC_NONE, // 0101 - illegal
    true,  // GCNENC_DS,   // 0110
    true,  // GCNENC_FLAT, // 0111
    true,  // GCNENC_MUBUF, // 1000
    false, // GCNENC_NONE,  // 1001 - illegal
    true,  // GCNENC_MTBUF, // 1010
    false, // GCNENC_NONE,  // 1011 - illegal
    true,  // GCNENC_MIMG,  // 1100
    false, // GCNENC_NONE,  // 1101 - illegal
    true,  // GCNENC_EXP,   // 1110
    false // GCNENC_NONE   // 1111 - illegal
};

static const bool gcnSize12Table[16] =
{
    true,  // GCNENC_SMEM, // 0000
    true,  // GCNENC_EXP, // 0001
    false, // GCNENC_NONE, // 0010 - illegal
    false, // GCNENC_NONE, // 0011 - illegal
    true,  // GCNENC_VOP3A, // 0100
    false, // GCNENC_VINTRP, // 0101
    true,  // GCNENC_DS,   // 0110
    true,  // GCNENC_FLAT, // 0111
    true,  // GCNENC_MUBUF, // 1000
    false, // GCNENC_NONE,  // 1001 - illegal
    true,  // GCNENC_MTBUF, // 1010
    false, // GCNENC_NONE,  // 1011 - illegal
    true,  // GCNENC_MIMG,  // 1100
    false, // GCNENC_NONE,  // 1101 - illegal
    false, // GCNENC_NONE,  // 1110 - illegal
    false // GCNENC_NONE   // 1111 - illegal
};

// get instruction size, used by register allocation to skip instruction
size_t GCNAssembler::getInstructionSize(size_t codeSize, const cxbyte* code) const
{
    if (codeSize < 4)
        return 0; // no instruction
    bool isGCN11 = (curArchMask & ARCH_RX2X0)!=0;
    bool isGCN12 = (curArchMask & ARCH_GCN_1_2_4)!=0;
    const uint32_t insnCode = ULEV(*reinterpret_cast<const uint32_t*>(code));
    uint32_t words = 1;
    if ((insnCode & 0x80000000U) != 0)
    {
        if ((insnCode & 0x40000000U) == 0)
        {
            // SOP???
            if  ((insnCode & 0x30000000U) == 0x30000000U)
            {
                // SOP1/SOPK/SOPC/SOPP
                const uint32_t encPart = (insnCode & 0x0f800000U);
                if (encPart == 0x0e800000U)
                {
                    // SOP1
                    if ((insnCode&0xff) == 0xff) // literal
                        words++;
                }
                else if (encPart == 0x0f000000U)
                {
                    // SOPC
                    if ((insnCode&0xff) == 0xff ||
                        (insnCode&0xff00) == 0xff00) // literal
                        words++;
                }
                else if (encPart != 0x0f800000U)
                {
                    // SOPK
                    const cxuint opcode = (insnCode>>23)&0x1f;
                    if ((!isGCN12 && opcode == 21) ||
                        (isGCN12 && opcode == 20))
                        words++; // additional literal
                }
            }
            else
            {
                // SOP2
                if ((insnCode&0xff) == 0xff || (insnCode&0xff00) == 0xff00)
                    words++;  // literal
            }
        }
        else
        {
            // SMRD and others
            const uint32_t encPart = (insnCode&0x3c000000U)>>26;
            if ((!isGCN12 && gcnSize11Table[encPart] && (encPart != 7 || isGCN11)) ||
                (isGCN12 && gcnSize12Table[encPart]))
                words++;
        }
    }
    else
    {
        // some vector instructions
        if ((insnCode & 0x7e000000U) == 0x7c000000U)
        {
            // VOPC
            if ((insnCode&0x1ff) == 0xff || // literal
                // SDWA, DDP
                (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                words++;
        }
        else if ((insnCode & 0x7e000000U) == 0x7e000000U)
        {
            // VOP1
            if ((insnCode&0x1ff) == 0xff || // literal
                // SDWA, DDP
                (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                words++;
        }
        else
        {
            // VOP2
            const cxuint opcode = (insnCode >> 25)&0x3f;
            if ((!isGCN12 && (opcode == 32 || opcode == 33)) ||
                (isGCN12 && (opcode == 23 || opcode == 24 ||
                opcode == 36 || opcode == 37))) // V_MADMK and V_MADAK
                words++;  // inline 32-bit constant
            else if ((insnCode&0x1ff) == 0xff || // literal
                // SDWA, DDP
                (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                words++;  // literal
        }
    }
    return words<<2;
}

// for GCN 1.0
static const AsmWaitConfig gcnWaitConfig10 =
{
    cxuint(GCNDELOP_MAX+1),
    cxuint(GCNWAIT_MAX+1),
    {
        { GCNWAIT_VMCNT, true, false, 255 },  // GCNDELOP_VMOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_LDSOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_GDSOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_SENDMSG
        { GCNWAIT_LGKMCNT, false, false, 4 },  // GCNDELOP_SMOP
        { GCNWAIT_EXPCNT, true, true, 255 },  // GCNDELOP_EXPVMWRITE
        { GCNWAIT_EXPCNT, false, false, 255 }  // GCNDELOP_EXPORT
    },
    { 16, 8, 8 }
};


static const AsmWaitConfig gcnWaitConfig =
{
    cxuint(GCNDELOP_MAX+1),
    cxuint(GCNWAIT_MAX+1),
    {
        { GCNWAIT_VMCNT, true, false, 255 },  // GCNDELOP_VMOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_LDSOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_GDSOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_SENDMSG
        { GCNWAIT_LGKMCNT, false, false, 4 },  // GCNDELOP_SMOP
        { GCNWAIT_EXPCNT, true, true, 255 },  // GCNDELOP_EXPVMWRITE
        { GCNWAIT_EXPCNT, false, true, 255 }  // GCNDELOP_EXPORT
    },
    { 16, 16, 8 }
};

// for RX VEGA
static const AsmWaitConfig gcnWaitConfig14 =
{
    cxuint(GCNDELOP_MAX+1),
    cxuint(GCNWAIT_MAX+1),
    {
        { GCNWAIT_VMCNT, true, false, 255 },  // GCNDELOP_VMOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_LDSOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_GDSOP
        { GCNWAIT_LGKMCNT, true, false, 255 },  // GCNDELOP_SENDMSG
        { GCNWAIT_LGKMCNT, false, false, 4 },  // GCNDELOP_SMOP
        { GCNWAIT_EXPCNT, true, true, 255 },  // GCNDELOP_EXPVMWRITE
        { GCNWAIT_EXPCNT, false, true, 255 }  // GCNDELOP_EXPORT
    },
    { 64, 16, 8 }
};

const AsmWaitConfig& GCNAssembler::getWaitConfig() const
{
    if ((curArchMask&ARCH_HD7X00)!=0)
        return gcnWaitConfig10;
    return (curArchMask&ARCH_GCN_1_4)!=0 ? gcnWaitConfig14 : gcnWaitConfig;
}
