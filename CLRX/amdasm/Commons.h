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
/*! \file amdasm/Commons.h
 * \brief common definitions for assembler and disassembler
 */

#ifndef __CLRX_ASMCOMMONS_H__
#define __CLRX_ASMCOMMONS_H__

#include <CLRX/amdbin/Commons.h>

/// main namespace
namespace CLRX
{

/// type for Asm kernel id (index)
typedef cxuint AsmKernelId;
/// type for Asm section id (index)
typedef cxuint AsmSectionId;

/// binary format for Assembler/Disassembler
enum class BinaryFormat
{
    AMD = 0,    ///< AMD CATALYST format
    GALLIUM,     ///< GalliumCompute format
    RAWCODE,     ///< raw code format
    AMDCL2,      ///< AMD OpenCL 2.0 format
    ROCM         ///< ROCm (RadeonOpenCompute) format
};

#define CLRX_POLICY_UNIFIED_SGPR_COUNT (200U)

};

#endif
