## GCN ISA VOP2/VOP3 instructions

VOP2 instructions can be encoded in the VOP2 encoding and the VOP3A/VOP3B encoding.
List of fields for VOP2 encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-8   | SRC0     | First (scalar or vector) source operand
9-16  | VSRC1    | Second vector source operand
17-24 | VDST     | Destination vector operand
25-30 | OPCODE   | Operation code
31    | ENCODING | Encoding type. Must be 0

Syntax: INSTRUCTION VDST, SRC0, VSRC1

List of fields for VOP3A/VOP3B encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Vector destination operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
11    | CLAMP    | CLAMP modifier (VOP3A)
17-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

List of fields for VOP3A/VOP3B encoding (GCN 1.2/1.4):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Destination vector operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
11-14 | OP_SEL   | Operand selection (VOP3A) (GCN 1.4)
15    | CLAMP    | CLAMP modifier
16-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

Syntax: INSTRUCTION VDST, SRC0, SRC1 [MODIFIERS]

Modifiers:

* CLAMP - clamps destination floating point value in range 0.0-1.0
* MUL:2, MUL:4, DIV:2 - OMOD modifiers. Multiply destination floating point value by
2.0, 4.0 or 0.5 respectively. Clamping applied after OMOD modifier.
* -SRC - negate floating point value from source operand. Applied after ABS modifier.
* ABS(SRC), |SRC| - apply absolute value to source operand
* OP_SEL:VALUE|[B0,...] - operand half selection (0 - lower 16-bits, 1 - bits)

NOTE: OMOD modifier doesn't work if output denormals are allowed
(5 bit of MODE register for single precision or 7 bit for double precision).  
NOTE: OMOD and CLAMP modifier affects only for instruction that output is
floating point value.  
NOTE: ABS and negation is applied to source operand for any instruction.  

Negation and absolute value can be combined: `-ABS(V0)`. Modifiers CLAMP and
OMOD (MUL:2, MUL:4 and DIV:2) can be given in random order.

Operand half selection (OP_SEL) take value with bits number depends of number operands.
Last bit control destination operand. Zero in bit choose lower 16-bits in dword,
one choose higher 16-bits. Example: op_sel:[0,1,1] - higher 16-bits in second source and
in destination. List of bits of OP_SEL field:

Bit | Operand | Description
----|---------|----------------------
 11 | SRC0    | Choose part of SRC0 (first source operand)
 12 | SRC1    | Choose part of SRC1 (second source operand)
 13 | SRC2    | Choose part of SRC2 (third source operand)
 14 | VDST    | Choose part of VDST (destination)

Limitations for operands:

* only one SGPR can be read by instruction. Multiple occurrences of this same
SGPR is allowed
* only one literal constant can be used, and only when a SGPR or M0 is not used in
source operands
* only SRC0 can holds LDS_DIRECT

Unaligned pairs of SGPRs are allowed in source and destination operands.

VOP2 opcodes (0-63) are reflected in VOP3 in range: 256-319.
List of the instructions by opcode:

 Opcode     | Opcode(VOP3)| Mnemonic (GCN1.0/1.1) | Mnemonic (GCN 1.2)
------------|-------------|----------------------|------------------------
 0 (0x0)    | 256 (0x100) | V_CNDMASK_B32        | V_CNDMASK_B32
 1 (0x1)    | 257 (0x101) | V_READLANE_B32       | V_ADD_F32
 2 (0x2)    | 258 (0x102) | V_WRITELANE_B32      | V_SUB_F32
 3 (0x3)    | 259 (0x103) | V_ADD_F32            | V_SUBREV_F32
 4 (0x4)    | 260 (0x104) | V_SUB_F32            | V_MUL_LEGACY_F32
 5 (0x5)    | 261 (0x105) | V_SUBREV_F32         | V_MUL_F32
 6 (0x6)    | 262 (0x106) | V_MAC_LEGACY_F32     | V_MUL_I32_I24
 7 (0x7)    | 263 (0x107) | V_MUL_LEGACY_F32     | V_MUL_HI_I32_I24
 8 (0x8)    | 264 (0x108) | V_MUL_F32            | V_MUL_U32_U24
 9 (0x9)    | 265 (0x109) | V_MUL_I32_I24        | V_MUL_HI_U32_U24
 10 (0xa)   | 266 (0x10a) | V_MUL_HI_I32_I24     | V_MIN_F32
 11 (0xb)   | 267 (0x10b) | V_MUL_U32_U24        | V_MAX_F32
 12 (0xc)   | 268 (0x10c) | V_MUL_HI_U32_U24     | V_MIN_I32
 13 (0xd)   | 269 (0x10d) | V_MIN_LEGACY_F32     | V_MAX_I32
 14 (0xe)   | 270 (0x10e) | V_MAX_LEGACY_F32     | V_MIN_U32
 15 (0xf)   | 271 (0x10f) | V_MIN_F32            | V_MAX_U32
 16 (0x10)  | 272 (0x110) | V_MAX_F32            | V_LSHRREV_B32
 17 (0x11)  | 273 (0x111) | V_MIN_I32            | V_ASHRREV_I32
 18 (0x12)  | 274 (0x112) | V_MAX_I32            | V_LSHLREV_B32
 19 (0x13)  | 275 (0x113) | V_MIN_U32            | V_AND_B32
 20 (0x14)  | 276 (0x114) | V_MAX_U32            | V_OR_B32
 21 (0x15)  | 277 (0x115) | V_LSHR_B32           | V_XOR_B32
 22 (0x16)  | 278 (0x116) | V_LSHRREV_B32        | V_MAC_F32
 23 (0x17)  | 279 (0x117) | V_ASHR_I32           | V_MADMK_F32
 24 (0x18)  | 280 (0x118) | V_ASHRREV_I32        | V_MADAK_F32
 25 (0x19)  | 281 (0x119) | V_LSHL_B32           | V_ADD_U32 (VOP3B)
 26 (0x1a)  | 282 (0x11a) | V_LSHLREV_B32        | V_SUB_U32 (VOP3B)
 27 (0x1b)  | 283 (0x11b) | V_AND_B32            | V_SUBREV_U32 (VOP3B)
 28 (0x1c)  | 284 (0x11c) | V_OR_B32             | V_ADDC_U32 (VOP3B)
 29 (0x1d)  | 285 (0x11d) | V_XOR_B32            | V_SUBB_U32 (VOP3B)
 30 (0x1e)  | 286 (0x11e) | V_BFM_B32            | V_SUBBREV_U32 (VOP3B)
 31 (0x1f)  | 287 (0x11f) | V_MAC_F32            | V_ADD_F16
 32 (0x20)  | 288 (0x120) | V_MADMK_F32          | V_SUB_F16
 33 (0x21)  | 289 (0x121) | V_MADAK_F32          | V_SUBREV_F16
 34 (0x22)  | 290 (0x122) | V_BCNT_U32_B32       | V_MUL_F16
 35 (0x23)  | 291 (0x123) | V_MBCNT_LO_U32_B32   | V_MAC_F16
 36 (0x24)  | 292 (0x124) | V_MBCNT_HI_U32_B32   | V_MADMK_F16
 37 (0x25)  | 293 (0x125) | V_ADD_I32 (VOP3B)    | V_MADAK_F16
 38 (0x26)  | 294 (0x126) | V_SUB_I32 (VOP3B)    | V_ADD_U16
 39 (0x27)  | 295 (0x127) | V_SUBREV_I32 (VOP3B) | V_SUB_U16
 40 (0x28)  | 296 (0x128) | V_ADDC_U32 (VOP3B)   | V_SUBREV_U16
 41 (0x29)  | 297 (0x129) | V_SUBB_U32 (VOP3B)   | V_MUL_LO_U16
 42 (0x2a)  | 298 (0x12a) | V_SUBBREV_U32 (VOP3B)| V_LSHLREV_B16
 43 (0x2b)  | 299 (0x12b) | V_LDEXP_F32          | V_LSHRREV_B16
 44 (0x2c)  | 300 (0x12c) | V_CVT_PKACCUM_U8_F32 | V_ASHRREV_I16
 45 (0x2d)  | 301 (0x12d) | V_CVT_PKNORM_I16_F32 | V_MAX_F16
 46 (0x2e)  | 302 (0x12e) | V_CVT_PKNORM_U16_F32 | V_MIN_F16
 47 (0x2f)  | 303 (0x12f) | V_CVT_PKRTZ_F16_F32  | V_MAX_U16
 48 (0x30)  | 304 (0x130) | V_CVT_PK_U16_U32     | V_MAX_I16
 49 (0x31)  | 305 (0x131) | V_CVT_PK_I16_I32     | V_MIN_U16
 50 (0x32)  | 306 (0x132) | --                   | V_MIN_I16
 51 (0x33)  | 307 (0x133) | --                   | V_LDEXP_F16

