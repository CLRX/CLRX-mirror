## GCN ISA DS instructions

These instructions access to local or global data share (LDS/GDS) memory.

List of fields for the DS encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-7   | OFFSET0  | 8-bit offset 0 for VDATA0
8-15  | OFFSET1  | 8-bit offset 1 for VDATA1
0-15  | OFFSET   | 16-bit offset for single DATA
17    | GDS      | Access to GDS instead LDS
18-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110110
32-39 | ADDR     | Vector register that holds byte address
40-47 | VDATA0   | Source data 0 vector register
48-55 | VDATA1   | Source data 1 vector register
56-63 | VDST     | Destination vector register

Syntax: INSTRUCTION ADDR, VDATA0 [offset:OFFSET] [GDS]  
Syntax: INSTRUCTION ADDR, VDATA0, VDATA1 [offset0:OFFSET0] [offset1:OFFSET1] [GDS]  
Write syntax: INSTRUCTION VDST, ADDR [offset:OFFSET] [GDS]

NOTE: Any operation LDS requires correctly set M0 register, prior to execution.
The M0 register holds maximum size of a LDS memory, that can be accessed by wavefront.
To set no limits, just set a M0 register to 0xffffffff.

List of the instructions by opcode:

 Opcode     |GCN 1.0|GCN 1.1| Mnemonic (GCN 1.0/1.1) | Mnemonic (GCN 1.2)
