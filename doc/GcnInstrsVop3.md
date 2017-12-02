## GCN ISA VOP3 instructions

The VOP3 instructions requires two dword to store in program code. By default, these
encoding of these instructions gives all features of the VOP3 encoding: all possible
modifiers, any source operand combination.

In an assembler's syntax you can force this encoding by using `VOP3` modifier in instruction.

List of fields for the VOP3A/VOP3B encoding (GCN 1.0/1.1):

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
11-14 | OP_SEL   | Operand selection (VOP3A) (GCN 1.4)
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
* ABS(SRC), |SRC| - apply absolute value to source operand
* OP_SEL:VALUE|[B0,...] - operand half selection (0 - lower 16-bits, 1 - bits)

NOTE: OMOD modifier doesn't work if output denormals are allowed
(5 bit of MODE register for single precision or 7 bit for double precision).  
NOTE: OMOD and CLAMP modifier affects only for instruction that output is
floating point value or for addition/subtraction instructions.  
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

List of the instructions by opcode (GCN 1.0/1.1):

 Opcode      | Mnemonic (GCN 1.0) | Mnemonic (GCN 1.1)
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

List of the instructions by opcode (GCN 1.2/1.4):

 Opcode      | Mnemonic (GCN 1.2)      | Mnemonic (GCN 1.4)
-------------|-------------------------|-------------------------
 448 (0x1c0) | V_MAD_LEGACY_F32        | V_MAD_LEGACY_F32
 449 (0x1c1) | V_MAD_F32               | V_MAD_F32
 450 (0x1c2) | V_MAD_I32_I24           | V_MAD_I32_I24
 451 (0x1c3) | V_MAD_U32_U24           | V_MAD_U32_U24
 452 (0x1c4) | V_CUBEID_F32            | V_CUBEID_F32
 453 (0x1c5) | V_CUBESC_F32            | V_CUBESC_F32
 454 (0x1c6) | V_CUBETC_F32            | V_CUBETC_F32
 455 (0x1c7) | V_CUBEMA_F32            | V_CUBEMA_F32
 456 (0x1c8) | V_BFE_U32               | V_BFE_U32
 457 (0x1c9) | V_BFE_I32               | V_BFE_I32
 458 (0x1ca) | V_BFI_B32               | V_BFI_B32
 459 (0x1cb) | V_FMA_F32               | V_FMA_F32
 460 (0x1cc) | V_FMA_F64               | V_FMA_F64
 461 (0x1cd) | V_LERP_U8               | V_LERP_U8
 462 (0x1ce) | V_ALIGNBIT_B32          | V_ALIGNBIT_B32
 463 (0x1cf) | V_ALIGNBYTE_B32         | V_ALIGNBYTE_B32
 464 (0x1d0) | V_MIN3_F32              | V_MIN3_F32
 465 (0x1d1) | V_MIN3_I32              | V_MIN3_I32
 466 (0x1d2) | V_MIN3_U32              | V_MIN3_U32
 467 (0x1d3) | V_MAX3_F32              | V_MAX3_F32
 468 (0x1d4) | V_MAX3_I32              | V_MAX3_I32
 469 (0x1d5) | V_MAX3_U32              | V_MAX3_U32
 470 (0x1d6) | V_MED3_F32              | V_MED3_F32
 471 (0x1d7) | V_MED3_I32              | V_MED3_I32
 472 (0x1d8) | V_MED3_U32              | V_MED3_U32
 473 (0x1d9) | V_SAD_U8                | V_SAD_U8
 474 (0x1da) | V_SAD_HI_U8             | V_SAD_HI_U8
 475 (0x1db) | V_SAD_U16               | V_SAD_U16
 476 (0x1dc) | V_SAD_U32               | V_SAD_U32
 477 (0x1dd) | V_CVT_PK_U8_F32         | V_CVT_PK_U8_F32
 478 (0x1de) | V_DIV_FIXUP_F32         | V_DIV_FIXUP_F32
 479 (0x1df) | V_DIV_FIXUP_F64         | V_DIV_FIXUP_F64
 480 (0x1e0) | V_DIV_SCALE_F32 (VOP3B) | V_DIV_SCALE_F32 (VOP3B)
 481 (0x1e1) | V_DIV_SCALE_F64 (VOP3B) | V_DIV_SCALE_F64 (VOP3B)
 482 (0x1e2) | V_DIV_FMAS_F32          | V_DIV_FMAS_F32
 483 (0x1e3) | V_DIV_FMAS_F64          | V_DIV_FMAS_F64
 484 (0x1e4) | V_MSAD_U8               | V_MSAD_U8
 485 (0x1e5) | V_QSAD_PK_U16_U8        | V_QSAD_PK_U16_U8
 486 (0x1e6) | V_MQSAD_PK_U16_U8       | V_MQSAD_PK_U16_U8
 487 (0x1e7) | V_MQSAD_U32_U8          | V_MQSAD_U32_U8
 488 (0x1e8) | V_MAD_U64_U32 (VOP3B)   | V_MAD_U64_U32 (VOP3B)
 489 (0x1e9) | V_MAD_I64_I32 (VOP3B)   | V_MAD_I64_I32 (VOP3B)
 490 (0x1ea) | V_MAD_F16               | V_MAD_LEGACY_F16
 491 (0x1eb) | V_MAD_U16               | V_MAD_LEGACY_U16
 492 (0x1ec) | V_MAD_I16               | V_MAD_LEGACY_I16
 493 (0x1ed) | V_PERM_B32              | V_PERM_B32
 494 (0x1ee) | V_FMA_F16               | V_FMA_LEGACY_F16
 495 (0x1ef) | V_DIV_FIXUP_F16         | V_DIV_FIXUP_LEGACY_F16
 496 (0x1f0) | V_CVT_PKACCUM_U8_F32    | V_CVT_PKACCUM_U8_F32
 497 (0x1f1) | --                      | V_MAD_U32_U16
 498 (0x1f2) | --                      | V_MAD_I32_I16
 499 (0x1f3) | --                      | V_XAD_U32
 500 (0x1f4) | --                      | V_MIN3_F16
 501 (0x1f5) | --                      | V_MIN3_I16
 502 (0x1f6) | --                      | V_MIN3_U16
 503 (0x1f7) | --                      | V_MAX3_F16
 504 (0x1f8) | --                      | V_MAX3_I16
 505 (0x1f9) | --                      | V_MAX3_U16
 506 (0x1fa) | --                      | V_MED3_F16
 507 (0x1fb) | --                      | V_MED3_I16
 508 (0x1fc) | --                      | V_MED3_U16
 509 (0x1fd) | --                      | V_LSHL_ADD_U32
 510 (0x1fe) | --                      | V_ADD_LSHL_U32
 511 (0x1ff) | --                      | V_ADD3_U32
 512 (0x200) | --                      | V_LSHL_OR_B32
 513 (0x201) | --                      | V_AND_OR_B32
 514 (0x202) | --                      | V_OR3_B32
 515 (0x203) | --                      | V_MAD_F16
 516 (0x204) | --                      | V_MAD_U16
 517 (0x205) | --                      | V_MAD_I16
 518 (0x206) | --                      | V_FMA_F16
 519 (0x207) | --                      | V_DIV_FIXUP_F16
 624 (0x270) | V_INTERP_P1_F32 (VINTRP) | V_INTERP_P1_F32 (VINTRP)
 625 (0x271) | V_INTERP_P2_F32 (VINTRP) | V_INTERP_P2_F32 (VINTRP)
 626 (0x272) | V_INTERP_MOV_F32 (VINTRP) | V_INTERP_MOV_F32 (VINTRP)
 628 (0x274) | V_INTERP_P1LL_F16 (VINTRP) | V_INTERP_P1LL_F16 (VINTRP)
 629 (0x275) | V_INTERP_P1LV_F16 (VINTRP) | V_INTERP_P1LV_F16 (VINTRP)
 630 (0x276) | V_INTERP_P2_F16 (VINTRP)| V_INTERP_P2_F16_LEGACY (VINTRP)
 631 (0x277) | --                      | V_INTERP_P2_F16 (VINTRP)
 640 (0x280) | V_ADD_F64               | V_ADD_F64
 641 (0x281) | V_MUL_F64               | V_MUL_F64
 642 (0x282) | V_MIN_F64               | V_MIN_F64
 643 (0x283) | V_MAX_F64               | V_MAX_F64
 644 (0x284) | V_LDEXP_F64             | V_LDEXP_F64
 645 (0x285) | V_MUL_LO_U32            | V_MUL_LO_U32
 646 (0x286) | V_MUL_HI_U32            | V_MUL_HI_U32
 647 (0x287) | V_MUL_HI_I32            | V_MUL_HI_I32
 648 (0x288) | V_LDEXP_F32             | V_LDEXP_F32
 649 (0x289) | V_READLANE_B32          | V_READLANE_B32
 650 (0x28a) | V_WRITELANE_B32         | V_WRITELANE_B32
 651 (0x28b) | V_BCNT_U32_B32          | V_BCNT_U32_B32
 652 (0x28c) | V_MBCNT_LO_U32_B32      | V_MBCNT_LO_U32_B32
 653 (0x28d) | V_MBCNT_HI_U32_B32      | V_MBCNT_HI_U32_B32
 654 (0x28e) | V_MAC_LEGACY_F32        | V_MAC_LEGACY_F32
 655 (0x28f) | V_LSHLREV_B64           | V_LSHLREV_B64
 656 (0x290) | V_LSHRREV_B64           | V_LSHRREV_B64
 657 (0x291) | V_ASHRREV_I64           | V_ASHRREV_I64
 658 (0x292) | V_TRIG_PREOP_F64        | V_TRIG_PREOP_F64
 659 (0x293) | V_BFM_B32               | V_BFM_B32
 660 (0x294) | V_CVT_PKNORM_I16_F32    | V_CVT_PKNORM_I16_F32
 661 (0x295) | V_CVT_PKNORM_U16_F32    | V_CVT_PKNORM_U16_F32
 662 (0x296) | V_CVT_PKRTZ_F16_F32     | V_CVT_PKRTZ_F16_F32
 663 (0x297) | V_CVT_PK_U16_U32        | V_CVT_PK_U16_U32
 664 (0x298) | V_CVT_PK_I16_I32        | V_CVT_PK_I16_I32
 665 (0x299) | V_CVT_PKNORM_I16_F16    | V_CVT_PKNORM_I16_F16
 666 (0x29a) | V_CVT_PKNORM_U16_F16    | V_CVT_PKNORM_U16_F16
 667 (0x29b) | V_READLANE_REGRD_B32    | V_READLANE_REGRD_B32
 668 (0x29c) | --                      | V_ADD_I32
 669 (0x29d) | --                      | V_SUB_I32
 670 (0x29e) | --                      | V_ADD_I16
 671 (0x29f) | --                      | V_SUB_I16
 672 (0x2a0) | --                      | V_PACK_B32_F16

