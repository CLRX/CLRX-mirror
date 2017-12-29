## GCN ISA SOP2 instructions

### Encoding

The basic encoding of the SOP2 instructions needs 4 bytes (dword). List of fields:

Bits  | Name     | Description
------|----------|------------------------------
0-7   | SSRC0    | First scalar source operand. Refer to operand encoding
8-15  | SSRC1    | Second scalar source operand. Refer to operand encoding
16-22 | SDST     | Destination scalar operand. Refer to operand encoding
23-29 | OPCODE   | Operation code
30-31 | ENCODING | Encoding type. Must be 0b10

Syntax for almost instructions: INSTRUCTION SDST, SSRC0, SSRC1

Example: s_and_b32 s0, s1, s2

List of the instructions by opcode:

 Opcode     | Mnemonic (GCN1.0/1.1) | Mnemonic (GCN 1.2)  | Mnemonic (GCN 1.4)
------------|----------------------|----------------------|---------------------
 0 (0x0)    | S_ADD_U32            | S_ADD_U32            | S_ADD_U32
 1 (0x1)    | S_SUB_U32            | S_SUB_U32            | S_SUB_U32
 2 (0x2)    | S_ADD_I32            | S_ADD_I32            | S_ADD_I32
 3 (0x3)    | S_SUB_I32            | S_SUB_I32            | S_SUB_I32
 4 (0x4)    | S_ADDC_U32           | S_ADDC_U32           | S_ADDC_U32
 5 (0x5)    | S_SUBB_U32           | S_SUBB_U32           | S_SUBB_U32
 6 (0x6)    | S_MIN_I32            | S_MIN_I32            | S_MIN_I32
 7 (0x7)    | S_MIN_U32            | S_MIN_U32            | S_MIN_U32
 8 (0x8)    | S_MAX_I32            | S_MAX_I32            | S_MAX_I32
 9 (0x9)    | S_MAX_U32            | S_MAX_U32            | S_MAX_U32
 10 (0xa)   | S_CSELECT_B32        | S_CSELECT_B32        | S_CSELECT_B32
 11 (0xb)   | S_CSELECT_B64        | S_CSELECT_B64        | S_CSELECT_B64
 12 (0xc)   | --                   | S_AND_B32            | S_AND_B32
 13 (0xd)   | --                   | S_AND_B64            | S_AND_B64
 14 (0xe)   | S_AND_B32            | S_OR_B32             | S_OR_B32
 15 (0xf)   | S_AND_B64            | S_OR_B64             | S_OR_B64
 16 (0x10)  | S_OR_B32             | S_XOR_B32            | S_XOR_B32
 17 (0x11)  | S_OR_B64             | S_XOR_B64            | S_XOR_B64
 18 (0x12)  | S_XOR_B32            | S_ANDN2_B32          | S_ANDN2_B32
 19 (0x13)  | S_XOR_B64            | S_ANDN2_B64          | S_ANDN2_B64
 20 (0x14)  | S_ANDN2_B32          | S_ORN2_B32           | S_ORN2_B32
 21 (0x15)  | S_ANDN2_B64          | S_ORN2_B64           | S_ORN2_B64
 22 (0x16)  | S_ORN2_B32           | S_NAND_B32           | S_NAND_B32
 23 (0x17)  | S_ORN2_B64           | S_NAND_B64           | S_NAND_B64
 24 (0x18)  | S_NAND_B32           | S_NOR_B32            | S_NOR_B32
 25 (0x19)  | S_NAND_B64           | S_NOR_B64            | S_NOR_B64
 26 (0x1a)  | S_NOR_B32            | S_XNOR_B32           | S_XNOR_B32
 27 (0x1b)  | S_NOR_B64            | S_XNOR_B64           | S_XNOR_B64
 28 (0x1c)  | S_XNOR_B32           | S_LSHL_B32           | S_LSHL_B32
 29 (0x1d)  | S_XNOR_B64           | S_LSHL_B64           | S_LSHL_B64
 30 (0x1e)  | S_LSHL_B32           | S_LSHR_B32           | S_LSHR_B32
 31 (0x1f)  | S_LSHL_B64           | S_LSHR_B64           | S_LSHR_B64
 32 (0x20)  | S_LSHR_B32           | S_ASHR_I32           | S_ASHR_I32
 33 (0x21)  | S_LSHR_B64           | S_ASHR_I64           | S_ASHR_I64
 34 (0x22)  | S_ASHR_I32           | S_BFM_B32            | S_BFM_B32
 35 (0x23)  | S_ASHR_I64           | S_BFM_B64            | S_BFM_B64
 36 (0x24)  | S_BFM_B32            | S_MUL_I32            | S_MUL_I32
 37 (0x25)  | S_BFM_B64            | S_BFE_U32            | S_BFE_U32
 38 (0x26)  | S_MUL_I32            | S_BFE_I32            | S_BFE_I32
 39 (0x27)  | S_BFE_U32            | S_BFE_U64            | S_BFE_U64
 40 (0x28)  | S_BFE_I32            | S_BFE_I64            | S_BFE_I64
 41 (0x29)  | S_BFE_U64            | S_CBRANCH_G_FORK     | S_CBRANCH_G_FORK
 42 (0x2a)  | S_BFE_I64            | S_ABSDIFF_I32        | S_ABSDIFF_I32
 43 (0x2b)  | S_CBRANCH_G_FORK     | S_RFE_RESTORE_B64    | S_RFE_RESTORE_B64
 44 (0x2c)  | S_ABSDIFF_I32        | --                   | S_MUL_HI_U32
 45 (0x2d)  | --                   | --                   | S_MUL_HI_I32
 46 (0x2e)  | --                   | --                   | S_LSHL1_ADD_U32
 47 (0x2f)  | --                   | --                   | S_LSHL2_ADD_U32
 48 (0x30)  | --                   | --                   | S_LSHL3_ADD_U32
 49 (0x31)  | --                   | --                   | S_LSHL4_ADD_U32
 50 (0x32)  | --                   | --                   | S_PACK_LL_B32_B16
 51 (0x33)  | --                   | --                   | S_PACK_LH_B32_B16
 52 (0x34)  | --                   | --                   | S_PACK_HH_B32_B16

