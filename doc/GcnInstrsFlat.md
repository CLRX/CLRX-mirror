## GCN ISA FLAT instructions

These instructions allow to access to main memory, LDS and scratch buffer.
FLAT instructions fetch address from 2 vector registers that hold 64-bit address.
FLAT instructions presents only in GCN 1.1 or later architecture.

List of fields for the FLAT encoding (GCN 1.1/1.2):

Bits  | Name     | Description
------|----------|------------------------------
0-12  | OFFSET   | Byte offset (GCN 1.4)
13    | LDS      | transfer DATA to LDS and memory (GCN 1.4)
14-15 | SEG      | Memory segment (instrunction type) (GCN 1.4)
16    | GLC      | Operation globally coherent
17    | SLC      | System level coherent
18-24 | OPCODE   | Operation code
25-31 | ENCODING | Encoding type. Must be 0b110111
32-39 | VADDR    | Vector address registers
40-47 | VDATA    | Vector data register
48-54 | SADDR    | Scalar SGPR offset (0x7f value disables it) (GCN 1.4)
55    | TFE      | Texture Fail Enable ??? (GCN 1.1/1.2)
55    | NV       | Non-Volatile (GCN 1.4)
56-63 | VDST     | Vector destination register

List of fields for the FLAT encoding (GCN 1.4):

Bits  | Name     | Description
------|----------|------------------------------
0-12  | OFFSET   | Byte offset
13    | LDS      | transfer DATA to LDS and memory
14-15 | SEG      | Memory segment (instrunction type)
16    | GLC      | Operation globally coherent
17    | SLC      | System level coherent
18-24 | OPCODE   | Operation code
25-31 | ENCODING | Encoding type. Must be 0b110111
32-39 | VADDR    | Vector address registers
40-47 | VDATA    | Vector data register
48-54 | SADDR    | Scalar SGPR offset (for GLOBAL/SCRATCH) (0x7f value disables it)
55    | NV       | Non-Volatile
56-63 | VDST     | Vector destination register

Instruction types:

SEG | Prefix  | Description
----|---------|-------------------------
 0  | FLAT    | FLAT instruction (global, private or scratch memory)
 1  | SCRATCH | SCRATCH instruction (only for scratch memory access)
 2  | GLOBAL  | GLOBAL instruction (only for global memory access)

Instruction syntax: INSTRUCTION VDST, VADDR(2) [MODIFIERS]  
Instruction syntax: INSTRUCTION VADDR(2), VDATA [MODIFIERS]

GLOBAL instruction syntax: INSTRUCTION VDST, VADDR(2), SADDR(2)|OFF [MODIFIERS]  
GLOBAL instruction syntax: INSTRUCTION VADDR(1:2), VDATA, SADDR(2)|OFF [MODIFIERS]  
SCRATCH instruction syntax: INSTRUCTION VDST, VADDR(2), SADDR|OFF [MODIFIERS]  
SCRATCH instruction syntax: INSTRUCTION VADDR, VDATA, SADDR|OFF [MODIFIERS]

Modifiers can be supplied in any order. Modifiers list: SLC, GLC, TFE,
LDS, NV, INST_OFFSET:OFFSET. The TFE flag requires additional the VDATA register.
LDS, NV and OFFSET are available only in GCN 1.4 architecture.

FLAT instruction can complete out of order with each other. This can be caused by different
resources from/to that instruction can load/store. FLAT instruction increase VMCNT if access
to main memory, or LKGMCNT if accesses to LDS.

OFFSET (INST_OFFSET modifier) can be 13-bit signed for GLOBAL_\* and SCRATCH_\*
instructions or 12-bit unsigned for FLAT_\* instructions.

For GLOBAL instruction VADDR have 2 registers if SADDR is OFF, otherwise VADDR holds
32-bit offset in single VGPR register.

### Instructions by opcode

List of the FLAT instructions by opcode (GCN 1.1/1.2):

 Opcode     | Mnemonic (GCN1.1)      | Mnemonic (GCN1.2)
------------|------------------------|------------------------
 8 (0x8)    | FLAT_LOAD_UBYTE        | --
 9 (0x9)    | FLAT_LOAD_SBYTE        | --
 10 (0xa)   | FLAT_LOAD_USHORT       | --
 11 (0xb)   | FLAT_LOAD_SSHORT       | --
 12 (0xc)   | FLAT_LOAD_DWORD        | --
 13 (0xd)   | FLAT_LOAD_DWORDX2      | --
 14 (0xe)   | FLAT_LOAD_DWORDX4      | --
 15 (0xf)   | FLAT_LOAD_DWORDX3      | --
 16 (0x10)  | --                     | FLAT_LOAD_UBYTE
 17 (0x11)  | --                     | FLAT_LOAD_SBYTE
 18 (0x12)  | --                     | FLAT_LOAD_USHORT
 19 (0x13)  | --                     | FLAT_LOAD_SSHORT
 20 (0x14)  | --                     | FLAT_LOAD_DWORD
 21 (0x15)  | --                     | FLAT_LOAD_DWORDX2
 22 (0x16)  | --                     | FLAT_LOAD_DWORDX3
 23 (0x17)  | --                     | FLAT_LOAD_DWORDX4
 24 (0x18)  | FLAT_STORE_BYTE        | FLAT_STORE_BYTE
 26 (0x1a)  | FLAT_STORE_SHORT       | FLAT_STORE_SHORT
 28 (0x1c)  | FLAT_STORE_DWORD       | FLAT_STORE_DWORD
 29 (0x1d)  | FLAT_STORE_DWORDX2     | FLAT_STORE_DWORDX2
 30 (0x1e)  | FLAT_STORE_DWORDX4     | FLAT_STORE_DWORDX3
 31 (0x1f)  | FLAT_STORE_DWORDX3     | FLAT_STORE_DWORDX4
 48 (0x30)  | FLAT_ATOMIC_SWAP       | --
 49 (0x31)  | FLAT_ATOMIC_CMPSWAP    | --
 50 (0x32)  | FLAT_ATOMIC_ADD        | --
 51 (0x33)  | FLAT_ATOMIC_SUB        | --
 53 (0x35)  | FLAT_ATOMIC_SMIN       | --
 54 (0x36)  | FLAT_ATOMIC_UMIN       | --
 55 (0x37)  | FLAT_ATOMIC_SMAX       | --
 56 (0x38)  | FLAT_ATOMIC_UMAX       | --
 57 (0x39)  | FLAT_ATOMIC_AND        | --
 58 (0x3a)  | FLAT_ATOMIC_OR         | --
 59 (0x3b)  | FLAT_ATOMIC_XOR        | --
 60 (0x3c)  | FLAT_ATOMIC_INC        | --
 61 (0x3d)  | FLAT_ATOMIC_DEC        | --
 62 (0x3e)  | FLAT_ATOMIC_FCMPSWAP   | --
 63 (0x3f)  | FLAT_ATOMIC_FMIN       | --
 64 (0x40)  | FLAT_ATOMIC_FMAX       | FLAT_ATOMIC_SWAP
 65 (0x41)  | --                     | FLAT_ATOMIC_CMPSWAP
 66 (0x42)  | --                     | FLAT_ATOMIC_ADD
 67 (0x43)  | --                     | FLAT_ATOMIC_SUB
 68 (0x44)  | --                     | FLAT_ATOMIC_SMIN
 69 (0x45)  | --                     | FLAT_ATOMIC_UMIN
 70 (0x46)  | --                     | FLAT_ATOMIC_SMAX
 71 (0x47)  | --                     | FLAT_ATOMIC_UMAX
 72 (0x48)  | --                     | FLAT_ATOMIC_AND
 73 (0x49)  | --                     | FLAT_ATOMIC_OR
 74 (0x4a)  | --                     | FLAT_ATOMIC_XOR
 75 (0x4b)  | --                     | FLAT_ATOMIC_INC
 76 (0x4c)  | --                     | FLAT_ATOMIC_DEC
 80 (0x50)  | FLAT_ATOMIC_SWAP_X2    | --
 81 (0x51)  | FLAT_ATOMIC_CMPSWAP_X2 | --
 82 (0x52)  | FLAT_ATOMIC_ADD_X2     | --
 83 (0x53)  | FLAT_ATOMIC_SUB_X2     | --
 85 (0x55)  | FLAT_ATOMIC_SMIN_X2    | --
 86 (0x56)  | FLAT_ATOMIC_UMIN_X2    | --
 87 (0x57)  | FLAT_ATOMIC_SMAX_X2    | --                                                                                         
 88 (0x58)  | FLAT_ATOMIC_UMAX_X2    | --                                                                                         
 89 (0x59)  | FLAT_ATOMIC_AND_X2     | --
 90 (0x5a)  | FLAT_ATOMIC_OR_X2      | --
 91 (0x5b)  | FLAT_ATOMIC_XOR_X2     | --
 92 (0x5c)  | FLAT_ATOMIC_INC_X2     | --
 93 (0x5d)  | FLAT_ATOMIC_DEC_X2     | --
 94 (0x5e)  | FLAT_ATOMIC_FCMPSWAP_X2 | --
 95 (0x5f)  | FLAT_ATOMIC_FMIN_X2    | --
 96 (0x60)  | FLAT_ATOMIC_FMAX_X2    | FLAT_ATOMIC_SWAP_X2
 97 (0x61)  | --                     | FLAT_ATOMIC_CMPSWAP_X2
 98 (0x62)  | --                     | FLAT_ATOMIC_ADD_X2
 99 (0x63)  | --                     | FLAT_ATOMIC_SUB_X2
 100 (0x64) | --                     | FLAT_ATOMIC_SMIN_X2
 101 (0x65) | --                     | FLAT_ATOMIC_UMIN_X2
 102 (0x66) | --                     | FLAT_ATOMIC_SMAX_X2
 103 (0x67) | --                     | FLAT_ATOMIC_UMAX_X2
 104 (0x68) | --                     | FLAT_ATOMIC_AND_X2
 105 (0x69) | --                     | FLAT_ATOMIC_OR_X2
 106 (0x6a) | --                     | FLAT_ATOMIC_XOR_X2
 107 (0x6b) | --                     | FLAT_ATOMIC_INC_X2
 108 (0x6c) | --                     | FLAT_ATOMIC_DEC_X2

