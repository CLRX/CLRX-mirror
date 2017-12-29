## GCN ISA MIMG instructions

These instructions allow to access to images. MIMG instructions
operates on the image resources and on the sampler resources.

List of fields for the MIMG encoding:

Bits  | Name     | Description
------|----------|------------------------------
8-11  | DMASK    | Enable mask for read/write data components
12    | UNORM    | Accepts unnormalized coordinates
13    | GLC      | Globally coherent
14    | DA       | Data array (required by array of 1D and 2D images)
15    | R128     | Image resource size = 128 bits (GCN 1.0/1.1/1.2)
15    | A16      | Address components are 16-bits (GCN 1.4)
16    | TFE      | Texture Fail Enable (for partially resident textures).
17    | LWE      | LOD Warning Enable (for partially resident textures).
18-24 | OPCODE   | Operation code
25    | SLC      | System level coherent
26-31 | ENCODING | Encoding type. Must be 0b111100
32-39 | VADDR    | Vector address operands
40-47 | VDATA    | Vector data registers
48-52 | SRSRC    | Scalar registers with buffer resource (SGPR# is 4*value)
53-57 | SSAMP    | Scalar registers with sampler resource (SGPR# is 4*value)
63    | D16      | Convert 32-bit data to 16-bit data (GCN 1.2/1.4)

Instruction syntax: INSTRUCTION VDATA, VADDR, SRSRC [MODIFIERS]  
Instruction syntax: INSTRUCTION VDATA, VADDR, SRSRC, SSAMP [MODIFIERS]

Modifiers can be supplied in any order. Modifiers list: SLC, GLC, TFE, LWE, DA, R128.
The TFE flag requires additional the VDATA register.

The MIMG instructions is executed in order. Any MIMG instruction increments VMCNT and
it decrements VMCNT after memory operation. Any memory-write operation increments EXPCNT,
and it decrements EXPCNT after reading data from VDATA.

### Instructions by opcode

List of the MIMG instructions by opcode (GCN 1.0/1.1):

 Opcode     |GCN 1.0|GCN 1.1| Mnemonic
------------|-------|-------|---------------------------
 0 (0x0)    |   ✓   |   ✓   | IMAGE_LOAD
 1 (0x1)    |   ✓   |   ✓   | IMAGE_LOAD_MIP
 2 (0x2)    |   ✓   |   ✓   | IMAGE_LOAD_PCK
 3 (0x3)    |   ✓   |   ✓   | IMAGE_LOAD_PCK_SGN
 4 (0x4)    |   ✓   |   ✓   | IMAGE_LOAD_MIP_PCK
 5 (0x5)    |   ✓   |   ✓   | IMAGE_LOAD_MIP_PCK_SGN
 8 (0x8)    |   ✓   |   ✓   | IMAGE_STORE
 9 (0x9)    |   ✓   |   ✓   | IMAGE_STORE_MIP
 10 (0xa)   |   ✓   |   ✓   | IMAGE_STORE_PCK
 11 (0xb)   |   ✓   |   ✓   | IMAGE_STORE_MIP_PCK
 14 (0xe)   |   ✓   |   ✓   | IMAGE_GET_RESINFO
 15 (0xf)   |   ✓   |   ✓   | IMAGE_ATOMIC_SWAP
 16 (0x10)  |   ✓   |   ✓   | IMAGE_ATOMIC_CMPSWAP
 17 (0x11)  |   ✓   |   ✓   | IMAGE_ATOMIC_ADD
 18 (0x12)  |   ✓   |   ✓   | IMAGE_ATOMIC_SUB
 19 (0x13)  |   ✓   |       | IMAGE_ATOMIC_RSUB
 20 (0x14)  |   ✓   |   ✓   | IMAGE_ATOMIC_SMIN
 21 (0x15)  |   ✓   |   ✓   | IMAGE_ATOMIC_UMIN
 22 (0x16)  |   ✓   |   ✓   | IMAGE_ATOMIC_SMAX
 23 (0x17)  |   ✓   |   ✓   | IMAGE_ATOMIC_UMAX
 24 (0x18)  |   ✓   |   ✓   | IMAGE_ATOMIC_AND
 25 (0x19)  |   ✓   |   ✓   | IMAGE_ATOMIC_OR
 26 (0x1a)  |   ✓   |   ✓   | IMAGE_ATOMIC_XOR
 27 (0x1b)  |   ✓   |   ✓   | IMAGE_ATOMIC_INC
 28 (0x1c)  |   ✓   |   ✓   | IMAGE_ATOMIC_DEC
 29 (0x1d)  |   ✓   |   ✓   | IMAGE_ATOMIC_FCMPSWAP
 30 (0x1e)  |   ✓   |   ✓   | IMAGE_ATOMIC_FMIN
 31 (0x1f)  |   ✓   |   ✓   | IMAGE_ATOMIC_FMAX
 32 (0x20)  |   ✓   |   ✓   | IMAGE_SAMPLE
 33 (0x21)  |   ✓   |   ✓   | IMAGE_SAMPLE_CL
 34 (0x22)  |   ✓   |   ✓   | IMAGE_SAMPLE_D
 35 (0x23)  |   ✓   |   ✓   | IMAGE_SAMPLE_D_CL
 36 (0x24)  |   ✓   |   ✓   | IMAGE_SAMPLE_L
 37 (0x25)  |   ✓   |   ✓   | IMAGE_SAMPLE_B
 38 (0x26)  |   ✓   |   ✓   | IMAGE_SAMPLE_B_CL
 39 (0x27)  |   ✓   |   ✓   | IMAGE_SAMPLE_LZ
 40 (0x28)  |   ✓   |   ✓   | IMAGE_SAMPLE_C
 41 (0x29)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_CL
 42 (0x2a)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_D
 43 (0x2b)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_D_CL
 44 (0x2c)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_L
 45 (0x2d)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_B
 46 (0x2e)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_B_CL
 47 (0x2f)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_LZ
 48 (0x30)  |   ✓   |   ✓   | IMAGE_SAMPLE_O
 49 (0x31)  |   ✓   |   ✓   | IMAGE_SAMPLE_CL_O
 50 (0x32)  |   ✓   |   ✓   | IMAGE_SAMPLE_D_O
 51 (0x33)  |   ✓   |   ✓   | IMAGE_SAMPLE_D_CL_O
 52 (0x34)  |   ✓   |   ✓   | IMAGE_SAMPLE_L_O
 53 (0x35)  |   ✓   |   ✓   | IMAGE_SAMPLE_B_O
 54 (0x36)  |   ✓   |   ✓   | IMAGE_SAMPLE_B_CL_O
 55 (0x37)  |   ✓   |   ✓   | IMAGE_SAMPLE_LZ_O
 56 (0x38)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_O
 57 (0x39)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_CL_O
 58 (0x3a)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_D_O
 59 (0x3b)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_D_CL_O
 60 (0x3c)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_L_O
 61 (0x3d)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_B_O
 62 (0x3e)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_B_CL_O
 63 (0x3f)  |   ✓   |   ✓   | IMAGE_SAMPLE_C_LZ_O
 64 (0x40)  |   ✓   |   ✓   | IMAGE_GATHER4
 65 (0x41)  |   ✓   |   ✓   | IMAGE_GATHER4_CL
 68 (0x44)  |   ✓   |   ✓   | IMAGE_GATHER4_L
 69 (0x45)  |   ✓   |   ✓   | IMAGE_GATHER4_B
 70 (0x46)  |   ✓   |   ✓   | IMAGE_GATHER4_B_CL
 71 (0x47)  |   ✓   |   ✓   | IMAGE_GATHER4_LZ
 72 (0x48)  |   ✓   |   ✓   | IMAGE_GATHER4_C
 73 (0x49)  |   ✓   |   ✓   | IMAGE_GATHER4_C_CL
 76 (0x4c)  |   ✓   |   ✓   | IMAGE_GATHER4_C_L
 77 (0x4d)  |   ✓   |   ✓   | IMAGE_GATHER4_C_B
 78 (0x4e)  |   ✓   |   ✓   | IMAGE_GATHER4_C_B_CL
 79 (0x4f)  |   ✓   |   ✓   | IMAGE_GATHER4_C_LZ
 80 (0x50)  |   ✓   |   ✓   | IMAGE_GATHER4_O
 81 (0x51)  |   ✓   |   ✓   | IMAGE_GATHER4_CL_O
 84 (0x54)  |   ✓   |   ✓   | IMAGE_GATHER4_L_O
 85 (0x55)  |   ✓   |   ✓   | IMAGE_GATHER4_B_O
 86 (0x56)  |   ✓   |   ✓   | IMAGE_GATHER4_B_CL_O
 87 (0x57)  |   ✓   |   ✓   | IMAGE_GATHER4_LZ_O
 88 (0x58)  |   ✓   |   ✓   | IMAGE_GATHER4_C_O
 89 (0x59)  |   ✓   |   ✓   | IMAGE_GATHER4_C_CL_O
 92 (0x5c)  |   ✓   |   ✓   | IMAGE_GATHER4_C_L_O
 93 (0x5d)  |   ✓   |   ✓   | IMAGE_GATHER4_C_B_O
 94 (0x5e)  |   ✓   |   ✓   | IMAGE_GATHER4_C_B_CL_O
 95 (0x5f)  |   ✓   |   ✓   | IMAGE_GATHER4_C_LZ_O
 96 (0x60)  |   ✓   |   ✓   | IMAGE_GET_LOD
 104 (0x68) |   ✓   |   ✓   | IMAGE_SAMPLE_CD
 105 (0x69) |   ✓   |   ✓   | IMAGE_SAMPLE_CD_CL
 106 (0x6a) |   ✓   |   ✓   | IMAGE_SAMPLE_C_CD
 107 (0x6b) |   ✓   |   ✓   | IMAGE_SAMPLE_C_CD_CL
 108 (0x6c) |   ✓   |   ✓   | IMAGE_SAMPLE_CD_O
 109 (0x6d) |   ✓   |   ✓   | IMAGE_SAMPLE_CD_CL_O
 110 (0x6e) |   ✓   |   ✓   | IMAGE_SAMPLE_C_CD_O
 111 (0x6f) |   ✓   |   ✓   | IMAGE_SAMPLE_C_CD_CL_O

List of the MIMG instructions by opcode (GCN 1.2/1.4):

 Opcode     | Mnemonic
------------|-----------------------
 0 (0x0)    | IMAGE_LOAD
 1 (0x1)    | IMAGE_LOAD_MIP
 2 (0x2)    | IMAGE_LOAD_PCK
 3 (0x3)    | IMAGE_LOAD_PCK_SGN
 4 (0x4)    | IMAGE_LOAD_MIP_PCK
 5 (0x5)    | IMAGE_LOAD_MIP_PCK_SGN
 8 (0x8)    | IMAGE_STORE
 9 (0x9)    | IMAGE_STORE_MIP
 10 (0xa)   | IMAGE_STORE_PCK
 11 (0xb)   | IMAGE_STORE_MIP_PCK
 14 (0xe)   | IMAGE_GET_RESINFO
 16 (0x10)  | IMAGE_ATOMIC_SWAP
 17 (0x11)  | IMAGE_ATOMIC_CMPSWAP
 18 (0x12)  | IMAGE_ATOMIC_ADD
 19 (0x13)  | IMAGE_ATOMIC_SUB
 20 (0x14)  | IMAGE_ATOMIC_SMIN
 21 (0x15)  | IMAGE_ATOMIC_UMIN
 22 (0x16)  | IMAGE_ATOMIC_SMAX
 23 (0x17)  | IMAGE_ATOMIC_UMAX
 24 (0x18)  | IMAGE_ATOMIC_AND
 25 (0x19)  | IMAGE_ATOMIC_OR
 26 (0x1a)  | IMAGE_ATOMIC_XOR
 27 (0x1b)  | IMAGE_ATOMIC_INC
 28 (0x1c)  | IMAGE_ATOMIC_DEC
 32 (0x20)  | IMAGE_SAMPLE
 33 (0x21)  | IMAGE_SAMPLE_CL
 34 (0x22)  | IMAGE_SAMPLE_D
 35 (0x23)  | IMAGE_SAMPLE_D_CL
 36 (0x24)  | IMAGE_SAMPLE_L
 37 (0x25)  | IMAGE_SAMPLE_B
 38 (0x26)  | IMAGE_SAMPLE_B_CL
 39 (0x27)  | IMAGE_SAMPLE_LZ
 40 (0x28)  | IMAGE_SAMPLE_C
 41 (0x29)  | IMAGE_SAMPLE_C_CL
 42 (0x2a)  | IMAGE_SAMPLE_C_D
 43 (0x2b)  | IMAGE_SAMPLE_C_D_CL
 44 (0x2c)  | IMAGE_SAMPLE_C_L
 45 (0x2d)  | IMAGE_SAMPLE_C_B
 46 (0x2e)  | IMAGE_SAMPLE_C_B_CL
 47 (0x2f)  | IMAGE_SAMPLE_C_LZ
 48 (0x30)  | IMAGE_SAMPLE_O
 49 (0x31)  | IMAGE_SAMPLE_CL_O
 50 (0x32)  | IMAGE_SAMPLE_D_O
 51 (0x33)  | IMAGE_SAMPLE_D_CL_O
 52 (0x34)  | IMAGE_SAMPLE_L_O
 53 (0x35)  | IMAGE_SAMPLE_B_O
 54 (0x36)  | IMAGE_SAMPLE_B_CL_O
 55 (0x37)  | IMAGE_SAMPLE_LZ_O
 56 (0x38)  | IMAGE_SAMPLE_C_O
 57 (0x39)  | IMAGE_SAMPLE_C_CL_O
 58 (0x3a)  | IMAGE_SAMPLE_C_D_O
 59 (0x3b)  | IMAGE_SAMPLE_C_D_CL_O
 60 (0x3c)  | IMAGE_SAMPLE_C_L_O
 61 (0x3d)  | IMAGE_SAMPLE_C_B_O
 62 (0x3e)  | IMAGE_SAMPLE_C_B_CL_O
 63 (0x3f)  | IMAGE_SAMPLE_C_LZ_O
 64 (0x40)  | IMAGE_GATHER4
 65 (0x41)  | IMAGE_GATHER4_CL
 68 (0x44)  | IMAGE_GATHER4_L
 69 (0x45)  | IMAGE_GATHER4_B
 70 (0x46)  | IMAGE_GATHER4_B_CL
 71 (0x47)  | IMAGE_GATHER4_LZ
 72 (0x48)  | IMAGE_GATHER4_C
 73 (0x49)  | IMAGE_GATHER4_C_CL
 76 (0x4c)  | IMAGE_GATHER4_C_L
 77 (0x4d)  | IMAGE_GATHER4_C_B
 78 (0x4e)  | IMAGE_GATHER4_C_B_CL
 79 (0x4f)  | IMAGE_GATHER4_C_LZ
 80 (0x50)  | IMAGE_GATHER4_O
 81 (0x51)  | IMAGE_GATHER4_CL_O
 84 (0x54)  | IMAGE_GATHER4_L_O
 85 (0x55)  | IMAGE_GATHER4_B_O
 86 (0x56)  | IMAGE_GATHER4_B_CL_O
 87 (0x57)  | IMAGE_GATHER4_LZ_O
 88 (0x58)  | IMAGE_GATHER4_C_O
 89 (0x59)  | IMAGE_GATHER4_C_CL_O
 92 (0x5c)  | IMAGE_GATHER4_C_L_O
 93 (0x5d)  | IMAGE_GATHER4_C_B_O
 94 (0x5e)  | IMAGE_GATHER4_C_B_CL_O
 95 (0x5f)  | IMAGE_GATHER4_C_LZ_O
 96 (0x60)  | IMAGE_GET_LOD
 104 (0x68) | IMAGE_SAMPLE_CD
 105 (0x69) | IMAGE_SAMPLE_CD_CL
 106 (0x6a) | IMAGE_SAMPLE_C_CD
 107 (0x6b) | IMAGE_SAMPLE_C_CD_CL
 108 (0x6c) | IMAGE_SAMPLE_CD_O
 109 (0x6d) | IMAGE_SAMPLE_CD_CL_O
 110 (0x6e) | IMAGE_SAMPLE_C_CD_O
 111 (0x6f) | IMAGE_SAMPLE_C_CD_CL_O

### Suffix instruction meaning

Following table describes suffixes for IMAGE_SAMPLE_\* and IMAGE_GATHER4_\* instructions:

Suffix | Meaning | Extra addresses | Description
-------|---------|-----------|---------------------
_L     | LOD     | 1: lod    | LOD is used instead of TA computed LOD.
_B     | LOD BIAS | lod bias | Add this BIAS to the LOD TA computes.
_CL    | LOD CLAMP | 1: clamp | Clamp the LOD to be no larger than this value.
_D     | Derivative | 2,4 or 6: dwords | Send dx/dv, dx/dy, etc. slopes to TA for it to used in LOD computation.
_CD    | Coarse Derivative | 2,4 or 6: dwords | Look at _D
_LZ | Level 0 | - | Force use of MIP level 0.
_C  | PCF     | 1: z-comp | Percentage closer filtering.
_O  | Offset  | 1: offsets | Send X, Y, Z integer offsets (packed into 1 Dword) to offset XYZ address.

* _L - choose LOD from VADDR from last register, after other components.
* _C - compare (by using z-compare function from sampler) fetched data from image
(first component of pixel) with Z-COMP component from VADDR. Working only with
floating point images.
* _O - apply offset to image's address (add X, Y and Z offset to X, Y and Z coordinates).

### Instruction set

NOTE: While discovering and testing, many instructions behaved in unexpected manner on
GCN 1.0 device (mainly non-standard IMAGE_SAMPLE_\* and IMAGE_GATHER4_\* instructions).
Hence, no operation's listing to these instructions and only brief descriptions. However,
IMAGE_LOAD_\*, IMAGE_STORE_\* and IMAGE_ATOMIC_\* has been explained quite good.

Alphabetically sorted instruction list:

#### IMAGE_ATOMIC_ADD

Opcode: 17 (0x11) for GCN 1.0/1.1; 18 (0x12) for GCN 1.2/1.4  
Syntax: IMAGE_ATOMIC_ADD VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Add VDATA dwords or 64-bit words (if VDATA size is greater than 32-bit)
to values of image SRSRC at address VADDR, and store result into image.
If GLC is set then return old values
from image, otherwise keep VDATA value. Only 32-bit, 64-bit and 128-bit data formats
are supported. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((UINT*)VM)[0] = VDATA[0]+((UINT*)VM)[0]
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((UINT64*)VM)[i] = VDATA[i]+((UINT64*)VM)[i]
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_AND

Opcode: 24 (0x18)  
Syntax: IMAGE_ATOMIC_AND VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Do bitwise AND on VDATA dwords and values of image SRSRC at address VADDR,
and store result into image. If GLC is set then return old values from image,
otherwise keep VDATA value. Only 32-bit, 64-bit and 128-bit data formats are supported.
Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
for (BYTE i = 0; i < (sizeof(VDATA)>>2); i++)
    ((UINT*)VM)[i] = VDATA[i] & ((UINT*)VM)[i]
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_CMPSWAP

Opcode: 16 (0x10) for GCN 1.0/1.1; 17 (0x11) for GCN 1.2/1.4  
Syntax: IMAGE_ATOMIC_CMPSWAP VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Store first half of VDATA into image SRSRC to pixel at address VADDR if
second half of VDATA is equal old value from image's pixel, otherwise keep
old value from that pixel. Data type determined by image data format and half number of
enabled bits in DMASK. Four dword data types are not supported.
If GLC is set then return old values from image, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE VDL = VDATA[0:(VDATA_SIZE>>1)-1]
PIXELTYPE VDH = VDATA[VDATA_SIZE>>1:VDATA_SIZE-1]
PIXELTYPE P = *VM; *VM = *VM==(VDH) ? VDL : *VM // part of atomic
VDATA[0:(VDATA_SIZE>>1)-1] = (GLC) ? P : VDL // last part of atomic
```

#### IMAGE_ATOMIC_DEC

Opcode: 28 (0x1c)  
Syntax: IMAGE_ATOMIC_DEC VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Compare dwords or 64-bit words (if VDATA size is greater than 32-bit)
of image SRSRC and if less or equal than VDATA words and this value is not zero,
then decrement value from image, otherwise store VDATA into image.
If GLC is set then return old values from image, otherwise keep VDATA value.
Only 32-bit, 64-bit and 128-bit data formats are supported. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((UINT*)VM)[0] = (*(UINT*)VM <= VDATA[0] && *(UINT*)VM!=0) ? \
        *(UINT*)VM-1 : VDATA
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((UINT64*)VM)[i] = (((UINT64*)VM)[i] <= VDATA[i] && ((UINT64*)VM)[i]!=0) ? \
                 ((UINT64*)VM)[i]-1 : VDATA[i]
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_FCMPSWAP

Opcode: 29 (0x1d) for GCN 1.0/1.1  
Syntax: IMAGE_ATOMIC_FCMPSWAP VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Store first single/double floating point value of VDATA into image
SRSRC to pixel at address VADDR if second single/double FP of VDATA is equal old value
from image's pixel, otherwise keep old value from that pixel.
Data type determined by image data format and half number of
enabled bits in DMASK. Four dword data types are not supported.
If GLC is set then return old values from image, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
if (sizeof(PIXELTYPE)==8)
{
    DOUBLE P = *VM; 
    *VM = *VM==ASDOUBLE(VDATA[1]) ? VDATA[0] : *VM // part of atomic
    VDATA[0] = (GLC) ? P : VDATA[0] // last part of atomic
}
else
{
    FLOAT P = *VM; 
    *VM = *VM==ASFLOAT(VDATA[1]) ? VDATA[0] : *VM // part of atomic
    VDATA[0] = (GLC) ? P : VDATA[0] // last part of atomic
}
```

#### IMAGE_ATOMIC_FMAX

Opcode: 31 (0x1f) for GCN 1.0/1.1  
Syntax: IMAGE_ATOMIC_FMAX VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Choose greatest single or double (if VDATA size is greater than 32-bit)
floating point values between VDATA values and values of image SRSRC at address VADDR,
and store result into image. If GLC is set then return old values from image,
otherwise keep VDATA value. Only 32-bit, 64-bit and 128-bit data formats are supported.
Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((FLOAT*)VM)[0] = MAX(ASFLOAT(VDATA[0]),((FLOAT*)VM)[0])
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((DOUBLE*)VM)[i] = MAX(ASDOUBLE(VDATA[i]),((DOUBLE*)VM)[i])
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_FMIN

Opcode: 30 (0x1e) for GCN 1.0/1.1  
Syntax: IMAGE_ATOMIC_FMIN VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Choose smallest single or double (if VDATA size is greater than 32-bit)
floating point values between VDATA values and values of image SRSRC at address VADDR,
and store result into image. If GLC is set then return old values from image,
otherwise keep VDATA value. Only 32-bit, 64-bit and 128-bit data formats are supported.
Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((FLOAT*)VM)[0] = MIN(ASFLOAT(VDATA[0]),((FLOAT*)VM)[0])
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((DOUBLE*)VM)[i] = MIN(ASDOUBLE(VDATA[i]),((DOUBLE*)VM)[i])
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_INC

Opcode: 27 (0x1b)  
Syntax: IMAGE_ATOMIC_INC VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Compare dwords or 64-bit words (if VDATA size is greater than 32-bit) of
image SRSRC and if less than VDATA words, then increment value from image,
otherwise store zero into image. If GLC is set then return old values from image,
otherwise keep VDATA value. Only 32-bit, 64-bit and 128-bit data formats are supported.
Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((UINT*)VM)[0] = (*(UINT*)VM < VDATA[0]) ? *(UINT*)VM-1 : 0
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((UINT64*)VM)[i] = (((UINT64*)VM)[i] < VDATA[i]) ? ((UINT64*)VM)[i]+1 : 0
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_OR

Opcode: 25 (0x19)  
Syntax: IMAGE_ATOMIC_OR VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Do bitwise OR on VDATA dwords and values of image SRSRC at address VADDR,
and store result into image. If GLC is set then return old values from image,
otherwise keep VDATA value. Only 32-bit, 64-bit and 128-bit data formats are supported.
Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
for (BYTE i = 0; i < (sizeof(VDATA)>>2); i++)
    ((UINT*)VM)[i] = VDATA[i] | ((UINT*)VM)[i]
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_RSUB

Opcode: 19 (0x13) for GCN 1.0/1.1  
Syntax: IMAGE_ATOMIC_RSUB VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Subtract values of image SRSRC at address VADDR from VDATA dwords or 64-bit
words (if VDATA size is greater than 32-bit), and store result into image.
If GLC is set then return old values from image, otherwise keep VDATA value.
Only 32-bit, 64-bit and 128-bit data formats are supported. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((UINT*)VM)[0] = ((UINT*)VM)[0]-VDATA[0]
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((UINT64*)VM)[i] = ((UINT64*)VM)[i]-VDATA[i]
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_SMAX

Opcode: 21 (0x15)  
Syntax: IMAGE_ATOMIC_SMAX VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Choose greatest signed dwords or 64-bit words
(if VDATA size is greater than 32-bit) between VDATA values and
values of image SRSRC at address VADDR, and store result into image.
If GLC is set then return old values from image, otherwise keep VDATA value.
Only 32-bit, 64-bit and 128-bit data formats are supported. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((INT*)VM)[0] = MAX((INT)VDATA[0],((INT*)VM)[0])
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((INT64*)VM)[i] = MAX(INT64)VDATA[i],((INT64*)VM)[i])
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_SMIN

Opcode: 20 (0x14)  
Syntax: IMAGE_ATOMIC_SMIN VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Choose smallest signed dwords or 64-bit words
(if VDATA size is greater than 32-bit) between VDATA values and
values of image SRSRC at address VADDR, and store result into image.
If GLC is set then return old values from image, otherwise keep VDATA value.
Only 32-bit, 64-bit and 128-bit data formats are supported. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((INT*)VM)[0] = MIN((INT)VDATA[0],((INT*)VM)[0])
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((INT64*)VM)[i] = MIN(INT64)VDATA[i],((INT64*)VM)[i])
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_SUB

