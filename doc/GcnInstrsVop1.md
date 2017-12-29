## GCN ISA VOP1/VOP3 instructions

VOP1 instructions can be encoded in the VOP1 encoding and the VOP3A/VOP3B encoding.
List of fields for VOP1 encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-8   | SRC0     | First (scalar or vector) source operand
9-16  | OPCODE   | Operation code
17-24 | VDST     | Destination vector operand
25-31 | ENCODING | Encoding type. Must be 0b0111111

Syntax: INSTRUCTION VDST, SRC0

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

Syntax: INSTRUCTION VDST, SRC0 [MODIFIERS]

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

Unaligned pairs of SGPRs are allowed in source operands.

VOP1 opcodes (0-127) are reflected in VOP3 in range: 384-511 for GCN 1.0/1.1 or
320-447 for GCN 1.2.

List of the instructions by opcode (GCN 1.0/1.1):

 Opcode     | Opcode(VOP3)|GCN 1.0|GCN 1.1| Mnemonic
------------|-------------|-------|-------|-----------------------------
 0 (0x0)    | 384 (0x180) |   ✓   |   ✓   | V_NOP
 1 (0x1)    | 385 (0x181) |   ✓   |   ✓   | V_MOV_B32
 2 (0x2)    | 386 (0x182) |   ✓   |   ✓   | V_READFIRSTLANE_B32
 3 (0x3)    | 387 (0x183) |   ✓   |   ✓   | V_CVT_I32_F64
 4 (0x4)    | 388 (0x184) |   ✓   |   ✓   | V_CVT_F64_I32
 5 (0x5)    | 389 (0x185) |   ✓   |   ✓   | V_CVT_F32_I32
 6 (0x6)    | 390 (0x186) |   ✓   |   ✓   | V_CVT_F32_U32
 7 (0x7)    | 391 (0x187) |   ✓   |   ✓   | V_CVT_U32_F32
 8 (0x8)    | 392 (0x188) |   ✓   |   ✓   | V_CVT_I32_F32
 9 (0x9)    | 393 (0x189) |   ✓   |   ✓   | V_MOV_FED_B32
 10 (0xa)   | 394 (0x18a) |   ✓   |   ✓   | V_CVT_F16_F32
 11 (0xb)   | 395 (0x18b) |   ✓   |   ✓   | V_CVT_F32_F16
 12 (0xc)   | 396 (0x18c) |   ✓   |   ✓   | V_CVT_RPI_I32_F32
 13 (0xd)   | 397 (0x18d) |   ✓   |   ✓   | V_CVT_FLR_I32_F32
 14 (0xe)   | 398 (0x18e) |   ✓   |   ✓   | V_CVT_OFF_F32_I4
 15 (0xf)   | 399 (0x18f) |   ✓   |   ✓   | V_CVT_F32_F64
 16 (0x10)  | 400 (0x190) |   ✓   |   ✓   | V_CVT_F64_F32
 17 (0x11)  | 401 (0x191) |   ✓   |   ✓   | V_CVT_F32_UBYTE0
 18 (0x12)  | 402 (0x192) |   ✓   |   ✓   | V_CVT_F32_UBYTE1
 19 (0x13)  | 403 (0x193) |   ✓   |   ✓   | V_CVT_F32_UBYTE2
 20 (0x14)  | 404 (0x194) |   ✓   |   ✓   | V_CVT_F32_UBYTE3
 21 (0x15)  | 405 (0x195) |   ✓   |   ✓   | V_CVT_U32_F64
 22 (0x16)  | 406 (0x196) |   ✓   |   ✓   | V_CVT_F64_U32
 23 (0x17)  | 407 (0x197) |       |   ✓   | V_TRUNC_F64
 24 (0x18)  | 408 (0x198) |       |   ✓   | V_CEIL_F64
 25 (0x19)  | 409 (0x199) |       |   ✓   | V_RNDNE_F64
 26 (0x1a)  | 410 (0x19a) |       |   ✓   | V_FLOOR_F64
 32 (0x20)  | 416 (0x1a0) |   ✓   |   ✓   | V_FRACT_F32
 33 (0x21)  | 417 (0x1a1) |   ✓   |   ✓   | V_TRUNC_F32
 34 (0x22)  | 418 (0x1a2) |   ✓   |   ✓   | V_CEIL_F32
 35 (0x23)  | 419 (0x1a3) |   ✓   |   ✓   | V_RNDNE_F32
 36 (0x24)  | 420 (0x1a4) |   ✓   |   ✓   | V_FLOOR_F32
 37 (0x25)  | 421 (0x1a5) |   ✓   |   ✓   | V_EXP_F32
 38 (0x26)  | 422 (0x1a6) |   ✓   |   ✓   | V_LOG_CLAMP_F32
 39 (0x27)  | 423 (0x1a7) |   ✓   |   ✓   | V_LOG_F32
 40 (0x28)  | 424 (0x1a8) |   ✓   |   ✓   | V_RCP_CLAMP_F32
 41 (0x29)  | 425 (0x1a9) |   ✓   |   ✓   | V_RCP_LEGACY_F32
 42 (0x2a)  | 426 (0x1aa) |   ✓   |   ✓   | V_RCP_F32
 43 (0x2b)  | 427 (0x1ab) |   ✓   |   ✓   | V_RCP_IFLAG_F32
 44 (0x2c)  | 428 (0x1ac) |   ✓   |   ✓   | V_RSQ_CLAMP_F32
 45 (0x2d)  | 429 (0x1ad) |   ✓   |   ✓   | V_RSQ_LEGACY_F32
 46 (0x2e)  | 430 (0x1ae) |   ✓   |   ✓   | V_RSQ_F32
 47 (0x2f)  | 431 (0x1af) |   ✓   |   ✓   | V_RCP_F64
 48 (0x30)  | 432 (0x1b0) |   ✓   |   ✓   | V_RCP_CLAMP_F64
 49 (0x31)  | 433 (0x1b1) |   ✓   |   ✓   | V_RSQ_F64
 50 (0x32)  | 434 (0x1b2) |   ✓   |   ✓   | V_RSQ_CLAMP_F64
 51 (0x33)  | 435 (0x1b3) |   ✓   |   ✓   | V_SQRT_F32
 52 (0x34)  | 436 (0x1b4) |   ✓   |   ✓   | V_SQRT_F64
 53 (0x35)  | 437 (0x1b5) |   ✓   |   ✓   | V_SIN_F32
 54 (0x36)  | 438 (0x1b6) |   ✓   |   ✓   | V_COS_F32
 55 (0x37)  | 439 (0x1b7) |   ✓   |   ✓   | V_NOT_B32
 56 (0x38)  | 440 (0x1b8) |   ✓   |   ✓   | V_BFREV_B32
 57 (0x39)  | 441 (0x1b9) |   ✓   |   ✓   | V_FFBH_U32
 58 (0x3a)  | 442 (0x1ba) |   ✓   |   ✓   | V_FFBL_B32
 59 (0x3b)  | 443 (0x1bb) |   ✓   |   ✓   | V_FFBH_I32
 60 (0x3c)  | 444 (0x1bc) |   ✓   |   ✓   | V_FREXP_EXP_I32_F64
 61 (0x3d)  | 445 (0x1bd) |   ✓   |   ✓   | V_FREXP_MANT_F64
 62 (0x3e)  | 446 (0x1be) |   ✓   |   ✓   | V_FRACT_F64
 63 (0x3f)  | 447 (0x1bf) |   ✓   |   ✓   | V_FREXP_EXP_I32_F32
 64 (0x40)  | 448 (0x1c0) |   ✓   |   ✓   | V_FREXP_MANT_F32
 65 (0x41)  | 449 (0x1c1) |   ✓   |   ✓   | V_CLREXCP
 66 (0x42)  | 450 (0x1c2) |   ✓   |   ✓   | V_MOVRELD_B32
 67 (0x43)  | 451 (0x1c3) |   ✓   |   ✓   | V_MOVRELS_B32
 68 (0x44)  | 452 (0x1c4) |   ✓   |   ✓   | V_MOVRELSD_B32
 69 (0x45)  | 453 (0x1c5) |       |   ✓   | V_LOG_LEGACY_F32
 70 (0x46)  | 454 (0x1c6) |       |   ✓   | V_EXP_LEGACY_F32

List of the instructions by opcode (GCN 1.2/1.4):

 Opcode     | Opcode(VOP3)| Mnemonic (GCN 1.2)  | Mnemonic (GCN 1.4)