### Instruction set

Alphabetically sorted instruction list:

#### V_ADD_F64

Opcode: 356 (0x164) for GCN 1.0/1.1; 640 (0x280) for GCN 1.2/1.4  
Syntax: V_ADD_F64 VDST(2), SRC0(2), SRC1(2)  
Description: Add two double FP value from SRC0 and SRC1 and store result to VDST.  
Operation:  
```
VDST = ASDOUBLE(SRC0) + ASDOUBLE(SRC1)
```

#### V_ADD_I16

Opcode: 670 (0x29e) for GCN 1.4  
Syntax: V_ADD_I16 VDST, SRC0, SRC1  
Description: Add 16-bit signed value from SRC0 to 16-bit signed value from SRC1 and
store result to VDST. If CLAMP modifier supplied, then result is saturated to
16-bit signed value.  
Operation:  
```
UINT16 result = (SRC0&0xffff) + (SRC1&0xffff)
if (CLAMP)
{
    INT32 temp = SEXT32((INT16)SRC0&0xffff) + SEXT32((INT16)SRC1&0xffff)
    if (temp > ((1<<16)-1))
        result = 0x7fff
    if temp < (-1<<16)
        result = 0x8000
}
VDST = (VDST & 0xffff0000) | result
```

#### V_ADD_I32

Opcode: 668 (0x29c) for GCN 1.4  
Syntax: V_ADD_I32 VDST, SRC0, SRC1  
Description: Add signed value from SRC0 to signed value from SRC1 and store result to VDST.
If CLAMP modifier supplied, then result is saturated to 32-bit signed value.  
Operation:  
```
VDST = SRC0 + SRC1
if (CLAMP)
{
    INT64 temp = SEXT64(SRC0) + SEXT64(SRC1)
    if (temp > ((1LL<<31)-1))
        VDST = 0x7fffffff
    if temp < (-1LL<<31)
        VDST = 0x80000000
}
```

#### V_ADD3_U32

Opcode: 511 (0x1ff) for GCN 1.4  
Syntax: V_ADD3_U32 VDST, SRC0, SRC1, SRC2  
Description: Make sum from SRC0, SRC1, and SRC2 and store final result to VDST.  
Operation:  
```
VDST = SRC0 + SRC1 + SRC2
```

#### V_ADD_LSHL_U32

Opcode: 510 (0x1fe) for GCN 1.4  
Syntax: V_ADD_LSHL_U32 VDST, SRC0, SRC1, SRC2  
Description: Add SRC0 and SRC1 and shift left by (SRC2&31) bits and store result to VDST.  
Operation:  
```
VDST = (SRC0 + SRC1) << (SRC2&31)
```

#### V_ALIGNBIT_B32

Opcode: 334 (0x14e) for GCN 1.0/1.1; 462 (0x1ce) for GCN 1.2/1.4  
Syntax: V_ALIGNBIT_B32 VDST, SRC0, SRC1, SRC2  
Description: Align bit. Shift right bits in 64-bit stored in SRC1 (low part) and
SRC0 (high part) by SRC2&31 bits, and store low 32-bit of the result in VDST.  
Operation:  
```
VDST = (((UINT64)SRC0)<<32) | SRC1) >> (SRC2&31)
```

#### V_ALIGNBYTE_B32

Opcode: 335 (0x14f) for GCN 1.0/1.1; 463 (0x1cf) for GCN 1.2/1.4  
Syntax: V_ALIGNBYTE_B32 VDST, SRC0, SRC1, SRC2  
Description: Align bit. Shift right bits in 64-bit stored in SRC1 (low part) and
SRC0 (high part) by (SRC2&3)*8 bits, and store low 32-bit of the result in VDST.  
Operation:  
```
VDST = (((UINT64)SRC0)<<32) | SRC1) >> ((SRC2&3)*8)
```

#### V_AND_OR_B32

Opcode: 513 (0x201) for GCN 1.4  
Syntax: V_AND_OR_B32 VDST, SRC0, SRC1, SRC2  
Description: Make btwise AND with SRC0 and SRC1, make bitwise OR with result and SRC2
and store result to VDST.  
Operation:  
```
VDST = (SRC0 & SRC1) | SRC2
```

#### V_ASHR_I64

Opcode: 355 (0x163) for GCN 1.0/1.1  
Syntax: V_ASHR_I32 VDST(2), SRC0(2), SRC1  
Description: Arithmetic shift right SRC0 by (SRC1&63) bits and store result into VDST.  
Operation:  
```
VDST = (INT64)SRC0 >> (SRC1&63)
```

#### V_ASHRREV_I64

Opcode: 657 (0x291) for GCN 1.2/1.4  
Syntax: V_ASHRREV_I32 VDST(2), SRC0, SRC1(2)  
Description: Arithmetic shift right SRC1 by (SRC0&63) bits and store result into VDST.  
Operation:  
```
VDST = (INT64)SRC0 >> (SRC0&63)
```

#### V_BCNT_U32_B32

Opcode: 651 (0x28b) for GCN 1.2/1.4  
Syntax: V_BCNT_U32_B32 VDST, SRC0, SRC1  
Description: Count bits in SRC0, adds SRC1, and store result to VDST.  
Operation:  
```
VDST = SRC1 + BITCOUNT(SRC0)
```

#### V_BFE_I32

Opcode: 329 (0x149) for GCN 1.0/1.1; 457 (0x1c9) for GCN 1.2/1.4  
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

Opcode: 328 (0x148) for GCN 1.0/1.1; 456 (0x1c8) for GCN 1.2/1.4  
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

Opcode: 330 (0x14a) for GCN 1.0/1.1; 458 (0x1ca) for GCN 1.2/1.4  
Syntax: V_BFI_B32 VDST, SRC0, SRC1, SRC2  
Description: Replace bits in SRC2 by bits from SRC1 marked by bits in SRC0, and store result
to VDST.  
Operation:  
```
VDST = (SRC0 & SRC1) | (~SRC0 & SRC2)
```

#### V_BFM_B32

Opcode: 659 (0x293) for GCN 1.2/1.4  
Syntax: V_BFM_B32 VDST, SRC0, SRC1  
Description: Make 32-bit bitmask from (SRC1 & 31) bit that have length (SRC0 & 31) and
store it to VDST.  
Operation:  
```
VDST = ((1U << (SRC0&31))-1) << (SRC1&31)
```

#### V_CUBEID_F32

Opcode: 324 (0x144) for GCN 1.0/1.1; 452 (0x1c4) for GCN 1.2/1.4  
Syntax: V_CUBEID_F32 VDST, SRC0, SRC1, SRC2  
Description: Cubemap face identification. Determine face by comparing three single FP
values: SRC0 (X), SRC1 (Y), SRC2(Z). Choose highest absolute value and check whether is
negative or positive. Store floating point value of face ID: (DIM*2.0)+(V[DIM]>=0?1:0),
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

Opcode: 327 (0x147) for GCN 1.0/1.1; 455 (0x1c7) for GCN 1.2/1.4  
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

Opcode: 325 (0x145) for GCN 1.0/1.1; 453 (0x1c5) for GCN 1.2/1.4  
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

Opcode: 326 (0x146) for GCN 1.0/1.1; 454 (0x1c6) for GCN 1.2/1.4  
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

