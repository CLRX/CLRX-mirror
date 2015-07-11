
typedef ulong4 typ1;

#ifdef __GPU__
#pragma OPENCL EXTENSION cl_ext_atomic_counters_32: enable
#endif

kernel void myKernel(
    uchar v0, uchar2 v1, uchar3 v2, uchar4 v3, uchar8 v4, uchar16 v5,
    char v6, char2 v7, char3 v8, char4 v9, char8 v10, char16 v11,
    ushort v12, ushort2 v13, ushort3 v14, ushort4 v15, ushort8 v16, ushort16 v17,
    short v18, short2 v19, short3 v20, short4 v21, short8 v22, short16 v23,
    uint v24, uint2 v25, uint3 v26, uint4 v27, uint8 v28, uint16 v29,
    int v30, int2 v31, int3 v32, int4 v33, int8 v34, int16 v35,
    ulong v36, ulong2 v37, ulong3 v38, ulong4 v39, ulong8 v40, ulong16 v41,
    long v42, long2 v43, long3 v44, long4 v45, long8 v46, long16 v47,
    sampler_t v48, read_only image1d_t v49, write_only image1d_t v50,
    read_only image2d_t v51, write_only image2d_t v52,
    read_only image3d_t v53, write_only image3d_t v54,
    read_only image1d_array_t v55, write_only image1d_array_t v56,
    read_only image1d_buffer_t v57, write_only image1d_buffer_t v58,
    read_only image2d_array_t v59, write_only image2d_array_t v60,
    const global void* v61, global void* v62,
    const local void* v63, local void* v64,
    const constant void* v65 __attribute__((max_constant_size(3200))),
    constant void* v66 __attribute__((max_constant_size(3200))),
    typ1 v67, volatile global void* v68, global void* restrict v69
#ifdef __GPU__
    , counter32_t v70
#endif
    )
{ }