------------|-------------|---------------------|------------------------
 0 (0x0)    | 320 (0x140) | V_NOP               | V_NOP
 1 (0x1)    | 321 (0x141) | V_MOV_B32           | V_MOV_B32
 2 (0x2)    | 322 (0x142) | V_READFIRSTLANE_B32 | V_READFIRSTLANE_B32
 3 (0x3)    | 323 (0x143) | V_CVT_I32_F64       | V_CVT_I32_F64
 4 (0x4)    | 324 (0x144) | V_CVT_F64_I32       | V_CVT_F64_I32
 5 (0x5)    | 325 (0x145) | V_CVT_F32_I32       | V_CVT_F32_I32
 6 (0x6)    | 326 (0x146) | V_CVT_F32_U32       | V_CVT_F32_U32
 7 (0x7)    | 327 (0x147) | V_CVT_U32_F32       | V_CVT_U32_F32
 8 (0x8)    | 328 (0x148) | V_CVT_I32_F32       | V_CVT_I32_F32
 9 (0x9)    | 329 (0x149) | V_MOV_FED_B32       | V_MOV_FED_B32
 10 (0xa)   | 330 (0x14a) | V_CVT_F16_F32       | V_CVT_F16_F32
 11 (0xb)   | 331 (0x14b) | V_CVT_F32_F16       | V_CVT_F32_F16
 12 (0xc)   | 332 (0x14c) | V_CVT_RPI_I32_F32   | V_CVT_RPI_I32_F32
 13 (0xd)   | 333 (0x14d) | V_CVT_FLR_I32_F32   | V_CVT_FLR_I32_F32
 14 (0xe)   | 334 (0x14e) | V_CVT_OFF_F32_I4    | V_CVT_OFF_F32_I4
 15 (0xf)   | 335 (0x14f) | V_CVT_F32_F64       | V_CVT_F32_F64
 16 (0x10)  | 336 (0x150) | V_CVT_F64_F32       | V_CVT_F64_F32
 17 (0x11)  | 337 (0x151) | V_CVT_F32_UBYTE0    | V_CVT_F32_UBYTE0
 18 (0x12)  | 338 (0x152) | V_CVT_F32_UBYTE1    | V_CVT_F32_UBYTE1
 19 (0x13)  | 339 (0x153) | V_CVT_F32_UBYTE2    | V_CVT_F32_UBYTE2
 20 (0x14)  | 340 (0x154) | V_CVT_F32_UBYTE3    | V_CVT_F32_UBYTE3
 21 (0x15)  | 341 (0x155) | V_CVT_U32_F64       | V_CVT_U32_F64
 22 (0x16)  | 342 (0x156) | V_CVT_F64_U32       | V_CVT_F64_U32
 23 (0x17)  | 343 (0x157) | V_TRUNC_F64         | V_TRUNC_F64
 24 (0x18)  | 344 (0x158) | V_CEIL_F64          | V_CEIL_F64
 25 (0x19)  | 345 (0x159) | V_RNDNE_F64         | V_RNDNE_F64
 26 (0x1a)  | 346 (0x15a) | V_FLOOR_F64         | V_FLOOR_F64
 27 (0x1b)  | 347 (0x15b) | V_FRACT_F32         | V_FRACT_F32
 28 (0x1c)  | 348 (0x15c) | V_TRUNC_F32         | V_TRUNC_F32
 29 (0x1d)  | 349 (0x15d) | V_CEIL_F32          | V_CEIL_F32
 30 (0x1e)  | 350 (0x15e) | V_RNDNE_F32         | V_RNDNE_F32
 31 (0x1f)  | 351 (0x15f) | V_FLOOR_F32         | V_FLOOR_F32
 32 (0x20)  | 352 (0x160) | V_EXP_F32           | V_EXP_F32
 33 (0x21)  | 353 (0x161) | V_LOG_F32           | V_LOG_F32
 34 (0x22)  | 354 (0x162) | V_RCP_F32           | V_RCP_F32
 35 (0x23)  | 355 (0x163) | V_RCP_IFLAG_F32     | V_RCP_IFLAG_F32
 36 (0x24)  | 356 (0x164) | V_RSQ_F32           | V_RSQ_F32
 37 (0x25)  | 357 (0x165) | V_RCP_F64           | V_RCP_F64
 38 (0x26)  | 358 (0x166) | V_RSQ_F64           | V_RSQ_F64
 39 (0x27)  | 359 (0x167) | V_SQRT_F32          | V_SQRT_F32
 40 (0x28)  | 360 (0x168) | V_SQRT_F64          | V_SQRT_F64
 41 (0x29)  | 361 (0x169) | V_SIN_F32           | V_SIN_F32
 42 (0x2a)  | 362 (0x16a) | V_COS_F32           | V_COS_F32
 43 (0x2b)  | 363 (0x16b) | V_NOT_B32           | V_NOT_B32
 44 (0x2c)  | 364 (0x16c) | V_BFREV_B32         | V_BFREV_B32
 45 (0x2d)  | 365 (0x16d) | V_FFBH_U32          | V_FFBH_U32
 46 (0x2e)  | 366 (0x16e) | V_FFBL_B32          | V_FFBL_B32
 47 (0x2f)  | 367 (0x16f) | V_FFBH_I32          | V_FFBH_I32
 48 (0x30)  | 368 (0x170) | V_FREXP_EXP_I32_F64 | V_FREXP_EXP_I32_F64
 49 (0x31)  | 369 (0x171) | V_FREXP_MANT_F64    | V_FREXP_MANT_F64
 50 (0x32)  | 370 (0x172) | V_FRACT_F64         | V_FRACT_F64
 51 (0x33)  | 371 (0x173) | V_FREXP_EXP_I32_F32 | V_FREXP_EXP_I32_F32
 52 (0x34)  | 372 (0x174) | V_FREXP_MANT_F32    | V_FREXP_MANT_F32
 53 (0x35)  | 373 (0x175) | V_CLREXCP           | V_CLREXCP
 54 (0x36)  | 374 (0x176) | V_MOVRELD_B32       | V_MOV_PRSV_B32
 55 (0x37)  | 375 (0x177) | V_MOVRELS_B32       | V_SCREEN_PARTITION_4SE_B32
 56 (0x38)  | 376 (0x178) | V_MOVRELSD_B32      | --
 57 (0x39)  | 377 (0x179) | V_CVT_F16_U16       | V_CVT_F16_U16
 58 (0x3a)  | 378 (0x17a) | V_CVT_F16_I16       | V_CVT_F16_I16
 59 (0x3b)  | 379 (0x17b) | V_CVT_U16_F16       | V_CVT_U16_F16
 60 (0x3c)  | 380 (0x17c) | V_CVT_I16_F16       | V_CVT_I16_F16
 61 (0x3d)  | 381 (0x17d) | V_RCP_F16           | V_RCP_F16
 62 (0x3e)  | 382 (0x17e) | V_SQRT_F16          | V_SQRT_F16
 63 (0x3f)  | 383 (0x17f) | V_RSQ_F16           | V_RSQ_F16
 64 (0x40)  | 384 (0x180) | V_LOG_F16           | V_LOG_F16
 65 (0x41)  | 385 (0x181) | V_EXP_F16           | V_EXP_F16
 66 (0x42)  | 386 (0x182) | V_FREXP_MANT_F16    | V_FREXP_MANT_F16
 67 (0x43)  | 387 (0x183) | V_FREXP_EXP_I16_F16 | V_FREXP_EXP_I16_F16
 68 (0x44)  | 388 (0x184) | V_FLOOR_F16         | V_FLOOR_F16
 69 (0x45)  | 389 (0x185) | V_CEIL_F16          | V_CEIL_F16
 70 (0x46)  | 390 (0x186) | V_TRUNC_F16         | V_TRUNC_F16
 71 (0x47)  | 391 (0x187) | V_RNDNE_F16         | V_RNDNE_F16
 72 (0x48)  | 392 (0x188) | V_FRACT_F16         | V_FRACT_F16
 73 (0x49)  | 393 (0x189) | V_SIN_F16           | V_SIN_F16
 74 (0x4a)  | 394 (0x18a) | V_COS_F16           | V_COS_F16
 75 (0x4b)  | 395 (0x18b) | V_EXP_LEGACY_F32    | V_EXP_LEGACY_F32
 76 (0x4c)  | 396 (0x18c) | V_LOG_LEGACY_F32    | V_LOG_LEGACY_F32
 77 (0x4d)  | 397 (0x18d) | --                  | V_CVT_NORM_I16_F16
 78 (0x4e)  | 398 (0x18e) | --                  | V_CVT_NORM_U16_F16
 79 (0x4f)  | 399 (0x18f) | --                  | V_SAT_PK_U8_I16
 80 (0x50)  | 400 (0x190  | --                  | V_WRITELANE_REGWR_B32
 81 (0x51)  | 401 (0x191) | --                  | V_SWAP_B32

### Instruction set

Alphabetically sorted instruction list:

#### V_BFREV_B32

Opcode VOP1: 56 (0x38) for GCN 1.0/1.1; 44 (0x2c) for GCN 1.2  
Opcode VOP3A: 440 (0x1b8) for GCN 1.0/1.1; 364 (0x16c) for GCN 1.2  
Syntax: V_BFREV_B32 VDST, SRC0  
Reverse bits in SRC0 and store result to VDST.  
Operation:  
```
VDST = REVBIT(SRC0)
```

#### V_CEIL_F16

Opcode VOP1: 69 (0x45) for GCN 1.2  
Opcode VOP3A: 389 (0x185) for GCN 1.2  
Syntax: V_CEIL_F16 VDST, SRC0  
Description: Truncate half floating point value from SRC0 with rounding to positive infinity
(ceilling), and store result to VDST. Implemented by flooring.
If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
HALF F = FLOOR(ASHALF(SRC0))
if (ASHALF(SRC0) > 0.0 && ASHALF(SRC0) != F)
    F += 1.0
VDST = F
```

#### V_CEIL_F32

Opcode VOP1: 34 (0x22) for GCN 1.0/1.1; 29 (0x1d) for GCN 1.2  
Opcode VOP3A: 418 (0x1a2) for GCN 1.0/1.1; 349 (0x15d) for GCN 1.2  
Syntax: V_CEIL_F32 VDST, SRC0  
Description: Truncate floating point value from SRC0 with rounding to positive infinity
(ceilling), and store result to VDST. Implemented by flooring.
If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
FLOAT F = FLOOR(ASFLOAT(SRC0))
if (ASFLOAT(SRC0) > 0.0 && ASFLOAT(SRC0) != F)
    F += 1.0
VDST = F
```

#### V_CEIL_F64

Opcode VOP1: 24 (0x18) for GCN 1.1/1.2  
Opcode VOP3A: 408 (0x198) for GCN 1.1; 344 (0x158) for GCN 1.2  
Syntax: V_CEIL_F64 VDST(2), SRC0(2)  
Description: Truncate double floating point value from SRC0 with rounding to
positive infinity (ceilling), and store result to VDST. Implemented by flooring.
If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
DOUBLE F = FLOOR(ASDOUBLE(SRC0))
if (ASDOUBLE(SRC0) > 0.0 && ASDOUBLE(SRC0) != F)
    F += 1.0
VDST = F
```

#### V_CLREXCP

Opcode VOP1: 65 (0x41) for GCN 1.0/1.1; 53 (0x35) for GCN 1.2  
Opcode VOP3A: 449 (0x1c1) for GCN 1.0/1.1; 373 (0x175) for GCN 1.2  
Syntax: V_CLREXCP  
Description: Clear wave's exception state in SIMD.  

#### V_COS_F16

Opcode VOP1: 74 (0x4a) for GCN 1.2  
Opcode VOP3A: 394 (0x18a) for GCN 1.2  
Syntax: V_COS_F16 VDST, SRC0  
Description: Compute cosine of half FP value from SRC0.
Input value must be normalized to range 1.0 - 1.0 (-360 degree : 360 degree).
If SRC0 value is out of range then store 1.0 to VDST.
If SRC0 value is infinity, store -NAN to VDST.  
Operation:  
```
FLOAT SF = ASHALF(SRC0)
VDST = 1.0
if (SF >= -1.0 && SF <= 1.0)
    VDST = APPROX_COS(SF)
else if (ABS(SF)==INF_H)
    VDST = -NAN_H
else if (ISNAN(SF))
    VDST = SRC0
```

#### V_COS_F32

Opcode VOP1: 54 (0x36) for GCN 1.0/1.1; 42 (0x2a) for GCN 1.2  
Opcode VOP3A: 438 (0x1b6) for GCN 1.0/1.1; 362 (0x16a) for GCN 1.2  
Syntax: V_COS_F32 VDST, SRC0  
Description: Compute cosine of FP value from SRC0. Input value must be normalized to range
1.0 - 1.0 (-360 degree : 360 degree). If SRC0 value is out of range then store 1.0 to VDST.
If SRC0 value is infinity, store -NAN to VDST.  
Operation:  
```
FLOAT SF = ASFLOAT(SRC0)
VDST = 1.0
if (SF >= -1.0 && SF <= 1.0)
    VDST = APPROX_COS(SF)
else if (ABS(SF)==INF)
    VDST = -NAN
else if (ISNAN(SF))
    VDST = SRC0
```

#### V_CVT_F16_F32

Opcode VOP1: 10 (0xa)  
Opcode VOP3A: 394 (0x18a) for GCN 1.0/1.1; 330 (0x14a) for GCN 1.2  
Syntax: V_CVT_F16_F32 VDST, SRC0  
Description: Convert single FP value to half floating point value with rounding from
MODE register (single FP rounding mode for GCN 1.0, double FP rounding modefor GCN 1.2),
and store result to VDST. If absolute value is too high, then store -/+infinity to VDST.
In GCN 1.2 flushing denormals controlled by MODE. In GCN 1.0/1.1, denormals are enabled.  
Operation:  
```
VDST = CVTHALF(ASFLOAT(SRC0))
```

#### V_CVT_F16_I16

Opcode: VOP1: 58 (0x3a) for GCN 1.2  
Opcode VOP3A: 378 (0x17a) for GCN 1.2  
Syntax: V_CVT_F16_I16 VDST, SRC0  
Description: Convert 16-bit signed valut to half floating point value.  
Operation:  
```
VDST = (HALF)(INT16)SRC0
```

#### V_CVT_F16_U16

Opcode: VOP1: 57 (0x39) for GCN 1.2  
Opcode VOP3A: 377 (0x179) for GCN 1.2  
Syntax: V_CVT_F16_U16 VDST, SRC0  
Description: Convert 16-bit unsigned valut to half floating point value.  
Operation:  
```
VDST = (HALF)(SRC0&0xffff)
```

#### V_CVT_F32_F16

Opcode VOP1: 11 (0xb)  
Opcode VOP3A: 395 (0x18b) for GCN 1.0/1.1; 331 (0x14b) for GCN 1.2  
Syntax: V_CVT_F32_F16 VDST, SRC0  
Description: Convert half FP value to single FP value, and store result to VDST.
**By default, immediate is in FP32 format!**.
In GCN 1.2 flushing denormals controlled by MODE. In GCN 1.0/1.1, denormals are enabled.  
Operation:  
```
VDST = (FLOAT)(ASHALF(SRC0))
```

#### V_CVT_F32_F64

Opcode VOP1: 15 (0xf)  
Opcode VOP3A: 399 (0x18f) for GCN 1.0/1.1; 335 (0x14f) for GCN 1.2  
Syntax: V_CVT_F32_F64 VDST, SRC0(2)  
Description: Convert double FP value to single floating point value with rounding from
MODE register (single FP rounding mode), and store result to VDST.
If absolute value is too high, then store -/+infinity to VDST.  
Operation:  
```
VDST = CVTHALF(ASDOUBLE(SRC0))
```

#### V_CVT_F32_I32

Opcode VOP1: 5 (0x5)  
Opcode VOP3A: 389 (0x185) for GCN 1.0/1.1; 325 (0x145) for GCN 1.2  
Syntax: V_CVT_F32_I32 VDST, SRC0  
Description: Convert signed 32-bit integer to single FP value, and store it to VDST.  
Operation:  
```
VDST = (FLOAT)(INT32)SRC0
```

#### V_CVT_F32_U32

Opcode VOP1: 6 (0x6)  
Opcode VOP3A: 390 (0x186) for GCN 1.0/1.1; 326 (0x146) for GCN 1.2  
Syntax: V_CVT_F32_U32 VDST, SRC0  
Description: Convert unsigned 32-bit integer to single FP value, and store it to VDST.  
Operation:  
```
VDST = (FLOAT)SRC0
```

#### V_CVT_F32_UBYTE0

Opcode VOP1: 17 (0x11)  
Opcode VOP3A: 401 (0x191) for GCN 1.0/1.1; 337 (0x151) for GCN 1.2  
Syntax: V_CVT_F32_UBYTE0 VDST, SRC0  
Description: Convert the first unsigned 8-bit byte from SRC0 to single FP value,
and store it to VDST.  
Operation:  
```
VDST = (FLOAT)(SRC0 & 0xff)
```

#### V_CVT_F32_UBYTE1

Opcode VOP1: 18 (0x12)  
Opcode VOP3A: 402 (0x192) for GCN 1.0/1.1; 338 (0x152) for GCN 1.2  
Syntax: V_CVT_F32_UBYTE1 VDST, SRC0  
Description: Convert the second unsigned 8-bit byte from SRC0 to single FP value,
and store it to VDST.  
Operation:  
```
VDST = (FLOAT)((SRC0>>8) & 0xff)
```

#### V_CVT_F32_UBYTE2

Opcode VOP1: 19 (0x13)  
Opcode VOP3A: 403 (0x193) for GCN 1.0/1.1; 339 (0x153) for GCN 1.2  
Syntax: V_CVT_F32_UBYTE2 VDST, SRC0  
Description: Convert the third unsigned 8-bit byte from SRC0 to single FP value,
and store it to VDST.  
Operation:  
```
VDST = (FLOAT)((SRC0>>16) & 0xff)
```

#### V_CVT_F32_UBYTE3

Opcode VOP1: 20 (0x14)  
Opcode VOP3A: 404 (0x194) for GCN 1.0/1.1; 340 (0x154) for GCN 1.2  
Syntax: V_CVT_F32_UBYTE3 VDST, SRC0  
Description: Convert the fourth unsigned 8-bit byte from SRC0 to single FP value,
and store it to VDST.  
Operation:  
```
VDST = (FLOAT)(SRC0>>24)
```

#### V_CVT_F64_F32

Opcode VOP1: 16 (0x10)  
Opcode VOP3A: 400 (0x190) for GCN 1.0/1.1; 336 (0x150) for GCN 1.2  
Syntax: V_CVT_F64_F32 VDST(2), SRC0  
Description: Convert single FP value to double FP value, and store result to VDST.  
Operation:  
```
VDST = (DOUBLE)(ASFLOAT(SRC0))
```

#### V_CVT_F64_I32

Opcode VOP1: 4 (0x4)  
Opcode VOP3A: 388 (0x184) for GCN 1.0/1.1; 324 (0x144) for GCN 1.2  
Syntax: V_CVT_F64_I32 VDST(2), SRC0  
Description: Convert signed 32-bit integer to double FP value, and store it to VDST.  
Operation:  
```
VDST = (DOUBLE)(INT32)SRC0
```

#### V_CVT_F64_U32

Opcode VOP1: 22 (0x16)  
Opcode VOP3A: 406 (0x196) for GCN 1.0/1.1; 342 (0x156) for GCN 1.2  
Syntax: V_CVT_F64_U32 VDST(2), SRC0  
Description: Convert unsigned 32-bit integer to double FP value, and store it to VDST.  
Operation:  
```
VDST = (DOUBLE)SRC0
```

#### V_CVT_FLR_I32_F32

Opcode VOP1: 13 (0xd)  
Opcode VOP3A: 397 (0x18d) for GCN 1.0/1.1; 333 (0x14d) for GCN 1.2  
Syntax: V_CVT_FLR_I32_F32 VDST, SRC0  
Description: Convert 32-bit floating point value from SRC0 to signed 32-bit integer, and
store result to VDST. Conversion uses rounding to negative infinity (floor).
If value is higher/lower than maximal/minimal integer then store MAX_INT32/MIN_INT32 to VDST.
If input value is NaN/-NaN then store MAX_INT32/MIN_INT32 to VDST.  
Operation:  
```
FLOAT SF = ASFLOAT(SF)
if (!ISNAN(SF))
    VDST = (INT32)MAX(MIN(FLOOR(SF), 2147483647.0), -2147483648.0)
else
    VDST = (INT32)SF>=0 ? 2147483647 : -2147483648
```

#### V_CVT_I16_F16

Opcode VOP1: 60 (0x3c)  
Opcode VOP3A: 380 (0x17c) for GCN 1.2  
Syntax: V_CVT_I16_F16 VDST, SRC0  
Description: Convert 16-bit floating point value from SRC0 to signed 16-bit integer, and
store result to VDST. Conversion uses rounding to zero. If value is higher/lower than
maximal/minimal integer then store MAX_INT16/MIN_INT16 to VDST.
If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (!ISNAN(ASHALF(SRC0)))
    VDST = (INT16)MAX(MIN(RNDTZINT(ASHALF(SRC0)), 32767.0), -32768.0)
```

#### V_CVT_I32_F32

Opcode VOP1: 8 (0x8)  
Opcode VOP3A: 392 (0x188) for GCN 1.0/1.1; 328 (0x148) for GCN 1.2  
Syntax: V_CVT_I32_F32 VDST, SRC0  
Description: Convert 32-bit floating point value from SRC0 to signed 32-bit integer, and
store result to VDST. Conversion uses rounding to zero. If value is higher/lower than
maximal/minimal integer then store MAX_INT32/MIN_INT32 to VDST.
If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (!ISNAN(ASFLOAT(SRC0)))
    VDST = (INT32)MAX(MIN(RNDTZINT(ASFLOAT(SRC0)), 2147483647.0), -2147483648.0)
```

#### V_CVT_I32_F64

Opcode VOP1: 3 (0x3)  
Opcode VOP3A: 387 (0x183) for GCN 1.0/1.1; 323 (0x143) for GCN 1.2  
Syntax: V_CVT_I32_F64 VDST, SRC0(2)  
Description: Convert 64-bit floating point value from SRC0 to signed 32-bit integer, and
store result to VDST. Conversion uses rounding to zero. If value is higher/lower than
maximal/minimal integer then store MAX_INT32/MIN_INT32 to VDST.
If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (!ISNAN(ASDOUBLE(SRC0)))
    VDST = (INT32)MAX(MIN(RNDTZINT(ASDOUBLE(SRC0)), 2147483647.0), -2147483648.0)
```

#### V_CVT_NORM_I16_F16

Opcode VOP1: 77 (0x4d) for GCN 1.4  
Opcode VOP3A: 397 (0x18d) for GCN 1.4  
Syntax: V_CVT_NORM_I16_F16 VDST, SRC0(2)  
Description: Convert 16-bit floating point value from SRC0 to signed normalized 16-bit value
by multiplying value by 32768.0 and make conversion to 16-bit signed integer, and
store result to VDST. Conversion depends on rounding mode.  
```
VDST = 0
if (!ISNAN(ASHALF(SRC0)))
    VDST = (INT16)(MAX(MIN(RNDINT(ASHALF(SRC0*32767.0)), 32767.0, -32767.0)))
```

#### V_CVT_NORM_U16_F16

Opcode VOP1: 78 (0x4e) for GCN 1.4  
Opcode VOP3A: 398 (0x18e) for GCN 1.4  
Syntax: V_CVT_NORM_U16_F16 VDST, SRC0(2)  
Description: Convert 16-bit floating point value from SRC0 to unsigned normalized
16-bit value by multiplying value by 65535.0 and make conversion to
16-bit unsigned integer, and store result to VDST. Probably rounds to +Infinity.  
```
VDST = 0
if (!ISNAN(ASHALF(SRC0)))
    VDST = (UINT16)(MAX(MIN(RNDINT(ASHALF(SRC0*65535.0)), 65535.0, 0.0)))
```

#### V_CVT_OFF_F32_I4

Opcode VOP1: 14 (0xe)  
Opcode VOP3A: 398 (0x18e) for GCN 1.0/1.1; 334 (0x14e) for GCN 1.2  
Syntax: V_CVT_OFF_F32_I4 VDST, SRC0  
Description: Convert 4-bit signed value from SRC0 to floating point value, normalize that
value to range -0.5:0.4375 and store result to VDST.  
Operation:  
```
VDST = (FLOAT)((SRC0 & 0xf) ^ 8) / 16.0 - 0.5
```

#### V_CVT_RPI_I32_F32

Opcode VOP1: 12 (0xc)  
Opcode VOP3A: 396 (0x18c) for GCN 1.0/1.1; 332 (0x14c) for GCN 1.2  
Syntax: V_CVT_RPI_I32_F32 VDST, SRC0  
Description: Convert 32-bit floating point value from SRC0 to signed 32-bit integer, and
store result to VDST. Conversion adds 0.5 to value and rounds negative infinity (floor).
If value is higher/lower than maximal/minimal integer then store MAX_INT32/MIN_INT32 to
VDST. If input value is NaN/-NaN then store MAX_INT32/MIN_INT32 to VDST.  
Operation:  
```
FLOAT SF = ASFLOAT(SRC0)
if (!ISNAN(SF))
    VDST = (INT32)MAX(MIN(FLOOR(SF + 0.5), 2147483647.0), -2147483648.0)
else
    VDST = (INT32)SF>=0 ? 2147483647 : -2147483648
```

#### V_CVT_U16_F16

Opcode VOP1: 59 (0x3b) for GCN 1.2  
Opcode VOP3A: 379 (0x17b) for GCN 1.2  
Syntax: V_CVT_U16_F16 VDST, SRC0  
Description: Convert 32-bit half floating point value from SRC0 to unsigned 16-bit integer,
and store result to VDST. Conversion uses rounding to zero. If value is higher than
maximal integer then store MAX_UINT16 to VDST. If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (!ISNAN(ASHALF(SRC0)))
    VDST = (UINT16)MIN(RNDTZINT(ASHALF(SRC0)), 65535.0)
```


#### V_CVT_U32_F32

Opcode VOP1: 7 (0x7)  
Opcode VOP3A: 391 (0x187) for GCN 1.0/1.1; 327 (0x147) for GCN 1.2  
Syntax: V_CVT_U32_F32 VDST, SRC0  
Description: Convert 32-bit floating point value from SRC0 to unsigned 32-bit integer, and
store result to VDST. Conversion uses rounding to zero. If value is higher than
maximal integer then store MAX_UINT32 to VDST.
If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (!ISNAN(ASFLOAT(SRC0)))
    VDST = (UINT32)MIN(RNDTZINT(ASFLOAT(SRC0)), 4294967295.0)
```

#### V_CVT_U32_F64

Opcode VOP1: 21 (0x15)  
Opcode VOP3A: 405 (0x195) for GCN 1.0/1.1; 341 (0x155) for GCN 1.2  
Syntax: V_CVT_U32_F64 VDST, SRC0(2)  
Description: Convert 64-bit floating point value from SRC0 to unsigned 32-bit integer, and
store result to VDST. Conversion uses rounding to zero. If value is higher than
maximal integer then store MAX_UINT32 to VDST.
If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (!ISNAN(ASDOUBLE(SRC0)))
    VDST = (UINT32)MIN(RNDTZINT(ASDOUBLE(SRC0)), 4294967295.0)
```

#### V_EXP_F16

Opcode VOP1: 65 (0x41) for GCN 1.2  
Opcode VOP3A: 385 (0x181) for GCN 1.2  
Syntax: V_EXP_F16 VDST, SRC0  
Description: Approximate power of two from half FP value SRC0 and store it to VDST.  
Operation:  
```
VDST = APPROX_POW2(ASHALF(SRC0))
```

#### V_EXP_F32

Opcode VOP1: 37 (0x25) for GCN 1.0/1.1; 32 (0x20) for GCN 1.2  
Opcode VOP3A: 421 (0x1a5) for GCN 1.0/1.1; 352 (0x160) for GCN 1.2  
Syntax: V_EXP_F32 VDST, SRC0  
Description: Approximate power of two from FP value SRC0 and store it to VDST. Instruction
for values smaller than -126.0 always returns 0 regardless floatmode in MODE register.  
Operation:  
```
if (ASFLOAT(SRC0)>=-126.0)
    VDST = APPROX_POW2(ASFLOAT(SRC0))
else
    VDST = 0.0
```

### V_EXP_LEGACY_F32

Opcode VOP1: 70 (0x46) for GCN 1.1; 75 (0x4b) for GCN 1.2  
Opcode VOP3A: 454 (0x1c6) for GCN 1.1; 395 (0x18b) for GCN 1.2  
Syntax: V_EXP_LEGACY_F32 VDST, SRC0  
Description: Approximate power of two from FP value SRC0 and store it to VDST. Instruction
for values smaller than -126.0 always returns 0 regardless floatmode in MODE register.
For some cases this instructions returns slightly less accurate result than V_EXP_F32.  
Operation:  
```
if (ASFLOAT(SRC0)>=-126.0)
    VDST = APPROX_POW2(ASFLOAT(SRC0))
else
    VDST = 0.0
```

#### V_FFBH_U32

Opcode VOP1: 57 (0x39) for GCN 1.0/1.1; 45 (0x2d) for GCN 1.2  
Opcode VOP3A: 441 (0x1b9) for GCN 1.0/1.1; 365 (0x16d) for GCN 1.2  
Syntax: V_FFBH_U32 VDST, SRC0  
Description: Find last one bit in SRC0. If found, store number of skipped bits to VDST,
otherwise set VDST to -1.  
Operation:  
```
VDST = -1
for (INT8 i = 31; i >= 0; i--)
    if ((1U<<i) & SRC0) != 0)
    { VDST = 31-i; break; }
