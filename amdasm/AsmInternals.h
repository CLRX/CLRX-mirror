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
    REG_ALL_32 = 0,
    REG_ALL_64 = 15,
    REG_DST_64 = 1,
    REG_SRC0_64 = 2,
    REG_SRC1_64 = 4,
    REG_SRC2_64 = 8,
    REG_DS0_64 = 3,
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
