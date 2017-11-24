## GCN ISA SMEM instructions (GCN 1.2/1.4)

The encoding of the SMEM instructions needs 8 bytes (2 dwords). List of fields:

Bits  | Name     | Description
------|----------|------------------------------
0-5   | SBASE    | Number of aligned SGPR pair.
6-12  | SDATA    | Scalar destination/data operand
14    | SOE      | Scalar offset enable (GCN 1.4)
15    | NV       | Non-volative (GCN 1.4)
16    | GLC      | Operation globally coherent
17    | IMM      | IMM indicator
18-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110000
32-51 | OFFSET   | Unsigned 20-bit byte offset or SGPR number that holds byte offset
32-52 | OFFSET   | Signed 21-bit byte offset or SGPR number (byte offset) (GCN 1.4)
57-63 | SOFFSET  | SGPR offset (only if SOE=1)

Value of the IMM determines meaning of the OFFSET field (GCN 1.2):

* IMM=1 - OFFSET holds a byte offset to SBASE.
* IMM=0 - OFFSET holds number of SGPR that holds byte offset to SBASE.

Value of the IMM and SOE determines encoding of OFFSET and SGPR offset (GCN 1.4):

 IMM | SOE | Address                            | Syntax
-----|-----|------------------------------------|--------------------
  0  |  0  | SGPR[base] + SGPR[OFFSET]
  0  |  1  | SGPR[base] + SGPR[SOFFSET]
  1  |  0  | SGPR[base] + OFFSET
  1  |  1  | SGPR[base] + OFFSET + SGPR[SOFFSET]

For S_LOAD_DWORD\* instructions, 2 SBASE SGPRs holds a base 64-bit address.
For S_BUFFER_LOAD_DWORD\* instructions, 4 SBASE SGPRs holds a
buffer descriptor. In this case, SBASE must be a multipla of 2.
S_STORE_\* and S_BUFFER_STORE_\* accepts only M0 as offset register for GCN 1.2.
In GCN 1.4 S_STORE_\* and S_BUFFER_STORE_\* accepts also SGPR as offset register.

The SMEM instructions can return the result data out of the order. Any SMEM operation
(including S_MEMTIME) increments LGKM_CNT counter. The best way to wait for results
is `S_WAITCNT LGKMCNT(0)`.

* LGKM_CNT incremented by one for every fetch of single Dword
* LGKM_CNT incremented by two for every fetch of two or more Dwords

Instruction syntax: INSTRUCTION SDATA, SBASE(2,4), OFFSET|SGPR [MODIFIERS]

Modifiers can be supplied in any order. Modifiers list: GLC, NV (GCN 1.4),
OFFSET:OFFSET (GCN 1.4).

NOTE: Between setting third dword from buffer resource and S_BUFFER_\* instruction
is required least one instruction (vector or scalar) due to delay.

List of the instructions by opcode:

 Opcode     |GCN 1.2|GCN 1.4| Mnemonic (GCN1.2/1.4)
------------|-------|-------|------------------------------
 0 (0x0)    |   ✓   |   ✓   | S_LOAD_DWORD
 1 (0x1)    |   ✓   |   ✓   | S_LOAD_DWORDX2
 2 (0x2)    |   ✓   |   ✓   | S_LOAD_DWORDX4
 3 (0x3)    |   ✓   |   ✓   | S_LOAD_DWORDX8
 4 (0x4)    |   ✓   |   ✓   | S_LOAD_DWORDX16
 5 (0x5)    |       |   ✓   | S_SCRATCH_LOAD_DWORD
 6 (0x6)    |       |   ✓   | S_SCRATCH_LOAD_DWORDX2
 7 (0x7)    |       |   ✓   | S_SCRATCH_LOAD_DWORDX4
 8 (0x8)    |   ✓   |   ✓   | S_BUFFER_LOAD_DWORD
 9 (0x9)    |   ✓   |   ✓   | S_BUFFER_LOAD_DWORDX2
 10 (0xa)   |   ✓   |   ✓   | S_BUFFER_LOAD_DWORDX4
 11 (0xb)   |   ✓   |   ✓   | S_BUFFER_LOAD_DWORDX8
 12 (0xc)   |   ✓   |   ✓   | S_BUFFER_LOAD_DWORDX16
 16 (0x10)  |   ✓   |   ✓   | S_STORE_DWORD
 17 (0x11)  |   ✓   |   ✓   | S_STORE_DWORDX2
 18 (0x12)  |   ✓   |   ✓   | S_STORE_DWORDX4
 21 (0x15)  |       |   ✓   | S_SCRATCH_STORE_DWORD
 22 (0x16)  |       |   ✓   | S_SCRATCH_STORE_DWORDX2
 23 (0x17)  |       |   ✓   | S_SCRATCH_STORE_DWORDX4
 24 (0x18)  |   ✓   |   ✓   | S_BUFFER_LOAD_DWORD
 25 (0x19)  |   ✓   |   ✓   | S_BUFFER_LOAD_DWORDX2
 26 (0x1a)  |   ✓   |   ✓   | S_BUFFER_LOAD_DWORDX4
 32 (0x20)  |   ✓   |   ✓   | S_DCACHE_INV
 33 (0x21)  |   ✓   |   ✓   | S_DCACHE_WB
 34 (0x22)  |   ✓   |   ✓   | S_DCACHE_INV_VOL
 35 (0x23)  |   ✓   |   ✓   | S_DCACHE_WB_VOL
 36 (0x24)  |   ✓   |   ✓   | S_MEMTIME
 37 (0x25)  |   ✓   |   ✓   | S_MEMREALTIME
 38 (0x26)  |   ✓   |   ✓   | S_ATC_PROBE
 39 (0x27)  |   ✓   |   ✓   | S_ATC_PROBE_BUFFER
 40 (0x28)  |       |   ✓   | S_DCACHE_DISCARD
 41 (0x29)  |       |   ✓   | S_DCACHE_DISCARD_X2
 64 (0x40)  |       |   ✓   | S_BUFFER_ATOMIC_SWAP
 65 (0x41)  |       |   ✓   | S_BUFFER_ATOMIC_CMPSWAP
 66 (0x42)  |       |   ✓   | S_BUFFER_ATOMIC_ADD
 67 (0x43)  |       |   ✓   | S_BUFFER_ATOMIC_SUB
 68 (0x44)  |       |   ✓   | S_BUFFER_ATOMIC_SMIN
 69 (0x45)  |       |   ✓   | S_BUFFER_ATOMIC_UMIN
 70 (0x46)  |       |   ✓   | S_BUFFER_ATOMIC_SMAX
 71 (0x47)  |       |   ✓   | S_BUFFER_ATOMIC_UMAX
 72 (0x48)  |       |   ✓   | S_BUFFER_ATOMIC_AND
 73 (0x49)  |       |   ✓   | S_BUFFER_ATOMIC_OR
 74 (0x4a)  |       |   ✓   | S_BUFFER_ATOMIC_XOR
 75 (0x4b)  |       |   ✓   | S_BUFFER_ATOMIC_INC
 76 (0x4c)  |       |   ✓   | S_BUFFER_ATOMIC_DEC
 96 (0x60)  |       |   ✓   | S_BUFFER_ATOMIC_SWAP_X2
 97 (0x61)  |       |   ✓   | S_BUFFER_ATOMIC_CMPSWAP_X2
 98 (0x62)  |       |   ✓   | S_BUFFER_ATOMIC_ADD_X2
 99 (0x63)  |       |   ✓   | S_BUFFER_ATOMIC_SUB_X2
 100 (0x64) |       |   ✓   | S_BUFFER_ATOMIC_SMIN_X2
 101 (0x65) |       |   ✓   | S_BUFFER_ATOMIC_UMIN_X2
 102 (0x66) |       |   ✓   | S_BUFFER_ATOMIC_SMAX_X2
 103 (0x67) |       |   ✓   | S_BUFFER_ATOMIC_UMAX_X2
 104 (0x68) |       |   ✓   | S_BUFFER_ATOMIC_AND_X2
 105 (0x69) |       |   ✓   | S_BUFFER_ATOMIC_OR_X2
 106 (0x6a) |       |   ✓   | S_BUFFER_ATOMIC_XOR_X2
 107 (0x6b) |       |   ✓   | S_BUFFER_ATOMIC_INC_X2
 108 (0x6c) |       |   ✓   | S_BUFFER_ATOMIC_DEC_X2
 128 (0x80) |       |   ✓   | S_ATOMIC_SWAP
 129 (0x81) |       |   ✓   | S_ATOMIC_CMPSWAP
 130 (0x82) |       |   ✓   | S_ATOMIC_ADD
 131 (0x83) |       |   ✓   | S_ATOMIC_SUB
 132 (0x84) |       |   ✓   | S_ATOMIC_SMIN
 133 (0x85) |       |   ✓   | S_ATOMIC_UMIN
 134 (0x86) |       |   ✓   | S_ATOMIC_SMAX
 135 (0x87) |       |   ✓   | S_ATOMIC_UMAX
 136 (0x88) |       |   ✓   | S_ATOMIC_AND
 137 (0x89) |       |   ✓   | S_ATOMIC_OR
 138 (0x8a) |       |   ✓   | S_ATOMIC_XOR
 139 (0x8b) |       |   ✓   | S_ATOMIC_INC
 140 (0x8c) |       |   ✓   | S_ATOMIC_DEC
 160 (0xa0) |       |   ✓   | S_ATOMIC_SWAP_X2
 161 (0xa1) |       |   ✓   | S_ATOMIC_CMPSWAP_X2
 162 (0xa2) |       |   ✓   | S_ATOMIC_ADD_X2
 163 (0xa3) |       |   ✓   | S_ATOMIC_SUB_X2
 164 (0xa4) |       |   ✓   | S_ATOMIC_SMIN_X2
 165 (0xa5) |       |   ✓   | S_ATOMIC_UMIN_X2
 166 (0xa6) |       |   ✓   | S_ATOMIC_SMAX_X2
 167 (0xa7) |       |   ✓   | S_ATOMIC_UMAX_X2
 168 (0xa8) |       |   ✓   | S_ATOMIC_AND_X2
 169 (0xa9) |       |   ✓   | S_ATOMIC_OR_X2
 170 (0xaa) |       |   ✓   | S_ATOMIC_XOR_X2
 171 (0xab) |       |   ✓   | S_ATOMIC_INC_X2
 172 (0xac) |       |   ✓   | S_ATOMIC_DEC_X2

### Instruction set

Alphabetically sorted instruction list:

#### S_ATOMIC_ADD

Opcode: 130 (0x82) only for GCN 1.4  
Syntax: S_ATOMIC_ADD SDATA, SBASE(2), OFFSET  
Description: Add SDATA to value from memory address, and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM + SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_ADD_X2

Opcode: 162 (0xa2) only for GCN 1.4  
Syntax: S_ATOMIC_ADD_X2 SDATA(2), SBASE(2), OFFSET  
Description: Add 64-bit SDATA to 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM + SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_AND

Opcode: 136 (0x88) only for GCN 1.4  
Syntax: S_ATOMIC_AND SDATA, SBASE(2), OFFSET  
Description: Do bitwise AND on SDATA and value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM & SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_AND_X2

Opcode: 168 (0xa8) only for GCN 1.4  
Syntax: S_ATOMIC_AND_X2 SDATA(2), SBASE(2), OFFSET  
Description: Do bitwise AND on 64-bit SDATA and 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM & SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_CMPSWAP

Opcode: 129 (0x81) only for GCN 1.4  
Syntax: S_ATOMIC_CMPSWAP SDATA(2), SBASE(2), OFFSET  
Description: Store lower SDATA dword into memory address if previous value
from memory address is equal SDATA>>32, otherwise keep old value from memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM = *VM==(SDATA>>32) ? SDATA&0xffffffff : *VM // atomic
SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_CMPSWAP_X2

Opcode: 161 (0xa1) only for GCN 1.4  
Syntax: S_ATOMIC_CMPSWAP_X2 SDATA(4), SBASE(2), OFFSET  
Description: Store lower SDATA quadword into memory address if previous value
from memory address is equal last SDATA quadword,
otherwise keep old value from memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM = *VM==(SDATA[2:3]) ? SDATA[0:1] : *VM // atomic
SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_DEC

Opcode: 140 (0x8c) only for GCN 1.4  
Syntax: S_ATOMIC_DEC SDATA, SBASE(2), OFFSET  
Description: Compare value from memory address and if less or equal than SDATA
and this value is not zero, then decrement value from memory address,
otherwise store SDATA to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA; // atomic
SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_DEC_X2

Opcode: 172 (0xac) only for GCN 1.4  
Syntax: S_ATOMIC_DEC_X2 SDATA, SBASE(2), OFFSET  
Description: Compare 64-bit value from memory address and if less or equal than
64-bit SDATA and this value is not zero, then decrement value from memory address,
otherwise store SDATA to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA; // atomic
SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_INC

Opcode: 139 (0x8b) only for GCN 1.4  
Syntax: S_ATOMIC_INC SDATA, SBASE(2), OFFSET  
Description: Compare value from memory address and if less than SDATA,
then increment value from memory address, otherwise store zero to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = (*VM < SDATA) ? *VM+1 : 0; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_INC_X2

Opcode: 171 (0xab) only for GCN 1.4  
Syntax: S_ATOMIC_INC_X2 SDATA(2), SBASE(2), OFFSET  
Description: Compare 64-bit value from memory address and if less than 64-bit SDATA,
then increment value from memory address, otherwise store zero to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = (*VM < SDATA) ? *VM+1 : 0; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_OR

Opcode: 137 (0x89) only for GCN 1.4  
Syntax: S_ATOMIC_OR SDATA, SBASE(2), OFFSET  
Description: Do bitwise OR on SDATA and value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM | SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_OR_X2

Opcode: 169 (0xa9) only for GCN 1.4  
Syntax: S_ATOMIC_OR_X2 SDATA(2), SBASE(2), OFFSET  
Description: Do bitwise OR on 64-bit SDATA and 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM | SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_SMAX