#### V_CVT_PK_I16_I32

Opcode: 664 (0x298) for GCN 1.2/1.4  
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

Opcode: 663 (0x297) for GCN 1.2/1.4  
Syntax: V_CVT_PK_U16_U32 VDST, SRC0, SRC1  
Description: Convert unsigned value from SRC0 and SRC1 to unsigned 16-bit values with
clamping, and store first value to low 16-bit and second to high 16-bit of the VDST.  
Operation:  
```
UINT16 D0 = MIN(SRC0, 0xffff)
UINT16 D1 = MIN(SRC1, 0xffff)
VDST = D0 | (((UINT32)D1) << 16)
```

#### V_CVT_PK_U8_F32

Opcode: 350 (0x15e) for GCN 1.0/1.1; 477 (0x1dd) for GCN 1.2/1.4  
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

#### V_CVT_PKACCUM_U8_F32

Opcode: 496 (0x1f0) for GCN 1.2/1.4  
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

#### V_CVT_PKNORM_I16_F16

Opcode: 665 (0x299) for GCN 1.4  
Syntax: V_CVT_PKNORM_I16_F16 VDST, SRC0, SRC1  
Description: Convert normalized half FP value from SRC0 and SRC1 to
signed 16-bit integers with rounding to nearest to even (??),
and store first value to low 16-bit and second to high 16-bit of the VDST.  
Operation:  
```
INT16 roundNorm(HALF S)
{
    FLOAT f = RNDNEINT(S*32767)
    if (ISNAN(f))
        return 0
    return (INT16)MAX(MIN(f, 32767.0), -32767.0)
}
VDST = roundNorm(ASHALF(SRC0)) | ((UINT32)roundNorm(ASHALF(SRC1)) << 16)
```

#### V_CVT_PKNORM_I16_F32

Opcode: 660 (0x294) for GCN 1.2/1.4  
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

#### V_CVT_PKNORM_U16_F16

Opcode: 666 (0x29a) for GCN 1.4  
Syntax: V_CVT_PKNORM_U16_F16 VDST, SRC0, SRC1  
Description: Convert normalized half FP value from SRC0 and SRC1 to
unsigned 16-bit integers with rounding to nearest to even (??),
and store first value to low 16-bit and second to high 16-bit of the VDST.  
Operation:  
```
UINT16 roundNorm(HALF S)
{
    HALF f = RNDNEINT(S*65535.0)
    if (ISNAN(f))
        return 0
    return (INT16)MAX(MIN(f, 65535.0), 0.0)
}
VDST = roundNorm(ASHALF(SRC0)) | ((UINT32)roundNorm(ASHALF(SRC1)) << 16)
```

#### V_CVT_PKNORM_U16_F32

Opcode: 661 (0x295) for GCN 1.2/1.4  
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

Opcode: 662 (0x296) for GCN 1.2/1.4  
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

#### V_DIV_FIXUP_F16

Opcode: 495 (0x1ef) for GCN 1.2; 519 (0x207) for GCN 1.4  
Syntax: V_DIV_FIXUP_F16 VDST, SRC0, SRC1, SRC2  
Description: Handle all exceptions requires for half floating point division.
SRC0 is quotient, SRC1 is denominator, SRC2 is nominator. Correct result stored to VDST.  
Operation:  
```
HALF SF0 = ASHALF(SRC0)
HALF SF1 = ASHALF(SRC1)
HALF SF2 = ASHALF(SRC2)
if (ISNAN(SF1) && !ISNAN(SF2))
    VDST = QUIETNAN(SF1)
else if (ISNAN(SF2))
    VDST = QUIETNAN(SF2)
else if (SF1 == 0.0 && SF2 == 0.0)
    VDST = NAN_H
else if (ABS(SF1)==INF && ABS(SF2)==INF)
    VDST = -NAN_H
else if (SF1 == 0.0)
    VDST = INF_H*SIGN(SF1)*SIGN(SF2)
else if (ABS(SF1) == INF)
    VDST = SIGN(SF1)*SIGN(SF2) >=0 ? 0.0 : -0.0
else if (ISNAN(SF0))
    VDST = SIGN(SF1)*SIGN(SF2)*INF_H
else
    VDST = SF0
```

#### V_DIV_FIXUP_F32

Opcode: 351 (0x15f) for GCN 1.0/1.1; 478 (0x1de) for GCN 1.2/1.4  
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

Opcode: 352 (0x160) for GCN 1.0/1.1; 479 (0x1df) for GCN 1.2/1.4  
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

#### V_DIV_FIXUP_LEGACY_F16

Opcode: 495 (0x1ef) for GCN 1.4  
Syntax: V_DIV_FIXUP_LEGACY_F16 VDST, SRC0, SRC1, SRC2  
Description: Handle all exceptions requires for half floating point division.
SRC0 is quotient, SRC1 is denominator, SRC2 is nominator. Correct result stored to VDST.  
Operation:  
```
HALF SF0 = ASHALF(SRC0)
HALF SF1 = ASHALF(SRC1)
HALF SF2 = ASHALF(SRC2)
if (ISNAN(SF1) && !ISNAN(SF2))
    VDST = QUIETNAN(SF1)
else if (ISNAN(SF2))
    VDST = QUIETNAN(SF2)
else if (SF1 == 0.0 && SF2 == 0.0)
    VDST = NAN_H
else if (ABS(SF1)==INF && ABS(SF2)==INF)
    VDST = -NAN_H
else if (SF1 == 0.0)
    VDST = INF_H*SIGN(SF1)*SIGN(SF2)
else if (ABS(SF1) == INF)
    VDST = SIGN(SF1)*SIGN(SF2) >=0 ? 0.0 : -0.0
else if (ISNAN(SF0))
    VDST = SIGN(SF1)*SIGN(SF2)*INF_H
else
    VDST = SF0
```

#### V_DIV_FMAS_F32

Opcode: 367 (0x16f) for GCN 1.0/1.1; 482 (0x1e2) for GCN 1.2/1.4  
Syntax: V_DIV_FMAS_F32 VDST, SRC0, SRC1, SRC2  
Description: Special case divide FMA with scale and flags.
SRC0 is quotient, SRC1 is denominator, SRC2 is nominator.
All input values are floating point values. Instruction does fussed multiply and addition,
multiply result by POW(2.0, -64) if absolute value of the SRC2 less than 2.0,
otherwise multiply by POW(2.0, 64); and store result to VDST.  
Operation:  
```
// SRC0*SRC1+SRC2
VDST = FMA(ASFLOAT(SRC0), ASFLOAT(SRC1), ASFLOAT(SRC2))
if (ABS(ASFLOAT(SRC2)) >= 2.0)
    VDST = ASFLOAT(VDST)*POW(2.0,64)
else
    VDST = ASFLOAT(VDST)*POW(-2.0,64)
```

#### V_DIV_FMAS_F64

Opcode: 368 (0x170) for GCN 1.0/1.1; 483 (0x1e3) for GCN 1.2/1.4  
Syntax: V_DIV_FMAS_F64 VDST(2), SRC0(2), SRC1(2), SRC2(2)  
Description: Special case divide FMA with scale and flags.
SRC0 is quotient, SRC1 is denominator, SRC2 is nominator.
All input values are double floating point values.
Instruction does fussed multiply and addition,
multiply result by POW(2.0, -128) if absolute value of the SRC2 less than 2.0,
otherwise multiply by POW(2.0, 128); and store result to VDST.  
Operation:  
```
// SRC0*SRC1+SRC2
VDST = FMA(ASDOUBLE(SRC0), ASDOUBLE(SRC1), ASDOUBLE(SRC2))
if (ABS(ASDOUBLE(SRC2)) >= 2.0)
    VDST = ASDOUBLE(VDST)*POW(2.0,128)
else
    VDST = ASDOUBLE(VDST)*POW(-2.0,128)
```

#### V_DIV_SCALE_F32

Opcode (VOP3B): 365 (0x16d) for GCN 1.0/1.1; 480 (0x1e0) for GCN 1.2/1.4  
Syntax: V_DIV_SCALE_F32 VDST, SDST(2), SRC0, SRC1, SRC2  
Description: Special case divide preop and flags. SRC0 is quotient, SRC1 is denominator,
SRC2 is nominator. All input values are floating point values. SRC0 must be equal SRC1
or SRC2 (register can be different, only values must be equal). If absolute value of
the input different than SRC0 is greater or equal than T2=POW(2.0, 96+EXP0)
(EXP0 is exponent part (base 2) of SRC0) or this value is NaN,
then instruction multiply SRC0 by POW(2.0, 64),
and store that value to VDST, and set flag in bit for current lane in SDST.
If SRC0 is NaN or infinity then store SRC0 to VDST and set flag.
Otherwise store SRC0 to VDST and clear flag.
Bits for inactive threads in SDST are always zeroed.  
Operation:  
```
FLOAT SF0 = ASFLOAT(SRC0)
FLOAT SF1 = ASFLOAT(SRC1)
FLOAT SF2 = ASFLOAT(SRC2)
FLOAT S12 = (SRC0!=SRC1) ? SF1 : SF2
SDST = 0
if (ISNAN(SF0) || ABS(SF0) == INF)
{
    VDST = SRC0
    SDST = (SDST & ~MASK) | MASK
}
else if (ABS(S12) >= ABS(POW(2.0, 96+FREXP_EXP(SF0)-1) || ISNAN(S12))
{
    VDST = SF0 * POW2(2.0, 64)
    UINT64 MASK = (1ULL<<LANEID)
    SDST = (SDST & ~MASK) | MASK
}
else
{
    VDST = SRC0
    SDST = (SDST & ~MASK)
}
```

