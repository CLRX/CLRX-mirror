## GCN ISA SOPK instructions

The basic encoding of the SOPK instructions needs 4 bytes (dword). List of fields:

Bits  | Name     | Description
------|----------|------------------------------
0-15  | SIMM16   | Signed or unsigned 16-bit immediate value
16-22 | SDST     | Destination scalar operand. Refer to operand encoding
23-27 | OPCODE   | Operation code
28-31 | ENCODING | Encoding type. Must be 0b1011

Syntax for almost instructions: INSTRUCTION SDST, SIMM16

SIMM16 - signed 16-bit immediate. IMM16 - unsigned 16-bit immediate.  
RELADDR - relative offset to this instruction (can be label or relative expresion).
RELADDR = NEXTPC + SIMM16, NEXTPC - PC for next instruction.

List of the instructions by opcode:

 Opcode     | Mnemonic (GCN1.0/1.1) | Mnemonic (GCN 1.2) | Mnemonic (GCN 1.4)
------------|----------------------|---------------------|-----------------------
 0 (0x0)    | S_MOVK_I32           | S_MOVK_I32          | S_MOVK_I32
 1 (0x1)    | --                   | S_CMOVK_I32         | S_CMOVK_I32
 2 (0x2)    | S_CMOVK_I32          | S_CMPK_EQ_I32       | S_CMPK_EQ_I32
 3 (0x3)    | S_CMPK_EQ_I32        | S_CMPK_LG_I32       | S_CMPK_LG_I32
 4 (0x4)    | S_CMPK_LG_I32        | S_CMPK_GT_I32       | S_CMPK_GT_I32
 5 (0x5)    | S_CMPK_GT_I32        | S_CMPK_GE_I32       | S_CMPK_GE_I32
 6 (0x6)    | S_CMPK_GE_I32        | S_CMPK_LT_I32       | S_CMPK_LT_I32
 7 (0x7)    | S_CMPK_LT_I32        | S_CMPK_LE_I32       | S_CMPK_LE_I32
 8 (0x8)    | S_CMPK_LE_I32        | S_CMPK_EQ_U32       | S_CMPK_EQ_U32
 9 (0x9)    | S_CMPK_EQ_U32        | S_CMPK_LG_U32       | S_CMPK_LG_U32
 10 (0xa)   | S_CMPK_LG_U32        | S_CMPK_GT_U32       | S_CMPK_GT_U32
 11 (0xb)   | S_CMPK_GT_U32        | S_CMPK_GE_U32       | S_CMPK_GE_U32
 12 (0xc)   | S_CMPK_GE_U32        | S_CMPK_LT_U32       | S_CMPK_LT_U32
 13 (0xd)   | S_CMPK_LT_U32        | S_CMPK_LE_U32       | S_CMPK_LE_U32
 14 (0xe)   | S_CMPK_LE_U32        | S_ADDK_I32          | S_ADDK_I32
 15 (0xf)   | S_ADDK_I32           | S_MULK_I32          | S_MULK_I32
 16 (0x10)  | S_MULK_I32           | S_CBRANCH_I_FORK    | S_CBRANCH_I_FORK
 17 (0x11)  | S_CBRANCH_I_FORK     | S_GETREG_B32        | S_GETREG_B32
 18 (0x12)  | S_GETREG_B32         | S_SETREG_B32        | S_SETREG_B32
 19 (0x13)  | S_SETREG_B32         | S_GETREG_REGRD_B32  | S_GETREG_REGRD_B32
 20 (0x14)  | S_GETREG_REGRD_B32   | S_SETREG_IMM32_B32  | S_SETREG_IMM32_B32
 21 (0x15)  | S_SETREG_IMM32_B32   | --                  | S_CALL_B64

### Instruction set

Alphabetically sorted instruction list:

#### S_ADDK_I32

Opcode: 15 (0xf) for GCN 1.0/1.1; 14 (0xe) for GCN 1.2/1.4  
Syntax: S_ADDK_I32 SDST, SIMM16  
Description: Add signed SDST to SIMM16 and store result into SDST and
store overflow flag into SCC.  
Operation:  
```
SDST = SDST + SIMM16
INT64 temp = SEXT64(SDST) + SEXT64(SIMM16)
SCC = temp > ((1LL<<31)-1) || temp < (-1LL<<31)
```

#### S_CALL_B64

