## GCN ISA FLAT instructions

These instructions allow to access to main memory, LDS and VGPRS registers.
FLAT instructions fetch address from 2 vector registers that hold 64-bit address.
FLAT instruction presents only in GCN 1.1 architecture.

List of fields for the FLAT encoding (GCN 1.1):

Bits  | Name     | Description
------|----------|------------------------------
16    | GLC      | Operation globally coherent
17    | SLC      | System level coherent
18-24 | OPCODE   | Operation code
25-31 | ENCODING | Encoding type. Must be 0b110111
32-39 | VADDR     | Vector address registers
40-47 | VDATA    | Vector data register
55    | TFE      | Texture Fail Enable ???
56-63 | VDST     | Vector destination register

Instruction syntax: INSTRUCTION VDST, VADDR(2) [MODIFIERS]  
Instruction syntax: INSTRUCTION VADDR(2), VDATA [MODIFIERS]

Modifiers can be supplied in any order. Modifiers list: SLC, GLC, TFE.
The TFE flag requires additional the VDATA register.

FLAT instruction can complete out of order with each other. This can be caused by different
resources from/to that instruction can load/store. FLAT instruction increase VMCNT if access
to main memory, or LKGMCNT if accesses to LDS.

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
 52 (0x34)  | FLAT_ATOMIC_SUB        | --
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
 84 (0x54)  | FLAT_ATOMIC_SUB_X2     | --
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

### Instruction set

Alphabetically sorted instruction list:

#### BUFFER_ATOMIC_CMPSWAP

Opcode: 49 (0x31) for GCN 1.0/1.1; 65 (0x41) for GCN 1.2  
Syntax: FLAT_ATOMIC_CMPSWAP VDST, VADDR(2), VDATA(2)  
Description: Store lower VDATA dword into VADDR address  if previous value
from that address is equal VDATA>>32, otherwise keep old value from address.
If GLC flag is set then return previous value from resource to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VADDR
UINT32 P = *VM; *VM = *VM==(VDATA>>32) ? VDATA&0xffffffff : *VM // part of atomic
VDST = (GLC) ? P : VDST // last part of atomic
```

#### BUFFER_ATOMIC_CMPSWAP_X2

Opcode: 81 (0x51) for GCN 1.0/1.1; 97 (0x61) for GCN 1.2  
Syntax: FLAT_ATOMIC_CMPSWAP_X2 VDST(2), VADDR(2), VDATA(4)  
Description: Store lower VDATA 64-bit word into VADDR address if previous value
from resource is equal VDATA>>64, otherwise keep old value from VADDR.
If GLC flag is set then return previous value from VADDR to VDST,
otherwise keep VDST value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VADDR
UINT64 P = *VM; *VM = *VM==(VDATA[2:3]) ? VDATA[0:1] : *VM // part of atomic
VDST = (GLC) ? P : VDST // last part of atomic
```

#### FLAT_ATOMIC_SWAP

Opcode: 48 (0x30) for GCN 1.0/1.1; 64 (0x40) for GCN 1.2  
Syntax: FLAT_ATOMIC_SWAP VDST, VADDR(2), VDATA
Description: Store VDATA dword into VADDR address. If GLC flag is set then
return previous value from VADDR address to VDST, otherwise keep old value from VDST.
Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)VADDR
UINT32 P = *VM; *VM = VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_ATOMIC_SWAP_X2

