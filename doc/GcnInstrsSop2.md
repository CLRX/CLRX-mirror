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

Syntax for all instructions: INSTRUCTION SDST, SSRC0, SSRC1

Example: s_and_b32 s0, s1, s2

List of the instructions by opcode:

Opcode | Mnemonic
-------|---------------
0      | S_ADD_U32
1      | S_SUB_U32
2      | S_ADD_I32
3      | S_SUB_I32
4      | S_ADDC_U32
5      | S_SUBB_U32
6      | S_MIN_I32
7      | S_MIN_U32
8      | S_MAX_I32
9      | S_MAX_U32
10     | S_CSELECT_B32
11     | S_CSELECT_B64
14     | S_AND_B32
15     | S_AND_B64
16     | S_OR_B32
17     | S_OR_B64
18     | S_XOR_B32
19     | S_XOR_B64

### Instruction set

Alphabetically sorted instruction list:

#### S_ADDC_U32

Opcode: 4 (0x4)  
Syntax: S_ADDC_U32 SDST, SSRC0, SSRC1  
Descrition: Add SSRC0 to SSRC1 with SCC value and store result into SDST and store
carry-out flag into SCC.  
Operation:  
```
temp = (UINT64)SSRC0 + (UINT64)SSRC1 + SCC
SDST = temp
SCC = temp>>32
```

#### S_ADD_I32

Opcode: 2 (0x2)  
Syntax: S_ADD_I32 SDST, SSRC0, SSRC1  
Description: Add SSRC0 to SSRC1 and store result into SDST and store overflow flag into SCC.  
Operation:  
```
SDST = SSRC0 + SSRC1
temp = (UINT64)SSRC0 + (UINT64)SSRC1
SCC = temp > ((1LL<<31)-1) || temp > (-1LL<<31)
```

#### S_ADD_U32

Opcode: 0 (0x0)  
Syntax: S_ADD_U32 SDST, SSRC0, SSRC1  
Description: Add SSRC0 to SSRC1 and store result into SDST and store carry-out into SCC.  
Operation:  
```
SDST = SSRC0 + SSRC1
SCC = ((UINT64)SSRC0 + (UINT64)SSRC1)>>32
```

#### S_AND_B32

Opcode: 14 (0xe)  
Syntax: S_AND_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise AND operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 & SSRC1
SCC = SDST!=0
```

#### S_AND_B64

Opcode: 15 (0xf)  
Syntax: S_AND_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise AND operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 & SSRC1
SCC = SDST!=0
```

#### S_ANDN2_B32

Opcode: 20 (0x14)  
Syntax: S_ANDN2_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise AND operation on SSRC0 and negated SSRC1 and store it to SDST,
and store 1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 & ~SSRC1
SCC = SDST!=0
```

#### S_ANDN2_B64

Opcode: 21 (0x15)  
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

Opcode: 34 (0x22)
Syntax: S_ASHR_I32 SDST, SSRC0, SSRC1  
Description: Arithmetic shift to right SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = (INT32)SSRC0 >> (SSRC1 & 31)
SCC = SDST!=0
```

#### S_ASHR_I64

Opcode: 35 (0x23)  
Syntax: S_ASHR_I64 SDST(2), SSRC0(2), SSRC1  
Description: Arithmetic Shift to right SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC. SDST, SSRC0 are 64-bit,
SSRC1 is 32 bit.  
Operation:  
```
SDST = (INT64)SSRC0 >> (SSRC1 & 63)
SCC = SDST!=0
```

#### S_BFE_U32
Opcode: 39 (0x27)  
Syntax: S_BFE_U32 SDST, SSRC0, SSRC1  
Description: Extracts bits in SSRC0 from range (SSRC1&31) with length ((SSRC1>>16)&0x7f).
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
shift = length & 31
length = (SSRC1>>16) & 0x7f
if (length==0)
    SDST = 0
if (shift+length < 32)
    SDST = SSRC0 << (32 - shift - length) >> (32 - length)
else
    SDST = SSRC0 >> shift
SCC = SDST!=0
```

#### S_BFE_I32
Opcode: 40 (0x28)  
Syntax: S_BFE_I32 SDST, SSRC0, SSRC1  
Description: Extracts bits in SSRC0 from range (SSRC1&31) with length ((SSRC1>>16)&0x7f)
and extend sign from last bit of extracted value.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
shift = length&31
length = (SSRC1>>16) & 0x7f
if (length==0)
    SDST = 0
if (shift+length < 32)
    SDST = (INT32)(SSRC0 << (32 - shift - length)) >> (32 - length)
else
    SDST = (INT32)SSRC0 >> shift
SCC = SDST!=0
```

#### S_BFM_B32
Opcode: 36 (0x24)
Syntax: S_BFM_B32 SDST, SSRC0, SSRC1  
Description: Make 32-bit bitmask from (SSRC1 & 31) bit that have length (SSRC0 & 31) and
store it to SDST. SCC not touched.  
Operation:  
```
SDST = ((1U << (SSRC0&31))-1) << (SSRC1&31)
```

#### S_BFM_B64
Opcode: 37 (0x25)
Syntax: S_BFM_B64 SDST(2), SSRC0, SSRC1  
Description: Make 64-bit bitmask from (SSRC1 & 63) bit that have length (SSRC0 & 63) and
store it to SDST. SCC not touched.  
Operation:  
```
SDST = ((1ULL << (SSRC0&63))-1) << (SSRC1&63)
```

#### S_CSELECT_B32

Opcode: 10 (0xa)  
Syntax: S_CSELECT_B32 SDST, SSRC0, SSRC1  
Description: If SCC is 1 then store SSRC0 into SDST, otherwise store SSRC1 into SDST.
SCC has not been changed.  
Operation:  
```
SDST = SCC ? SSRC0 : SSRC1
```

#### S_CSELECT_B64

Opcode: 11 (0xb)  
Syntax: S_CSELECT_B32 SDST(2), SSRC0(2), SSRC1(2)  
Description: If SCC is 1 then store 64-bit SSRC0 into SDST, otherwise store
64-bit SSRC1 into SDST. SCC has not been changed.  
Operation:  
```
SDST = SCC ? SSRC0 : SSRC1
```

#### S_LSHL_B32

Opcode: 30 (0x1e)
Syntax: S_LSHL_B32 SDST, SSRC0, SSRC1  
Description: Shift to left SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 << (SSRC1 & 31)
SCC = SDST!=0
```

#### S_LSHL_B64

Opcode: 31 (0x1f)
Syntax: S_LSHL_B64 SDST(2), SSRC0(2), SSRC1  
Description: Shift to left SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC. SDST, SSRC0 are 64-bit,
SSRC1 is 32 bit.  
Operation:  
```
SDST = SSRC0 << (SSRC1 & 63)
SCC = SDST!=0
```

#### S_LSHR_B32

Opcode: 32 (0x20)
Syntax: S_LSHR_B32 SDST, SSRC0, SSRC1  
Description: Shift to right SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 >> (SSRC1 & 31)
SCC = SDST!=0
```

#### S_LSHR_B64

Opcode: 33 (0x21)
Syntax: S_LSHR_B64 SDST(2), SSRC0(2), SSRC1  
Description: Shift to right SSRC0 by (SSRC1&31) bits and store result into SDST.
If result is non-zero store 1 to SCC, otherwise store 0 to SCC. SDST, SSRC0 are 64-bit,
SSRC1 is 32 bit.  
Operation:  
```
SDST = SSRC0 >> (SSRC1 & 63)
SCC = SDST!=0
```

#### S_MAX_I32

Opcode: 8 (0x9)
Syntax: S_MIN_I32 SDST, SSRC0, SSRC1  
Description: Choose largest signed value value from SSRC0 and SSRC1 and store its into SDST,
and store 1 to SCC if SSSRC0 value has been choosen, otherwise store 0 to SCC  
Operation:  
```
SDST = (INT32)SSSRC0 > (INT32)SSSRC1 ? SSSRC0 : SSSRC1
SCC = (INT32)SSSRC0 > (INT32)SSSRC1
```

#### S_MAX_U32

Opcode: 9 (0x9)  
Syntax: S_MAX_U32 SDST, SSRC0, SSRC1  
Description: Choose largest unsigned value value from SSRC0 and SSRC1 and store its into SDST,
and store 1 to SCC if SSSRC0 value has been choosen, otherwise store 0 to SCC  
Operation:  
```
SDST = (UINT32)SSSRC0 > (UINT32)SSSRC1 ? SSSRC0 : SSSRC1
SCC = (UINT32)SSSRC0 > (UINT32)SSSRC1
```

#### S_MIN_I32

Opcode: 6 (0x6)
Syntax: S_MIN_I32 SDST, SSRC0, SSRC1  
Description: Choose smallest signed value value from SSRC0 and SSRC1 and store its into SDST,
and store 1 to SCC if SSSRC0 value has been choosen, otherwise store 0 to SCC  
Operation:  
```
SDST = (INT32)SSSRC0 < (INT32)SSSRC1 ? SSSRC0 : SSSRC1
SCC = (INT32)SSSRC0 < (INT32)SSSRC1
```

#### S_MIN_U32

