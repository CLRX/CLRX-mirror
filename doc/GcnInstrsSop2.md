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

Syntax for all instructions: INSTRUCTION SDST, SRC0, SRC1

Example: s_and_b32 s0, s1, s2

### Instruction set

Alphabetically sorted instruction list:

#### S_ADDC_U32

Opcode: 4 (0x4)
Descrition: Add SRC0 to SRC1 with SCC value and store result into SDST and store
carry-out flag into SCC.  
Operation:  
```
temp = (UINT64)SRC0 + (UINT64)SRC1 + SCC
SDST = temp
SCC = temp>>32
```

#### S_ADD_I32

Opcode: 2 (0x2)
Description: Add SRC0 to SRC1 and store result into SDST and store overflow flag into SCC.  
Operation:  

```
SDST = SRC0 + SRC1
temp = (UINT64)SRC0 + (UINT64)SRC1
SCC = temp>((1LL<<31)-1) || temp>(-1LL<<31)
```

#### S_ADD_U32

Opcode: 0 (0x0)  
Description: Add SRC0 to SRC1 and store result into SDST and store carry-out into SCC.  
Operation:  
```
SDST = SRC0 + SRC1
SCC = ((UINT64)SRC0 + (UINT64)SRC1)>>32
```

#### S_MIN_I32

Opcode: 6 (0x6)
Description: Choose smallest signed value value from SRC0 and SRC1 and store its into SDST,
and store 1 to SCC if SSRC0 value has been choosen, otherwise store 0 to SCC  
Operation:  
```
SDST = (INT32)SSRC0<(INT32)SSRC1 ? SSRC0 : SSRC1
SCC = (INT32)SSRC0<(INT32)SSRC1
```

#### S_MIN_U32

Opcode: 6 (0x6)
Description: Choose smallest unsigned value value from SRC0 and SRC1 and store its into SDST,
and store 1 to SCC if SSRC0 value has been choosen, otherwise store 0 to SCC  
Operation:  
```
SDST = (UINT32)SSRC0<(UINT32)SSRC1 ? SSRC0 : SSRC1
SCC = (UINT32)SSRC0<(UINT32)SSRC1
```

#### S_SUBB_U32

Opcode: 5 (0x5)
Descrition: Subtract SRC0 to SRC1 with SCC value and store result into SDST and store
carry-out flag into SCC.  
Operation:  
```
temp = (UINT64)SRC0 - (UINT64)SRC1 - SCC
SDST = temp
SCC = temp>>32
```

#### S_SUB_I32

Opcode: 3 (0x3)
Description: Subtract SRC0 to SRC1 and store result into SDST and
store overflow flag into SCC. SCC register value can be BROKEN for some
architectures (GCN1.0)  
Operation:  
```
SDST = SRC0 - SRC1
temp = (UINT64)SRC0 - (UINT64)SRC1
SCC = temp>((1LL<<31)-1) || temp>(-1LL<<31)
```

#### S_SUB_U32

Opcode: 1 (0x1)  
Description: Subtract SRC0 to SRC1 and store result into SDST and store borrow into SCC.  
Operation:  
```
SDST = SRC0 - SRC1
SCC = ((INT64)SRC0 - (INT64)SRC1)>>32
```
