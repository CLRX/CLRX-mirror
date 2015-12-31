## GCN ISA MUBUF instructions

These instructions allow to access to main video memory. MUBUF instructions
operates on the buffer resources. The buffer resources are 4 dwords which holds the
base address, buffer size, their structure and format of their data.

List of fields for the MUBUF encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-11  | OFFSET   | Unsigned byte offset
12    | OFFEN    | If set, send additional offset from VADDR
13    | IDXEN    | If set, send index from VADDR
14    | GLC      | Operation globally coherent
15    | ADDR64   | If set, address is 64-bit
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

The buffer resource format:

Bits   | Name       | Description
-------|------------|------------------------------
0-47   | BASE       | Base address
48-61  | STRIDE     | Stride in bytes. Size of records
62     | Cache swizzle |  Buffer access. Optionally, swizzle texture cache TC L1 cache banks
63     | Swizzle enable | If set, enable swizzle addressing mode
64-95  | NUMRECORDS | Number of records (size of the buffer)
96-98  | DST_SEL_X  | Select destination component for X
99-101 | DST_SEL_Y  | Select destination component for Y
102-104 | DST_SEL_Z  | Select destination component for Z
105-107 | DST_SEL_W  | Select destination component for W
108-110 | NUMFORMAT  | Number format
111-114 | DATAFORMAT | Data format
115-116 | ELEMSIZE   | Element size (used only by swizzle mode)
117-118 | INDEXSTRIDE | Index stride (used only by swizzle mode)
119    | TID_ENABLE | Add thread id to index
121    | Hash enable | Enable address hashing 
122    | HEAP       | Buffer is heap
126-127 | TYPE      | Resource type. 0 - for buffer

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

### Buffer addressing

The address depends on couple factors which are from instruction and the buffer resource.
The buffer is organized in simple structure that contains records. Buffer address starts
from the base offset given in buffer resource. The index indicates number of the record,
and the offset indicates position in record. Expression that describes address:

```
BUFOFFSET = (UINT32)(AINDEX*STRIDE) + AOFFSET
```

Optionally, buffer can be addressed in the swizzle mode that is very effective manner to
addressing the scratch buffers that holds spilled registers. Expression that describes
swizzle mode addressing:

```
AINDEX_MSB = AINDEX / INDEXSTRIDE
AINDEX_LSB = AINDEX % INDEXSTRIDE
AOFFSET_MSB = AOFFSET / ELEMSIZE
AOFFSET_LSB = AOFFSET % ELEMSIZE
BUFOFFSET = AOFFSET_LSB + ELEMSIZE*AINDEX_LSB + INDEXSTRIDE * (AINDEX_MSB*STRIDE + \
        AOFFSET_MSB * ELEMSIZE)
```

The 64-bit addressing can be enabled by set ADDR64 flag in instruction. In this case,
two VADDR registers contains an address. Expression to calculate address in this case:

```
ADDRESS = BASE + VGPR_ADDRESS + OFFSET + SGPR_OFFSET // 64-bit addressing
```

No range checking for 64-bit mode addressing.

The base address are calculated as sum of BASE and SGPR offset. The instruction format
supply OFFEN and IDXEN that includes index from VPGR registers. These flags are permitted
only if 64-bit addressing is not enabled. Table describes combination of these flags:

 IDXEN |OFFEN | Indexreg | Offset reg 
-------|------|----------|------------------------
   0   |  0   | N/A      | N/A
   1   |  0   | VGPR[V]  | N/A
   0   |  1   | N/A      | VGPR[V]
   1   |  1   | VGPR[V]  | VGPR[V+1]

Expressions that describes offsets and indices:

```
UINT32 AOFFSET = OFFSET + (OFFEN ? VGPR_OFFSET : 0)
UINT32 AINDEX = (IDXEN ? VGPR_INDEX : 0) + (TID_ENABLE ? LANEID : 0)
```

The hardware checks range for buffer resources with STRIDE=0 in following way:
if BUFOFFSET >= NUMRECORDS-SGPR_OFFSET then an address is out of range.
For STRIDE!=0 if AINDEX >= NUMRECORDS or OFFSET >= STRIDE when IDXEN or
TID_ENABLE is set, then an address is out of range. Reads are zero and writes are ignored
for an addresses out of range.

For 32-bit operations, an address are aligned to 4 bytes.

The coalescing works for STRIDE==0 on offset (hardware looks at offset), otherwise works
if stride<=1 or swizzle mode enabled and all offsets are equal and ELEMSIZE have same value
as size of element that can be operated by instruction. Then hardware coalesce across any
set of contiguous indices for raw buffers. For swizzled buffers, it
cannot coalesce across INDEXSTRIDE boundaries.

### Instruction set

Alphabetically sorted instruction list:

