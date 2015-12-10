## GCN ISA VOP3 instructions

The VOP3 instructions requires two dword to store in program code. By default, these
encoding of these instructions gives all features of the VOP3 encoding: all possible
modifiers, any source operand combination.

List of fields for the VOP3A/VOP3B encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Vector destination operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
11    | CLAMP    | CLAMP modifier (VOP3A)
15    | CLAMP    | CLAMP modifier (VOP3B)
17-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

List of fields for VOP3A/VOP3B encoding (GCN 1.2):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Destination vector operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
15    | CLAMP    | CLAMP modifier
16-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

Typical syntax: INSTRUCTION VDST, SRC0, SRC1, SRC2 [MODIFIERS]

Modifiers:

* CLAMP - clamps destination floating point value in range 0.0-1.0
* MUL:2, MUL:4, DIV:2 - OMOD modifiers. Multiply destination floating point value by
2.0, 4.0 or 0.5 respectively. Clamping applied after OMOD modifier.
* -SRC - negate floating point value from source operand. Applied after ABS modifier.
* ABS(SRC) - apply absolute value to source operand

NOTE: OMOD modifier doesn't work if output denormals are allowed
(5 bit of MODE register for single precision or 7 bit for double precision).  
NOTE: OMOD and CLAMP modifier affects only for instruction that output is
floating point value.  
NOTE: ABS and negation is applied to source operand for any instruction.

Negation and absolute value can be combined: `-ABS(V0)`. Modifiers CLAMP and
OMOD (MUL:2, MUL:4 and DIV:2) can be given in random order.

Limitations for operands:

* only one SGPR can be read by instruction. Multiple occurrences of this same
SGPR is allowed
* only one literal constant can be used, and only when a SGPR or M0 is not used in
source operands
* only SRC0 can holds LDS_DIRECT

Unaligned pairs of SGPRs are allowed in source operands.

List of the instructions by opcode (GCN 1.0/1.1):

 Opcode      | Mnemonic (GCN 1.0) | Mnemonic (GCN 1.0)
-------------|--------------------|-----------------------------
 320 (0x140) | V_MAD_LEGACY_F32   | V_MAD_LEGACY_F32
 321 (0x141) | V_MAD_F32          | V_MAD_F32
 322 (0x142) | V_MAD_I32_I24      | V_MAD_I32_I24
 323 (0x143) | V_MAD_U32_U24      | V_MAD_U32_U24
 324 (0x144) | V_CUBEID_F32       | V_CUBEID_F32
 325 (0x145) | V_CUBESC_F32       | V_CUBESC_F32
 326 (0x146) | V_CUBETC_F32       | V_CUBETC_F32
 327 (0x147) | V_CUBEMA_F32       | V_CUBEMA_F32
 328 (0x148) | V_BFE_U32          | V_BFE_U32
 329 (0x149) | V_BFE_I32          | V_BFE_I32
 330 (0x14a) | V_BFI_B32          | V_BFI_B32
 331 (0x14b) | V_FMA_F32          | V_FMA_F32
 332 (0x14c) | V_FMA_F64          | V_FMA_F64
 333 (0x14d) | V_LERP_U8          | V_LERP_U8
 334 (0x14e) | V_ALIGNBIT_B32     | V_ALIGNBIT_B32
 335 (0x14f) | V_ALIGNBYTE_B32    | V_ALIGNBYTE_B32
 336 (0x150) | V_MULLIT_F32       | V_MULLIT_F32
 337 (0x151) | V_MIN3_F32         | V_MIN3_F32
 338 (0x152) | V_MIN3_I32         | V_MIN3_I32
 339 (0x153) | V_MIN3_U32         | V_MIN3_U32
 340 (0x154) | V_MAX3_F32         | V_MAX3_F32
 341 (0x155) | V_MAX3_I32         | V_MAX3_I32
 342 (0x156) | V_MAX3_U32         | V_MAX3_U32
 343 (0x157) | V_MED3_F32         | V_MED3_F32
 344 (0x158) | V_MED3_I32         | V_MED3_I32
 345 (0x159) | V_MED3_U32         | V_MED3_U32
 346 (0x15a) | V_SAD_U8           | V_SAD_U8
 347 (0x15b) | V_SAD_HI_U8        | V_SAD_HI_U8
 348 (0x15c) | V_SAD_U16          | V_SAD_U16
 349 (0x15d) | V_SAD_U32          | V_SAD_U32
 350 (0x15e) | V_CVT_PK_U8_F32    | V_CVT_PK_U8_F32
 351 (0x15f) | V_DIV_FIXUP_F32    | V_DIV_FIXUP_F32
 352 (0x160) | V_DIV_FIXUP_F64    | V_DIV_FIXUP_F64
 353 (0x161) | V_LSHL_B64         | V_LSHL_B64
 354 (0x162) | V_LSHR_B64         | V_LSHR_B64
 355 (0x163) | V_ASHR_I64         | V_ASHR_I64
 356 (0x164) | V_ADD_F64          | V_ADD_F64
 357 (0x165) | V_MUL_F64          | V_MUL_F64
 358 (0x166) | V_MIN_F64          | V_MIN_F64
 359 (0x167) | V_MAX_F64          | V_MAX_F64
 360 (0x168) | V_LDEXP_F64        | V_LDEXP_F64
 361 (0x169) | V_MUL_LO_U32       | V_MUL_LO_U32
 362 (0x16a) | V_MUL_HI_U32       | V_MUL_HI_U32
 363 (0x16b) | V_MUL_LO_I32       | V_MUL_LO_I32
 364 (0x16c) | V_MUL_HI_I32       | V_MUL_HI_I32
 365 (0x16d) | V_DIV_SCALE_F32 (VOP3B) | V_DIV_SCALE_F32 (VOP3B)
 366 (0x16e) | V_DIV_SCALE_F64 (VOP3B) | V_DIV_SCALE_F64 (VOP3B)
 367 (0x16f) | V_DIV_FMAS_F32     | V_DIV_FMAS_F32
 368 (0x170) | V_DIV_FMAS_F64     | V_DIV_FMAS_F64
 369 (0x171) | V_MSAD_U8          | V_MSAD_U8
 370 (0x172) | V_QSAD_U8          | V_QSAD_PK_U16_U8
 371 (0x173) | V_MQSAD_U8         | V_MQSAD_PK_U16_U8
 372 (0x174) | V_TRIG_PREOP_F64   | V_TRIG_PREOP_F64
 373 (0x175) | --                 | V_MQSAD_U32_U8
 374 (0x176) | --                 | V_MAD_U64_U32 (VOP3B)
 375 (0x177) | --                 | V_MAD_I64_I32 (VOP3B)