### Instruction set

Alphabetically sorted instruction list:

#### S_ABSDIFF_I32

Opcode: 44 (0x2c) for GCN 1.0/11; 42 (0x2a) for GCN 1.2   
Syntax: S_ABSDIFF_I32 SDST, SSRC0, SSRC1  
Description: Compute absolute difference from SSRC0 and SSRC1 and store result to SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = ABS(SSRC0 - SSRC1)
SCC = SDST!=0
```

#### S_ADDC_U32

Opcode: 4 (0x4)  
Syntax: S_ADDC_U32 SDST, SSRC0, SSRC1  
Descrition: Add SSRC0 to SSRC1 with SCC value and store result into SDST and store
carry-out flag into SCC.  
Operation:  
```
UINT64 temp = (UINT64)SSRC0 + (UINT64)SSRC1 + SCC
SDST = temp
SCC = temp >> 32
```

#### S_ADD_I32

Opcode: 2 (0x2)  
Syntax: S_ADD_I32 SDST, SSRC0, SSRC1  
Description: Add SSRC0 to SSRC1 and store result into SDST and store overflow
flag into SCC.  
Operation:  
```
SDST = SSRC0 + SSRC1
INT64 temp = SEXT64(SSRC0) + SEXT64(SSRC1)
SCC = temp > ((1LL<<31)-1) || temp < (-1LL<<31)
```

#### S_ADD_U32

Opcode: 0 (0x0)  
Syntax: S_ADD_U32 SDST, SSRC0, SSRC1  
Description: Add SSRC0 to SSRC1 and store result into SDST and store carry-out into SCC.  
Operation:  
```
UINT64 temp = (UINT64)SSRC0 + (UINT64)SSRC1
SDST = temp
SCC = temp >> 32
```

#### S_AND_B32

Opcode: 14 (0xe) for GCN 1.0/1.1; 12 (0xc) for GCN 1.2  
Syntax: S_AND_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise AND operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 & SSRC1
SCC = SDST!=0
```

#### S_AND_B64