List of the FLAT/GLOBAL/SCRATCH instructions by opcode (GCN 1.4):

 Opcode     | FLAT | GLOBAL | SCRATCH | Mnemonic
------------|------|--------|---------|------------------------
 16 (0x10)  |   ✓  |   ✓    |    ✓    | \*_LOAD_UBYTE
 17 (0x11)  |   ✓  |   ✓    |    ✓    | \*_LOAD_SBYTE
 18 (0x12)  |   ✓  |   ✓    |    ✓    | \*_LOAD_USHORT
 19 (0x13)  |   ✓  |   ✓    |    ✓    | \*_LOAD_SSHORT
 20 (0x14)  |   ✓  |   ✓    |    ✓    | \*_LOAD_DWORD
 21 (0x15)  |   ✓  |   ✓    |    ✓    | \*_LOAD_DWORDX2
 22 (0x16)  |   ✓  |   ✓    |    ✓    | \*_LOAD_DWORDX3
 23 (0x17)  |   ✓  |   ✓    |    ✓    | \*_LOAD_DWORDX4
 24 (0x18)  |   ✓  |   ✓    |    ✓    | \*_STORE_BYTE
 25 (0x19)  |   ✓  |   ✓    |    ✓    | \*_STORE_BYTE_D16_HI
 26 (0x1a)  |   ✓  |   ✓    |    ✓    | \*_STORE_SHORT
 27 (0x1b)  |   ✓  |   ✓    |    ✓    | \*_STORE_SHORT_D16_HI
 28 (0x1c)  |   ✓  |   ✓    |    ✓    | \*_STORE_DWORD
 29 (0x1d)  |   ✓  |   ✓    |    ✓    | \*_STORE_DWORDX2
 30 (0x1e)  |   ✓  |   ✓    |    ✓    | \*_STORE_DWORDX3
 31 (0x1f)  |   ✓  |   ✓    |    ✓    | \*_STORE_DWORDX4
 32 (0x20)  |   ✓  |   ✓    |    ✓    | \*_LOAD_UBYTE_D16
 33 (0x21)  |   ✓  |   ✓    |    ✓    | \*_LOAD_UBYTE_D16_HI
 34 (0x22)  |   ✓  |   ✓    |    ✓    | \*_LOAD_SBYTE_D16
 35 (0x23)  |   ✓  |   ✓    |    ✓    | \*_LOAD_SBYTE_D16_HI
 36 (0x24)  |   ✓  |   ✓    |    ✓    | \*_LOAD_SHORT_D16
 37 (0x25)  |   ✓  |   ✓    |    ✓    | \*_LOAD_SHORT_D16_HI
 64 (0x40)  |   ✓  |   ✓    |         | \*_ATOMIC_SWAP
 65 (0x41)  |   ✓  |   ✓    |         | \*_ATOMIC_CMPSWAP
 66 (0x42)  |   ✓  |   ✓    |         | \*_ATOMIC_ADD
 67 (0x43)  |   ✓  |   ✓    |         | \*_ATOMIC_SUB
 68 (0x44)  |   ✓  |   ✓    |         | \*_ATOMIC_SMIN
 69 (0x45)  |   ✓  |   ✓    |         | \*_ATOMIC_UMIN
 70 (0x46)  |   ✓  |   ✓    |         | \*_ATOMIC_SMAX
 71 (0x47)  |   ✓  |   ✓    |         | \*_ATOMIC_UMAX
 72 (0x48)  |   ✓  |   ✓    |         | \*_ATOMIC_AND
 73 (0x49)  |   ✓  |   ✓    |         | \*_ATOMIC_OR
 74 (0x4a)  |   ✓  |   ✓    |         | \*_ATOMIC_XOR
 75 (0x4b)  |   ✓  |   ✓    |         | \*_ATOMIC_INC
 76 (0x4c)  |   ✓  |   ✓    |         | \*_ATOMIC_DEC
 96 (0x60)  |   ✓  |   ✓    |         | \*_ATOMIC_SWAP_X2
 97 (0x61)  |   ✓  |   ✓    |         | \*_ATOMIC_CMPSWAP_X2
 98 (0x62)  |   ✓  |   ✓    |         | \*_ATOMIC_ADD_X2
 99 (0x63)  |   ✓  |   ✓    |         | \*_ATOMIC_SUB_X2
 100 (0x64) |   ✓  |   ✓    |         | \*_ATOMIC_SMIN_X2
 101 (0x65) |   ✓  |   ✓    |         | \*_ATOMIC_UMIN_X2
 102 (0x66) |   ✓  |   ✓    |         | \*_ATOMIC_SMAX_X2
 103 (0x67) |   ✓  |   ✓    |         | \*_ATOMIC_UMAX_X2
 104 (0x68) |   ✓  |   ✓    |         | \*_ATOMIC_AND_X2
 105 (0x69) |   ✓  |   ✓    |         | \*_ATOMIC_OR_X2
 106 (0x6a) |   ✓  |   ✓    |         | \*_ATOMIC_XOR_X2
 107 (0x6b) |   ✓  |   ✓    |         | \*_ATOMIC_INC_X2
 108 (0x6c) |   ✓  |   ✓    |         | \*_ATOMIC_DEC_X2

