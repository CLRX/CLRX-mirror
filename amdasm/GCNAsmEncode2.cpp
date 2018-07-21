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
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/GCNDefs.h>
#include "GCNAsmInternals.h"

namespace CLRX
{

// data format names (sorted by names) (for MUBUF/MTBUF)
static const std::pair<const char*, uint16_t> mtbufDFMTNamesMap[] =
{
    { "10_10_10_2", 8 },
    { "10_11_11", 6 },
    { "11_11_10", 7 },
    { "16", 2 },
    { "16_16", 5 },
    { "16_16_16_16", 12 },
    { "2_10_10_10", 9 },
    { "32", 4 },
    { "32_32", 11 },
    { "32_32_32", 13 },
    { "32_32_32_32", 14 },
    { "8", 1 },
    { "8_8", 3 },
    { "8_8_8_8", 10 }
};

// number format names (sorted by names) (for MUBUF/MTBUF)
static const std::pair<const char*, cxuint> mtbufNFMTNamesMap[] =
{
    { "float", 7 },
    { "sint", 5 },
    { "snorm", 1 },
    { "snorm_ogl", 6 },
    { "sscaled", 3 },
    { "uint", 4 },
    { "unorm", 0 },
    { "uscaled", 2 }
};

void GCNAsmUtils::prepareRVUAndWait(GCNAssembler* gcnAsm, GPUArchMask arch,
            bool vdataToRead, bool vdataToWrite, bool haveLds, bool haveTfe,
            std::vector<cxbyte>& output, const GCNAsmInstruction& gcnInsn)
{
    // fix access for VDATA field
    gcnAsm->instrRVUs[0].rwFlags = (vdataToWrite ? ASMRVU_WRITE : 0) |
            (vdataToRead ? ASMRVU_READ : 0);
    // check cmp_swap/fcmp_swap
    bool vdataDivided = false;
    if ((gcnInsn.mode & GCN_MHALFWRITE) != 0 && vdataToWrite &&
        gcnAsm->instrRVUs[0].regField != ASMFIELD_NONE)
    {
        // fix access
        AsmRegVarUsage& rvu = gcnAsm->instrRVUs[0];
        uint16_t size = rvu.rend-rvu.rstart;
        rvu.rend = rvu.rstart + (size>>1);
        AsmRegVarUsage& nextRvu = gcnAsm->instrRVUs[4];
        nextRvu = rvu;
        nextRvu.regField = GCNFIELD_M_VDATAH;
        nextRvu.rstart += (size>>1);
        nextRvu.rend = rvu.rstart + size;
        nextRvu.rwFlags = ASMRVU_READ;
        vdataDivided = true;
    }
    
    if (haveTfe && (vdataDivided ||
            gcnAsm->instrRVUs[0].rwFlags!=(ASMRVU_READ|ASMRVU_WRITE)) &&
            gcnAsm->instrRVUs[0].regField != ASMFIELD_NONE)
    {
        // fix for tfe
        const cxuint rvuId = (vdataDivided ? 4 : 0);
        AsmRegVarUsage& rvu = gcnAsm->instrRVUs[rvuId];
        AsmRegVarUsage& lastRvu = gcnAsm->instrRVUs[5];
        lastRvu = rvu;
        lastRvu.rstart = lastRvu.rend-1;
        lastRvu.rwFlags = ASMRVU_READ|ASMRVU_WRITE;
        lastRvu.regField = GCNFIELD_M_VDATALAST;
        rvu.rend--;
    }
    
    // register delayed operations
    const bool needExpWrite = (vdataToRead && (arch & ARCH_HD7X00) != 0);
    if (gcnAsm->instrRVUs[0].regField != ASMFIELD_NONE)
    {
        if (!haveLds)
        {
            gcnAsm->delayedOps[0] = { output.size(), gcnAsm->instrRVUs[0].regVar,
                    gcnAsm->instrRVUs[0].rstart, gcnAsm->instrRVUs[0].rend, 1,
                    GCNDELOP_VMOP, needExpWrite ? GCNDELOP_EXPVMWRITE : GCNDELOP_NONE,
                    gcnAsm->instrRVUs[0].rwFlags,
                    cxbyte(needExpWrite ? ASMRVU_READ : 0) };
            if (haveTfe)
                gcnAsm->delayedOps[2] = { output.size(), gcnAsm->instrRVUs[5].regVar,
                        gcnAsm->instrRVUs[5].rstart, gcnAsm->instrRVUs[5].rend, 1,
                        GCNDELOP_VMOP, GCNDELOP_NONE, gcnAsm->instrRVUs[5].rwFlags };
            if (vdataDivided)
                gcnAsm->delayedOps[3] = { output.size(), gcnAsm->instrRVUs[4].regVar,
                        gcnAsm->instrRVUs[4].rstart, gcnAsm->instrRVUs[4].rend, 1,
                        GCNDELOP_VMOP, GCNDELOP_NONE, gcnAsm->instrRVUs[4].rwFlags };
        }
    }
    else if (haveLds)
        gcnAsm->delayedOps[0] = { output.size(), nullptr, uint16_t(0), uint16_t(0),
                1, GCNDELOP_VMOP, needExpWrite ? GCNDELOP_EXPVMWRITE : GCNDELOP_NONE,
                cxbyte(0) };
}

bool GCNAsmUtils::parseMUBUFEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT32)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 64-bit size for MUBUF/MTBUF encoding")
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    RegRange vaddrReg(0, 0);
    RegRange vdataReg(0, 0);
    GCNOperand soffsetOp{};
    RegRange srsrcReg(0, 0);
    const bool isGCN12 = (arch & ARCH_GCN_1_2_4)!=0;
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    skipSpacesToEnd(linePtr, end);
    const char* vdataPlace = linePtr;
    const char* vaddrPlace = nullptr;
    bool parsedVaddr = false;
    if (mode1 != GCN_ARG_NONE)
    {
        if (mode1 != GCN_MUBUF_NOVAD)
        {
            gcnAsm->setCurrentRVU(0);
            // parse VDATA (various VGPR number, verified later)
            good &= parseVRegRange(asmr, linePtr, vdataReg, 0, GCNFIELD_M_VDATA, true,
                        INSTROP_SYMREGRANGE|INSTROP_READ);
            if (!skipRequiredComma(asmr, linePtr))
                return false;
            
            skipSpacesToEnd(linePtr, end);
            vaddrPlace = linePtr;
            gcnAsm->setCurrentRVU(1);
            // parse VADDR (1 or 2 VGPR's) (optional)
            if (!parseVRegRange(asmr, linePtr, vaddrReg, 0, GCNFIELD_M_VADDR, false,
                        INSTROP_SYMREGRANGE|INSTROP_READ))
                good = false;
            if (vaddrReg) // only if vaddr is
            {
                parsedVaddr = true;
                if (!skipRequiredComma(asmr, linePtr))
                    return false;
            }
            else
            {
                // if not, default is v0, then parse off
                if (linePtr+3<=end && ::strncasecmp(linePtr, "off", 3)==0 &&
                    (isSpace(linePtr[3]) || linePtr[3]==','))
                {
                    linePtr+=3;
                    if (!skipRequiredComma(asmr, linePtr))
                        return false;
                }
                vaddrReg = {256, 257};
            }
        }
        // parse SRSREG (4 SGPR's)
        gcnAsm->setCurrentRVU(2);
        good &= parseSRegRange(asmr, linePtr, srsrcReg, arch, 4, GCNFIELD_M_SRSRC, true,
                        INSTROP_SYMREGRANGE|INSTROP_READ);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        // parse SOFFSET (SGPR or scalar source or constant)
        gcnAsm->setCurrentRVU(3);
        good &= parseOperand(asmr, linePtr, soffsetOp, nullptr, arch, 1,
                 INSTROP_SREGS|INSTROP_SSOURCE|INSTROP_ONLYINLINECONSTS|INSTROP_READ|
                 INSTROP_NOLITERALERRORMUBUF, GCNFIELD_M_SOFFSET);
    }
    
    bool haveOffset = false, haveFormat = false;
    cxuint dfmt = 1, nfmt = 0;
    cxuint offset = 0;
    std::unique_ptr<AsmExpression> offsetExpr;
    bool haveAddr64 = false, haveTfe = false, haveSlc = false, haveLds = false;
    bool haveGlc = false, haveOffen = false, haveIdxen = false;
    const char* modName = (gcnInsn.encoding==GCNENC_MTBUF) ?
            "MTBUF modifier" : "MUBUF modifier";
    
    // main loop to parsing MUBUF/MTBUF modifiers
    while(linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        char name[10];
        const char* modPlace = linePtr;
        if (!getNameArgS(asmr, 10, name, linePtr, modName))
        {
            good = false;
            continue;
        }
        toLowerString(name);
        
        if (name[0] == 'o')
        {
            // offen, offset
            if (::strcmp(name+1, "ffen")==0)
                good &= parseModEnable(asmr, linePtr, haveOffen, "offen modifier");
            else if (::strcmp(name+1, "ffset")==0)
            {
                // parse offset
                if (parseModImm(asmr, linePtr, offset, &offsetExpr, "offset",
                                12, WS_UNSIGNED))
                {
                    if (haveOffset)
                        asmr.printWarning(modPlace, "Offset is already defined");
                    haveOffset = true;
                }
                else
                    good = false;
            }
            else
                ASM_NOTGOOD_BY_ERROR(modPlace, (gcnInsn.encoding==GCNENC_MUBUF) ? 
                    "Unknown MUBUF modifier" : "Unknown MTBUF modifier")
        }
        else if (gcnInsn.encoding==GCNENC_MTBUF && ::strcmp(name, "format")==0)
        {
            // parse format
            bool modGood = true;
            skipSpacesToEnd(linePtr, end);
            if (linePtr==end || *linePtr!=':')
            {
                ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before format")
                continue;
            }
            skipCharAndSpacesToEnd(linePtr, end);
            
            // parse [DATA_FORMAT:NUMBER_FORMAT]
            if (linePtr==end || *linePtr!='[')
                ASM_NOTGOOD_BY_ERROR1(modGood = good, modPlace,
                                "Expected '[' before format")
            if (modGood)
            {
                skipCharAndSpacesToEnd(linePtr, end);
                const char* fmtPlace = linePtr;
                char fmtName[30];
                bool haveNFMT = false;
                if (linePtr != end && *linePtr=='@')
                {
                    // expression, parse DATA_FORMAT
                    linePtr++;
                    if (!parseImm(asmr, linePtr, dfmt, nullptr, 4, WS_UNSIGNED))
                        modGood = good = false;
                }
                else if (getMUBUFFmtNameArg(
                            asmr, 30, fmtName, linePtr, "data/number format"))
                {
                    toLowerString(fmtName);
                    size_t dfmtNameIndex = (::strncmp(fmtName,
                                 "buf_data_format_", 16)==0) ? 16 : 0;
                    size_t dfmtIdx = binaryMapFind(mtbufDFMTNamesMap, mtbufDFMTNamesMap+14,
                                fmtName+dfmtNameIndex, CStringLess())-mtbufDFMTNamesMap;
                    if (dfmtIdx != 14)
                        dfmt = mtbufDFMTNamesMap[dfmtIdx].second;
                    else
                    {
                        // nfmt (if not found, then try parse number format)
                        size_t nfmtNameIndex = (::strncmp(fmtName,
                                 "buf_num_format_", 15)==0) ? 15 : 0;
                        size_t nfmtIdx = binaryMapFind(mtbufNFMTNamesMap,
                               mtbufNFMTNamesMap+8, fmtName+nfmtNameIndex,
                               CStringLess())-mtbufNFMTNamesMap;
                        // check if found
                        if (nfmtIdx!=8)
                        {
                            nfmt = mtbufNFMTNamesMap[nfmtIdx].second;
                            haveNFMT = true;
                        }
                        else
                            ASM_NOTGOOD_BY_ERROR1(modGood = good, fmtPlace,
                                        "Unknown data/number format")
                    }
                }
                else
                    modGood = good = false;
                
                skipSpacesToEnd(linePtr, end);
                if (!haveNFMT && linePtr!=end && *linePtr==',')
                {
                    skipCharAndSpacesToEnd(linePtr, end);
                    if (linePtr != end && *linePtr=='@')
                    {
                        // expression (number format)
                        linePtr++;
                        if (!parseImm(asmr, linePtr, nfmt, nullptr, 3, WS_UNSIGNED))
                            modGood = good = false;
                    }
                    else
                    {
                        // parse NUMBER format from name
                        fmtPlace = linePtr;
                        good &= getEnumeration(asmr, linePtr, "number format",
                                8, mtbufNFMTNamesMap, nfmt, "buf_num_format_");
                    }
                }
                skipSpacesToEnd(linePtr, end);
                // close format
                if (linePtr!=end && *linePtr==']')
                    linePtr++;
                else
                    ASM_NOTGOOD_BY_ERROR(linePtr, "Unterminated format modifier")
                if (modGood)
                {
                    if (haveFormat)
                        asmr.printWarning(modPlace, "Format is already defined");
                    haveFormat = true;
                }
            }
        }
        // other modifiers
        else if (!isGCN12 && ::strcmp(name, "addr64")==0)
            good &= parseModEnable(asmr, linePtr, haveAddr64, "addr64 modifier");
        else if (::strcmp(name, "tfe")==0)
            good &= parseModEnable(asmr, linePtr, haveTfe, "tfe modifier");
        else if (::strcmp(name, "glc")==0)
            good &= parseModEnable(asmr, linePtr, haveGlc, "glc modifier");
        else if (::strcmp(name, "slc")==0)
            good &= parseModEnable(asmr, linePtr, haveSlc, "slc modifier");
        else if (gcnInsn.encoding==GCNENC_MUBUF && ::strcmp(name, "lds")==0)
            good &= parseModEnable(asmr, linePtr, haveLds, "lds modifier");
        else if (::strcmp(name, "idxen")==0)
            good &= parseModEnable(asmr, linePtr, haveIdxen, "idxen modifier");
        else
            ASM_NOTGOOD_BY_ERROR(modPlace, (gcnInsn.encoding==GCNENC_MUBUF) ? 
                    "Unknown MUBUF modifier" : "Unknown MTBUF modifier")
    }
    
    /* checking addr range and vdata range */
    bool vdataToRead = false;
    bool vdataToWrite = false;
    if (vdataReg)
    {
        vdataToWrite = ((gcnInsn.mode&GCN_MLOAD) != 0 ||
                ((gcnInsn.mode&GCN_MATOMIC)!=0 && haveGlc));
        vdataToRead = (gcnInsn.mode&GCN_MLOAD)==0 ||
                (gcnInsn.mode&GCN_MATOMIC)!=0;
        // check register range (number of register) in VDATA
        cxuint dregsNum = (((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1);
        if ((gcnInsn.mode & GCN_MUBUF_D16)!=0 && isGCN14)
            // 16-bit values packed into half of number of registers
            dregsNum = (dregsNum+1)>>1;
        dregsNum += (haveTfe);
        if (!isXRegRange(vdataReg, dregsNum))
        {
            char errorMsg[40];
            snprintf(errorMsg, 40, "Required %u vector register%s", dregsNum,
                     (dregsNum>1) ? "s" : "");
            ASM_NOTGOOD_BY_ERROR(vdataPlace, errorMsg)
        }
    }
    if (vaddrReg)
    {
        if (!parsedVaddr && (haveIdxen || haveOffen || haveAddr64))
            // no vaddr in instruction
            ASM_NOTGOOD_BY_ERROR(vaddrPlace, "VADDR is required if idxen, offen "
                    "or addr64 is enabled")
        else
        {
            const cxuint vaddrSize = ((haveOffen&&haveIdxen) || haveAddr64) ? 2 : 1;
            // check register range (number of register) in VADDR
            if (!isXRegRange(vaddrReg, vaddrSize))
                ASM_NOTGOOD_BY_ERROR(vaddrPlace, (vaddrSize==2) ?
                        "Required 2 vector registers" : "Required 1 vector register")
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    /* checking modifiers conditions */
    if (haveAddr64 && (haveOffen || haveIdxen))
        ASM_FAIL_BY_ERROR(instrPlace, "Idxen and offen must be zero in 64-bit address mode")
    if (haveTfe && haveLds)
        ASM_FAIL_BY_ERROR(instrPlace, "Both LDS and TFE is illegal")
    
    // ignore vdata if LDS
    if (haveLds)
        gcnAsm->instrRVUs[0].regField = ASMFIELD_NONE;
    // do not read vaddr if no offen and idxen and no addr64
    if (!haveAddr64 && !haveOffen && !haveIdxen)
        gcnAsm->instrRVUs[1].regField = ASMFIELD_NONE; // ignore this
    
    prepareRVUAndWait(gcnAsm, arch, vdataToRead, vdataToWrite, haveLds, haveTfe,
                      output, gcnInsn);
    
    if (offsetExpr!=nullptr)
        offsetExpr->setTarget(AsmExprTarget(GCNTGT_MXBUFOFFSET, asmr.currentSection,
                    output.size()));
    
    // put data (instruction words)
    uint32_t words[2];
    if (gcnInsn.encoding==GCNENC_MUBUF)
        SLEV(words[0], 0xe0000000U | offset | (haveOffen ? 0x1000U : 0U) |
                (haveIdxen ? 0x2000U : 0U) | (haveGlc ? 0x4000U : 0U) |
                ((haveAddr64 && !isGCN12) ? 0x8000U : 0U) | (haveLds ? 0x10000U : 0U) |
                ((haveSlc && isGCN12) ? 0x20000U : 0) | (uint32_t(gcnInsn.code1)<<18));
    else
    {
        // MTBUF encoding
        uint32_t code = (isGCN12) ? (uint32_t(gcnInsn.code1)<<15) :
                (uint32_t(gcnInsn.code1)<<16);
        SLEV(words[0], 0xe8000000U | offset | (haveOffen ? 0x1000U : 0U) |
                (haveIdxen ? 0x2000U : 0U) | (haveGlc ? 0x4000U : 0U) |
                ((haveAddr64 && !isGCN12) ? 0x8000U : 0U) | code |
                (uint32_t(dfmt)<<19) | (uint32_t(nfmt)<<23));
    }
    // second word
    SLEV(words[1], (vaddrReg.bstart()&0xff) | (uint32_t(vdataReg.bstart()&0xff)<<8) |
            (uint32_t(srsrcReg.bstart()>>2)<<16) |
            ((haveSlc && (!isGCN12 || gcnInsn.encoding==GCNENC_MTBUF)) ? (1U<<22) : 0) |
            (haveTfe ? (1U<<23) : 0) | (uint32_t(soffsetOp.range.bstart())<<24));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    offsetExpr.release();
    // update register pool (instr loads or save old value) */
    if (vdataReg && !vdataReg.isRegVar() && (vdataToWrite || haveTfe) && !haveLds)
        updateVGPRsNum(gcnRegs.vgprsNum, vdataReg.end-257);
    if (soffsetOp.range && !soffsetOp.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, soffsetOp.range.start, arch);
    return true;
}

bool GCNAsmUtils::parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    if (gcnEncSize==GCNEncSize::BIT32)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 64-bit size for MIMG encoding")
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    bool good = true;
    RegRange vaddrReg(0, 0);
    RegRange vdataReg(0, 0);
    RegRange ssampReg(0, 0);
    RegRange srsrcReg(0, 0);
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    skipSpacesToEnd(linePtr, end);
    const char* vdataPlace = linePtr;
    gcnAsm->setCurrentRVU(0);
    // parse VDATA (various VGPR number, verified later)
    good &= parseVRegRange(asmr, linePtr, vdataReg, 0, GCNFIELD_M_VDATA, true,
                    INSTROP_SYMREGRANGE|INSTROP_READ);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    skipSpacesToEnd(linePtr, end);
    const char* vaddrPlace = linePtr;
    gcnAsm->setCurrentRVU(1);
    // // parse VADDR (various VGPR number, verified later)
    good &= parseVRegRange(asmr, linePtr, vaddrReg, 0, GCNFIELD_M_VADDR, true,
                    INSTROP_SYMREGRANGE|INSTROP_READ);
    cxuint geRegRequired = (gcnInsn.mode&GCN_MIMG_VA_MASK)+1;
    cxuint vaddrRegsNum = vaddrReg.end-vaddrReg.start;
    cxuint vaddrMaxExtraRegs = (gcnInsn.mode&GCN_MIMG_VADERIV) ? 7 : 3;
    if (vaddrRegsNum < geRegRequired || vaddrRegsNum > geRegRequired+vaddrMaxExtraRegs)
    {
        char buf[60];
        snprintf(buf, 60, "Required (%u-%u) vector registers", geRegRequired,
                 geRegRequired+vaddrMaxExtraRegs);
        ASM_NOTGOOD_BY_ERROR(vaddrPlace, buf)
    }
    
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    skipSpacesToEnd(linePtr, end);
    const char* srsrcPlace = linePtr;
    gcnAsm->setCurrentRVU(2);
    // parse SRSRC (4 or 8 SGPR's) number of register verified later
    good &= parseSRegRange(asmr, linePtr, srsrcReg, arch, 0, GCNFIELD_M_SRSRC, true,
                    INSTROP_SYMREGRANGE|INSTROP_READ);
    
    if ((gcnInsn.mode & GCN_MIMG_SAMPLE) != 0)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        gcnAsm->setCurrentRVU(3);
        // parse SSAMP (4 SGPR's)
        good &= parseSRegRange(asmr, linePtr, ssampReg, arch, 4, GCNFIELD_MIMG_SSAMP,
                               true, INSTROP_SYMREGRANGE|INSTROP_READ);
    }
    
    bool haveTfe = false, haveSlc = false, haveGlc = false;
    bool haveDa = false, haveR128 = false, haveLwe = false, haveUnorm = false;
    bool haveDMask = false, haveD16 = false, haveA16 = false;
    cxbyte dmask = 0x1;
    /* modifiers and modifiers */
    while(linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        char name[10];
        const char* modPlace = linePtr;
        if (!getNameArgS(asmr, 10, name, linePtr, "MIMG modifier"))
        {
            good = false;
            continue;
        }
        toLowerString(name);
        
        if (name[0] == 'd')
        {
            if (name[1]=='a' && name[2]==0)
                // DA modifier
                good &= parseModEnable(asmr, linePtr, haveDa, "da modifier");
            else if ((arch & ARCH_GCN_1_2_4)!=0 && name[1]=='1' &&
                name[2]=='6' && name[3]==0)
                // D16 modifier
                good &= parseModEnable(asmr, linePtr, haveD16, "d16 modifier");
            else if (::strcmp(name+1, "mask")==0)
            {
                // parse dmask
                skipSpacesToEnd(linePtr, end);
                if (linePtr!=end && *linePtr==':')
                {
                    /* parse dmask immediate */
                    skipCharAndSpacesToEnd(linePtr, end);
                    const char* valuePlace = linePtr;
                    uint64_t value;
                    if (getAbsoluteValueArg(asmr, value, linePtr, true))
                    {
                        // detect multiple occurrences
                        if (haveDMask)
                            asmr.printWarning(modPlace, "Dmask is already defined");
                        haveDMask = true;
                        if (value>0xf)
                            asmr.printWarning(valuePlace, "Dmask out of range (0-15)");
                        dmask = value&0xf;
                        if (dmask == 0)
                            ASM_NOTGOOD_BY_ERROR(valuePlace, "Zero in dmask is illegal")
                    }
                    else
                        good = false;
                }
                else
                    ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before dmask")
            }
            else
                ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown MIMG modifier")
        }
        else if (name[0] < 's')
        {
            // glc, lwe, r128, a16 modifiers
            if (::strcmp(name, "glc")==0)
                good &= parseModEnable(asmr, linePtr, haveGlc, "glc modifier");
            else if (::strcmp(name, "lwe")==0)
                good &= parseModEnable(asmr, linePtr, haveLwe, "lwe modifier");
            else if (!isGCN14 && ::strcmp(name, "r128")==0)
                good &= parseModEnable(asmr, linePtr, haveR128, "r128 modifier");
            else if (isGCN14 && ::strcmp(name, "a16")==0)
                good &= parseModEnable(asmr, linePtr, haveA16, "a16 modifier");
            else
                ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown MIMG modifier")
        }
        // other modifiers
        else if (::strcmp(name, "tfe")==0)
            good &= parseModEnable(asmr, linePtr, haveTfe, "tfe modifier");
        else if (::strcmp(name, "slc")==0)
            good &= parseModEnable(asmr, linePtr, haveSlc, "slc modifier");
        else if (::strcmp(name, "unorm")==0)
            good &= parseModEnable(asmr, linePtr, haveUnorm, "unorm modifier");
        else
            ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown MIMG modifier")
    }
    
    cxuint dregsNum = 4;
    // check number of registers in VDATA
    if ((gcnInsn.mode & GCN_MIMG_VDATA4) == 0)
        dregsNum = ((dmask & 1)?1:0) + ((dmask & 2)?1:0) + ((dmask & 4)?1:0) +
                ((dmask & 8)?1:0) + (haveTfe);
    if (dregsNum!=0 && !isXRegRange(vdataReg, dregsNum))
    {
        char errorMsg[40];
        snprintf(errorMsg, 40, "Required %u vector register%s", dregsNum,
                 (dregsNum>1) ? "s" : "");
        ASM_NOTGOOD_BY_ERROR(vdataPlace, errorMsg)
    }
    // check number of registers in SRSRC
    if (!isXRegRange(srsrcReg, (haveR128)?4:8))
        ASM_NOTGOOD_BY_ERROR(srsrcPlace, (haveR128) ? "Required 4 scalar registers" :
                    "Required 8 scalar registers")
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    /* checking modifiers conditions */
    if (!haveUnorm && ((gcnInsn.mode&GCN_MLOAD) == 0 || (gcnInsn.mode&GCN_MATOMIC)!=0))
        // unorm is not set for this instruction
        ASM_FAIL_BY_ERROR(instrPlace, "Unorm is not set for store or atomic instruction")
    
    const bool vdataToWrite = ((gcnInsn.mode&GCN_MLOAD) != 0 ||
                ((gcnInsn.mode&GCN_MATOMIC)!=0 && haveGlc));
    const bool vdataToRead = ((gcnInsn.mode&GCN_MLOAD) == 0 ||
                ((gcnInsn.mode&GCN_MATOMIC)!=0));
    
    // fix alignment
    if (gcnAsm->instrRVUs[2].regVar != nullptr)
        gcnAsm->instrRVUs[2].align = 4;
    
    prepareRVUAndWait(gcnAsm, arch, vdataToRead, vdataToWrite, false, haveTfe,
                      output, gcnInsn);
    
    // put instruction words
    uint32_t words[2];
    SLEV(words[0], 0xf0000000U | (uint32_t(dmask&0xf)<<8) | (haveUnorm ? 0x1000U : 0) |
        (haveGlc ? 0x2000U : 0) | (haveDa ? 0x4000U : 0) |
        (haveR128|haveA16 ? 0x8000U : 0) |
        (haveTfe ? 0x10000U : 0) | (haveLwe ? 0x20000U : 0) |
        (uint32_t(gcnInsn.code1)<<18) | (haveSlc ? (1U<<25) : 0));
    SLEV(words[1], (vaddrReg.bstart()&0xff) | (uint32_t(vdataReg.bstart()&0xff)<<8) |
            (uint32_t(srsrcReg.bstart()>>2)<<16) | (uint32_t(ssampReg.bstart()>>2)<<21) |
            (haveD16 ? (1U<<31) : 0));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    // update register pool (instr loads or save old value) */
    if (vdataReg && !vdataReg.isRegVar() && (vdataToWrite || haveTfe))
        updateVGPRsNum(gcnRegs.vgprsNum, vdataReg.end-257);
    return true;
}

bool GCNAsmUtils::parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    if (gcnEncSize==GCNEncSize::BIT32)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 64-bit size for EXP encoding")
    bool good = true;
    cxbyte enMask = 0xf;
    cxbyte target = 0;
    RegRange vsrcsReg[4];
    const char* vsrcPlaces[4];
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    char name[20];
    skipSpacesToEnd(linePtr, end);
    const char* targetPlace = linePtr;
    
    try
    {
    if (getNameArg(asmr, 20, name, linePtr, "target"))
    {
        size_t nameSize = linePtr-targetPlace;
        const char* nameStart = name;
        toLowerString(name);
        // parse mrt / mrtz / mrt0 - mrt7
        if (name[0]=='m' && name[1]=='r' && name[2]=='t')
        {
            // parse mrtX target
            if (name[3]!='z' || name[4]!=0)
            {
                nameStart+=3;
                target = cstrtobyte(nameStart, name+nameSize);
                if (target>=8)
                    ASM_NOTGOOD_BY_ERROR(targetPlace, "MRT number out of range (0-7)")
            }
            else
                target = 8; // mrtz
        }
        // parse pos0 - pos3
        else if (name[0]=='p' && name[1]=='o' && name[2]=='s')
        {
            // parse pos target
            nameStart+=3;
            cxbyte posNum = cstrtobyte(nameStart, name+nameSize);
            if (posNum>=4)
                ASM_NOTGOOD_BY_ERROR(targetPlace, "Pos number out of range (0-3)")
            else
                target = posNum+12;
        }
        else if (strcmp(name, "null")==0)
            target = 9;
        // param0 - param 31
        else if (memcmp(name, "param", 5)==0)
        {
            nameStart+=5;
            cxbyte posNum = cstrtobyte(nameStart, name+nameSize);
            if (posNum>=32)
                ASM_NOTGOOD_BY_ERROR(targetPlace, "Param number out of range (0-31)")
            else
                target = posNum+32;
        }
        else
            ASM_NOTGOOD_BY_ERROR(targetPlace, "Unknown EXP target")
    }
    else
        good = false;
    }
    catch (const ParseException& ex)
    {
        // number parsing error
        asmr.printError(targetPlace, ex.what());
        good = false;
    }
    
    /* parse VSRC0-3 registers */
    for (cxuint i = 0; i < 4; i++)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        skipSpacesToEnd(linePtr, end);
        vsrcPlaces[i] = linePtr;
        // if not 'off', then parse vector register
        if (linePtr+2>=end || toLower(linePtr[0])!='o' || toLower(linePtr[1])!='f' ||
            toLower(linePtr[2])!='f' || (linePtr+3!=end && isAlnum(linePtr[3])))
        {
            gcnAsm->setCurrentRVU(i);
            good &= parseVRegRange(asmr, linePtr, vsrcsReg[i], 1, GCNFIELD_EXP_VSRC0+i,
                        true, INSTROP_SYMREGRANGE|INSTROP_READ);
            gcnAsm->delayedOps[i] = { output.size(), gcnAsm->instrRVUs[i].regVar,
                        gcnAsm->instrRVUs[i].rstart, gcnAsm->instrRVUs[i].rend,
                        1, GCNDELOP_EXPORT, GCNDELOP_NONE, gcnAsm->instrRVUs[i].rwFlags };
        }
        else
        {
            // if vsrcX is off
            enMask &= ~(1U<<i);
            vsrcsReg[i] = { 0, 0 };
            linePtr += 3;
        }
    }
    
    /* EXP modifiers */
    bool haveVM = false, haveCompr = false, haveDone = false;
    while(linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        const char* modPlace = linePtr;
        if (!getNameArgS(asmr, 10, name, linePtr, "EXP modifier"))
        {
            good = false;
            continue;
        }
        toLowerString(name);
        if (name[0]=='v' && name[1]=='m' && name[2]==0)
            good &= parseModEnable(asmr, linePtr, haveVM, "vm modifier");
        else if (::strcmp(name, "done")==0)
            good &= parseModEnable(asmr, linePtr, haveDone, "done modifier");
        else if (::strcmp(name, "compr")==0)
            good &= parseModEnable(asmr, linePtr, haveCompr, "compr modifier");
        else
            ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown EXP modifier")
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    // checking whether VSRC's is correct in compr mode if enabled
    if (haveCompr && !vsrcsReg[0].isRegVar() && !vsrcsReg[1].isRegVar() &&
            !vsrcsReg[0].isRegVar() && !vsrcsReg[1].isRegVar())
    {
        if (vsrcsReg[0].start!=vsrcsReg[1].start && (enMask&3)==3)
            // error (vsrc1!=vsrc0)
            ASM_FAIL_BY_ERROR(vsrcPlaces[1], "VSRC1 must be equal to VSRC0 in compr mode")
        if (vsrcsReg[2].start!=vsrcsReg[3].start && (enMask&12)==12)
            // error (vsrc3!=vsrc2)
            ASM_FAIL_BY_ERROR(vsrcPlaces[3], "VSRC3 must be equal to VSRC2 in compr mode")
        vsrcsReg[1] = vsrcsReg[2];
        vsrcsReg[2] = vsrcsReg[3] = { 0, 0 };
    }
    
    // put instruction words
    uint32_t words[2];
    SLEV(words[0], ((arch&ARCH_GCN_1_2_4) ? 0xc4000000 : 0xf8000000U) | enMask |
            (uint32_t(target)<<4) | (haveCompr ? 0x400 : 0) | (haveDone ? 0x800 : 0) |
            (haveVM ? 0x1000U : 0));
    SLEV(words[1], uint32_t(vsrcsReg[0].bstart()&0xff) |
            (uint32_t(vsrcsReg[1].bstart()&0xff)<<8) |
            (uint32_t(vsrcsReg[2].bstart()&0xff)<<16) |
            (uint32_t(vsrcsReg[3].bstart()&0xff)<<24));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    return true;
}

bool GCNAsmUtils::parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    if (gcnEncSize==GCNEncSize::BIT32)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 64-bit size for FLAT encoding")
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    const cxuint flatMode = (gcnInsn.mode & GCN_FLAT_MODEMASK);
    bool good = true;
    RegRange vaddrReg(0, 0);
    RegRange vdstReg(0, 0);
    RegRange vdataReg(0, 0);
    RegRange saddrReg(0, 0);
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    skipSpacesToEnd(linePtr, end);
    const char* vdstPlace = nullptr;
    
    bool vaddrOff = false;
    const cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
    
    const cxuint addrRegsNum = (flatMode != GCN_FLAT_SCRATCH ?
                (flatMode==GCN_FLAT_FLAT ? 2 : 0)  : 1);
    const char* addrPlace = nullptr;
    if ((gcnInsn.mode & GCN_FLAT_ADST) == 0)
    {
        // first is destination
        vdstPlace = linePtr;
        
        gcnAsm->setCurrentRVU(0);
        good &= parseVRegRange(asmr, linePtr, vdstReg, 0, GCNFIELD_FLAT_VDST, true,
                        INSTROP_SYMREGRANGE|INSTROP_WRITE);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        skipSpacesToEnd(linePtr, end);
        addrPlace = linePtr;
        if (flatMode == GCN_FLAT_SCRATCH && linePtr+3<=end &&
            strncasecmp(linePtr, "off", 3)==0 && (linePtr+3==end || !isAlnum(linePtr[3])))
        {
            // // if 'off' word
            vaddrOff = true;
            linePtr+=3;
        }
        else
        {
            gcnAsm->setCurrentRVU(1);
            // parse VADDR (1 or 2 VGPR's)
            good &= parseVRegRange(asmr, linePtr, vaddrReg, addrRegsNum,
                    GCNFIELD_FLAT_ADDR, true, INSTROP_SYMREGRANGE|INSTROP_READ);
        }
    }
    else
    {
        // first is data
        skipSpacesToEnd(linePtr, end);
        addrPlace = linePtr;
        if (flatMode == GCN_FLAT_SCRATCH && linePtr+3<=end &&
            strncasecmp(linePtr, "off", 3)==0 && (linePtr+3==end || !isAlnum(linePtr[3])))
        {
            // if 'off' word
            vaddrOff = true;
            linePtr+=3;
        }
        else
        {
            gcnAsm->setCurrentRVU(1);
            // parse VADDR (1 or 2 VGPR's)
            good &= parseVRegRange(asmr, linePtr, vaddrReg, addrRegsNum,
                        GCNFIELD_FLAT_ADDR, true, INSTROP_SYMREGRANGE|INSTROP_READ);
        }
        if ((gcnInsn.mode & GCN_FLAT_NODST) == 0)
        {
            if (!skipRequiredComma(asmr, linePtr))
                return false;
            skipSpacesToEnd(linePtr, end);
            vdstPlace = linePtr;
            gcnAsm->setCurrentRVU(0);
            // parse VDST (VGPRs, various number of register, verified later)
            good &= parseVRegRange(asmr, linePtr, vdstReg, 0, GCNFIELD_FLAT_VDST, true,
                        INSTROP_SYMREGRANGE|INSTROP_WRITE);
        }
    }
    
    if ((gcnInsn.mode & GCN_FLAT_NODATA) == 0) /* print data */
    {
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        gcnAsm->setCurrentRVU(2);
        // parse VDATA (VGPRS, 1-4 registers)
        good &= parseVRegRange(asmr, linePtr, vdataReg, dregsNum, GCNFIELD_FLAT_DATA,
                               true, INSTROP_SYMREGRANGE|INSTROP_READ);
    }
    
    bool saddrOff = false;
    if (flatMode != 0)
    {
        // SADDR
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        skipSpacesToEnd(linePtr, end);
        if (flatMode != 0 && linePtr+3<=end && strncasecmp(linePtr, "off", 3)==0 &&
            (linePtr+3==end || !isAlnum(linePtr[3])))
        {  // if 'off' word
            saddrOff = true;
            linePtr+=3;
        }
        else
        {
            gcnAsm->setCurrentRVU(3);
            good &= parseSRegRange(asmr, linePtr, saddrReg, arch,
                        (flatMode==GCN_FLAT_SCRATCH ? 1 : 2), GCNFIELD_FLAT_SADDR, true,
                        INSTROP_SYMREGRANGE|INSTROP_READ);
        }
    }
    
    if (addrRegsNum == 0)
    {
        // check size of addrRange
        // if SADDR then 1 VADDR offset register, otherwise 2 VADDR VGPRs
        cxuint reqAddrRegsNum = saddrOff ? 2 : 1;
        if (!isXRegRange(vaddrReg, reqAddrRegsNum))
        {
            char errorMsg[40];
            snprintf(errorMsg, 40, "Required %u vector register%s", reqAddrRegsNum,
                     (reqAddrRegsNum>1) ? "s" : "");
            ASM_NOTGOOD_BY_ERROR(addrPlace, errorMsg)
        }
    }
    
    if (flatMode == GCN_FLAT_SCRATCH && !saddrOff && !vaddrOff)
        ASM_NOTGOOD_BY_ERROR(instrPlace, "Only one of VADDR and SADDR can be set in "
                    "SCRATCH mode")
    
    if (saddrOff)
        saddrReg.start = 0x7f;
    if (vaddrOff)
        vaddrReg.start = 0x00;
    
    uint16_t instOffset = 0;
    std::unique_ptr<AsmExpression> instOffsetExpr;
    bool haveTfe = false, haveSlc = false, haveGlc = false;
    bool haveNv = false, haveLds = false, haveInstOffset = false;
    
    // main loop to parsing FLAT modifiers
    while(linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        char name[20];
        const char* modPlace = linePtr;
        // get modifier name
        if (!getNameArgS(asmr, 20, name, linePtr, "FLAT modifier"))
        {
            good = false;
            continue;
        }
        // only GCN1.2 modifiers
        if (!isGCN14 && ::strcmp(name, "tfe") == 0)
            good &= parseModEnable(asmr, linePtr, haveTfe, "tfe modifier");
        // only GCN1.4 modifiers
        else if (isGCN14 && ::strcmp(name, "nv") == 0)
            good &= parseModEnable(asmr, linePtr, haveNv, "nv modifier");
        else if (isGCN14 && ::strcmp(name, "lds") == 0)
            good &= parseModEnable(asmr, linePtr, haveLds, "lds modifier");
        // GCN 1.2/1.4 modifiers
        else if (::strcmp(name, "glc") == 0)
            good &= parseModEnable(asmr, linePtr, haveGlc, "glc modifier");
        else if (::strcmp(name, "slc") == 0)
            good &= parseModEnable(asmr, linePtr, haveSlc, "slc modifier");
        else if (isGCN14 && ::strcmp(name, "inst_offset")==0)
        {
            // parse inst_offset, 13-bit with sign, or 12-bit unsigned
            if (parseModImm(asmr, linePtr, instOffset, &instOffsetExpr, "inst_offset",
                            flatMode!=0 ? 13 : 12, flatMode!=0 ? WS_BOTH : WS_UNSIGNED))
            {
                if (haveInstOffset)
                    asmr.printWarning(modPlace, "InstOffset is already defined");
                haveInstOffset = true;
            }
            else
                good = false;
        }
        else
            ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown FLAT modifier")
    }
    /* check register ranges */
    bool dstToWrite = vdstReg && ((gcnInsn.mode & GCN_MATOMIC)==0 || haveGlc);
    if (vdstReg)
    {
        cxuint dstRegsNum = ((gcnInsn.mode & GCN_CMPSWAP)!=0) ? (dregsNum>>1) : dregsNum;
        dstRegsNum = (haveTfe) ? dstRegsNum+1:dstRegsNum; // include tfe 
        // check number of registers for VDST
        if (!isXRegRange(vdstReg, dstRegsNum))
        {
            char errorMsg[40];
            snprintf(errorMsg, 40, "Required %u vector register%s", dstRegsNum,
                     (dstRegsNum>1) ? "s" : "");
            ASM_NOTGOOD_BY_ERROR(vdstPlace, errorMsg)
        }
        
        if (haveTfe && vdstReg && gcnAsm->instrRVUs[0].regField != ASMFIELD_NONE)
        {
            // fix for tfe
            AsmRegVarUsage& rvu = gcnAsm->instrRVUs[0];
            AsmRegVarUsage& lastRvu = gcnAsm->instrRVUs[3];
            lastRvu = rvu;
            lastRvu.rstart = lastRvu.rend-1;
            lastRvu.rwFlags = ASMRVU_READ|ASMRVU_WRITE;
            lastRvu.regField = GCNFIELD_FLAT_VDSTLAST;
            rvu.rend--;
        }
        
        if (!dstToWrite)
            gcnAsm->instrRVUs[0].regField = ASMFIELD_NONE;
    }
    
    if (haveLds && flatMode == GCN_FLAT_FLAT)
        ASM_NOTGOOD_BY_ERROR(vdstPlace,
                        "LDS is allowed only for SCRATCH and GLOBAL instructions")
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    const cxbyte secondDelOpType = (flatMode==GCN_FLAT_FLAT) ? GCNDELOP_LDSOP :
                GCNDELOP_NONE;
    if (gcnAsm->instrRVUs[0].regField != ASMFIELD_NONE)
    {
        if (!haveLds)
            gcnAsm->delayedOps[0] = { output.size(), gcnAsm->instrRVUs[0].regVar,
                    gcnAsm->instrRVUs[0].rstart, gcnAsm->instrRVUs[0].rend,
                    1, GCNDELOP_VMOP, secondDelOpType, gcnAsm->instrRVUs[0].rwFlags,
                    cxbyte(secondDelOpType!=GCNDELOP_NONE ?
                            gcnAsm->instrRVUs[0].rwFlags : 0) };
        else
            gcnAsm->delayedOps[0] = { output.size(), nullptr, uint16_t(0), uint16_t(0),
                        1, GCNDELOP_VMOP, secondDelOpType, cxbyte(0), cxbyte(0) };
        
        if (haveTfe && vdstReg && !haveLds)
            gcnAsm->delayedOps[1] = { output.size(), gcnAsm->instrRVUs[3].regVar,
                    gcnAsm->instrRVUs[3].rstart, gcnAsm->instrRVUs[3].rend, 1,
                    GCNDELOP_VMOP, GCNDELOP_NONE, gcnAsm->instrRVUs[3].rwFlags };
    }
    if ((gcnInsn.mode & GCN_FLAT_NODATA) == 0)
        gcnAsm->delayedOps[2] = { output.size(), gcnAsm->instrRVUs[2].regVar,
                gcnAsm->instrRVUs[2].rstart, gcnAsm->instrRVUs[2].rend,
                1, GCNDELOP_VMOP, secondDelOpType, gcnAsm->instrRVUs[2].rwFlags,
                cxbyte(secondDelOpType!=GCNDELOP_NONE ?
                            gcnAsm->instrRVUs[2].rwFlags : 0) };
    
    if (instOffsetExpr!=nullptr)
        instOffsetExpr->setTarget(AsmExprTarget(flatMode!=0 ?
                    GCNTGT_INSTOFFSET_S : GCNTGT_INSTOFFSET, asmr.currentSection,
                    output.size()));
    
    // put data (instruction words)
    uint32_t words[2];
    SLEV(words[0], 0xdc000000U | (haveGlc ? 0x10000 : 0) | (haveSlc ? 0x20000: 0) |
            (uint32_t(gcnInsn.code1)<<18) | (haveLds ? 0x2000U : 0) | instOffset |
            (uint32_t(flatMode)<<14));
    SLEV(words[1], (vaddrReg.bstart()&0xff) | (uint32_t(vdataReg.bstart()&0xff)<<8) |
            (haveTfe|haveNv ? (1U<<23) : 0) | (uint32_t(vdstReg.bstart()&0xff)<<24) |
            (uint32_t(saddrReg.bstart())<<16));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    instOffsetExpr.release();
    // update register pool
    if (vdstReg && !vdstReg.isRegVar() && (dstToWrite || haveTfe))
        updateVGPRsNum(gcnRegs.vgprsNum, vdstReg.end-257);
    return true;
}

};