```

#### V_FFBH_I32

Opcode VOP1: 59 (0x3b) for GCN 1.0/1.1; 47 (0x2f) for GCN 1.2  
Opcode VOP3A: 443 (0x1bb) for GCN 1.0/1.1; 367 (0x16f) for GCN 1.2  
Syntax: V_FFBH_I32 VDST, SRC0  
Description: Find last opposite bit to sign in SRC0. If found, store number of skipped bits
to VDST, otherwise set VDST to -1.  
Operation:  
```
VDST = -1
UINT32 bitval = (INT32)SRC0>=0 ? 1 : 0
for (INT8 i = 31; i >= 0; i--)
    if ((1U<<i) & SRC0) == (bitval<<i))
    { VDST = 31-i; break; }
```

#### V_FFBL_B32

Opcode VOP1: 58 (0x3a) for GCN 1.0/1.1; 46 (0x2e) for GCN 1.2  
Opcode VOP3A: 442 (0x1ba) for GCN 1.0/1.1; 366 (0x16e) for GCN 1.2  
Syntax: V_FFBL_B32 VDST, SRC0  
Description: Find first one bit in SRC0. If found, store number of bit to VDST,
otherwise set VDST to -1.  
Operation:  
```
VDST = -1
for (UINT8 i = 0; i < 32; i++)
    if ((1U<<i) & SRC0) != 0)
    { VDST = i; break; }
