## GCN ISA SOPC instructions

The basic encoding of the SOPC instructions needs 4 bytes (dword). List of fields:

Bits  | Name     | Description
------|----------|------------------------------
0-7   | SSRC0    | First scalar source operand. Refer to operand encoding
8-15  | SSRC1    | Second scalar source operand. Refer to operand encoding
16-22 | OPCODE   | Operation code
23-31 | ENCODING | Encoding type. Must be 0b101111110

Syntax for almost instructions: INSTRUCTION SSRC0, SSRC1

Example: s_cmp_eq_i32 s0, s1

List of the instructions by opcode:

 Opcode     | Mnemonic (GCN1.0/1.1) | Mnemonic (GCN 1.2/1.4)
------------|----------------------|------------------------
 0 (0x0)    | S_CMP_EQ_I32         | S_CMP_EQ_I32
 1 (0x1)    | S_CMP_LG_I32         | S_CMP_LG_I32
 2 (0x2)    | S_CMP_GT_I32         | S_CMP_GT_I32
 3 (0x3)    | S_CMP_GE_I32         | S_CMP_GE_I32
 4 (0x4)    | S_CMP_LT_I32         | S_CMP_LT_I32
 5 (0x5)    | S_CMP_LE_I32         | S_CMP_LE_I32
 6 (0x6)    | S_CMP_EQ_U32         | S_CMP_EQ_U32
 7 (0x7)    | S_CMP_LG_U32         | S_CMP_LG_U32
 8 (0x8)    | S_CMP_GT_U32         | S_CMP_GT_U32
 9 (0x9)    | S_CMP_GE_U32         | S_CMP_GE_U32
 10 (0xa)   | S_CMP_LT_U32         | S_CMP_LT_U32
 11 (0xb)   | S_CMP_LE_U32         | S_CMP_LE_U32
 12 (0xc)   | S_BITCMP0_B32        | S_BITCMP0_B32
 13 (0xd)   | S_BITCMP1_B32        | S_BITCMP1_B32
 14 (0xe)   | S_BITCMP0_B64        | S_BITCMP0_B64
 15 (0xf)   | S_BITCMP1_B64        | S_BITCMP1_B64
 16 (0x10)  | S_SETVSKIP           | S_SETVSKIP
 17 (0x11)  | --                   | S_SET_GPR_IDX_ON
 18 (0x12)  | --                   | S_CMP_EQ_U64
 19 (0x13)  | --                   | S_CMP_LG_U64, S_CMP_NE_U64

### Instruction set

Alphabetically sorted instruction list:

#### S_BITCMP0_B32

Opcode: 12 (0xc)  
Syntax: S_BITCMP0_B32 SSRC0, SSRC1  
Description: Test bit in SSRC0 with specified number (SSRC1&31) and if bit is clear,
store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (SSRC0 & (1U << (SSRC1&31))) == 0
```
#### S_BITCMP0_B64

Opcode: 14 (0xe)  
Syntax: S_BITCMP0_B64 SSRC0(2), SSRC1  
Description: Test bit in SSRC0 with specified number (SSRC1&63) and if bit is clear,
store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (SSRC0 & (1ULL << (SSRC1&63))) == 0
```

#### S_BITCMP1_B32

Opcode: 13 (0xd)  
Syntax: S_BITCMP1_B32 SSRC0, SSRC1  
Description: Test bit in SSRC0 with specified number (SSRC1&31) and if bit is set,
store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (SSRC0 & (1U << (SSRC1&31))) != 0
```

#### S_BITCMP1_B64

Opcode: 15 (0xf)  
Syntax: S_BITCMP1_B64 SSRC0(2), SSRC1  
Description: Test bit in SSRC0 with specified number (SSRC1&63) and if bit is set,
store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (SSRC0 & (1ULL << (SSRC1&63))) != 0
```

#### S_CMP_EQ_I32

Opcode: 0 (0x0)  
Syntax: S_CMP_EQ_I32 SSRC0, SSRC1  
Description: Compare SSRC0 to SSRC1. If SSRC0 and SSRC1 are equal, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = SSRC0==SSRC1
```

#### S_CMP_EQ_U32

Opcode: 6 (0x6)  
Syntax: S_CMP_EQ_U32 SSRC0, SSRC1  
Description: Compare SSRC0 to SSRC1. If SSRC0 and SSRC1 are equal, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = SSRC0==SSRC1
```

#### S_CMP_EQ_U64

Opcode: 18 (0x12) for GCN 1.2/1.4  
Syntax: S_CMP_EQ_U64 SSRC0(2), SSRC1(2)  
Description: Compare SSRC0 to SSRC1. If SSRC0 and SSRC1 are equal, store 1 to SCC,
otherwise store 0 to SCC. SSRC0 and SSRC1 are 64-bit.  
Operation:  
```
SCC = SSRC0==SSRC1
```

