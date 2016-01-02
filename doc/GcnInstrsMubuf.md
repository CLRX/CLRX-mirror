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