------------|-------|-------|------------------------|-----------------------------
 0 (0x0)    |   ✓   |   ✓   | DS_ADD_U32             | DS_ADD_U32
 1 (0x1)    |   ✓   |   ✓   | DS_SUB_U32             | DS_SUB_U32
 2 (0x2)    |   ✓   |   ✓   | DS_RSUB_U32            | DS_RSUB_U32
 3 (0x3)    |   ✓   |   ✓   | DS_INC_U32             | DS_INC_U32
 4 (0x4)    |   ✓   |   ✓   | DS_DEC_U32             | DS_DEC_U32
 5 (0x5)    |   ✓   |   ✓   | DS_MIN_I32             | DS_MIN_I32
 6 (0x6)    |   ✓   |   ✓   | DS_MAX_I32             | DS_MAX_I32
 7 (0x7)    |   ✓   |   ✓   | DS_MIN_U32             | DS_MIN_U32
 8 (0x8)    |   ✓   |   ✓   | DS_MAX_U32             | DS_MAX_U32
 9 (0x9)    |   ✓   |   ✓   | DS_AND_B32             | DS_AND_B32
 10 (0xa)   |   ✓   |   ✓   | DS_OR_B32              | DS_OR_B32
 11 (0xb)   |   ✓   |   ✓   | DS_XOR_B32             | DS_XOR_B32
 12 (0xc)   |   ✓   |   ✓   | DS_MSKOR_B32           | DS_MSKOR_B32
 13 (0xd)   |   ✓   |   ✓   | DS_WRITE_B32           | DS_WRITE_B32
 14 (0xe)   |   ✓   |   ✓   | DS_WRITE2_B32          | DS_WRITE2_B32
 15 (0xf)   |   ✓   |   ✓   | DS_WRITE2ST64_B32      | DS_WRITE2ST64_B32
 16 (0x10)  |   ✓   |   ✓   | DS_CMPST_B32           | DS_CMPST_B32
 17 (0x11)  |   ✓   |   ✓   | DS_CMPST_F32           | DS_CMPST_F32
 18 (0x12)  |   ✓   |   ✓   | DS_MIN_F32             | DS_MIN_F32
 19 (0x13)  |   ✓   |   ✓   | DS_MAX_F32             | DS_MAX_F32
 20 (0x14)  |       |   ✓   | DS_NOP                 | DS_NOP
 21 (0x15)  |       |       | --                     | DS_ADD_F32
 24 (0x18)  |       |   ✓   | DS_GWS_SEMA_RELEASE_ALL| --
 25 (0x19)  |   ✓   |   ✓   | DS_GWS_INIT            | --
 26 (0x1a)  |   ✓   |   ✓   | DS_GWS_SEMA_V          | --
 27 (0x1b)  |   ✓   |   ✓   | DS_GWS_SEMA_BR         | --
 28 (0x1c)  |   ✓   |   ✓   | DS_GWS_SEMA_P          | --
 29 (0x1d)  |   ✓   |   ✓   | DS_GWS_BARRIER         | --
 30 (0x1e)  |   ✓   |   ✓   | DS_WRITE_B8            | DS_WRITE_B8
 31 (0x1f)  |   ✓   |   ✓   | DS_WRITE_B16           | DS_WRITE_B16
 32 (0x20)  |   ✓   |   ✓   | DS_ADD_RTN_U32         | DS_ADD_RTN_U32
 33 (0x21)  |   ✓   |   ✓   | DS_SUB_RTN_U32         | DS_SUB_RTN_U32
 34 (0x22)  |   ✓   |   ✓   | DS_RSUB_RTN_U32        | DS_RSUB_RTN_U32
 35 (0x23)  |   ✓   |   ✓   | DS_INC_RTN_U32         | DS_INC_RTN_U32
 36 (0x24)  |   ✓   |   ✓   | DS_DEC_RTN_U32         | DS_DEC_RTN_U32
 37 (0x25)  |   ✓   |   ✓   | DS_MIN_RTN_I32         | DS_MIN_RTN_I32
 38 (0x26)  |   ✓   |   ✓   | DS_MAX_RTN_I32         | DS_MAX_RTN_I32
 39 (0x27)  |   ✓   |   ✓   | DS_MIN_RTN_U32         | DS_MIN_RTN_U32
 40 (0x28)  |   ✓   |   ✓   | DS_MAX_RTN_U32         | DS_MAX_RTN_U32
 41 (0x29)  |   ✓   |   ✓   | DS_AND_RTN_B32         | DS_AND_RTN_B32
 42 (0x2a)  |   ✓   |   ✓   | DS_OR_RTN_B32          | DS_OR_RTN_B32
 43 (0x2b)  |   ✓   |   ✓   | DS_XOR_RTN_B32         | DS_XOR_RTN_B32
 44 (0x2c)  |   ✓   |   ✓   | DS_MSKOR_RTN_B32       | DS_MSKOR_RTN_B32
 45 (0x2d)  |   ✓   |   ✓   | DS_WRXCHG_RTN_B32      | DS_WRXCHG_RTN_B32
 46 (0x2e)  |   ✓   |   ✓   | DS_WRXCHG2_RTN_B32     | DS_WRXCHG2_RTN_B32
 47 (0x2f)  |   ✓   |   ✓   | DS_WRXCHG2ST64_RTN_B32 | DS_WRXCHG2ST64_RTN_B32
 48 (0x30)  |   ✓   |   ✓   | DS_CMPST_RTN_B32       | DS_CMPST_RTN_B32
 49 (0x31)  |   ✓   |   ✓   | DS_CMPST_RTN_F32       | DS_CMPST_RTN_F32
 50 (0x32)  |   ✓   |   ✓   | DS_MIN_RTN_F32         | DS_MIN_RTN_F32
 51 (0x33)  |   ✓   |   ✓   | DS_MAX_RTN_F32         | DS_MAX_RTN_F32
 52 (0x34)  |       |   ✓   | DS_WRAP_RTN_B32        | DS_WRAP_RTN_B32
 53 (0x35)  |   ✓   |   ✓   | DS_SWIZZLE_B32         | DS_ADD_RTN_F32
 54 (0x36)  |   ✓   |   ✓   | DS_READ_B32            | DS_READ_B32
 55 (0x37)  |   ✓   |   ✓   | DS_READ2_B32           | DS_READ2_B32
 56 (0x38)  |   ✓   |   ✓   | DS_READ2ST64_B32       | DS_READ2ST64_B32
 57 (0x39)  |   ✓   |   ✓   | DS_READ_I8             | DS_READ_I8
 58 (0x3a)  |   ✓   |   ✓   | DS_READ_U8             | DS_READ_U8
 59 (0x3b)  |   ✓   |   ✓   | DS_READ_I16            | DS_READ_I16
 60 (0x3c)  |   ✓   |   ✓   | DS_READ_U16            | DS_READ_U16
 61 (0x3d)  |   ✓   |   ✓   | DS_CONSUME             | DS_SWIZZLE_B32
 62 (0x3e)  |   ✓   |   ✓   | DS_APPEND              | DS_PERMUTE_B32
 63 (0x3f)  |   ✓   |   ✓   | DS_ORDERED_COUNT       | DS_BPERMUTE_B32
 64 (0x40)  |   ✓   |   ✓   | DS_ADD_U64             | DS_ADD_U64
 65 (0x41)  |   ✓   |   ✓   | DS_SUB_U64             | DS_SUB_U64
 66 (0x42)  |   ✓   |   ✓   | DS_RSUB_U64            | DS_RSUB_U64
 67 (0x43)  |   ✓   |   ✓   | DS_INC_U64             | DS_INC_U64
 68 (0x44)  |   ✓   |   ✓   | DS_DEC_U64             | DS_DEC_U64
 69 (0x45)  |   ✓   |   ✓   | DS_MIN_I64             | DS_MIN_I64
 70 (0x46)  |   ✓   |   ✓   | DS_MAX_I64             | DS_MAX_I64
 71 (0x47)  |   ✓   |   ✓   | DS_MIN_U64             | DS_MIN_U64
 72 (0x48)  |   ✓   |   ✓   | DS_MAX_U64             | DS_MAX_U64
 73 (0x49)  |   ✓   |   ✓   | DS_AND_B64             | DS_AND_B64
 74 (0x4a)  |   ✓   |   ✓   | DS_OR_B64              | DS_OR_B64
 75 (0x4b)  |   ✓   |   ✓   | DS_XOR_B64             | DS_XOR_B64
 76 (0x4c)  |   ✓   |   ✓   | DS_MSKOR_B64           | DS_MSKOR_B64
 77 (0x4d)  |   ✓   |   ✓   | DS_WRITE_B64           | DS_WRITE_B64
 78 (0x4e)  |   ✓   |   ✓   | DS_WRITE2_B64          | DS_WRITE2_B64
 79 (0x4f)  |   ✓   |   ✓   | DS_WRITE2ST64_B64      | DS_WRITE2ST64_B64
 80 (0x50)  |   ✓   |   ✓   | DS_CMPST_B64           | DS_CMPST_B64
 81 (0x51)  |   ✓   |   ✓   | DS_CMPST_F64           | DS_CMPST_F64
 82 (0x52)  |   ✓   |   ✓   | DS_MIN_F64             | DS_MIN_F64
 83 (0x53)  |   ✓   |   ✓   | DS_MAX_F64             | DS_MAX_F64
 96 (0x60)  |   ✓   |   ✓   | DS_ADD_RTN_U64         | DS_ADD_RTN_U64
 97 (0x61)  |   ✓   |   ✓   | DS_SUB_RTN_U64         | DS_SUB_RTN_U64
 98 (0x62)  |   ✓   |   ✓   | DS_RSUB_RTN_U64        | DS_RSUB_RTN_U64
 99 (0x63)  |   ✓   |   ✓   | DS_INC_RTN_U64         | DS_INC_RTN_U64
 100 (0x64) |   ✓   |   ✓   | DS_DEC_RTN_U64         | DS_DEC_RTN_U64
 101 (0x65) |   ✓   |   ✓   | DS_MIN_RTN_I64         | DS_MIN_RTN_I64
 102 (0x66) |   ✓   |   ✓   | DS_MAX_RTN_I64         | DS_MAX_RTN_I64
 103 (0x67) |   ✓   |   ✓   | DS_MIN_RTN_U64         | DS_MIN_RTN_U64
 104 (0x68) |   ✓   |   ✓   | DS_MAX_RTN_U64         | DS_MAX_RTN_U64
 105 (0x69) |   ✓   |   ✓   | DS_AND_RTN_B64         | DS_AND_RTN_B64
 106 (0x6a) |   ✓   |   ✓   | DS_OR_RTN_B64          | DS_OR_RTN_B64
 107 (0x6b) |   ✓   |   ✓   | DS_XOR_RTN_B64         | DS_XOR_RTN_B64
 108 (0x6c) |   ✓   |   ✓   | DS_MSKOR_RTN_B64       | DS_MSKOR_RTN_B64
 109 (0x6d) |   ✓   |   ✓   | DS_WRXCHG_RTN_B6       | DS_WRXCHG_RTN_B6
 110 (0x6e) |   ✓   |   ✓   | DS_WRXCHG2_RTN_B64     | DS_WRXCHG2_RTN_B64
 111 (0x6f) |   ✓   |   ✓   | DS_WRXCHG2ST64_RTN_B64 | DS_WRXCHG2ST64_RTN_B64
 112 (0x70) |   ✓   |   ✓   | DS_CMPST_RTN_B64       | DS_CMPST_RTN_B64
 113 (0x71) |   ✓   |   ✓   | DS_CMPST_RTN_F64       | DS_CMPST_RTN_F64
 114 (0x72) |   ✓   |   ✓   | DS_MIN_RTN_F64         | DS_MIN_RTN_F64
 115 (0x73) |   ✓   |   ✓   | DS_MAX_RTN_F64         | DS_MAX_RTN_F64
 118 (0x76) |   ✓   |   ✓   | DS_READ_B64            | DS_READ_B64
 119 (0x77) |   ✓   |   ✓   | DS_READ2_B64           | DS_READ2_B64
 120 (0x78) |   ✓   |   ✓   | DS_READ2ST64_B64       | DS_READ2ST64_B64
 126 (0x7e) |       |   ✓   | DS_CONDXCHG32_RTN_B64  | DS_CONDXCHG32_RTN_B64
 128 (0x80) |   ✓   |   ✓   | DS_ADD_SRC2_U32        | DS_ADD_SRC2_U32
 129 (0x81) |   ✓   |   ✓   | DS_SUB_SRC2_U32        | DS_SUB_SRC2_U32
 130 (0x82) |   ✓   |   ✓   | DS_RSUB_SRC2_U32       | DS_RSUB_SRC2_U32
 131 (0x83) |   ✓   |   ✓   | DS_INC_SRC2_U32        | DS_INC_SRC2_U32
 132 (0x84) |   ✓   |   ✓   | DS_DEC_SRC2_U32        | DS_DEC_SRC2_U32
 133 (0x85) |   ✓   |   ✓   | DS_MIN_SRC2_I32        | DS_MIN_SRC2_I32
 134 (0x86) |   ✓   |   ✓   | DS_MAX_SRC2_I32        | DS_MAX_SRC2_I32
 135 (0x87) |   ✓   |   ✓   | DS_MIN_SRC2_U32        | DS_MIN_SRC2_U32
 136 (0x88) |   ✓   |   ✓   | DS_MAX_SRC2_U32        | DS_MAX_SRC2_U32
 137 (0x89) |   ✓   |   ✓   | DS_AND_SRC2_B32        | DS_AND_SRC2_B32
 138 (0x8a) |   ✓   |   ✓   | DS_OR_SRC2_B32         | DS_OR_SRC2_B32
 139 (0x8b) |   ✓   |   ✓   | DS_XOR_SRC2_B32        | DS_XOR_SRC2_B32
 141 (0x8d) |   ✓   |   ✓   | DS_WRITE_SRC2_B32      | DS_WRITE_SRC2_B32
 146 (0x92) |   ✓   |   ✓   | DS_MIN_SRC2_F32        | DS_MIN_SRC2_F32
 147 (0x93) |   ✓   |   ✓   | DS_MAX_SRC2_F32        | DS_MAX_SRC2_F32
 149 (0x95) |       |       | --                     | DS_ADD_SRC2_F32
 152 (0x98) |       |       | --                     | DS_GWS_SEMA_RELEASE_ALL
 153 (0x99) |       |       | --                     | DS_GWS_INIT
 154 (0x9a) |       |       | --                     | DS_GWS_SEMA_V
 155 (0x9b) |       |       | --                     | DS_GWS_SEMA_BR
 156 (0x9c) |       |       | --                     | DS_GWS_SEMA_P
 157 (0x9d) |       |       | --                     | DS_GWS_BARRIER
 189 (0xbd) |       |       | --                     | DS_CONSUME
 190 (0xbe) |       |       | --                     | DS_APPEND
 191 (0xbf) |       |       | --                     | DS_ORDERED_COUNT
 192 (0xc0) |   ✓   |   ✓   | DS_ADD_SRC2_U64        | DS_ADD_SRC2_U64
 193 (0xc1) |   ✓   |   ✓   | DS_SUB_SRC2_U64        | DS_SUB_SRC2_U64
 194 (0xc2) |   ✓   |   ✓   | DS_RSUB_SRC2_U64       | DS_RSUB_SRC2_U64
 195 (0xc3) |   ✓   |   ✓   | DS_INC_SRC2_U64        | DS_INC_SRC2_U64
 196 (0xc4) |   ✓   |   ✓   | DS_DEC_SRC2_U64        | DS_DEC_SRC2_U64
 197 (0xc5) |   ✓   |   ✓   | DS_MIN_SRC2_I64        | DS_MIN_SRC2_I64
 198 (0xc6) |   ✓   |   ✓   | DS_MAX_SRC2_I64        | DS_MAX_SRC2_I64
 199 (0xc7) |   ✓   |   ✓   | DS_MIN_SRC2_U64        | DS_MIN_SRC2_U64
 200 (0xc8) |   ✓   |   ✓   | DS_MAX_SRC2_U64        | DS_MAX_SRC2_U64
 201 (0xc9) |   ✓   |   ✓   | DS_AND_SRC2_B64        | DS_AND_SRC2_B64
 202 (0xca) |   ✓   |   ✓   | DS_OR_SRC2_B64         | DS_OR_SRC2_B64
 203 (0xcb) |   ✓   |   ✓   | DS_XOR_SRC2_B64        | DS_XOR_SRC2_B64
 204 (0xcc) |   ✓   |   ✓   | DS_WRITE_SRC2_B64      | --
 205 (0xcd) |       |       | --                     | DS_WRITE_SRC2_B64
 210 (0xd2) |   ✓   |   ✓   | DS_MIN_SRC2_F64        | DS_MIN_SRC2_F64
 211 (0xd3) |   ✓   |   ✓   | DS_MAX_SRC2_F64        | DS_MAX_SRC2_F64
 222 (0xde) |       |   ✓   | DS_WRITE_B96           | DS_WRITE_B96
 223 (0xdf) |       |   ✓   | DS_WRITE_B128          | DS_WRITE_B128
 253 (0xfd) |       |   ✓   | DS_CONDXCHG32_RTN_B128 | DS_CONDXCHG32_RTN_B128
 254 (0xfe) |       |   ✓   | DS_READ_B96            | DS_READ_B96
 255 (0xff) |       |   ✓   | DS_READ_B128           | DS_READ_B128

### Instruction set

Alphabetically sorted instruction list:

#### DS_ADD_U32

Opcode: 0 (0x0)  
Syntax: DS_ADD_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Adds unsigned integer value from LDS/GDS at address (ADDR+OFFSET) & ~3 and
VDATA0, and store result back to LDS/GDS at this address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = *V + VDATA0  // atomic operation
```

#### DS_AND_B32

Opcode: 9 (0x9)  
Syntax: DS_AND_B32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise AND operatin on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = *V & VDATA0  // atomic operation
```

#### DS_DEC_U32

Opcode: 4 (0x4)  
Syntax: DS_DEC_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Load unsigned value from LDS/GDS at  address (ADDR+OFFSET) & ~3, and
compare with unsigned value from VDATA0. If VDATA0 greater or equal and loaded
unsigned value is zero, then increment value from LDS/GDS, otherwise store
VDATA0 to LDS/GDS. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = (VDATA0 >= *V && *V!=0) ? *V-1 : VDATA0  // atomic operation
```

#### DS_INC_U32

Opcode: 3 (0x3)  
Syntax: DS_INC_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Load unsigned value from LDS/GDS at address (ADDR+OFFSET) & ~3, and
compare with unsigned value from VDATA0. If VDATA0 greater, then increment value
from LDS/GDS, otherwise store 0 to LDS/GDS. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = (VDATA0 > *V) ? *V+1 : 0  // atomic operation
```