#### V_DIV_SCALE_F64

Opcode (VOP3B): 366 (0x16e) for GCN 1.0/1.1; 481 (0x1e1) for GCN 1.2/1.4  
Syntax: V_DIV_SCALE_F64 VDST(2), SDST(2), SRC0(2), SRC1(2), SRC2(2)  
Description: Special case divide preop and flags. SRC0 is quotient, SRC1 is denominator,
SRC2 is nominator. All input values are double floating point values.
SRC0 must be equal SRC1 or SRC2 (register can be different, only values must be equal).
If absolute value of the input different than SRC0 is greater or equal than
T2=POW(2.0, 768+EXP0) (EXP0 is exponent part (base 2) of SRC0) or this value is NaN,
then instruction multiply SRC0 by POW(2.0, 128),
and store that value to VDST, and set flag in bit for current lane in SDST.
If SRC0 is NaN or infinity then store SRC0 to VDST and set flag.
Otherwise store SRC0 to VDST and clear flag.
Bits for inactive threads in SDST are always zeroed.  
Operation:  
```
DOUBLE SD0 = ASDOUBLE(SRC0)
DOUBLE SD1 = ASDOUBLE(SRC1)
DOUBLE SD2 = ASDOUBLE(SRC2)
DOUBLE S12 = (SRC0!=SRC1) ? SD1 : SD2
UINT64 MASK = (1ULL<<LANEID)
SDST = 0
if (ISNAN(SD0) || ABS(SD0) == INF)
{
    VDST = SRC0
    SDST = (SDST & ~MASK) | MASK
}
else if (ABS(S12) >= ABS(POW(2.0, 768+FREXP_EXP(SD0)-1) || ISNAN(S12))
{
    VDST = SD0 * POW2(2.0, 128)
    SDST = (SDST & ~MASK) | MASK
}
else
{
    VDST = SRC0
    SDST = (SDST & ~MASK)
}
```

#### V_FMA_F16

Opcode: 494 (0x1ee) for GCN 1.2; 518 (0x206) for GCN 1.4  
Syntax: V_FMA_F16 VDST, SRC0, SRC1, SRC2  
Description: Fused multiply addition on half floating point values from
SRC0, SRC1 and SRC2. Result stored in VDST.  
Operation:  
```
// SRC0*SRC1+SRC2
VDST = FMA(ASHALF(SRC0), ASHALF(SRC1), ASHALF(SRC2))
```

#### V_FMA_F32

Opcode: 331 (0x14b) for GCN 1.0/1.1; 459 (0x1cb) for GCN 1.2/1.4  
Syntax: V_FMA_F32 VDST, SRC0, SRC1, SRC2  
Description: Fused multiply addition on single floating point values from
SRC0, SRC1 and SRC2. Result stored in VDST.  
Operation:  
```
// SRC0*SRC1+SRC2
VDST = FMA(ASFLOAT(SRC0), ASFLOAT(SRC1), ASFLOAT(SRC2))
```

#### V_FMA_F64

Opcode: 332 (0x14c) for GCN 1.0/1.1; 460 (0x1cc) for GCN 1.2/1.4  
Syntax: V_FMA_F64 VDST(2), SRC0(2), SRC1(2), SRC2(2)  
Description: Fused multiply addition on double floating point values from
SRC0, SRC1 and SRC2. Result stored in VDST.  
Operation:  
```
// SRC0*SRC1+SRC2
VDST = FMA(ASDOUBLE(SRC0), ASDOUBLE(SRC1), ASDOUBLE(SRC2))
```

#### V_FMA_LEGACY_F16

Opcode: 494 (0x1ee) for GCN 1.4  
Syntax: V_FMA_LEGACY_F16 VDST, SRC0, SRC1, SRC2  
Description: Fused multiply addition on half floating point values from
SRC0, SRC1 and SRC2. Result stored in VDST.  
Operation:  
```
// SRC0*SRC1+SRC2
VDST = FMA(ASHALF(SRC0), ASHALF(SRC1), ASHALF(SRC2))
```

#### V_INTERP_MOV_F32

Opcode: 626 (0x272) for GCN 1.2/1.4  
Syntax: V_INTERP_MOV_F32 VDST, PARAMTYPE, ATTR.ATTRCHAN  
Description: Move parameter value into VDST. The PARAMTYPE is P0, P10 or P20.
Refer to [VINTRP instructions](GcnInstrsVintrp).  
NOTE: The indices in LDS is dword indices.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
if (PARAMTYPE==P0)
    VDST[LANEID] = ASFLOAT(LDS[S + ATTRCHAN*2])
else if (PARAMTYPE==P10)
    VDST[LANEID] = ASFLOAT(LDS[S + ATTRCHAN*2 + 1])
else if (PARAMTYPE==P20)
    VDST[LANEID] = ASFLOAT(LDS[S + ATTRCHAN + 8])
```

#### V_INTERP_P1_F32

Opcode: 624 (0x270) for GCN 1.2/1.4  
Syntax: V_INTERP_P1_F32 VDST, VSRC, ATTR.ATTRCHAN  
Description: Instruction does the first step of the interpolation (P0 + P10*I). The I
coordinate given in VSRC register. Refer to [VINTRP instructions](GcnInstrsVintrp).  
NOTE: The indices in LDS is dword indices.  
NOTE: VDST and VSRC registers must not be same.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
FLOAT P0[LANEID] = ASFLOAT(LDS[S + ATTRCHAN*2])
FLOAT P10[LANEID] = ASFLOAT(LDS[S + ATTRCHAN*2 + 1])
VDST[LANEID] = P0[LANEID] + ASFLOAT(VSRC[LANEID]) * P10[LANEID]
```

#### V_INTERP_P1LL_F16

Opcode: 628 (0x274) for GCN 1.2/1.4  
Syntax: V_INTERP_P1LL_F16 VDST, VSRC, ATTR.ATTRCHAN [HIGH]  
Description: Instruction does the first step of the interpolation (P0 + P10*I). The I
coordinate given in VSRC register. P0 and P10 factors are half floating point values stored
in lower or higher (if 'HIGH' given) part of 32-bit dword.
Refer to [VINTRP instructions](GcnInstrsVintrp).  
NOTE: The indices in LDS is dword indices.  
NOTE: VDST and VSRC registers must not be same.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
HALF P0[LANEID], P10[LANEID]
if (HIGH)
{
    P0[LANEID] = ASHALF(LDS[S + ATTRCHAN*2] >> 16)
    P10[LANEID] = ASHALF(LDS[S + ATTRCHAN*2 + 1] >> 16)
}
else
{
    P0[LANEID] = ASHALF(LDS[S + ATTRCHAN*2] & 0xffff)
    P10[LANEID] = ASHALF(LDS[S + ATTRCHAN*2 + 1] & 0xffff)
}
VDST[LANEID] = P0[LANEID] + ASFLOAT(VSRC[LANEID]) * P10[LANEID]
```

#### V_INTERP_P1LV_F16

Opcode: 629 (0x275) for GCN 1.2/1.4  
Syntax: V_INTERP_P1LL_F16 VDST, VSRC, ATTR.ATTRCHAN, VSRC1 [HIGH]  
Description: Instruction does the first step of the interpolation (P0 + P10*I). The I
coordinate given in VSRC register. P10 and P0 factors are half floating point values stored
in lower or higher (if 'HIGH' given) part of 32-bit dword. P0 is stored in VSRC1.
Refer to [VINTRP instructions](GcnInstrsVintrp).  
NOTE: The indices in LDS is dword indices.  
NOTE: VDST and VSRC registers must not be same.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
HALF P0[LANEID], P10[LANEID]
if (HIGH)
{
    P0[LANEID] = ASHALF(VSRC1[LANEID] >> 16)
    P10[LANEID] = ASHALF(LDS[S + ATTRCHAN*2 + 1] >> 16)
}
else
{
    P0[LANEID] = ASHALF(VSRC1[LANEID] & 0xffff)
    P10[LANEID] = ASHALF(LDS[S + ATTRCHAN*2 + 1] & 0xffff)
}
VDST[LANEID] = P0 + ASFLOAT(VSRC[LANEID]) * P10[LANEID]
```

#### V_INTERP_P2_F32

Opcode: 625 (0x271) for GCN 1.2/1.4  
Syntax: V_INTERP_P1_F32 VDST, VSRC, ATTR.ATTRCHAN  
Description: Instruction does the second step of the interpolation (P20*J + D). The J
coordinate given in VSRC register. Refer to [VINTRP instructions](GcnInstrsVintrp).  
NOTE: The indices in LDS is dword indices.  
NOTE: VDST and VSRC registers must not be same.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
FLOAT P20[LANEID] = ASFLOAT(LDS[S + ATTRCHAN + 8])
VDST[LANEID] = ASFLOAT(VDST[LANEID]) + ASFLOAT(VSRC[LANEID]) * P20[LANEID]
```

#### V_INTERP_P2_F16 (GCN 1.2)

Opcode: 630 (0x276) for GCN 1.2/1.4  
Syntax: V_INTERP_P1_F16 VDST, VSRC, ATTR.ATTRCHAN, VSRC1 [HIGH]  
Syntax (GCN 1.4): V_INTERP_P1_F16_LEGACY VDST, VSRC, ATTR.ATTRCHAN [HIGH], VSRC1  
Description: Instruction does the second step of the interpolation (P20*J + D). The J
coordinate given in VSRC register. P2 factor is half floating point value stored in
lower or higher (if HIGH given) part of dword.
Refer to [VINTRP instructions](GcnInstrsVintrp).  
NOTE: The indices in LDS is dword indices.  
NOTE: VDST and VSRC registers must not be same.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
HALF P20[LANEID]
if (HIGH)
    HALF P20[LANEID] = ASFLOAT(LDS[S + ATTRCHAN + 8] >> 16)
else
    HALF P20[LANEID] = ASFLOAT(LDS[S + ATTRCHAN + 8] & 0xffff)
VDST[LANEID] = ASFLOAT(VSRC1[LANEID]) + ASFLOAT(VSRC[LANEID]) * P20[LANEID]
```

#### V_INTERP_P2_F16 (GCN 1.4)

Opcode: 631 (0x277) for GCN 1.4  
Syntax: V_INTERP_P1_F16 VDST, VSRC, ATTR.ATTRCHAN, VSRC1 [HIGH]  
Description: Instruction does the second step of the interpolation (P20*J + D). The J
coordinate given in VSRC register. P2 factor is half floating point value stored in
lower or higher (if HIGH given) part of dword.
The 3-bit in OPSEL choose 16-bit part of destination (other part is preserved).
Refer to [VINTRP instructions](GcnInstrsVintrp).  
NOTE: The indices in LDS is dword indices.  
NOTE: VDST and VSRC registers must not be same.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
HALF P20[LANEID]
if (HIGH)
    HALF P20[LANEID] = ASFLOAT(LDS[S + ATTRCHAN + 8] >> 16)
else
    HALF P20[LANEID] = ASFLOAT(LDS[S + ATTRCHAN + 8] & 0xffff)
VDST[LANEID] = ASFLOAT(VSRC1[LANEID]) + ASFLOAT(VSRC[LANEID]) * P20[LANEID]
```

#### V_LDEXP_F32

Opcode: 648 (0x288) for GCN 1.2/1.4  
Syntax: V_LDEXP_F32 VDST, SRC0, SRC1  
Description: Do ldexp operation on SRC0 and SRC1 (multiply SRC0 by 2**(SRC1)).
SRC1 is signed integer, SRC0 is floating point value.  
Operation:  
```
VDST = ASFLOAT(SRC0) * POW(2.0, (INT32)SRC1)
```

#### V_LDEXP_F64

Opcode: 360 (0x168) for GCN 1.0/1.1; 644 (0x284) for GCN 1.2/1.4  
Syntax: V_LDEXP_F64 VDST(2), SRC0(2), SRC1  
Description: Do ldexp operation on SRC0 and SRC1 (multiply SRC0 by 2**(SRC1)).
SRC1 is signed integer, SRC0 is double floating point value.  
Operation:  
```
VDST = ASDOUBLE(SRC0) * POW(2.0, (INT32)SRC1)
```

#### V_LERP_U8

Opcode: 333 (0x14d) for GCN 1.0/1.1; 461 (0x1cd) for GCN 1.2/1.4  
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

#### V_LSHL_ADD_U32

Opcode: 509 (0x1fd) for GCN 1.4  
Syntax: V_LSHL_ADD_U32 VDST, SRC0, SRC1, SRC2  
Description: Shift left SRC0 by (SRC1&31) bits and add to SRC2 and store result to VDST.  
Operation:  
```
VDST = (SRC0 << (SRC1&31)) + SRC2
```

#### V_LSHL_B64

Opcode: 353 (0x161) for GCN 1.0/1.1  
Syntax: V_LSHL_B64 VDST(2), SRC0(2), SRC1  
Description: Shift left SRC0 by (SRC1&63) bits and store result into VDST.  
Operation:  
```
VDST = SRC0 << (SRC1&63)
```

#### V_LSHL_OR_B32

Opcode: 512 (0x200) for GCN 1.4  
Syntax: V_LSHL_OR_B32 VDST, SRC0, SRC1, SRC2  
Description: Shift left SRC0 by (SRC1&31) bits and make bitwise OR with SRC2
and store result to VDST.  
Operation:  
```
VDST = (SRC0 << (SRC1&31)) | SRC2
```

#### V_LSHLREV_B64

Opcode: 655 (0x28f) for GCN 1.2/1.4  
Syntax: V_LSHLREV_B64 VDST(2), SRC0, SRC1(2)  
Description: Shift left SRC1 by (SRC0&63) bits and store result into VDST.  
Operation:  
```
VDST = SRC1 << (SRC0&63)
```

#### V_LSHR_B64

Opcode: 354 (0x162) for GCN 1.0/1.1  
Syntax: V_LSHR_B64 VDST(2), SRC0(2), SRC1  
Description: Shift right SRC0 by (SRC1&63) bits and store result into VDST.  
Operation:  
```
VDST = SRC0 >> (SRC1&63)
```

#### V_LSHRREV_B64

Opcode: 656 (0x290) for GCN 1.2/1.4  
Syntax: V_LSHRREV_B64 VDST(2), SRC0, SRC1(2)  
Description: Shift right SRC1 by (SRC0&63) bits and store result into VDST.  
Operation:  
```
VDST = SRC1 >> (SRC0&63)
```

#### V_MAC_LEGACY_F32

Opcode: 654 (0x28e) for GCN 1.2/1.4  
Syntax: V_MAC_LEGACY_F32 VDST, SRC0, SRC1  
Description: Multiply FP value from SRC0 by FP value from SRC1 and add result to VDST.
If one of value is 0.0 then always do not change VDST (do not apply IEEE rules for 0.0*x).  
Operation:  
```
if (ASFLOAT(SRC0)!=0.0 && ASFLOAT(SRC1)!=0.0)
    VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1) + ASFLOAT(VDST)
```

#### V_MAD_F16

Opcode: 490 (0x1ea) for GCN 1.2; 515 (0x203) for GCN 1.4  
Syntax: V_MAD_F16 VDST, SRC0, SRC1, SRC2  
Description: Multiply half FP value from SRC0 by half FP value from
SRC1 and add SRC2, and store result to VDST.
It applies OMOD modifier to result and it flush denormals.  
Operation:  
```
VDST = ASHALF(SRC0) * ASHALF(SRC1) + ASHALF(SRC2)
```

#### V_MAD_F32

Opcode: 321 (0x141) for GCN 1.0/1.1; 449 (0x1c1) for GCN 1.2/1.4  
Syntax: V_MAD_F32 VDST, SRC0, SRC1, SRC2  
Description: Multiply FP value from SRC0 by FP value from SRC1 and add SRC2, and store
result to VDST. It applies OMOD modifier to result and it flush denormals.  
Operation:  
```
VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1) + ASFLOAT(SRC2)
```

#### V_MAD_I16

Opcode: 492 (0x1ec) for GCN 1.2; 517 (0x205) for GCN 1.4  
Syntax: V_MAD_I16 VDST, SRC0, SRC1, SRC2  
Description: Multiply 16-bit signed value from SRC0 by 16-bit signed value from
SRC1 and add 16-bit signed value from SRC2, and store 16-bit signed result to VDST.
If CLAMP modifier supplied, then result is saturated to 16-bit signed value.  
Operation:  
```
UINT32 temp = (SEXT32((INT16)SRC0)*(INT16)SRC1 + (INT16)SRC2)
VDST = CLAMP ? MIN(MAX(temp), -32768), 32767) : temp&0xffff
```

#### V_MAD_I32_I16

Opcode: 498 (0x1f2) for GCN 1.4  
Syntax: V_MAD_I32_I16 VDST, SRC0, SRC1, SRC2  
Description: Multiply 16-bit signed value from SRC0 by 16-bit signed value from
SRC1 and add 32-bit value from SRC2, and store 32-bit result to VDST.  
Operation:  
```
VDST = (UINT32)(SEXT32((INT16)SRC0)*(INT16)SRC1) + SRC2
```

#### V_MAD_I32_I24

Opcode: 322 (0x142) for GCN 1.0/1.1; 450 (0x1c2) for GCN 1.2/1.4  
Syntax: V_MAD_I32_I24 VDST, SRC0, SRC1, SRC2  
Description: Multiply 24-bit signed integer value from SRC0 by 24-bit signed value from
SRC1, add SRC2 to this product, and and store result to VDST.  
Operation:  
```
INT32 V0 = (INT32)((SRC0&0x7fffff) | (SSRC0&0x800000 ? 0xff800000 : 0))
INT32 V1 = (INT32)((SRC1&0x7fffff) | (SSRC1&0x800000 ? 0xff800000 : 0))
VDST = V0 * V1 + SRC2
```

#### V_MAD_I64_I32

Opcode (VOP3B): 375 (0x177) for GCN 1.1; 489 (0x1e9) for GCN 1.2/1.4  
Syntax: V_MAD_I64_I32 VDST(2), SDST(2), SRC0, SRC1, SRC2(2)  
Description: Multiply 32-bit signed integer value from SRC0 by 32-bit signed value
from SRC1 and add 64-bit unsigned value to this result, and store final result into
VDST and store some value of bits to SDST (unknown behavior).  
Operation:  
```
INT64 PROD = (INT64)SRC0*(INT32)SRC1
VDST = SRC2 + PROD
SDST = 0
UINT64 mask = (1ULL<<LANEID)
//SDST = (SDST&~mask) | ((?????) ? mask : 0)
```

#### V_MAD_LEGACY_F16

Opcode: 490 (0x1ea) for GCN 1.4  
Syntax: V_MAD_LEGACY_F16 VDST, SRC0, SRC1, SRC2  
Description: Multiply half FP value from SRC0 by half FP value from
SRC1 and add SRC2, and store result to VDST.
It applies OMOD modifier to result and it flush denormals.  
Operation:  
```
VDST = ASHALF(SRC0) * ASHALF(SRC1) + ASHALF(SRC2)
```

#### V_MAD_LEGACY_F32

Opcode: 320 (0x140) for GCN 1.0/1.1; 448 (0x1c0) for GCN 1.2/1.4  
Syntax: V_MAD_LEGACY_F32 VDST, SRC0, SRC1, SRC2  
Description: Multiply FP value from SRC0 by FP value from SRC1 and add result to SRC2, and
store result to VDST. If one of value is 0.0 then always store SRC2 to VDST
(do not apply IEEE rules for 0.0*x). It applies OMOD modifier to result and it flush
denormals.  
Operation:  
```
if (ASFLOAT(SRC0)!=0.0 && ASFLOAT(SRC1)!=0.0)
    VDST = ASFLOAT(SRC0) * ASFLOAT(SRC1) + ASFLOAT(SRC2)