Opcode: 18 (0x12) for GCN 1.0/1.1; 19 (0x13) for GCN 1.2/1.4  
Syntax: IMAGE_ATOMIC_SUB VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Subtract VDATA dwords or 64-bit words (if VDATA size is greater than 32-bit)
from values of image SRSRC at address VADDR, and store result into image.
If GLC is set then return old values from image, otherwise keep VDATA value.
Only 32-bit, 64-bit and 128-bit data formats
are supported. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((UINT*)VM)[0] = VDATA[0]-((UINT*)VM)[0]
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((UINT64*)VM)[i] = VDATA[i]-((UINT64*)VM)[i]
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_SWAP

Opcode: 15 (0xf) for GCN 1.0/1.1; 16 (0x10) for GCN 1.2/1.4  
Syntax: IMAGE_ATOMIC_SWAP VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Store VDATA into image SRSRC to pixel at address VADDR. If GLC is set then
return old values from image, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM; *VM = VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_UMAX

Opcode: 23 (0x17)  
Syntax: IMAGE_ATOMIC_UMAX VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Choose greatest unsigned dwords or 64-bit words
(if VDATA size is greater than 32-bit) between VDATA values and
values of image SRSRC at address VADDR, and store result into image.
If GLC is set then return old values from image, otherwise keep VDATA value.
Only 32-bit, 64-bit and 128-bit data formats are supported. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((UINT*)VM)[0] = MAX(VDATA[0],((UINT*)VM)[0])
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((UINT64*)VM)[i] = MAX(VDATA[i],((UINT64*)VM)[i])
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_UMIN

