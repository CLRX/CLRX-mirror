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
#include <CLRX/Config.h.in>

namespace CLRX
{

enum class GCNEncoding: cxbyte
{
    SOPC,
    SOPP,
    SOP1,
    SOP2,
    SOPK,
    SMRD,
    VOPC,
    VOP1,
    VOP2,
    VOP3A,
    VOP3,
    VINTRP,
    DS,
    MUBUF,
    MTBUF,
    MIMG,
    EXP,
    FLAT
};

enum : cxuint
{
    ARCH_SOUTHERN_ISLANDS = 1,
    ARCH_SEA_ISLANDS = 2,
    ARCH_HD7X00 = 1,
    ARCH_RX2X0 = 2,
};

enum : cxuint
{
    GCN_REG_ALL_32 = 0,
    GCN_REG_ALL_64 = 15,
    GCN_REG_DST_64 = 1,
    GCN_REG_SRC0_64 = 2,
    GCN_REG_SRC1_64 = 4,
    GCN_REG_SRC2_64 = 8,
    GCN_REG_DS0_64 = 3,
    GCN_MODE_NORMAL = 0,
    GCN_IMM_NONE = 0x10,
    GCN_ARG_NONE = 0x20,
    GCN_REG_S1_JMP = 0x20,
    GCN_IMM_REL = 0x30,
    GCN_IMM_LOCKS = 0x40,
    GCN_IMM_MSGS = 0x50,
    GCN_SRC2_NONE = 0x60,
    GCN_SRC2_VCC = 0x70,
    GCN_SRC12_NONE = 0x80,
    GCN_MEMOP_MX1 = 0x0,
    GCN_MEMOP_MX2 = 0x100,
    GCN_MEMOP_MX4 = 0x200,
    GCN_MEMOP_MX8 = 0x300,
    GCN_MEMOP_MX16 = 0x400,
};

struct GCNInstruction
{
    GCNEncoding encoding;
    const char* mnemonic;
    cxuint mode;
    cxuint code;
    cxuint archMask; // mask of architectures whose have instruction
};

};

#endif
