## GCN ISA MUBUF instructions

These instructions allow to access to main memory. MUBUF instructions
operates on the buffer resources. The buffer resources are 4 dwords which holds the
base address, buffer size, their structure and format of their data.

List of fields for the MUBUF encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-11  | OFFSET   | Unsigned byte offset
12    | OFFEN    | If set, send additional offset from VADDR
13    | IDXEN    | If set, send index from VADDR
14    | GLC      | Operation globally coherent
15    | ADDR64   | If set, address is 64-bit (VADDR is 64-bit)
16    | LDS      | Data is read from or written to LDS, otherwise from VGPR
18-24 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b111000
32-39 | VADDR    | Vector address registers
40-47 | VDATA    | Vector data register
48-52 | SRSRC    | Scalar registers with buffer resource (SGPR# is 4*value)
54    | SLC      | System level coherent
55    | TFE      | Texture Fail Enable ???
56-63 | SOFFSET  | Scalar base offset operand

List of fields for the MUBUF encoding (GCN 1.2):

Bits  | Name     | Description
------|----------|------------------------------
0-11  | OFFSET   | Unsigned byte offset
12    | OFFEN    | If set, send additional offset from VADDR
13    | IDXEN    | If set, send index from VADDR
14    | GLC      | Operation globally coherent
16    | LDS      | Data is read from or written to LDS, otherwise from VGPR
17    | SLC      | System level coherent
18-24 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b111000
32-39 | VADDR    | Vector address registers
40-47 | VDATA    | Vector data register
48-52 | SRSRC    | Scalar registers with buffer resource (SGPR# is 4*value)
55    | TFE      | Texture Fail Enable ???
56-63 | SOFFSET  | Scalar base offset operand

Instruction syntax: INSTRUCTION VDATA, VADDR, SRSRC, SOFFSET [MODIFIERS]

Modifiers can be supplied in any order. Modifiers list:
OFFEN, IDXEN, SLC, GLC, TFE, ADDR64, LDS, OFFSET:OFFSET.
The TFE flag requires additional the VDATA register. IDXEN and OFFEN both enabled
requires 64-bit VADDR.

### Instructions by opcode

List of the MUBUF instructions by opcode (GCN 1.0/1.1):

 Opcode     |GCN 1.0|GCN 1.1| Mnemonic
------------|-------|-------|------------------------------
 0 (0x0)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_X
 1 (0x1)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_XY
 2 (0x2)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_XYZ
 3 (0x3)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_XYZW
 4 (0x4)    |   ✓   |   ✓   | BUFFER_STORE_FORMAT_X
 5 (0x5)    |   ✓   |   ✓   | BUFFER_STORE_FORMAT_XY
 6 (0x6)    |   ✓   |   ✓   | BUFFER_STORE_FORMAT_XYZ
 7 (0x7)    |   ✓   |   ✓   | BUFFER_STORE_FORMAT_XYZW
 8 (0x8)    |   ✓   |   ✓   | BUFFER_LOAD_UBYTE
 9 (0x9)    |   ✓   |   ✓   | BUFFER_LOAD_SBYTE
 10 (0xa)   |   ✓   |   ✓   | BUFFER_LOAD_USHORT
 11 (0xb)   |   ✓   |   ✓   | BUFFER_LOAD_SSHORT
 12 (0xc)   |   ✓   |   ✓   | BUFFER_LOAD_DWORD
 13 (0xd)   |   ✓   |   ✓   | BUFFER_LOAD_DWORDX2
 14 (0xe)   |   ✓   |   ✓   | BUFFER_LOAD_DWORDX4
 15 (0xf)   |       |   ✓   | BUFFER_LOAD_DWORDX3
 24 (0x18)  |   ✓   |   ✓   | BUFFER_STORE_BYTE
 26 (0x1a)  |   ✓   |   ✓   | BUFFER_STORE_SHORT
 28 (0x1c)  |   ✓   |   ✓   | BUFFER_STORE_DWORD
 29 (0x1d)  |   ✓   |   ✓   | BUFFER_STORE_DWORDX2
 30 (0x1e)  |   ✓   |   ✓   | BUFFER_STORE_DWORDX4
 31 (0x1f)  |       |   ✓   | BUFFER_STORE_DWORDX3
 48 (0x30)  |   ✓   |   ✓   | BUFFER_ATOMIC_SWAP
 49 (0x31)  |   ✓   |   ✓   | BUFFER_ATOMIC_CMPSWAP
 50 (0x32)  |   ✓   |   ✓   | BUFFER_ATOMIC_ADD
 51 (0x33)  |   ✓   |   ✓   | BUFFER_ATOMIC_SUB
 52 (0x34)  |   ✓   |       | BUFFER_ATOMIC_RSUB
 53 (0x35)  |   ✓   |   ✓   | BUFFER_ATOMIC_SMIN
 54 (0x36)  |   ✓   |   ✓   | BUFFER_ATOMIC_UMIN
 55 (0x37)  |   ✓   |   ✓   | BUFFER_ATOMIC_SMAX
 56 (0x38)  |   ✓   |   ✓   | BUFFER_ATOMIC_UMAX
 57 (0x39)  |   ✓   |   ✓   | BUFFER_ATOMIC_AND
 58 (0x3a)  |   ✓   |   ✓   | BUFFER_ATOMIC_OR
 59 (0x3b)  |   ✓   |   ✓   | BUFFER_ATOMIC_XOR
 60 (0x3c)  |   ✓   |   ✓   | BUFFER_ATOMIC_INC
 61 (0x3d)  |   ✓   |   ✓   | BUFFER_ATOMIC_DEC
 62 (0x3e)  |   ✓   |   ✓   | BUFFER_ATOMIC_FCMPSWAP
 63 (0x3f)  |   ✓   |   ✓   | BUFFER_ATOMIC_FMIN
 64 (0x40)  |   ✓   |   ✓   | BUFFER_ATOMIC_FMAX
 80 (0x50)  |   ✓   |   ✓   | BUFFER_ATOMIC_SWAP_X2
 81 (0x51)  |   ✓   |   ✓   | BUFFER_ATOMIC_CMPSWAP_X2
 82 (0x52)  |   ✓   |   ✓   | BUFFER_ATOMIC_ADD_X2
 83 (0x53)  |   ✓   |   ✓   | BUFFER_ATOMIC_SUB_X2
 84 (0x54)  |   ✓   |       | BUFFER_ATOMIC_RSUB_X2
 85 (0x55)  |   ✓   |   ✓   | BUFFER_ATOMIC_SMIN_X2
 86 (0x56)  |   ✓   |   ✓   | BUFFER_ATOMIC_UMIN_X2
 87 (0x57)  |   ✓   |   ✓   | BUFFER_ATOMIC_SMAX_X2
 88 (0x58)  |   ✓   |   ✓   | BUFFER_ATOMIC_UMAX_X2
 89 (0x59)  |   ✓   |   ✓   | BUFFER_ATOMIC_AND_X2
 90 (0x5a)  |   ✓   |   ✓   | BUFFER_ATOMIC_OR_X2
 91 (0x5b)  |   ✓   |   ✓   | BUFFER_ATOMIC_XOR_X2
 92 (0x5c)  |   ✓   |   ✓   | BUFFER_ATOMIC_INC_X2
 93 (0x5d)  |   ✓   |   ✓   | BUFFER_ATOMIC_DEC_X2
 94 (0x5e)  |   ✓   |   ✓   | BUFFER_ATOMIC_FCMPSWAP_X2
 95 (0x5f)  |   ✓   |   ✓   | BUFFER_ATOMIC_FMIN_X2
 96 (0x60)  |   ✓   |   ✓   | BUFFER_ATOMIC_FMAX_X2
 112 (0x70) |   ✓   |   ✓   | BUFFER_WBINVL1_SC
 113 (0x71) |   ✓   |   ✓   | BUFFER_WBINVL1

List of the MUBUF instructions by opcode (GCN 1.0/1.1):

 Opcode     | Mnemonic
------------|------------------------------
 0 (0x0)    | BUFFER_LOAD_FORMAT_X
 1 (0x1)    | BUFFER_LOAD_FORMAT_XY
 2 (0x2)    | BUFFER_LOAD_FORMAT_XYZ
 3 (0x3)    | BUFFER_LOAD_FORMAT_XYZW
 4 (0x4)    | BUFFER_STORE_FORMAT_X
 5 (0x5)    | BUFFER_STORE_FORMAT_XY
 6 (0x6)    | BUFFER_STORE_FORMAT_XYZ
 7 (0x7)    | BUFFER_STORE_FORMAT_XYZW
 8 (0x8)    | BUFFER_LOAD_FORMAT_D16_X
 9 (0x9)    | BUFFER_LOAD_FORMAT_D16_XY
 10 (0xa)   | BUFFER_LOAD_FORMAT_D16_XYZ
 11 (0xb)   | BUFFER_LOAD_FORMAT_D16_XYZW
 12 (0xc)   | BUFFER_STORE_FORMAT_D16_X
 13 (0xd)   | BUFFER_STORE_FORMAT_D16_XY
 14 (0xe)   | BUFFER_STORE_FORMAT_D16_XYZ
 15 (0xf)   | BUFFER_STORE_FORMAT_D16_XYZW
 16 (0x10)  | BUFFER_LOAD_UBYTE
 17 (0x11)  | BUFFER_LOAD_SBYTE
 18 (0x12)  | BUFFER_LOAD_USHORT
 19 (0x13)  | BUFFER_LOAD_SSHORT
 20 (0x14)  | BUFFER_LOAD_DWORD
 21 (0x15)  | BUFFER_LOAD_DWORDX2
 22 (0x16)  | BUFFER_LOAD_DWORDX3
 23 (0x17)  | BUFFER_LOAD_DWORDX4
 24 (0x18)  | BUFFER_STORE_BYTE
 26 (0x1a)  | BUFFER_STORE_SHORT
 28 (0x1c)  | BUFFER_STORE_DWORD
 29 (0x1d)  | BUFFER_STORE_DWORDX2
 30 (0x1e)  | BUFFER_STORE_DWORDX3
 31 (0x1f)  | BUFFER_STORE_DWORDX4
 61 (0x3d)  | BUFFER_STORE_LDS_DWORD
 62 (0x3e)  | BUFFER_WBINVL1
 63 (0x3f)  | BUFFER_WBINVL1_VOL
 64 (0x40)  | BUFFER_ATOMIC_SWAP
 65 (0x41)  | BUFFER_ATOMIC_CMPSWAP
 66 (0x42)  | BUFFER_ATOMIC_ADD
 67 (0x43)  | BUFFER_ATOMIC_SUB
 68 (0x44)  | BUFFER_ATOMIC_SMIN
 69 (0x45)  | BUFFER_ATOMIC_UMIN
 70 (0x46)  | BUFFER_ATOMIC_SMAX
 71 (0x47)  | BUFFER_ATOMIC_UMAX
 72 (0x48)  | BUFFER_ATOMIC_AND
 73 (0x49)  | BUFFER_ATOMIC_OR
 74 (0x4a)  | BUFFER_ATOMIC_XOR
 75 (0x4b)  | BUFFER_ATOMIC_INC
 76 (0x4c)  | BUFFER_ATOMIC_DEC
 96 (0x60)  | BUFFER_ATOMIC_SWAP_X2
 97 (0x61)  | BUFFER_ATOMIC_CMPSWAP_X2
 98 (0x62)  | BUFFER_ATOMIC_ADD_X2
 99 (0x63)  | BUFFER_ATOMIC_SUB_X2
 100 (0x64) | BUFFER_ATOMIC_SMIN_X2
 101 (0x65) | BUFFER_ATOMIC_UMIN_X2
 102 (0x66) | BUFFER_ATOMIC_SMAX_X2
 103 (0x67) | BUFFER_ATOMIC_UMAX_X2
 104 (0x68) | BUFFER_ATOMIC_AND_X2
 105 (0x69) | BUFFER_ATOMIC_OR_X2
 106 (0x6a) | BUFFER_ATOMIC_XOR_X2
 107 (0x6b) | BUFFER_ATOMIC_INC_X2
 108 (0x6c) | BUFFER_ATOMIC_DEC_X2

### Details

Informations about addressing and format conversion here:
[Main memory handling](GcnMemHandling)
 
### Instruction set

Alphabetically sorted instruction list:

#### BUFFER_ATOMIC_ADD

Opcode: 50 (0x32) for GCN 1.0/1.1; 66 (0x42) for GCN 1.2  
Syntax: BUFFER_ATOMIC_ADD VDATA, VADDR, SRSRC, SOFFSET  
Description: Add VDATA to value of SRSRC resource, and store result to this resource.
If GLC flag is set then return previous value to VDATA, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = *VM + VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_CMPSWAP

Opcode: 49 (0x31) for GCN 1.0/1.1; 65 (0x41) for GCN 1.2  
Syntax: BUFFER_ATOMIC_CMPSWAP VDATA(2), VADDR, SRSRC, SOFFSET  
Description: Store lower VDATA dword into SRSRC resource if previous value
from resources is equal VDATA>>32, otherwise keep old value from resource.
If GLC flag is set then return previous value to VDATA, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = *VM==(VDATA>>32) ? VDATA&0xffffffff : *VM // part of atomic
VDATA = (GLC) ? P : VDATA // last part of atomic
```

#### BUFFER_ATOMIC_RSUB

Opcode: 52 (0x34) for GCN 1.0
Syntax: BUFFER_ATOMIC_RSUB VDATA, VADDR, SRSRC, SOFFSET  
Description: Subtract value of SRSRC resource from VDATA, and store result to
this resource. If GLC flag is set then return previous value to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = VDATA - *VM; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SMAX

Opcode: 55 (0x37) for GCN 1.0/1.1; 70 (0x46) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SMAX VDATA, VADDR, SRSRC, SOFFSET  
Description: Choose greatest signed 32-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value to VDATA, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = MAX(*VM, (INT32)VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SMIN

Opcode: 53 (0x35) for GCN 1.0/1.1; 68 (0x44) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SMIN VDATA, VADDR, SRSRC, SOFFSET  
Description: Choose smallest signed 32-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value to VDATA, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = MIN(*VM, (INT32)VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SUB

Opcode: 51 (0x33) for GCN 1.0/1.1; 67 (0x43) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SUB VDATA, VADDR, SRSRC, SOFFSET  
Description: Subtract VDATA from value from SRSRC resource, and store result to
this resource. If GLC flag is set then return previous value to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = *VM - VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SWAP

Opcode: 48 (0x30) for GCN 1.0/1.1; 64 (0x40) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SWAP VDATA, VADDR, SRSRC, SOFFSET  
Description: Store VDATA dword into SRSRC resource. If GLC flag is set then
return previous value to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_UMAX

Opcode: 56 (0x38) for GCN 1.0/1.1; 71 (0x47) for GCN 1.2  
Syntax: BUFFER_ATOMIC_UMAX VDATA, VADDR, SRSRC, SOFFSET  
Description: Choose greatest unsigned 32-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value to VDATA, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = MAX(*VM, (INT32)VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_UMIN

Opcode: 54 (0x36) for GCN 1.0/1.1; 69 (0x45) for GCN 1.2  
Syntax: BUFFER_ATOMIC_UMIN VDATA, VADDR, SRSRC, SOFFSET  
Description: Choose smallest unsigned 32-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value to VDATA, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
UINT32* P = *VM; *VM = MIN(*VM, (INT32)VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_LOAD_DWORD

Opcode: 12 (0xc) for GCN 1.0/1.1; 20 (0x14) for GCN 1.2  
Syntax: BUFFER_LOAD_DWORD VDATA, VADDR, SRSRC, SOFFSET  
Description: Load dword to VDATA from SRSRC resource.  
Operation:  
```
VDATA = *(UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_LOAD_DWORDX2

Opcode: 13 (0xd) for GCN 1.0/1.1; 21 (0x15) for GCN 1.2  
Syntax: BUFFER_LOAD_DWORDX2 VDATA(2), VADDR, SRSRC, SOFFSET  
Description: Load two dwords to VDATA from SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
VDATA[0] = VM[0]
VDATA[1] = VM[1]
```

#### BUFFER_LOAD_DWORDX3

Opcode: 15 (0xf) for GCN 1.1; 22 (0x16) for GCN 1.2  
Syntax: BUFFER_LOAD_DWORDX3 VDATA(3), VADDR, SRSRC, SOFFSET  
Description: Load three dwords to VDATA from SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
VDATA[0] = VM[0]
VDATA[1] = VM[1]
VDATA[2] = VM[2]
```

#### BUFFER_LOAD_DWORDX4

Opcode: 14 (0xe) for GCN 1.0/1.1; 23 (0x17) for GCN 1.2  
Syntax: BUFFER_LOAD_DWORDX4 VDATA(4), VADDR, SRSRC, SOFFSET  
Description: Load four dwords to VDATA from SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
VDATA[0] = VM[0]
VDATA[1] = VM[1]
VDATA[2] = VM[2]
VDATA[3] = VM[3]
```

#### BUFFER_LOAD_FORMAT_X

Opcode: 0 (0x0)  
Syntax: BUFFER_LOAD_FORMAT_X VDATA, VADDR, SRSRC, SOFFSET  
Description: Load the first component of the element from SRSRC including format from
buffer resource.  
Operation:  
```
VDATA = LOAD_FORMAT_X(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_XY

Opcode: 1 (0x1)  
Syntax: BUFFER_LOAD_FORMAT_XY VDATA(2), VADDR, SRSRC, SOFFSET  
Description: Load the first two components of the element from SRSRC resource
including format from SRSRC.  
Operation:  
```
VDATA[0] = LOAD_FORMAT_XY(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_XYZ

Opcode: 2 (0x2)  
Syntax: BUFFER_LOAD_FORMAT_XYZ VDATA(3), VADDR, SRSRC, SOFFSET  
Description: Load the first three components of the element from SRSRC resource
including format from SRSRC.  
Operation:  
```
VDATA[0] = LOAD_FORMAT_XYZ(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_XYZW

Opcode: 3 (0x3)  
Syntax: BUFFER_LOAD_FORMAT_XYZW VDATA(4), VADDR, SRSRC, SOFFSET  
Description: Load the all four components of the element from SRSRC resource 
including format from SRSRC.  
Operation:  
```
VDATA[0] = LOAD_FORMAT_XYZW(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_LOAD_SBYTE

Opcode: 9 (0x9) for GCN 1.0/1.1; 17 (0x11) for GCN 1.2  
Syntax: BUFFER_LOAD_SBYTE VDATA, VADDR, SRSRC, SOFFSET  
Description: Load byte to VDATA from SRSRC resource with sign extending.  
Operation:  
```
VDATA = *(INT8*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_LOAD_SSHORT

Opcode: 11 (0xb) for GCN 1.0/1.1; 19 (0x13) for GCN 1.2  
Syntax: BUFFER_LOAD_SSHORT VDATA, VADDR, SRSRC, SOFFSET  
Description: Load 16-bit word to VDATA from SRSRC resource with sign extending.  
Operation:  
```
VDATA = *(INT16*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_LOAD_UBYTE

Opcode: 8 (0x8) for GCN 1.0/1.1; 16 (0x10) for GCN 1.2  
Syntax: BUFFER_LOAD_UBYTE VDATA, VADDR, SRSRC, SOFFSET  
Description: Load byte to VDATA from SRSRC resource with zero extending.  
Operation:  
```
VDATA = *(UINT8*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_LOAD_USHORT

Opcode: 10 (0xa) for GCN 1.0/1.1; 18 (0x12) for GCN 1.2  
Syntax: BUFFER_LOAD_USHORT VDATA, VADDR, SRSRC, SOFFSET  
Description: Load 16-bit word to VDATA from SRSRC resource with zero extending.  
Operation:  
```
VDATA = *(UINT16*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
```

#### BUFFER_STORE_BYTE

Opcode: 24 (0x18)  
Syntax: BUFFER_STORE_BYTE VDATA, VADDR, SRSRC, SOFFSET  
Description: Store byte from VDATA into SRSRC resource.  
Operation:  
```
*(UINT8*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET) = VDATA&0xff
```

#### BUFFER_STORE_DWORD

Opcode: 28 (0x1c)  
Syntax: BUFFER_STORE_DWORD VDATA, VADDR, SRSRC, SOFFSET  
Description: Store dword from VDATA into SRSRC resource.  
Operation:  
```
*(UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET) = VDATA
```

#### BUFFER_STORE_DWORDX2

Opcode: 29 (0x1d)  
Syntax: BUFFER_STORE_DWORDX2 VDATA(2), VADDR, SRSRC, SOFFSET  
Description: Store two dwords from VDATA into SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
VM[0] = VDATA[0]
VM[1] = VDATA[1]
```

#### BUFFER_STORE_DWORDX3

Opcode: 31 (0x1f) for GCN 1.1; 30 (0x1e) for GCN 1.2  
Syntax: BUFFER_STORE_DWORDX2 VDATA(3), VADDR, SRSRC, SOFFSET  
Description: Store three dwords from VDATA into SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
VM[0] = VDATA[0]
VM[1] = VDATA[1]
VM[2] = VDATA[2]
```

#### BUFFER_STORE_DWORDX4

Opcode: 31 (0x1e) for GCN 1.1; 31 (0x1f) for GCN 1.2  
Syntax: BUFFER_STORE_DWORDX2 VDATA(4), VADDR, SRSRC, SOFFSET  
Description: Store four dwords from VDATA into SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET)
VM[0] = VDATA[0]
VM[1] = VDATA[1]
VM[2] = VDATA[2]
VM[3] = VDATA[3]
```

#### BUFFER_STORE_FORMAT_X

Opcode: 4 (0x4)  
Syntax: BUFFER_STORE_FORMAT_X VDATA, VADDR, SRSRC, SOFFSET  
Description: Store the first component of the element into SRSRC resource
including format from SRSRC.  
Operation:  
```
STORE_FORMAT_X(SRSRC, VADDR, SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_XY

Opcode: 5 (0x5)  
Syntax: BUFFER_STORE_FORMAT_XY VDATA(2), VADDR, SRSRC, SOFFSET  
Description: Store the first two components of the element into SRSRC resource
including format from SRSRC.  
Operation:  
```
STORE_FORMAT_XY(SRSRC, VADDR, SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_XYZ

Opcode: 6 (0x6)  
Syntax: BUFFER_STORE_FORMAT_XYZ VDATA(3), VADDR, SRSRC, SOFFSET  
Description: Store the first three components of the element into SRSRC resource
including format from SRSRC.  
Operation:  
```
STORE_FORMAT_XYZ(SRSRC, VADDR, SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_XYZW

Opcode: 7 (0x7)  
Syntax: BUFFER_STORE_FORMAT_XYZW VDATA(4), VADDR, SRSRC, SOFFSET  
Description: Store the all components of the element into SRSRC resource
including format from SRSRC.  
Operation:  
```
STORE_FORMAT_XYZW(SRSRC, VADDR, SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_SHORT

Opcode: 26 (0x1a)  
Syntax: BUFFER_STORE_SHORT VDATA, VADDR, SRSRC, SOFFSET  
Description: Store 16-bit word from VDATA into SRSRC resource.  
Operation:  
```
*(UINT16*)VMEM(SRSRC, VADDR, SOFFSET, OFFSET) = VDATA&0xffff
```