Opcode: 22 (0x16)  
Syntax: IMAGE_ATOMIC_UMIN VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Choose smallest unsigned dwords or 64-bit words
(if VDATA size is greater than 32-bit) between VDATA values and
values of image SRSRC at address VADDR, and store result into image.
If GLC is set then return old values from image, otherwise keep VDATA value.
Only 32-bit, 64-bit and 128-bit data formats are supported. Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
if (sizeof(PIXELTYPE)==4)
    ((UINT*)VM)[0] = MIN(VDATA[0],((UINT*)VM)[0])
else    // add 64-bit dwords
    for (BYTE i = 0; i < (sizeof(VDATA)>>3); i++)
        ((UINT64*)VM)[i] = MIN(VDATA[i],((UINT64*)VM)[i])
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_ATOMIC_XOR

Opcode: 26 (0x1a)  
Syntax: IMAGE_ATOMIC_XOR VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Do bitwise XOR on VDATA dwords and values of image SRSRC at address VADDR,
and store result into image. If GLC is set then return old values from image,
otherwise keep VDATA value. Only 32-bit, 64-bit and 128-bit data formats are supported.
Operation is atomic.  
Operation:  
```
PIXELTYPE* VM = VMIMG(SRSRC, VADDR)
PIXELTYPE P = *VM;
for (BYTE i = 0; i < (sizeof(VDATA)>>2); i++)
    ((UINT*)VM)[i] = VDATA[i] ^ ((UINT*)VM)[i]
VDATA = (GLC) ? P : VDATA // atomic
```

