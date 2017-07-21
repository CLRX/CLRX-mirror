## GCN ISA SMEM instructions (GCN 1.2)

The encoding of the SMEM instructions needs 8 bytes (2 dwords). List of fields:

Bits  | Name     | Description
------|----------|------------------------------
0-5   | SBASE    | Number of aligned SGPR pair.
6-12  | SDATA    | Scalar destination/data operand
16    | GLC      | Operation globally coherent
17    | IMM      | IMM indicator
18-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110000
32-51 | OFFSET   | Unsigned 20-bit byte offset or SGPR number that holds byte offset

Value of the IMM determines meaning of the OFFSET field:

* IMM=1 - OFFSET holds a byte offset to SBASE.
* IMM=0 - OFFSET holds number of SGPR that holds byte offset to SBASE.

For S_LOAD_DWORD\* instructions, 2 SBASE SGPRs holds an base 48-bit address and a
16-bit size. For S_BUFFER_LOAD_DWORD\* instructions, 4 SBASE SGPRs holds a
buffer descriptor. In this case, SBASE must be a multipla of 2.
S_STORE_\* and S_BUFFER_STORE_\* accepts only M0 as offset register.

The SMEM instructions can return the result data out of the order. Any SMEM operation
(including S_MEMTIME) increments LGKM_CNT counter. The best way to wait for results
is `S_WAITCNT LGKMCNT(0)`.

* LGKM_CNT incremented by one for every fetch of single Dword
* LGKM_CNT incremented by two for every fetch of two or more Dwords

NOTE: Between setting third dword from buffer resource and S_BUFFER_* instruction
is required least one instruction (vector or scalar) due to delay.

List of the instructions by opcode:

 Opcode     | Mnemonic (GCN1.2)
------------|--------------------------
 0 (0x0)    | S_LOAD_DWORD
 1 (0x1)    | S_LOAD_DWORDX2
 2 (0x2)    | S_LOAD_DWORDX4
 3 (0x3)    | S_LOAD_DWORDX8
 4 (0x4)    | S_LOAD_DWORDX16
 8 (0x8)    | S_BUFFER_LOAD_DWORD
 9 (0x9)    | S_BUFFER_LOAD_DWORDX2
 10 (0xa)   | S_BUFFER_LOAD_DWORDX4
 11 (0xb)   | S_BUFFER_LOAD_DWORDX8
 12 (0xc)   | S_BUFFER_LOAD_DWORDX16
 16 (0x10)  | S_STORE_DWORD
 17 (0x11)  | S_STORE_DWORDX2
 18 (0x12)  | S_STORE_DWORDX4
 24 (0x18)  | S_BUFFER_LOAD_DWORD
 25 (0x19)  | S_BUFFER_LOAD_DWORDX2
 27 (0x1a)  | S_BUFFER_LOAD_DWORDX4
 32 (0x20)  | S_DCACHE_INV
 33 (0x21)  | S_DCACHE_WB
 34 (0x22)  | S_DCACHE_INV_VOL
 35 (0x23)  | S_DCACHE_WB_VOL
 36 (0x24)  | S_MEMTIME
 37 (0x25)  | S_MEMREALTIME
 38 (0x26)  | S_ATC_PROBE
 39 (0x27)  | S_ATC_PROBE_BUFFER

### Instruction set

Alphabetically sorted instruction list:

#### S_BUFFER_LOAD_DWORD

Opcode: 8 (0x8)  
Syntax: S_BUFFER_LOAD_DWORD SDATA, SBASE(4), OFFSET  
Description: Load single dword from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
SDATA = *(UINT32*)(SMEM + (OFFSET & ~3))
```

#### S_BUFFER_LOAD_DWORDX16

Opcode: 12 (0xc)  
Syntax: S_BUFFER_LOAD_DWORDX16 SDATA(16), SBASE(4), OFFSET  
Description: Load 16 dwords from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
for (BYTE i = 0; i < 16; i++)
    SDATA[i] = *(UINT32*)(SMEM + i*4 + (OFFSET & ~3))
```

#### S_BUFFER_LOAD_DWORDX2

Opcode: 9 (0x9)  
Syntax: S_BUFFER_LOAD_DWORDX2 SDATA(2), SBASE(4), OFFSET  
Description: Load two dwords from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
SDATA = *(UINT64*)(SMEM + (OFFSET & ~3))
```

#### S_BUFFER_LOAD_DWORDX4

Opcode: 10 (0xa)  
Syntax: S_BUFFER_LOAD_DWORDX4 SDATA(4), SBASE(4), OFFSET  
Description: Load four dwords from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    SDATA[i] = *(UINT32*)(SMEM + i*4 + (OFFSET & ~3))
```

#### S_BUFFER_LOAD_DWORDX8

Opcode: 11 (0xb)  
Syntax: S_BUFFER_LOAD_DWORDX8 SDATA(8), SBASE(4), OFFSET  
Description: Load eight dwords from read-only memory through constant cache (kcache).
SBASE is buffer descriptor.  
Operation:  
```
for (BYTE i = 0; i < 8; i++)
    SDATA[i] = *(UINT32*)(SMEM + i*4 + (OFFSET & ~3))
```

#### S_BUFFER_STORE_DWORD

Opcode: 24 (0x18)  
Syntax: S_BUFFER_STORE_DWORD SDATA, SBASE(4), OFFSET  
Description: Store single dword to memory. It accepts only offset as M0 or any immediate.
SBASE is buffer descriptor.  
Operation:  
```
*(UINT32*)(SMEM + (OFFSET & ~3)) = SDATA
```

#### S_BUFFER_STORE_DWORDX2

Opcode: 25 (0x19)  
Syntax: S_BUFFER_STORE_DWORDX2 SDATA(2), SBASE(4), OFFSET  
Description: Store two dwords to memory. It accepts only offset as M0 or any immediate.
SBASE is buffer descriptor.  
Operation:  
```
*(UINT64*)(SMEM + (OFFSET & ~3)) = SDATA
```

#### S_BUFFER_STORE_DWORDX4

Opcode: 26 (0x1a)  
Syntax: S_BUFFER_STORE_DWORDX4 SDATA(4), SBASE(4), OFFSET  
Description: Store four dwords to memory. It accepts only offset as M0 or any immediate.
SBASE is buffer descriptor.  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    *(UINT32*)(SMEM + i*4 + (OFFSET & ~3)) = SDATA[i]
```

#### S_DCACHE_INV

Opcode: 32 (0x20)  
Syntax: S_DCACHE_INV  
Description: Invalidate entire L1 K cache.

#### S_DCACHE_INV_VOL

Opcode: 34 (0x22)  
Syntax: S_DCACHE_INV_VOL  
Description: Invalidate all volatile lines in L1 K cache.


#### S_LOAD_DWORD

Opcode: 0 (0x0)  
Syntax: S_LOAD_DWORD SDATA, SBASE(2), OFFSET  
Description: Load single dword from read-only memory through constant cache (kcache).  
Operation:  
```
SDATA = *(UINT32*)(SMEM + (OFFSET & ~3))
```

#### S_LOAD_DWORDX16

Opcode: 4 (0x4)  
Syntax: S_LOAD_DWORDX16 SDATA(16), SBASE(2), OFFSET  
Description: Load 16 dwords from read-only memory through constant cache (kcache).  
Operation:  
```
for (BYTE i = 0; i < 16; i++)
    SDATA[i] = *(UINT32*)(SMEM + i*4 + (OFFSET & ~3))
```

#### S_LOAD_DWORDX2

Opcode: 1 (0x1)  
Syntax: S_LOAD_DWORDX2 SDATA(2), SBASE(2), OFFSET  
Description: Load two dwords from read-only memory through constant cache (kcache).  
```
SDATA = *(UINT64*)(SMEM + (OFFSET & ~3))
```

#### S_LOAD_DWORDX4

Opcode: 2 (0x2)  
Syntax: S_LOAD_DWORDX4 SDATA(4), SBASE(2), OFFSET  
Description: Load four dwords from read-only memory through constant cache (kcache).  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    SDATA[i] = *(UINT32*)(SMEM + i*4 + (OFFSET & ~3))
```

#### S_LOAD_DWORDX8

Opcode: 3 (0x3)  
Syntax: S_LOAD_DWORDX8 SDATA(8), SBASE(2), OFFSET  
Description: Load eight dwords from read-only memory through constant cache (kcache).  
Operation:  
```
for (BYTE i = 0; i < 8; i++)
    SDATA[i] = *(UINT32*)(SMEM + i*4 + (OFFSET & ~3))
```

#### S_MEMREALTIME

Opcode: 37 (0x25)  
Syntax: S_MEMREALTIME SDATA(2)  
Description: Store value of 64-bit RTC counter to SDATA.
Before reading result, S_WAITCNT LGKMCNT(0) is required.  
Operation:  
```
SDATA = CLOCKCNT
```

#### S_MEMTIME

Opcode: 36 (0x24)  
Syntax: S_MEMTIME SDATA(2)  
Description: Store value of 64-bit clock counter to SDATA.
This "time" is a free-running clock counter based on the shader core clock.
Before reading result, S_WAITCNT LGKMCNT(0) is required.  
Operation:  
```
SDATA = CLOCKCNT
```

#### S_STORE_DWORD

Opcode: 16 (0x10)  
Syntax: S_STORE_DWORD SDATA, SBASE(2), OFFSET  
Description: Store single dword to memory. It accepts only offset as M0 or any immediate.  
SBASE is buffer descriptor.  
Operation:  
```
*(UINT32*)(SMEM + (OFFSET & ~3)) = SDATA
```

#### S_STORE_DWORDX2

Opcode: 17 (0x11)  
Syntax: S_STORE_DWORDX2 SDATA(2), SBASE(2), OFFSET  
Description: Store two dwords to memory. It accepts only offset as M0 or any immediate.  
Operation:  
```
*(UINT64*)(SMEM + (OFFSET & ~3)) = SDATA
```

#### S_STORE_DWORDX4

Opcode: 18 (0x12)  
Syntax: S_STORE_DWORDX4 SDATA(4), SBASE(2), OFFSET  
Description: Store four dwords to memory. It accepts only offset as M0 or any immediate.  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    *(UINT32*)(SMEM + i*4 + (OFFSET & ~3)) = SDATA[i]
```