List of the instructions by opcode (GCN 1.4):

 Opcode     | Opcode(VOP3)| Mnemonic (GCN 1.4)
------------|-------------|------------------------
 0 (0x0)    | 256 (0x100) | V_CNDMASK_B32
 1 (0x1)    | 257 (0x101) | V_ADD_F32
 2 (0x2)    | 258 (0x102) | V_SUB_F32
 3 (0x3)    | 259 (0x103) | V_SUBREV_F32
 4 (0x4)    | 260 (0x104) | V_MUL_LEGACY_F32
 5 (0x5)    | 261 (0x105) | V_MUL_F32
 6 (0x6)    | 262 (0x106) | V_MUL_I32_I24
 7 (0x7)    | 263 (0x107) | V_MUL_HI_I32_I24
 8 (0x8)    | 264 (0x108) | V_MUL_U32_U24
 9 (0x9)    | 265 (0x109) | V_MUL_HI_U32_U24
 10 (0xa)   | 266 (0x10a) | V_MIN_F32
 11 (0xb)   | 267 (0x10b) | V_MAX_F32
 12 (0xc)   | 268 (0x10c) | V_MIN_I32
 13 (0xd)   | 269 (0x10d) | V_MAX_I32
 14 (0xe)   | 270 (0x10e) | V_MIN_U32
 15 (0xf)   | 271 (0x10f) | V_MAX_U32
 16 (0x10)  | 272 (0x110) | V_LSHRREV_B32
 17 (0x11)  | 273 (0x111) | V_ASHRREV_I32
 18 (0x12)  | 274 (0x112) | V_LSHLREV_B32
 19 (0x13)  | 275 (0x113) | V_AND_B32
 20 (0x14)  | 276 (0x114) | V_OR_B32
 21 (0x15)  | 277 (0x115) | V_XOR_B32
 22 (0x16)  | 278 (0x116) | V_MAC_F32
 23 (0x17)  | 279 (0x117) | V_MADMK_F32
 24 (0x18)  | 280 (0x118) | V_MADAK_F32
 25 (0x19)  | 281 (0x119) | V_ADD_CO_U32 (VOP3B)
 26 (0x1a)  | 282 (0x11a) | V_SUB_CO_U32 (VOP3B)
 27 (0x1b)  | 283 (0x11b) | V_SUBREV_CO_U32 (VOP3B)
 28 (0x1c)  | 284 (0x11c) | V_ADDC_CO_U32 (VOP3B)
 29 (0x1d)  | 285 (0x11d) | V_SUBB_CO_U32 (VOP3B)
 30 (0x1e)  | 286 (0x11e) | V_SUBBREV_CO_U32 (VOP3B)
 31 (0x1f)  | 287 (0x11f) | V_ADD_F16
 32 (0x20)  | 288 (0x120) | V_SUB_F16
 33 (0x21)  | 289 (0x121) | V_SUBREV_F16
 34 (0x22)  | 290 (0x122) | V_MUL_F16
 35 (0x23)  | 291 (0x123) | V_MAC_F16
 36 (0x24)  | 292 (0x124) | V_MADMK_F16
 37 (0x25)  | 293 (0x125) | V_MADAK_F16
 38 (0x26)  | 294 (0x126) | V_ADD_U16
 39 (0x27)  | 295 (0x127) | V_SUB_U16
 40 (0x28)  | 296 (0x128) | V_SUBREV_U16
 41 (0x29)  | 297 (0x129) | V_MUL_LO_U16
 42 (0x2a)  | 298 (0x12a) | V_LSHLREV_B16
 43 (0x2b)  | 299 (0x12b) | V_LSHRREV_B16
 44 (0x2c)  | 300 (0x12c) | V_ASHRREV_I16
 45 (0x2d)  | 301 (0x12d) | V_MAX_F16
 46 (0x2e)  | 302 (0x12e) | V_MIN_F16
 47 (0x2f)  | 303 (0x12f) | V_MAX_U16
 48 (0x30)  | 304 (0x130) | V_MAX_I16
 49 (0x31)  | 305 (0x131) | V_MIN_U16
 50 (0x32)  | 306 (0x132) | V_MIN_I16
 51 (0x33)  | 307 (0x133) | V_LDEXP_F16
 52 (0x34)  | 308 (0x134) | V_ADD_U32
 53 (0x35)  | 309 (0x135) | V_SUB_U32
 54 (0x36)  | 310 (0x136) | V_SUBREV_U32

### Instruction set

Alphabetically sorted instruction list:

#### V_ADD_F16

Opcode VOP2: 31 (0x1f) for GCN 1.2  
Opcode VOP3A: 287 (0x11f) for GCN 1.2  
Syntax: V_ADD_F16 VDST, SRC0, SRC1  
Description: Add two FP16 values from SRC0 and SRC1 and store result to VDST.  
Operation:  
```
VDST = ASHALF(SRC0) + ASHALF(SRC1)
```

#### V_ADD_F32

Opcode VOP2: 3 (0x3) for GCN 1.0/1.1; 1 (0x1) for GCN 1.2  
Opcode VOP3A: 259 (0x103) for GCN 1.0/1.1; 257 (0x101) for GCN 1.2  
Syntax: V_ADD_F32 VDST, SRC0, SRC1  
Description: Add two FP values from SRC0 and SRC1 and store result to VDST.  
Operation:  
```
VDST = ASFLOAT(SRC0) + ASFLOAT(SRC1)
```

#### V_ADD_CO_U32

Opcode VOP2: 25 (0x19) for GCN 1.4  
Opcode VOP3B: 281 (0x119) for GCN 1.4  
Syntax VOP2: V_ADD_CO_U32 VDST, VCC, SRC0, SRC1  
Syntax VOP3B: V_ADD_CO_U32 VDST, SDST(2), SRC0, SRC1  
Description: Add SRC0 to SRC1 and store result to VDST and store carry flag to
SDST (or VCC) bit with number that equal to lane id. SDST is 64-bit.
Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 temp = (UINT64)SRC0 + (UINT64)SRC1
VDST = CLAMP ? MIN(temp, 0xffffffff) : temp
SDST = 0
UINT64 mask = (1ULL<<LANEID)
SDST = (SDST&~mask) | ((temp >> 32) ? mask : 0)
```

#### V_ADD_I32, V_ADD_U32 (GCN 1.0/1.1/1.2)

Opcode VOP2: 37 (0x25) for GCN 1.0/1.1; 25 (0x19) for GCN 1.2  
Opcode VOP3B: 293 (0x125) for GCN 1.0/1.1; 281 (0x119) for GCN 1.2  
Syntax VOP2 GCN 1.0/1.1: V_ADD_I32 VDST, VCC, SRC0, SRC1  
Syntax VOP3B GCN 1.0/1.1: V_ADD_I32 VDST, SDST(2), SRC0, SRC1  
Syntax VOP2 GCN 1.2: V_ADD_U32 VDST, VCC, SRC0, SRC1  
Syntax VOP3B GCN 1.2: V_ADD_U32 VDST, SDST(2), SRC0, SRC1  
Description: Add SRC0 to SRC1 and store result to VDST and store carry flag to
SDST (or VCC) bit with number that equal to lane id. SDST is 64-bit.
Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 temp = (UINT64)SRC0 + (UINT64)SRC1
VDST = CLAMP ? MIN(temp, 0xffffffff) : temp
SDST = 0
UINT64 mask = (1ULL<<LANEID)
SDST = (SDST&~mask) | ((temp >> 32) ? mask : 0)
```

#### V_ADD_U16

Opcode VOP2: 38 (0x26) for GCN 1.2  
Opcode VOP3A: 294 (0x126) for GCN 1.2  
Syntax: V_ADD_U16 VDST, SRC0, SRC1  
Description: Add two 16-bit unsigned values from SRC0 and SRC1 and
store 16-bit unsigned result to VDST.
If CLAMP modifier supplied, then result is saturated to 16-bit unsigned value.  
Operation:  
```
UINT32 TEMP = (SRC0 & 0xffff) + (SRC1 & 0xffff)
VDST = CLAMP ? MIN(0xffff, TEMP) : TEMP
```

#### V_ADD_U32 (GCN 1.4)

Opcode VOP2: 52 (0x34) for GCN 1.4  
Opcode VOP3B: 308 (0x134) for GCN 1.4  
Syntax: V_ADD_U32 VDST, SRC0, SRC1  
Description: Add SRC0 to SRC1 and store result to VDST.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 TEMP = (UINT64)SRC0 + SRC1
VDST = CLAMP ? MIN(TEMP, 0xffffffff) : TEMP
```

#### V_ADDC_CO_U32

Opcode VOP2: 28 (0x1c) for GCN 1.4  
Opcode VOP3B: 284 (0x11c) for GCN 1.4  
Syntax VOP2: V_ADDC_CO_U32 VDST, VCC, SRC0, SRC1, VCC  
Syntax VOP3B: V_ADDC_CO_U32 VDST, SDST(2), SRC0, SRC1, SSRC2(2)  
Description: Add SRC0 to SRC1 with carry stored in SSRC2 bit with number that equal lane id,
and store result to VDST and store carry flag to SDST (or VCC) bit with number
that equal to lane id. SDST and SSRC2 are 64-bit.
Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 mask = (1ULL<<LANEID)
UINT8 CC = ((SSRC2&mask) ? 1 : 0)
UINT64 temp = (UINT64)SRC0 + (UINT64)SRC1 + CC
SDST = 0
VDST = CLAMP ? MIN(temp, 0xffffffff) : temp
SDST = (SDST&~mask) | ((temp >> 32) ? mask : 0)
```

#### V_ADDC_U32 (GCN 1.0/1.1/1.2)

Opcode VOP2: 40 (0x28) for GCN 1.0/1.1; 28 (0x1c) for GCN 1.2  
Opcode VOP3B: 296 (0x128) for GCN 1.0/1.1; 284 (0x11c) for GCN 1.2  
Syntax VOP2: V_ADDC_U32 VDST, VCC, SRC0, SRC1, VCC  
Syntax VOP3B: V_ADDC_U32 VDST, SDST(2), SRC0, SRC1, SSRC2(2)  
Description: Add SRC0 to SRC1 with carry stored in SSRC2 bit with number that equal lane id,
and store result to VDST and store carry flag to SDST (or VCC) bit with number
that equal to lane id. SDST and SSRC2 are 64-bit.
Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 mask = (1ULL<<LANEID)
UINT8 CC = ((SSRC2&mask) ? 1 : 0)
UINT64 temp = (UINT64)SRC0 + (UINT64)SRC1 + CC
SDST = 0
VDST = CLAMP ? MIN(temp, 0xffffffff) : temp
SDST = (SDST&~mask) | ((temp >> 32) ? mask : 0)
```

#### V_AND_B32

Opcode: VOP2: 27 (0x1b) for GCN 1.0/1.1; 19 (0x13) for GCN 1.2  
Opcode: VOP3A: 283 (0x11b) for GCN 1.0/1.1; 275 (0x113) for GCN 1.2  
Syntax: V_AND_B32 VDST, SRC0, SRC1  
Description: Do bitwise AND on SRC0 and SRC1, store result to VDST.  
Operation:  
```
VDST = SRC0 & SRC1
```

#### V_ASHR_I32

Opcode VOP2: 23 (0x17) for GCN 1.0/1.1  
Opcode VOP3A: 279 (0x117) for GCN 1.0/1.1  
Syntax: V_ASHR_I32 VDST, SRC0, SRC1  
Description: Arithmetic shift right SRC0 by (SRC1&31) bits and store result into VDST.  
Operation:  
```
VDST = (INT32)SRC0 >> (SRC1&31)
```

#### V_ASHRREV_B16

Opcode VOP2: 44 (0x2c) for GCN 1.2  
Opcode VOP3A: 300 (0x12c) for GCN 1.2  
Syntax: V_ASHRREV_B16 VDST, SRC0, SRC1  
Description: Shift right signed 16-bit value from SRC1 by (SRC0&15) bits and
store 16-bit signed result into VDST.  
Operation:  
```
VDST = ((INT16)SRC1 >> (SRC0&15)) & 0xffff
```

#### V_ASHRREV_I32

Opcode VOP2: 24 (0x18) for GCN 1.0/1.1; 16 (0x11) for GCN 1.2  
Opcode VOP3A: 280 (0x118) for GCN 1.0/1.1; 272 (0x111) for GCN 1.2  
Syntax: V_ASHRREV_I32 VDST, SRC0, SRC1  
Description: Arithmetic shift right SRC1 by (SRC0&31) bits and store result into VDST.  
Operation:  
```
VDST = (INT32)SRC1 >> (SRC0&31)
```

#### V_BCNT_U32_B32

Opcode VOP2: 34 (0x22) for GCN 1.0/1.1  
Opcode VOP3A: 290 (0x122) for GCN 1.0/1.1  
Syntax: V_BCNT_U32_B32 VDST, SRC0, SRC1  
Description: Count bits in SRC0, adds SSRC1, and store result to VDST.  
Operation:  
```
VDST = SRC1 + BITCOUNT(SRC0)
```

#### V_BFM_B32

Opcode VOP2: 30 (0x1e) for GCN 1.0/1.1  
Opcode VOP3A: 286 (0x11e) for GCN 1.0/1.1  
Syntax: V_BFM_B32 VDST, SRC0, SRC1  
Description: Make 32-bit bitmask from (SRC1 & 31) bit that have length (SRC0 & 31) and
store it to VDST.  
Operation:  
```
VDST = ((1U << (SRC0&31))-1) << (SRC1&31)
```

#### V_CNDMASK_B32

Opcode VOP2: 0 (0x0) for GCN 1.0/1.1; 1 (0x0) for GCN 1.2  
Opcode VOP3A: 256 (0x100) for GCN 1.0/1.1; 256 (0x100) for GCN 1.2  
Syntax VOP2: V_CNDMASK_B32 VDST, SRC0, SRC1, VCC  
Syntax VOP3A: V_CNDMASK_B32 VDST, SRC0, SRC1, SSRC2(2)  
Description: If bit for current lane of VCC or SDST is set then store SRC1 to VDST,
otherwise store SRC0 to VDST.  
Operation:  
```
VDST = SSRC2&(1ULL<<LANEID) ? SRC1 : SRC0
```

#### V_CVT_PK_I16_I32

Opcode VOP2: 49 (0x31) for GCN 1.0/1.1  
Opcode VOP3A: 305 (0x131) for GCN 1.0/1.1  
Syntax: V_CVT_PK_I16_I32 VDST, SRC0, SRC1  
Description: Convert signed value from SRC0 and SRC1 to signed 16-bit values with
clamping, and store first value to low 16-bit and second to high 16-bit of the VDST.  
Operation:  
```
INT16 D0 = MAX(MIN((INT32)SRC0, 0x7fff), -0x8000) 
INT16 D1 = MAX(MIN((INT32)SRC1, 0x7fff), -0x8000)
VDST = D0 | (((UINT32)D1) << 16)
```

#### V_CVT_PK_U16_U32

Opcode VOP2: 48 (0x30) for GCN 1.0/1.1  
Opcode VOP3A: 304 (0x130) for GCN 1.0/1.1  
Syntax: V_CVT_PK_U16_U32 VDST, SRC0, SRC1  
Description: Convert unsigned value from SRC0 and SRC1 to unsigned 16-bit values with
clamping, and store first value to low 16-bit and second to high 16-bit of the VDST.  
Operation:  
```
UINT16 D0 = MIN(SRC0, 0xffff)
UINT16 D1 = MIN(SRC1, 0xffff)
VDST = D0 | (((UINT32)D1) << 16)
```

#### V_CVT_PKACCUM_U8_F32

Opcode VOP2: 44 (0x2c) for GCN 1.0/1.1  
Opcode VOP3A: 300 (0x12c) for GCN 1.0/1.1  
Syntax: V_CVT_PKACCUM_U8_F32 VDST, SRC0, SRC1  
Description: Convert floating point value from SRC0 to unsigned byte value with
rounding mode from MODE register, and store this byte to (SRC1&3)'th byte of VDST.  
Operation:  
```
UINT8 shift = ((SRC1&3) * 8)
UINT32 mask = 0xff << shift
FLOAT f = RNDINT(ASFLOAT(SRC0))
UINT8 VAL8 = 0
if (ISNAN(f))
    VAL8 = (UINT8)MAX(MIN(f, 255.0), 0.0)
VDST = (VDST&~mask) | (((UINT32)VAL8) << shift)
```

#### V_CVT_PKNORM_I16_F32

Opcode VOP2: 45 (0x2d) for GCN 1.0/1.1  
Opcode VOP3A: 301 (0x12d) for GCN 1.0/1.1  
Syntax: V_CVT_PKNORM_I16_F32 VDST, SRC0, SRC1  
Description: Convert normalized FP value from SRC0 and SRC1 to signed 16-bit integers with
rounding to nearest to even (??), and store first value to low 16-bit and
second to high 16-bit of the VDST.  
Operation:  
```
INT16 roundNorm(FLOAT S)
{
    FLOAT f = RNDNEINT(S*32767)
    if (ISNAN(f))
        return 0
    return (INT16)MAX(MIN(f, 32767.0), -32767.0)
}
VDST = roundNorm(ASFLOAT(SRC0)) | ((UINT32)roundNorm(ASFLOAT(SRC1)) << 16)
```

#### V_CVT_PKNORM_U16_F32

Opcode VOP2: 46 (0x2e) for GCN 1.0/1.1  
Opcode VOP3A: 302 (0x12e) for GCN 1.0/1.1  
Syntax: V_CVT_PKNORM_U16_F32 VDST, SRC0, SRC1  
Description: Convert normalized FP value from SRC0 and SRC1 to unsigned 16-bit integers with
rounding to nearest to even (??), and store first value to low 16-bit and
second to high 16-bit of the VDST.  
Operation:  
```
UINT16 roundNorm(FLOAT S)
{
    FLOAT f = RNDNEINT(S*65535.0)
    if (ISNAN(f))
        return 0
    return (INT16)MAX(MIN(f, 65535.0), 0.0)
}
VDST = roundNorm(ASFLOAT(SRC0)) | ((UINT32)roundNorm(ASFLOAT(SRC1)) << 16)
```

#### V_CVT_PKRTZ_F16_F32

Opcode VOP2: 47 (0x2f) for GCN 1.0/1.1  
Opcode VOP3A: 303 (0x12f) for GCN 1.0/1.1  
Syntax: V_CVT_PKRTZ_F16_F32 VDST, SRC0, SRC1  
Description: Convert normalized FP value from SRC0 and SRC1 to half floating points with
rounding to zero, and store first value to low 16-bit and
second to high 16-bit of the VDST.  
Operation:  
```
UINT16 D0 = ASINT16(CVT_HALF_RTZ(ASFLOAT(SRC0)))
UINT16 D1 = ASINT16(CVT_HALF_RTZ(ASFLOAT(SRC1)))
VDST = D0 | (((UINT32)D1) << 16)
```

#### V_LDEXP_F16

Opcode VOP2: 51 (0x33) for GCN 1.2  
Opcode VOP3A: 307 (0x133) for GCN 1.2  
Syntax: V_LDEXP_F16 VDST, SRC0, SRC1  
Description: Do ldexp operation on SRC0 and SRC1 (multiply SRC0 by 2**(SRC1)).
SRC1 is signed integer, SRC0 is half floating point value.  
Operation:  
```
VDST = ASHALF(SRC0) * POW(2.0, (INT32)SRC1)
```

#### V_LDEXP_F32

Opcode VOP2: 43 (0x2b) for GCN 1.0/1.1  
Opcode VOP3A: 299 (0x12b) for GCN 1.0/1.1  
Syntax: V_LDEXP_F32 VDST, SRC0, SRC1  
Description: Do ldexp operation on SRC0 and SRC1 (multiply SRC0 by 2**(SRC1)).
SRC1 is signed integer, SRC0 is floating point value.  
Operation:  
```
VDST = ASFLOAT(SRC0) * POW(2.0, (INT32)SRC1)
```

#### V_LSHL_B32

Opcode VOP2: 25 (0x19) for GCN 1.0/1.1  
Opcode VOP3A: 281 (0x119) for GCN 1.0/1.1  
Syntax: V_LSHL_B32 VDST, SRC0, SRC1  
Description: Shift left SRC0 by (SRC1&31) bits and store result into VDST.  
Operation:  
```
VDST = SRC0 << (SRC1&31)
```

#### V_LSHLREV_B16

Opcode VOP2: 42 (0x2a) for GCN 1.2  
Opcode VOP3A: 298 (0x12a) for GCN 1.2  
Syntax: V_LSHLREV_B16 VDST, SRC0, SRC1  
Description: Shift left unsigned 16-bit value from SRC1 by (SRC0&15) bits and
store 16-bit unsigned result into VDST.  
Operation:  
```
VDST = (SRC1 << (SRC0&15)) & 0xffff
```

#### V_LSHLREV_B32

Opcode VOP2: 26 (0x1a) for GCN 1.0/1.1; 18 (0x12) for GCN 1.2  
Opcode VOP3A: 282 (0x11a) for GCN 1.0/1.1; 274 (0x112) for GCN 1.2  
Syntax: V_LSHLREV_B32 VDST, SRC0, SRC1  
Description: Shift left SRC1 by (SRC0&31) bits and store result into VDST.  
Operation:  
```
VDST = SRC1 << (SRC0&31)
```

#### V_LSHR_B32

Opcode VOP2: 21 (0x15) for GCN 1.0/1.1  
Opcode VOP3A: 277 (0x115) for GCN 1.0/1.1  
Syntax: V_LSHR_B32 VDST, SRC0, SRC1  
Description: Shift right SRC0 by (SRC1&31) bits and store result into VDST.  
Operation:  
```
VDST = SRC0 >> (SRC1&31)
```

#### V_LSHRREV_B16

Opcode VOP2: 43 (0x2b) for GCN 1.2  
Opcode VOP3A: 299 (0x12b) for GCN 1.2  
Syntax: V_LSHRREV_B16 VDST, SRC0, SRC1  
Description: Shift right unsigned 16-bit value from SRC1 by (SRC0&15) bits and
store 16-bit unsigned result into VDST.  
Operation:  
```
VDST = (SRC1 >> (SRC0&15)) & 0xffff
```

#### V_LSHRREV_B32

Opcode VOP2: 22 (0x16) for GCN 1.0/1.1; 16 (0x10) for GCN 1.2  
Opcode VOP3A: 278 (0x116) for GCN 1.0/1.1; 272 (0x110) for GCN 1.2  
Syntax: V_LSHRREV_B32 VDST, SRC0, SRC1  
Description: Shift right SRC1 by (SRC0&31) bits and store result into VDST.  
Operation:  
```
VDST = SRC1 >> (SRC0&31)
```

#### V_MAC_F16

Opcode VOP2: 35 (0x23) for GCN 1.2  
Opcode VOP3A: 291 (0x123) for GCN 1.2  
Syntax: V_MAC_F16 VDST, SRC0, SRC1  
Description: Multiply FP16 value from SRC0 by FP16 value from SRC1 and
add result to VDST. It applies OMOD modifier to result and it flush denormals.  
Operation:  
```
VDST = ASHALF(SRC0) * ASHALF(SRC1) + ASHALF(VDST)
```

#### V_MAC_F32

Opcode VOP2: 31 (0x1f) for GCN 1.0/1.1; 22 (0x16) for GCN 1.2  
Opcode VOP3A: 287 (0x11f) for GCN 1.0/1.1; 278 (0x116) for GCN 1.2  
Syntax: V_MAC_F32 VDST, SRC0, SRC1  
Description: Multiply FP value from SRC0 by FP value from SRC1 and add result to VDST.
It applies OMOD modifier to result and it flush denormals.  
Operation:  
```
VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1) + ASFLOAT(VDST)
```

#### V_MAC_LEGACY_F32

Opcode VOP2: 6 (0x6) for GCN 1.0/1.1  
Opcode VOP3A: 262 (0x106) for GCN 1.0/1.1  
Syntax: V_MAC_LEGACY_F32 VDST, SRC0, SRC1  
Description: Multiply FP value from SRC0 by FP value from SRC1 and add result to VDST.
If one of value is 0.0 then always do not change VDST (do not apply IEEE rules for 0.0*x).
It applies OMOD modifier to result and it flush denormals.  
Operation:  
```
if (ASFLOAT(SRC0)!=0.0 && ASFLOAT(SRC1)!=0.0)
    VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1) + ASFLOAT(VDST)