Opcode: 15 (0xf) for GCN 1.0/1.1; 13 (0xd) for GCN 1.2  
Syntax: S_AND_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise AND operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 & SSRC1
SCC = SDST!=0
```

#### S_ANDN2_B32

Opcode: 20 (0x14) for GCN 1.0/1.1; 18 (0x12) for GCN 1.2  
Syntax: S_ANDN2_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise AND operation on SSRC0 and negated SSRC1 and store it to SDST,
and store 1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 & ~SSRC1
SCC = SDST!=0
```

#### S_ANDN2_B64

Opcode: 21 (0x15) for GCN 1.0/1.1; 19 (0x13) for GCN 1.2  
Syntax: S_ANDN2_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise AND operation on SSRC0 and bitwise negated SSRC1 and store
it to SDST, and store 1 to SCC if result is not zero, otherwise store 0 to SCC.
SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 & ~SSRC1
SCC = SDST!=0
```

#### S_ASHR_I32

Opcode: 34 (0x22) for GCN 1.0/1.1; 32 (0x20) for GCN 1.2  
Syntax: S_ASHR_I32 SDST, SSRC0, SSRC1  
Description: Arithmetic shift right SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = (INT32)SSRC0 >> (SSRC1 & 31)
SCC = SDST!=0
```

#### S_ASHR_I64

Opcode: 35 (0x23) for GCN 1.0/1.1; 33 (0x21) for GCN 1.2  
Syntax: S_ASHR_I64 SDST(2), SSRC0(2), SSRC1  
Description: Arithmetic Shift right SSRC0 by (SSRC1&63) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC. SDST, SSRC0 are 64-bit,
SSRC1 is 32 bit.  
Operation:  
```
SDST = (INT64)SSRC0 >> (SSRC1 & 63)
SCC = SDST!=0
```

#### S_BFE_I32

Opcode: 40 (0x28) for GCN 1.0/1.1; 38 (0x26) for GCN 1.2  
Syntax: S_BFE_I32 SDST, SSRC0, SSRC1  
Description: Extracts bits in SSRC0 from range (SSRC1&31) with length ((SSRC1>>16)&0x7f)
and extend sign from last bit of extracted value, and store result to SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT8 shift = SSRC1 & 31
UINT8 length = (SSRC1>>16) & 0x7f
if (length==0)
    SDST = 0
if (shift+length < 32)
    SDST = (INT32)(SSRC0 << (32 - shift - length)) >> (32 - length)
else
    SDST = (INT32)SSRC0 >> shift
SCC = SDST!=0
```

#### S_BFE_I64

Opcode: 42 (0x2a) for GCN 1.0/1.1; 40 (0x28) for GCN 1.2  
Syntax: S_BFE_I64 SDST, SSRC0, SSRC1  
Description: Extracts bits in SSRC0 from range (SSRC1&63) with length ((SSRC1>>16)&0x7f)
and extend sign from last bit of extracted value, and store result to SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT8 shift = SSRC1 & 63
UINT8 length = (SSRC1>>16) & 0x7f
if (length==0)
    SDST = 0
if (shift+length < 64)
    SDST = (INT64)(SSRC0 << (64 - shift - length)) >> (64 - length)
else
    SDST = (INT64)SSRC0 >> shift
SCC = SDST!=0
```

#### S_BFE_U32

Opcode: 39 (0x27) for GCN 1.0/1.1; 37 (0x25) for GCN 1.2  
Syntax: S_BFE_U32 SDST, SSRC0, SSRC1  
Description: Extracts bits in SSRC0 from range (SSRC1&31) with length ((SSRC1>>16)&0x7f),
and store result to SDST. If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT8 shift = SSRC1 & 31
UINT8 length = (SSRC1>>16) & 0x7f
if (length==0)
    SDST = 0
if (shift+length < 32)
    SDST = SSRC0 << (32 - shift - length) >> (32 - length)
else
    SDST = SSRC0 >> shift
SCC = SDST!=0
```

#### S_BFE_U64

Opcode: 41 (0x29) for GCN 1.0/1.1; 39 (0x27) for GCN 1.2  
Syntax: S_BFE_U64 SDST(2), SSRC0(2), SSRC1  
Description: Extracts bits in SSRC0 from range (SSRC1&63) with length ((SSRC1>>16)&0x7f),
and store result to SDST. If result is non-zero store 1 to SCC, otherwise store 0 to SCC.
SDST, SSRC0 are 64-bit, SSRC1 is 32-bit.  
Operation:  
```
UINT8 shift = SSRC1 & 63
UINT8 length = (SSRC1>>16) & 0x7f
if (length==0)
    SDST = 0
