## AMD Catalyst OpenCL 2.0 ABI description

This chapter describes how kernel gets its argument, how access to constant data. Because
Kernel setup is AMD HSA configuration, hence we recommend to refer to ROCm-ABI documentation
to get information about kernel setup and kernel arguments passing. Now an assembler have
all the AMD HSA configuration's pseudo-ops to do it.

In this chapter, size is given in dwords. Dword is 4-byte value.

### Passing options

CLRX assembler give ability to set what feature will be used by kernel in configuration.
Following feature can be enabled:

* usesetup - use sizes information. Add kernel setup and sizes buffer
to user data registers.
* useargs - kernel uses arguments. Add kernel arguments to user data registers.
* useenqueue - enable enqueue mechanism support
* usegeneric - enable generic pointers support

The number of user data registers depends on set of an enabled features. Following rules will
be applied:

* if no feature enabled only 4 user data registers will be used.
* if useargs enabled, then 6 user data registers will be used. 4-5 user data are
argument's pointer.
* if usesetup enabled, then 8 user data registers will be used. 4-5 user data are kernel
setup pointer. 6-7 user data regs are argument's pointer.
* if useenqueue enabled, then 10 user data registers will be used. 4-5 user data regs
are kernel setup pointer. 6-7 user data regs are argument's pointer.
* if usegeneric enabled, then 12 user data registers will be used. 4-5 user data regs
are kernel setup pointer. 8-9 user data regs are argument's pointer.
* for VEGA (GFX9) architecture, then 10 user data registers will be used. 4-5 user data regs
are kernel setup pointer. 6-7 user data regs are argument's pointer.

### Argument passing and kernel setup

First pointer that is present in user data registers is kernel setup pointer.
This pointer points to setup buffer that holds kernel execution setup. Following
dwords:

* 0 dword - general setup. Bit 16-31 - dimensions number
* 1 dword - enqueued local size ??. Bit 0-15 - local size X, bit 16-31 - local size Y
* 2 dword - enqueued local size. Bit 0-15 - local size Z
* 3-5 dword - global size for each dimension

Second pointer is argument's pointer. This pointer points to argument's buffer.
First argument are setup arguments.

* size_t global_offset_0 - 32-bit or 64-bit global offset for X
* size_t global_offset_1 - 32-bit or 64-bit global offset for Y
* size_t global_offset_2 - 32-bit or 64-bit global offset for Z
* void* printf_buffer - 32-bit or 64-bit printf buffer
* void* vqueue_pointer - 32-bit or 64-bit
* void* aqlwrap_pointer - 32-bit or 64-bit

Further arguments in that buffer are an user arguments defined for a kernel. Any pointer,
command queue, image, sampler, structure tooks 8 bytes (64-bit pointer) or
4 bytes (32-bit pointer) in 32-bit AMD OpenCL 2.0.
3 component vector tooks number of bytes  of 4 element vector.
Smaller types likes (char, short) tooks 1-3 bytes. An alignment depends on same type
or type of element (for vectors).

For 64-bit AMD OpenCL 2.0 all setup arguments and pointers are 64-bit.
For 32-bit AMD OpenCL 2.0 all setup arguments and pointers are 32-bit.

### Image arguments

An images are passed via pointers to argument's buffer. An image pointers points to
image resource and image informations. Image resources tooks 8 dwords. 8 dword hold
information about channel data type. Following table describes data channel type value's
and their counterpart from OpenCL:

 Value | OpenCL value          | Value | OpenCL value
-------|-----------------------|-------|-----------------------
 0     | CL_SNORM_INT8         | 8     | CL_SIGNED_INT8
 1     | CL_SNORM_INT16        | 9     | CL_SIGNED_INT16 
 2     | CL_UNORM_INT8         | 10    | CL_SIGNED_INT32
 3     | CL_UNORM_INT16        | 11    | CL_UNSIGNED_INT8
 4     | CL_UNORM_INT24        | 12    | CL_UNSIGNED_INT16
 5     | CL_UNORM_SHORT_555    | 13    | CL_UNSIGNED_INT32
 6     | CL_UNORM_SHORT_565    | 14    | CL_HALF_FLOAT
 7     | CL_UNORM_INT_101010   | 15    | CL_FLOAT

Before looking up table, value should be masked: (value&0xf).

Likewise, 9 dword holds channel order information. Following table describes values and
OpenCL counterparts:

 Value | OpenCL value | Value  | OpenCL value 
-------|--------------|--------|------------------
 0     | CL_A         |  10    | CL_ARGB
 1     | CL_R         |  11    | CL_ABGR
 2     | CL_Rx        |  12    | CL_sRGB
 3     | CL_RG        |  13    | CL_sRGBx
 4     | CL_RGx       |  14    | CL_sRGBA
 5     | CL_RA        |  15    | CL_sBGRA
 6     | CL_RGB       |  16    | CL_INTENSITY
 7     | CL_RGBx      |  17    | CL_LUMINANCE
 8     | CL_RGBA      |  18    | CL_DEPTH
 9     | CL_BGRA      |  19    | CL_DEPTH_STENCIL

Before looking up table, value should be masked: (value&0x1f).

### Sampler arguments

A samplers are passed via pointers. A sampler pointers points to sampler resource.

### Scratch buffer access

First four scalar registers holds scratch buffer descriptor. Refer to
[GCN Machine State](GcnState) to learn about vector and scalar initial registers.

### Flat access

By default, FLAT instructions read or write values from main memory.
Generic addressing (usegeneric) allow to access to LDS and scratch buffer by using
FLAT instructions. A following rules gives ability to correctly setting up that mechanism.
Registers S[6-7] holds special buffer that hold a LDS and scratch buffer base addresses for
FLAT instructions.
16 dword of that buffer holds 32-63 bits of LDS base address for FLAT instructions.
17 dword of that buffer holds 32-63 bits of scratch buffer base address for
FLAT instructions.
Register S10 holds base scratch buffer offset for FLAT_SCRATCH. Register S11 holds
size of scratch per thread (for FLAT_SCRATCH).