```

#### V_FLOOR_F16

Opcode VOP1: 68 (0x44) for GCN 1.2  
Opcode VOP3A: 388 (0x184) for GCN 1.2  
Syntax: V_FLOOR_F16 VDST, SRC0  
Description: Truncate half floating point value SRC0 with rounding to negative infinity
(flooring), and store result to VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = FLOOR(ASHALF(SRC0))
```

#### V_FLOOR_F32

Opcode VOP1: 36 (0x24) for GCN 1.0/1.1; 31 (0x1f) for GCN 1.2  
Opcode VOP3A: 420 (0x1a4) for GCN 1.0/1.1; 351 (0x15f) for GCN 1.2  
Syntax: V_FLOOR_F32 VDST, SRC0  
Description: Truncate floating point value SRC0 with rounding to negative infinity
(flooring), and store result to VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = FLOOR(ASFLOAT(SRC0))
```

#### V_FLOOR_F64

Opcode VOP1: 26 (0x1a) for GCN 1.1/1.2  
Opcode VOP3A: 410 (0x19a) for GCN 1.1; 346 (0x15a) for GCN 1.2  
Syntax: V_FLOOR_F64 VDST(2), SRC0(2)  
Description: Truncate double floating point value SRC0 with rounding to negative infinity
(flooring), and store result to VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = FLOOR(ASDOUBLE(SRC0))
```