if (shift+length < 64)
    SDST = SSRC0 << (64 - shift - length) >> (64 - length)
else
    SDST = SSRC0 >> shift
SCC = SDST!=0
```

#### S_BFM_B32

Opcode: 36 (0x24) for GCN 1.0/1.1; 34 (0x22) for GCN 1.2  
Syntax: S_BFM_B32 SDST, SSRC0, SSRC1  
Description: Make 32-bit bitmask from (SSRC1 & 31) bit that have length (SSRC0 & 31) and
store it to SDST. SCC not touched.  
Operation:  
```
SDST = ((1U << (SSRC0&31))-1) << (SSRC1&31)
```

#### S_BFM_B64

Opcode: 37 (0x25) for GCN 1.0/1.1; 35 (0x23) for GCN 1.2  
Syntax: S_BFM_B64 SDST(2), SSRC0, SSRC1  
Description: Make 64-bit bitmask from (SSRC1 & 63) bit that have length (SSRC0 & 63) and
store it to SDST. SCC not touched.  
Operation:  
```
SDST = ((1ULL << (SSRC0&63))-1) << (SSRC1&63)
```

#### S_CBRANCH_G_FORK

Opcode: 43 (0x2b) for GCN 1.0/1.1; 41 (0x29) for GCN 1.2  
Syntax: S_CBRANCH_G_FORK SSRC0(2), SSRC1(2)  
Description: Fork control flow to passed and failed condition, jump to address SSRC1 for
passed conditions. Make two masks: for passed conditions (EXEC & SSRC0),
for failed conditions: (EXEC & ~SSRC0).
Choose way that have smallest active threads and push data for second way to control stack 
(EXEC mask, jump address). Control stack pointer is stored in CSP
(3 last bits in MODE register). One entry of the stack have 4 dwords.
This instruction doesn't work if SSRC0 is immediate value.  
Operation:  
```
UINT64 passes = (EXEC & SSRC0)
UINT64 failures = (EXEC & ~SSRC0)
if (passes == EXEC)
    PC = SSRC1
else if (failures == EXEC)
    PC += 4
else if (BITCOUNT(failures) < BITCOUNT(passes)) {
    EXEC = failures
    SGPR[CSP*4:CSP*4+1] = passes
    SGPR[CSP*4+2:CSP*4+3] = SSRC1
    CSP++
    PC += 4 /* jump to failure */
} else {
    EXEC = passes
    SGPR[CSP*4:CSP*4+1] = failures
    SGPR[CSP*4+2:CSP*4+3] = PC+4
    CSP++
    PC = SSRC1  /* jump to passes */
}
```

#### S_CSELECT_B32

Opcode: 10 (0xa)  
Syntax: S_CSELECT_B32 SDST, SSRC0, SSRC1  
Description: If SCC is 1 then store SSRC0 into SDST, otherwise store SSRC1 into SDST.
SCC is not changed.  
Operation:  
```
SDST = SCC ? SSRC0 : SSRC1
```

#### S_CSELECT_B64

Opcode: 11 (0xb)  
Syntax: S_CSELECT_B32 SDST(2), SSRC0(2), SSRC1(2)  
Description: If SCC is 1 then store 64-bit SSRC0 into SDST, otherwise store
64-bit SSRC1 into SDST. SCC is not changed.  
Operation:  
```
SDST = SCC ? SSRC0 : SSRC1
```

#### S_LSHL_B32

Opcode: 30 (0x1e) for GCN 1.0/1.1; 28 (0x1c) for GCN 1.2/1.4  
Syntax: S_LSHL_B32 SDST, SSRC0, SSRC1  
Description: Shift left SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 << (SSRC1 & 31)
SCC = SDST!=0
```

#### S_LSHL_B64

Opcode: 31 (0x1f) for GCN 1.0/1.1; 29 (0x1d) for GCN 1.2/1.4  
Syntax: S_LSHL_B64 SDST(2), SSRC0(2), SSRC1  
Description: Shift left SSRC0 by (SSRC1&63) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC. SDST, SSRC0 are 64-bit,
SSRC1 is 32 bit.  
Operation:  
```
SDST = SSRC0 << (SSRC1 & 63)
SCC = SDST!=0
```