#### DS_MAX_I32

Opcode: 6 (0x6)  
Syntax: DS_MAX_I32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose greatest signed integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
INT32* V = (INT32*)(DS + (ADDR+OFFSET)&~3)
*V = MAX(*V, (INT32)VDATA0) // atomic operation
```

#### DS_MAX_U32

Opcode: 8 (0x8)  
Syntax: DS_MAX_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose greatest unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = MAX(*V, VDATA0) // atomic operation
```

#### DS_MIN_I32

Opcode: 5 (0x5)  
Syntax: DS_MIN_I32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest signed integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
INT32* V = (INT32*)(DS + (ADDR+OFFSET)&~3)
*V = MIN(*V, (INT32)VDATA0) // atomic operation
```

#### DS_MIN_U32

Opcode: 7 (0x7)  
Syntax: DS_MIN_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = MIN(*V, VDATA0) // atomic operation
```

#### DS_MSKOR_B32

Opcode: 11 (0xb)  
Syntax: DS_MSKOR_U32 ADDR, VDATA0, VDATA1 [OFFSET:OFFSET]  
Description: Load first argument from LDS/GDS at address (ADDR+OFFSET) & ~3.
Second and third arguments are from VDATA0 and VDATA1. The instruction keeps all bits
that is zeroed in second argument, and set all bits from third argument.
Result is stored in LDS/GDS at this same address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = (*V & ~VDATA0) | VDATA1 // atomic operation
```

#### DS_OR_B32

Opcode: 10 (0xa)  
Syntax: DS_OR_B32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise OR operatin on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = *V | VDATA0  // atomic operation
```

