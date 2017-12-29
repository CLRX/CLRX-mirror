## GCN ISA SOP1 instructions

The basic encoding of the SOP1 instructions needs 4 bytes (dword). List of fields:

Bits  | Name     | Description
------|----------|------------------------------
0-7   | SSRC0    | Scalar source operand. Refer to operand encoding
8-15  | OPCODE   | Operation code
16-22 | SDST     | Destination scalar operand. Refer to operand encoding
23-31 | ENCODING | Encoding type. Must be 0b101111101

Syntax for almost instructions: INSTRUCTION SDST, SSRC0

Example: s_mov_b32 s0, s1

List of the instructions by opcode:

 Opcode     | Mnemonic (GCN1.0/1.1) | Mnemonic (GCN 1.2)  | Mnemonic (GCN 1.4)
------------|----------------------|----------------------|------------------------
 0 (0x0)    | --                   | S_MOV_B32            | S_MOV_B32
 1 (0x1)    | --                   | S_MOV_B64            | S_MOV_B64
 2 (0x2)    | --                   | S_CMOV_B32           | S_CMOV_B32
 3 (0x3)    | S_MOV_B32            | S_CMOV_B64           | S_CMOV_B64
 4 (0x4)    | S_MOV_B64            | S_NOT_B32            | S_NOT_B32
 5 (0x5)    | S_CMOV_B32           | S_NOT_B64            | S_NOT_B64
 6 (0x6)    | S_CMOV_B64           | S_WQM_B32            | S_WQM_B32
 7 (0x7)    | S_NOT_B32            | S_WQM_B64            | S_WQM_B64
 8 (0x8)    | S_NOT_B64            | S_BREV_B32           | S_BREV_B32
 9 (0x9)    | S_WQM_B32            | S_BREV_B64           | S_BREV_B64
 10 (0xa)   | S_WQM_B64            | S_BCNT0_I32_B32      | S_BCNT0_I32_B32
 11 (0xb)   | S_BREV_B32           | S_BCNT0_I32_B64      | S_BCNT0_I32_B64
 12 (0xc)   | S_BREV_B64           | S_BCNT1_I32_B32      | S_BCNT1_I32_B32
 13 (0xd)   | S_BCNT0_I32_B32      | S_BCNT1_I32_B64      | S_BCNT1_I32_B64
 14 (0xe)   | S_BCNT0_I32_B64      | S_FF0_I32_B32        | S_FF0_I32_B32
 15 (0xf)   | S_BCNT1_I32_B32      | S_FF0_I32_B64        | S_FF0_I32_B64
 16 (0x10)  | S_BCNT1_I32_B64      | S_FF1_I32_B32        | S_FF1_I32_B32
 17 (0x11)  | S_FF0_I32_B32        | S_FF1_I32_B64        | S_FF1_I32_B64
 18 (0x12)  | S_FF0_I32_B64        | S_FLBIT_I32_B32      | S_FLBIT_I32_B32
 19 (0x13)  | S_FF1_I32_B32        | S_FLBIT_I32_B64      | S_FLBIT_I32_B64
 20 (0x14)  | S_FF1_I32_B64        | S_FLBIT_I32          | S_FLBIT_I32
 21 (0x15)  | S_FLBIT_I32_B32      | S_FLBIT_I32_I64      | S_FLBIT_I32_I64
 22 (0x16)  | S_FLBIT_I32_B64      | S_SEXT_I32_I8        | S_SEXT_I32_I8
 23 (0x17)  | S_FLBIT_I32          | S_SEXT_I32_I16       | S_SEXT_I32_I16
 24 (0x18)  | S_FLBIT_I32_I64      | S_BITSET0_B32        | S_BITSET0_B32
 25 (0x19)  | S_SEXT_I32_I8        | S_BITSET0_B64        | S_BITSET0_B64
 26 (0x1a)  | S_SEXT_I32_I16       | S_BITSET1_B32        | S_BITSET1_B32
 27 (0x1b)  | S_BITSET0_B32        | S_BITSET1_B64        | S_BITSET1_B64
 28 (0x1c)  | S_BITSET0_B64        | S_GETPC_B64          | S_GETPC_B64
 29 (0x1d)  | S_BITSET1_B32        | S_SETPC_B64          | S_SETPC_B64
 30 (0x1e)  | S_BITSET1_B64        | S_SWAPPC_B64         | S_SWAPPC_B64
 31 (0x1f)  | S_GETPC_B64          | S_RFE_B64            | S_RFE_B64
 32 (0x20)  | S_SETPC_B64          | S_AND_SAVEEXEC_B64   | S_AND_SAVEEXEC_B64
 33 (0x21)  | S_SWAPPC_B64         | S_OR_SAVEEXEC_B64    | S_OR_SAVEEXEC_B64
 34 (0x22)  | S_RFE_B64            | S_XOR_SAVEEXEC_B64   | S_XOR_SAVEEXEC_B64
 35 (0x23)  | --                   | S_ANDN2_SAVEEXEC_B64 | S_ANDN2_SAVEEXEC_B64
 36 (0x24)  | S_AND_SAVEEXEC_B64   | S_ORN2_SAVEEXEC_B64  | S_ORN2_SAVEEXEC_B64
 37 (0x25)  | S_OR_SAVEEXEC_B64    | S_NAND_SAVEEXEC_B64  | S_NAND_SAVEEXEC_B64
 38 (0x26)  | S_XOR_SAVEEXEC_B64   | S_NOR_SAVEEXEC_B64   | S_NOR_SAVEEXEC_B64
 39 (0x27)  | S_ANDN2_SAVEEXEC_B64 | S_XNOR_SAVEEXEC_B64  | S_XNOR_SAVEEXEC_B64
 40 (0x28)  | S_ORN2_SAVEEXEC_B64  | S_QUADMASK_B32       | S_QUADMASK_B32
 41 (0x29)  | S_NAND_SAVEEXEC_B64  | S_QUADMASK_B64       | S_QUADMASK_B64
 42 (0x2a)  | S_NOR_SAVEEXEC_B64   | S_MOVRELS_B32        | S_MOVRELS_B32
 43 (0x2b)  | S_XNOR_SAVEEXEC_B64  | S_MOVRELS_B64        | S_MOVRELS_B64
 44 (0x2c)  | S_QUADMASK_B32       | S_MOVRELD_B32        | S_MOVRELD_B32
 45 (0x2d)  | S_QUADMASK_B64       | S_MOVRELD_B64        | S_MOVRELD_B64
 46 (0x2e)  | S_MOVRELS_B32        | S_CBRANCH_JOIN       | S_CBRANCH_JOIN
 47 (0x2f)  | S_MOVRELS_B64        | S_MOV_REGRD_B32      | S_MOV_REGRD_B32
 48 (0x30)  | S_MOVRELD_B32        | S_ABS_I32            | S_ABS_I32
 49 (0x31)  | S_MOVRELD_B64        | S_MOV_FED_B32        | S_MOV_FED_B32
 50 (0x32)  | S_CBRANCH_JOIN       | S_SET_GPR_IDX_IDX    | S_SET_GPR_IDX_IDX
 51 (0x33)  | S_MOV_REGRD_B32      | --                   | S_ANDN1_SAVEEXEC_B64
 52 (0x34)  | S_ABS_I32            | --                   | S_ORN1_SAVEEXEC_B64
 53 (0x35)  | S_MOV_FED_B32        | --                   | S_ANDN1_WREXEC_B64
 54 (0x36)  | --                   | --                   | S_ANDN2_WREXEC_B64
 55 (0x37)  | --                   | --                   | S_BITREPLICATE_B64_B32