#### S_LSHL1_ADD_U32

Opcode: 46 (0x2e) for GCN 1.4  
Syntax: S_LSHL1_ADD_U32 SDST, SRC0, SRC1  
Description: Shift left SSRC0 by 1 bits and adds this result to SSRC1 and store final
result into SDST. If final value is greater than maximal value of 32-bit type then store
1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT64 TMP = (SSRC0<<1) + SSRC1
SDST = TMP&0xffffffff
SCC = TMP >= (1ULL<<32)
```

#### S_LSHL2_ADD_U32

Opcode: 47 (0x2f) for GCN 1.4  
Syntax: S_LSHL2_ADD_U32 SDST, SRC0, SRC1  
Description: Shift left SSRC0 by 2 bits and adds this result to SSRC1 and store final
result into SDST. If final value is greater than maximal value of 32-bit type then store
1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT64 TMP = (SSRC0<<2) + SSRC1
SDST = TMP&0xffffffff
SCC = TMP >= (1ULL<<32)
```

#### S_LSHL3_ADD_U32

Opcode: 48 (0x30) for GCN 1.4  
Syntax: S_LSHL3_ADD_U32 SDST, SRC0, SRC1  
Description: Shift left SSRC0 by 3 bits and adds this result to SSRC1 and store final
result into SDST. If final value is greater than maximal value of 32-bit type then store
1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT64 TMP = (SSRC0<<3) + SSRC1
SDST = TMP&0xffffffff
SCC = TMP >= (1ULL<<32)
```

#### S_LSHL4_ADD_U32

Opcode: 49 (0x31) for GCN 1.4  
Syntax: S_LSHL4_ADD_U32 SDST, SRC0, SRC1  
Description: Shift left SSRC0 by 4 bits and adds this result to SSRC1 and store final
result into SDST. If final value is greater than maximal value of 32-bit type then store
1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT64 TMP = (SSRC0<<4) + SSRC1
SDST = TMP&0xffffffff
SCC = TMP >= (1ULL<<32)
```

#### S_LSHR_B32

