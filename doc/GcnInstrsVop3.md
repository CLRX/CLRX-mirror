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

#### V_MAD_LEGACY_F32

Opcode: 320 (0x140) for GCN 1.0/1.1; 448 (0x1c0) for GCN 1.2  
Syntax: V_MAD_LEGACY_F32 VDST, SRC0, SRC1, SRC2  
Description: