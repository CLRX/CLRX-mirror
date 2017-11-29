## GCN ISA MTBUF instructions

These instructions allow to access to main memory. MTBUF instructions
operates on the buffer resources. The buffer resources are 4 dwords which holds the
base address, buffer size, their structure and format of their data.
The buffer data format are encoding in instructions (instructions are typed).

List of fields for the MTBUF encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-11  | OFFSET   | Unsigned byte offset
12    | OFFEN    | If set, send additional offset from VADDR
13    | IDXEN    | If set, send index from VADDR
14    | GLC      | Operation globally coherent
15    | ADDR64   | If set, address is 64-bit (VADDR is 64-bit)
16-18 | OPCODE   | Operation code
19-22 | DFMT     | Data format
23-25 | NFMT     | Number format
26-31 | ENCODING | Encoding type. Must be 0b111010
32-39 | VADDR    | Vector address registers
40-47 | VDATA    | Vector data registers
48-52 | SRSRC    | Scalar registers with buffer resource (SGPR# is 4*value)
54    | SLC      | System level coherent
55    | TFE      | Texture Fail Enable ???
56-63 | SOFFSET  | Scalar base offset operand

List of fields for the MTBUF encoding (GCN 1.2/1.4):

Bits  | Name     | Description
------|----------|------------------------------
0-11  | OFFSET   | Unsigned byte offset
12    | OFFEN    | If set, send additional offset from VADDR
13    | IDXEN    | If set, send index from VADDR
14    | GLC      | Operation globally coherent
15-18 | OPCODE   | Operation code
19-22 | DFMT     | Data format
23-25 | NFMT     | Number format
26-31 | ENCODING | Encoding type. Must be 0b111010
32-39 | VADDR    | Vector address registers
40-47 | VDATA    | Vector data registers
48-52 | SRSRC    | Scalar registers with buffer resource (SGPR# is 4*value)
54    | SLC      | System level coherent
55    | TFE      | Texture Fail Enable ???
56-63 | SOFFSET  | Scalar base offset operand

Instruction syntax: INSTRUCTION VDATA, [VADDR(1:2),] SRSRC(4), SOFFSET [MODIFIERS]

Modifiers can be supplied in any order. Modifiers list:
OFFEN, IDXEN, SLC, GLC, TFE, ADDR64, LDS, OFFSET:OFFSET, FORMAT:[DFMT,NFMT].
The TFE flag requires additional the VDATA register. IDXEN and OFFEN both enabled
requires 64-bit VADDR. VADDR is optional if no IDXEN, OFFEN and ADDR64.

The MTBUF instructions is executed in order. Any MTBUF instruction increments VMCNT and
it decrements VMCNT after memory operation. Any memory-write operation increments EXPCNT,
and it decrements EXPCNT after reading data from VDATA.

### Instructions by opcode

List of the MTBUF instructions by opcode:

 Opcode     |GCN 1.0|GCN 1.1|GCN 1.2| Mnemonic
------------|-------|-------|-------|---------------------------
 0 (0x0)    |   ✓   |   ✓   |   ✓   | TBUFFER_LOAD_FORMAT_X
 1 (0x1)    |   ✓   |   ✓   |   ✓   | TBUFFER_LOAD_FORMAT_XY
 2 (0x2)    |   ✓   |   ✓   |   ✓   | TBUFFER_LOAD_FORMAT_XYZ
 3 (0x3)    |   ✓   |   ✓   |   ✓   | TBUFFER_LOAD_FORMAT_XYZW
 4 (0x4)    |   ✓   |   ✓   |   ✓   | TBUFFER_STORE_FORMAT_X
 5 (0x5)    |   ✓   |   ✓   |   ✓   | TBUFFER_STORE_FORMAT_XY
 6 (0x6)    |   ✓   |   ✓   |   ✓   | TBUFFER_STORE_FORMAT_XYZ
 7 (0x7)    |   ✓   |   ✓   |   ✓   | TBUFFER_STORE_FORMAT_XYZW
 8 (0x8)    |       |       |   ✓   | TBUFFER_LOAD_FORMAT_D16_X
 9 (0x9)    |       |       |   ✓   | TBUFFER_LOAD_FORMAT_D16_XY
 10 (0xa)   |       |       |   ✓   | TBUFFER_LOAD_FORMAT_D16_XYZ
 11 (0xb)   |       |       |   ✓   | TBUFFER_LOAD_FORMAT_D16_XYZW
 12 (0xc)   |       |       |   ✓   | TBUFFER_STORE_FORMAT_D16_X
 13 (0xd)   |       |       |   ✓   | TBUFFER_STORE_FORMAT_D16_XY
 14 (0xe)   |       |       |   ✓   | TBUFFER_STORE_FORMAT_D16_XYZ
 15 (0xf)   |       |       |   ✓   | TBUFFER_STORE_FORMAT_D16_XYZW

### Details

Informations about addressing and format conversion are here:
[Main memory handling](GcnMemHandling)

### Instruction set

Alphabetically sorted instruction list:

#### TBUFFER_LOAD_FORMAT_D16_X

Opcode: 8 (0x8) for GCN 1.2  
Syntax: TBUFFER_LOAD_FORMAT_D16_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first component of the element from SRSRC including format from
instruction fields. Store result as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT)
```

#### TBUFFER_LOAD_FORMAT_D16_XY

Opcode: 9 (0x9) for GCN 1.2  
Syntax: TBUFFER_LOAD_FORMAT_D16_XY VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): TBUFFER_LOAD_FORMAT_D16_XY VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the two first components of the element from SRSRC including format from
instruction fields. Store results as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_XY(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT)
```

#### TBUFFER_LOAD_FORMAT_D16_XYZ

Opcode: 10 (0xa) for GCN 1.2  
Syntax: TBUFFER_LOAD_FORMAT_XYZ VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): TBUFFER_LOAD_FORMAT_XYZ VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the three first components of the element from SRSRC including format
from instruction fields. Store results as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_XYZ(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT)
```

#### TBUFFER_LOAD_FORMAT_D16_XYZW

Opcode: 11 (0xb) for GCN 1.2  
Syntax: TBUFFER_LOAD_FORMAT_D16_XYZW VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): TBUFFER_LOAD_FORMAT_D16_XYZW VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load four components of the element from SRSRC including format
from instruction fields. Store results as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
VDATA = LOAD_FORMAT_D16_XYZW(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT)
```

