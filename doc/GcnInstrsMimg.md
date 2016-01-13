## GCN ISA MTBUF instructions

These instructions allow to access to images. MIMG instructions
operates on the image resources and on the sampler resources.

List of fields for the MIMG encoding:

Bits  | Name     | Description
------|----------|------------------------------
8-11  | DMASK    | Enable mask for read/write data components
12    | UNORM    | Accepts unnormalized coordinates
13    | GLC      | Globally coherent
14    | DA       | Data array (required by array of 1D and 2D images)
15    | R128     | Image resource size = 128
16    | TFE      | Texture Fail Enable (for partially resident textures).
17    | LWE      | LOD Warning Enable (for partially resident textures).
18-24 | OPCODE   | Operation code
25    | SLC      | System level coherent
26-31 | ENCODING | Encoding type. Must be 0b111100
32-39 | VADDR    | Vector address operands
40-47 | VDATA    | Vector data registers
48-52 | SRSRC    | Scalar registers with buffer resource (SGPR# is 4*value)
53-57 | SSAMP    | Scalar registers with sampler resource (SGPR# is 4*value)
63    | D16      | Convert 32-bit data to 16-bit data (GCN 1.2)

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

List of the MIMG instructions by opcode (GCN 1.2):

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
 29 (0x1d)  | IMAGE_ATOMIC_FCMPSWAP
 30 (0x1e)  | IMAGE_ATOMIC_FMIN
 31 (0x1f)  | IMAGE_ATOMIC_FMAX
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


### Instruction set

Alphabetically sorted instruction list:

