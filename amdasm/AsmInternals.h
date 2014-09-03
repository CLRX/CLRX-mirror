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

#ifndef __CLRX_ASMINTERNALS_H__
#define __CLRX_ASMINTERNALS_H__

#include <CLRX/Config.h>
#include <cstdint>

namespace CLRX
{

enum GCNEncoding: cxbyte
{
    GCNENC_SOPC,
    GCNENC_SOPP,
    GCNENC_SOP1,
    GCNENC_SOP2,
    GCNENC_SOPK,
    GCNENC_SMRD,
    GCNENC_VOPC,
    GCNENC_VOP1,
    GCNENC_VOP2,
    GCNENC_VOP3A,
    GCNENC_VOP3B,
    GCNENC_VINTRP,
    GCNENC_DS,
    GCNENC_MUBUF,
    GCNENC_MTBUF,
    GCNENC_MIMG,
    GCNENC_EXP,
    GCNENC_FLAT
};

enum : uint16_t
{
    ARCH_SOUTHERN_ISLANDS = 1,
    ARCH_SEA_ISLANDS = 2,
    ARCH_HD7X00 = 1,
    ARCH_RX2X0 = 2,
    ARCH_GCN_ALL = 0xff,
};

enum : uint16_t
{
    GCN_STDMODE = 0,
    GCN_REG_ALL_64 = 15,
    GCN_REG_DST_64 = 1,
    GCN_REG_SRC0_64 = 2,
    GCN_REG_SRC1_64 = 4,
    GCN_REG_SRC2_64 = 8,
    GCN_REG_DS0_64 = 3,
    GCN_REG_DS2_64 = 9,
    GCN_IMM_NONE = 0x10,
    GCN_ARG_NONE = 0x20,
    GCN_REG_S1_JMP = 0x20,
    GCN_IMM_REL = 0x30,
    GCN_IMM_LOCKS = 0x40,
    GCN_IMM_MSGS = 0x50,
    GCN_IMM_SREG = 0x60,
    GCN_SRC2_NONE = 0x70,
    GCN_SRC2_VCC = 0x80,
    GCN_SRC12_NONE = 0x90,
    GCN_ARG1_IMM = 0xa0,
    GCN_ARG2_IMM = 0xb0,
    GCN_ARGS_VCMP = 0xc0,
    GCN_ARGS_VCMP64 = 0xcf,
    GCN_S0EQS12 = 0xd0,
    // DS encoding modes
    GCN_ADDR_NONE = 0x0,
    GCN_ADDR_DST = 0x10,
    GCN_ADDR_SRC = 0x20,
    GCN_ADDR_DST64 = 0x1f,
    GCN_ADDR_SRC64 = 0x2f,
    GCN_VDATA2 = 0x40,
    GCN_NOSRC = 0x80,
    GCN_2SRCS = 0xc0,
    GCN_SRC_ADDR2  = 0x100,
    GCN_SRC_ADDR2_64  = 0x10f,
    GCN_ADDR_DST96  = 0x150,
    GCN_ADDR_DST128  = 0x190,
    GCN_ADDR_SRC96  = 0x160,
    GCN_ADDR_SRC128  = 0x1a0,
    GCN_ONLYDST = 0x200,
    // others
    GCN_FLOATLIT = 0x100,
    GCN_F16LIT = 0x200,
    GCN_MEMOP_MX1 = 0x0,
    GCN_MEMOP_MX2 = 0x100,
    GCN_MEMOP_MX4 = 0x200,
    GCN_MEMOP_MX8 = 0x300,
    GCN_MEMOP_MX16 = 0x400,
    GCN_MUBUF_X = 0x0,
    GCN_MUBUF_XY = 0x100,
    GCN_MUBUF_XYZ = 0x200,
    GCN_MUBUF_XYZW = 0x300,
    GCN_MUBUF_MX1 = 0x400,
    GCN_MUBUF_MX2 = 0x500,
    GCN_MUBUF_MX3 = 0x600,
    GCN_MUBUF_MX4 = 0x700
};

struct CLRX_INTERNAL GCNInstruction
{
    const char* mnemonic;
    GCNEncoding encoding;
    uint16_t mode;
    uint16_t code;
    uint16_t archMask; // mask of architectures whose have instruction
};

CLRX_INTERNAL extern const CLRX::GCNInstruction gcnInstrsTable[];

};

#endif