The '\*' means prefix of instruction (FLAT, GLOBAL or SCRATCH).
 
### Instruction set

Alphabetically sorted instruction list:

#### FLAT_ATOMIC_ADD

Opcode: 50 (0x32) for GCN 1.1; 66 (0x42) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_ADD VDST, VADDR(2), VDATA  
Description: Add VDATA to value of memory address, and store result to this address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM + VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_ADD_X2

Opcode: 82 (0x52) for GCN 1.1; 98 (0x62) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_ADD_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Add 64-bit VDATA to 64-bit value of memory address, and store result
to this address. If GLC flag is set then return previous value from address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM + VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_AND

Opcode: 57 (0x39) for GCN 1.1; 72 (0x48) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_AND VDST, VADDR(2), VDATA  
Description: Do bitwise AND on VDATA and value of memory address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM & VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_AND_X2

Opcode: 89 (0x59) for GCN 1.1; 104 (0x68) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_AND_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Do 64-bit bitwise AND on VDATA and value of memory address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM & VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_CMPSWAP

Opcode: 49 (0x31) for GCN 1.1; 65 (0x41) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_CMPSWAP VDST, VADDR(2), VDATA(2)  
Description: Store lower VDATA dword into memory address  if previous value
from that address is equal VDATA>>32, otherwise keep old value from address.
If GLC flag is set then return previous value from address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM==(VDATA>>32) ? VDATA&0xffffffff : *VM // part of atomic
VDST = (GLC) ? P : VDST // last part of atomic
```

#### FLAT_ATOMIC_CMPSWAP_X2

Opcode: 81 (0x51) for GCN 1.1; 97 (0x61) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_CMPSWAP_X2 VDST(2), VADDR(2), VDATA(4)  
Description: Store lower VDATA 64-bit word into memory address if previous value
from address is equal VDATA>>64, otherwise keep old value from VADDR.
If GLC flag is set then return previous value from VADDR to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM==(VDATA[2:3]) ? VDATA[0:1] : *VM // part of atomic
VDST = (GLC) ? P : VDST // last part of atomic
```

#### FLAT_ATOMIC_DEC

Opcode: 61 (0x3d) for GCN 1.1; 76 (0x4c) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_DEC VDST, VADDR(2), VDATA  
Description: Compare value from memory address and if less or equal than VDATA
and this value is not zero, then decrement value from memory address,
otherwise store VDATA to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA // atomic
VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_DEC_X2

Opcode: 93 (0x5d) for GCN 1.1; 108 (0x6c) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_DEC_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Compare 64-bit value from memory address and if less or equal than VDATA
and this value is not zero, then decrement value from memory address,
otherwise store VDATA to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA // atomic
VDST = (GLC) ? P : VDST // atomic
```

#### BUFFER_ATOMIC_FCMPSWAP

Opcode: 62 (0x3e) for GCN 1.1  
Syntax: FLAT_ATOMIC_FCMPSWAP VDST, VADDR(1:2), VDATA(2)  
Description: Store lower VDATA dword into memory address if previous single floating point
value from address is equal singe floating point value VDATA>>32,
otherwise keep old value from memory address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
FLOAT* VM = (FLOAT*)(VADDR + INST_OFFSET)
FLOAT P = *VM; *VM = *VM==ASFLOAT(VDATA>>32) ? VDATA&0xffffffff : *VM // part of atomic
VDST[0] = (GLC) ? P : VDST // last part of atomic
```

#### FLAT_ATOMIC_FCMPSWAP_X2

Opcode: 94 (0x5e) for GCN 1.1  
Syntax: FLAT_ATOMIC_FCMPSWAP_X2 VDATA(2), VADDR(2), SRSRC(4), SOFFSET  
Description: Store lower VDATA 64-bit word into memory address if previous double
floating point value from address is equal singe floating point value VDATA>>32,
otherwise keep old value from memory address.
If GLC flag is set then return previous value from address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
DOUBLE* VM = (DOUBLE*)(VADDR + INST_OFFSET)
DOUBLE P = *VM; *VM = *VM==ASDOUBLE(VDATA[2:3]) ? VDATA[0:1] : *VM // part of atomic
VDST = (GLC) ? P : VDST // last part of atomic
```

#### FLAT_ATOMIC_FMAX

Opcode: 64 (0x40) for GCN 1.1  
Syntax: FLAT_ATOMIC_FMAX VDST, VADDR(2), VDATA  
Description: Choose greatest single floating point value from VDATA and from
memory address, and store result to this address.
If GLC flag is set then return previous value from address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
FLOAT* VM = (FLOAT*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MAX(*VM, ASFLOAT(VDATA)); VDST = (GLC) ? P : VDST // atomic
```

#### BUFFER_ATOMIC_FMAX_X2