#### IMAGE_GATHER4

Opcode: 64 (0x40)  
Syntax: IMAGE_GATHER4 VDATA(4), VADDR(1:4), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Choosen component is first one bit in DMASK. The left top pixel are
choosen from FLOOR(X-0.5) for X coordinate, FLOOR(Y-0.5) for Y coordinate.
Following VDATA registers stores:

* VDATA[0] - bottom left pixel's component (X,Y+1)
* VDATA[1] - bottom right pixel's component (X+1,Y+1)
* VDATA[2] - top right pixel's component (X+1,Y)
* VDATA[3] - top left  pixel's component (X,Y)

Operation:  
```
INT X = FLOOR(ASFLOAT(VADDR[0])-0.5)
INT Y = FLOOR(ASFLOAT(VADDR[1])-0.5)
COMPTYPE* VMLT = VMIMG_SAMPLE(SRSRC, { X, Y, VADDR[2] }, SSAMP)
COMPTYPE* VMRT = VMIMG_SAMPLE(SRSRC, { X+1 Y, VADDR[2] }, SSAMP)
COMPTYPE* VMLB = VMIMG_SAMPLE(SRSRC, { X, Y+1, VADDR[2] }, SSAMP)
COMPTYPE* VMRB = VMIMG_SAMPLE(SRSRC, { X+1, Y+1, VADDR[2] }, SSAMP)
BYTE COMP = (DMASK&1) ? 0 : (DMASK&2) ? 1 : (DMASK&4) ? 2 : 3;
VDATA[0] = CONVERT_FROM_IMAGE(SRSRC, VMLB)[COMP]
VDATA[1] = CONVERT_FROM_IMAGE(SRSRC, VMRB)[COMP]
VDATA[2] = CONVERT_FROM_IMAGE(SRSRC, VMRT)[COMP]
VDATA[3] = CONVERT_FROM_IMAGE(SRSRC, VMLT)[COMP]
```