Opcode: 80 (0x50) for GCN 1.0/1.1; 96 (0x60) for GCN 1.2  
Syntax: FLAT_ATOMIC_SWAP_X2 VDST(2), VADDR(2), VDATA(2)
Description: Store VDATA 64-bit word into VADDR address. If GLC flag is set then
return previous value from VADDR address to VDST, otherwise keep old value from VDST.
Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)VADDR
UINT64 P = *VM; *VM = VDATA; VDST = (GLC) ? P : VDST // atomic
```

#### FLAT_LOAD_DWORD

Opcode: 12 (0xc) for GCN 1.1; 20 (0x14) for GCN 1.2  
Syntax: FLAT_LOAD_DWORD VDST, VADDR(2)  
Description Load dword to VDST from VADDR address.  
Operation:  
```
VDST = *(UINT32*)VADDR
```

#### FLAT_LOAD_DWORDX2

Opcode: 13 (0xd) for GCN 1.1; 21 (0x15) for GCN 1.2  
Syntax: FLAT_LOAD_DWORDX2 VDST(, VADDR(2)  
Description Load two dwords to VDST from VADDR address.  
Operation:  
```
VDST = *(UINT64*)VADDR
```

#### FLAT_LOAD_DWORDX3

Opcode: 15 (0xf) for GCN 1.1; 22 (0x16) for GCN 1.2  
Syntax: FLAT_LOAD_DWORDX3 VDST(3), VADDR(2)  
Description Load three dwords to VDST from VADDR address.  
Operation:  
```
VDST[0] = *(UINT32*)VADDR
VDST[1] = *(UINT32*)(VADDR+4)
VDST[2] = *(UINT32*)(VADDR+8)
```

#### FLAT_LOAD_DWORDX4

Opcode: 13 (0xe) for GCN 1.1; 23 (0x17) for GCN 1.2  
Syntax: FLAT_LOAD_DWORDX4 VDST(4), VADDR(2)  
Description Load four dwords to VDST from VADDR address.  
Operation:  
```
VDST[0] = *(UINT32*)VADDR
VDST[1] = *(UINT32*)(VADDR+4)
VDST[2] = *(UINT32*)(VADDR+8)
VDST[3] = *(UINT32*)(VADDR+12)
```

#### FLAT_LOAD_SBYTE

Opcode: 9 (0x9) for GCN 1.1; 17 (0x11) for GCN 1.2  
Syntax: FLAT_LOAD_SBYTE VDST, VADDR(2)  
Description: Load byte to VDST from VADDR address with sign extending.  
Operation:  
```
VDST = *(INT8*)VADDR
```

#### FLAT_LOAD_SSHORT

Opcode: 11 (0xb) for GCN 1.1; 19 (0x13) for GCN 1.2  
Syntax: FLAT_LOAD_SSHORT VDST, VADDR(2)  
Description: Load 16-bit word to VDST from VADDR address with sign extending.  
Operation:  
```
VDST = *(INT16*)VADDR
```

#### FLAT_LOAD_UBYTE

Opcode: 8 (0x8) for GCN 1.1; 16 (0x10) for GCN 1.2  
Syntax: FLAT_LOAD_UBYTE VDST, VADDR(2)  
Description: Load byte to VDST from VADDR address with zero extending.  
Operation:  
```
VDST = *(UINT8*)VADDR
```

#### FLAT_LOAD_USHORT

Opcode: 10 (0xa) for GCN 1.1; 18 (0x12) for GCN 1.2  
Syntax: FLAT_LOAD_USHORT VDST, VADDR(1:2)  
Description: Load 16-bit word to VDST from VADDR address with zero extending.  
Operation:  
```
VDST = *(UINT16*)VADDR
```

#### FLAT_STORE_BYTE

Opcode: 24 (0x18)  
Syntax: FLAT_STORE_BYTE VADDR(2), VDATA  
Description: Store byte from VDATA to VADDR address.  
Operation:  
```
*(UINT8*)VADDR = VDATA&0xff
```

#### FLAT_STORE_DWORD

Opcode: 28 (0x1c)  
Syntax: FLAT_STORE_DWORD VADDR(2), VDATA  
Description: Store dword from VDATA to VADDR address.  
Operation:  
```
*(UINT32*)VADDR = VDATA
```

#### FLAT_STORE_DWORDX2

Opcode: 29 (0x1d)  
Syntax: FLAT_STORE_DWORDX2 VADDR(2), VDATA(2)  
Description: Store two dwords from VDATA to VADDR address.  
Operation:  
```
*(UINT64*)VADDR = VDATA
```

#### FLAT_STORE_DWORDX3

Opcode: 31 (0x1f) for GCN 1.1; 30 (0x1e) for GCN 1.2  
Syntax: FLAT_STORE_DWORDX3 VADDR(2), VDATA(3)  
Description: Store three dwords from VDATA to VADDR address.  
Operation:  
```
*(UINT32*)(VADDR) = VDATA[0]
*(UINT32*)(VADDR+4) = VDATA[1]
*(UINT32*)(VADDR+8) = VDATA[2]
```

#### FLAT_STORE_DWORDX4

Opcode: 30 (0x1e) for GCN 1.1; 31 (0x1d) for GCN 1.2  
Syntax: FLAT_STORE_DWORDX4 VADDR(2), VDATA(4)  
Description: Store three dwords from VDATA to VADDR address.  
Operation:  
```
*(UINT32*)(VADDR) = VDATA[0]
*(UINT32*)(VADDR+4) = VDATA[1]
*(UINT32*)(VADDR+8) = VDATA[2]
*(UINT32*)(VADDR+12) = VDATA[3]
```

#### FLAT_STORE_SHORT

Opcode: 26 (0x1a)  
Syntax: FLAT_STORE_SHORT VADDR(2), VDATA  
Description: Store 16-bit word from VDATA to VADDR address.  
Operation:  
```
*(UINT16*)VADDR = VDATA&0xffff
```