List of the instructions by opcode (GCN 1.2):

 Opcode      | Mnemonic
-------------|-------------------------
 448 (0x1c0) | V_MAD_LEGACY_F32
 449 (0x1c1) | V_MAD_F32
 450 (0x1c2) | V_MAD_I32_I24
 451 (0x1c3) | V_MAD_U32_U24
 452 (0x1c4) | V_CUBEID_F32
 453 (0x1c5) | V_CUBESC_F32
 454 (0x1c6) | V_CUBETC_F32
 455 (0x1c7) | V_CUBEMA_F32
 456 (0x1c8) | V_BFE_U32
 457 (0x1c9) | V_BFE_I32
 458 (0x1ca) | V_BFI_B32
 459 (0x1cb) | V_FMA_F32
 460 (0x1cc) | V_FMA_F64
 461 (0x1cd) | V_LERP_U8
 462 (0x1ce) | V_ALIGNBIT_B32
 463 (0x1cf) | V_ALIGNBYTE_B32
 464 (0x1d0) | V_MIN3_F32
 465 (0x1d1) | V_MIN3_I32
 466 (0x1d2) | V_MIN3_U32
 467 (0x1d3) | V_MAX3_F32
 468 (0x1d4) | V_MAX3_I32
 469 (0x1d5) | V_MAX3_U32
 470 (0x1d6) | V_MED3_F32
 471 (0x1d7) | V_MED3_I32
 472 (0x1d8) | V_MED3_U32
 473 (0x1d9) | V_SAD_U8
 474 (0x1da) | V_SAD_HI_U8
 475 (0x1db) | V_SAD_U16
 476 (0x1dc) | V_SAD_U32
 477 (0x1dd) | V_CVT_PK_U8_F32
 478 (0x1de) | V_DIV_FIXUP_F32
 479 (0x1df) | V_DIV_FIXUP_F64
 480 (0x1e0) | V_DIV_SCALE_F32 (VOP3B)
 481 (0x1e1) | V_DIV_SCALE_F64 (VOP3B)
 482 (0x1e2) | V_DIV_FMAS_F32
 483 (0x1e3) | V_DIV_FMAS_F64
 484 (0x1e4) | V_MSAD_U8
 485 (0x1e5) | V_QSAD_PK_U16_U8
 486 (0x1e6) | V_MQSAD_PK_U16_U8
 487 (0x1e7) | V_MQSAD_U32_U8
 488 (0x1e8) | V_MAD_U64_U32 (VOP3B)
 489 (0x1e9) | V_MAD_I64_I32 (VOP3B)
 490 (0x1ea) | V_MAD_F16
 491 (0x1eb) | V_MAD_U16
 492 (0x1ec) | V_MAD_I16
 493 (0x1ed) | V_PERM_B32
 494 (0x1ee) | V_FMA_F16
 495 (0x1ef) | V_DIV_FIXUP_F16
 496 (0x1f0) | V_CVT_PKACCUM_U8_F32
 624 (0x270) | V_INTERP_P1_F32 (VINTRP)
 625 (0x271) | V_INTERP_P2_F32 (VINTRP)
 626 (0x272) | V_INTERP_MOV_F32 (VINTRP)
 627 (0x273) | V_INTERP_P1LL_F16 (VINTRP)
 628 (0x274) | V_INTERP_P1LV_F16 (VINTRP)
 629 (0x275) | V_INTERP_P2_F16 (VINTRP)
 640 (0x280) | V_ADD_F64
 641 (0x281) | V_MUL_F64
 642 (0x282) | V_MIN_F64
 643 (0x283) | V_MAX_F64
 644 (0x284) | V_LDEXP_F64
 645 (0x285) | V_MUL_LO_U32
 646 (0x286) | V_MUL_HI_U32
 647 (0x287) | V_MUL_HI_I32
 648 (0x288) | V_LDEXP_F32
 649 (0x289) | V_READLANE_B32
 650 (0x28a) | V_WRITELANE_B32
 651 (0x28b) | V_BCNT_U32_B32
 652 (0x28c) | V_MBCNT_LO_U32_B32
 653 (0x28d) | V_MBCNT_HI_U32_B32
 654 (0x28e) | V_MAC_LEGACY_F32
 655 (0x28f) | V_LSHLREV_B64
 656 (0x290) | V_LSHRREV_B64
 657 (0x291) | V_ASHRREV_I64
 658 (0x292) | V_TRIG_PREOP_F64
 659 (0x293) | V_BFM_B32
 660 (0x294) | V_CVT_PKNORM_I16_F32
 661 (0x295) | V_CVT_PKNORM_U16_F32
 662 (0x296) | V_CVT_PKRTZ_F16_F32
 663 (0x297) | V_CVT_PK_U16_U32
 664 (0x298) | V_CVT_PK_I16_I32