```

#### V_MADAK_F16

Opcode: 37 (0x25) for GCN 1.2  
Opcode: 293 (0x125) for GCN 1.2  
Syntax: V_MADAK_F16 VDST, SRC0, SRC1, FLOAT16LIT  
Description: Multiply FP16 value from SRC0 with FP16 value from SRC1 and add
the constant literal FLOATLIT16; and store result to VDST. Constant literal follows
after instruction word.  
Operation:
```
VDST = ASHALF(SRC0) * ASHALF(SRC1) + ASHALF(FLOAT16LIT)
```

#### V_MADAK_F32

Opcode: VOP2: 33 (0x21) for GCN 1.0/1.1; 24 (0x18) for GCN 1.2  
Opcode: VOP3A: 289 (0x121) for GCN 1.0/1.1; 280 (0x118) for GCN 1.2  
Syntax: V_MADAK_F32 VDST, SRC0, SRC1, FLOATLIT  
Description: Multiply FP value from SRC0 with FP value from SRC1 and add
the constant literal FLOATLIT; and store result to VDST. Constant literal follows
after instruction word. It flush denormals.  
Operation:
```
VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1) + ASFLOAT(FLOATLIT)
```

#### V_MADMK_F16

Opcode: 36 (0x24) for GCN 1.2  
Opcode: 292 (0x124) for GCN 1.2  
Syntax: V_MADMK_F16 VDST, SRC0, FLOAT16LIT, SRC1  
Description: Multiply FP16 value from SRC0 with the constant literal FLOAT16LIT and add
FP16 value from SRC1; and store result to VDST. Constant literal follows
after instruction word.  
Operation:
```
VDST = ASHALF(SRC0) * ASHALF(FLOAT16LIT) + ASHALF(SRC1)
```

#### V_MADMK_F32

Opcode: VOP2: 32 (0x20) for GCN 1.0/1.1; 23 (0x17) for GCN 1.2  
Opcode: VOP3A: 288 (0x120) for GCN 1.0/1.1; 279 (0x117) for GCN 1.2  
Syntax: V_MADMK_F32 VDST, SRC0, FLOATLIT, SRC1  
Description: Multiply FP value from SRC0 with the constant literal FLOATLIT and add
FP value from SRC1; and store result to VDST. Constant literal follows
after instruction word. It flush denormals.  
Operation:
```
VDST = ASFLOAT(SRC0) * ASFLOAT(FLOATLIT) + ASFLOAT(SRC1)
```

#### V_MAX_F16

Opcode VOP2: 45 (0x2d) for GCN 1.2  
Opcode VOP3A: 301 (0x12d) for GCN 1.2  
Syntax: V_MAX_F16 VDST, SRC0, SRC1  
Description: Choose largest half floating point value from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
VDST = MAX(ASFHALF(SRC0), ASFHALF(SRC1))
```

#### V_MAX_F32

Opcode VOP2: 16 (0x10) for GCN 1.0/1.1; 11 (0xb) for GCN 1.2  
Opcode VOP3A: 272 (0x110) for GCN 1.0/1.1; 267 (0x10b) for GCN 1.2  
Syntax: V_MAX_F32 VDST, SRC0, SRC1  
Description: Choose largest floating point value from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
VDST = MAX(ASFLOAT(SRC0), ASFLOAT(SRC1))
```

#### V_MAX_I16

Opcode VOP2: 48 (0x30) for GCN 1.2  
Opcode VOP3A: 304 (0x130) for GCN 1.2  
Syntax: V_MIN_i16 VDST, SRC0, SRC1  
Description: Choose largest signed 16-bit value from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
VDST = MAX((INT16)SRC0, (INT16)SRC1)
```

#### V_MAX_I32

Opcode VOP2: 18 (0x12) for GCN 1.0/1.1; 13 (0xd) for GCN 1.2  
Opcode VOP3A: 274 (0x112) for GCN 1.0/1.1; 269 (0x10d) for GCN 1.2  
Syntax: V_MAX_I32 VDST, SRC0, SRC1  
Description: Choose largest signed value from SRC0 and SRC1, and store result to VDST.  
Operation:  
```
VDST = MAX((INT32)SRC0, (INT32)SRC1)
```

#### V_MAX_LEGACY_F32

Opcode VOP2: 14 (0xe) for GCN 1.0/1.1  
Opcode VOP3A: 270 (0x10e) for GCN 1.0/1.1  
Syntax: V_MAX_LEGACY_F32 VDST, SRC0, SRC1  
Description: Choose largest floating point value from SRC0 and SRC1,
and store result to VDST. If SSRC1 is NaN value then store NaN value to VDST
(legacy rules for handling NaNs).  
Operation:  
```
if (!ISNAN(ASFLOAT(SRC1)))
    VDST = MAX(ASFLOAT(SRC0), ASFLOAT(SRC1))
else
    VDST = NaN
```

#### V_MAX_U16