#### V_FRACT_F32

Opcode VOP1: 32 (0x20) for GCN 1.0/1.1; 27 (0x1b) for GCN 1.2  
Opcode VOP3A: 416 (0x1a0) for GCN 1.0/1.1; 347 (0x15b) for GCN 1.2  
Syntax: V_FRACT_F32 VDST, SRC0  
Description: Get fractional from floating point value SRC0 and store it to VDST.
Fractional will be computed by subtracting floor(SRC0) from SRC0.
If SRC0 is infinity or NaN then NaN with proper sign is stored to VDST.  
Operation:  
```
FLOAT SF = ASFLOAT(SRC0)
if (!ISNAN(SF) && SF!=-INF && SF!=INF)
    VDST = SF - FLOOR(ASFLOAT(SF))
else
    VDST = NAN * SIGN(SF)
```

#### V_FRACT_F64

Opcode VOP1: 62 (0x3e) for GCN 1.0/1.1; 52 (0x32) for GCN 1.2  
Opcode VOP3A: 446 (0x1be) for GCN 1.0/1.1; 372 (0x172) for GCN 1.2  
Syntax: V_FRACT_F64 VDST(2), SRC0(2)  
Description: Get fractional from double floating point value SRC0 and store it to VDST.
Fractional will be computed by subtracting floor(SRC0) from SRC0.
If SRC0 is infinity or NaN then NaN with proper sign is stored to VDST.  
Operation:  
```
FLOAT SD = ASDOUBLE(SRC0)
if (!ISNAN(SD) && SD!=-INF && SD!=INF)
    VDST = SD - FLOOR(ASDOUBLE(SD))
else
    VDST = NAN * SIGN(SD)
```

#### V_FREXP_EXP_I16_F16

Opcode VOP1: 67 (0x43) for GCN 1.2  
Opcode VOP3A: 387 (0x183) for GCN 1.2  
Syntax: V_FREXP_EXP_I16_F16 VDST, SRC0  
Description: Get exponent plus 1 from half FP value SRC0, and store that exponent to VDST
as 16-bit signed integer. This instruction realizes frexp function.
If SRC0 is infinity or NAN then store 0 to VDST.  
Operation:  
```
HALF SF = ASHALF(SRC0)
if (ABS(SF) != INF_H && !ISNAN(SF))
    VDST = (INT16)FREXP_EXP(SF)
else
    VDST = 0
```

#### V_FREXP_EXP_I32_F32

Opcode VOP1: 63 (0x3f) for GCN 1.0/1.1; 51 (0x33) for GCN 1.2  
Opcode VOP3A: 447 (0x1bf) for GCN 1.0/1.1; 371 (0x173) for GCN 1.2  
Syntax: V_FREXP_EXP_I32_F32 VDST, SRC0  
Description: Get exponent plus 1 from single FP value SRC0, and store that exponent to VDST.
This instruction realizes frexp function.
If SRC0 is infinity or NAN then store -1 if GCN 1.0 or 0 to VDST.  
Operation:  
```
FLOAT SF = ASFLOAT(SRC0)
if (ABS(SF) != INF && !ISNAN(SF))
    VDST = FREXP_EXP(SF)
else
    VDST = -1 // GCN 1.0
    VDST = 0 // later
```

#### V_FREXP_EXP_I32_F64

Opcode VOP1: 60 (0x3c) for GCN 1.0/1.1; 48 (0x30) for GCN 1.2  
Opcode VOP3A: 444 (0x1bc) for GCN 1.0/1.1; 368 (0x170) for GCN 1.2  
Syntax: V_FREXP_EXP_I32_F64 VDST, SRC0(2)  
Description: Get exponent plus 1 from double FP value SRC0, and store that exponent to VDST.
This instruction realizes frexp function.
If SRC0 is infinity or NAN then store -1 if GCN 1.0 or 0 to VDST.  
Operation:  
```
DOUBLE SD = ASDOUBLE(SRC0)
if (ABS(SD) != INF && !ISNAN(SD))
    VDST = FREXP_EXP(SD)
else
    VDST = -1 // GCN 1.0
    VDST = 0 // later
```

#### V_FREXP_MANT_F16

Opcode VOP1: 66 (0x42) for GCN 1.2  
Opcode VOP3A: 386 (0x182) for GCN 1.2  
Syntax: V_FREXP_MANT_F16 VDST, SRC0  
Description: Get mantisa from half FP value SRC0, and store it to VDST. Mantisa includes
sign of input.  
Operation:  
```
HALF SF = ASHALF(SRC0)
if (ABS(SF) == INF)
    VDST = SF
else if (!ISNAN(SF))
    VDST = FREXP_MANT(SF) * SIGN(SF)
else
    VDST = NAN_H * SIGN(SF)
```

#### V_FREXP_MANT_F32

Opcode VOP1: 64 (0x40) for GCN 1.0/1.1; 52 (0x34) for GCN 1.2  
Opcode VOP3A: 448 (0x1c0) for GCN 1.0/1.1; 372 (0x174) for GCN 1.2  
Syntax: V_FREXP_MANT_F32 VDST, SRC0  
Description: Get mantisa from single FP value SRC0, and store it to VDST. Mantisa includes
sign of input. For GCN 1.0, if SRC0 is infinity then store -NAN to VDST.  
Operation:  
```
FLOAT SF = ASFLOAT(SRC0)
if (ABS(SF) == INF)
    VDST = -NAN // GCN 1.0
    VDST = SF // later
else if (!ISNAN(SF))
    VDST = FREXP_MANT(SF) * SIGN(SF)
else
    VDST = NAN * SIGN(SF)
```

#### V_FREXP_MANT_F64

Opcode VOP1: 61 (0x3d) for GCN 1.0/1.1; 49 (0x31) for GCN 1.2  
Opcode VOP3A: 445 (0x1bd) for GCN 1.0/1.1; 369 (0x171) for GCN 1.2  
Syntax: V_FREXP_MANT_F64 VDST(2), SRC0(2)  
Description: Get mantisa from double FP value SRC0, and store it to VDST. Mantisa includes
sign of input. If SRC0 is infinity then store -NAN to VDST.  
Operation:  
```
DOUBLE SD = ASDOUBLE(SRC0)
if (ABS(SD) == INF)
    VDST = -NAN // GCN 1.0
    VDST = SF // later
else if (!ISNAN(SD))
    VDST = FREXP_MANT(SD) * SIGN(SD)
else
    VDST = NAN * SIGN(SD)
```

#### V_LOG_CLAMP_F32

Opcode VOP1: 38 (0x26) for GCN 1.0/1.1  
Opcode VOP3A: 422 (0x1a6) for GCN 1.0/1.1  
Syntax: V_LOG_CLAMP_F32 VDST, SRC0  
Description: Approximate logarithm of the base 2 from floating point value SRC0 with
clamping infinities to -MAX_FLOAT. Result is stored in VDST.
If SRC0 is negative then store -NaN to VDST. This instruction doesn't handle denormalized
values regardless FLOAT MODE register setup.  
Operation:  
```
FLOAT F = ASFLOAT(SRC0)
if (F==1.0)
    VDST = 0.0f
if (F<0.0)
    VDST = -NaN
else
{
    VDST = APPROX_LOG2(F)
    if (ASFLOAT(VDST)==-INF)
        VDST = -MAX_FLOAT
}
```

#### V_LOG_F16

Opcode VOP1: 64 (0x40) for GCN 1.2  
Opcode VOP3A: 384 (0x180) for GCN 1.2  
Syntax: V_LOG_F16 VDST, SRC0  
Description: Approximate logarithm of the base 2 from half floating point value SRC0,
and store result to VDST. If SRC0 is negative then store -NaN to VDST.  
Operation:  
```
HALF F = ASHALF(SRC0)
if (F==1.0)
    VDST = 0.0h
if (F<0.0)
    VDST = -NaN_F
else
    VDST = APPROX_LOG2(F)
```

#### V_LOG_F32

Opcode VOP1: 39 (0x27) for GCN 1.0/1.1; 33 (0x21) for GCN 1.2  
Opcode VOP3A: 423 (0x1a7) for GCN 1.0/1.1; 353 (0x161) for GCN 1.2  
Syntax: V_LOG_F32 VDST, SRC0  
Description: Approximate logarithm of base the 2 from floating point value SRC0, and store
result to VDST. If SRC0 is negative then store -NaN to VDST.
This instruction doesn't handle denormalized values regardless FLOAT MODE register setup.  
Operation:  
```
FLOAT F = ASFLOAT(SRC0)
if (F==1.0)
    VDST = 0.0f
if (F<0.0)
    VDST = -NaN
else
    VDST = APPROX_LOG2(F)
```

#### V_LOG_LEGACY_F32

Opcode VOP1: 69 (0x45) for GCN 1.1; 76 (0x4c) for GCN 1.2  
Opcode VOP3A: 453 (0x1c5) for GCN 1.1; 396 (0x18c) for GCN 1.2  
Syntax: V_LOG_LEGACY_F32 VDST, SRC0  
Description: Approximate logarithm of the base 2 from floating point value SRC0, and store
result to VDST. If SRC0 is negative then store -NaN to VDST.
This instruction doesn't handle denormalized values regardless FLOAT MODE register setup.
This instruction returns slightly different results than V_LOG_F32.  
Operation:  
```
FLOAT F = ASFLOAT(SRC0)
if (F==1.0)
    VDST = 0.0f
if (F<0.0)
    VDST = -NaN
else
    VDST = APPROX_LOG2(F)
```

#### V_MOV_B32

Opcode VOP1: 1 (0x1)  
Opcode VOP3A: 385 (0x181) for GCN 1.0/1.1; 321 (0x141) for GCN 1.2  
Syntax: V_MOV_B32 VDST, SRC0  
Description: Move SRC0 into VDST.  
Operation:  
```
VDST = SRC0
```

#### V_MOV_FED_B32

Opcode VOP1: 9 (0x9)  
Opcode VOP3A: 393 (0x189) for GCN 1.0/1.1; 329 (0x149) for GCN 1.2  
Syntax: V_MOV_FED_B32 VDST, SRC0  
Description: Introduce edc double error upon write to dest vgpr without causing an exception
(???).

#### V_MOVRELD_B32

Opcode VOP1: 66 (0x42) for GCN 1.0/1.1; 54 (0x34) for GCN 1.2  
Opcode VOP3A: 450 (0x1c2) for GCN 1.0/1.1; 374 (0x174) for GCN 1.2  
Syntax: V_MOVRELD_B32 VDST, VSRC0  
Description: Move SRC0 to VGPR[VDST_NUMBER+M0].  
Operation:  
```
VGPR[VDST_NUMBER+M0] = SRC0
```

#### V_MOVRELS_B32

Opcode VOP1: 67 (0x43) for GCN 1.0/1.1; 55 (0x35) for GCN 1.2  
Opcode VOP3A: 451 (0x1c3) for GCN 1.0/1.1; 375 (0x175) for GCN 1.2  
Syntax: V_MOVRELS_B32 VDST, VSRC0  
Description: Move SRC0[SRC0_NUMBER+M0] to VDST.  
Operation:  
```
VDST = VGPR[SRC0_NUMBER+M0]
```

#### V_MOVRELSD_B32

Opcode VOP1: 68 (0x44) for GCN 1.0/1.1; 56 (0x36) for GCN 1.2  
Opcode VOP3A: 452 (0x1c4) for GCN 1.0/1.1; 376 (0x176) for GCN 1.2  
Syntax: V_MOVRELSD_B32 VDST, VSRC0  
Description: Move SRC0[SRC0_NUMBER+M0] to VGPR[VDST_NUMBER+M0].  
Operation:  
```
VGPR[VDST_NUMBER+M0] = VGPR[SRC0_NUMBER+M0]
```

#### V_NOP

Opcode VOP1: 0 (0x0)  
Opcode VOP3A: 384 (0x180) for GCN 1.0/1.1; 320 (0x140) for GCN 1.2  
Syntax: V_NOP  
Description: Do nothing.

#### V_NOT_B32

Opcode VOP1: 55 (0x37) for GCN 1.0/1.1; 43 (0x2b) for GCN 1.2  
Opcode VOP3A: 439 (0x1b7) for GCN 1.0/1.1; 363 (0x16b) for GCN 1.2  
Syntax: V_NOT_B32 VDST, SRC0  
Description: Do bitwise negation on 32-bit SRC0, and store result to VDST.  
Operation:  
```
VDST = ~SRC0
```

#### V_RCP_CLAMP_F32

Opcode VOP1: 40 (0x28) for GCN 1.0/1.1  
Opcode VOP3A: 424 (0x1a8) for GCN 1.0/1.1  
Syntax: V_RCP_CLAMP_F32 VDST, SRC0  
Description: Approximate reciprocal from floating point value SRC0 and store it to VDST.
Guaranted error below 1ulp. Result is clamped to MAX_FLOAT including sign of a result.  
Operation:  
```
VDST = APPROX_RCP(ASFLOAT(SRC0))
if (ABS(ASFLOAT(VDST))==INF)
    VDST = SIGN(ASFLOAT(VDST)) * MAX_FLOAT
```

#### V_RCP_CLAMP_F64