### Instruction set

Alphabetically sorted instruction list:

#### V_ADD_F64

Opcode: 356 (0x164) for GCN 1.0/1.1; 640 (0x280) for GCN 1.2  
Syntax: V_ADD_F64 VDST(2), SRC0(2), SRC1(2)  
Description: Add two double FP value from SRC0 and SRC1 and store result to VDST.  
Operation:  
```
VDST = ASDOUBLE(SRC0) + ASDOUBLE(SRC1)
```

#### V_ALIGNBIT_B32

Opcode: 334 (0x14e) for GCN 1.0/1.1; 462 (0x1ce) for GCN 1.2  
Syntax: V_ALIGNBIT_B32 VDST, SRC0, SRC1, SRC2  
Description: Align bit. Shift right bits in 64-bit stored in SRC1 (low part) and
SRC0 (high part) by SRC2&31 bits, and store low 32-bit of the result in VDST.  
Operation:  
```
VDST = (((UINT64)SRC0)<<32) | SRC1) >> (SRC2&31)
```

#### V_ALIGNBYTE_B32

Opcode: 335 (0x14f) for GCN 1.0/1.1; 463 (0x1cf) for GCN 1.2  
Syntax: V_ALIGNBYTE_B32 VDST, SRC0, SRC1, SRC2  
Description: Align bit. Shift right bits in 64-bit stored in SRC1 (low part) and
SRC0 (high part) by (SRC2&3)*8 bits, and store low 32-bit of the result in VDST.  
Operation:  
```
VDST = (((UINT64)SRC0)<<32) | SRC1) >> ((SRC2&3)*8)
```

#### V_ASHR_I64

Opcode: 355 (0x163) for GCN 1.0/1.1  
Syntax: V_ASHR_I32 VDST(2), SRC0(2), SRC1  
Description: Arithmetic shift right SRC0 by (SRC1&63) bits and store result into VDST.  
Operation:  
```
VDST = (INT64)SRC0 >> (SRC1&63)
```

#### V_BFE_I32

Opcode: 329 (0x149) for GCN 1.0/1.1; 457 (0x1c9) for GCN 1.2  
Syntax: V_BFE_I32 VDST, SRC0, SRC1, SRC2  
Description: Extracts bits in SRC0 from range (SRC1&31) with length (SRC2&31)
and extend sign from last bit of extracted value, and store result to VDST.  
Operation:  
```
UINT8 shift = SRC1 & 31
UINT8 length = SRC2 & 31
if (length==0)
    VDST = 0
if (shift+length < 32)
    VDST = (INT32)(SRC0 << (32 - shift - length)) >> (32 - length)
else
    VDST = (INT32)SRC0 >> shift
```

#### V_BFE_U32