Opcode: 21 (0x15) for GCN 1.4  
Syntax: S_CALL_B64 SDST(2), RELADDR  
Description: Call (short) a subroutine. Store address of next instruction to SDST and
go to RELADDR (store RELADDR into PC).  
Operation:  
```
SDST = PC + 4
PC = RELADDR
```

#### S_CBRANCH_I_FORK

Opcode: 17 (0x11) for GCN 1.0/1.1; 16 (0x10) for GCN 1.2/1.4  
Syntax: S_CBRANCH_I_FORK SSRC0(2), RELADDR  
Description: Fork control flow to passed and failed condition, jump to address RELADDR for
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
    PC = RELADDR
else if (failures == EXEC)
    PC += 4
else if (BITCOUNT(failures) < BITCOUNT(passes)) {
    EXEC = failures
    SGPR[CSP*4:CSP*4+1] = passes
    SGPR[CSP*4+2:CSP*4+3] = RELADDR
    CSP++
    PC += 4 /* jump to failure */
} else {
    EXEC = passes
    SGPR[CSP*4:CSP*4+1] = failures
    SGPR[CSP*4+2:CSP*4+3] = PC+4
    CSP++
    PC = RELADDR /* jump to passes */
}
```

#### S_CMOVK_I32

Opcode: 2 (0x2) for GCN 1.0/1.1; 1 (0x1) for GCN 1.2/1.4  
Syntax: S_MOVK_I32 SDST, SIMM16  
Description: If SCC is 1 then move signed extended 16-bit immediate into SDST.
SCC has not been changed.  
Operation:  
```
SDST = SCC ? SIMM16 : SDST
```

#### S_CMPK_EQ_I32

Opcode: 3 (0x3) for GCN1.0/1.1; 2 (0x2) for GCN 1.2/1.4  
Syntax: S_CMPK_EQ_I32 SDST, SIMM16  
Description: Compare signed value from SDST with SIMM16. If SDST equal, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SDST == SIMM16
```

#### S_CMPK_EQ_U32

Opcode: 9 (0x9) for GCN1.0/1.1; 8 (0x8) for GCN 1.2/1.4  
Syntax: S_CMPK_EQ_U32 SDST, IMM16  
Description: Compare unsigned value from SDST with IMM16. If SDST equal, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = SDST == IMM16
```

#### S_CMPK_GE_I32

Opcode: 6 (0x6) for GCN1.0/1.1; 5 (0x5) for GCN 1.2/1.4  
Syntax: S_CMPK_GE_I32 SDST, SIMM16  
Description: Compare signed value from SDST with SIMM16. If SDST greater or equal,
store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SDST >= SIMM16
```

#### S_CMPK_GE_U32

Opcode: 12 (0xc) for GCN1.0/1.1; 11 (0xb) for GCN 1.2/1.4  
Syntax: S_CMPK_GE_U32 SDST, IMM16  
Description: Compare unsigned value from SDST with IMM16. If SDST greater or equal,
store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = SDST >= IMM16
```

#### S_CMPK_GT_I32

Opcode: 5 (0x5) for GCN1.0/1.1; 4 (0x4) for GCN 1.2/1.4  
Syntax: S_CMPK_GT_I32 SDST, SIMM16  
Description: Compare signed value from SDST with SIMM16. If SDST greater, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SDST > SIMM16
```

#### S_CMPK_GT_U32

Opcode: 11 (0xb) for GCN1.0/1.1; 10 (0xa) for GCN 1.2/1.4  
Syntax: S_CMPK_GT_U32 SDST, IMM16  
Description: Compare unsigned value from SDST with IMM16. If SDST greater, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = SDST > IMM16
```

#### S_CMPK_LE_I32

Opcode: 8 (0x8) for GCN1.0/1.1; 7 (0x7) for GCN 1.2/1.4  
Syntax: S_CMPK_LE_I32 SDST, SIMM16  
Description: Compare signed value from SDST with SIMM16. If SDST less or equal,
store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SDST <= SIMM16
```

#### S_CMPK_LE_U32

Opcode: 14 (0xe) for GCN1.0/1.1; 13 (0xd) for GCN 1.2/1.4  
Syntax: S_CMPK_LE_U32 SDST, IMM16  
Description: Compare unsigned value from SDST with IMM16. If SDST less or equal,
store 1 to SCC, otherwise store 0 to SCC.  
Operation:  
```
SCC = SDST <= IMM16
```