```

#### V_MAD_LEGACY_I16

Opcode: 492 (0x1ec) for GCN 1.4  
Syntax: V_MAD_LEGACY_I16 VDST, SRC0, SRC1, SRC2  
Description: Multiply 16-bit signed value from SRC0 by 16-bit signed value from
SRC1 and add 16-bit signed value from SRC2, and store 16-bit signed result to VDST.
If CLAMP modifier supplied, then result is saturated to 16-bit signed value.  
Operation:  
```
UINT32 temp = (SEXT32((INT16)SRC0)*(INT16)SRC1 + (INT16)SRC2)
VDST = CLAMP ? MIN(MAX(temp), -32768), 32767) : temp&0xffff
```

#### V_MAD_LEGACY_U16

Opcode: 491 (0x1eb) for GCN 1.4  
Syntax: V_MAD_LEGACY_U16 VDST, SRC0, SRC1, SRC2  
Description: Multiply 16-bit unsigned value from SRC0 by 16-bit unsigned value from
SRC1 and add 16-bit unsigned value from SRC2, and store 16-bit unsigned result to VDST.
If CLAMP modifier supplied, then result is saturated to 16-bit unsigned value.  
Operation:  
```
UINT32 temp = ((UINT16)SRC0*(UINT16)SRC1 + (UINT16)SRC2) & 0xffff
VDST = CLAMP ? MIN(temp, 0xffff) : (temp&0xffff)
```

#### V_MAD_U16

Opcode: 491 (0x1eb) for GCN 1.2; 516 (0x204) for GCN 1.4  
Syntax: V_MAD_U16 VDST, SRC0, SRC1, SRC2  
Description: Multiply 16-bit unsigned value from SRC0 by 16-bit unsigned value from
SRC1 and add 16-bit unsigned value from SRC2, and store 16-bit unsigned result to VDST.
If CLAMP modifier supplied, then result is saturated to 16-bit unsigned value.  
Operation:  
```
UINT32 temp = ((UINT16)SRC0*(UINT16)SRC1 + (UINT16)SRC2) & 0xffff
VDST = CLAMP ? MIN(temp, 0xffff) : (temp&0xffff)
```

#### V_MAD_U32_U16

Opcode: 497 (0x1f1) for GCN 1.4  
Syntax: V_MAD_U32_U16 VDST, SRC0, SRC1, SRC2  
Description: Multiply 16-bit unsigned value from SRC0 by 16-bit unsigned value from
SRC1 and add 32-bit unsigned value from SRC2, and store 32-bit result to VDST.  
Operation:  
```
VDST = (UINT32)((SRC0&0xffff)*(SRC1&0xffff)) + SRC2
```

#### V_MAD_U32_U24

Opcode: 323 (0x143) for GCN 1.0/1.1; 451 (0x1c3) for GCN 1.2/1.4  
Syntax: V_MAD_U32_U24 VDST, SRC0, SRC1, SRC2  
Description: Multiply 24-bit unsigned integer value from SRC0 by 24-bit unsigned value
from SRC1, add SRC2 to this product and store result to VDST.  
Operation:  
```
VDST = (UINT32)(SRC0&0xffffff) * (UINT32)(SRC1&0xffffff) + SRC2
```

#### V_MAD_U64_U32

Opcode (VOP3B): 374 (0x176) for GCN 1.1; 488 (0x1e8) for GCN 1.2/1.4  
Syntax: V_MAD_U64_U32 VDST(2), SDST(2), SRC0, SRC1, SRC2(2)  
Description: Multiply 32-bit unsigned integer value from SRC0 by 32-bit unsigned value
from SRC1 and add 64-bit unsigned value to this result, and store final result into
VDST and store carry bits to SDST.  
Operation:  
```
UINT64 PROD = (UINT64)SRC0*SRC1
VDST = SRC2 + PROD
SDST = 0
UINT64 mask = (1ULL<<LANEID)
SDST = (SDST&~mask) | ((VDST < PROD) ? mask : 0)
```

#### V_MAX_F64

Opcode: 359 (0x167) for GCN 1.0/1.1; 643 (0x283) for GCN 1.2/1.4  
Syntax: V_MAX_F64 VDST(2), SRC0(2), SRC1(2)  
Description: Choose largest double FP value from SRC0 and SRC1, and store result to VDST.  
Operation:  
```
VDST = MAX((ASDOUBLE(SRC0), ASDOUBLE(SRC1))
```

#### V_MAX3_F16

Opcode: 503 (0x1f7) for GCN 1.4  
Syntax: V_MAX3_F16 VDST, SRC0, SRC1, SRC2  
Description: Choose largest value from half FP values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
HALF SF0 = ASHALF(SRC0)
HALF SF1 = ASHALF(SRC1)
HALF SF2 = ASHALF(SRC2)
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

#### V_MAX3_F32

Opcode: 340 (0x154) for GCN 1.0/1.1; 467 (0x1d3) for GCN 1.2/1.4  
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

#### V_MAX3_I16

Opcode: 504 (0x1f8) for GCN 1.4  
Syntax: V_MAX3_I16 VDST, SRC0, SRC1, SRC2  
Description: Choose largest value from signed 16-bit integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if ((INT16)SRC2 > (INT16)SRC0 && (INT16)SRC2 > (INT16)SRC1)
    VDST = (UINT16)SRC2
else
    VDST = (UINT16)MAX((INT16)SRC1, (INT16)SRC0)
```

#### V_MAX3_I32

Opcode: 341 (0x155) for GCN 1.0/1.1; 468 (0x1d4) for GCN 1.2/1.4  
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

#### V_MAX3_U16

Opcode: 505 (0x1f9) for GCN 1.4  
Syntax: V_MAX3_U16 VDST, SRC0, SRC1, SRC2  
Description: Choose largest value from unsigned 16-bit integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if ((UINT16)SRC2 > (UINT16)SRC0 && (UINT16)SRC2 > (UINT16)SRC1)
    VDST = (UINT16)SRC2
else
    VDST = MAX((UINT16)SRC1, (UINT16)SRC0)
```

#### V_MAX3_U32

Opcode: 342 (0x156) for GCN 1.0/1.1; 469 (0x1d5) for GCN 1.2/1.4  
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

#### V_MBCNT_HI_U32_B32

Opcode: 653 (0x28d) for GCN 1.2/1.4  
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

Opcode: 652 (0x28c) for GCN 1.2/1.4  
Syntax: V_MBCNT_LO_U32_B32 VDST, SRC0, SRC1  
Description: Make mask for all lanes ending at current lane,
get from that mask lower 32-bits, use it to mask SSRC0,
count bits in that value, and store result to VDST.  
Operation:  
```
UINT32 MASK = ((1ULL << LANEID) - 1ULL) & SRC0
VDST = SRC1 + BITCOUNT(MASK)
```

#### V_MED3_F16

Opcode: 506 (0x1fa) for GCN 1.4  
Syntax: V_MED3_F16 VDST, SRC0, SRC1, SRC2  
Description: Choose medium value from half FP values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
HALF SF0 = ASHALF(SRC0)
HALF SF1 = ASHALF(SRC1)
HALF SF2 = ASHALF(SRC2)
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

#### V_MED3_F32

Opcode: 343 (0x157) for GCN 1.0/1.1; 470 (0x1d6) for GCN 1.2/1.4  
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

#### V_MED3_I16

Opcode: 507 (0x1fb) for GCN 1.4  
Syntax: V_MED3_I16 VDST, SRC0, SRC1, SRC2  
Description: Choose medium value from signed 16-bit integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
INT16 S0 = (INT16)SRC0
INT16 S1 = (INT32)SRC1
INT16 S2 = (INT32)SRC2
if ((S2 > S1 && S2 < S0) || (S2 < S1 && S2 > S0))
    VDST = (UINT16)S2
else if ((S1 > S2 && S1 < S0) || (S1 < S2 && S1 > S0))
    VDST = (UINT16)S1
else
    VDST = (UINT16)S0
```

#### V_MED3_I32

Opcode: 344 (0x158) for GCN 1.0/1.1; 471 (0x1d7) for GCN 1.2/1.4  
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

#### V_MED3_U16

Opcode: 508 (0x1fc) for GCN 1.4  
Syntax: V_MED3_U16 VDST, SRC0, SRC1, SRC2  
Description: Choose medium value from unsigned 16-bit integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
UINT16 S0 = (UINT16)SRC0
UINT16 S1 = (UINT16)SRC1
UINT16 S2 = (UINT16)SRC2
if ((S2 > S1 && S2 < S0) || (S2 < S1 && S2 > S0))
    VDST = S2
else if ((S1 > S2 && S1 < S0) || (S1 < S2 && S1 > S0))
    VDST = S1
else
    VDST = S0
```

#### V_MED3_U32

Opcode: 345 (0x159) for GCN 1.0/1.1; 472 (0x1d8) for GCN 1.2/1.4  
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

Opcode: 358 (0x166) for GCN 1.0/1.1; 642 (0x282) for GCN 1.2/1.4  
Syntax: V_MIN_F64 VDST(2), SRC0(2), SRC1(2)  
Description: Choose smallest double FP value from SRC0 and SRC1, and store result to VDST.  
Operation:  
```
VDST = MIN((ASDOUBLE(SRC0), ASDOUBLE(SRC1))
```

#### V_MIN3_F16

Opcode: 500 (0x1f4) for GCN 1.4  
Syntax: V_MIN3_F16 VDST, SRC0, SRC1, SRC2  
Description: Choose smallest value from half FP values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
HALF SF0 = ASHALF(SRC0)
HALF SF1 = ASHALF(SRC1)
HALF SF2 = ASHALF(SRC2)
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

#### V_MIN3_F32

Opcode: 337 (0x151) for GCN 1.0/1.1; 464 (0x1d0) for GCN 1.2/1.4  
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

#### V_MIN3_I16

Opcode: 501 (0x1f5) for GCN 1.4  
Syntax: V_MIN3_I16 VDST, SRC0, SRC1, SRC2  
Description: Choose smallest value from signed 16-bit integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if ((INT16)SRC2 < (INT16)SRC0 && (INT16)SRC2 < (INT16)SRC1)
    VDST = (UINT16)SRC2
else
    VDST = (UINT16)MIN((INT16)SRC1, (INT16)SRC0)
```

#### V_MIN3_I32

Opcode: 338 (0x152) for GCN 1.0/1.1; 465 (0x1d1) for GCN 1.2/1.4  
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

#### V_MIN3_U16

Opcode: 502 (0x1f6) for GCN 1.4  
Syntax: V_MIN3_U16 VDST, SRC0, SRC1, SRC2  
Description: Choose smallest value from unsigned 16-bit integer values SRC0, SRC1, SRC2,
and store it to VDST.  
Operation:  
```
if ((UINT16)SRC2 < (UINT16)SRC0 && (UINT16)SRC2 < (UINT16)SRC1)
    VDST = (UINT16)SRC2
else
    VDST = MIN(S(UINT16)RC1, (UINT16)SRC0)
```

#### V_MIN3_U32

Opcode: 339 (0x153) for GCN 1.0/1.1; 466 (0x1d2) for GCN 1.2/1.4  
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

#### V_MQSAD_U32_U8

Opcode: 373 (0x175) for GCN 1.1; 487 (0x1e7) for GCN 1.2/1.4  
Syntax: V_MQSAD_U32_U8 VDST(4), SRC0(2), SRC1, SRC2(4)  
Description: Compute four masked sum of absolute differences with accumulation.
Any that operation get first argument from four bytes begins from N and ends to N+3
(where N is number of operation), second argument is SRC1, and third argument is
N'th 32-bit dword from SRC2.  
Operation:  
```
void MSADU8(UINT32 S0, UINT32 S1, UINT32 S2)
{
    UINT64 OUT = S2;
    for (UINT8 i = 0; i < 4; i++)
        if ((S1 >> (i*8)) & 0xff) != 0)
            OUT += ABS(((S0 >> (i*8)) & 0xff) - ((S1 >> (i*8)) & 0xff))
    return (UINT32)MIN(OUT,0xffffffff);
}
VDST = (MSADU8((UINT32)SRC0, SRC1, SRC2)
VDST |= (MSADU8((UINT32)(SRC0>>8), SRC1, SRC2>>32)<<32
VDST |= (MSADU8((UINT32)(SRC0>>16), SRC1, SRC2>>64)<<64
VDST |= (MSADU8((UINT32)(SRC0>>24), SRC1, SRC2>>96)<<96
```

#### V_MQSAD_U8, V_MQSAD_PK_U16_U8

Opcode: 371 (0x173) for GCN 1.0/1.1; 486 (0x1e6) for GCN 1.2/1.4  
Syntax (GCN 1.0): V_MQSAD_U8 VDST(2), SRC0(2), SRC1, SRC2(2)  
Syntax (GCN 1.1/1.2): V_MQSAD_PK_U16_U8 VDST(2), SRC0(2), SRC1, SRC2(2)  
Description: Compute four masked sum of absolute differences with accumulation.
Any that operation get first argument from four bytes begins from N and ends to N+3
(where N is number of operation), second argument is SRC1, and third argument is
N'th 16-bit dword from SRC2.  
Operation:  
```
void MSADU8(UINT32 S0, UINT32 S1, UINT32 S2)
{
    UINT32 OUT = S2;
    for (UINT8 i = 0; i < 4; i++)
        if ((S1 >> (i*8)) & 0xff) != 0)
            OUT += ABS(((S0 >> (i*8)) & 0xff) - ((S1 >> (i*8)) & 0xff))
    return OUT;
}
VDST = (MSADU8((UINT32)SRC0, SRC1, SRC2 & 0xffff)
VDST |= (MSADU8((UINT32)(SRC0>>8), SRC1, (SRC2>>16) & 0xffff)<<16
VDST |= (MSADU8((UINT32)(SRC0>>16), SRC1, (SRC2>>32) & 0xffff)<<32
VDST |= (MSADU8((UINT32)(SRC0>>24), SRC1, (SRC2>>48) & 0xffff)<<48
```

#### V_MSAD_U8

Opcode: 369 (0x171) for GCN 1.0/1.1; 484 (0x1e4) for GCN 1.2/1.4  
Syntax: V_MSAD_U8 VDST, SRC0, SRC1, SRC2  
Description: Calculate sum of absolute differences in SRC0 and SRC1 for bytes that have
non-zero value in SRC1; add SRC2 to result, and store result to VDST.  
Operation:  
```
VDST = SRC2
for (UINT8 i = 0; i < 4; i++)
    if ((SRC1 >> (i*8)) & 0xff) != 0)
        VDST += ABS(((SRC0 >> (i*8)) & 0xff) - ((SRC1 >> (i*8)) & 0xff))
```

#### V_MUL_F64

Opcode: 357 (0x165) for GCN 1.0/1.1; 641 (0x281) for GCN 1.2/1.4  
Syntax: V_MUL_F64 VDST(2), SRC0(2), SRC1(2)  
Description: Multiply two double FP values from SRC0 and SRC1 and store result to VDST.  
Operation:  
```
VDST = ASDOUBLE(SRC0) * ASDOUBLE(SRC1)
```

#### V_MUL_HI_I32

Opcode: 364 (0x16c) for GCN 1.0/1.1; 647 (0x287) for GCN 1.2/1.4  
Syntax: V_MUL_HI_I32 VDST, SRC0, SRC1  
Description: Multiply 32-bit signed value SRC0 and SRC1, and store higher part of
the result to VDST.  
Operation:  
```
VDST = ((INT64)SRC0 * (INT32)SRC1) >> 32
```

#### V_MUL_HI_U32

Opcode: 362 (0x16a) for GCN 1.0/1.1; 646 (0x286) for GCN 1.2/1.4  
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

Opcode: 361 (0x169) for GCN 1.0/1.1; 645 (0x285) for GCN 1.2/1.4  
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

#### V_OR3_B32

Opcode: 514 (0x202) for GCN 1.4  
Syntax: V_OR3_B32 VDST, SRC0, SRC1, SRC2  
Description: Make bitwise OR with SRC0, SRC1 and SRC2 and store result to VDST.  
Operation:  
```
VDST = SRC0 | SRC1 | SRC2
```

#### V_PACK_B32_F16

Opcode: 672 (0x2a0) for GCN 1.4  
Syntax: V_PACK_B32_F16 VDST, SRC0, SRC1  
Description: Get lower 16-bits from SRC0 and put to lower 16-bits in VDST, 
get lower 16-bits from SRC1 and put to higher 16-bits in VDST.  
Operation:  
```
VDST = (SRC0&0xffff) | (SRC1<<16)
```

#### V_PERM_B32

Opcode: 493 (0x1ed) for GCN 1.2/1.4  
Syntax: V_PERM_B32 VDST, SRC0, SRC1, SRC2  
Description: Permute bytes. Choose for every byte in dword, specified value. Bytes in
SRC2 dword selects value for result dword. Value 0-7 choose byte of this index of quadword
(64-bit value) built from SRC0 (higher bits) and SRC1 (lower bits). Value from 8-11
choose 0xff*BIT, where BIT is last bit from 2*N+1 from 64-bit value (SRC0,SRC1).
Value 12 choose zero. Value equal or greater than 13 choose 0xff.  
Operation:  
```
VDST = 0
UINT64 qword = (((UINT64)SRC0)<<32) | SRC1 
for (int i = 0; i < 4; i++)
{
    BYTE choice = (SRC2 >> (8*i)) & 0xff
    BYTE result
    if (choice >= 13)
        result = 0xff
    else if (choice == 12)
        result = 0
    else if (choice >= 8)
        result = 0xff * qword>>((choice-8)*16 + 15)
    else
        result = (qword >> (choice*8)) & 0xff
    VDST |= (result << (i*8))
}
```

#### V_QSAD_U8, V_QSAD_PK_U16_U8

Opcode: 370 (0x172) for GCN 1.0/1.1; 485 (0x1e5) for GCN 1.2/1.4  
Syntax (GCN 1.0): V_QSAD_U8 VDST(2), SRC0(2), SRC1, SRC2(2)  
Syntax (GCN 1.1/1.2): V_QSAD_PK_U16_U8 VDST(2), SRC0(2), SRC1, SRC2(2)  
Description: Compute four sum of absolute differences with accumulation. Any that operation
get first argument from four bytes begins from N and ends to N+3 (where N is number of
operation), second argument is SRC1, and third argument is N'th 16-bit dword from SRC2.  
Operation:  
```
void SADU8(UINT32 S0, UINT32 S1, UINT32 S2)
{
    UINT32 OUT = S2;
    for (UINT8 i = 0; i < 4; i++)
        OUT += ABS(((S0 >> (i*8)) & 0xff) - ((S1 >> (i*8)) & 0xff))
    return OUT;
}
VDST = (SADU8((UINT32)SRC0, SRC1, SRC2 & 0xffff)
VDST |= (SADU8((UINT32)(SRC0>>8), SRC1, (SRC2>>16) & 0xffff)<<16
VDST |= (SADU8((UINT32)(SRC0>>16), SRC1, (SRC2>>32) & 0xffff)<<32
VDST |= (SADU8((UINT32)(SRC0>>24), SRC1, (SRC2>>48) & 0xffff)<<48
```

#### V_READLANE_B32

Opcode: 649 (0x289) for GCN 1.2/1.4  
Syntax: V_READLANE_B32 SDST, VSRC0, SSRC1  
Description: Copy one VSRC0 lane value to one SDST. Lane (thread id) choosen from SSRC1&63.
SSRC1 can be SGPR or M0. Ignores EXEC mask.  
Operation:  
```
SDST = VSRC0[SSRC1 & 63]
```

#### V_SAD_HI_U8

Opcode: 347 (0x15b) for GCN 1.0/1.1; 474 (0x1da) for GCN 1.2/1.4  
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

Opcode: 348 (0x15c) for GCN 1.0/1.1; 475 (0x1db) for GCN 1.2/1.4  
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

Opcode: 349 (0x15d) for GCN 1.0/1.1; 476 (0x1dc) for GCN 1.2/1.4  
Syntax: V_SAD_U32 VDST, SRC0, SRC1, SRC2  
Description: Calculate sum of absolute difference for SRC0 and SRC1, add
SRC2 to result, and store result to VDST.  
Operation:  
```
VDST = SRC2 + ABS(SRC0 - SRC1)
```

#### V_SAD_U8

Opcode: 346 (0x15a) for GCN 1.0/1.1; 473 (0x1d9) for GCN 1.2/1.4  
Syntax: V_SAD_U8 VDST, SRC0, SRC1, SRC2  
Description: Calculate sum of absolute differences for all four bytes in SRC0 and SRC1, add
SRC2 to result, and store result to VDST.  
Operation:  
```
VDST = SRC2
for (UINT8 i = 0; i < 4; i++)
    VDST += ABS(((SRC0 >> (i*8)) & 0xff) - ((SRC1 >> (i*8)) & 0xff))
```

#### V_SUB_I16

Opcode: 671 (0x29f) for GCN 1.4  
Syntax: V_SUB_I16 VDST, SRC0, SRC1  
Description: Subtract 16-bit signed value from SRC1 from 16-bit signed value from SRC0 and
store result to VDST. If CLAMP modifier supplied, then result is saturated to
16-bit signed value.  
Operation:  
```
UINT16 result = (SRC0&0xffff) - (SRC1&0xffff)
if (CLAMP)
{
    INT32 temp = SEXT32((INT16)SRC0&0xffff) - SEXT32((INT16)SRC1&0xffff)
    if (temp > ((1<<16)-1))
        result = 0x7fff
    if temp < (-1<<16)
        result = 0x8000
}
VDST = (VDST & 0xffff0000) | result
```

#### V_SUB_I32

Opcode: 669 (0x29d) for GCN 1.4  
Syntax: V_SUB_I32 VDST, SRC0, SRC1  
Description: Subtract signed value from SRC1 from signed value from SRC0 and
store result to VDST. If CLAMP modifier supplied, then result is saturated to
32-bit signed value.  
Operation:  
```
VDST = SRC0 - SRC1
if (CLAMP)
{
    INT64 temp = SEXT64(SRC0) - SEXT64(SRC1)
    if (temp > ((1LL<<31)-1))
        VDST = 0x7fffffff
    if temp < (-1LL<<31)
        VDST = 0x80000000
}
```

#### V_TRIG_PREOP_F64

Opcode: 372 (0x174) for GCN 1.0/1.1; 658 (0x292) for GCN 1.2/1.4  
Syntax: V_TRIG_PREOP_F64 VDST(2), SRC0(2), SRC1  
Description:  D.d = Look Up 2/PI (S0.d) with segment select S1.u[4:0].
Save choosen 53 bits of 2/PI in double floating point value in VDST. Second argument
is initial segment. First argument is shift of the value (in power form).
Bit are numbered from MSB to LSB, begins from value 1.0. Choosen bits begins from:
53\*SEGMENT + (FREXP_EXP(SRC0)-1)-(53\*SEGMENT if SRC0>=POW(2.0, 53\*SEGMENT),
otherwise 53\*SEGMENT.  
Operation:  
```
ASDOUBLE SD0 = ASDOUBLE(SRC0)
BIT = (SRC1&31) * 53
if (SD0 >= POW(2.0, 53)
{
    if (ABS(SD0) != INF) && !ISNAN(SD0))
        BIT += (FREXP_EXP(SD0)-1) - BIT
    else
        BIT += 1024 - BIT
}
VDST = (DOUBLE)(TWOPERPI[BIT:BIT+52]) * POW(2.0, -BIT-53)
```

#### V_WRITELANE_B32

Opcode: 650 (0x28a) for GCN 1.2/1.4  
Syntax: V_WRITELANE_B32 VDST, VSRC0, SSRC1  
Description: Copy SGPR to one lane of VDST. Lane choosen (thread id) from SSRC1&63.
SSRC1 can be SGPR or M0. Ignores EXEC mask.  
Operation:  
```
VDST[SSRC1 & 63] = SSRC0
```

#### V_XAD_U32

Opcode: 499 (0x1f3) for GCN 1.4  
Syntax: V_XAD_U32 VDST, SRC0, SRC1, SRC2  
Description: Make XOR bitwise operation on SRC0 and SRC1, add SRC2 and store result to VDST.
Instruction added to speed up SHA256 sum.  
Operation:  
```
VDST = (SRC0 ^ SRC1) + SRC2
```