#### DS_RSUB_U32

Opcode: 2 (0x2)  
Syntax: DS_RSUB_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Subtract unsigned integer value from LDS/GDS at address (ADDR+OFFSET) & ~3
from value in VDATA0, and store result back to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = VDATA0 - *V  // atomic operation
```

#### DS_SUB_U32

Opcode: 1 (0x1)  
Syntax: DS_SUB_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Subtract VDATA0 from unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and store result back to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = *V - VDATA0  // atomic operation
```

#### DS_WRITE_B32

Opcode: 13 (0xd)  
Syntax: DS_WRITE_B32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Store value from VDATA0 into LDS/GDS at address (ADDR+OFFSET) & ~3.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = VDATA0
```

#### DS_WRITE2_B32

Opcode: 14 (0xe)  
Syntax: DS_WRITE2_B32 ADDR, VDATA0, VDATA1 [OFFSET0:OFFSET1] [OFFSET1:OFFSET1]  
Description: Store one dword from VDATA0 into LDS/GDS at address (ADDR+OFFSET0*4) & ~3,
and second into LDS/GDS at address (ADDR+OFFSET0*4) & ~3.  
Operation:  
```
UINT32* V0 = (UINT32*)(DS + (ADDR + OFFSET0*4)&~3) 
UINT32* V1 = (UINT32*)(DS + (ADDR + OFFSET1*4)&~3)
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_XOR_B32

Opcode: 11 (0xb)  
Syntax: DS_XOR_B32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise XOR operatin on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (ADDR+OFFSET)&~3)
*V = *V ^ VDATA0  // atomic operation
```