#### IMAGE_GATHER4_B

Opcode: 69 (0x45)  
Syntax: IMAGE_GATHER4_B VDATA(4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.
The first address register holds the LOD bias value.

#### IMAGE_GATHER4_B_O

Opcode: 85 (0x55)  
Syntax: IMAGE_GATHER4_B_O VDATA(4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.
The first address register holds the offset for X,Y,Z.
Next address register holds the LOD bias value.

#### IMAGE_GATHER4_B_CL

Opcode: 70 (0x46)  
Syntax: IMAGE_GATHER4_B_CL VDATA(4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The first address
register holds the LOD bias value. The last address register holds the clamp value.

#### IMAGE_GATHER4_B_CL_O

Opcode: 86 (0x56)  
Syntax: IMAGE_GATHER4_B_CL_O VDATA(4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.
The first address register holds the offset for X,Y,Z. Next address
register holds the LOD bias value. The last address register holds the clamp value.

#### IMAGE_GATHER4_C

Opcode: 72 (0x48)  
Syntax: IMAGE_GATHER4_C VDATA(4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.
The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value.

#### IMAGE_GATHER4_C_O

Opcode: 88 (0x58)  
Syntax: IMAGE_GATHER4_C_O VDATA(4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.
The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds
the Z-compare value.

#### IMAGE_GATHER4_C_B

Opcode: 77 (0x4d)  
Syntax: IMAGE_GATHER4_C_B VDATA(4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction performs
Z-compare operation choosen in SSAMP sampler. The first address register holds
the LOD bias value. Next address register holds the Z-compare value. 

#### IMAGE_GATHER4_C_B_O

Opcode: 93 (0x5d)  
Syntax: IMAGE_GATHER4_C_B_O VDATA(4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction performs
Z-compare operation choosen in SSAMP sampler. The first address register holds
the offset for X,Y,Z. Next address register holds the LOD bias value.
Next address register holds the Z-compare value. 

#### IMAGE_GATHER4_C_B_CL

Opcode: 78 (0x4e)  
Syntax: IMAGE_GATHER4_C_B_CL VDATA(4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction performs
Z-compare operation choosen in SSAMP sampler. The first address register holds
the LOD bias value. Next address register holds the Z-compare value. The last address
register holds the clamp value.

#### IMAGE_GATHER4_C_B_CL_O

Opcode: 94 (0x5e)  
Syntax: IMAGE_GATHER4_C_B_CL_O VDATA(4), VADDR(5:8), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction performs
Z-compare operation choosen in SSAMP sampler. The first address register holds
the offset for X,Y,Z. Next address register holds the LOD bias value.
Next address register holds the Z-compare value. The last address register holds
the clamp value.

#### IMAGE_GATHER4_C_CL

Opcode: 73 (0x49)  
Syntax: IMAGE_GATHER4_C_CL VDATA(4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction
performs Z-compare operation choosen in SSAMP sampler. The first address register
holds the Z-compare value. The last address register holds the clamp value.

#### IMAGE_GATHER4_C_CL_O

Opcode: 89 (0x59)  
Syntax: IMAGE_GATHER4_C_CL_O VDATA(4), VADDR(5:8), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction
performs Z-compare operation choosen in SSAMP sampler. The first address register holds
the offset for X,Y,Z. Next address register holds the Z-compare value.
The last address register holds the clamp value.

#### IMAGE_GATHER4_C_L

Opcode: 76 (0x4c)  
Syntax: IMAGE_GATHER4_C_L VDATA(4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction
performs Z-compare operation choosen in SSAMP sampler. The first address register
holds the Z-compare value. The last address register holds the LOD value.

#### IMAGE_GATHER4_C_L_O

Opcode: 92 (0x5c)  
Syntax: IMAGE_GATHER4_C_L_O VDATA(4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction
performs Z-compare operation choosen in SSAMP sampler. The first address register holds
the offset for X,Y,Z. Next address register holds the Z-compare value.
The last address register holds the LOD value.

#### IMAGE_GATHER4_C_LZ

Opcode: 79 (0x4f)  
Syntax: IMAGE_GATHER4_C_LZ VDATA(4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction
performs Z-compare operation choosen in SSAMP sampler. The first address register
holds the Z-compare value. Force use of mipmap level 0 (???).

#### IMAGE_GATHER4_C_LZ_O

Opcode: 95 (0x5f)  
Syntax: IMAGE_GATHER4_C_LZ_O VDATA(4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The instruction
performs Z-compare operation choosen in SSAMP sampler. The first address register holds
the offset for X,Y,Z. Next address register holds the Z-compare value.
Force use of mipmap level 0 (???).

#### IMAGE_GATHER4_CL

Opcode: 65 (0x41)  
Syntax: IMAGE_GATHER4_CL VDATA(4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.
The last address register holds the clamp value.

#### IMAGE_GATHER4_CL_O

Opcode: 81 (0x51)  
Syntax: IMAGE_GATHER4_CL_O VDATA(4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The first address register
holds the offset for X,Y,Z. The last address register holds the clamp value.

#### IMAGE_GATHER4_L

Opcode: 68 (0x44)  
Syntax: IMAGE_GATHER4_L VDATA(4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.
The last address register holds the LOD value.

#### IMAGE_GATHER4_L_O

Opcode: 84 (0x54)  
Syntax: IMAGE_GATHER4_L_O VDATA(4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The first address register
holds the offset for X,Y,Z. The last address register holds the LOD value.

#### IMAGE_GATHER4_LZ

Opcode: 71 (0x47)  
Syntax: IMAGE_GATHER4_LZ VDATA(4), VADDR(1:4), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.
Force use of mipmap level 0 (???).

#### IMAGE_GATHER4_LZ_O

Opcode: 87 (0x57)  
Syntax: IMAGE_GATHER4_LZ_O VDATA(4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm. The first address register
holds the offset for X,Y,Z. Force use of mipmap level 0 (???).

#### IMAGE_GATHER4_O

Opcode: 80 (0x50)  
Syntax: IMAGE_GATHER4_O VDATA(4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get component's value from 4 neighboring pixels, starting from coordinates
from VADDR. Refer to IMAGE_GATHER4 to learn about algorithm.  The first address register
holds the offset for X,Y,Z. 

#### IMAGE_LOAD

Opcode: 0 (0x0)  
Syntax: IMAGE_LOAD VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Load data from image SRSRC from pixel at address VADDR, and store their data
to VDATA. Loaded data are converted to format given in image resource.  
Operation:  
```
PIXELTYPE* V = VMIMG(SRSRC, VADDR)
VDATA = GET_SAMPLES(CONVERT_FROM_IMAGE(SRSRC, V), DMASK)
```

#### IMAGE_LOAD_MIP

Opcode: 1 (0x1)  
Syntax: IMAGE_LOAD_MIP VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Load data from image SRSRC from pixel at address VADDR including MIP index,
and store their data to VDATA. Loaded data are converted to format given in
image resource.  
Operation:  
```
PIXELTYPE* V = VMIMG_MIP(SRSRC, VADDR)
VDATA = GET_SAMPLES(CONVERT_FROM_IMAGE(SRSRC, V), DMASK)
```

#### IMAGE_LOAD_MIP_PCK

Opcode: 4 (0x4)  
Syntax: IMAGE_LOAD_MIP_PCK VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Load data from image SRSRC from pixel at address VADDR including MIP index,
and store their data to VDATA. Loaded data are raw without any conversion. DMASK controls
what dwords will be stored to VDATA.  
Operation:  
```
PIXELTYPE* V = VMIMG_MIP(SRSRC, VADDR)
VDATA = GET_SAMPLES(V, DMASK)
```

#### IMAGE_LOAD_MIP_PCK_SGN

Opcode: 5 (0x5)  
Syntax: IMAGE_LOAD_MIP_PCK_SGN VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Load data from image SRSRC from pixel at address VADDR including MIP index,
and store their data to VDATA. Loaded data are raw without any conversion,
but sign extended. DMASK controls what dwords will be stored to VDATA.  
Operation:  
```
PIXELTYPE* V = VMIMG_MIP(SRSRC, VADDR)
VDATA = GET_SAMPLES(V, DMASK)
BYTE COMPBITS = COMPBITS(SRSRC)
for (BYTE i = 0; i < BIT_CNT(DMASK); i++)
    VDATA[i] = SEXT(VDATA[i], COMPBITS)
```

#### IMAGE_LOAD_PCK

Opcode: 2 (0x2)  
Syntax: IMAGE_LOAD_PCK VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Load data from image SRSRC from pixel at address VADDR, and store their data
to VDATA. Loaded data are raw without any conversion. DMASK controls what dwords
will be stored to VDATA.  
Operation:  
```
PIXELTYPE* V = VMIMG(SRSRC, VADDR)
VDATA = GET_SAMPLES(V, DMASK)
```

#### IMAGE_LOAD_PCK_SGN

Opcode: 3 (0x3)  
Syntax: IMAGE_LOAD_PCK_SGN VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Load data from image SRSRC from pixel at address VADDR, and store their data
to VDATA. Loaded data are raw without any conversion, but sign extended.
DMASK controls what dwords will be stored to VDATA.  
Operation:  
```
PIXELTYPE* V = VMIMG(SRSRC, VADDR)
VDATA = GET_SAMPLES(V, DMASK)
BYTE COMPBITS = COMPBITS(SRSRC)
for (BYTE i = 0; i < BIT_CNT(DMASK); i++)
    VDATA[i] = SEXT(VDATA[i], COMPBITS)
```

#### IMAGE_SAMPLE

Opcode: 32 (0x20)  
Syntax: IMAGE_SAMPLE VDATA(1:4), VADDR(1:4), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler.

#### IMAGE_SAMPLE_B

Opcode: 37 (0x25)  
Syntax: IMAGE_SAMPLE_B VDATA(1:4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the LOD bias value.

#### IMAGE_SAMPLE_B_CL

Opcode: 38 (0x26)  
Syntax: IMAGE_SAMPLE_B_CL VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the LOD bias value.
The last address register holds the clamp value.

#### IMAGE_SAMPLE_B_CL_O

Opcode: 54 (0x36)  
Syntax: IMAGE_SAMPLE_B_CL_O VDATA(1:4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.
Next address register holds the LOD bias value.
The last address register holds the clamp value.

#### IMAGE_SAMPLE_B_O

Opcode: 53 (0x35)  
Syntax: IMAGE_SAMPLE_B_O VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.
Next address register holds the LOD bias value.
The last address register holds the clamp value.

#### IMAGE_SAMPLE_C

Opcode: 40 (0x28)  
Syntax: IMAGE_SAMPLE_C VDATA(1:4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value.

#### IMAGE_SAMPLE_C_B

Opcode: 45 (0x2d)  
Syntax: IMAGE_SAMPLE_C_B VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first register holds the LOD bias value. Next address register holds the
Z-compare value.

#### IMAGE_SAMPLE_C_B_CL

Opcode: 46 (0x2e)  
Syntax: IMAGE_SAMPLE_C_B_CL VDATA(1:4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first register holds the LOD bias value. Next address register holds the
Z-compare value. The last address register holds the clamp value.

#### IMAGE_SAMPLE_C_B_CL_O

Opcode: 62 (0x62)  
Syntax: IMAGE_SAMPLE_C_B_CL_O VDATA(1:4), VADDR(5:8), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds
the LOD bias value. Next address register holds the Z-compare value.
The last address register holds the clamp value.

#### IMAGE_SAMPLE_C_B_O

Opcode: 61 (0x3d)  
Syntax: IMAGE_SAMPLE_C_B_O VDATA(1:4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds
the LOD bias value. Next address register holds the Z-compare value. 

#### IMAGE_SAMPLE_C_CD

Opcode: 106 (0x6a)  
Syntax: IMAGE_SAMPLE_C_CD VDATA(1:4), VADDR(4:11), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value. Next 2-6 address registers
holds user derivatives (coarse derivatives).

#### IMAGE_SAMPLE_C_CD_CL

Opcode: 107 (0x6b)  
Syntax: IMAGE_SAMPLE_C_CD_CL VDATA(1:4), VADDR(5:12), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value. Next 2-6 address registers
holds user derivatives (coarse derivatives). The last address register holds
the clamp value.

#### IMAGE_SAMPLE_C_CL

Opcode: 41 (0x29)  
Syntax: IMAGE_SAMPLE_C_CL VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value. The last address register
holds the clamp value.

#### IMAGE_SAMPLE_C_CL_O

Opcode: 57 (0x39)  
Syntax: IMAGE_SAMPLE_C_CL_O VDATA(1:4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds
the Z-compare value. The last address register holds the clamp value.

#### IMAGE_SAMPLE_C_D

Opcode: 42 (0x2a)  
Syntax: IMAGE_SAMPLE_C_D VDATA(1:4), VADDR(4:11), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value. Next 2-6 address registers
holds user derivatives.

#### IMAGE_SAMPLE_C_D_CL

Opcode: 43 (0x2b)  
Syntax: IMAGE_SAMPLE_C_D_CL VDATA(1:4), VADDR(5:12), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value. Next 2-6 address registers
holds user derivatives. The last address register holds the clamp value.

#### IMAGE_SAMPLE_C_CD_CL_O

Opcode: 111 (0x6f)  
Syntax: IMAGE_SAMPLE_C_CD_CL_O VDATA(1:4), VADDR(6:13), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds
the Z-compare value. Next 2-6 address registers holds user derivatives (coarse
derivatives). The last address register holds the clamp value.

#### IMAGE_SAMPLE_C_CD_O

Opcode: 110 (0x6e)  
Syntax: IMAGE_SAMPLE_C_CD_O VDATA(1:4), VADDR(5:12), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds
the Z-compare value. Next 2-6 address registers holds user derivatives (coarse
derivatives).

#### IMAGE_SAMPLE_C_D_CL_O

Opcode: 59 (0x3b)  
Syntax: IMAGE_SAMPLE_C_D_CL_O VDATA(1:4), VADDR(6:13), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds
the Z-compare value. Next 2-6 address registers holds user derivatives.
The last address register holds the clamp value.

#### IMAGE_SAMPLE_C_D_O

Opcode: 58 (0x3a)  
Syntax: IMAGE_SAMPLE_C_D_O VDATA(1:4), VADDR(5:12), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds
the Z-compare value. Next 2-6 address registers holds user derivatives.

#### IMAGE_SAMPLE_C_L

Opcode: 44 (0x2c)  
Syntax: IMAGE_SAMPLE_C_L VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value. The last address register holds
the LOD value.

#### IMAGE_SAMPLE_C_L_O

Opcode: 60 (0x3c)  
Syntax: IMAGE_SAMPLE_C_L_O VDATA(1:4), VADDR(4:7), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z.  Next address register holds
the Z-compare value. The last address register holds the LOD value.

#### IMAGE_SAMPLE_C_LZ

Opcode: 47 (0x2f)  
Syntax: IMAGE_SAMPLE_C_LZ VDATA(1:4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the Z-compare value. Force use of mipmap level 0 (???).

#### IMAGE_SAMPLE_C_LZ_O

Opcode: 63 (0x3f)  
Syntax: IMAGE_SAMPLE_C_LZ_O VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z. Next address register holds the
Z-compare value. Force use of mipmap level 0 (???).

#### IMAGE_SAMPLE_C_O

Opcode: 56 (0x38)  
Syntax: IMAGE_SAMPLE_C_O VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The instruction performs Z-compare operation choosen in SSAMP sampler.
The first address register holds the offset for X,Y,Z.
Next address register holds the Z-compare value.

#### IMAGE_SAMPLE_CD

Opcode: 104 (0x68)  
Syntax: IMAGE_SAMPLE_CD VDATA(1:4), VADDR(3:10), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first 2-6 address registers holds user derivatives (coarse derivatives).

#### IMAGE_SAMPLE_CD_CL

Opcode: 105 (0x69)  
Syntax: IMAGE_SAMPLE_CD_CL VDATA(1:4), VADDR(4:11), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first 2-6 address registers holds user derivatives (coarse derivatives).
The last address register holds the clamp value.

#### IMAGE_SAMPLE_CD_CL_O

Opcode: 109 (0x6d)  
Syntax: IMAGE_SAMPLE_CD_CL_O VDATA(1:4), VADDR(5:12), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.
Next 2-6 address registers holds user derivatives (coarse derivatives).
The last address register holds the clamp value.

#### IMAGE_SAMPLE_CD_O

Opcode: 108 (0x6c)  
Syntax: IMAGE_SAMPLE_CD_O VDATA(1:4), VADDR(4:11), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.
Next 2-6 address registers holds user derivatives (coarse derivatives).

#### IMAGE_SAMPLE_CL

Opcode: 33 (0x21)  
Syntax: IMAGE_SAMPLE_CL VDATA(1:4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The last address register holds the clamp value.

#### IMAGE_SAMPLE_CL_O

Opcode: 49 (0x31)  
Syntax: IMAGE_SAMPLE_CL_O VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.
The last address register holds the clamp value.

#### IMAGE_SAMPLE_D

Opcode: 34 (0x22)  
Syntax: IMAGE_SAMPLE_D VDATA(1:4), VADDR(3:10), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first 2-6 address registers holds user derivatives.

#### IMAGE_SAMPLE_D_CL

Opcode: 35 (0x23)  
Syntax: IMAGE_SAMPLE_D_CL VDATA(1:4), VADDR(4:11), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first 2-6 address registers holds user derivatives.
The last address register holds the clamp value.

#### IMAGE_SAMPLE_D_CL_O

Opcode: 51 (0x33)  
Syntax: IMAGE_SAMPLE_D_CL_O VDATA(1:4), VADDR(5:12), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.
Next 2-6 address registers holds user derivatives. The last address register
holds the clamp value.

#### IMAGE_SAMPLE_D_O

Opcode: 50 (0x32)  
Syntax: IMAGE_SAMPLE_D_O VDATA(1:4), VADDR(4:11), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.
Next 2-6 address registers holds user derivatives.

#### IMAGE_SAMPLE_L

Opcode: 36 (0x24)  
Syntax: IMAGE_SAMPLE_L VDATA(1:4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The last address register holds the LOD value.

#### IMAGE_SAMPLE_L_O

Opcode: 52 (0x34)  
Syntax: IMAGE_SAMPLE_L_O VDATA(1:4), VADDR(3:6), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.
The last address register holds the LOD value.

#### IMAGE_SAMPLE_LZ

Opcode: 39 (0x27)  
Syntax: IMAGE_SAMPLE_LZ VDATA(1:4), VADDR(1:4), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. Force use of mipmap level 0 (???).

#### IMAGE_SAMPLE_LZ_O

Opcode: 55 (0x37)  
Syntax: IMAGE_SAMPLE_LZ_O VDATA(1:4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. Force use of mipmap level 0 (???). The first address register holds
the offset for X,Y,Z.

#### IMAGE_SAMPLE_O

Opcode: 48 (0x30)  
Syntax: IMAGE_SAMPLE_O VDATA(1:4), VADDR(2:5), SRSRC(4,8), SSAMP(4)  
Description: Get sampled pixel value from SRSRC image at address VADDR by using
SSAMP sampler. The first address register holds the offset for X,Y,Z.

#### IMAGE_STORE

Opcode: 8 (0x8)  
Syntax: IMAGE_STORE VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Store data VDATA into image SRSRC to pixel at address VADDR. Data in VDATA
is in format given image resource.  
Operation:  
```
PIXELTYPE* V = VMIMG(SRSRC, VADDR)
STORE_IMAGE(V, CONVERT_TO_IMAGE(SRSRC, VDATA), DMASK)
```

#### IMAGE_STORE_MIP

Opcode: 9 (0x9)  
Syntax: IMAGE_STORE_MIP VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Store data VDATA into image SRSRC to pixel at address VADDR including
MIP index. Data in VDATA is in format given image resource.  
Operation:  
```
PIXELTYPE* V = VMIMG_MIP (SRSRC, VADDR)
STORE_IMAGE(V, CONVERT_TO_IMAGE(SRSRC, VDATA), DMASK)
```

#### IMAGE_STORE_PCK

Opcode: 10 (0xa)  
Syntax: IMAGE_STORE_PCK VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Store data VDATA into image SRSRC to pixel at address VADDR. Data in VDATA
is in raw format.  
Operation:  
```
PIXELTYPE* V = VMIMG(SRSRC, VADDR)
STORE_IMAGE(V, VDATA, DMASK)
```

#### IMAGE_STORE_MIP_PCK

Opcode: 11 (0xb)  
Syntax: IMAGE_STORE_MIP_PCK VDATA(1:4), VADDR(1:4), SRSRC(4,8)  
Description: Store data VDATA into image SRSRC to pixel at address VADDR including
MIP index. Data in VDATA is in raw format.  
Operation:  
```
PIXELTYPE* V = VMIMG_MIP(SRSRC, VADDR)
STORE_IMAGE(V, VDATA, DMASK)
```