#### TBUFFER_LOAD_FORMAT_X

Opcode: 0 (0x0)  
Syntax: TBUFFER_LOAD_FORMAT_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the first component of the element from SRSRC including format from
instruction fields.  
Operation:  
```
VDATA = LOAD_FORMAT_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT)
```

#### TBUFFER_LOAD_FORMAT_XY

Opcode: 1 (0x1)  
Syntax: TBUFFER_LOAD_FORMAT_XY VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the two first components of the element from SRSRC including format from
instruction fields.  
Operation:  
```
VDATA = LOAD_FORMAT_XY(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT)
```

#### TBUFFER_LOAD_FORMAT_XYZ

Opcode: 2 (0x2)  
Syntax: TBUFFER_LOAD_FORMAT_XYZ VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load the three first components of the element from SRSRC including format
from instruction fields.  
Operation:  
```
VDATA = LOAD_FORMAT_XYZ(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT)
```

#### TBUFFER_LOAD_FORMAT_XYZW

Opcode: 3 (0x3)  
Syntax: TBUFFER_LOAD_FORMAT_XYZW VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Load four components of the element from SRSRC including format
from instruction fields.  
Operation:  
```
VDATA = LOAD_FORMAT_XYZW(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT)
```

#### TBUFFER_STORE_FORMAT_D16_X

Opcode: 12 (0xc) for GCN 1.2  
Syntax: TBUFFER_STORE_FORMAT_D16_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first component of the element into SRSRC resource
including format from instruction fields.
Treat input as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT, VDATA)
```

#### TBUFFER_STORE_FORMAT_D16_XY

Opcode: 13 (0xd) for GCN 1.2  
Syntax: TBUFFER_STORE_FORMAT_D16_XY VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): TBUFFER_STORE_FORMAT_D16_XY VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first two components of the element into SRSRC resource
including format from instruction fields.
Treat input as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_XY(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT, VDATA)
```

#### TBUFFER_STORE_FORMAT_D16_XYZ

Opcode: 14 (0xe) for GCN 1.2  
Syntax: TBUFFER_STORE_FORMAT_D16_XYZ VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): TBUFFER_STORE_FORMAT_D16_XYZ VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first three components of the element into SRSRC resource
including format from instruction fields.
Treat input as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_XYZ(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT, VDATA)
```

#### TBUFFER_STORE_FORMAT_D16_XYZW

Opcode: 15 (0xf) for GCN 1.2  
Syntax: TBUFFER_STORE_FORMAT_D16_XYZW VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Syntax (GCN 1.4): TBUFFER_STORE_FORMAT_D16_XYZW VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the all components of the element into SRSRC resource
including format from instruction fields.
Treat input as 16-bit value (half FP or 16-bit integer).  
Operation:  
```
STORE_FORMAT_D16_XYZW(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT, VDATA)
```

#### TBUFFER_STORE_FORMAT_X

Opcode: 4 (0x4)  
Syntax: TBUFFER_STORE_FORMAT_X VDATA, VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first component of the element into SRSRC resource
including format from instruction fields.  
Operation:  
```
STORE_FORMAT_X(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT, VDATA)
```

#### TBUFFER_STORE_FORMAT_XY

Opcode: 5 (0x5)  
Syntax: TBUFFER_STORE_FORMAT_XY VDATA(2), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first two components of the element into SRSRC resource
including format from instruction fields.  
Operation:  
```
STORE_FORMAT_XY(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT, VDATA)
```

#### TBUFFER_STORE_FORMAT_XYZ

Opcode: 6 (0x6)  
Syntax: TBUFFER_STORE_FORMAT_XYZ VDATA(3), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the first three components of the element into SRSRC resource
including format from instruction fields.  
Operation:  
```
STORE_FORMAT_XYZ(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT, VDATA)
```

#### TBUFFER_STORE_FORMAT_XYZW

Opcode: 7 (0x7)  
Syntax: TBUFFER_STORE_FORMAT_XYZW VDATA(4), VADDR(1:2), SRSRC(4), SOFFSET  
Description: Store the all components of the element into SRSRC resource
including format from instruction fields.  
Operation:  
```
STORE_FORMAT_XYZW(SRSRC, VADDR(1:2), SOFFSET, OFFSET, DFMT, NFMT, VDATA)
```