Opcode: 32 (0x20) for GCN 1.0/1.1; 30 (0x1e) for GCN 1.2/1.4  
Syntax: S_LSHR_B32 SDST, SSRC0, SSRC1  
Description: Shift right SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 >> (SSRC1 & 31)
SCC = SDST!=0
```

#### S_LSHR_B64

Opcode: 33 (0x21) for GCN 1.0/1.1; 31 (0x1f) for GCN 1.2/1.4  
Syntax: S_LSHR_B64 SDST(2), SSRC0(2), SSRC1  
Description: Shift right SSRC0 by (SSRC1&63) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC. SDST, SSRC0 are 64-bit,
SSRC1 is 32 bit.  
Operation:  
```
SDST = SSRC0 >> (SSRC1 & 63)
SCC = SDST!=0
```

#### S_MAX_I32

Opcode: 8 (0x8)  
Syntax: S_MIN_I32 SDST, SSRC0, SSRC1  
Description: Choose largest signed value value from SSRC0 and SSRC1 and store
its into SDST, and store 1 to SCC if SSRC0 value was choosen, otherwise store 0 to SCC.  
Operation:  
```
SDST = MAX((INT32)SSRC0, (INT32)SSRC1)
SCC = (INT32)SSRC0 > (INT32)SSRC1
```

#### S_MAX_U32

Opcode: 9 (0x9)  
Syntax: S_MAX_U32 SDST, SSRC0, SSRC1  
Description: Choose largest unsigned value value from SSRC0 and SSRC1 and store
its into SDST, and store 1 to SCC if SSRC0 value was choosen, otherwise store 0 to SCC.  
Operation:  
```
SDST = MAX(SSRC0, SSRC1)
SCC = SSRC0 > SSRC1
```

#### S_MIN_I32

Opcode: 6 (0x6)  
Syntax: S_MIN_I32 SDST, SSRC0, SSRC1  
Description: Choose smallest signed value value from SSRC0 and SSRC1 and store
its into SDST, and store 1 to SCC if SSRC0 value was choosen, otherwise store 0 to SCC.  
Operation:  
```
SDST = MIN((INT32)SSRC0, (INT32)SSRC1)
SCC = (INT32)SSRC0 < (INT32)SSRC1
```

#### S_MIN_U32

Opcode: 7 (0x7)  
Syntax: S_MIN_U32 SDST, SSRC0, SSRC1  
Description: Choose smallest unsigned value value from SSRC0 and SSRC1 and store
its into SDST, and store 1 to SCC if SSRC0 value was choosen, otherwise store 0 to SCC.  
Operation:  
```
SDST = MIN(SSRC0, SSRC1)
SCC = SSRC0 < SSRC1
```

#### S_MUL_HI_I32

Opcode: 45 (0x2d) for GCN 1.4  
Syntax: S_MUL_HI_I32 SDST, SSRC0, SSRC1  
Description: Multiply signed values of SSRC0 and SSRC1 and store high 32 bits of
signed result into SDST. Do no change SCC.  
Operation:  
```
SDST = ((INT64)SSRC0 * (INT32)SSRC1)>>32
```

#### S_MUL_HI_U32

Opcode: 44 (0x2c) for GCN 1.4  
Syntax: S_MUL_HI_U32 SDST, SSRC0, SSRC1  
Description: Multiply unsigned values of SSRC0 and SSRC1 and store high 32 bits of
unsigned result into SDST. Do no change SCC.  
Operation:  
```
SDST = ((UINT64)SSRC0 * SSRC1)>>32
```

#### S_MUL_I32

Opcode: 38 (0x26) for GCN 1.0/1.1; 36 (0x24) for GCN 1.2/1.4  
Syntax: S_MUL_I32 SDST, SSRC0, SSRC1  
Description: Multiply SSRC0 and SSRC1 and store result into SDST. Do not change SCC.  
Operation:  
```
SDST = SSRC0 * SSRC1
```

#### S_NAND_B32

Opcode: 24 (0x18) for GCN 1.0/1.1; 22 (0x16) for GCN 1.2/1.4  
Syntax: S_NAND_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise NAND operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = ~(SSRC0 & SSRC1)
SCC = SDST!=0
```

#### S_NAND_B64

Opcode: 25 (0x19) for GCN 1.0/1.1; 23 (0x17) for GCN 1.2/1.4  
Syntax: S_NAND_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise NAND operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = ~(SSRC0 & SSRC1)
SCC = SDST!=0
```

#### S_NOR_B32

Opcode: 26 (0x1a) for GCN 1.0/1.1; 24 (0x18) for GCN 1.2/1.4  
Syntax: S_NOR_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise NOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = ~(SSRC0 | SSRC1)
SCC = SDST!=0
```

#### S_NOR_B64

Opcode: 27 (0x1b) for GCN 1.0/1.1; 25 (0x19) for GCN 1.2/1.4  
Syntax: S_NOR_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise NOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = ~(SSRC0 | SSRC1)
SCC = SDST!=0
```

#### S_OR_B32

Opcode: 16 (0x10) for GCN 1.0/1.1; 14 (0xe) for GCN 1.2/1.4  
Syntax: S_OR_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise OR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 | SSRC1
SCC = SDST!=0
```

#### S_OR_B64

Opcode: 17 (0x11) for GCN 1.0/1.1; 15 (0xf) for GCN 1.2/1.4  
Syntax: S_OR_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise OR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 | SSRC1
SCC = SDST!=0
```

#### S_ORN2_B32

Opcode: 22 (0x16) for GCN 1.0/1.1; 20 (0x14) for GCN 1.2/1.4  
Syntax: S_ORN2_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise OR operation on SSRC0 and negated SSRC1 and store it to SDST,
and store 1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 | ~SSRC1
SCC = SDST!=0
```

#### S_ORN2_B64

