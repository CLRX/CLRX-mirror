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
#include <iostream>
#include <sstream>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include "../TestUtils.h"

using namespace CLRX;

// test results for AMD kernel argument types
static const AmdKernelArg expectedKernelArgs1[] =
{
    { KernelArgType::UCHAR, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar", "v0" },
    { KernelArgType::UCHAR2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar2", "v1" },
    { KernelArgType::UCHAR3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar3", "v2" },
    { KernelArgType::UCHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar4", "v3" },
    { KernelArgType::UCHAR8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar8", "v4" },
    { KernelArgType::UCHAR16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar16", "v5" },
    { KernelArgType::CHAR, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char", "v6" },
    { KernelArgType::CHAR2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char2", "v7" },
    { KernelArgType::CHAR3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char3", "v8" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char4", "v9" },
    { KernelArgType::CHAR8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char8", "v10" },
    { KernelArgType::CHAR16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char16", "v11" },
    { KernelArgType::USHORT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort", "v12" },
    { KernelArgType::USHORT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort2", "v13" },
    { KernelArgType::USHORT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort3", "v14" },
    { KernelArgType::USHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort4", "v15" },
    { KernelArgType::USHORT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort8", "v16" },
    { KernelArgType::USHORT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort16", "v17" },
    { KernelArgType::SHORT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short", "v18" },
    { KernelArgType::SHORT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short2", "v19" },
    { KernelArgType::SHORT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short3", "v20" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short4", "v21" },
    { KernelArgType::SHORT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short8", "v22" },
    { KernelArgType::SHORT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short16", "v23" },
    { KernelArgType::UINT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint", "v24" },
    { KernelArgType::UINT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint2", "v25" },
    { KernelArgType::UINT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint3", "v26" },
    { KernelArgType::UINT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint4", "v27" },
    { KernelArgType::UINT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint8", "v28" },
    { KernelArgType::UINT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint16", "v29" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int", "v30" },
    { KernelArgType::INT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int2", "v31" },
    { KernelArgType::INT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int3", "v32" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int4", "v33" },
    { KernelArgType::INT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int8", "v34" },
    { KernelArgType::INT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int16", "v35" },
    { KernelArgType::ULONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong", "v36" },
    { KernelArgType::ULONG2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong2", "v37" },
    { KernelArgType::ULONG3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong3", "v38" },
    { KernelArgType::ULONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong4", "v39" },
    { KernelArgType::ULONG8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong8", "v40" },
    { KernelArgType::ULONG16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong16", "v41" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long", "v42" },
    { KernelArgType::LONG2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long2", "v43" },
    { KernelArgType::LONG3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long3", "v44" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long4", "v45" },
    { KernelArgType::LONG8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long8", "v46" },
    { KernelArgType::LONG16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long16", "v47" },
    { KernelArgType::SAMPLER, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "sampler_t", "v48" },
    { KernelArgType::IMAGE1D, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_t", "v49" },
    { KernelArgType::IMAGE1D, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_t", "v50" },
    { KernelArgType::IMAGE2D, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image2d_t", "v51" },
    { KernelArgType::IMAGE2D, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image2d_t", "v52" },
    { KernelArgType::IMAGE3D, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image3d_t", "v53" },
    { KernelArgType::IMAGE3D, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image3d_t", "v54" },
    { KernelArgType::IMAGE1D_ARRAY, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_array_t", "v55" },
    { KernelArgType::IMAGE1D_ARRAY, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_array_t", "v56" },
    { KernelArgType::IMAGE1D_BUFFER, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_buffer_t", "v57" },
    { KernelArgType::IMAGE1D_BUFFER, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_buffer_t", "v58" },
    { KernelArgType::IMAGE2D_ARRAY, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image2d_array_t", "v59" },
    { KernelArgType::IMAGE2D_ARRAY, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image2d_array_t", "v60" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_CONST, "void*", "v61" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "void*", "v62" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_CONST, "void*", "v63" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_NORMAL, "void*", "v64" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_CONST, "void*", "v65" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_NORMAL, "void*", "v66" },
    { KernelArgType::ULONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "typ1", "v67" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_VOLATILE, "void*", "v68" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_RESTRICT, "void*", "v69" },
    { KernelArgType::COUNTER32, KernelPtrSpace::NONE, KARG_PTR_NORMAL,
        "counter32_t", "v70" }
};

static const AmdKernelArg expectedKernelArgs2[] =
{
    { KernelArgType::STRUCTURE, KernelPtrSpace::NONE, KARG_PTR_NORMAL,
        "struct mystruct", "b" },
    { KernelArgType::UINT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint", "n" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "float*", "inout" },
    { KernelArgType::STRUCTURE, KernelPtrSpace::NONE, KARG_PTR_NORMAL,
        "union myunion", "ddv" },
    { KernelArgType::STRUCTURE, KernelPtrSpace::NONE, KARG_PTR_NORMAL,
        "struct mystructsub2", "xdf" },
    { KernelArgType::UINT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "EnumX", "enumx" }
};

static const AmdKernelArg expectedCPUKernelArgs1[] =
{
    { KernelArgType::CHAR, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar", "v0" },
    { KernelArgType::CHAR2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar2", "v1" },
    { KernelArgType::CHAR3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar3", "v2" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar4", "v3" },
    { KernelArgType::CHAR8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar8", "v4" },
    { KernelArgType::CHAR16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar16", "v5" },
    { KernelArgType::CHAR, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char", "v6" },
    { KernelArgType::CHAR2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char2", "v7" },
    { KernelArgType::CHAR3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char3", "v8" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char4", "v9" },
    { KernelArgType::CHAR8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char8", "v10" },
    { KernelArgType::CHAR16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char16", "v11" },
    { KernelArgType::SHORT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort", "v12" },
    { KernelArgType::SHORT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort2", "v13" },
    { KernelArgType::SHORT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort3", "v14" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort4", "v15" },
    { KernelArgType::SHORT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort8", "v16" },
    { KernelArgType::SHORT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort16", "v17" },
    { KernelArgType::SHORT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short", "v18" },
    { KernelArgType::SHORT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short2", "v19" },
    { KernelArgType::SHORT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short3", "v20" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short4", "v21" },
    { KernelArgType::SHORT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short8", "v22" },
    { KernelArgType::SHORT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short16", "v23" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint", "v24" },
    { KernelArgType::INT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint2", "v25" },
    { KernelArgType::INT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint3", "v26" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint4", "v27" },
    { KernelArgType::INT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint8", "v28" },
    { KernelArgType::INT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint16", "v29" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int", "v30" },
    { KernelArgType::INT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int2", "v31" },
    { KernelArgType::INT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int3", "v32" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int4", "v33" },
    { KernelArgType::INT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int8", "v34" },
    { KernelArgType::INT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int16", "v35" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong", "v36" },
    { KernelArgType::LONG2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong2", "v37" },
    { KernelArgType::LONG3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong3", "v38" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong4", "v39" },
    { KernelArgType::LONG8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong8", "v40" },
    { KernelArgType::LONG16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong16", "v41" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long", "v42" },
    { KernelArgType::LONG2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long2", "v43" },
    { KernelArgType::LONG3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long3", "v44" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long4", "v45" },
    { KernelArgType::LONG8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long8", "v46" },
    { KernelArgType::LONG16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long16", "v47" },
    { KernelArgType::SAMPLER, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "sampler_t", "v48" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_t", "v49" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_t", "v50" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image2d_t", "v51" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image2d_t", "v52" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image3d_t", "v53" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image3d_t", "v54" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_array_t", "v55" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_array_t", "v56" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_buffer_t", "v57" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_buffer_t", "v58" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image2d_array_t", "v59" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image2d_array_t", "v60" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_CONST, "void*", "v61" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "void*", "v62" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_CONST, "void*", "v63" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_NORMAL, "void*", "v64" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_CONST, "void*", "v65" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_NORMAL, "void*", "v66" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "typ1", "v67" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_VOLATILE, "void*", "v68" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_RESTRICT, "void*", "v69" }
};

static const AmdKernelArg expectedCPUKernelArgs2[] =
{
    { KernelArgType::STRUCTURE, KernelPtrSpace::NONE, KARG_PTR_NORMAL,
        "struct mystruct", "b" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint", "n" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "float*", "inout" },
    { KernelArgType::STRUCTURE, KernelPtrSpace::NONE, KARG_PTR_NORMAL,
        "union myunion", "ddv" },
    { KernelArgType::STRUCTURE, KernelPtrSpace::NONE, KARG_PTR_NORMAL,
        "struct mystructsub2", "xdf" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "EnumX", "enumx" }
};

static const AmdKernelArg expectedCL2KernelArgs1[] =
{
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_0" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_1" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_2" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "size_t",
        "_.printf_buffer" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.vqueue_pointer" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.aqlwrap_pointer" },
    { KernelArgType::CHAR, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar", "v0" },
    { KernelArgType::CHAR2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar2", "v1" },
    { KernelArgType::CHAR3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar3", "v2" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar4", "v3" },
    { KernelArgType::CHAR8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar8", "v4" },
    { KernelArgType::CHAR16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar16", "v5" },
    { KernelArgType::CHAR, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char", "v6" },
    { KernelArgType::CHAR2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char2", "v7" },
    { KernelArgType::CHAR3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char3", "v8" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char4", "v9" },
    { KernelArgType::CHAR8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char8", "v10" },
    { KernelArgType::CHAR16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char16", "v11" },
    { KernelArgType::SHORT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort", "v12" },
    { KernelArgType::SHORT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort2", "v13" },
    { KernelArgType::SHORT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort3", "v14" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort4", "v15" },
    { KernelArgType::SHORT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort8", "v16" },
    { KernelArgType::SHORT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort16", "v17" },
    { KernelArgType::SHORT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short", "v18" },
    { KernelArgType::SHORT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short2", "v19" },
    { KernelArgType::SHORT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short3", "v20" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short4", "v21" },
    { KernelArgType::SHORT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short8", "v22" },
    { KernelArgType::SHORT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short16", "v23" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint", "v24" },
    { KernelArgType::INT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint2", "v25" },
    { KernelArgType::INT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint3", "v26" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint4", "v27" },
    { KernelArgType::INT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint8", "v28" },
    { KernelArgType::INT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint16", "v29" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int", "v30" },
    { KernelArgType::INT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int2", "v31" },
    { KernelArgType::INT3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int3", "v32" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int4", "v33" },
    { KernelArgType::INT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int8", "v34" },
    { KernelArgType::INT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int16", "v35" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong", "v36" },
    { KernelArgType::LONG2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong2", "v37" },
    { KernelArgType::LONG3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong3", "v38" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong4", "v39" },
    { KernelArgType::LONG8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong8", "v40" },
    { KernelArgType::LONG16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong16", "v41" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long", "v42" },
    { KernelArgType::LONG2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long2", "v43" },
    { KernelArgType::LONG3, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long3", "v44" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long4", "v45" },
    { KernelArgType::LONG8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long8", "v46" },
    { KernelArgType::LONG16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long16", "v47" },
    { KernelArgType::SAMPLER, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "sampler_t", "v48" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_t", "v49" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_t", "v50" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image2d_t", "v51" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image2d_t", "v52" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image3d_t", "v53" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image3d_t", "v54" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_array_t", "v55" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_array_t", "v56" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_buffer_t", "v57" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_buffer_t", "v58" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image2d_array_t", "v59" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image2d_array_t", "v60" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_CONST, "void*", "v61" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "void*", "v62" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_CONST, "void*", "v63" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_NORMAL, "void*", "v64" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_CONST, "void*", "v65" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_CONST, "void*", "v66" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "typ1", "v67" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "void*", "v68" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "void*", "v69" }
};

static const AmdKernelArg expectedCL2NewKernelArgs1[] =
{
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_0" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_1" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_2" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "size_t",
        "_.printf_buffer" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.vqueue_pointer" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.aqlwrap_pointer" },
    { KernelArgType::CHAR, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar", "v0" },
    { KernelArgType::CHAR2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar2", "v1" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar3", "v2" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar4", "v3" },
    { KernelArgType::CHAR8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar8", "v4" },
    { KernelArgType::CHAR16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uchar16", "v5" },
    { KernelArgType::CHAR, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char", "v6" },
    { KernelArgType::CHAR2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char2", "v7" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char3", "v8" },
    { KernelArgType::CHAR4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char4", "v9" },
    { KernelArgType::CHAR8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char8", "v10" },
    { KernelArgType::CHAR16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "char16", "v11" },
    { KernelArgType::SHORT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort", "v12" },
    { KernelArgType::SHORT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort2", "v13" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort3", "v14" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort4", "v15" },
    { KernelArgType::SHORT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort8", "v16" },
    { KernelArgType::SHORT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ushort16", "v17" },
    { KernelArgType::SHORT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short", "v18" },
    { KernelArgType::SHORT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short2", "v19" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short3", "v20" },
    { KernelArgType::SHORT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short4", "v21" },
    { KernelArgType::SHORT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short8", "v22" },
    { KernelArgType::SHORT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "short16", "v23" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint", "v24" },
    { KernelArgType::INT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint2", "v25" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint3", "v26" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint4", "v27" },
    { KernelArgType::INT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint8", "v28" },
    { KernelArgType::INT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "uint16", "v29" },
    { KernelArgType::INT, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int", "v30" },
    { KernelArgType::INT2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int2", "v31" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int3", "v32" },
    { KernelArgType::INT4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int4", "v33" },
    { KernelArgType::INT8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int8", "v34" },
    { KernelArgType::INT16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "int16", "v35" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong", "v36" },
    { KernelArgType::LONG2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong2", "v37" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong3", "v38" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong4", "v39" },
    { KernelArgType::LONG8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong8", "v40" },
    { KernelArgType::LONG16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "ulong16", "v41" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long", "v42" },
    { KernelArgType::LONG2, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long2", "v43" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long3", "v44" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long4", "v45" },
    { KernelArgType::LONG8, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long8", "v46" },
    { KernelArgType::LONG16, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "long16", "v47" },
    { KernelArgType::SAMPLER, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "sampler_t", "v48" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_t", "v49" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_t", "v50" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image2d_t", "v51" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image2d_t", "v52" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image3d_t", "v53" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image3d_t", "v54" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_array_t", "v55" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_array_t", "v56" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image1d_buffer_t", "v57" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image1d_buffer_t", "v58" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_ONLY,
        "image2d_array_t", "v59" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_WRITE_ONLY,
        "image2d_array_t", "v60" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_CONST, "void*", "v61" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "void*", "v62" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_CONST, "void*", "v63" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_NORMAL, "void*", "v64" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_CONST, "void*", "v65" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_CONST, "void*", "v66" },
    { KernelArgType::LONG4, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "typ1", "v67" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "void*", "v68" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "void*", "v69" }
};

static const AmdKernelArg expectedCL2KernelArgs2[] =
{
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_0" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_1" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.global_offset_2" },
    { KernelArgType::POINTER, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL, "size_t",
        "_.printf_buffer" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.vqueue_pointer" },
    { KernelArgType::LONG, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "size_t",
        "_.aqlwrap_pointer" },
    { KernelArgType::PIPE, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL,
        "pipe __global uint *", "in" },
    { KernelArgType::PIPE, KernelPtrSpace::GLOBAL, KARG_PTR_NORMAL,
        "pipe __global float *", "out" },
    { KernelArgType::POINTER, KernelPtrSpace::LOCAL, KARG_PTR_NORMAL, "uint*", "v" },
    { KernelArgType::CMDQUEUE, KernelPtrSpace::NONE, KARG_PTR_NORMAL, "queue_t", "cmdq" },
    { KernelArgType::STRUCTURE, KernelPtrSpace::NONE, KARG_PTR_NORMAL,
        "struct Xdata", "d1" },
    { KernelArgType::POINTER, KernelPtrSpace::CONSTANT, KARG_PTR_CONST,
        "struct Xdata*", "expected" },
    { KernelArgType::IMAGE, KernelPtrSpace::GLOBAL, KARG_PTR_READ_WRITE,
        "image2d_t", "rwimg" }
};

// testing kernel argument types for binaries
static void testKernelArgs(const char* filename, const char* kernelName,
               size_t expKernelArgsNum, const AmdKernelArg* expKernelArgs)
{
    const std::string testName = std::string("testKernelArgs:") + filename;
    
    Array<cxbyte> data = loadDataFromFile(filename);
    std::unique_ptr<AmdMainBinaryBase> base;
    if (isAmdCL2Binary(data.size(), data.data()))
        base.reset(new AmdCL2MainGPUBinary64(data.size(), data.data()));
    else // old Amd CL1.1/1.2 binary
        base.reset(createAmdBinaryFromCode(data.size(), data.data()));
    
    const KernelInfo& kernelInfo = base->getKernelInfo(size_t(0));
    
    assertValue(testName, "KernelName", CString(kernelName), kernelInfo.kernelName);
    assertValue(testName, "argInfosNum", expKernelArgsNum, kernelInfo.argInfos.size());
    
    const auto& argInfos = kernelInfo.argInfos;
    for (size_t i = 0; i < expKernelArgsNum; i++)
    {
        std::ostringstream oss;
        oss << "#" << i;
        const std::string caseName = oss.str();
        assertValue(testName, caseName+" argType", cxuint(expKernelArgs[i].argType),
                    cxuint(argInfos[i].argType));
        assertValue(testName, caseName+" ptrSpace", cxuint(expKernelArgs[i].ptrSpace),
                    cxuint(argInfos[i].ptrSpace));
        assertValue(testName, caseName+" ptrAccess", expKernelArgs[i].ptrAccess,
                    argInfos[i].ptrAccess);
        assertValue(testName, caseName+" argName", expKernelArgs[i].argName,
                    argInfos[i].argName);
        assertValue(testName, caseName+" typeName", expKernelArgs[i].typeName,
                    argInfos[i].typeName);
    }
}

static const cxbyte defaultHeader[32] = { };

static AmdMainGPUBinaryBase* genAmdBinWithMetadata(const std::string& metadata)
{
    static uint32_t code4 = 3333;
    AmdKernelInput kernelInput { "myKernel0", 0, nullptr, 32, defaultHeader,
        metadata.size(), metadata.c_str(), { }, false, { }, 4, (const cxbyte*)&code4 };
    AmdInput amdInput { false, GPUDeviceType::CAPE_VERDE, 0, nullptr, 0, "", "",
            {  kernelInput } };
    AmdGPUBinGenerator amdBinGen(&amdInput);
    
    Array<cxbyte> output;
    amdBinGen.generate(output);
    return static_cast<AmdMainGPUBinaryBase*>(createAmdBinaryFromCode(
                output.size(), output.data()));
}

static void tryGenAmdBinWithMetadata(const std::string& metadata)
{
    std::unique_ptr<AmdMainGPUBinaryBase>(genAmdBinWithMetadata(metadata));
}

// checking parsing of AMD GPU metadata
static void testAmdGPUMetadataGen()
{
    const std::string testName = "testAmdGPUMetadataGen";
    assertCLRXException(testName, "aaaa", "1: This is not KernelDesc line",
                        tryGenAmdBinWithMetadata, "aaaa");
    // no fail
    tryGenAmdBinWithMetadata(R"blaB(;ARGSTART:__OpenCL_bitonicSort_kernel
;version:3:1:111
;device:pitcairn)blaB");
    assertCLRXException(testName, "Unex end of data", "10: Unexpected end of data",
        tryGenAmdBinWithMetadata, R"blaB(;ARGSTART:__OpenCL_bitonicSort_kernel
;version:3:1:111
;device:pitcairn
;uniqueid:1024
;memory:uavprivate:0
;memory:hwlocal:0
;memory:hwregion:0
;pointer:theArray:u32:1:1:0:uav:12:4:RW:0:0
;value:stage:u32:1:)blaB");
    assertCLRXException(testName, "Unknown pointer type", "8: Unknown pointer type",
        tryGenAmdBinWithMetadata, R"blaB(;ARGSTART:__OpenCL_bitonicSort_kernel
;version:3:1:111
;device:pitcairn
;uniqueid:1024
;memory:uavprivate:0
;memory:hwlocal:0
;memory:hwregion:0
;pointer:theArray:u32:1:1:0:uv:12:4:RW:0:0
;value:stage:u32:1:)blaB");
}

struct BinLoadingFailCase
{
    const char* filename;
    size_t changeOffset;
    Array<cxbyte> change;
    const char* exception;
};

static const BinLoadingFailCase binLoadingTestCases[] =
{
    { CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_12.clo.1_0.reconf",
      /* change section header offset in main binary */
      0x21, { 0x4a }, "SectionHeaders offset out of range!" },
    { CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_12.clo.1_0.reconf",
      /* add non-null character in last symbol name (main binary) */
      0x484, { 0x2a }, "Unfinished symbol name!" },
    { CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_12.clo.1_0.reconf",
      /* change program header offset in main binary */
      0x1c, { 0xff, 0xbb, 0xdd }, "ProgramHeaders offset out of range!" },
    { CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_12.clo.1_0.reconf",
      /* change section shstrndx in main binary */
      0x32, { 0x7 }, "Shstrndx out of range!" },
    { CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_12.clo.1_0.reconf",
      /* change name offset of the symbol __OpenCL_global_0 in main binary */
      0x498, { 0x44, 0x42 }, "Symbol name index out of range!" },
    { CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_12.clo.1_0.reconf",
      /* change size of the symbol __OpenCL_global_0 in main binary */
      0x4a0, { 0x20, 0x22 }, "globalData value+size out of range" }
};

// checking failures on AMD GPU binary loading
static void testBinLoadingFailCase(cxuint testCaseId, const BinLoadingFailCase& testCase)
{
    Array<cxbyte> data = loadDataFromFile(testCase.filename);
    for (size_t i = 0; i < testCase.change.size(); i++)
        data[testCase.changeOffset+i] = testCase.change[i];
    bool failed = false;
    try
    {
        std::unique_ptr<AmdMainBinaryBase> base(createAmdBinaryFromCode(
                data.size(), data.data(), 0));
    }
    catch(const Exception& ex)
    {
        if (::strcmp(testCase.exception, ex.what())!=0)
        {
            std::ostringstream oss;
            oss << "Exception not match for #" << testCaseId <<
                    " file=" << testCase.filename <<
                    ": expectedException=" << testCase.exception  <<
                    ", resultException=" << ex.what();
            throw Exception(oss.str());
        }
        failed = true;
    }
    if (!failed)
    {
        std::ostringstream oss;
        oss << "Not failed for #" << testCaseId << " file=" << testCase.filename;
        throw Exception(oss.str());
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/alltypes.clo",
            "myKernel", sizeof(expectedKernelArgs1)/sizeof(AmdKernelArg),
            expectedKernelArgs1);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/alltypes_64.clo",
            "myKernel", sizeof(expectedKernelArgs1)/sizeof(AmdKernelArg),
            expectedKernelArgs1);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/alltypes_cpu64.clo",
            "myKernel", sizeof(expectedCPUKernelArgs1)/sizeof(AmdKernelArg),
            expectedCPUKernelArgs1);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/alltypes_cpu.clo",
            "myKernel", sizeof(expectedCPUKernelArgs1)/sizeof(AmdKernelArg),
            expectedCPUKernelArgs1);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/alltypes-15_7.clo",
            "myKernel", sizeof(expectedCL2KernelArgs1)/sizeof(AmdKernelArg),
            expectedCL2KernelArgs1);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/alltypes-15_11.clo",
            "myKernel", sizeof(expectedCL2NewKernelArgs1)/sizeof(AmdKernelArg),
            expectedCL2NewKernelArgs1);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/test3-15_7.clo",
            "Piper", sizeof(expectedCL2KernelArgs2)/sizeof(AmdKernelArg),
            expectedCL2KernelArgs2);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/test3-15_11.clo",
            "Piper", sizeof(expectedCL2KernelArgs2)/sizeof(AmdKernelArg),
            expectedCL2KernelArgs2);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/structkernel2.clo",
            "myKernel1", sizeof(expectedKernelArgs2)/sizeof(AmdKernelArg),
            expectedKernelArgs2);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/structkernel2_64.clo", "myKernel1",
            sizeof(expectedKernelArgs2)/sizeof(AmdKernelArg), expectedKernelArgs2);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/structkernel2_cpu.clo", "myKernel1",
            sizeof(expectedCPUKernelArgs2)/sizeof(AmdKernelArg), expectedCPUKernelArgs2);
    retVal |= callTest(testKernelArgs, CLRX_SOURCE_DIR
            "/tests/amdbin/amdbins/structkernel2_cpu64.clo", "myKernel1",
            sizeof(expectedCPUKernelArgs2)/sizeof(AmdKernelArg), expectedCPUKernelArgs2);
    retVal |= callTest(testAmdGPUMetadataGen);
    
    for (cxuint i = 0; i < sizeof(binLoadingTestCases)/sizeof(BinLoadingFailCase); i++)
    {
        try
        { testBinLoadingFailCase(i, binLoadingTestCases[i]); }
        catch(const Exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    }
    
    return retVal;
}