Opcode: 134 (0x86) only for GCN 1.4  
Syntax: S_ATOMIC_SMAX SDATA, SBASE(2), OFFSET  
Description: Choose largest signed 32-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)((SMEM + (OFFSET & ~3))
INT32 P = *VM; *VM = MAX(*VM, (INT32)SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_SMAX_X2

Opcode: 166 (0xa6) only for GCN 1.4  
Syntax: S_ATOMIC_SMAX_X2 SDATA(2), SBASE(2), OFFSET  
Description: Choose largest signed 64-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
INT64* VM = (INT64*)((SMEM + (OFFSET & ~3))
INT64 P = *VM; *VM = MAX(*VM, (INT64)SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_SMIN

Opcode: 132 (0x84) only for GCN 1.4  
Syntax: S_ATOMIC_SMIN SDATA, SBASE(2), OFFSET  
Description: Choose smallest signed 32-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
INT32* VM = (INT32*)((SMEM + (OFFSET & ~3))
INT32 P = *VM; *VM = MIN(*VM, (INT32)SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_SMIN_X2

Opcode: 164 (0xa4) only for GCN 1.4  
Syntax: S_ATOMIC_SMIN_X2 SDATA(2), SBASE(2), OFFSET  
Description: Choose smallest signed 64-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
INT64* VM = (INT64*)((SMEM + (OFFSET & ~3))
INT64 P = *VM; *VM = MIN(*VM, (INT64)SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_SUB

Opcode: 131 (0x83) only for GCN 1.4  
Syntax: S_ATOMIC_SUB SDATA, SBASE(2), OFFSET  
Description: Subtract SDATA from value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM - SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_SUB_X2

Opcode: 163 (0xa3) only for GCN 1.4  
Syntax: S_ATOMIC_SUB_X2 SDATA(2), SBASE(2), OFFSET  
Description: Subtract 64-bit SDATA from 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM - SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_SWAP

Opcode: 128 (0x80) only for GCN 1.4  
Syntax: S_ATOMIC_SWAP SDATA, SBASE(2), OFFSET  
Description: Store SDATA into memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_SWAP_X2

Opcode: 160 (0xa0) only for GCN 1.4  
Syntax: S_ATOMIC_SWAP_X2 SDATA(2), SBASE(2), OFFSET  
Description: Store 64-bit SDATA into memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_UMAX

Opcode: 135 (0x87) only for GCN 1.4  
Syntax: S_ATOMIC_UMAX SDATA, SBASE(2), OFFSET  
Description: Choose largest unsigned 32-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = MAX(*VM, SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_UMAX_X2

Opcode: 167 (0xa7) only for GCN 1.4  
Syntax: S_ATOMIC_UMAX_X2 SDATA(2), SBASE(2), OFFSET  
Description: Choose largest unsigned 64-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = MAX(*VM, SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_UMIN

Opcode: 133 (0x85) only for GCN 1.4  
Syntax: S_ATOMIC_UMIN SDATA, SBASE(2), OFFSET  
Description: Choose smallest unsigned 32-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = MIN(*VM, SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_UMIN_X2

Opcode: 165 (0xa5) only for GCN 1.4  
Syntax: S_ATOMIC_UMIN_X2 SDATA(2), SBASE(2), OFFSET  
Description: Choose smallest unsigned 64-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = MIN(*VM, SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_XOR

Opcode: 138 (0x8a) only for GCN 1.4  
Syntax: S_ATOMIC_XOR SDATA, SBASE(2), OFFSET  
Description: Do bitwise XOR on SDATA and value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM ^ SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_ATOMIC_XOR_X2

Opcode: 170 (0xaa) only for GCN 1.4  
Syntax: S_ATOMIC_XOR_X2 SDATA(2), SBASE(2), OFFSET  
Description: Do bitwise XOR on 64-bit SDATA and 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM ^ SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_ADD

Opcode: 66 (0x42) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_ADD SDATA, SBASE(4), OFFSET  
Description: Add SDATA to value from memory address, and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM + SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_ADD_X2

Opcode: 98 (0x62) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_ADD_X2 SDATA(2), SBASE(4), OFFSET  
Description: Add 64-bit SDATA to 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM + SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_AND

Opcode: 72 (0x48) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_AND SDATA, SBASE(4), OFFSET  
Description: Do bitwise AND on SDATA and value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM & SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_AND_X2

Opcode: 104 (0x68) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_AND_X2 SDATA(2), SBASE(4), OFFSET  
Description: Do bitwise AND on 64-bit SDATA and 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM & SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_CMPSWAP

Opcode: 65 (0x41) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_CMPSWAP SDATA(2), SBASE(4), OFFSET  
Description: Store lower SDATA dword into memory address if previous value
from memory address is equal SDATA>>32, otherwise keep old value from memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM = *VM==(SDATA>>32) ? SDATA&0xffffffff : *VM // atomic
SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_CMPSWAP_X2

Opcode: 97 (0x61) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_CMPSWAP_X2 SDATA(4), SBASE(4), OFFSET  
Description: Store lower SDATA quadword into memory address if previous value
from memory address is equal last SDATA quadword,
otherwise keep old value from memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM = *VM==(SDATA[2:3]) ? SDATA[0:1] : *VM // atomic
SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_DEC

Opcode: 76 (0x4c) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_DEC SDATA, SBASE(4), OFFSET  
Description: Compare value from memory address and if less or equal than SDATA
and this value is not zero, then decrement value from memory address,
otherwise store SDATA to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA; // atomic
SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_DEC_X2

Opcode: 108 (0x6c) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_DEC_X2 SDATA, SBASE(4), OFFSET  
Description: Compare 64-bit value from memory address and if less or equal than
64-bit SDATA and this value is not zero, then decrement value from memory address,
otherwise store SDATA to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = (*VM <= VDATA && *VM!=0) ? *VM-1 : VDATA; // atomic
SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_INC

Opcode: 75 (0x4b) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_INC SDATA, SBASE(4), OFFSET  
Description: Compare value from memory address and if less than SDATA,
then increment value from memory address, otherwise store zero to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = (*VM < SDATA) ? *VM+1 : 0; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_INC_X2

Opcode: 107 (0x6b) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_INC_X2 SDATA(2), SBASE(4), OFFSET  
Description: Compare 64-bit value from memory address and if less than 64-bit SDATA,
then increment value from memory address, otherwise store zero to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = (*VM < SDATA) ? *VM+1 : 0; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_OR

Opcode: 73 (0x49) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_OR SDATA, SBASE(4), OFFSET  
Description: Do bitwise OR on SDATA and value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM | SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_OR_X2

Opcode: 105 (0x69) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_OR_X2 SDATA(2), SBASE(4), OFFSET  
Description: Do bitwise OR on 64-bit SDATA and 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM | SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_SMAX

Opcode: 72 (0x46) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_SMAX SDATA, SBASE(4), OFFSET  
Description: Choose largest signed 32-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
INT32* VM = (INT32*)((SMEM + (OFFSET & ~3))
INT32 P = *VM; *VM = MAX(*VM, (INT32)SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_SMAX_X2

Opcode: 102 (0x66) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_SMAX_X2 SDATA(2), SBASE(4), OFFSET  
Description: Choose largest signed 64-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
INT64* VM = (INT64*)((SMEM + (OFFSET & ~3))
INT64 P = *VM; *VM = MAX(*VM, (INT64)SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_SMIN

Opcode: 70 (0x44) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_SMIN SDATA, SBASE(4), OFFSET  
Description: Choose smallest signed 32-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
INT32* VM = (INT32*)((SMEM + (OFFSET & ~3))
INT32 P = *VM; *VM = MIN(*VM, (INT32)SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_SMIN_X2

Opcode: 100 (0x64) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_SMIN_X2 SDATA(2), SBASE(4), OFFSET  
Description: Choose smallest signed 64-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
INT64* VM = (INT64*)((SMEM + (OFFSET & ~3))
INT64 P = *VM; *VM = MIN(*VM, (INT64)SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_SUB

Opcode: 69 (0x43) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_SUB SDATA, SBASE(4), OFFSET  
Description: Subtract SDATA from value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM - SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_SUB_X2

Opcode: 99 (0x63) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_SUB_X2 SDATA(2), SBASE(4), OFFSET  
Description: Subtract 64-bit SDATA from 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM - SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_SWAP

Opcode: 64 (0x40) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_SWAP SDATA, SBASE(4), OFFSET  
Description: Store SDATA into memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_SWAP_X2

Opcode: 96 (0x60) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_SWAP_X2 SDATA(2), SBASE(4), OFFSET  
Description: Store 64-bit SDATA into memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_UMAX

Opcode: 71 (0x47) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_UMAX SDATA, SBASE(4), OFFSET  
Description: Choose largest unsigned 32-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = MAX(*VM, SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_UMAX_X2

Opcode: 103 (0x67) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_UMAX_X2 SDATA(2), SBASE(4), OFFSET  
Description: Choose largest unsigned 64-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = MAX(*VM, SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_UMIN

Opcode: 69 (0x45) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_UMIN SDATA, SBASE(4), OFFSET  
Description: Choose smallest unsigned 32-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = MIN(*VM, SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_UMIN_X2

Opcode: 101 (0x65) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_UMIN_X2 SDATA(2), SBASE(4), OFFSET  
Description: Choose smallest unsigned 64-bit value from SDATA and from memory address,
and store result to this memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = MIN(*VM, SDATA); SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_XOR

Opcode: 74 (0x4a) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_XOR SDATA, SBASE(4), OFFSET  
Description: Do bitwise XOR on SDATA and value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT32* VM = (UINT32*)((SMEM + (OFFSET & ~3))
UINT32 P = *VM; *VM = *VM ^ SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

#### S_BUFFER_ATOMIC_XOR_X2

Opcode: 106 (0x6a) only for GCN 1.4  
Syntax: S_BUFFER_ATOMIC_XOR_X2 SDATA(2), SBASE(4), OFFSET  
Description: Do bitwise XOR on 64-bit SDATA and 64-bit value from memory address,
and store result to memory address.
If GLC flag is set then return previous value from memory address to SDATA,
otherwise keep SDATA value. Operation is atomic. SBASE is buffer descriptor.  
Operation:  
```
UINT64* VM = (UINT64*)((SMEM + (OFFSET & ~3))
UINT64 P = *VM; *VM = *VM ^ SDATA; SDATA = (GLC) ? P : SDATA // atomic
```

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

#### S_DCACHE_DISCARD

Opcode 40 (0x28) only for GCN 1.4  
Syntax: S_DCACHE_DISCARD SBASE(2), SOFFSET1  
Description: Discard one dirty scalar data cache line. A cache line is 64
bytes. Address calculated as S_STORE_DWORD with alignment to 64-byte boundary.
LGKM count is incremented by 1 for this opcode.

#### S_DCACHE_DISCARD_X2

Opcode 41 (0x29) only for GCN 1.4  
Syntax: S_DCACHE_DISCARD_X2 SBASE(2), SOFFSET1  
Description: Discard two dirty scalar data cache lines. A cache line is 64
bytes. Address calculated as S_STORE_DWORD with alignment to 64-byte boundary.
LGKM count is incremented by 1 for this opcode.

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

#### S_SCRATCH_LOAD_DWORD

Opcode: 5 (0x5) only for GCN 1.4  
Syntax: S_SCRATCH_LOAD_DWORD SDATA, SBASE(2), SGPROFFSET OFFSET:OFFSET  
Description: Load single dword from read-only memory through constant cache (kcache).  
Operation:  
```
SDATA = *(UINT32*)(SMEM + (OFFSET & ~3) + (SGPROFFSET & ~3)*64)
```

#### S_SCRATCH_LOAD_DWORDX2

Opcode: 6 (0x6) only for GCN 1.4  
Syntax: S_SCRATCH_LOAD_DWORDX2 SDATA, SBASE(2), SGPROFFSET OFFSET:OFFSET  
Description: Load two dwords from read-only memory through constant cache (kcache).  
Operation:  
```
SDATA = *(UINT64*)(SMEM + (OFFSET & ~3) + (SGPROFFSET & ~3)*64)
```

#### S_SCRATCH_LOAD_DWORDX4

Opcode: 7 (0x7) only for GCN 1.4  
Syntax: S_SCRATCH_LOAD_DWORDX4 SDATA, SBASE(2), SGPROFFSET OFFSET:OFFSET  
Description: Load four dwords from read-only memory through constant cache (kcache).  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    SDATA[i] = *(UINT32*)(SMEM + i*4 + (OFFSET & ~3) + (SGPROFFSET & ~3)*64)
```

#### S_SCRATCH_STORE_DWORD

Opcode: 21 (0x15) only for GCN 1.4  
Syntax: S_SCRATCH_STORE_DWORD SDATA, SBASE(2), SGPROFFSET OFFSET:OFFSET  
Description: Store single dword to memory.  
Operation:  
```
*(UINT32*)(SMEM + (OFFSET & ~3) + (SGPROFFSET & ~3)*64) = SDATA
```

#### S_SCRATCH_STORE_DWORDX2

Opcode: 22 (0x16) only for GCN 1.4  
Syntax: S_SCRATCH_STORE_DWORDX2 SDATA(2), SBASE(2), SGPROFFSET OFFSET:OFFSET  
Description: Store two dwords to memory.  
Operation:  
```
*(UINT64*)(SMEM + (OFFSET & ~3) + (SGPROFFSET & ~3)*64) = SDATA
```

#### S_SCRATCH_STORE_DWORDX4

Opcode: 23 (0x17) only for GCN 1.4  
Syntax: S_SCRATCH_STORE_DWORDX4 SDATA(4), SBASE(2), SGPROFFSET OFFSET:OFFSET  
Description: Store four dwords to memory.  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    *(UINT32*)(SMEM + i*4 + (OFFSET & ~3) + (SGPROFFSET & ~3)*64) = SDATA[i]
```

#### S_STORE_DWORD

Opcode: 16 (0x10)  
Syntax: S_STORE_DWORD SDATA, SBASE(2), OFFSET  
Description: Store single dword to memory.
It accepts only offset as M0 or any immediate (only GCN 1.2).  
Operation:  
```
*(UINT32*)(SMEM + (OFFSET & ~3)) = SDATA
```

#### S_STORE_DWORDX2

Opcode: 17 (0x11)  
Syntax: S_STORE_DWORDX2 SDATA(2), SBASE(2), OFFSET  
Description: Store two dwords to memory.
It accepts only offset as M0 or any immediate (only GCN 1.2).  
Operation:  
```
*(UINT64*)(SMEM + (OFFSET & ~3)) = SDATA
```

#### S_STORE_DWORDX4

Opcode: 18 (0x12)  
Syntax: S_STORE_DWORDX4 SDATA(4), SBASE(2), OFFSET  
Description: Store four dwords to memory.
It accepts only offset as M0 or any immediate (only GCN 1.2).  
Operation:  
```
for (BYTE i = 0; i < 4; i++)
    *(UINT32*)(SMEM + i*4 + (OFFSET & ~3)) = SDATA[i]
```