Opcode: 328 (0x148) for GCN 1.0/1.1; 456 (0x1c8) for GCN 1.2  
Syntax: V_BFE_U32 VDST, SRC0, SRC1, SRC2  
Description: Extracts bits in SRC0 from range SRC1&31 with length SRC2&31, and
store result to VDST.  
Operation:  
```
UINT8 shift = SRC1 & 31
UINT8 length = SRC2 & 31
if (length==0)
    VDST = 0
if (shift+length < 32)
    VDST = SRC0 << (32 - shift - length) >> (32 - length)
else
    VDST = SRC0 >> shift
```

#### V_BFI_B32

Opcode: 330 (0x14a) for GCN 1.0/1.1; 458 (0x1ca) for GCN 1.2  
Syntax: V_BFI_B32 VDST, SRC0, SRC1, SRC2  
Description: Replace bits in SRC2 by bits from SRC1 marked by bits in SRC0, and store result
to VDST.  
Operation:  
```
VDST = (SRC0 & SRC1) | (~SRC0 & SRC2)
```

#### V_CUBEID_F32

Opcode: 324 (0x144) for GCN 1.0/1.1; 452 (0x1c4) for GCN 1.2  
Syntax: V_CUBEID_F32 VDST, SRC0, SRC1, SRC2  
Description: Cubemap face identification. Determine face by comparing three single FP values:
SRC0 (X), SRC1 (Y), SRC2(Z). Choose highest absolute value and check whether is negative or
positive. Store floating point value of face ID: (DIM*2.0)+(V[DIM]>=0?1:0),
where DIM is number of choosen dimension (X - 0, Y - 1, Z - 2);
V - vector = [SRC0, SRC1, SRC2].  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
FLOAT OUT
if (ABS(SF2) >= ABS(SF1) && ABS(SF2) >= ABS(SF0))
    OUT = (SF2 >= 0.0) ? 4 : 5
