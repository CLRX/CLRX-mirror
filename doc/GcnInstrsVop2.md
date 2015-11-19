## GCN ISA VOP2/VOP3 instructions

VOP2 instructions can be encoded in the VOP2 encoding and the VOP3a/VOP3b encoding.
List of fields for VOP2 encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-8   | SRC0     | First (scalar or vector) source operand
9-16  | VSRC1    | Second vector source operand
17-24 | VDST     | Destination vector operand
25-30 | OPCODE   | Operation code
31    | ENCODING | Encoding type. Must be 0

Syntax: INSTRUCTION VDST, SRC0, VSRC1

List of fields for VOP3A/VOP3B encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Vector destination operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
11    | CLAMP    | CLAMP modifier (VOP3A)
15    | CLAMP    | CLAMP modifier (VOP3B)
17-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

List of fields for VOP3A encoding (GCN 1.2):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Destination vector operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
15    | CLAMP    | CLAMP modifier
16-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

Syntax: INSTRUCTION VDST, SRC0, SRC1 [MODIFIERS]

Modifiers:

* CLAMP - clamps destination floating point value in range 0.0-1.0
* MUL:2, MUL:4, DIV:2 - OMOD modifiers. Multiply destination floating point value by
2.0, 4.0 or 0.5 respectively
* -SRC - negate floating point value from source operand
* ABS(SRC) - apply absolute value to source operand

Negation and absolute value can be combined: `-ABS(V0)`. Modifiers CLAMP and
OMOD (MUL:2, MUL:4 and DIV:2) can be given in random order.

VOP2 opcodes (0-63) are reflected in VOP3 in range: 256-319.
List of the instructions by opcode:

 Opcode     | Mnemonic (GCN1.0/1.1) | Mnemonic (GCN 1.2)
------------|----------------------|------------------------
 1 (0x1)    | --                   | V_ADD_F32
 2 (0x2)    | --                   | V_SUB_F32
 3 (0x3)    | V_ADD_F32            | V_SUBREV_F32
 4 (0x4)    | V_SUB_F32            | V_MUL_LEGACY_F32
 5 (0x5)    | V_SUBREV_F32         | V_MUL_F32
 6 (0x6)    | V_MAC_LEGACY_F32     | V_MUL_I32_I24
 7 (0x7)    | V_MUL_LEGACY_F32     | V_MUL_HI_I32_I24
 8 (0x8)    | V_MUL_F32            | V_MUL_U32_U24
 9 (0x9)    | V_MUL_I32_I24        | V_MUL_HI_U32_U24
 10 (0xa)   | V_MUL_HI_I32_I24     | V_MIN_F32
 11 (0xb)   | V_MUL_U32_U24        | V_MAX_F32
 12 (0xc)   | V_MUL_HI_U32_U24     | V_MIN_I32
 13 (0xd)   | V_MIN_LEGACY_F32     | V_MAX_I32
 14 (0xe)   | V_MAX_LEGACY_F32     | V_MIN_U32
 15 (0xf)   | V_MIN_F32            | V_MAX_U32

### Instruction set

Alphabetically sorted instruction list:
