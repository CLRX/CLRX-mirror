## GCN ISA MTBUF instructions

These instructions allow to access to images. MIMG instructions
operates on the image resources and on the sampler resources.

List of fields for the MIMG encoding:

Bits  | Name     | Description
------|----------|------------------------------
8-11  | DMASK    | Enable mask for read/write data components
12    | UNORM    | Accepts unnormalized coordinates
13    | GLC      | Globally coherent
14    | DA       | Data array (required by array of 1D and 2D images)
15    | R128     | Image resource size = 128
16    | TFE      | Texture Fail Enable (for partially resident textures).
17    | LWE      | LOD Warning Enable (for partially resident textures).
18-24 | OPCODE   | Operation code
25    | SLC      | System level coherent
26-31 | ENCODING | Encoding type. Must be 0b111100
32-39 | VADDR    | Vector address operands
40-47 | VDATA    | Vector data registers
48-52 | SRSRC    | Scalar registers with buffer resource (SGPR# is 4*value)
53-57 | SSAMP    | Scalar registers with sampler resource (SGPR# is 4*value)
63    | D16      | Convert 32-bit data to 16-bit data (GCN 1.2)

Instruction syntax: INSTRUCTION VDATA, VADDR, SRSRC [MODIFIERS]  
Instruction syntax: INSTRUCTION VDATA, VADDR, SRSRC, SSAMP [MODIFIERS]

Modifiers can be supplied in any order. Modifiers list: SLC, GLC, TFE, LWE, DA, R128.
The TFE flag requires additional the VDATA register.

The MIMG instructions is executed in order. Any MIMG instruction increments VMCNT and
it decrements VMCNT after memory operation. Any memory-write operation increments EXPCNT,
and it decrements EXPCNT after reading data from VDATA.

### Instructions by opcode

List of the MIMG instructions by opcode (GCN 1.0/1.1):

 Opcode    |GCN 1.0|GCN 1.1| Mnemonic
-----------|-------|-------|---------------------------
 0 (0x0)   |   ✓   |   ✓   | IMAGE_LOAD
 1 (0x1)   |   ✓   |   ✓   | IMAGE_LOAD_MIP
 2 (0x2)   |   ✓   |   ✓   | IMAGE_LOAD_PCK
 3 (0x3)   |   ✓   |   ✓   | IMAGE_LOAD_PCK_SGN
 4 (0x4)   |   ✓   |   ✓   | IMAGE_LOAD_MIP_PCK
 5 (0x5)   |   ✓   |   ✓   | IMAGE_LOAD_MIP_PCK_SGN
 8 (0x8)   |   ✓   |   ✓   | IMAGE_STORE
 9 (0x9)   |   ✓   |   ✓   | IMAGE_STORE_MIP
 10 (0xa)  |   ✓   |   ✓   | IMAGE_STORE_PCK
 11 (0xb)  |   ✓   |   ✓   | IMAGE_STORE_MIP_PCK
 14 (0xe)  |   ✓   |   ✓   | IMAGE_GET_RESINFO

 