else if (ABS(SF1) >= ABS(SF0)
    OUT = (SF1 >= 0.0) ? 2 : 3
else
    OUT = (SF0 >= 0.0) ? 0 : 1
VDST = OUT
```

#### V_CUBEMA_F32

Opcode: 327 (0x147) for GCN 1.0/1.1; 455 (0x1c7) for GCN 1.2  
Syntax: V_CUBEMA_F32 VDST, SRC0, SRC1, SRC2  
Description: Cubemap Major Axis. Choose highest absolute value from all three FP values
(SRC0, SRC1, SRC2) and multiply choosen FP value by two. Result is stored in VDST.  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
if (ABS(SF2) >= ABS(SF1) && ABS(SF2) >= ABS(SF0))
    OUT = 2*SF2
else if (ABS(SF1) >= ABS(SF0)
    OUT = 2*SF1
else
    OUT = 2*SF0
VDST = OUT
```

#### V_CUBESC_F32

Opcode: 325 (0x145) for GCN 1.0/1.1; 453 (0x1c5) for GCN 1.2  
Syntax: V_CUBESC_F32 VDST, SRC0, SRC1, SRC2  
Description: Cubemap S coordination. Algorithm below.  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
if (ABS(SF2) >= ABS(SF1) && ABS(SF2) >= ABS(SF0))
    OUT = SIGN((SF2) * SF0
else if (ABS(SF1) >= ABS(SF0)
    OUT = SF0
else
    OUT = -SIGN((SF0) * SF2
VDST = OUT
```

#### V_CUBETC_F32

Opcode: 326 (0x146) for GCN 1.0/1.1; 454 (0x1c6) for GCN 1.2  
Syntax: V_CUBETC_F32 VDST, SRC0, SRC1, SRC2  
Description: Cubemap T coordination. Algorithm below.  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
if (ABS(SF2) >= ABS(SF1) && ABS(SF2) >= ABS(SF0))
    OUT = -SF1
else if (ABS(SF1) >= ABS(SF0)
    OUT = SIGN(SF1) * SF2
else
    OUT = -SF1
VDST = OUT
```

#### V_CVT_PK_U8_F32

Opcode: 350 (0x15e) for GCN 1.0/1.1; 477 (0x1dd) for GCN 1.2  
Syntax: V_CVT_PK_U8_F32 VDST, SRC0, SRC1, SRC2  
Description: Convert floating point value from SRC0 to unsigned byte value with
rounding mode from MODE register, and store this byte to (SRC1&3)'th byte with
other bytes of SRC2 of VDST.  
Operation:  
```
UINT8 shift = ((SRC1&3) * 8)
UINT32 mask = 0xff << shift
FLOAT f = RNDINT(ASFLOAT(SRC0))
UINT8 VAL8 = 0
if (ISNAN(f))
    VAL8 = (UINT8)MAX(MIN(f, 255.0), 0.0)
VDST = (SRC2&~mask) | (((UINT32)VAL8) << shift)
```

#### V_DIV_FIXUP_F32

Opcode: 351 (0x15f) for GCN 1.0/1.1; 478 (0x1de) for GCN 1.2  
Syntax: V_DIV_FIXUP_F32 VDST, SRC0, SRC1, SRC2  
Description: Handle all exceptions requires for single floating point division.
SRC0 is quotient, SRC1 is denominator, SRC2 is nominator. Correct result stored to VDST.  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
if (ISNAN(SF1) && !ISNAN(SF2))
    VDST = QUIETNAN(SF1)
else if (ISNAN(SF2))
    VDST = QUIETNAN(SF2)
else if (SF1 == 0.0 && SF2 == 0.0)
    VDST = NAN
else if (ABS(SF1)==INF && ABS(SF2)==INF)
    VDST = -NAN
else if (SF1 == 0.0)
    VDST = INF*SIGN(SF1)*SIGN(SF2)
else if (ABS(SF1) == INF)
    VDST = SIGN(SF1)*SIGN(SF2) >=0 ? 0.0 : -0.0
else if (ISNAN(SF0))
    VDST = SIGN(SF1)*SIGN(SF2)*INF
else
    VDST = SF0
```

#### V_DIV_FIXUP_F64

Opcode: 352 (0x160) for GCN 1.0/1.1; 479 (0x1df) for GCN 1.2  
Syntax: V_DIV_FIXUP_F64 VDST(2), SRC0(2), SRC1(2), SRC2(2)  
Description: Handle all exceptions requires for double floating point division.
SRC0 is quotient, SRC1 is denominator, SRC2 is nominator. Correct result stored to VDST.  
Operation:  
```
DOUBLE SF0 = ASDOUBLE(SRC0)
DOUBLE SF1 = ASDOUBLE(SRC1)
DOUBLE SF2 = ASDOUBLE(SRC2)
if (ISNAN(SF1) && !ISNAN(SF2))
    VDST = QUIETNAN(SF1)
else if (ISNAN(SF2))
    VDST = QUIETNAN(SF2)
else if (SF1 == 0.0 && SF2 == 0.0)
    VDST = NAN
else if (ABS(SF1)==INF && ABS(SF2)==INF)
    VDST = -NAN
else if (SF1 == 0.0)
    VDST = INF*SIGN(SF1)*SIGN(SF2)
else if (ABS(SF1) == INF)
    VDST = SIGN(SF1)*SIGN(SF2) >=0 ? 0.0 : -0.0
else if (ISNAN(SF0))
    VDST = SIGN(SF1)*SIGN(SF2)*INF
else
    VDST = SF0
```

#### V_DIV_SCALE_F32

#### V_DIV_SCALE_F64

#### V_FMA_F32

Opcode: 331 (0x14b) for GCN 1.0/1.1; 459 (0x1cb) for GCN 1.2  
Syntax: V_FMA_F32 VDST, SRC0, SRC1, SRC2  
Description: Fused multiply addition on single floating point values from
SRC0, SRC1 and SRC2. Result stored in VDST.  
Operation:  
```
// SRC0*SRC1+SRC2
VDST = FMA(ASFLOAT(SRC0), ASFLOAT(SRC1), ASFLOAT(SRC2))
```

#### V_FMA_F64

Opcode: 332 (0x14c) for GCN 1.0/1.1; 460 (0x1cc) for GCN 1.2  
Syntax: V_FMA_F64 VDST(2), SRC0(2), SRC1(2), SRC2(2)  
Description: Fused multiply addition on double floating point values from
SRC0, SRC1 and SRC2. Result stored in VDST.  
Operation:  
```
// SRC0*SRC1+SRC2
VDST = FMA(ASDOUBLE(SRC0), ASDOUBLE(SRC1), ASDOUBLE(SRC2))
```

#### V_LDEXP_F64

Opcode: 360 (0x168) for GCN 1.0/1.1; 644 (0x284) for GCN 1.2  
Syntax: V_LDEXP_F64 VDST(2), SRC0(2), SRC1  
Description: Do ldexp operation on SRC0 and SRC1 (multiply SRC0 by 2**(SRC1)).
SRC1 is signed integer, SRC0 is double floating point value.  
Operation:  
```
VDST = ASDOUBLE(SRC0) * POW(2.0, (INT32)SRC1)
```

#### V_LERP_U8

Opcode: 333 (0x14d) for GCN 1.0/1.1; 461 (0x1cd) for GCN 1.2  
Syntax: V_LERP_U8 VDST, SRC0, SRC1, SRC2  
Description: For each byte of dword, calculate average from SRC0 byte and SRC1 byte with
rounding mode defined in first of the byte SRC2. If rounding bit is set then result for
that byte is rounded, otherwise truncated. All bytes will be stored in VDST.  
Operation:  
```
for (UINT8 i = 0; i < 4; i++)
{
    UINT8 S0 = (SRC0 >> (i*8)) & 0xff
    UINT8 S1 = (SRC1 >> (i*8)) & 0xff
    UINT8 S2 = (SRC2 >> (i*8)) & 1
    VDST = (VDST & ~(255U<<(i*8))) | (((S0+S1+S2) >> 1) << (i*8)) 
}
```

#### V_LSHL_B64

Opcode: 353 (0x161) for GCN 1.0/1.1  
Syntax: V_LSHL_B32 VDST(2), SRC0(2), SRC1  
Description: Shift left SRC0 by (SRC1&63) bits and store result into VDST.  
Operation:  
```
VDST = SRC0 << (SRC1&63)
```

#### V_LSHR_B64

Opcode: 354 (0x162) for GCN 1.0/1.1  
Syntax: V_LSHR_B32 VDST(2), SRC0(2), SRC1  
Description: Shift right SRC0 by (SRC1&63) bits and store result into VDST.  
Operation:  
```
VDST = SRC0 >> (SRC1&63)
```

#### V_MAD_F32

Opcode: 321 (0x141) for GCN 1.0/1.1; 449 (0x1c1) for GCN 1.2  
Syntax: V_MAD_F32 VDST, SRC0, SRC1, SRC2  
Description: Multiply FP value from SRC0 by FP value from SRC1 and add SRC2, and store
result to VDST.  
Operation:  
```
VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1) + ASFLOAT(SRC2)
```

#### V_MAD_I32_I24

Opcode: 322 (0x142) for GCN 1.0/1.1; 450 (0x1c2) for GCN 1.2  
Syntax: V_MAD_I32_I24 VDST, SRC0, SRC1, SRC2  
Description: Multiply 24-bit signed integer value from SRC0 by 24-bit signed value from SRC1,
add SRC2 to this product, and and store result to VDST.  
Operation:  
```
INT32 V0 = (INT32)((SRC0&0x7fffff) | (SSRC0&0x800000 ? 0xff800000 : 0))
INT32 V1 = (INT32)((SRC1&0x7fffff) | (SSRC1&0x800000 ? 0xff800000 : 0))
VDST = V0 * V1 + SRC2
```

#### V_MAD_LEGACY_F32

Opcode: 320 (0x140) for GCN 1.0/1.1; 448 (0x1c0) for GCN 1.2  
Syntax: V_MAD_LEGACY_F32 VDST, SRC0, SRC1, SRC2  
Description: Multiply FP value from SRC0 by FP value from SRC1 and add result to SRC2, and
store result to VDST. If one of value is 0.0 then always store SRC2 to VDST
(do not apply IEEE rules for 0.0*x).  
Operation:  
```
if (ASFLOAT(SRC0)!=0.0 && ASFLOAT(SRC1)!=0.0)
    VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1) + ASFLOAT(SRC2)
```

#### V_MAD_U32_U24

Opcode: 323 (0x143) for GCN 1.0/1.1; 451 (0x1c3) for GCN 1.2  
Syntax: V_MAD_U32_U24 VDST, SRC0, SRC1, SRC2  
Description: Multiply 24-bit unsigned integer value from SRC0 by 24-bit unsigned value
from SRC1, add SRC2 to this product and store result to VDST.  
Operation:  
```
VDST = (UINT32)(SRC0&0xffffff) * (UINT32)(SRC1&0xffffff) + SRC2
```

#### V_MAX_F64

Opcode: 359 (0x167) for GCN 1.0/1.1; 643 (0x283) for GCN 1.2  
Syntax: V_MAX_F64 VDST(2), SRC0(2), SRC1(2)  
Description: Choose largest double FP value from SRC0 and SRC1, and store result to VDST.  
Operation:  
```
VDST = MAX((ASDOUBLE(SRC0), ASDOUBLE(SRC1))
```

#### V_MAX3_F32

Opcode: 340 (0x154) for GCN 1.0/1.1; 467 (0x1d3) for GCN 1.2  
Syntax: V_MAX3_F32 VDST, SRC0, SRC1, SRC2  
Description: Choose largest value from FP values SRC0, SRC1, SRC2, and store it to VDST.  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
if (ISNAN(SF0))
    VDST = MAX(SF1, SF2)
else if (ISNAN(SF1))
    VDST = MAX(SF0, SF2)
else if (ISNAN(SF2))
    VDST = MAX(SF0, SF1)
else if (SF2 > SF0 && SF2 > SF1)
    VDST = SF2
else
    VDST = MAX(SF1, SF0)
```

#### V_MAX3_I32

Opcode: 341 (0x155) for GCN 1.0/1.1; 468 (0x1d4) for GCN 1.2  
Syntax: V_MAX3_I32 VDST, SRC0, SRC1, SRC2  
Description: Choose largest value from signed integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if ((INT32)SRC2 > (INT32)SRC0 && (INT32)SRC2 > (INT32)SRC1)
    VDST = SRC2
else
    VDST = MAX((INT32)SRC1, (INT32)SRC0)
```

#### V_MAX3_U32

Opcode: 342 (0x156) for GCN 1.0/1.1; 469 (0x1d5) for GCN 1.2  
Syntax: V_MAX3_U32 VDST, SRC0, SRC1, SRC2  
Description: Choose largest value from unsigned integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if (SRC2 > SRC0 && SRC2 > SRC1)
    VDST = SRC2
else
    VDST = MAX(SRC1, SRC0)
```

#### V_MED3_F32

Opcode: 343 (0x157) for GCN 1.0/1.1; 470 (0x1d6) for GCN 1.2  
Syntax: V_MED3_F32 VDST, SRC0, SRC1, SRC2  
Description: Choose medium value from FP values SRC0, SRC1, SRC2, and store it to VDST.  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
if (ISNAN(SF0))
    VDST = MIN(SF1, SF2)
else if (ISNAN(SF1))
    VDST = MIN(SF0, SF2)
else if (ISNAN(SF2))
    VDST = MIN(SF0, SF1)
else if ((SF2 > SF1 && SF2 < SF0) || (SF2 < SF1 && SF2 > SF0))
    VDST = SF2
else if ((SF1 > SF2 && SF1 < SF0) || (SF1 < SF2 && SF1 > SF0))
    VDST = SF1
else
    VDST = SF0
```

#### V_MED3_I32

Opcode: 344 (0x158) for GCN 1.0/1.1; 471 (0x1d7) for GCN 1.2  
Syntax: V_MED3_I32 VDST, SRC0, SRC1, SRC2  
Description: Choose medium value from signed integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
INT32 S0 = (INT32)SRC0
INT32 S1 = (INT32)SRC1
INT32 S2 = (INT32)SRC2
if ((S2 > S1 && S2 < S0) || (S2 < S1 && S2 > S0))
    VDST = S2
else if ((S1 > S2 && S1 < S0) || (S1 < S2 && S1 > S0))
    VDST = S1
else
    VDST = S0
```

#### V_MED3_U32

Opcode: 345 (0x159) for GCN 1.0/1.1; 472 (0x1d8) for GCN 1.2  
Syntax: V_MED3_U32 VDST, SRC0, SRC1, SRC2  
Description: Choose medium value from unsigned integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if ((SRC2 > SRC1 && SRC2 < SRC0) || (SRC2 < SRC1 && SRC2 > SRC0))
    VDST = SRC2
else if ((SRC1 > SRC2 && SRC1 < SRC0) || (SRC1 < SRC2 && SRC1 > SRC0))
    VDST = SRC1
else
    VDST = SRC0
```

#### V_MIN_F64

Opcode: 358 (0x166) for GCN 1.0/1.1; 642 (0x282) for GCN 1.2  
Syntax: V_MIN_F64 VDST(2), SRC0(2), SRC1(2)  
Description: Choose smallest double FP value from SRC0 and SRC1, and store result to VDST.  
Operation:  
```
VDST = MIN((ASDOUBLE(SRC0), ASDOUBLE(SRC1))
```

#### V_MIN3_F32

Opcode: 337 (0x151) for GCN 1.0/1.1; 464 (0x1d0) for GCN 1.2  
Syntax: V_MIN3_F32 VDST, SRC0, SRC1, SRC2  
Description: Choose smallest value from FP values SRC0, SRC1, SRC2, and store it to VDST.  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
if (ISNAN(SF0))
    VDST = MIN(SF1, SF2)
else if (ISNAN(SF1))
    VDST = MIN(SF0, SF2)
else if (ISNAN(SF2))
    VDST = MIN(SF0, SF1)
else if (SF2 < SF0 && SF2 < SF1)
    VDST = SF2
else
    VDST = MIN(SF1, SF0)
```

#### V_MIN3_I32

Opcode: 338 (0x152) for GCN 1.0/1.1; 465 (0x1d1) for GCN 1.2  
Syntax: V_MIN3_I32 VDST, SRC0, SRC1, SRC2  
Description: Choose smallest value from signed integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if ((INT32)SRC2 < (INT32)SRC0 && (INT32)SRC2 < (INT32)SRC1)
    VDST = SRC2
else
    VDST = MIN((INT32)SRC1, (INT32)SRC0)
```

#### V_MIN3_U32

Opcode: 339 (0x153) for GCN 1.0/1.1; 466 (0x1d2) for GCN 1.2  
Syntax: V_MIN3_U32 VDST, SRC0, SRC1, SRC2  
Description: Choose smallest value from unsigned integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if (SRC2 < SRC0 && SRC2 < SRC1)
    VDST = SRC2
else
    VDST = MIN(SRC1, SRC0)
```

#### V_MUL_F64

Opcode: 357 (0x165) for GCN 1.0/1.1; 641 (0x281) for GCN 1.2  
Syntax: V_MUL_F64 VDST(2), SRC0(2), SRC1(2)  
Description: Multiply two double FP values from SRC0 and SRC1 and store result to VDST.  
Operation:  
```
VDST = ASDOUBLE(SRC0) * ASDOUBLE(SRC1)
```

#### V_MUL_HI_I32

Opcode: 364 (0x16c) for GCN 1.0/1.1; 647 (0x287) for GCN 1.2  
Syntax: V_MUL_HI_I32 VDST, SRC0, SRC1  
Description: Multiply 32-bit signed value SRC0 and SRC1, and store higher part of
the result to VDST.  
Operation:  
```
VDST = ((INT64)SRC0 * (INT32)SRC1) >> 32
```

#### V_MUL_HI_U32

Opcode: 362 (0x16a) for GCN 1.0/1.1; 646 (0x286) for GCN 1.2  
Syntax: V_MUL_HI_U32 VDST, SRC0, SRC1  
Description: Multiply 32-bit unsigned value SRC0 and SRC1, and store higher part of
the result to VDST.  
Operation:  
```
VDST = ((UINT64)SRC0 * SRC1) >> 32
```

#### V_MUL_LO_I32

Opcode: 363 (0x16b) for GCN 1.0/1.1
Syntax: V_MUL_LO_I32 VDST, SRC0, SRC1  
Description: Multiply 32-bit signed value SRC0 and SRC1, and store lower part of
the result to VDST.  
Operation:  
```
VDST = (INT32)SRC0 * (INT32)SRC1
```

#### V_MUL_LO_U32

Opcode: 361 (0x169) for GCN 1.0/1.1; 645 (0x285) for GCN 1.2  
Syntax: V_MUL_LO_U32 VDST, SRC0, SRC1  
Description: Multiply 32-bit unsigned value SRC0 and SRC1, and store lower part of
the result to VDST.  
Operation:  
```
VDST = SRC0 * SRC1
```

#### V_MULLIT_F32

Opcode: 336 (0x150) for GCN 1.0/1.1  
Syntax: V_MULLIT_F32 VDST, SRC0, SRC1, SRC2  
Description: Multiply FP value SRC0 and FP value SRC1, and store it to VDST if FP value in
SRC2 is greater than zero, otherwise, store -MAX_FLOAT to VDST.
If one of value is 0.0 and previous condition is satisfied then always store
0.0 to VDST (do not apply IEEE rules for 0.0*x).  
Operation:  
```
VDST = -MAX_FLOAT
if (ASFLOAT(SRC2) > 0.0 && !ISNAN(ASFLOAT(SRC2)))
{
    VDST = 0.0
    if (ASFLOAT(SRC0)!=0.0 && ASFLOAT(SRC1)!=0.0)
        VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1)
}
```

#### V_SAD_HI_U8

Opcode: 347 (0x15b) for GCN 1.0/1.1; 474 (0x1da) for GCN 1.2  
Syntax: V_SAD_HI_U8 VDST, SRC0, SRC1, SRC2  
Description: Calculate sum of absolute differences for all four bytes in SRC0 and SRC1,
shift result to high 16-bits, add SRC2 to result, and store result to VDST.  
Operation:  
```
VDST = SRC2
for (UINT8 i = 0; i < 4; i++)
    VDST += (ABS(((SRC0 >> (i*8)) & 0xff) - ((SRC1 >> (i*8)) & 0xff)))<<16
```

#### V_SAD_U16

Opcode: 348 (0x15c) for GCN 1.0/1.1; 475 (0x1db) for GCN 1.2  
Syntax: V_SAD_U16 VDST, SRC0, SRC1, SRC2  
Description: Calculate sum of absolute differences for two 16-bit words in SRC0 and SRC1,
add SRC2 to result, and store result to VDST.  
Operation:  
```
VDST = SRC2
VDST += ABS((SRC0 & 0xffff) - (SRC1 & 0xffff))
VDST += ABS((SRC0 >> 16) - (SRC1 >> 16))
```

#### V_SAD_U32

Opcode: 349 (0x15d) for GCN 1.0/1.1; 476 (0x1dc) for GCN 1.2  
Syntax: V_SAD_U32 VDST, SRC0, SRC1, SRC2  
Description: Calculate sum of absolute difference for SRC0 and SRC1, add
SRC2 to result, and store result to VDST.  
Operation:  
```
VDST = SRC2 + ABS(SRC0 - SRC1)
```

#### V_SAD_U8

Opcode: 346 (0x15a) for GCN 1.0/1.1; 473 (0x1d9) for GCN 1.2  
Syntax: V_SAD_U8 VDST, SRC0, SRC1, SRC2  
Description: Calculate sum of absolute differences for all four bytes in SRC0 and SRC1, add
SRC2 to result, and store result to VDST.  
Operation:  
```
VDST = SRC2
for (UINT8 i = 0; i < 4; i++)
    VDST += ABS(((SRC0 >> (i*8)) & 0xff) - ((SRC1 >> (i*8)) & 0xff))
```