Opcode VOP2: 47 (0x2f) for GCN 1.2  
Opcode VOP3A: 303 (0x12f) for GCN 1.2  
Syntax: V_MAX_U16 VDST, SRC0, SRC1  
Description: Choose largest unsigned 16-bit value from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
VDST = MAX(SRC0&0xffff, SRC1&0xffff)
```

#### V_MAX_U32

Opcode VOP2: 20 (0x14) for GCN 1.0/1.1; 15 (0xf) for GCN 1.2  
Opcode VOP3A: 276 (0x114) for GCN 1.0/1.1; 271 (0x10f) for GCN 1.2  
Syntax: V_MAX_U32 VDST, SRC0, SRC1  
Description: Choose largest unsigned value from SRC0 and SRC1, and store result to VDST.  
Operation:  
```
VDST = MAX(SRC0, SRC1)
```

#### V_MBCNT_HI_U32_B32

Opcode VOP2: 36 (0x24) for GCN 1.0/1.1  
Opcode VOP3A: 292 (0x124) for GCN 1.0/1.1  
Syntax: V_MBCNT_HI_U32_B32 VDST, SRC0, SRC1  
Description: Make mask for all lanes ending at current lane,
get from that mask higher 32-bits, use it to mask SSRC0,
count bits in that value, and store result to VDST.  
Operation:  
```
UINT32 MASK = ((1ULL << (LANEID-32)) - 1ULL) & SRC0
VDST = SRC1 + BITCOUNT(MASK)
```

#### V_MBCNT_LO_U32_B32

Opcode VOP2: 35 (0x23) for GCN 1.0/1.1  
Opcode VOP3A: 291 (0x123) for GCN 1.0/1.1  
Syntax: V_MBCNT_LO_U32_B32 VDST, SRC0, SRC1  
Description: Make mask for all lanes ending at current lane,
get from that mask lower 32-bits, use it to mask SSRC0,
count bits in that value, and store result to VDST.  
Operation:  
```
UINT32 MASK = ((1ULL << LANEID) - 1ULL) & SRC0
VDST = SRC1 + BITCOUNT(MASK)
```

#### V_MIN_F16

Opcode VOP2: 46 (0x2e) for GCN 1.2  
Opcode VOP3A: 302 (0x12e) for GCN 1.2  
Syntax: V_MIN_F16 VDST, SRC0, SRC1  
Description: Choose smallest half floating point value from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
VDST = MIN(ASFHALF(SRC0), ASFHALF(SRC1))
```

#### V_MIN_F32

Opcode VOP2: 15 (0xf) for GCN 1.0/1.1; 10 (0xa) for GCN 1.2  
Opcode VOP3A: 271 (0x10f) for GCN 1.0/1.1; 266 (0x10a) for GCN 1.2  
Syntax: V_MIN_F32 VDST, SRC0, SRC1  
Description: Choose smallest floating point value from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
VDST = MIN(ASFLOAT(SRC0), ASFLOAT(SRC1))
```

#### V_MIN_I16

Opcode VOP2: 50 (0x32) for GCN 1.2  
Opcode VOP3A: 306 (0x132) for GCN 1.2  
Syntax: V_MIN_i16 VDST, SRC0, SRC1  
Description: Choose smallest signed 16-bit value from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
VDST = MIN((INT16)SRC0, (INT16)SRC1)
```

#### V_MIN_I32

Opcode VOP2: 17 (0x11) for GCN 1.0/1.1; 12 (0xc) for GCN 1.2  
Opcode VOP3A: 273 (0x111) for GCN 1.0/1.1; 268 (0x10c) for GCN 1.2  
Syntax: V_MIN_I32 VDST, SRC0, SRC1  
Description: Choose smallest signed value from SRC0 and SRC1, and store result to VDST.  
Operation:  
```
VDST = MIN((INT32)SRC0, (INT32)SRC1)
```

#### V_MIN_LEGACY_F32

Opcode VOP2: 13 (0xd) for GCN 1.0/1.1  
Opcode VOP3A: 269 (0x10d) for GCN 1.0/1.1  
Syntax: V_MIN_LEGACY_F32 VDST, SRC0, SRC1  
Description: Choose smallest floating point value from SRC0 and SRC1,
and store result to VDST. If SSRC1 is NaN value then store NaN value to VDST
(legacy rules for handling NaNs).  
Operation:  
```
if (!ISNAN(ASFLOAT(SRC1)))
    VDST = MIN(ASFLOAT(SRC0), ASFLOAT(SRC1))
else
    VDST = NaN
```

#### V_MIN_U16

Opcode VOP2: 49 (0x31) for GCN 1.2  
Opcode VOP3A: 305 (0x131) for GCN 1.2  
Syntax: V_MIN_U16 VDST, SRC0, SRC1  
Description: Choose smallest unsigned 16-bit value from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
VDST = MIN(SRC0&0xffff, SRC1&0xffff)
```

#### V_MIN_U32

Opcode VOP2: 19 (0x13) for GCN 1.0/1.1; 14 (0xe) for GCN 1.2  
Opcode VOP3A: 275 (0x113) for GCN 1.0/1.1; 270 (0x10e) for GCN 1.2  
Syntax: V_MIN_U32 VDST, SRC0, SRC1  
Description: Choose smallest unsigned value from SRC0 and SRC1, and store result to VDST.  
Operation:  
```
VDST = MIN(SRC0, SRC1)
```

#### V_MUL_LEGACY_F32

Opcode VOP2: 7 (0x7) for GCN 1.0/1.1; 5 (0x4) for GCN 1.2  
Opcode VOP3A: 263 (0x107) for GCN 1.0/1.1; 260 (0x104) for GCN 1.2  
Syntax: V_MUL_LEGACY_F32 VDST, SRC0, SRC1  
Description: Multiply FP value from SRC0 by FP value from SRC1 and store result to VDST.
If one of value is 0.0 then always store 0.0 to VDST (do not apply IEEE rules for 0.0*x).  
Operation:  
```
if (ASFLOAT(SRC0)!=0.0 && ASFLOAT(SRC1)!=0.0)
    VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1)
else
    VDST = 0.0
