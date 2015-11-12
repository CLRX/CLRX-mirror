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

List of the instruction by opcode:

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
SCC = temp>((1LL<<31)-1) || temp>(-1LL<<31)
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

#### S_MIN_I32

Opcode: 6 (0x6)
Syntax: S_MIN_I32 SDST, SSRC0, SSRC1  
Description: Choose smallest signed value value from SSRC0 and SSRC1 and store its into SDST,
and store 1 to SCC if SSSRC0 value has been choosen, otherwise store 0 to SCC  
Operation:  
```
SDST = (INT32)SSSRC0<(INT32)SSSRC1 ? SSSRC0 : SSSRC1
SCC = (INT32)SSSRC0<(INT32)SSSRC1
```

#### S_MIN_U32

Opcode: 7 (0x7)  
Syntax: S_MIN_U32 SDST, SSRC0, SSRC1  
Description: Choose smallest unsigned value value from SSRC0 and SSRC1 and store its into SDST,
and store 1 to SCC if SSSRC0 value has been choosen, otherwise store 0 to SCC  
Operation:  
```
SDST = (UINT32)SSSRC0<(UINT32)SSSRC1 ? SSSRC0 : SSSRC1
SCC = (UINT32)SSSRC0<(UINT32)SSSRC1
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
