## GCN ISA MUBUF instructions

These instructions allow to access to main memory. MUBUF instructions
operates on the buffer resources. The buffer resources are 4 dwords which holds the
base address, buffer size, their structure and format of their data.
These instructions are untyped, and they get number/data format from an resource
or that format are determined by operation (data format is not encoded in
instruction's format). 

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

List of fields for the MUBUF encoding (GCN 1.2/1.4):

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

Instruction syntax: INSTRUCTION VDATA, [VADDR(1:2),] SRSRC(4), SOFFSET [MODIFIERS]

Modifiers can be supplied in any order. Modifiers list:
OFFEN, IDXEN, SLC, GLC, TFE, ADDR64, LDS, OFFSET:OFFSET.
The TFE flag requires additional the VDATA register. IDXEN and OFFEN both enabled
requires 64-bit VADDR. VADDR is optional if no IDXEN, OFFEN and ADDR64.

The MUBUF instructions is executed in order. Any MUBUF instruction increments VMCNT and
it decrements VMCNT after memory operation. Any memory-write operation increments EXPCNT,
and it decrements EXPCNT after reading data from VDATA.

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

List of the MUBUF instructions by opcode (GCN 1.2/1.4):

 Opcode     |GCN 1.2|GCN 1.4| Mnemonic
------------|-------|-------|------------------------------
 0 (0x0)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_X
 1 (0x1)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_XY
 2 (0x2)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_XYZ
 3 (0x3)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_XYZW
 4 (0x4)    |   ✓   |   ✓   | BUFFER_STORE_FORMAT_X
 5 (0x5)    |   ✓   |   ✓   | BUFFER_STORE_FORMAT_XY
 6 (0x6)    |   ✓   |   ✓   | BUFFER_STORE_FORMAT_XYZ
 7 (0x7)    |   ✓   |   ✓   | BUFFER_STORE_FORMAT_XYZW
 8 (0x8)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_D16_X
 9 (0x9)    |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_D16_XY
 10 (0xa)   |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_D16_XYZ
 11 (0xb)   |   ✓   |   ✓   | BUFFER_LOAD_FORMAT_D16_XYZW
 12 (0xc)   |   ✓   |   ✓   | BUFFER_STORE_FORMAT_D16_X
 13 (0xd)   |   ✓   |   ✓   | BUFFER_STORE_FORMAT_D16_XY
 14 (0xe)   |   ✓   |   ✓   | BUFFER_STORE_FORMAT_D16_XYZ
 15 (0xf)   |   ✓   |   ✓   | BUFFER_STORE_FORMAT_D16_XYZW
 16 (0x10)  |   ✓   |   ✓   | BUFFER_LOAD_UBYTE
 17 (0x11)  |   ✓   |   ✓   | BUFFER_LOAD_SBYTE
 18 (0x12)  |   ✓   |   ✓   | BUFFER_LOAD_USHORT
 19 (0x13)  |   ✓   |   ✓   | BUFFER_LOAD_SSHORT
 20 (0x14)  |   ✓   |   ✓   | BUFFER_LOAD_DWORD
 21 (0x15)  |   ✓   |   ✓   | BUFFER_LOAD_DWORDX2
 22 (0x16)  |   ✓   |   ✓   | BUFFER_LOAD_DWORDX3
 23 (0x17)  |   ✓   |   ✓   | BUFFER_LOAD_DWORDX4
 24 (0x18)  |   ✓   |   ✓   | BUFFER_STORE_BYTE
 25 (0x19)  |       |   ✓   | BUFFER_STORE_BYTE_D16
 26 (0x1a)  |   ✓   |   ✓   | BUFFER_STORE_SHORT
 27 (0x1b)  |       |   ✓   | BUFFER_STORE_SHORT_D16
 28 (0x1c)  |   ✓   |   ✓   | BUFFER_STORE_DWORD
 29 (0x1d)  |   ✓   |   ✓   | BUFFER_STORE_DWORDX2
 30 (0x1e)  |   ✓   |   ✓   | BUFFER_STORE_DWORDX3
 31 (0x1f)  |   ✓   |   ✓   | BUFFER_STORE_DWORDX4
 32 (0x20)  |       |   ✓   | BUFFER_LOAD_UBYTE_D16
 33 (0x21)  |       |   ✓   | BUFFER_LOAD_UBYTE_D16_HI
 34 (0x22)  |       |   ✓   | BUFFER_LOAD_SBYTE_D16
 35 (0x23)  |       |   ✓   | BUFFER_LOAD_SBYTE_D16_HI
 36 (0x24)  |       |   ✓   | BUFFER_LOAD_SHORT_D16
 37 (0x25)  |       |   ✓   | BUFFER_LOAD_SHORT_D16_HI
 38 (0x26)  |       |   ✓   | BUFFER_LOAD_FORMAT_D16_HI_X
 39 (0x27)  |       |   ✓   | BUFFER_STORE_FORMAT_D16_HI_X
 61 (0x3d)  |   ✓   |   ✓   | BUFFER_STORE_LDS_DWORD
 62 (0x3e)  |   ✓   |   ✓   | BUFFER_WBINVL1
 63 (0x3f)  |   ✓   |   ✓   | BUFFER_WBINVL1_VOL
 64 (0x40)  |   ✓   |   ✓   | BUFFER_ATOMIC_SWAP
 65 (0x41)  |   ✓   |   ✓   | BUFFER_ATOMIC_CMPSWAP
 66 (0x42)  |   ✓   |   ✓   | BUFFER_ATOMIC_ADD
 67 (0x43)  |   ✓   |   ✓   | BUFFER_ATOMIC_SUB
 68 (0x44)  |   ✓   |   ✓   | BUFFER_ATOMIC_SMIN
 69 (0x45)  |   ✓   |   ✓   | BUFFER_ATOMIC_UMIN
 70 (0x46)  |   ✓   |   ✓   | BUFFER_ATOMIC_SMAX
 71 (0x47)  |   ✓   |   ✓   | BUFFER_ATOMIC_UMAX
 72 (0x48)  |   ✓   |   ✓   | BUFFER_ATOMIC_AND
 73 (0x49)  |   ✓   |   ✓   | BUFFER_ATOMIC_OR
 74 (0x4a)  |   ✓   |   ✓   | BUFFER_ATOMIC_XOR
 75 (0x4b)  |   ✓   |   ✓   | BUFFER_ATOMIC_INC
 76 (0x4c)  |   ✓   |   ✓   | BUFFER_ATOMIC_DEC
 96 (0x60)  |   ✓   |   ✓   | BUFFER_ATOMIC_SWAP_X2
 97 (0x61)  |   ✓   |   ✓   | BUFFER_ATOMIC_CMPSWAP_X2
 98 (0x62)  |   ✓   |   ✓   | BUFFER_ATOMIC_ADD_X2
 99 (0x63)  |   ✓   |   ✓   | BUFFER_ATOMIC_SUB_X2
 100 (0x64) |   ✓   |   ✓   | BUFFER_ATOMIC_SMIN_X2
 101 (0x65) |   ✓   |   ✓   | BUFFER_ATOMIC_UMIN_X2
 102 (0x66) |   ✓   |   ✓   | BUFFER_ATOMIC_SMAX_X2
 103 (0x67) |   ✓   |   ✓   | BUFFER_ATOMIC_UMAX_X2
 104 (0x68) |   ✓   |   ✓   | BUFFER_ATOMIC_AND_X2
 105 (0x69) |   ✓   |   ✓   | BUFFER_ATOMIC_OR_X2
 106 (0x6a) |   ✓   |   ✓   | BUFFER_ATOMIC_XOR_X2
 107 (0x6b) |   ✓   |   ✓   | BUFFER_ATOMIC_INC_X2
 108 (0x6c) |   ✓   |   ✓   | BUFFER_ATOMIC_DEC_X2

### Details

Informations about addressing and format conversion are here:
[Main memory handling](GcnMemHandling)
 
### Instruction set

Alphabetically sorted instruction list:

#### BUFFER_ATOMIC_ADD

Opcode: 50 (0x32) for GCN 1.0/1.1; 66 (0x42) for GCN 1.2  
Syntax: BUFFER_ATOMIC_ADD VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Add VDATA to value of SRSRC resource, and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = *VM + VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_ADD_X2

Opcode: 82 (0x52) for GCN 1.0/1.1; 98 (0x62) for GCN 1.2  
Syntax: BUFFER_ATOMIC_ADD_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Add 64-bit VDATA to 64-bit value of SRSRC resource, and store result
to this resource. If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = *VM + VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_AND

Opcode: 57 (0x39) for GCN 1.0/1.1; 72 (0x48) for GCN 1.2  
Syntax: BUFFER_ATOMIC_AND VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Do bitwise AND on VDATA and value of SRSRC resource,
and store result to this resource. If GLC flag is set then return previous value
from resource to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = *VM & VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_AND_X2

Opcode: 89 (0x59) for GCN 1.0/1.1; 104 (0x68) for GCN 1.2  
Syntax: BUFFER_ATOMIC_AND_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Do 64-bit bitwise AND on VDATA and value of SRSRC resource,
and store result to this resource. If GLC flag is set then return previous value
from resource to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = *VM & VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_CMPSWAP

Opcode: 49 (0x31) for GCN 1.0/1.1; 65 (0x41) for GCN 1.2  
Syntax: BUFFER_ATOMIC_CMPSWAP VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store lower VDATA dword into SRSRC resource if previous value
from resource is equal VDATA>>32, otherwise keep old value from resource.
If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = *VM==(VDATA>>32) ? VDATA&0xffffffff : *VM // part of atomic
VDATA[0] = (GLC) ? P : VDATA[0] // last part of atomic
```

#### BUFFER_ATOMIC_CMPSWAP_X2

Opcode: 81 (0x51) for GCN 1.0/1.1; 97 (0x61) for GCN 1.2  
Syntax: BUFFER_ATOMIC_CMPSWAP_X2 VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store lower VDATA 64-bit word into SRSRC resource if previous value
from resource is equal VDATA>>64, otherwise keep old value from resource.
If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = *VM==(VDATA[2:3]) ? VDATA[0:1] : *VM // part of atomic
VDATA[0:1] = (GLC) ? P : VDATA[0:1] // last part of atomic
```

#### BUFFER_ATOMIC_DEC

Opcode: 61 (0x3d) for GCN 1.0/1.1; 76 (0x4c) for GCN 1.2  
Syntax: BUFFER_ATOMIC_DEC VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Compare value from SRSRC resource and if less or equal than VDATA
and this value is not zero, then decrement value from resource,
otherwise store VDATA to resource. If GLC flag is set then return previous value
from resource to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA // atomic
VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_DEC_X2

Opcode: 93 (0x5d) for GCN 1.0/1.1; 108 (0x6c) for GCN 1.2  
Syntax: BUFFER_ATOMIC_DEC_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Compare 64-bit value from SRSRC resource and if less or equal than VDATA
and this value is not zero, then decrement value from resource,
otherwise store VDATA to resource. If GLC flag is set then return previous value
from resource to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA // atomic
VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_FCMPSWAP

Opcode: 62 (0x3e) for GCN 1.0/1.1  
Syntax: BUFFER_ATOMIC_FCMPSWAP VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store lower VDATA dword into SRSRC resource if previous single floating point
value from resource is equal singe floating point value VDATA>>32,
otherwise keep old value from resource.
If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
FLOAT* VM = (FLOAT*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
FLOAT P = *VM; *VM = *VM==ASFLOAT(VDATA>>32) ? VDATA&0xffffffff : *VM // part of atomic
VDATA[0] = (GLC) ? P : VDATA[0] // last part of atomic
```

#### BUFFER_ATOMIC_FCMPSWAP_X2

Opcode: 94 (0x5e) for GCN 1.0/1.1  
Syntax: BUFFER_ATOMIC_FCMPSWAP_X2 VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store lower VDATA 64-bit word into SRSRC resource if previous double
floating point value from resource is equal singe floating point value VDATA>>32,
otherwise keep old value from resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
DOUBLE* VM = (DOUBLE*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
DOUBLE P = *VM; *VM = *VM==ASDOUBLE(VDATA[2:3]) ? VDATA[0:1] : *VM // part of atomic
VDATA[0:1] = (GLC) ? P : VDATA[0:1] // last part of atomic
```

#### BUFFER_ATOMIC_FMAX

Opcode: 64 (0x40) for GCN 1.0/1.1  
Syntax: BUFFER_ATOMIC_FMAX VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose greatest single floating point value from VDATA and from
SRSRC resource, and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
FLOAT* VM = (FLOAT*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = MAX(*VM, ASFLOAT(VDATA)); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_FMAX_X2

Opcode: 96 (0x60) for GCN 1.0/1.1  
Syntax: BUFFER_ATOMIC_FMAX_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose greatest double floating point value from VDATA and from
SRSRC resource, and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
DOUBLE* VM = (DOUBLE*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = MAX(*VM, ASDOUBLE(VDATA)); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_FMIN

Opcode: 63 (0x3f) for GCN 1.0/1.1  
Syntax: BUFFER_ATOMIC_FMIN VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose smallest single floating point value from VDATA and from
SRSRC resource, and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
FLOAT* VM = (FLOAT*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = MIN(*VM, ASFLOAT(VDATA)); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_FMIN_X2

Opcode: 95 (0x5f) for GCN 1.0/1.1  
Syntax: BUFFER_ATOMIC_FMIN_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose smallest double floating point value from VDATA and from
SRSRC resource, and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
DOUBLE* VM = (DOUBLE*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = MIN(*VM, ASDOUBLE(VDATA)); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_INC

Opcode: 60 (0x3c) for GCN 1.0/1.1; 75 (0x4b) for GCN 1.2  
Syntax: BUFFER_ATOMIC_INC VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Compare value from SRSRC resource and if less than VDATA,
then increment value from resource, otherwise store zero to resource.
If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = (*VM < VDATA) ? *VM+1 : 0; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_INC_X2

Opcode: 92 (0x5c) for GCN 1.0/1.1; 107 (0x9b) for GCN 1.2  
Syntax: BUFFER_ATOMIC_INC_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Compare 64-bit value from SRSRC resource and if less than VDATA,
then increment value from resource, otherwise store zero to resource.
If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = (*VM < VDATA) ? *VM+1 : 0; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_OR

Opcode: 58 (0x3a) for GCN 1.0/1.1; 73 (0x49) for GCN 1.2  
Syntax: BUFFER_ATOMIC_OR VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Do bitwise OR on VDATA and value of SRSRC resource,
and store result to this resource. If GLC flag is set then return previous value
from resource to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = *VM | VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_OR_X2

Opcode: 90 (0x5a) for GCN 1.0/1.1; 105 (0x69) for GCN 1.2  
Syntax: BUFFER_ATOMIC_OR_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Do 64-bit bitwise OR on VDATA and value of SRSRC resource,
and store result to this resource. If GLC flag is set then return previous value
from resource to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = *VM | VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_RSUB

Opcode: 52 (0x34) for GCN 1.0  
Syntax: BUFFER_ATOMIC_RSUB VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Subtract value of SRSRC resource from VDATA, and store result to
this resource. If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = VDATA - *VM; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_RSUB_X2

Opcode: 84 (0x54) for GCN 1.0  
Syntax: BUFFER_ATOMIC_RSUB_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Subtract 64-bit value of SRSRC resource from 64-bit VDATA, and store result
to this resource. If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = VDATA - *VM; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SMAX

Opcode: 55 (0x37) for GCN 1.0/1.1; 70 (0x46) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SMAX VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose greatest signed 32-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = MAX(*VM, (INT32)VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SMAX_X2

Opcode: 87 (0x57) for GCN 1.0/1.1; 102 (0x66) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SMAX_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose greatest signed 64-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
INT64* VM = (INT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = MAX(*VM, (INT64)VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SMIN

Opcode: 53 (0x35) for GCN 1.0/1.1; 68 (0x44) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SMIN VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose smallest signed 32-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = MIN(*VM, (INT32)VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SMIN_X2

Opcode: 85 (0x55) for GCN 1.0/1.1; 100 (0x64) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SMIN_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose smallest signed 64-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
INT64* VM = (INT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = MIN(*VM, (INT64)VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SUB

Opcode: 51 (0x33) for GCN 1.0/1.1; 67 (0x43) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SUB VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Subtract VDATA from value from SRSRC resource, and store result to
this resource. If GLC flag is set then return previous value from resource to VDATA,
otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = *VM - VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SUB_X2

Opcode: 83 (0x53) for GCN 1.0/1.1; 99 (0x63) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SUB_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Subtract 64-bit VDATA from 64-bit value from SRSRC resource, and store
result to this resource. If GLC flag is set then return previous value from resource to
VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = *VM - VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SWAP

Opcode: 48 (0x30) for GCN 1.0/1.1; 64 (0x40) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SWAP VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store VDATA dword into SRSRC resource. If GLC flag is set then
return previous value from resource to VDATA, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_SWAP_X2

Opcode: 80 (0x50) for GCN 1.0/1.1; 96 (0x60) for GCN 1.2  
Syntax: BUFFER_ATOMIC_SWAP_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store VDATA 64-bit word into SRSRC resource. If GLC flag is set then
return previous value from resource to VDATA, otherwise keep VDATA value.
Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_UMAX

Opcode: 56 (0x38) for GCN 1.0/1.1; 71 (0x47) for GCN 1.2  
Syntax: BUFFER_ATOMIC_UMAX VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose greatest unsigned 32-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise
keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = MAX(*VM, VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_UMAX_X2

Opcode: 88 (0x58) for GCN 1.0/1.1; 103 (0x67) for GCN 1.2  
Syntax: BUFFER_ATOMIC_UMAX_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose greatest unsigned 64-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = MAX(*VM, VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_UMIN

Opcode: 54 (0x36) for GCN 1.0/1.1; 69 (0x45) for GCN 1.2  
Syntax: BUFFER_ATOMIC_UMIN VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose smallest unsigned 32-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = MIN(*VM, VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_UMIN_X2

Opcode: 86 (0x56) for GCN 1.0/1.1; 101 (0x65) for GCN 1.2  
Syntax: BUFFER_ATOMIC_UMIN_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Choose smallest unsigned 64-bit value from VDATA and from SRSRC resource,
and store result to this resource.
If GLC flag is set then return previous value from resource to VDATA, otherwise keep
VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = MIN(*VM, VDATA); VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_XOR

Opcode: 59 (0x3b) for GCN 1.0/1.1; 74 (0x4a) for GCN 1.2  
Syntax: BUFFER_ATOMIC_XOR VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Do bitwise XOR on VDATA and value of SRSRC resource,
and store result to this resource. If GLC flag is set then return previous value
from resource to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT32 P = *VM; *VM = *VM ^ VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_ATOMIC_XOR_X2

Opcode: 91 (0x5b) for GCN 1.0/1.1; 106 (0x6a) for GCN 1.2  
Syntax: BUFFER_ATOMIC_XOR_X2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Do 64-bit bitwise XOR on VDATA and value of SRSRC resource,
and store result to this resource. If GLC flag is set then return previous value
from resource to VDATA, otherwise keep VDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
UINT64 P = *VM; *VM = *VM ^ VDATA; VDATA = (GLC) ? P : VDATA // atomic
```

#### BUFFER_LOAD_DWORD

Opcode: 12 (0xc) for GCN 1.0/1.1; 20 (0x14) for GCN 1.2  
Syntax: BUFFER_LOAD_DWORD VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load dword to VDATA from SRSRC resource.  
Operation:  
```
VDATA = *(UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_DWORDX2

Opcode: 13 (0xd) for GCN 1.0/1.1; 21 (0x15) for GCN 1.2  
Syntax: BUFFER_LOAD_DWORDX2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load two dwords to VDATA from SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
VDATA[0] = VM[0]
VDATA[1] = VM[1]
```

#### BUFFER_LOAD_DWORDX3

Opcode: 15 (0xf) for GCN 1.1; 22 (0x16) for GCN 1.2  
Syntax: BUFFER_LOAD_DWORDX3 VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load three dwords to VDATA from SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
VDATA[0] = VM[0]
VDATA[1] = VM[1]
VDATA[2] = VM[2]
```

#### BUFFER_LOAD_DWORDX4

Opcode: 14 (0xe) for GCN 1.0/1.1; 23 (0x17) for GCN 1.2  
Syntax: BUFFER_LOAD_DWORDX4 VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load four dwords to VDATA from SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
VDATA[0] = VM[0]
VDATA[1] = VM[1]
VDATA[2] = VM[2]
VDATA[3] = VM[3]
```

#### BUFFER_LOAD_FORMAT_D16_X

Opcode: 8 (0x8)  
Syntax: BUFFER_LOAD_FORMAT_D16_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first component of the element from SRSRC including format from
buffer resource. Store result as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_D16_HI_X

Opcode: 38 (0x26) for GCN 1.4  
Syntax: BUFFER_LOAD_FORMAT_D16_HI_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first component of the element from SRSRC including format from
buffer resource. Store result as 16-bit value to higher part of VDATA register
(half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET)<<16
```

#### BUFFER_LOAD_FORMAT_D16_XY

Opcode: 9 (0x9)  
Syntax: BUFFER_LOAD_FORMAT_D16_XY VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): BUFFER_LOAD_FORMAT_D16_XY VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first two components of the element from SRSRC resource
including format from SRSRC. Store result as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_XY(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_D16_XYZ

Opcode: 10 (0xa)  
Syntax: BUFFER_LOAD_FORMAT_D16_XYZ VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): BUFFER_LOAD_FORMAT_D16_XYZ VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first three components of the element from SRSRC resource
including format from SRSRC. Store result as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_XYZ(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_D16_XYZW

Opcode: 11 (0xb)  
Syntax: BUFFER_LOAD_FORMAT_D16_XYZW VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): BUFFER_LOAD_FORMAT_D16_XYZW VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the all four components of the element from SRSRC resource 
including format from SRSRC. Store result as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_XYZW(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_X

Opcode: 0 (0x0)  
Syntax: BUFFER_LOAD_FORMAT_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first component of the element from SRSRC including format from
buffer resource.  
Operation:  
```
VDATA = LOAD_FORMAT_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_XY

Opcode: 1 (0x1)  
Syntax: BUFFER_LOAD_FORMAT_XY VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first two components of the element from SRSRC resource
including format from SRSRC.  
Operation:  
```
VDATA = LOAD_FORMAT_XY(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_XYZ

Opcode: 2 (0x2)  
Syntax: BUFFER_LOAD_FORMAT_XYZ VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first three components of the element from SRSRC resource
including format from SRSRC.  
Operation:  
```
VDATA = LOAD_FORMAT_XYZ(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_FORMAT_XYZW

Opcode: 3 (0x3)  
Syntax: BUFFER_LOAD_FORMAT_XYZW VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the all four components of the element from SRSRC resource 
including format from SRSRC.  
Operation:  
```
VDATA = LOAD_FORMAT_XYZW(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_SBYTE

Opcode: 9 (0x9) for GCN 1.0/1.1; 17 (0x11) for GCN 1.2  
Syntax: BUFFER_LOAD_SBYTE VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load byte to VDATA from SRSRC resource with sign extending.  
Operation:  
```
VDATA = *(INT8*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_SBYTE_D16

Opcode: 34 (0x22) for GCN 1.4  
Syntax: BUFFER_LOAD_SBYTE_D16 VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load byte to VDATA from SRSRC resource with sign extending to
lower 16-bit part of VDATA.  
Operation:  
```
VDATA &= 0xffff0000
VDATA |= (UINT32)*(INT8*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)&0xffff
```

#### BUFFER_LOAD_SBYTE_D16_HI

Opcode: 35 (0x23) for GCN 1.4  
Syntax: BUFFER_LOAD_SBYTE_D16_HI VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load byte to VDATA from SRSRC resource with sign extending to
higher 16-bit part of VDATA.  
Operation:  
```
VDATA &= 0xffff
VDATA |= (UINT32)*(INT8*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)<<16
```

#### BUFFER_LOAD_SHORT_D16

Opcode: 36 (0x24) for GCN 1.4  
Syntax: BUFFER_LOAD_SHORT_D16 VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load 16-bit word to VDATA from SRSRC resource to lower part of VDATA.  
Operation:  
```
VDATA &= 0xffff0000
VDATA |= *(UINT16*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_SHORT_D16

Opcode: 37 (0x25) for GCN 1.4  
Syntax: BUFFER_LOAD_SHORT_D16_HI VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load 16-bit word to VDATA from SRSRC resource to part part of VDATA.  
Operation:  
```
VDATA &= 0xffff
VDATA |= *(UINT16*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)<<16
```

#### BUFFER_LOAD_SSHORT

Opcode: 11 (0xb) for GCN 1.0/1.1; 19 (0x13) for GCN 1.2  
Syntax: BUFFER_LOAD_SSHORT VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load 16-bit word to VDATA from SRSRC resource with sign extending.  
Operation:  
```
VDATA = *(INT16*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_UBYTE

Opcode: 8 (0x8) for GCN 1.0/1.1; 16 (0x10) for GCN 1.2  
Syntax: BUFFER_LOAD_UBYTE VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load byte to VDATA from SRSRC resource with zero extending.  
Operation:  
```
VDATA = *(UINT8*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_LOAD_UBYTE_D16

Opcode: 32 (0x20) for GCN 1.4  
Syntax: BUFFER_LOAD_UBYTE_D16 VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load byte to VDATA from SRSRC resource with zero extending to
lower 16-bit part of VDATA.  
Operation:  
```
VDATA &= 0xffff0000
VDATA |= (UINT32)*(UINT8*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)&0xffff
```

#### BUFFER_LOAD_UBYTE_D16_HI

Opcode: 33 (0x21) for GCN 1.4  
Syntax: BUFFER_LOAD_UBYTE_D16_HI VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load byte to VDATA from SRSRC resource with zero extending to
higher 16-bit part of VDATA.  
Operation:  
```
VDATA &= 0xffff
VDATA |= (UINT32)*(UINT8*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)<<16
```

#### BUFFER_LOAD_USHORT

Opcode: 10 (0xa) for GCN 1.0/1.1; 18 (0x12) for GCN 1.2  
Syntax: BUFFER_LOAD_USHORT VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load 16-bit word to VDATA from SRSRC resource with zero extending.  
Operation:  
```
VDATA = *(UINT16*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
```

#### BUFFER_STORE_BYTE

Opcode: 24 (0x18)  
Syntax: BUFFER_STORE_BYTE VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store byte from VDATA into SRSRC resource.  
Operation:  
```
*(UINT8*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET) = VDATA&0xff
```

#### BUFFER_STORE_BYTE

Opcode: 25 (0x19)  
Syntax: BUFFER_STORE_BYTE_D16_HI VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store byte from 16-23 bits of VDATA into SRSRC resource.  
Operation:  
```
*(UINT8*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET) = (VDATA>>16)&0xff
```

#### BUFFER_STORE_DWORD

Opcode: 28 (0x1c)  
Syntax: BUFFER_STORE_DWORD VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store dword from VDATA into SRSRC resource.  
Operation:  
```
*(UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET) = VDATA
```

#### BUFFER_STORE_DWORDX2

Opcode: 29 (0x1d)  
Syntax: BUFFER_STORE_DWORDX2 VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store two dwords from VDATA into SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
VM[0] = VDATA[0]
VM[1] = VDATA[1]
```

#### BUFFER_STORE_DWORDX3

Opcode: 31 (0x1f) for GCN 1.1; 30 (0x1e) for GCN 1.2  
Syntax: BUFFER_STORE_DWORDX2 VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store three dwords from VDATA into SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
VM[0] = VDATA[0]
VM[1] = VDATA[1]
VM[2] = VDATA[2]
```

#### BUFFER_STORE_DWORDX4

Opcode: 31 (0x1e) for GCN 1.1; 31 (0x1f) for GCN 1.2  
Syntax: BUFFER_STORE_DWORDX2 VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store four dwords from VDATA into SRSRC resource.  
Operation:  
```
UINT32* VM = *(UINT32*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET)
VM[0] = VDATA[0]
VM[1] = VDATA[1]
VM[2] = VDATA[2]
VM[3] = VDATA[3]
```

#### BUFFER_STORE_FORMAT_D16_X

Opcode: 12 (0xc)  
Syntax: BUFFER_STORE_FORMAT_D16_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first component of the element into SRSRC resource
including format from SRSRC. Treat input as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_D16_HI_X

Opcode: 39 (0x27) for GCN 1.4  
Syntax: BUFFER_STORE_FORMAT_D16_HI_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first component of the element into SRSRC resource
including format from SRSRC. Treat input as 16-bit value stored in higher part of VDATA
(half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA>>16)
```

#### BUFFER_STORE_FORMAT_D16_XY

Opcode: 13 (0xd)  
Syntax: BUFFER_STORE_FORMAT_D16_XY VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): BUFFER_STORE_FORMAT_D16_XY VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first two components of the element into SRSRC resource
including format from SRSRC. Treat input as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_XY(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_D16_XYZ

Opcode: 14 (0xe)  
Syntax: BUFFER_STORE_FORMAT_D16_XYZ VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): BUFFER_STORE_FORMAT_D16_XYZ VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first three components of the element into SRSRC resource
including format from SRSRC. Treat input as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_XYZ(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_D16_XYZW

Opcode: 15 (0xf)  
Syntax: BUFFER_STORE_FORMAT_D16_XYZW VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): BUFFER_STORE_FORMAT_D16_XYZW VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the all components of the element into SRSRC resource
including format from SRSRC. Treat input as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_XYZW(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_X

Opcode: 4 (0x4)  
Syntax: BUFFER_STORE_FORMAT_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first component of the element into SRSRC resource
including format from SRSRC.  
Operation:  
```
STORE_FORMAT_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_XY

Opcode: 5 (0x5)  
Syntax: BUFFER_STORE_FORMAT_XY VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first two components of the element into SRSRC resource
including format from SRSRC.  
Operation:  
```
STORE_FORMAT_XY(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_XYZ

Opcode: 6 (0x6)  
Syntax: BUFFER_STORE_FORMAT_XYZ VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first three components of the element into SRSRC resource
including format from SRSRC.  
Operation:  
```
STORE_FORMAT_XYZ(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_FORMAT_XYZW

Opcode: 7 (0x7)  
Syntax: BUFFER_STORE_FORMAT_XYZW VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the all components of the element into SRSRC resource
including format from SRSRC.  
Operation:  
```
STORE_FORMAT_XYZW(SRSRC, VADDR(1:2), SOFFSET, OFFSET, VDATA)
```

#### BUFFER_STORE_LDS_DWORD

Opcode: 61 (0x3d) for GCN 1.2  
Syntax: BUFFER_STORE_LDS_DWORD SRSRC(4), SOFFSET  
Description: Store single dword from LDS into SRSRC resource.  
Operation:  
```
UINT32 VAL = *(UINT32*)(LDS + (M0&0xffff) + OFFSET + LANEID*4)
*(UINT32*)VMEM(SRSRC, 0, SOFFSET, OFFSET) = VAL
```

#### BUFFER_STORE_SHORT

Opcode: 26 (0x1a)  
Syntax: BUFFER_STORE_SHORT VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store 16-bit word from VDATA into SRSRC resource.  
Operation:  
```
*(UINT16*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET) = VDATA&0xffff
```

#### BUFFER_STORE_SHORT

Opcode: 27 (0x1b) for GCN 1.4  
Syntax: BUFFER_STORE_SHORT_D16 VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store higher 16-bit word from VDATA into SRSRC resource.  
Operation:  
```
*(UINT16*)VMEM(SRSRC, VADDR(1:2), SOFFSET, OFFSET) = VDATA>>16
```
