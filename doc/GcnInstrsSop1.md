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

 Opcode     | Mnemonic (GCN1.0/1.1) | Mnemonic (GCN 1.2)
------------|----------------------|------------------------
 0 (0x0)    | --                   | S_MOV_B32
 1 (0x1)    | --                   | S_MOV_B64
 2 (0x2)    | --                   | S_CMOV_B32
 3 (0x3)    | S_MOV_B32            | S_CMOV_B64
 4 (0x4)    | S_MOV_B64            | S_NOT_B32
 5 (0x5)    | S_CMOV_B32           | S_NOT_B64
 6 (0x6)    | S_CMOV_B64           | S_WQM_B32
 7 (0x7)    | S_NOT_B32            | S_WQM_B64
 8 (0x8)    | S_NOT_B64            | S_BREV_B32
 9 (0x9)    | S_WQM_B32            | S_BREV_B64
 10 (0xa)   | S_WQM_B64            | S_BCNT0_I32_B32
 11 (0xb)   | S_BREV_B32           | S_BCNT0_I32_B64
 12 (0xc)   | S_BREV_B64           | S_BCNT1_I32_B32
 13 (0xd)   | S_BCNT0_I32_B32      | S_BCNT1_I32_B64
 14 (0xe)   | S_BCNT0_I32_B64      | S_FF0_I32_B32
 15 (0xf)   | S_BCNT1_I32_B32      | S_FF0_I32_B64
 16 (0x10)  | S_BCNT1_I32_B64      | S_FF1_I32_B32
 17 (0x11)  | S_FF0_I32_B32        | S_FF1_I32_B64
 18 (0x12)  | S_FF0_I32_B64        | S_FLBIT_I32_B32
 19 (0x13)  | S_FF1_I32_B32        | S_FLBIT_I32_B64
 20 (0x14)  | S_FF1_I32_B64        | S_FLBIT_I32
 21 (0x15)  | S_FLBIT_I32_B32      | S_FLBIT_I32_I64
 22 (0x16)  | S_FLBIT_I32_B64      | S_SEXT_I32_I8
 23 (0x17)  | S_FLBIT_I32          | S_SEXT_I32_I16
 24 (0x18)  | S_FLBIT_I32_I64      | S_BITSET0_B32
 25 (0x19)  | S_SEXT_I32_I8        | S_BITSET0_B64
 26 (0x1a)  | S_SEXT_I32_I16       | S_BITSET1_B32
 27 (0x1b)  | S_BITSET0_B32        | S_BITSET1_B64
 28 (0x1c)  | S_BITSET0_B64        | S_GETPC_B64
 29 (0x1d)  | S_BITSET1_B32        | S_SETPC_B64
 30 (0x1e)  | S_BITSET1_B64        | S_SWAPPC_B64
 31 (0x1f)  | S_GETPC_B64          | S_RFE_B64
 32 (0x20)  | S_SETPC_B64          | S_AND_SAVEEXEC_B64
 33 (0x21)  | S_SWAPPC_B64         | S_OR_SAVEEXEC_B64
 34 (0x22)  | S_RFE_B64            | S_XOR_SAVEEXEC_B64
 35 (0x23)  | --                   | S_ANDN2_SAVEEXEC_B64
 36 (0x24)  | S_AND_SAVEEXEC_B64   | S_ORN2_SAVEEXEC_B64
 37 (0x25)  | S_OR_SAVEEXEC_B64    | S_NAND_SAVEEXEC_B64
 38 (0x26)  | S_XOR_SAVEEXEC_B64   | S_NOR_SAVEEXEC_B64
 39 (0x27)  | S_ANDN2_SAVEEXEC_B64 | S_XNOR_SAVEEXEC_B64
 40 (0x28)  | S_ORN2_SAVEEXEC_B64  | S_QUADMASK_B32
 41 (0x29)  | S_NAND_SAVEEXEC_B64  | S_QUADMASK_B64
 42 (0x2a)  | S_NOR_SAVEEXEC_B64   | S_MOVRELS_B32
 43 (0x2b)  | S_XNOR_SAVEEXEC_B64  | S_MOVRELS_B64
 44 (0x2c)  | S_QUADMASK_B32       | S_MOVRELD_B32
 45 (0x2d)  | S_QUADMASK_B64       | S_MOVRELD_B64
 46 (0x2e)  | S_MOVRELS_B32        | S_CBRANCH_JOIN
 47 (0x2f)  | S_MOVRELS_B64        | S_MOV_REGRD_B32
 48 (0x30)  | S_MOVRELD_B32        | S_ABS_I32
 49 (0x31)  | S_MOVRELD_B64        | S_MOV_FED_B32
 50 (0x32)  | S_CBRANCH_JOIN       | S_SET_GPR_IDX_IDX
 51 (0x33)  | S_MOV_REGRD_B32      | --
 52 (0x34)  | S_ABS_I32            | --
 53 (0x35)  | S_MOV_FED_B32        | --

### Instruction set

Alphabetically sorted instruction list:

### S_BREV_B32

Opcode: 11 (0xb) for GCN 1.0/1.1; 8 (0x8) for GCN 1.2  
Syntax: S_BREV_B32 SDST, SSRC0  
Description: Reverse bits in SSRC0 and store result to SDST. SCC is not changed.  
```
SDST = REVBIT(SSRC0)
```

### S_BREV_B64

Opcode: 12 (0xc) for GCN 1.0/1.1; 9 (0x9) for GCN 1.2  
Syntax: S_BREV_B64 SDST(2), SSRC0(2)  
Description: Reverse bits in SSRC0 and store result to SDST. SCC is not changed.
SDST and SSRC0 are 64-bit.  
```
SDST = REVBIT(SSRC0)
```

### S_CMOV_B32

Opcode: 5 (0x5) for GCN 1.0/1.1; 2 (0x2) for GCN 1.2  
Syntax: S_CMOV_B32 SDST, SSRC0  
Description: If SCC is 1, store SSRC0 into SDST, otherwise do not change SDST.
SCC is not changed.  
Operation:  
```
SDST = SCC ? SSRC0 : SDST
```

### S_CMOV_B64

Opcode: 6 (0x6) for GCN 1.0/1.1; 3 (0x3) for GCN 1.2  
Syntax: S_CMOV_B64 SDST(2), SSRC0(2)  
Description: If SCC is 1, store SSRC0 into SDST, otherwise do not change SDST.
SCC is not changed. SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = SCC ? SSRC0 : SDST
```

### S_MOV_B32

Opcode: 3 (0x3) for GCN 1.0/1.1; 0 (0x0) for GCN 1.2  
Syntax: S_MOV_B32 SDST, SSRC0  
Description: Move value of SSRC0 into SDST.  
Operation:  
```
SDST = SSRC0
```

### S_MOV_B64

Opcode: 4 (0x4) for GCN 1.0/1.1; 1 (0x1) for GCN 1.2  
Syntax: S_MOV_B64 SDST(2), SSRC0(2)  
Description: Move value of SSRC0 into SDST. SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = SSRC0
```

### S_MOV_B32

Opcode: 3 (0x3) for GCN 1.0/1.1; 0 (0x0) for GCN 1.2  
Syntax: S_MOV_B32 SDST, SSRC0  
Description: Move value of SSRC0 into SDST.  
Operation:  
```
SDST = SSRC0
```

### S_NOT_B32

Opcode: 7 (0x7) for GCN 1.0/1.1; 4 (0x4) for GCN 1.2  
Syntax: S_NOT_B32 SDST, SSRC0  
Description: Store bitwise negation of the SSRC0 into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SDST = ~SSRC0
SCC = SDST!=0
```

### S_NOT_B64

Opcode: 8 (0x8) for GCN 1.0/1.1; 5 (0x5) for GCN 1.2  
Syntax: S_NOT_B64 SDST(2), SSRC0(2)  
Description: Store bitwise negation of the SSRC0 into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
SDST = ~SSRC0
SCC = SDST!=0
```

### S_WQM_B32

Opcode: 9 (0x9) for GCN 1.0/1.1; 6 (0x6) for GCN 1.2  
Syntax: S_WQM_B32 SDST, SSRC0  
Description: For every 4-bit groups in SSRC0, if any bit of that group is set, then
set all four bits for that group, otherwise zeroes all bits; and store that result into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
UINT32 temp
for (UINT8 i = 0; i < 32; i+=4)
    temp |= ((SSRC0>>i) & 15)!=0 ? (15<<i) : 0
SDST = temp
SCC = SDST!=0
```

### S_WQM_B64

Opcode: 10 (0xa) for GCN 1.0/1.1; 7 (0x7) for GCN 1.2  
Syntax: S_WQM_B64 SDST(2), SSRC0(2)  
Description: For every 4-bit groups in SSRC0, if any bit of that group is set, then
set all four bits for that group, otherwise zeroes all bits; and store that result into SDST.
If result is non-zero, store 1 to SCC, otherwise store 0 to SCC.
SDST and SSRC0 are 64-bit.  
Operation:  
```
UINT64 temp
for (UINT8 i = 0; i < 64; i+=4)
    temp |= ((SSRC0>>i) & 15)!=0 ? (15ULL<<i) : 0
SDST = temp
SCC = SDST!=0
```
