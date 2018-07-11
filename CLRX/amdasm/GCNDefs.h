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
/*! \file GCNDefs.h
 * \brief an GCN definitions for assembler
 */

#ifndef __CLRX_GCNDEFS_H__
#define __CLRX_GCNDEFS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <CLRX/amdasm/AsmDefs.h>

/// main namespace
namespace CLRX
{

enum : AsmExprTargetType
{
    GCNTGT_LITIMM = 16,
    GCNTGT_SOPKSIMM16,
    GCNTGT_SOPJMP,
    GCNTGT_SMRDOFFSET,
    GCNTGT_DSOFFSET16,
    GCNTGT_DSOFFSET8_0,
    GCNTGT_DSOFFSET8_1,
    GCNTGT_MXBUFOFFSET,
    GCNTGT_SMEMOFFSET,
    GCNTGT_SOPCIMM8,
    GCNTGT_SMEMIMM,
    GCNTGT_SMEMOFFSETVEGA,
    GCNTGT_INSTOFFSET,
    GCNTGT_INSTOFFSET_S
};

enum : AsmRegField
{
    GCNFIELD_SSRC0 = ASMFIELD_NONE+1,
    GCNFIELD_SSRC1,
    GCNFIELD_SDST,
    GCNFIELD_SMRD_SBASE,
    GCNFIELD_SMRD_SDST,
    GCNFIELD_SMRD_SDSTH,
    GCNFIELD_SMRD_SOFFSET,
    GCNFIELD_VOP_SRC0,
    GCNFIELD_VOP_VSRC1,
    GCNFIELD_VOP_SSRC1,
    GCNFIELD_VOP_VDST,
    GCNFIELD_VOP_SDST,
    GCNFIELD_VOP3_SRC0,
    GCNFIELD_VOP3_SRC1,
    GCNFIELD_VOP3_SRC2,
    GCNFIELD_VOP3_VDST,
    GCNFIELD_VOP3_SDST0,
    GCNFIELD_VOP3_SSRC,
    GCNFIELD_VOP3_SDST1,
    GCNFIELD_VINTRP_VSRC0,
    GCNFIELD_VINTRP_VDST,
    GCNFIELD_DS_ADDR,
    GCNFIELD_DS_DATA0,
    GCNFIELD_DS_DATA1,
    GCNFIELD_DS_VDST,
    GCNFIELD_M_VADDR,
    GCNFIELD_M_VDATA,
    GCNFIELD_M_VDATAH,
    GCNFIELD_M_VDATALAST,
    GCNFIELD_M_SRSRC,
    GCNFIELD_MIMG_SSAMP,
    GCNFIELD_M_SOFFSET,
    GCNFIELD_EXP_VSRC0,
    GCNFIELD_EXP_VSRC1,
    GCNFIELD_EXP_VSRC2,
    GCNFIELD_EXP_VSRC3,
    GCNFIELD_FLAT_ADDR,
    GCNFIELD_FLAT_DATA,
    GCNFIELD_FLAT_VDST,
    GCNFIELD_FLAT_VDSTLAST,
    GCNFIELD_DPPSDWA_SRC0,
    GCNFIELD_DPPSDWA_SSRC0,
    GCNFIELD_FLAT_SADDR,
    GCNFIELD_SDWAB_SDST,
    GCNFIELD_VOP_VCC_SSRC,
    GCNFIELD_VOP_VCC_SDST0,
    GCNFIELD_VOP_VCC_SDST1
};

enum : cxbyte
{
    GCNWAIT_VMCNT = 0,
    GCNWAIT_LGKMCNT,
    GCNWAIT_EXPCNT,
    GCNWAIT_MAX = GCNWAIT_EXPCNT
};

enum : cxbyte
{
    GCNDELOP_VMOP = 0,
    GCNDELOP_LDSOP,
    GCNDELOP_GDSOP,
    GCNDELOP_SENDMSG,
    GCNDELOP_SMEMOP,
    GCNDELOP_EXPVMWRITE,
    GCNDELOP_EXPORT,
    GCNDELOP_MAX = GCNDELOP_EXPORT,
    GCNDELOP_NONE = ASMDELOP_NONE
};

};

#endif