```

#### V_MUL_F16

Opcode VOP2: 34 (0x22) for GCN 1.2  
Opcode VOP3A: 290 (0x122) for GCN 1.2  
Syntax: V_MUL_F16 VDST, SRC0, SRC1  
Description: Multiply FP16 value from SRC0 by FP16 value from SRC1
and store result to VDST.  
Operation:  
```
VDST = ASHALF(SRC0) * ASHALF(SRC1)
```

#### V_MUL_F32

Opcode VOP2: 8 (0x8) for GCN 1.0/1.1; 5 (0x5) for GCN 1.2  
Opcode VOP3A: 264 (0x108) for GCN 1.0/1.1; 261 (0x105) for GCN 1.2  
Syntax: V_MUL_F32 VDST, SRC0, SRC1  
Description: Multiply FP value from SRC0 by FP value from SRC1 and store result to VDST.  
Operation:  
```
VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1)
```

#### V_MUL_HI_I32_I24

Opcode VOP2: 10 (0xa) for GCN 1.0/1.1; 7 (0x7) for GCN 1.2  
Opcode VOP3A: 266 (0x10a) for GCN 1.0/1.1; 263 (0x107) for GCN 1.2  
Syntax: V_MUL_HI_I32_I24 VDST, SRC0, SRC1  
Description: Multiply 24-bit signed integer value from SRC0 by 24-bit signed value from SRC1
and store higher 16-bit of the result to VDST with sign extension.
Any modifier doesn't affect on result.  
Operation:  
```
INT32 V0 = (INT32)((SRC0&0x7fffff) | (SSRC0&0x800000 ? 0xff800000 : 0))
INT32 V1 = (INT32)((SRC1&0x7fffff) | (SSRC1&0x800000 ? 0xff800000 : 0))
VDST = ((INT64)V0 * V1)>>32
```

#### V_MUL_HI_U32_U24

Opcode VOP2: 12 (0xc) for GCN 1.0/1.1; 9 (0x9) for GCN 1.2  
Opcode VOP3A: 268 (0x10c) for GCN 1.0/1.1; 265 (0x109) for GCN 1.2  
Syntax: V_MUL_HI_U32_U24 VDST, SRC0, SRC1  
Description: Multiply 24-bit unsigned integer value from SRC0 by 24-bit unsigned value
from SRC1 and store higher 16-bit of the result to VDST.
Any modifier doesn't affect on result.  
Operation:  
```
VDST = ((UINT64)(SRC0&0xffffff) * (UINT32)(SRC1&0xffffff)) >> 32
```

#### V_MUL_I32_I24

Opcode VOP2: 9 (0x9) for GCN 1.0/1.1; 6 (0x6) for GCN 1.2  
Opcode VOP3A: 265 (0x109) for GCN 1.0/1.1; 262 (0x106) for GCN 1.2  
Syntax: V_MUL_I32_I24 VDST, SRC0, SRC1  
Description: Multiply 24-bit signed integer value from SRC0 by 24-bit signed value from SRC1
and store result to VDST. Any modifier doesn't affect on result.  
Operation:  
```
INT32 V0 = (INT32)((SRC0&0x7fffff) | (SSRC0&0x800000 ? 0xff800000 : 0))
INT32 V1 = (INT32)((SRC1&0x7fffff) | (SSRC1&0x800000 ? 0xff800000 : 0))
VDST = V0 * V1
```

#### V_MUL_LO_U16

Opcode VOP2: 41 (0x29) for GCN 1.2  
Opcode VOP3A: 297 (0x129) for GCN 1.2  
Syntax: V_MUL_LO_U16 VDST, SRC0, SRC1  
Description: Multiply 16-bit unsigned value from SRC0 by 16-bit unsigned value from SRC1
and store 16-bit result to VDST.  
Operation:  
```
VDST = ((SRC0&0Xffff) * (SRC1&0xffff)) & 0xffff
```

#### V_MUL_U32_U24

Opcode VOP2: 11 (0xb) for GCN 1.0/1.1; 8 (0x8) for GCN 1.2  
Opcode VOP3A: 267 (0x10b) for GCN 1.0/1.1; 264 (0x108) for GCN 1.2  
Syntax: V_MUL_U32_U24 VDST, SRC0, SRC1  
Description: Multiply 24-bit unsigned integer value from SRC0 by 24-bit unsigned value
from SRC1 and store result to VDST. Any modifier doesn't affect on result.  
Operation:  
```
VDST = (UINT32)(SRC0&0xffffff) * (UINT32)(SRC1&0xffffff)
```

#### V_OR_B32

Opcode: VOP2: 28 (0x1c) for GCN 1.0/1.1; 20 (0x14) for GCN 1.2  
Opcode: VOP3A: 284 (0x11c) for GCN 1.0/1.1; 276 (0x114) for GCN 1.2  
Syntax: V_OR_B32 VDST, SRC0, SRC1  
Description: Do bitwise OR operation on SRC0 and SRC1, store result to VDST.
CLAMP and OMOD modifier doesn't affect on result.  
Operation:  
```
VDST = SRC0 | SRC1
```

#### V_READLANE_B32

Opcode VOP2: 1 (0x1) for GCN 1.0/1.1  
Opcode VOP3A: 257 (0x101) for GCN 1.0/1.1  
Syntax: V_READLANE_B32 SDST, VSRC0, SSRC1  
Description: Copy one VSRC0 lane value to one SDST. Lane (thread id) choosen from SSRC1&63.
SSRC1 can be SGPR or M0. Ignores EXEC mask.  
Operation:  
```
SDST = VSRC0[SSRC1 & 63]
```

#### V_SUB_F16

Opcode VOP2: 32 (0x20) for GCN 1.2  
Opcode VOP3A: 288 (0x120) for GCN 1.2  
Syntax: V_SUB_F16 VDST, SRC0, SRC1  
Description: Subtract FP16 value of SRC1 from FP16 value of SRC0 and store result to VDST.  
Operation:  
```
VDST = ASHALF(SRC0) - ASHALF(SRC1)
```

#### V_SUB_F32

Opcode VOP2: 4 (0x4) for GCN 1.0/1.1; 2 (0x2) for GCN 1.2  
Opcode VOP3A: 260 (0x104) for GCN 1.0/1.1; 258 (0x102) for GCN 1.2  
Syntax: V_SUB_F32 VDST, SRC0, SRC1  
Description: Subtract FP value of SRC1 from FP value of SRC0 and store result to VDST.  
Operation:  
```
VDST = ASFLOAT(SRC0) - ASFLOAT(SRC1)
```

#### V_SUB_U16

Opcode VOP2: 39 (0x27) for GCN 1.2  
Opcode VOP3A: 295 (0x127) for GCN 1.2  
Syntax: V_SUB_U16 VDST, SRC0, SRC1  
Description: Subtract unsigned 16-bit value of SRC1 from SRC0 and store
16-bit unsigned result to VDST.
If CLAMP modifier supplied, then result is saturated to 16-bit unsigned value.  
Operation:  
```
INT32 TEMP = (SRC0 & 0xffff) - (SRC1 & 0xffff)
VDST = CLAMP ? MAX(TEMP, 0) : TEMP
```

#### V_SUB_CO_U32

Opcode VOP2: 26 (0x1a) for GCN 1.4  
Opcode VOP3B: 282 (0x11a) for GCN 1.4  
Syntax VOP2: V_SUB_CO_U32 VDST, VCC, SRC0, SRC1  
Syntax VOP3B: V_SUB_CO_U32 VDST, SDST(2), SRC0, SRC1  
Description: Subtract SRC1 from SRC0 and store result to VDST and store borrow flag to
SDST (or VCC) bit with number that equal to lane id. SDST is 64-bit.
Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 temp = (UINT64)SRC0 - (UINT64)SRC1
VDST = CLAMP ? (temp>>32 ? 0 : temp) : temp
SDST = 0
UINT64 mask = (1ULL<<LANEID)
SDST = (SDST&~mask) | ((temp>>32) ? mask : 0)
```

#### V_SUB_I32, V_SUB_U32 (GCN 1.0/1.1/1.2)

Opcode VOP2: 38 (0x26) for GCN 1.0/1.1; 26 (0x1a) for GCN 1.2  
Opcode VOP3B: 294 (0x126) for GCN 1.0/1.1; 282 (0x11a) for GCN 1.2  
Syntax VOP2 GCN 1.0/1.1: V_SUB_I32 VDST, VCC, SRC0, SRC1  
Syntax VOP3B GCN 1.0/1.1: V_SUB_I32 VDST, SDST(2), SRC0, SRC1  
Syntax VOP2 GCN 1.2: V_SUB_U32 VDST, VCC, SRC0, SRC1  
Syntax VOP3B GCN 1.2: V_SUB_U32 VDST, SDST(2), SRC0, SRC1  
Description: Subtract SRC1 from SRC0 and store result to VDST and store borrow flag to
SDST (or VCC) bit with number that equal to lane id. SDST is 64-bit.
Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 temp = (UINT64)SRC0 - (UINT64)SRC1
VDST = CLAMP ? (temp>>32 ? 0 : temp) : temp
SDST = 0
UINT64 mask = (1ULL<<LANEID)
SDST = (SDST&~mask) | ((temp>>32) ? mask : 0)
```

#### V_SUB_U32 (GCN 1.4)

Opcode VOP2: 53 (0x35) for GCN 1.4  
Opcode VOP3B: 309 (0x135) for GCN 1.4  
Syntax: V_SUB_U32 VDST, SRC0, SRC1  
Description: Subtract SRC1 with borrow from SRC0, and store result to VDST.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
INT64 TEMP = (UINT64)SRC0 - SRC1
VDST = CLAMP ? MAX(0, TEMP) : TEMP
```

#### V_SUBB_CO_U32

Opcode VOP2: 29 (0x1d) for GCN 1.4  
Opcode VOP3B: 285 (0x11d) for GCN 1.4  
Syntax VOP2: V_SUBB_CO_U32 VDST, VCC, SRC0, SRC1, VCC  
Syntax VOP3B: V_SUBB_CO_U32 VDST, SDST(2), SRC0, SRC1, SSRC2(2)  
Description: Subtract SRC1 with borrow from SRC0,
and store result to VDST and store carry flag to SDST (or VCC) bit with number
that equal to lane id. Borrow is stored in SSRC2 bit with number of lane id.
SDST and SSRC2 are 64-bit. Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 mask = (1ULL<<LANEID)
UINT8 CC = ((SSRC2&mask) ? 1 : 0)
UINT64 temp = (UINT64)SRC0 - (UINT64)SRC1 - CC
SDST = 0
VDST = CLAMP ? (temp>>32 ? 0 : temp) : temp
SDST = (SDST&~mask) | ((temp >> 32) ? mask : 0)
```

#### V_SUBB_U32 (GCN 1.0/1.1/1.2)

Opcode VOP2: 41 (0x29) for GCN 1.0/1.1; 29 (0x1d) for GCN 1.2  
Opcode VOP3B: 297 (0x129) for GCN 1.0/1.1; 285 (0x11d) for GCN 1.2  
Syntax VOP2: V_SUBB_U32 VDST, VCC, SRC0, SRC1, VCC  
Syntax VOP3B: V_SUBB_U32 VDST, SDST(2), SRC0, SRC1, SSRC2(2)  
Description: Subtract SRC1 with borrow from SRC0,
and store result to VDST and store carry flag to SDST (or VCC) bit with number
that equal to lane id. Borrow is stored in SSRC2 bit with number of lane id.
SDST and SSRC2 are 64-bit. Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 mask = (1ULL<<LANEID)
UINT8 CC = ((SSRC2&mask) ? 1 : 0)
UINT64 temp = (UINT64)SRC0 - (UINT64)SRC1 - CC
SDST = 0
VDST = CLAMP ? (temp>>32 ? 0 : temp) : temp
SDST = (SDST&~mask) | ((temp >> 32) ? mask : 0)
```