#### S_CMP_GE_I32

Opcode: 3 (0x3)  
Syntax: S_CMP_GE_I32 SSRC0, SSRC1  
Description: Compare signed SSRC0 to signed SSRC1.
If SSRC0 greater or equal to than SSRC1, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SSRC0 >= (INT32)SSRC1
```

#### S_CMP_GE_U32

Opcode: 9 (0x9)  
Syntax: S_CMP_GE_U32 SSRC0, SSRC1  
Description: Compare unsigned SSRC0 to unsigned SSRC1.
If SSRC0 greater or equal to than SSRC1, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = SSRC0 >= SSRC1
```

#### S_CMP_GT_I32

Opcode: 2 (0x2)  
Syntax: S_CMP_GT_I32 SSRC0, SSRC1  
Description: Compare signed SSRC0 to signed SSRC1.
If SSRC0 greater than SSRC1, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SSRC0 > (INT32)SSRC1
```

#### S_CMP_GT_U32

Opcode: 8 (0x8)  
Syntax: S_CMP_GT_U32 SSRC0, SSRC1  
Description: Compare unssigned SSRC0 to unsigned SSRC1.
If SSRC0 greater than SSRC1, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = SSRC0 > SSRC1
```

#### S_CMP_LE_I32

Opcode: 5 (0x5)  
Syntax: S_CMP_LE_I32 SSRC0, SSRC1  
Description: Compare signed SSRC0 to signed SSRC1.
If SSRC0 less or equal to than SSRC1, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SSRC0 <= (INT32)SSRC1
```

#### S_CMP_LE_U32

Opcode: 11 (0xb)  
Syntax: S_CMP_LE_U32 SSRC0, SSRC1  
Description: Compare unsigned SSRC0 to unsigned SSRC1.
If SSRC0 less or equal to than SSRC1, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = SSRC0 <= SSRC1
```

#### S_CMP_LG_I32

Opcode: 1 (0x1)  
Syntax: S_CMP_LG_I32 SSRC0, SSRC1  
Description: Compare SSRC0 to SSRC1. If SSRC0 and SSRC1 are not equal, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = SSRC0!=SSRC1
```

#### S_CMP_LG_U32

Opcode: 7 (0x7)  
Syntax: S_CMP_LG_I32 SSRC0, SSRC1  
Description: Compare SSRC0 to SSRC1. If SSRC0 and SSRC1 are not equal, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = SSRC0!=SSRC1
```

#### S_CMP_LG_U64, S_CMP_NE_U64

Opcode: 19 (0x13) for GCN 1.2/1.4  
Syntax: S_CMP_LG_U64 SSRC0(2), SSRC1(2)  
Syntax: S_CMP_NE_U64 SSRC0(2), SSRC1(2)  
Description: Compare SSRC0 to SSRC1. If SSRC0 and SSRC1 are equal, store 1 to SCC,
otherwise store 0 to SCC. SSRC0 and SSRC1 are 64-bit.  
Operation:  
```
SCC = SSRC0!=SSRC1
```

#### S_CMP_LT_I32

Opcode: 4 (0x4)  
Syntax: S_CMP_LT_I32 SSRC0, SSRC1  
Description: Compare signed SSRC0 to signed SSRC1.
If SSRC0 less than SSRC1, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SSRC0 < (INT32)SSRC1
```

#### S_CMP_LT_U32

Opcode: 10 (0xa)  
Syntax: S_CMP_LT_U32 SSRC0, SSRC1  
Description: Compare unsigned SSRC0 to unsigned SSRC1.
If SSRC0 less than SSRC1, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = SSRC0 < SSRC1
```

#### S_SET_GPR_IDX_ON

Opcode: 17 (0x11) for GCN 1.2/1.4  
Syntax:S_SET_GPR_IDX_ON SSRC0(0), IMM8  
Description: Enable GPR indexing mode. Set mode and index for GPR indexing. The mode
and index for GPR indexing are in M0. Refer to GPR indexing mode in [GcnOperands]  
Operation:  
```
MODE = (MODE & ~(1U<<27)) | (1U<<27)
M0 = (M0 & 0xffff0f00) | ((IMM8 & 15)<<12) | (SSRC0 & 0xff)
```

#### S_SETVSKIP

Opcode: 16 (0x10)  
Syntax: S_SETVSKIP SSRC0, SSRC1  
Description: If bit in SSRC0 with specified number (SSRC1&31) is set enable VSKIP mode
(skip all vector instructions), otherwise disable VSKIP mode.  
Operation:  
```
VSKIP = (SSRC0 & 1<<(SSRC1&31)) != 0
```