### Instruction set

Alphabetically sorted instruction list:

#### S_ABS_I32

Opcode: 52 (0x34) for GCN 1.0/1.1; 48 (0x30) for GCN 1.2/1.4  
Syntax: S_ABS_B32 SDST, SSRC0  
Description: Store absolute signed value of the SSRC0 into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = ABS(SSRC0)
SCC = SDST!=0
```

#### S_AND_SAVEEXEC_B64

Opcode: 36 (0x24) for GCN 1.0/1.1; 32 (0x20) for GCN 1.2/1.4  
Syntax: S_AND_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise AND on SSRC0 and EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = SSRC0 & EXEC
SCC = EXEC!=0
```

#### S_ANDN1_SAVEEXEC_B64

Opcode: 51 (0x33) for GCN 1.4  
Syntax: S_ANDN2_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise AND on negated SSRC0 and EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = ~SSRC0 & EXEC
SCC = EXEC!=0
```

#### S_ANDN1_WREXEC_B64

Opcode: 53 (0x35) for GCN 1.4  
Syntax: S_ANDN1_WREXEC_B64 SDST(2), SSRC0(2)  
Description: Make bitwise AND on negated SSRC0 and EXEC
and store result to EXEC and SDST. If result is non-zero, store 1 to SCC,
otherwise store 0 to SCC. SDST and SSRC0 are 64-bit.  
Operation:  
```
EXEC = ~SSRC0 & EXEC
SDST = EXEC
SCC = EXEC!=0
```

#### S_ANDN2_SAVEEXEC_B64

Opcode: 39 (0x27) for GCN 1.0/1.1; 35 (0x23) for GCN 1.2/1.4  
Syntax: S_ANDN2_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise AND on SSRC0 and negated EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = SSRC0 & ~EXEC
SCC = EXEC!=0
```

#### S_ANDN2_WREXEC_B64

Opcode: 54 (0x36) for GCN 1.4  
Syntax: S_ANDN2_WREXEC_B64 SDST(2), SSRC0(2)  
Description: Make bitwise AND on SSRC0 and negated EXEC
and store result to EXEC and SDST. If result is non-zero, store 1 to SCC,
otherwise store 0 to SCC. SDST and SSRC0 are 64-bit.  
Operation:  
```
EXEC = SSRC0 & ~EXEC
SDST = EXEC
SCC = EXEC!=0
```

#### S_BCNT0_I32_B32

Opcode: 13 (0xd) for GCN 1.0/1.1; 10 (0xa) for GCN 1.2/1.4  
Syntax: S_BCNT0_I32_B32 SDST, SSRC0  
Description: Count zero bits in SSRC0 and store result to SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = BITCOUNT(~SSRC0)
SCC = SDST!=0
```

#### S_BCNT0_I32_B64

Opcode: 14 (0xd) for GCN 1.0/1.1; 11 (0xb) for GCN 1.2/1.4  
Syntax: S_BCNT0_I32_B64 SDST, SSRC0(2)  
Description: Count zero bits in SSRC0 and store result to SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC. SSRC0 is 64-bit.  
Operation:  
```
SDST = BITCOUNT(~SSRC0)
SCC = SDST!=0
```

#### S_BCNT1_I32_B32

Opcode: 15 (0xf) for GCN 1.0/1.1; 12 (0xc) for GCN 1.2/1.4  
Syntax: S_BCNT1_I32_B64 SDST, SSRC0  
Description: Count one bits in SSRC0 and store result to SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = BITCOUNT(SSRC0)
SCC = SDST!=0
```

#### S_BCNT1_I32_B64

Opcode: 16 (0x10) for GCN 1.0/1.1; 13 (0xd) for GCN 1.2/1.4  
Syntax: S_BCNT1_I32_B64 SDST, SSRC0(2)  
Description: Count one bits in SSRC0 and store result to SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC. SSRC0 is 64-bit.  
Operation:  
```
SDST = BITCOUNT(SSRC0)
SCC = SDST!=0
```

#### S_BITREPLICATE_B64_B32

Opcode: 55 (0x37) for GCN 1.4  
Syntax: S_BITREPLICATE_B64_B32 SDST(2), SSRC0  
Description: Get bits from SSRC0 and doubles and store them to SDST.  
Operation:  
```
SDST = 0
for (BYTE I=0; I<32; I++)
    SDST |= (((SSRC0>>I)&1)*3)<<(I<<1)
```

#### S_BITSET0_B32

Opcode: 27 (0x1b) for GCN 1.0/1.1, 24 (0x18) for GCN 1.2/1.4  
Syntax: S_BITSET0_B32 SDST, SSRC0  
Description: Get value from SDST, clear its bit with number specified from SSRC0, and
store result to SDST.  
Operation:  
```
SDST &= ~(1U << (SSRC0&31))
```

#### S_BITSET0_B64

Opcode: 28 (0x1c) for GCN 1.0/1.1, 25 (0x19) for GCN 1.2/1.4  
Syntax: S_BITSET0_B64 SDST(2), SSRC0  
Description: Get value from SDST, clear its bit with number specified from SSRC0, and
store result to SDST. SDST is 64-bit.  
Operation:  
```
SDST &= ~(1ULL << (SSRC0&63))
```

#### S_BITSET1_B32

Opcode: 29 (0x1d) for GCN 1.0/1.1, 26 (0x1a) for GCN 1.2/1.4  
Syntax: S_BITSET1_B32 SDST, SSRC0  
Description: Get value from SDST, set its bit with number specified from SSRC0, and
store result to SDST.  
Operation:  
```
SDST |= 1U << (SSRC0&31)
```

#### S_BITSET1_B64

Opcode: 30 (0x1e) for GCN 1.0/1.1, 27 (0x1c) for GCN 1.2/1.4  
Syntax: S_BITSET1_B64 SDST(2), SSRC0  
Description: Get value from SDST, set its bit with number specified from SSRC0, and
store result to SDST. SDST is 64-bit.  
Operation:  
```
SDST |= 1ULL << (SSRC0&63)
```

#### S_BREV_B32

Opcode: 11 (0xb) for GCN 1.0/1.1; 8 (0x8) for GCN 1.2/1.4  
Syntax: S_BREV_B32 SDST, SSRC0  
Description: Reverse bits in SSRC0 and store result to SDST. SCC is not changed.  
Operation:  
```
SDST = REVBIT(SSRC0)
```

#### S_BREV_B64

Opcode: 12 (0xc) for GCN 1.0/1.1; 9 (0x9) for GCN 1.2/1.4  
Syntax: S_BREV_B64 SDST(2), SSRC0(2)  
Description: Reverse bits in SSRC0 and store result to SDST. SCC is not changed.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = REVBIT(SSRC0)
```

#### S_CBRANCH_JOIN

Opcode: 50 (0x32) for GCN 1.0/1.1; 46 (0x2e) for GCN 1.2/1.4  
Syntax: S_CBRANCH_JOIN SSRC0  
Description: Join conditional branch that begin from S_CBRANCH_*_FORK. If control stack
pointer have same value as SSRC0 then do nothing and jump to next instruction, otherwise
pop from control stack program counter and EXEC value.  
Operation:  
```
if (CSP==SSRC0)
    PC += 4
else
{
    CSP--
    EXEC = SGPR[CSP*4:CSP*4+1]
    PC = SGPRS[CSP*4+2:CSP*4+3]
}
```

#### S_CMOV_B32

Opcode: 5 (0x5) for GCN 1.0/1.1; 2 (0x2) for GCN 1.2/1.4  
Syntax: S_CMOV_B32 SDST, SSRC0  
Description: If SCC is 1, store SSRC0 into SDST, otherwise do not change SDST.
SCC is not changed.  
Operation:  
```
SDST = SCC ? SSRC0 : SDST
```

#### S_CMOV_B64

Opcode: 6 (0x6) for GCN 1.0/1.1; 3 (0x3) for GCN 1.2/1.4  
Syntax: S_CMOV_B64 SDST(2), SSRC0(2)  
Description: If SCC is 1, store SSRC0 into SDST, otherwise do not change SDST.
SCC is not changed. SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = SCC ? SSRC0 : SDST
```

#### S_FF0_I32_B32

Opcode: 17 (0x11) for GCN 1.0/1.1; 14 (0xe) for GCN 1.2/1.4  
Syntax: S_FF0_I32_B32 SDST, SSRC0  
Description: Find first zero bit in SSRC0. If found, store number of bit to SDST,
otherwise set SDST to -1.  
Operation:  
```
SDST = -1
for (UINT8 i = 0; i < 32; i++)
    if ((1U<<i) & SSRC0) == 0)
    { SDST = i; break; }
```

#### S_FF0_I32_B64

Opcode: 18 (0x12) for GCN 1.0/1.1; 15 (0xf) for GCN 1.2/1.4  
Syntax: S_FF0_I32_B64 SDST, SSRC0(2)  
Description: Find first zero bit in SSRC0. If found, store number of bit to SDST,
otherwise set SDST to -1. SSRC0 is 64-bit.  
Operation:  
```
SDST = -1
for (UINT8 i = 0; i < 64; i++)
    if ((1ULL<<i) & SSRC0) == 0)
    { SDST = i; break; }
```

#### S_FF1_I32_B32

Opcode: 19 (0x13) for GCN 1.0/1.1; 16 (0x10) for GCN 1.2/1.4  
Syntax: S_FF1_I32_B32 SDST, SSRC0  
Description: Find first one bit in SSRC0. If found, store number of bit to SDST,
otherwise set SDST to -1.  
Operation:  
```
SDST = -1
for (UINT8 i = 0; i < 32; i++)
    if ((1U<<i) & SSRC0) != 0)
    { SDST = i; break; }
```

#### S_FF1_I32_B64

Opcode: 20 (0x14) for GCN 1.0/1.1; 17 (0x11) for GCN 1.2/1.4  
Syntax: S_FF0_I32_B64 SDST, SSRC0(2)  
Description: Find first one bit in SSRC0. If found, store number of bit to SDST,
otherwise set SDST to -1. SSRC0 is 64-bit.  
Operation:  
```
SDST = -1
for (UINT8 i = 0; i < 64; i++)
    if ((1ULL<<i) & SSRC0) != 0)
    { SDST = i; break; }
```

#### S_FLBIT_I32_B32

Opcode: 21 (0x15) for GCN 1.0/1.1; 18 (0x12) for GCN 1.2/1.4  
Syntax: S_FLBIT_I32_B32 SDST, SSRC0  
Description: Find last one bit in SSRC0. If found, store number of skipped bits to SDST,
otherwise set SDST to -1.  
Operation:  
```
SDST = -1
for (INT8 i = 31; i >= 0; i--)
    if ((1U<<i) & SSRC0) != 0)
    { SDST = 31-i; break; }
```

#### S_FLBIT_I32_B64

Opcode: 22 (0x16) for GCN 1.0/1.1; 19 (0x13) for GCN 1.2/1.4  
Syntax: S_FLBIT_I32_B64 SDST, SSRC0(2)  
Description: Find last one bit in SSRC0. If found, store number of skipped bits to SDST,
otherwise set SDST to -1.  SSRC0 is 64-bit.  
Operation:  
```
SDST = -1
for (INT8 i = 63; i >= 0; i--)
    if ((1ULL<<i) & SSRC0) != 0)
    { SDST = 63-i; break; }
```

#### S_FLBIT_I32

Opcode: 23 (0x17) for GCN 1.0/1.1; 20 (0x14) for GCN 1.2/1.4  
Syntax: S_FLBIT_I32 SDST, SSRC0  
Description: Find last opposite bit to sign in SSRC0. If found, store number of skipped bits
to SDST, otherwise set SDST to -1.  
Operation:  
```
SDST = -1
UINT32 bitval = (INT32)SSRC0>=0 ? 1 : 0
for (INT8 i = 31; i >= 0; i--)
    if ((1U<<i) & SSRC0) == (bitval<<i))
    { SDST = 31-i; break; }
```

#### S_FLBIT_I32_I64

Opcode: 24 (0x18) for GCN 1.0/1.1; 21 (0x15) for GCN 1.2/1.4  
Syntax: S_FLBIT_I32_I64 SDST, SSRC0(2)  
Description: Find last opposite bit to sign in SSRC0. If found, store number of skipped bits
to SDST, otherwise set SDST to -1. SSRC0 is 64-bit.  
Operation:  
```
SDST = -1
UINT64 bitval = (INT64)SSRC0>=0 ? 1 : 0
for (INT8 i = 63; i >= 0; i--)
    if ((1U<<i) & SSRC0) == (bitval<<i))
    { SDST = 63-i; break; }
```

#### S_GETPC_B64

Opcode: 31 (0x1f) for GCN 1.0/1.1; 28 (0x1c) for GCN 1.2/1.4  
Syntax: S_GETPC_B64 SDST(2)  
Description: Store program counter (PC) for next instruction to SDST. SDST is 64-bit.  
Operation:  
```
SDST = PC + 4
```

#### S_MOV_B32

Opcode: 3 (0x3) for GCN 1.0/1.1; 0 (0x0) for GCN 1.2/1.4  
Syntax: S_MOV_B32 SDST, SSRC0  
Description: Move value of SSRC0 into SDST.  
Operation:  
```
SDST = SSRC0
```

#### S_MOV_B64

Opcode: 4 (0x4) for GCN 1.0/1.1; 1 (0x1) for GCN 1.2/1.4  
Syntax: S_MOV_B64 SDST(2), SSRC0(2)  
Description: Move value of SSRC0 into SDST. SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = SSRC0
```

#### S_MOVRELD_B32

Opcode: 48 (0x30) for GCN 1.0/1.1; 44 (0x2c) for GCN 1.2/1.4  
Syntax: S_MOVRELD_B32 SDST, SSRC0  
Description: Store value from SSRC0 to SGPR[SDST_NUMBER+M0 : SDST_NUMBER+M0+1].
SDST_NUMBER is number of SDST register.  
Operation:  
```
SGPR[SDST_NUMBER + M0] = SSRC0
```

#### S_MOVRELD_B64

Opcode: 49 (0x31) for GCN 1.0/1.1; 45 (0x2d) for GCN 1.2/1.4  
Syntax: S_MOVRELD_B64 SDST, SSRC0  
Description: Store value from SSRC0 to SGPR[SDST_NUMBER+M0].
SDST_NUMBER is number of SDST register. SDST and SSRC0 are 64-bit.  
Operation:  
```
SGPR[SDST_NUMBER + M0 : SDST_NUMBER + M0 + 1] = SSRC0
```

#### S_MOVRELS_B32

Opcode: 46 (0x2e) for GCN 1.0/1.1; 42 (0x2a) for GCN 1.2/1.4  
Syntax: S_MOVRELS_B32 SDST, SSRC0  
Description: Store value from SGPR[M0+SSRC0_NUMBER] to SDST.
SSRC0_NUMBER is number of SSRC0 register.  
Operation:  
```
SDST = SGPR[SSRC0_NUMBER + M0]
```

#### S_MOVRELS_B64

Opcode: 47 (0x2f) for GCN 1.0/1.1; 43 (0x2b) for GCN 1.2/1.4  
Syntax: S_MOVRELS_B64 SDST(2), SSRC0(2)  
Description: Store 64-bit value from SGPR[M0+SSRC0_NUMBER : M0+SSRC0_NUMBER+1] to SDST.
SSRC0_NUMBER is number of SSRC0 register. SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = SGPR[SSRC0_NUMBER + M0 : SSRC0_NUMBER + M0 + 1]
```

#### S_NAND_SAVEEXEC_B64

Opcode: 41 (0x29) for GCN 1.0/1.1; 37 (0x25) for GCN 1.2/1.4  
Syntax: S_NAND_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise NAND on SSRC0 and EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = ~(SSRC0 & EXEC)
SCC = EXEC!=0
```

#### S_NOR_SAVEEXEC_B64

Opcode: 42 (0x2a) for GCN 1.0/1.1; 38 (0x26) for GCN 1.2/1.4  
Syntax: S_NOR_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise NOR on SSRC0 and EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = ~(SSRC0 | EXEC)
SCC = EXEC!=0
```

#### S_NOT_B32

Opcode: 7 (0x7) for GCN 1.0/1.1; 4 (0x4) for GCN 1.2/1.4  
Syntax: S_NOT_B32 SDST, SSRC0  
Description: Store bitwise negation of the SSRC0 into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = ~SSRC0
SCC = SDST!=0
```

#### S_NOT_B64

Opcode: 8 (0x8) for GCN 1.0/1.1; 5 (0x5) for GCN 1.2/1.4  
Syntax: S_NOT_B64 SDST(2), SSRC0(2)  
Description: Store bitwise negation of the SSRC0 into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = ~SSRC0
SCC = SDST!=0
```

#### S_OR_SAVEEXEC_B64

Opcode: 37 (0x25) for GCN 1.0/1.1; 33 (0x21) for GCN 1.2/1.4  
Syntax: S_OR_SAVEEXEC_B64 SDST(2), SDST(2)  
Description: Store EXEC register to SDST. Make bitwise OR on SSRC0 and EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = SSRC0 | EXEC
SCC = EXEC!=0
```

#### S_ORN2_SAVEEXEC_B64

Opcode: 52 (0x34) for GCN 1.4  
Syntax: S_ORN2_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise OR on negated SSRC0 and EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = ~SSRC0 & EXEC
SCC = EXEC!=0
```

#### S_ORN2_SAVEEXEC_B64

Opcode: 40 (0x28) for GCN 1.0/1.1; 36 (0x24) for GCN 1.2/1.4  
Syntax: S_ORN2_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise OR on SSRC0 and negated EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = SSRC0 & ~EXEC
SCC = EXEC!=0
```

#### S_QUADMASK_B32

Opcode: 44 (0x2c) for GCN 1.0/1.1; 40 (0x28) for GCN 1.2/1.4  
Syntax: S_QUADMASK_B32 SDST, SSRC0  
Description: For every 4-bit groups in SSRC0, if any bit of that group is set, then
set bit for that group in order, otherwise clear that bits; and store that result into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT32 temp = 0
for (UINT8 i = 0; i < 8; i++)
    temp |= ((SSRC0>>(i<<2)) & 15)!=0 ? (1U<<i) : 0
SDST = temp
SCC = SDST!=0
```

#### S_QUADMASK_B64

Opcode: 45 (0x2d) for GCN 1.0/1.1; 41 (0x29) for GCN 1.2/1.4  
Syntax: S_QUADMASK_B64 SDST(2), SSRC0(2)  
Description: For every 4-bit groups in SSRC0, if any bit of that group is set, then
set bit for that group in order, otherwise clear that bits; and store that result into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
UINT64 temp = 0
for (UINT8 i = 0; i < 16; i++)
    temp |= ((SSRC0>>(i<<2)) & 15)!=0 ? (1U<<i) : 0
SDST = temp
SCC = SDST!=0
```

#### S_RFE_B64

Opcode: 34 (0x22) for GCN 1.0/1.1; 31 (0x1f) for GCN 1.2/1.4  
Syntac: S_RFE_B64 SSRC0(2)  
Description: Return from exception (store TTMP[0:1] to PC ???).  
Operation: ???  
```
PC = TTMP[0:1]
```

#### S_SET_GPR_IDX_IDX

Opcode: 50 (0x32) for GCN 1.2/1.4  
Syntax S_SET_GPR_IDX_IDX SSRC0(1)  
Description: Move lowest 8 bits from SSRC0 to lowest 8 bits M0.  
Operation:  
```
M0 = (M0 & 0xffffff00) | (SSRC0 & 0xff)
```

#### S_SETPC_B64

Opcode: 32 (0x20) for GCN 1.0/1.1; 29 (0x1d) for GCN 1.2/1.4  
Syntax: S_SETPC_B64 SSRC0(2)  
Description: Jump to address given SSRC0 (store SSRC0 to PC). SSRC0 is 64-bit.  
Operation:  
```
PC = SSRC0
```

#### S_SEXT_I32_I8

Opcode: 25 (0x19) for GCN 1.0/1.1; 22 (0x16) for GCN 1.2/1.4  
Syntax: S_SEXT_I32_I8 SDST, SSRC0  
Description: Store signed extended 8-bit value from SSRC0 to SDST.  
Operation:  
```
SDST = SEXT((INT8)SSRC0)
```

#### S_SEXT_I32_I16

Opcode: 26 (0x1a) for GCN 1.0/1.1; 23 (0x17) for GCN 1.2/1.4  
Syntax: S_SEXT_I32_I16 SDST, SSRC0  
Description: Store signed extended 16-bit value from SSRC0 to SDST.  
Operation:  
```
SDST = SEXT((INT16)SSRC0)
```

#### S_SWAPPC_B64

Opcode: 33 (0x21) for GCN 1.0/1.1; 30 (0x1e) for GCN 1.2/1.4  
Syntax: S_SWAPPC_B64 SDST(2), SSRC0(2)  
Description: Store program counter to SDST and jump to address given SSRC0
(store SSRC0 to PC). SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = PC + 4
PC = SSRC0
```

#### S_WQM_B32

Opcode: 9 (0x9) for GCN 1.0/1.1; 6 (0x6) for GCN 1.2/1.4  
Syntax: S_WQM_B32 SDST, SSRC0  
Description: For every 4-bit groups in SSRC0, if any bit of that group is set, then
set all four bits for that group, otherwise zeroes all bits; and store that result into
SDST. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT32 temp = 0
for (UINT8 i = 0; i < 32; i+=4)
    temp |= ((SSRC0>>i) & 15)!=0 ? (15<<i) : 0
SDST = temp
SCC = SDST!=0
```

#### S_WQM_B64

Opcode: 10 (0xa) for GCN 1.0/1.1; 7 (0x7) for GCN 1.2/1.4  
Syntax: S_WQM_B64 SDST(2), SSRC0(2)  
Description: For every 4-bit groups in SSRC0, if any bit of that group is set, then
set all four bits for that group, otherwise zeroes all bits; and store that result into
SDST. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
UINT64 temp = 0
for (UINT8 i = 0; i < 64; i+=4)
    temp |= ((SSRC0>>i) & 15)!=0 ? (15ULL<<i) : 0
SDST = temp
SCC = SDST!=0
```

#### S_XNOR_SAVEEXEC_B64

Opcode: 43 (0x2b) for GCN 1.0/1.1; 39 (0x27) for GCN 1.2/1.4  
Syntax: S_XNOR_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise XNOR on SSRC0 and EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = ~(SSRC0 ^ EXEC)
SCC = EXEC!=0
```

#### S_XOR_SAVEEXEC_B64

Opcode: 38 (0x26) for GCN 1.0/1.1; 34 (0x22) for GCN 1.2/1.4  
Syntax: S_XOR_SAVEEXEC_B64 SDST(2), SSRC0(2)  
Description: Store EXEC register to SDST. Make bitwise XOR on SSRC0 and EXEC
and store result to EXEC. If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = EXEC
EXEC = SSRC0 ^ EXEC
SCC = EXEC!=0
```