#### V_SUBBREV_CO_U32

Opcode VOP2: 30 (0x1e) for GCN 1.4  
Opcode VOP3B: 286 (0x11e) for GCN 1.4  
Syntax VOP2: V_SUBBREV_CO_U32 VDST, VCC, SRC0, SRC1, VCC  
Syntax VOP3B: V_SUBBREV_CO_U32 VDST, SDST(2), SRC0, SRC1, SSRC2(2)  
Description: Subtract SRC0 with borrow from SRC1,
and store result to VDST and store carry flag to SDST (or VCC) bit with number
that equal to lane id. Borrow is stored in SSRC2 bit with number of lane id.
SDST and SSRC2 are 64-bit. Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 mask = (1ULL<<LANEID)
UINT8 CC = ((SSRC2&mask) ? 1 : 0)
UINT64 temp = (UINT64)SRC1 - (UINT64)SRC0 - CC
SDST = 0
VDST = CLAMP ? (temp>>32 ? 0 : temp) : temp
SDST = (SDST&~mask) | ((temp >> 32) ? mask : 0)
```

#### V_SUBBREV_U32 (GCN 1.0/1.1/1.2)

Opcode VOP2: 42 (0x2a) for GCN 1.0/1.1; 30 (0x1e) for GCN 1.2  
Opcode VOP3B: 298 (0x12a) for GCN 1.0/1.1; 286 (0x11e) for GCN 1.2  
Syntax VOP2: V_SUBBREV_U32 VDST, VCC, SRC0, SRC1, VCC  
Syntax VOP3B: V_SUBBREV_U32 VDST, SDST(2), SRC0, SRC1, SSRC2(2)  
Description: Subtract SRC0 with borrow from SRC1,
and store result to VDST and store carry flag to SDST (or VCC) bit with number
that equal to lane id. Borrow is stored in SSRC2 bit with number of lane id.
SDST and SSRC2 are 64-bit. Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 mask = (1ULL<<LANEID)
UINT8 CC = ((SSRC2&mask) ? 1 : 0)
UINT64 temp = (UINT64)SRC1 - (UINT64)SRC0 - CC
SDST = 0
VDST = CLAMP ? (temp>>32 ? 0 : temp) : temp
SDST = (SDST&~mask) | ((temp >> 32) ? mask : 0)
```

#### V_SUBREV_F16

Opcode VOP2: 33 (0x21) for GCN 1.2  
Opcode VOP3A: 289 (0x121) for GCN 1.2  
Syntax: V_SUBREV_F16 VDST, SRC0, SRC1  
Description: Subtract FP16 value of SRC0 from FP16 value of SRC1 and store result to VDST.  
Operation:  
```
VDST = ASHALF(SRC1) - ASHALF(SRC0)
```

#### V_SUBREV_F32

Opcode VOP2: 5 (0x5) for GCN 1.0/1.1; 2 (0x3) for GCN 1.2  
Opcode VOP3A: 261 (0x105) for GCN 1.0/1.1; 259 (0x103) for GCN 1.2  
Syntax: V_SUBREV_F32 VDST, SRC0, SRC1  
Description: Subtract FP value of SRC0 from FP value of SRC1 and store result to VDST.  
Operation:  
```
VDST = ASFLOAT(SRC1) - ASFLOAT(SRC0)
```

#### V_SUBREV_I32, V_SUBREV_U32 (GCN 1.0/1.1/1.2)

Opcode VOP2: 39 (0x27) for GCN 1.0/1.1; 27 (0x1b) for GCN 1.2  
Opcode VOP3B: 295 (0x127) for GCN 1.0/1.1; 283 (0x11b) for GCN 1.2  
Syntax VOP2 GCN 1.0/1.1: V_SUBREV_I32 VDST, VCC, SRC0, SRC1  
Syntax VOP3B GCN 1.0/1.1: V_SUBREV_I32 VDST, SDST(2), SRC0, SRC1  
Syntax VOP2 GCN 1.2: V_SUBREV_U32 VDST, VCC, SRC0, SRC1  
Syntax VOP3B GCN 1.2: V_SUBREV_U32 VDST, SDST(2), SRC0, SRC1  
Description: Subtract SRC0 from SRC1 and store result to VDST and store borrow flag to
SDST (or VCC) bit with number that equal to lane id. SDST is 64-bit.
Bits for inactive threads in SDST are always zeroed.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
UINT64 temp = (UINT64)SRC1 - (UINT64)SRC0
VDST = CLAMP ? (temp>>32 ? 0 : temp) : temp
SDST = 0
UINT64 mask = (1ULL<<LANEID)
SDST = (SDST&~mask) | ((temp>>32) ? mask : 0)
```

#### V_SUBREV_U16

Opcode VOP2: 40 (0x28) for GCN 1.2  
Opcode VOP3A: 296 (0x128) for GCN 1.2  
Syntax: V_SUBREV_U16 VDST, SRC0, SRC1  
Description: Subtract unsigned 16-bit value of SRC0 from SRC1 and store
16-bit unsigned result to VDST.
If CLAMP modifier supplied, then result is saturated to 16-bit unsigned value.  
Operation:  
```
INT32 TEMP = (SRC1 & 0xffff) - (SRC0 & 0xffff)
VDST = CLAMP ? MAX(0, TEMP) : TEMP
```

#### V_SUBREV_U32 (GCN 1.4)

Opcode VOP2: 54 (0x36) for GCN 1.4  
Opcode VOP3B: 310 (0x136) for GCN 1.4  
Syntax: V_SUBREV_U32 VDST, SRC0, SRC1  
Description: Subtract SRC0 with borrow from SRC1, and store result to VDST.
If CLAMP modifier supplied, then result is saturated to 32-bit unsigned value.  
Operation:  
```
INT64 TEMP = (UINT64)SRC1 - SRC0
VDST = CLAMP ? MAX(0, TEMP) : TEMP
```

#### V_XOR_B32

Opcode: VOP2: 29 (0x1d) for GCN 1.0/1.1; 21 (0x15) for GCN 1.2  
Opcode: VOP3A: 285 (0x11d) for GCN 1.0/1.1; 277 (0x115) for GCN 1.2  
Syntax: V_XOR_B32 VDST, SRC0, SRC1  
Description: Do bitwise XOR operation on SRC0 and SRC1, store result to VDST.
CLAMP and OMOD modifier doesn't affect on result.  
Operation:  
```
VDST = SRC0 ^ SRC1
```

#### V_WRITELANE_B32

Opcode VOP2: 2 (0x2) for GCN 1.0/1.1  
Opcode VOP3A: 258 (0x102) for GCN 1.0/1.1  
Syntax: V_WRITELANE_B32 VDST, VSRC0, SSRC1  
Description: Copy SGPR to one lane of VDST. Lane choosen (thread id) from SSRC1&63.
SSRC1 can be SGPR or M0. Ignores EXEC mask.  
Operation:  
```
VDST[SSRC1 & 63] = SSRC0
```