Opcode: 23 (0x17) for GCN 1.0/1.1; 21 (0x15) for GCN 1.2/1.4  
Syntax: S_ORN2_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise OR operation on SSRC0 and negated SSRC1 and store it to SDST,
and store 1 to SCC if result is not zero, otherwise store 0 to SCC.
SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 | ~SSRC1
SCC = SDST!=0
```

#### S_PACK_HH_B32_B16

Opcode: 52 (0x34) for GCN 1.4  
Syntax: S_PACK_HH_B32_B16 SDST, SSRC0, SSRC1  
Description: Get last 16 bits from SSRC0 and store in first 16 bits in SDST,
Get last 16 bits from SSRC1 and store in last 16 bits in SDST. Do not change SCC.  
Operation:  
```
SDST = (SSRC0>>16) | (SSRC1&0xffff0000)
```

#### S_PACK_LH_B32_B16

Opcode: 51 (0x33) for GCN 1.4  
Syntax: S_PACK_LH_B32_B16 SDST, SSRC0, SSRC1  
Description: Get first 16 bits from SSRC0 and store in first 16 bits in SDST,
Get last 16 bits from SSRC1 and store in last 16 bits in SDST. Do not change SCC.  
Operation:  
```
SDST = (SSRC0&0xffff) | (SSRC1&0xffff0000)
```

#### S_PACK_LL_B32_B16

Opcode: 50 (0x32) for GCN 1.4  
Syntax: S_PACK_LL_B32_B16 SDST, SSRC0, SSRC1  
Description: Get first 16 bits from SSRC0 and store in first 16 bits in SDST,
Get first 16 bits from SSRC1 and store in last 16 bits in SDST. Do not change SCC.  
Operation:  
```
SDST = (SSRC0&0xffff) | ((SSRC1&0xffff)<<16)
```

#### S_RFE_RESTORE_B64

Opcode: 43 (0x2b) for GCN 1.2/1.4  
Syntax: S_RFE_RESTORE_B64 SDST(2), SSRC0(1)  
Description: Return from exception handler and set: INST_ATC = SSRC1.U32[0] ???

#### S_SUBB_U32

Opcode: 5 (0x5)  
Syntax: S_SUBB_U32 SDST, SSRC0, SSRC1  
Descrition: Subtract SSRC1 with SCC from SSRC0 value and store result into SDST and store
carry-out flag into SCC.  
Operation:  
```
UINT64 temp = (UINT64)SSRC0 - (UINT64)SSRC1 - SCC
SDST = temp
SCC = (temp>>32) & 1
```

#### S_SUB_I32

Opcode: 3 (0x3)  
Syntax: S_SUB_I32 SDST, SSRC0, SSRC1  
Description: Subtract SSRC1 from SSRC0 and store result into SDST and
store overflow flag into SCC. SCC register value can be BROKEN for some
architectures (GCN1.0)  
Operation:  
```
SDST = SSRC0 - SSRC1
INT64 temp = SEXT64(SSRC0) - SEXT64(SSRC1)
SCC = temp > ((1LL<<31)-1) || temp < (-1LL<<31)
```

#### S_SUB_U32

Opcode: 1 (0x1)  
Syntax: S_SUB_U32 SDST, SSRC0, SSRC1  
Description: Subtract SSRC1 from SSRC0 and store result into SDST and store borrow
into SCC.  
Operation:  
```
UINT64 temp = (UINT64)SSRC0 - (UINT64)SSRC1
SDST = temp
SCC = (temp>>32)!=0
```

#### S_XNOR_B32

Opcode: 28 (0x1c) for GCN 1.0/1.1; 26 (0x1a) for GCN 1.2/1.4  
Syntax: S_XNOR_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise XNOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = ~(SSRC0 ^ SSRC1)
SCC = SDST!=0
```

#### S_XNOR_B64

Opcode: 29 (0x1d) for GCN 1.0/1.1; 27 (0x1b) for GCN 1.2/1.4  
Syntax: S_XNOR_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise XNOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = ~(SSRC0 ^ SSRC1)
SCC = SDST!=0
```

#### S_XOR_B32

Opcode: 18 (0x12) for GCN 1.0/1.1; 16 (0x10) for GCN 1.2/1.4  
Syntax: S_XOR_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise XOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 ^ SSRC1
SCC = SDST!=0
```

#### S_XOR_B64

Opcode: 19 (0x13) for GCN 1.0/1.1; 17 (0x11) for GCN 1.2/1.4  
Syntax: S_XOR_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise XOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 ^ SSRC1
SCC = SDST!=0
```