#### S_CMPK_LG_I32

Opcode: 4 (0x4) for GCN1.0/1.1; 3 (0x3) for GCN 1.2/1.4  
Syntax: S_CMPK_LG_I32 SDST, SIMM16  
Description: Compare signed value from SDST with SIMM16. If SDST not equal, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SDST != SIMM16
```

#### S_CMPK_LG_U32

Opcode: 10 (0xa) for GCN1.0/1.1; 9 (0x9) for GCN 1.2/1.4  
Syntax: S_CMPK_LG_U32 SDST, IMM16  
Description: Compare unsigned value from SDST with IMM16. If SDST not equal, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = SDST != IMM16
```

#### S_CMPK_LT_I32

Opcode: 7 (0x7) for GCN1.0/1.1; 6 (0x6) for GCN 1.2/1.4  
Syntax: S_CMPK_LT_I32 SDST, SIMM16  
Description: Compare signed value from SDST with SIMM16. If SDST less, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = (INT32)SDST < SIMM16
```

#### S_CMPK_LT_U32

Opcode: 13 (0xd) for GCN1.0/1.1; 12 (0xc) for GCN 1.2/1.4  
Syntax: S_CMPK_LT_U32 SDST, IMM16  
Description: Compare unsigned value from SDST with IMM16. If SDST less, store 1 to SCC,
otherwise store 0 to SCC.  
Operation:  
```
SCC = SDST < IMM16
```

#### S_GETREG_B32

Opcode: 18 (0x12) for GCN1.0/1.1; 17 (0x11) for GCN 1.2/1.4  
Syntax: S_GETREG_B32 SDST, HWREG(HWREGNAME, BITOFFSET, BITSIZE)  
Description: Store hardware register part to SDST. BITOFFSET (0-31) is first bit in
hardware register, BITSIZE (1-32) is number of bits to extract.  
Operation:  
```
SDST = (HWREG >> BITOFFSET) & ((1U << BITSIZE) - 1U)
```

#### S_GETREG_REGRD_B32

Opcode: 20 (0x14) for GCN1.0/1.1; 19 (0x13) for GCN 1.2/1.4  
Syntax: S_GETREG_REGRD_B32 SDST, HWREG(HWREGNAME, BITOFFSET, BITSIZE)  
Description: ???  
Operation: ???  

#### S_MOVK_I32

Opcode: 0 (0x0)  
Syntax: S_MOVK_I32 SDST, SIMM16  
Description: Move signed extended 16-bit immediate into SDST. SCC has not been changed.  
Operation:  
```
SCC = SIMM16
```

#### S_MULK_I32

Opcode: 16 (0x10) for GCN 1.0/1.1; 15 (0xf) for GCN 1.2/1.4  
Syntax: S_MULK_I32 SDST, SIMM16  
Description: Multiply signed SDST with SIMM16 and store result into SDST.
SCC has not been changed.  
Operation:  
```
SDST = SDST * SIMM16
```

#### S_SETREG_B32

Opcode: 19 (0x13) for GCN1.0/1.1; 18 (0x12) for GCN 1.2/1.4  
Syntax: S_SETREG_B32 HWREG(HWREGNAME, BITOFFSET, BITSIZE), SDST  
Description: Store value from SDST to part of the hardware register.
BITOFFSET (0-31) is first bit in hardware register,
BITSIZE (1-32) is number of bits to store.  
Operation:  
```
UINT32 mask = ((1U<<BITSIZE) - 1U) << BITOFFSET
HWREG = (HWREG & ~mask) | ((SDST<<BITOFFSET) & mask)
```

#### S_SETREG_IMM32_B32

Opcode: 21 (0x15) for GCN1.0/1.1; 20 (0x14) for GCN 1.2/1.4  
Syntax: S_SETREG_B32 HWREG(HWREGNAME, BITOFFSET, BITSIZE), IMM32  
Description: Store value from IMM32 to part of the hardware register.
BITOFFSET (0-31) is first bit in hardware register,
BITSIZE (1-32) is number of bits to store. IMM32 is immediate 32-bit value after
instruction dword.  
Operation:  
```
UINT32 mask = ((1U<<BITSIZE) - 1U) << BITOFFSET
HWREG = (HWREG & ~mask) | ((IMM32<<BITOFFSET) & mask)
```
