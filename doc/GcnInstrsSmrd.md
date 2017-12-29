## GCN ISA SMRD instructions

The basic encoding of the SMRD instructions needs 4 bytes (dword). List of fields:

Bits  | Name     | Description
------|----------|------------------------------
0-7   | OFFSET   | Unsigned 8-bit dword offset or SGPR number that holds byte offset
8     | IMM      | IMM indicator
9-14  | SBASE    | Number of aligned SGPR pair.
15-21 | SDST     | Scalar destination operand
22-26 | OPCODE   | Operation code
27-31 | ENCODING | Encoding type. Must be 0b11000

Value of the IMM determines meaning of the OFFSET field:

* IMM=1 - OFFSET holds a dword offset to SBASE.
* IMM=0 - OFFSET holds number of SGPR that holds byte offset to SBASE.

For S_LOAD_DWORD\* instructions, 2 SBASE SGPRs holds a base 64-bit address.
For S_BUFFER_LOAD_DWORD\* instructions, 4 SBASE SGPRs holds a buffer descriptor.
In this case, SBASE must be a multipla of 2.

The SMRD instructions can return the result data out of the order. Any SMRD operation
(including S_MEMTIME) increments LGKM_CNT counter. The best way to wait for results
is `S_WAITCNT LGKMCNT(0)`.

* LGKM_CNT incremented by one for every fetch of single Dword
* LGKM_CNT incremented by two for every fetch of two or more Dwords

NOTE: Between setting third dword from buffer resource and S_BUFFER_* instruction
is required least one instruction (vector or scalar) due to delay.

List of the instructions by opcode:

 Opcode     | Mnemonic (GCN1.0)        | Mnemonic (GCN1.1)
------------|--------------------------|--------------------------
 0 (0x0)    | S_LOAD_DWORD             | S_LOAD_DWORD
 1 (0x1)    | S_LOAD_DWORDX2           | S_LOAD_DWORDX2
 2 (0x2)    | S_LOAD_DWORDX4           | S_LOAD_DWORDX4
 3 (0x3)    | S_LOAD_DWORDX8           | S_LOAD_DWORDX8
 4 (0x4)    | S_LOAD_DWORDX16          | S_LOAD_DWORDX16
 8 (0x8)    | S_BUFFER_LOAD_DWORD      | S_BUFFER_LOAD_DWORD
 9 (0x9)    | S_BUFFER_LOAD_DWORDX2    | S_BUFFER_LOAD_DWORDX2
 10 (0xa)   | S_BUFFER_LOAD_DWORDX4    | S_BUFFER_LOAD_DWORDX4
 11 (0xb)   | S_BUFFER_LOAD_DWORDX8    | S_BUFFER_LOAD_DWORDX8
 12 (0xc)   | S_BUFFER_LOAD_DWORDX16   | S_BUFFER_LOAD_DWORDX16
 29 (0x1d)  | --                       | S_DCACHE_INV_VOL
 30 (0x1e)  | S_MEMTIME                | S_MEMTIME
 31 (0x1f)  | S_DCACHE_INV             | S_DCACHE_INV

### Instruction set

Alphabetically sorted instruction list:

#### S_BUFFER_LOAD_DWORD

Opcode: 8 (0x8)  
Syntax: S_BUFFER_LOAD_DWORD SDST, SBASE(4), OFFSET  
Description: Load single dword from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
SDST = *(UINT32*)(SMRD + (OFFSET & ~3))
```

#### S_BUFFER_LOAD_DWORDX16

Opcode: 12 (0xc)  
Syntax: S_BUFFER_LOAD_DWORDX16 SDST(16), SBASE(4), OFFSET  
Description: Load 16 dwords from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
for (BYTE i = 0; i < 16; i++)
    SDST[i] = *(UINT32*)(SMRD + i*4 + (OFFSET & ~3))
```

#### S_BUFFER_LOAD_DWORDX2

Opcode: 9 (0x9)  
Syntax: S_BUFFER_LOAD_DWORDX2 SDST(2), SBASE(4), OFFSET  
Description: Load two dwords from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
SDST = *(UINT64*)(SMRD + (OFFSET & ~3))
```

#### S_BUFFER_LOAD_DWORDX4

Opcode: 10 (0xa)  
Syntax: S_BUFFER_LOAD_DWORDX4 SDST(4), SBASE(4), OFFSET  
Description: Load four dwords from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    SDST[i] = *(UINT32*)(SMRD + i*4 + (OFFSET & ~3))
```

#### S_BUFFER_LOAD_DWORDX8

Opcode: 11 (0xb)  
Syntax: S_BUFFER_LOAD_DWORDX8 SDST(8), SBASE(4), OFFSET  
Description: Load eight dwords from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
for (BYTE i = 0; i < 8; i++)
    SDST[i] = *(UINT32*)(SMRD + i*4 + (OFFSET & ~3))
```

#### S_DCACHE_INV

Opcode: 31 (0x1f)  
Syntax: S_DCACHE_INV  
Description: Invalidate entire L1 K cache.

#### S_DCACHE_INV_VOL

Opcode: 29 (0x1d) for GCN 1.1  
Syntax: S_DCACHE_INV_VOL  
Description: Invalidate all volatile lines in L1 K cache.

#### S_LOAD_DWORD

Opcode: 0 (0x0)  
Syntax: S_LOAD_DWORD SDST, SBASE(2), OFFSET  
Description: Load single dword from read-only memory through constant cache (kcache).  
Operation:  
```
SDST = *(UINT32*)(SMRD + (OFFSET & ~3))
```

#### S_LOAD_DWORDX16

Opcode: 4 (0x4)  
Syntax: S_LOAD_DWORDX16 SDST(16), SBASE(2), OFFSET  
Description: Load 16 dwords from read-only memory through constant cache (kcache).  
Operation:  
```
for (BYTE i = 0; i < 16; i++)
    SDST[i] = *(UINT32*)(SMRD + i*4 + (OFFSET & ~3))
```

#### S_LOAD_DWORDX2

Opcode: 1 (0x1)  
Syntax: S_LOAD_DWORDX2 SDST(2), SBASE(2), OFFSET  
Description: Load two dwords from read-only memory through constant cache (kcache).  
```
SDST = *(UINT64*)(SMRD + (OFFSET & ~3))
```

#### S_LOAD_DWORDX4

Opcode: 2 (0x2)  
Syntax: S_LOAD_DWORDX4 SDST(4), SBASE(2), OFFSET  
Description: Load four dwords from read-only memory through constant cache (kcache).  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    SDST[i] = *(UINT32*)(SMRD + i*4 + (OFFSET & ~3))
```

#### S_LOAD_DWORDX8

Opcode: 3 (0x3)  
Syntax: S_LOAD_DWORDX8 SDST(8), SBASE(2), OFFSET  
Description: Load eight dwords from read-only memory through constant cache (kcache).  
Operation:  
```
for (BYTE i = 0; i < 8; i++)
    SDST[i] = *(UINT32*)(SMRD + i*4 + (OFFSET & ~3))
```

#### S_MEMTIME

Opcode: 30 (0x1e)  
Syntax: S_MEMTIME SDST(2)  
Description: Store value of 64-bit clock counter to SDST. Before reading result, S_WAITCNT
LGKMCNT(0) is required.  
Operation:  
```
SDST = CLOCKCNT
```