Opcode: 96 (0x60) for GCN 1.1  
Syntax: FLAT_ATOMIC_FMAX_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Choose greatest double floating point value from VDATA and from
memory address, and store result to this address.
If GLC flag is set then return previous value from address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
DOUBLE* VM = (DOUBLE*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MAX(*VM, ASDOUBLE(VDATA)); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_FMIN

Opcode: 63 (0x3f) for GCN 1.1  
Syntax: FLAT_ATOMIC_FMIN VDST, VADDR(2), VDATA  
Description: Choose smallest single floating point value from VDATA and from
memory address, and store result to this address.
If GLC flag is set then return previous value from address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
FLOAT* VM = (FLOAT*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MIN(*VM, ASFLOAT(VDATA)); VDST = (GLC) ? P : VDST // atomic
```

#### BUFFER_ATOMIC_FMIN_X2

Opcode: 95 (0x5f) for GCN 1.1  
Syntax: FLAT_ATOMIC_FMIN_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Choose smallest double floating point value from VDATA and from
memory address, and store result to this address.
If GLC flag is set then return previous value from address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
DOUBLE* VM = (DOUBLE*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MIN(*VM, ASDOUBLE(VDATA)); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_INC

Opcode: 60 (0x3c) for GCN 1.1; 75 (0x4b) for GCN 1.2/1.4  
Syntax: FLT_ATOMIC_INC VDST, VADDR(2), VDATA  
Description: Compare value from memory address and if less than VDATA,
then increment value from address, otherwise store zero to address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = (*VM < VDATA) ? *VM+1 : 0; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_INC_X2

Opcode: 92 (0x5c) for GCN 1.1; 107 (0x9b) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_INC_X2 VDST(2), VADDR(2), VADDR(2)  
Description: Compare 64-bit value from memory address and if less than VDATA,
then increment value from address, otherwise store zero to address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = (*VM < VDATA) ? *VM+1 : 0; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_OR

Opcode: 58 (0x3a) for GCN 1.1; 73 (0x49) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_OR VDST, VADDR(2), VDATA  
Description: Do bitwise OR on VDATA and value of memory address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM | VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_OR_X2

Opcode: 90 (0x5a) for GCN 1.1; 105 (0x69) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_OR_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Do 64-bit bitwise OR on VDATA and value of memory address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM | VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SMAX

Opcode: 55 (0x37) for GCN 1.1; 70 (0x46) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_SMAX VDST, VADDR(2), VDATA  
Description: Choose greatest signed 32-bit value from VDATA and from memory address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MAX(*VM, (INT32)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SMAX_X2

Opcode: 87 (0x57) for GCN 1.1; 102 (0x66) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_SMAX_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Choose greatest signed 64-bit value from VDATA and from memory address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
INT64* VM = (INT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MAX(*VM, (INT64)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SMIN

Opcode: 53 (0x35) for GCN 1.1; 68 (0x44) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_SMIN VDST, VADDR(2), VDATA  
Description: Choose smallest signed 32-bit value from VDATA and from memory address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MIN(*VM, (INT32)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SMIN_X2

Opcode: 85 (0x55) for GCN 1.1; 100 (0x64) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_SMIN_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Choose smallest signed 64-bit value from VDATA and from memory address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
INT64* VM = (INT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MIN(*VM, (INT64)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SUB

Opcode: 51 (0x33) for GCN 1.1; 67 (0x43) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_SUB VDST, VADDR(2), VDATA  
Description: Subtract VDATA from value of memory address, and store result to this address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM - VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SUB_X2

Opcode: 83 (0x53) for GCN 1.1; 99 (0x63) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_SUB_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Subtract 64-bit VDATA from 64-bit value of memory address, and store result
to this address. If GLC flag is set then return previous value from address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM - VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SWAP

Opcode: 48 (0x30) for GCN 1.1; 64 (0x40) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_SWAP VDST, VADDR(2), VDATA  
Description: Store VDATA dword into memory address. If GLC flag is set then
return previous value from memory address to VDST, otherwise keep old value from VDST.
Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SWAP_X2

Opcode: 80 (0x50) for GCN 1.1; 96 (0x60) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_SWAP_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Store VDATA 64-bit word into memory address. If GLC flag is set then
return previous value from memory address to VDST, otherwise keep old value from VDST.
Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_UMAX

Opcode: 56 (0x38) for GCN 1.1; 71 (0x47) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_UMAX VDST, VADDR(2), VDATA  
Description: Choose greatest unsigned 32-bit value from VDATA and from memory address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MAX(*VM, (UINT32)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_UMAX_X2

Opcode: 88 (0x58) for GCN 1.1; 103 (0x67) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_UMAX_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Choose greatest unsigned 64-bit value from VDATA and from memory address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MAX(*VM, (UINT64)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_UMIN

Opcode: 54 (0x36) for GCN 1.1; 69 (0x45) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_UMIN VDST, VADDR(2), VDATA  
Description: Choose smallest unsigned 32-bit value from VDATA and from memory address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MIN(*VM, (UINT32)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_UMIN_X2

Opcode: 86 (0x56) for GCN 1.1; 101 (0x65) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_UMIN_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Choose smallest unsigned 64-bit value from VDATA and from memory address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MIN(*VM, (UINT64)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_XOR

Opcode: 59 (0x3b) for GCN 1.1; 74 (0x4a) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_XOR VDST, VADDR(2), VDATA  
Description: Do bitwise XOR on VDATA and value of memory address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM ^ VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_XOR_X2

Opcode: 91 (0x5b) for GCN 1.1; 106 (0x6a) for GCN 1.2/1.4  
Syntax: FLAT_ATOMIC_XOR_X2 VDST(2), VADDR(2), VDATA(2)  
Description: Do 64-bit bitwise XOR on VDATA and value of memory address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM ^ VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_LOAD_DWORD

Opcode: 12 (0xc) for GCN 1.1; 20 (0x14) for GCN 1.2/1.4  
Syntax: FLAT_LOAD_DWORD VDST, VADDR(2)  
Description Load dword to VDST from memory address.  
Operation:  
```
VDST = *(UINT32*)(VADDR + INST_OFFSET)
```

#### FLAT_LOAD_DWORDX2

Opcode: 13 (0xd) for GCN 1.1; 21 (0x15) for GCN 1.2/1.4  
Syntax: FLAT_LOAD_DWORDX2 VDST(, VADDR(2)  
Description Load two dwords to VDST from memory address.  
Operation:  
```
VDST = *(UINT64*)(VADDR + INST_OFFSET)
```

#### FLAT_LOAD_DWORDX3

Opcode: 15 (0xf) for GCN 1.1; 22 (0x16) for GCN 1.2/1.4  
Syntax: FLAT_LOAD_DWORDX3 VDST(3), VADDR(2)  
Description Load three dwords to VDST from memory address.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
VDST[0] = *(UINT32*)VM
VDST[1] = *(UINT32*)(VM+4)
VDST[2] = *(UINT32*)(VM+8)
```

#### FLAT_LOAD_DWORDX4

Opcode: 13 (0xe) for GCN 1.1; 23 (0x17) for GCN 1.2/1.4  
Syntax: FLAT_LOAD_DWORDX4 VDST(4), VADDR(2)  
Description Load four dwords to VDST from memory address.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
VDST[0] = *(UINT32*)VM
VDST[1] = *(UINT32*)(VM+4)
VDST[2] = *(UINT32*)(VM+8)
VDST[3] = *(UINT32*)(VM+12)
```

#### FLAT_LOAD_SBYTE

Opcode: 9 (0x9) for GCN 1.1; 17 (0x11) for GCN 1.2/1.4  
Syntax: FLAT_LOAD_SBYTE VDST, VADDR(2)  
Description: Load byte to VDST from memory address with sign extending.  
Operation:  
```
VDST = *(INT8*)(VADDR + INST_OFFSET)
```

#### FLAT_LOAD_SBYTE_D16

Opcode: 34 (0x22) for GCN 1.4  
Syntax: FLAT_LOAD_SBYTE_D16 VDST, VADDR(2)  
Description: Load byte to lower 16-bit part of VDST from
memory address with sign extending.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
VDST = ((UINT16)*(INT8*)VM) | (VDST&0xffff0000)
```

#### FLAT_LOAD_SBYTE_D16_HI

Opcode: 35 (0x23) for GCN 1.4  
Syntax: FLAT_LOAD_SBYTE_D16_HI VDST, VADDR(2)  
Description: Load byte to higher 16-bit part of VDST from
memory address with sign extending.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
VDST = (((UINT32)*(INT8*)VM)<<16) | (VDST&0xffff)
```

#### FLAT_LOAD_SHORT_D16

Opcode: 36 (0x24) for GCN 1.4  
Syntax: FLAT_LOAD_SHORT_D16 VDST, VADDR(2)  
Description: Load 16-bit word to lower 16-bit part of VDST from memory address.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
VDST = *(UINT16*)VM | (VDST & 0xffff0000)
```

#### FLAT_LOAD_SHORT_D16_HI

Opcode: 36 (0x24) for GCN 1.4  
Syntax: FLAT_LOAD_SHORT_D16_HI VDST, VADDR(2)  
Description: Load 16-bit word to lower 16-bit part of VDST from memory address.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
VDST = (((UINT32)*(UINT16*)VM)<<16) | (VDST & 0xffff)
```

#### FLAT_LOAD_SSHORT

Opcode: 11 (0xb) for GCN 1.1; 19 (0x13) for GCN 1.2/1.4  
Syntax: FLAT_LOAD_SSHORT VDST, VADDR(2)  
Description: Load 16-bit word to VDST from memory address with sign extending.  
Operation:  
```
VDST = *(INT16*)(VADDR + INST_OFFSET)
```

#### FLAT_LOAD_UBYTE

Opcode: 8 (0x8) for GCN 1.1; 16 (0x10) for GCN 1.2/1.4  
Syntax: FLAT_LOAD_UBYTE VDST, VADDR(2)  
Description: Load byte to VDST from memory address with zero extending.  
Operation:  
```
VDST = *(UINT8*)(VADDR + INST_OFFSET)
```

#### FLAT_LOAD_UBYTE_D16

Opcode: 32 (0x20) for GCN 1.4  
Syntax: FLAT_LOAD_UBYTE_D16 VDST, VADDR(2)  
Description: Load byte to lower 16-bit part of VDST from
memory address with zero extending.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
VDST = ((UINT16)*(UINT8*)VM) | (VDST&0xffff0000)
```

#### FLAT_LOAD_UBYTE_D16_HI

Opcode: 33 (0x21) for GCN 1.4  
Syntax: FLAT_LOAD_UBYTE_D16_HI VDST, VADDR(2)  
Description: Load byte to higher 16-bit part of VDST from
memory address with zero extending.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
VDST = (((UINT32)*(UINT8*)VM)<<16) | (VDST&0xffff)
```

#### FLAT_LOAD_USHORT

Opcode: 10 (0xa) for GCN 1.1; 18 (0x12) for GCN 1.2/1.4  
Syntax: FLAT_LOAD_USHORT VDST, VADDR(1:2)  
Description: Load 16-bit word to VDST from memory address with zero extending.  
Operation:  
```
VDST = *(UINT16*)(VADDR + INST_OFFSET)
```

#### FLAT_STORE_BYTE

Opcode: 24 (0x18)  
Syntax: FLAT_STORE_BYTE VADDR(2), VDATA  
Description: Store byte from VDATA to memory address.  
Operation:  
```
*(UINT8*)(VADDR + INST_OFFSET) = VDATA&0xff
```

#### FLAT_STORE_BYTE_D16_HI

Opcode: 25 (0x19) for GCN 1.4  
Syntax: FLAT_STORE_BYTE_D16_HI VADDR(2), VDATA  
Description: Store byte from 16-23 bits of VDATA to memory address.  
Operation:  
```
*(UINT8*)(VADDR + INST_OFFSET) = (VDATA>>16)&0xff
```

#### FLAT_STORE_DWORD

Opcode: 28 (0x1c)  
Syntax: FLAT_STORE_DWORD VADDR(2), VDATA  
Description: Store dword from VDATA to memory address.  
Operation:  
```
*(UINT32*)(VADDR + INST_OFFSET) = VDATA
```

#### FLAT_STORE_DWORDX2

Opcode: 29 (0x1d)  
Syntax: FLAT_STORE_DWORDX2 VADDR(2), VDATA(2)  
Description: Store two dwords from VDATA to memory address.  
Operation:  
```
*(UINT64*)(VADDR + INST_OFFSET) = VDATA
```

#### FLAT_STORE_DWORDX3

Opcode: 31 (0x1f) for GCN 1.1; 30 (0x1e) for GCN 1.2/1.4  
Syntax: FLAT_STORE_DWORDX3 VADDR(2), VDATA(3)  
Description: Store three dwords from VDATA to memory address.  
Operation:  
```
BYTE* VM = (VADDR + INST_OFFSET)
*(UINT32*)(VM) = VDATA[0]
*(UINT32*)(VM+4) = VDATA[1]
*(UINT32*)(VM+8) = VDATA[2]
```

#### FLAT_STORE_DWORDX4

Opcode: 30 (0x1e) for GCN 1.1; 31 (0x1d) for GCN 1.2/1.4  
Syntax: FLAT_STORE_DWORDX4 VADDR(2), VDATA(4)  
Description: Store four dwords from VDATA to memory address.  
Operation:  
```
*(UINT32*)(VM) = VDATA[0]
*(UINT32*)(VM+4) = VDATA[1]
*(UINT32*)(VM+8) = VDATA[2]
*(UINT32*)(VM+12) = VDATA[3]
```

#### FLAT_STORE_SHORT

Opcode: 26 (0x1a)  
Syntax: FLAT_STORE_SHORT VADDR(2), VDATA  
Description: Store 16-bit word from VDATA to memory address.  
Operation:  
```
*(UINT16*)(VADDR + INST_OFFSET) = VDATA&0xffff
```

#### FLAT_STORE_SHORT_D16_HI

Opcode: 27 (0x1b) for GCN 1.4  
Syntax: FLAT_STORE_SHORT_D16_HI VADDR(2), VDATA  
Description: Store 16-bit word from higher 16-bit part of VDATA to memory address.  
Operation:  
```
*(UINT16*)(VADDR + INST_OFFSET) = VDATA>>16
```

#### GLOBAL_ATOMIC_ADD

Opcode: 66 (0x42) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_ADD VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Add VDATA to value of global address, and store result to this address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM + VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_ADD_X2

Opcode: 98 (0x62) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_ADD_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Add 64-bit VDATA to 64-bit value of global address, and store result
to this address. If GLC flag is set then return previous value from address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM + VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_AND

Opcode: 72 (0x48) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_AND VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Do bitwise AND on VDATA and value of global address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM & VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_AND_X2

Opcode: 104 (0x68) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_AND_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Do 64-bit bitwise AND on VDATA and value of global address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM & VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_CMPSWAP

Opcode: 65 (0x41) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_CMPSWAP VDST, VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Store lower VDATA dword into global address  if previous value
from that address is equal VDATA>>32, otherwise keep old value from address.
If GLC flag is set then return previous value from address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM==(VDATA>>32) ? VDATA&0xffffffff : *VM // part of atomic
VDST = (GLC) ? P : VDST // last part of atomic
```

#### GLOBAL_ATOMIC_CMPSWAP_X2

Opcode: 97 (0x61) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_CMPSWAP_X2 VDST(2), VADDR(1:2), VDATA(4), SADDR(2)|OFF  
Description: Store lower VDATA 64-bit word into global address if previous value
from address is equal VDATA>>64, otherwise keep old value from VADDR.
If GLC flag is set then return previous value from VADDR to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM==(VDATA[2:3]) ? VDATA[0:1] : *VM // part of atomic
VDST = (GLC) ? P : VDST // last part of atomic
```

#### GLOBAL_ATOMIC_DEC

Opcode: 76 (0x4c) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_DEC VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Compare value from global address and if less or equal than VDATA
and this value is not zero, then decrement value from global address,
otherwise store VDATA to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA // atomic
VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_DEC_X2

Opcode: 108 (0x6c) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_DEC_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Compare 64-bit value from global address and if less or equal than VDATA
and this value is not zero, then decrement value from global address,
otherwise store VDATA to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA // atomic
VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_INC

Opcode: 75 (0x4b) for GCN 1.4  
Syntax: FLT_ATOMIC_INC VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Compare value from global address and if less than VDATA,
then increment value from address, otherwise store zero to address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = (*VM < VDATA) ? *VM+1 : 0; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_INC_X2

Opcode: 107 (0x9b) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_INC_X2 VDST(2), VADDR(1:2), VADDR(1:2), SADDR(2)|OFF  
Description: Compare 64-bit value from global address and if less than VDATA,
then increment value from address, otherwise store zero to address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = (*VM < VDATA) ? *VM+1 : 0; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_OR

Opcode: 73 (0x49) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_OR VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Do bitwise OR on VDATA and value of global address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM | VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_OR_X2

Opcode: 105 (0x69) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_OR_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Do 64-bit bitwise OR on VDATA and value of global address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM | VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_SMAX

Opcode: 70 (0x46) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_SMAX VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Choose greatest signed 32-bit value from VDATA and from global address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MAX(*VM, (INT32)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_SMAX_X2

Opcode: 102 (0x66) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_SMAX_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Choose greatest signed 64-bit value from VDATA and from global address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
INT64* VM = (INT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MAX(*VM, (INT64)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_SMIN

Opcode: 68 (0x44) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_SMIN VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Choose smallest signed 32-bit value from VDATA and from global address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MIN(*VM, (INT32)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_SMIN_X2

Opcode: 100 (0x64) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_SMIN_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Choose smallest signed 64-bit value from VDATA and from global address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
INT64* VM = (INT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MIN(*VM, (INT64)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_SUB

Opcode: 67 (0x43) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_SUB VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Subtract VDATA from value of global address, and store result to this address.
If GLC flag is set then return previous value from this address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM - VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_SUB_X2

Opcode: 99 (0x63) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_SUB_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Subtract 64-bit VDATA from 64-bit value of global address, and store result
to this address. If GLC flag is set then return previous value from address to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM - VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_SWAP

Opcode: 64 (0x40) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_SWAP VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Store VDATA dword into global address. If GLC flag is set then
return previous value from global address to VDST, otherwise keep old value from VDST.
Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_SWAP_X2

Opcode: 96 (0x60) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_SWAP_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Store VDATA 64-bit word into global address. If GLC flag is set then
return previous value from global address to VDST, otherwise keep old value from VDST.
Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_UMAX

Opcode: 71 (0x47) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_UMAX VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Choose greatest unsigned 32-bit value from VDATA and from global address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MAX(*VM, (UINT32)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_UMAX_X2

Opcode: 103 (0x67) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_UMAX_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Choose greatest unsigned 64-bit value from VDATA and from global address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MAX(*VM, (UINT64)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_UMIN

Opcode: 69 (0x45) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_UMIN VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Choose smallest unsigned 32-bit value from VDATA and from global address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = MIN(*VM, (UINT32)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_UMIN_X2

Opcode: 101 (0x65) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_UMIN_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Choose smallest unsigned 64-bit value from VDATA and from global address,
and store result to this address.
If GLC flag is set then return previous value from this address to VDST, otherwise keep
VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = MIN(*VM, (UINT64)VDATA); VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_XOR

Opcode: 74 (0x4a) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_XOR VDST, VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Do bitwise XOR on VDATA and value of global address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)(VADDR + SADDR + INST_OFFSET)
UINT32 P = *VM; *VM = *VM ^ VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_ATOMIC_XOR_X2

Opcode: 106 (0x6a) for GCN 1.4  
Syntax: GLOBAL_ATOMIC_XOR_X2 VDST(2), VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Do 64-bit bitwise XOR on VDATA and value of global address,
and store result to this address. If GLC flag is set then return previous value
from this address to VDST, otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)(VADDR + SADDR + INST_OFFSET)
UINT64 P = *VM; *VM = *VM ^ VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### GLOBAL_LOAD_DWORD

Opcode: 20 (0x14) for GCN 1.4  
Syntax: GLOBAL_LOAD_DWORD VDST, VADDR(1:2), SADDR(2)|OFF  
Description Load dword to VDST from global address.  
Operation:  
```
VDST = *(UINT32*)(VADDR + SADDR + INST_OFFSET)
```

#### GLOBAL_LOAD_DWORDX2

Opcode: 21 (0x15) for GCN 1.4  
Syntax: GLOBAL_LOAD_DWORDX2 VDST(, VADDR(1:2), SADDR(2)|OFF  
Description Load two dwords to VDST from global address.  
Operation:  
```
VDST = *(UINT64*)(VADDR + SADDR + INST_OFFSET)
```

#### GLOBAL_LOAD_DWORDX3

Opcode: 22 (0x16) for GCN 1.4  
Syntax: GLOBAL_LOAD_DWORDX3 VDST(3), VADDR(1:2), SADDR(2)|OFF  
Description Load three dwords to VDST from global address.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
VDST[0] = *(UINT32*)VM
VDST[1] = *(UINT32*)(VM+4)
VDST[2] = *(UINT32*)(VM+8)
```

#### GLOBAL_LOAD_DWORDX4

Opcode: 23 (0x17) for GCN 1.4  
Syntax: GLOBAL_LOAD_DWORDX4 VDST(4), VADDR(1:2), SADDR(2)|OFF  
Description Load four dwords to VDST from global address.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
VDST[0] = *(UINT32*)VM
VDST[1] = *(UINT32*)(VM+4)
VDST[2] = *(UINT32*)(VM+8)
VDST[3] = *(UINT32*)(VM+12)
```

#### GLOBAL_LOAD_SBYTE

Opcode: 17 (0x11) for GCN 1.4  
Syntax: GLOBAL_LOAD_SBYTE VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load byte to VDST from global address with sign extending.  
Operation:  
```
VDST = *(INT8*)(VADDR + SADDR + INST_OFFSET)
```

#### GLOBAL_LOAD_SBYTE_D16

Opcode: 34 (0x22) for GCN 1.4  
Syntax: GLOBAL_LOAD_SBYTE_D16 VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load byte to lower 16-bit part of VDST from
global address with sign extending.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
VDST = ((UINT16)*(INT8*)VM) | (VDST&0xffff0000)
```

#### GLOBAL_LOAD_SBYTE_D16_HI

Opcode: 35 (0x23) for GCN 1.4  
Syntax: GLOBAL_LOAD_SBYTE_D16_HI VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load byte to higher 16-bit part of VDST from
global address with sign extending.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
VDST = (((UINT32)*(INT8*)VM)<<16) | (VDST&0xffff)
```

#### GLOBAL_LOAD_SHORT_D16

Opcode: 36 (0x24) for GCN 1.4  
Syntax: GLOBAL_LOAD_SHORT_D16 VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load 16-bit word to lower 16-bit part of VDST from global address.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
VDST = *(UINT16*)VM | (VDST & 0xffff0000)
```

#### GLOBAL_LOAD_SHORT_D16_HI

Opcode: 36 (0x24) for GCN 1.4  
Syntax: GLOBAL_LOAD_SHORT_D16_HI VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load 16-bit word to lower 16-bit part of VDST from global address.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
VDST = (((UINT32)*(UINT16*)VM)<<16) | (VDST & 0xffff)
```

#### GLOBAL_LOAD_SSHORT

Opcode: 19 (0x13) for GCN 1.4  
Syntax: GLOBAL_LOAD_SSHORT VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load 16-bit word to VDST from global address with sign extending.  
Operation:  
```
VDST = *(INT16*)(VADDR + SADDR + INST_OFFSET)
```

#### GLOBAL_LOAD_UBYTE

Opcode: 16 (0x10) for GCN 1.4  
Syntax: GLOBAL_LOAD_UBYTE VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load byte to VDST from global address with zero extending.  
Operation:  
```
VDST = *(UINT8*)(VADDR + SADDR + INST_OFFSET)
```

#### GLOBAL_LOAD_UBYTE_D16

Opcode: 32 (0x20) for GCN 1.4  
Syntax: GLOBAL_LOAD_UBYTE_D16 VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load byte to lower 16-bit part of VDST from
global address with zero extending.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
VDST = ((UINT16)*(UINT8*)VM) | (VDST&0xffff0000)
```

#### GLOBAL_LOAD_UBYTE_D16_HI

Opcode: 33 (0x21) for GCN 1.4  
Syntax: GLOBAL_LOAD_UBYTE_D16_HI VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load byte to higher 16-bit part of VDST from
global address with zero extending.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
VDST = (((UINT32)*(UINT8*)VM)<<16) | (VDST&0xffff)
```

#### GLOBAL_LOAD_USHORT

Opcode: 18 (0x12) for GCN 1.4  
Syntax: GLOBAL_LOAD_USHORT VDST, VADDR(1:2), SADDR(2)|OFF  
Description: Load 16-bit word to VDST from global address with zero extending.  
Operation:  
```
VDST = *(UINT16*)(VADDR + SADDR + INST_OFFSET)
```

#### GLOBAL_STORE_BYTE

Opcode: 24 (0x18) for GCN 1.4  
Syntax: GLOBAL_STORE_BYTE VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Store byte from VDATA to global address.  
Operation:  
```
*(UINT8*)(VADDR + SADDR + INST_OFFSET) = VDATA&0xff
```

#### GLOBAL_STORE_BYTE_D16_HI

Opcode: 25 (0x19) for GCN 1.4  
Syntax: GLOBAL_STORE_BYTE_D16_HI VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Store byte from 16-23 bits of VDATA to global address.  
Operation:  
```
*(UINT8*)(VADDR + SADDR + INST_OFFSET) = (VDATA>>16)&0xff
```

#### GLOBAL_STORE_DWORD

Opcode: 28 (0x1c) for GCN 1.4  
Syntax: GLOBAL_STORE_DWORD VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Store dword from VDATA to global address.  
Operation:  
```
*(UINT32*)(VADDR + SADDR + INST_OFFSET) = VDATA
```

#### GLOBAL_STORE_DWORDX2

Opcode: 29 (0x1d) for GCN 1.4  
Syntax: GLOBAL_STORE_DWORDX2 VADDR(1:2), VDATA(2), SADDR(2)|OFF  
Description: Store two dwords from VDATA to global address.  
Operation:  
```
*(UINT64*)(VADDR + SADDR + INST_OFFSET) = VDATA
```

#### GLOBAL_STORE_DWORDX3

Opcode: 30 (0x1e) for GCN 1.4  
Syntax: GLOBAL_STORE_DWORDX3 VADDR(1:2), VDATA(3), SADDR(2)|OFF  
Description: Store three dwords from VDATA to global address.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
*(UINT32*)(VM) = VDATA[0]
*(UINT32*)(VM+4) = VDATA[1]
*(UINT32*)(VM+8) = VDATA[2]
```

#### GLOBAL_STORE_DWORDX4

Opcode: 31 (0x1d) for GCN 1.4  
Syntax: GLOBAL_STORE_DWORDX4 VADDR(1:2), VDATA(4), SADDR(2)|OFF  
Description: Store four dwords from VDATA to global address.  
Operation:  
```
BYTE* VM = (VADDR + SADDR + INST_OFFSET)
*(UINT32*)(VM) = VDATA[0]
*(UINT32*)(VM+4) = VDATA[1]
*(UINT32*)(VM+8) = VDATA[2]
*(UINT32*)(VM+12) = VDATA[3]
```

#### GLOBAL_STORE_SHORT

Opcode: 26 (0x1a) for GCN 1.4  
Syntax: GLOBAL_STORE_SHORT VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Store 16-bit word from VDATA to global address.  
Operation:  
```
*(UINT16*)(VADDR + SADDR + INST_OFFSET) = VDATA&0xffff
```

#### GLOBAL_STORE_SHORT_D16_HI

Opcode: 27 (0x1b) for GCN 1.4  
Syntax: GLOBAL_STORE_SHORT_D16_HI VADDR(1:2), VDATA, SADDR(2)|OFF  
Description: Store 16-bit word from higher 16-bit part of VDATA to global address.  
Operation:  
```
*(UINT16*)(VADDR + SADDR + INST_OFFSET) = VDATA>>16
```

#### SCRATCH_LOAD_DWORD

Opcode: 20 (0x14) for GCN 1.4  
Syntax: SCRATCH_LOAD_DWORD VDST, VADDR|OFF, SADDR|OFF  
Description Load dword to VDST from scratch memory address.  
Operation:  
```
VDST = *(UINT32*)SWIZZLE(ADDR, INST_OFFSET, LANEID)
```

#### SCRATCH_LOAD_DWORDX2

Opcode: 21 (0x15) for GCN 1.4  
Syntax: SCRATCH_LOAD_DWORDX2 VDST(, VADDR|OFF, SADDR|OFF  
Description Load two dwords to VDST from scratch memory address.  
Operation:  
```
VDST = *(UINT64*)SWIZZLE(ADDR, INST_OFFSET, LANEID)
```

#### SCRATCH_LOAD_DWORDX3

Opcode: 22 (0x16) for GCN 1.4  
Syntax: SCRATCH_LOAD_DWORDX3 VDST(3), VADDR|OFF, SADDR|OFF  
Description Load three dwords to VDST from scratch memory address.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
VDST[0] = *(UINT32*)VM
VDST[1] = *(UINT32*)(VM+4)
VDST[2] = *(UINT32*)(VM+8)
```

#### SCRATCH_LOAD_DWORDX4

Opcode: 23 (0x17) for GCN 1.4  
Syntax: SCRATCH_LOAD_DWORDX4 VDST(4), VADDR|OFF, SADDR|OFF  
Description Load four dwords to VDST from scratch memory address.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
VDST[0] = *(UINT32*)VM
VDST[1] = *(UINT32*)(VM+4)
VDST[2] = *(UINT32*)(VM+8)
VDST[3] = *(UINT32*)(VM+12)
```

#### SCRATCH_LOAD_SBYTE

Opcode: 17 (0x11) for GCN 1.4  
Syntax: SCRATCH_LOAD_SBYTE VDST, VADDR|OFF, SADDR|OFF  
Description: Load byte to VDST from scratch memory address with sign extending.  
Operation:  
```
VDST = *(INT8*)SWIZZLE(ADDR, INST_OFFSET, LANEID)
```

#### SCRATCH_LOAD_SBYTE_D16

Opcode: 34 (0x22) for GCN 1.4  
Syntax: SCRATCH_LOAD_SBYTE_D16 VDST, VADDR|OFF, SADDR|OFF  
Description: Load byte to lower 16-bit part of VDST from
scratch memory address with sign extending.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
VDST = ((UINT16)*(INT8*)VM) | (VDST&0xffff0000)
```

#### SCRATCH_LOAD_SBYTE_D16_HI

Opcode: 35 (0x23) for GCN 1.4  
Syntax: SCRATCH_LOAD_SBYTE_D16_HI VDST, VADDR|OFF, SADDR|OFF  
Description: Load byte to higher 16-bit part of VDST from
scratch memory address with sign extending.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
VDST = (((UINT32)*(INT8*)VM)<<16) | (VDST&0xffff)
```

#### SCRATCH_LOAD_SHORT_D16

Opcode: 36 (0x24) for GCN 1.4  
Syntax: SCRATCH_LOAD_SHORT_D16 VDST, VADDR|OFF, SADDR|OFF  
Description: Load 16-bit word to lower 16-bit part of VDST from scratch memory address.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
VDST = *(UINT16*)VM | (VDST & 0xffff0000)
```

#### SCRATCH_LOAD_SHORT_D16_HI

Opcode: 36 (0x24) for GCN 1.4  
Syntax: SCRATCH_LOAD_SHORT_D16_HI VDST, VADDR|OFF, SADDR|OFF  
Description: Load 16-bit word to lower 16-bit part of VDST from scratch memory address.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
VDST = (((UINT32)*(UINT16*)VM)<<16) | (VDST & 0xffff)
```

#### SCRATCH_LOAD_SSHORT

Opcode: 19 (0x13) for GCN 1.4  
Syntax: SCRATCH_LOAD_SSHORT VDST, VADDR|OFF, SADDR|OFF  
Description: Load 16-bit word to VDST from scratch memory address with sign extending.  
Operation:  
```
VDST = *(INT16*)SWIZZLE(ADDR, INST_OFFSET, LANEID)
```

#### SCRATCH_LOAD_UBYTE

Opcode: 16 (0x10) for GCN 1.4  
Syntax: SCRATCH_LOAD_UBYTE VDST, VADDR|OFF, SADDR|OFF  
Description: Load byte to VDST from scratch memory address with zero extending.  
Operation:  
```
VDST = *(UINT8*)SWIZZLE(ADDR, INST_OFFSET, LANEID)
```

#### SCRATCH_LOAD_UBYTE_D16

Opcode: 32 (0x20) for GCN 1.4  
Syntax: SCRATCH_LOAD_UBYTE_D16 VDST, VADDR|OFF, SADDR|OFF  
Description: Load byte to lower 16-bit part of VDST from
scratch memory address with zero extending.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
VDST = ((UINT16)*(UINT8*)VM) | (VDST&0xffff0000)
```

#### SCRATCH_LOAD_UBYTE_D16_HI

Opcode: 33 (0x21) for GCN 1.4  
Syntax: SCRATCH_LOAD_UBYTE_D16_HI VDST, VADDR|OFF, SADDR|OFF  
Description: Load byte to higher 16-bit part of VDST from
scratch memory address with zero extending.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
VDST = (((UINT32)*(UINT8*)VM)<<16) | (VDST&0xffff)
```

#### SCRATCH_LOAD_USHORT

Opcode: 18 (0x12) for GCN 1.4  
Syntax: SCRATCH_LOAD_USHORT VDST, VADDR|OFF, SADDR|OFF  
Description: Load 16-bit word to VDST from scratch memory address with zero extending.  
Operation:  
```
VDST = *(UINT16*)SWIZZLE(ADDR, INST_OFFSET, LANEID)
```

#### SCRATCH_STORE_BYTE

Opcode: 24 (0x18) for GCN 1.4  
Syntax: SCRATCH_STORE_BYTE VADDR|OFF, VDATA, SADDR|OFF  
Description: Store byte from VDATA to scratch memory address.  
Operation:  
```
*(UINT8*)SWIZZLE(ADDR, INST_OFFSET, LANEID) = VDATA&0xff
```

#### SCRATCH_STORE_BYTE_D16_HI

Opcode: 25 (0x19) for GCN 1.4  
Syntax: SCRATCH_STORE_BYTE_D16_HI VADDR|OFF, VDATA, SADDR|OFF  
Description: Store byte from 16-23 bits of VDATA to scratch memory address.  
Operation:  
```
*(UINT8*)SWIZZLE(ADDR, INST_OFFSET, LANEID) = (VDATA>>16)&0xff
```

#### SCRATCH_STORE_DWORD

Opcode: 28 (0x1c) for GCN 1.4  
Syntax: SCRATCH_STORE_DWORD VADDR|OFF, VDATA, SADDR|OFF  
Description: Store dword from VDATA to scratch memory address.  
Operation:  
```
*(UINT32*)SWIZZLE(ADDR, INST_OFFSET, LANEID) = VDATA
```

#### SCRATCH_STORE_DWORDX2

Opcode: 29 (0x1d) for GCN 1.4  
Syntax: SCRATCH_STORE_DWORDX2 VADDR|OFF, VDATA(2), SADDR|OFF  
Description: Store two dwords from VDATA to scratch memory address.  
Operation:  
```
*(UINT64*)SWIZZLE(ADDR, INST_OFFSET, LANEID) = VDATA
```

#### SCRATCH_STORE_DWORDX3

Opcode: 30 (0x1e) for GCN 1.4  
Syntax: SCRATCH_STORE_DWORDX3 VADDR|OFF, VDATA(3), SADDR|OFF  
Description: Store three dwords from VDATA to scratch memory address.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
*(UINT32*)(VM) = VDATA[0]
*(UINT32*)(VM+4) = VDATA[1]
*(UINT32*)(VM+8) = VDATA[2]
```

#### SCRATCH_STORE_DWORDX4

Opcode: 31 (0x1d) for GCN 1.4  
Syntax: SCRATCH_STORE_DWORDX4 VADDR|OFF, VDATA(4), SADDR|OFF  
Description: Store four dwords from VDATA to scratch memory address.  
Operation:  
```
BYTE* VM = SWIZZLE(ADDR, INST_OFFSET, LANEID)
*(UINT32*)(VM) = VDATA[0]
*(UINT32*)(VM+4) = VDATA[1]
*(UINT32*)(VM+8) = VDATA[2]
*(UINT32*)(VM+12) = VDATA[3]
```

#### SCRATCH_STORE_SHORT

Opcode: 26 (0x1a) for GCN 1.4  
Syntax: SCRATCH_STORE_SHORT VADDR|OFF, VDATA, SADDR|OFF  
Description: Store 16-bit word from VDATA to scratch memory address.  
Operation:  
```
*(UINT16*)SWIZZLE(ADDR, INST_OFFSET, LANEID) = VDATA&0xffff
```

#### SCRATCH_STORE_SHORT_D16_HI

Opcode: 27 (0x1b) for GCN 1.4  
Syntax: SCRATCH_STORE_SHORT_D16_HI VADDR|OFF, VDATA, SADDR|OFF  
Description: Store 16-bit word from higher 16-bit part of VDATA to scratch memory address.  
Operation:  
```
*(UINT16*)SWIZZLE(ADDR, INST_OFFSET, LANEID) = VDATA>>16
```