Opcode: 7 (0x7)  
Syntax: S_MIN_U32 SDST, SSRC0, SSRC1  
Description: Choose smallest unsigned value value from SSRC0 and SSRC1 and store its into SDST,
and store 1 to SCC if SSSRC0 value has been choosen, otherwise store 0 to SCC  
Operation:  
```
SDST = (UINT32)SSSRC0 < (UINT32)SSSRC1 ? SSSRC0 : SSSRC1
SCC = (UINT32)SSSRC0 < (UINT32)SSSRC1
```

#### S_MUL_I32
Opcode: 38 (0x26)
Syntax: S_MUL_I32 SDST, SSRC0, SSRC1
Description: Multiply SSRC0 and SSRC1 and store result into SDST. Do not change SCC.  
Operation:  
```
SDST = SSRC0 * SSRC1
```

#### S_NAND_B32

Opcode: 24 (0x18)  
Syntax: S_NAND_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise NAND operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = ~(SSRC0 & SSRC1)
SCC = SDST!=0
```

#### S_NAND_B64

Opcode: 25 (0x19)  
Syntax: S_NAND_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise NAND operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = ~(SSRC0 & SSRC1)
SCC = SDST!=0
```

#### S_NOR_B32

Opcode: 26 (0x1a)  
Syntax: S_NOR_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise NOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = ~(SSRC0 | SSRC1)
SCC = SDST!=0
```

#### S_NOR_B64

Opcode: 27 (0x1b)  
Syntax: S_NOR_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise NOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = ~(SSRC0 | SSRC1)
SCC = SDST!=0
```

#### S_OR_B32

Opcode: 16 (0x10)  
Syntax: S_OR_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise OR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 | SSRC1
SCC = SDST!=0
```

#### S_OR_B64

Opcode: 17 (0x11)  
Syntax: S_OR_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise OR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 | SSRC1
SCC = SDST!=0
```

#### S_ORN2_B32

Opcode: 22 (0x16)  
Syntax: S_ORN2_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise OR operation on SSRC0 and negated SSRC1 and store it to SDST,
and store 1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 | ~SSRC1
SCC = SDST!=0
```

#### S_ORN2_B64

Opcode: 23 (0x17)  
Syntax: S_ORN2_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise OR operation on SSRC0 and negated SSRC1 and store it to SDST,
and store 1 to SCC if result is not zero, otherwise store 0 to SCC.
SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 | ~SSRC1
SCC = SDST!=0
```

#### S_SUBB_U32

Opcode: 5 (0x5)  
Syntax: S_SUBB_U32 SDST, SSRC0, SSRC1  
Descrition: Subtract SSRC0 to SSRC1 with SCC value and store result into SDST and store
carry-out flag into SCC.  
Operation:  
```
temp = (UINT64)SSRC0 - (UINT64)SSRC1 - SCC
SDST = temp
SCC = temp>>32
```

#### S_SUB_I32

Opcode: 3 (0x3)  
Syntax: S_SUB_I32 SDST, SSRC0, SSRC1  
Description: Subtract SSRC0 to SSRC1 and store result into SDST and
store overflow flag into SCC. SCC register value can be BROKEN for some
architectures (GCN1.0)  
Operation:  
```
SDST = SSRC0 - SSRC1
temp = (UINT64)SSRC0 - (UINT64)SSRC1
SCC = temp>((1LL<<31)-1) || temp>(-1LL<<31)
```

#### S_SUB_U32

Opcode: 1 (0x1)  
Syntax: S_SUB_U32 SDST, SSRC0, SSRC1  
Description: Subtract SSRC0 to SSRC1 and store result into SDST and store borrow into SCC.  
Operation:  
```
SDST = SSRC0 - SSRC1
SCC = ((INT64)SSRC0 - (INT64)SSRC1)>>32
```

#### S_XNOR_B32

Opcode: 28 (0x1c)  
Syntax: S_XNOR_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise XNOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = ~(SSRC0 ^ SSRC1)
SCC = SDST!=0
```

#### S_XNOR_B64

Opcode: 29 (0x1d)  
Syntax: S_XNOR_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise XNOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = ~(SSRC0 ^ SSRC1)
SCC = SDST!=0
```

#### S_XOR_B32

Opcode: 18 (0x12)  
Syntax: S_XOR_B32 SDST, SSRC0, SSRC1  
Description: Do bitwise XOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC.  
Operation:  
```
SDST = SSRC0 ^ SSRC1
SCC = SDST!=0
```

#### S_XOR_B64

Opcode: 19 (0x13)  
Syntax: S_XOR_B64 SDST(2), SSRC0(2), SSRC1(2)  
Description: Do bitwise XOR operation on SSRC0 and SSRC1 and store it to SDST, and store
1 to SCC if result is not zero, otherwise store 0 to SCC. SDST, SSRC0, SSRC1 are 64-bit.  
Operation:  
```
SDST = SSRC0 ^ SSRC1
SCC = SDST!=0
```