Opcode VOP1: 48 (0x30) for GCN 1.0/1.1  
Opcode VOP3A: 432 (0x1b0) for GCN 1.0/1.1  
Syntax: V_RCP_CLAMP_F64 VDST(2), SRC0(2)  
Description: Approximate reciprocal from double FP value SRC0 and store it to VDST.
Relative error of approximation is ~1e-8.
Result is clamped to MAX_DOUBLE value including sign of a result.  
Operation:  
```
VDST = APPROX_RCP(ASDOUBLE(SRC0))
if (ABS(ASDOUBLE(VDST))==INF)
    VDST = SIGN(ASDOUBLE(VDST)) * MAX_DOUBLE
```

#### V_RCP_F16

Opcode VOP1: 61 (0x3d) for GCN 1.2  
Opcode VOP3A: 381 (0x17d) for GCN 1.2  
Syntax: V_RCP_F16 VDST, SRC0  
Description: Approximate reciprocal from half floating point value SRC0 and
store it to VDST. Guaranted error below 1ulp.  
Operation:  
```
VDST = APPROX_RCP(ASHALF(SRC0))
```

#### V_RCP_F32

Opcode VOP1: 42 (0x2a) for GCN 1.0/1.1; 34 (0x22) for GCN 1.2  
Opcode VOP3A: 426 (0x1aa) for GCN 1.0/1.1; 354 (0x162) for GCN 1.2  
Syntax: V_RCP_F32 VDST, SRC0  
Description: Approximate reciprocal from floating point value SRC0 and store it to VDST.
Guaranted error below 1ulp.  
Operation:  
```
VDST = APPROX_RCP(ASFLOAT(SRC0))
```

#### V_RCP_F64

Opcode VOP1: 47 (0x2f) for GCN 1.0/1.1; 37 (0x25) for GCN 1.2  
Opcode VOP3A: 431 (0x1af) for GCN 1.0/1.1; 357 (0x165) for GCN 1.2  
Syntax: V_RCP_F64 VDST(2), SRC0(2)  
Description: Approximate reciprocal from double FP value SRC0 and store it to VDST.
Relative error of approximation is ~1e-8.  
Operation:  
```
VDST = APPROX_RCP(ASDOUBLE(SRC0))
```

#### V_RCP_IFLAG_F32

Opcode VOP1: 43 (0x2b) for GCN 1.0/1.1; 35 (0x23) for GCN 1.2  
Opcode VOP3A: 427 (0x1ab) for GCN 1.0/1.1; 355 (0x163) for GCN 1.2  
Syntax: V_RCP_IFLAG_F32 VDST, SRC0  
Description: Approximate reciprocal from floating point value SRC0 and store it to VDST.
Guaranted error below 1ulp. This instruction signals integer division by zero, instead
any floating point exception when error is occurred.  
Operation:  
```
VDST = APPROX_RCP_IFLAG(ASFLOAT(SRC0))
```

#### V_RCP_LEGACY_F32

Opcode VOP1: 41 (0x29) for GCN 1.0/1.1  
Opcode VOP3A: 425 (0x1a9) for GCN 1.0/1.1  
Syntax: V_RCP_LEGACY_F32 VDST, SRC0  
Description: Approximate reciprocal from floating point value SRC0 and store it to VDST.
Guaranted error below 1ulp. If SRC0 or VDST is zero or infinity then store 0 with proper
sign to VDST.  
Operation:  
```
FLOAT SF = ASFLOAT(SRC0)
if (ABS(SF)==0.0)
    VDST = SIGN(SF)*0.0
else
{
    VDST = APPROX_RCP(SF)
    if (ABS(ASFLOAT(VDST)) == INF)
        VDST = SIGN(SF)*0.0
}
```

#### V_READFIRSTLANE_B32

Opcode VOP1: 2 (0x2)  
Opcode VOP3A: 386 (0x182) for GCN 1.0/1.1; 322 (0x142) for GCN 1.2  
Syntax: V_READFIRSTLANE_B32 SDST, VSRC0  
Description: Copy one VSRC0 lane value to one SDST. Lane (thread id) is first active lane id
or first lane id all lanes are inactive. SSRC1 can be SGPR or M0. Ignores EXEC mask.  
Operation:  
```
UINT8 firstlane = 0
for (UINT8 i = 0; i < 64; i++)
    if ((1ULL<<i) & EXEC) != 0)
    { firstlane = i; break; }
SDST = VSRC0[firstlane]
```
#### V_RNDNE_F16

Opcode VOP1: 71 (0x47) for GCN 1.2  
Opcode VOP3A: 391 (0x187) for GCN 1.2  
Syntax: V_RNDNE_F16 VDST, SRC0  
Description: Round half floating point value SRC0 to nearest even integer,
and store result to VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = RNDNE(ASHALF(SRC0))
```

#### V_RNDNE_F32

Opcode VOP1: 35 (0x23) for GCN 1.0/1.1; 30 (0x1e) for GCN 1.2  
Opcode VOP3A: 420 (0x1a4) for GCN 1.0/1.1; 350 (0x15e) for GCN 1.2  
Syntax: V_RNDNE_F32 VDST, SRC0  
Description: Round floating point value SRC0 to nearest even integer, and store result to
VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = RNDNE(ASFLOAT(SRC0))
```

#### V_RNDNE_F64

Opcode VOP1: 25 (0x19) for GCN 1.1/1.2  
Opcode VOP3A: 409 (0x199) for GCN 1.1; 345 (0x159) for GCN 1.2  
Syntax: V_RNDNE_F64 VDST(2), SRC0(2)  
Description: Round double floating point value SRC0 to nearest even integer,
and store result to VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = RNDNE(ASDOUBLE(SRC0))
```

#### V_RSQ_CLAMP_F32

Opcode VOP1: 44 (0x2c) for GCN 1.0/1.1  
Opcode VOP3A: 428 (0x1ac) for GCN 1.0/1.1  
Syntax: V_RSQ_CLAMP_F32 VDST, SRC0  
Description: Approximate reciprocal square root from floating point value SRC0 with
clamping to MAX_FLOAT, and store result to VDST.
If SRC0 is negative value, store -NAN to VDST.
This instruction doesn't handle denormalized values regardless FLOAT MODE register setup.  
Operation:  
```
VDST = APPROX_RSQRT(ASFLOAT(SRC0))
if (ASFLOAT(VDST)==INF)
    VDST = MAX_FLOAT
```

#### V_RSQ_CLAMP_F64

Opcode VOP1: 50 (0x32) for GCN 1.0/1.1
Opcode VOP3A: 434 (0x1b2) for GCN 1.0/1.1
Syntax: V_RSQ_CLAMP_F64 VDST(2), SRC0(2)  
Description: Approximate reciprocal square root from double floating point value SRC0
with clamping to MAX_DOUBLE ,and store it to VDST. If SRC0 is negative value,
store -NAN to VDST.  
Operation:  
```
VDST = APPROX_RSQRT(ASDOUBLE(SRC0))
if (ASDOUBLE(VDST)==INF)
    VDST = MAX_DOUBLE
```

#### V_RSQ_F16

Opcode VOP1: 63 (0x3f) for GCN 1.2  
Opcode VOP3A: 383 (0x17f) for GCN 1.2  
Syntax: V_RSQ_F16 VDST, SRC0  
Description: Approximate reciprocal square root from half floating point value SRC0 and
store it to VDST. If SRC0 is negative value, store -NAN to VDST.  
Operation:  
```
VDST = APPROX_RSQRT(ASHALF(SRC0))
```

#### V_RSQ_F32

Opcode VOP1: 46 (0x2e) for GCN 1.0/1.1; 36 (0x24) for GCN 1.2  
Opcode VOP3A: 430 (0x1ae) for GCN 1.0/1.1; 356 (0x164) for GCN 1.2  
Syntax: V_RSQ_F32 VDST, SRC0  
Description: Approximate reciprocal square root from floating point value SRC0 and
store it to VDST. If SRC0 is negative value, store -NAN to VDST.
This instruction doesn't handle denormalized values regardless FLOAT MODE register setup.  
Operation:  
```
VDST = APPROX_RSQRT(ASFLOAT(SRC0))
```

#### V_RSQ_F64

Opcode VOP1: 49 (0x31) for GCN 1.0/1.1; 38 (0x26) for GCN 1.2  
Opcode VOP3A: 433 (0x1b1) for GCN 1.0/1.1; 358 (0x166) for GCN 1.2  
Syntax: V_RSQ_F64 VDST(2), SRC0(2)  
Description: Approximate reciprocal square root from double floating point value SRC0 and
store it to VDST. If SRC0 is negative value, store -NAN to VDST.  
Operation:  
```
VDST = APPROX_RSQRT(ASDOUBLE(SRC0))
```

#### V_RSQ_LEGACY_F32

Opcode VOP1: 45 (0x2d) for GCN 1.0/1.1  
Opcode VOP3A: 429 (0x1ad) for GCN 1.0/1.1  
Syntax: V_RCP_LEGACY_F32 VDST, SRC0  
Description: Approximate reciprocal square root from floating point value SRC0,
and store result to VDST. If SRC0 is negative value, store -NAN to VDST.
If result is zero then store 0.0 to VDST.
This instruction doesn't handle denormalized values regardless FLOAT MODE register setup.  
Operation:  
```
VDST = APPROX_RSQRT(ASFLOAT(SRC0))
if (ASFLOAT(VDST)==INF)
    VDST = 0.0
```

#### V_SAT_PK_U8_I16

Opcode VOP1: 79 (0x4f) for GCN 1.4  
Opcode VOP3A: 399 (0x18f) for GCN 1.4  
Syntax: V_SAT_PK_U8_I16 VDST, SRC0  
Description: Saturate two packed signed 16-bit values in SRC0 to 8-bit unsigned value
and store they values to VDST in lower 16-bits.  
Operation:  
```
VDST = MAX(MIN((INT16)(SRC0&0xffff), 255), 0)
VDST |= MAX(MIN((INT16)(SRC0>>16), 255), 0) << 8
```

#### V_SCREEN_PARTITION_4SE_B32

Opcode: VOP1: 55 (0x37) for GCN 1.4  
Opcode: VOP3A: 375 (0x177) for GCN 1.4  
Syntax: V_SCREEN_PARTITION_4SE_B32 VDST, SRC0  
Description: 4SE version of LUT instruction for screen partitioning/filtering (see more in ISA manual). Get lower 8-bits from SRC0 and translate by table and store result to VDST.  
Operation:  
```
BYTE TABLE[256] = {
0x1, 0x3, 0x7, 0xf, 0x5, 0xf, 0xf, 0xf, 0x7, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 
0xf, 0x2, 0x6, 0xe, 0xf, 0xa, 0xf, 0xf, 0xf, 0xb, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf,
0xd, 0xf, 0x4, 0xc, 0xf, 0xf, 0x5, 0xf, 0xf, 0xf, 0xd, 0xf, 0xf, 0xf, 0xf, 0xf,
0x9, 0xb, 0xf, 0x8, 0xf, 0xf, 0xf, 0xa, 0xf, 0xf, 0xf, 0xe, 0xf, 0xf, 0xf, 0xf,
0xf, 0xf, 0xf, 0xf, 0x4, 0xc, 0xd, 0xf, 0x6, 0xf, 0xf, 0xf, 0xe, 0xf, 0xf, 0xf,
0xf, 0xf, 0xf, 0xf, 0xf, 0x8, 0x9, 0xb, 0xf, 0x9, 0x9, 0xf, 0xf, 0xd, 0xf, 0xf,
0xf, 0xf, 0xf, 0xf, 0x7, 0xf, 0x1, 0x3, 0xf, 0xf, 0x9, 0xf, 0xf, 0xf, 0xb, 0xf,
0xf, 0xf, 0xf, 0xf, 0x6, 0xe, 0xf, 0x2, 0x6, 0xf, 0xf, 0x6, 0xf, 0xf, 0xf, 0x7,
0xb, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x2, 0x3, 0xb, 0xf, 0xa, 0xf, 0xf, 0xf,
0xf, 0x7, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x1, 0x9, 0xd, 0xf, 0x5, 0xf, 0xf,
0xf, 0xf, 0xe, 0xf, 0xf, 0xf, 0xf, 0xf, 0xe, 0xf, 0x8, 0xc, 0xf, 0xf, 0xa, 0xf,
0xf, 0xf, 0xf, 0xd, 0xf, 0xf, 0xf, 0xf, 0x6, 0x7, 0xf, 0x4, 0xf, 0xf, 0xf, 0x5,
0x9, 0xf, 0xf, 0xf, 0xd, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x8, 0xc, 0xe, 0xf,
0xf, 0x6, 0x6, 0xf, 0xf, 0xe, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x4, 0x6, 0x7,
0xf, 0xf, 0x6, 0xf, 0xf, 0xf, 0x7, 0xf, 0xf, 0xf, 0xf, 0xf, 0xb, 0xf, 0x2, 0x3,
0x9, 0xf, 0xf, 0x9, 0xf, 0xf, 0xf, 0xb, 0xf, 0xf, 0xf, 0xf, 0x9, 0xd, 0xf, 0x1 }
VDST = TABLE[SRC0&0xff]
```

#### V_SIN_F16

Opcode VOP1: 73 (0x49) for GCN 1.2  
Opcode VOP3A: 393 (0x189) for GCN 1.2  
Syntax: V_SIN_F16 VDST, SRC0  
Description: Compute sine of half FP value from SRC0. Input value must be
normalized to range 1.0 - 1.0 (-360 degree : 360 degree).
If SRC0 value is out of range then store 0.0 to VDST.
If SRC0 value is infinity, store -NAN to VDST.  
Operation:  
```
HALF SF = ASHALF(SRC0)
VDST = 0.0
if (SF >= -1.0 && SF <= 1.0)
    VDST = APPROX_SIN(SF)
else if (ABS(SF)==INF_H)
    VDST = -NAN_H
else if (ISNAN(SF))
    VDST = SRC0
```

#### V_SIN_F32

Opcode VOP1: 53 (0x35) for GCN 1.0/1.1; 41 (0x29) for GCN 1.2  
Opcode VOP3A: 437 (0x1b5) for GCN 1.0/1.1; 361 (0x169) for GCN 1.2  
Syntax: V_SIN_F32 VDST, SRC0  
Description: Compute sine of FP value from SRC0. Input value must be normalized to range
1.0 - 1.0 (-360 degree : 360 degree). If SRC0 value is out of range then store 0.0 to VDST.
If SRC0 value is infinity, store -NAN to VDST.  
Operation:  
```
FLOAT SF = ASFLOAT(SRC0)
VDST = 0.0
if (SF >= -1.0 && SF <= 1.0)
    VDST = APPROX_SIN(SF)
else if (ABS(SF)==INF)
    VDST = -NAN
else if (ISNAN(SF))
    VDST = SRC0
```

#### V_SQRT_F16

Opcode VOP1: 62 (0x3e) for GCN 1.2  
Opcode VOP3A: 382 (0x17e) for GCN 1.2  
Syntax: V_SQRT_F16 VDST, SRC0  
Description: Compute square root of half floating point value SRC0, and
store result to VDST. If SRC0 is negative value then store -NaN to VDST.  
Operation:  
```
if (ASHALF(SRC0)>=0.0)
    VDST = APPROX_SQRT(ASHALF(SRC0))
else
    VDST = -NAN_H
```

#### V_SQRT_F32

Opcode VOP1: 51 (0x33) for GCN 1.0/1.1; 39 (0x27) for GCN 1.2  
Opcode VOP3A: 435 (0x1b3) for GCN 1.0/1.1; 359 (0x167) for GCN 1.2  
Syntax: V_SQRT_F32 VDST, SRC0  
Description: Compute square root of floating point value SRC0, and store result to VDST.
If SRC0 is negative value then store -NaN to VDST.  
Operation:  
```
if (ASFLOAT(SRC0)>=0.0)
    VDST = APPROX_SQRT(ASFLOAT(SRC0))
else
    VDST = -NAN
```

#### V_SQRT_F64

Opcode VOP1: 52 (0x34) for GCN 1.0/1.1; 40 (0x28) for GCN 1.2  
Opcode VOP3A: 436 (0x1b4) for GCN 1.0/1.1; 360 (0x168) for GCN 1.2  
Syntax: V_SQRT_F64 VDST(2), SRC0(2)  
Description: Compute square root of double floating point value SRC0, and store result
to VDST. Relative error of approximation is ~1e-8.
If SRC0 is negative value then store -NaN to VDST.  
Operation:  
```
if (ASDOUBLE(SRC0)>=0.0)
    VDST = APPROX_SQRT(ASDOUBLE(SRC0))
else
    VDST = -NAN
```

#### V_SWAP_B32

Opcode VOP1: 81 (0x51) for GCN 1.4  
Opcode VOP3A: 401 (0x191) for GCN 1.4  
Syntax: V_SWAP_B32 VDST, SRC0  
Description: Swap SRC0 and VDST.  
```
UINT32 TMP = VDST
VDST = SRC0
SRC0 = TMP
```

#### V_TRUNC_F16

Opcode VOP1: 70 (0x46) for GCN 1.2  
Opcode VOP3A: 390 (0x186) for GCN 1.2  
Syntax: V_TRUNC_F16 VDST, SRC0  
Description: Get integer value from half floating point value SRC0, and store (as half)
it to VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = RNDTZ(ASHALF(SRC0))
```

#### V_TRUNC_F32

Opcode VOP1: 33 (0x21) for GCN 1.0/1.1; 28 (0x1c) for GCN 1.2  
Opcode VOP3A: 417 (0x1a1) for GCN 1.0/1.1; 348 (0x15c) for GCN 1.2  
Syntax: V_TRUNC_F32 VDST, SRC0  
Description: Get integer value from floating point value SRC0, and store (as float)
it to VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = RNDTZ(ASFLOAT(SRC0))
```

#### V_TRUNC_F64

Opcode VOP1: 23 (0x17) for GCN 1.1/1.2  
Opcode VOP3A: 407 (0x197) for GCN 1.1; 343 (0x157) for GCN 1.2  
Syntax: V_TRUNC_F64 VDST(2), SRC0(2)  
Description: Get integer value from double floating point value SRC0, and store (as float)
it to VDST. If SRC0 is infinity or NaN then copy SRC0 to VDST.  
Operation:  
```
VDST = RNDTZ(ASDOUBLE(SRC0))
```
