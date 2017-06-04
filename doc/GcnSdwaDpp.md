## GCN 1.2 VOP_SDWA and VOP_DPP encodings

The GCN 1.2 architecture provides two new VOP encoding enhancements which adds new features.
The VOP_SDWA adds possibility to choosing and interesting bytes, words in operands.
The VOP_DPP adds new parallel features to manipulating wavefronts (moving data between
threads).

### VOP_SDWA

The VOP_SDWA encoding is enabled by setting 0xf9 in VSRC0 field in VOP1/VOP2/VOPC encoding.
List of fields:

Bits  | Name       | Description
------|------------|------------------------------
0-7   | SRC0       | First source vector operand
8-10  | DST_SEL    | Destination selection
11-12 | DST_UNUSED | Format for unused bits
13    | CLAMP      | CLAMP modifier
16-18 | SRC0_SEL   | Source data selection for SRC0
19    | SRC0_SEXT  | Signed extension for SRC0
20    | SRC0_NEG   | Negation modifier for SRC0
21    | SRC0_ABS   | Absolute value for SRC0
24-26 | SRC0_SEL   | Source data selectio for SRC1
27    | SRC1_SEXT  | Signed extension for SRC1
28    | SRC1_NEG   | Negation modifier for SRC1
29    | SRC1_ABS   | Absolute value for SRC1

The DST_SEL modifier determines which part of dword will hold first part (with same length)
of destination dword will be placed. This make operation `(RESULT & PARTMASK) << PARTSHIFT`.
The SRC0_SEL and SRC1_SEL determines which part of source dword will be placed to first
part of this source dword. This make operation `(SOURCE>>PARTSHIFT) & PARTMASK`.
Possible part selection for DST_SEL, SRC0_SEL and SRC1_SEL:

Value | Name        | CLRX names         | Description
------|-------------|--------------------|----------------------
 0    | SDWA_BYTE_0 | BYTE0, BYTE_0, B0  | Byte 0 of dword
 1    | SDWA_BYTE_1 | BYTE1, BYTE_1, B1  | Byte 1 of dword
 2    | SDWA_BYTE_2 | BYTE2, BYTE_2, B2  | Byte 2 of dword
 3    | SDWA_BYTE_3 | BYTE3, BYTE_3, B3  | Byte 3 of dword
 4    | SDWA_WORD_0 | WORD0, WORD_0, W0  | Word 0 of dword
 5    | SDWA_WORD_1 | WORD1, WORD_1, W1  | Word 1 of dword
 6    | SDWA_DWORD  | DWORD, DW          | Whole dword

The DST_UNUSED modifier specify how to fill bits that do not hold by choosen part.
Following options:

Value | Name                 | CLRX name      | Descrption
------|----------------------|----------------|--------------------------------------
 0    | SDWA_UNUSED_PAD      | PAD            | Fill by zeroes
 1    | SDWA_UNUSED_SEXT     | SEXT           | Fill by last bit of parts (sign), zero pad lower bits
 2    | SDWA_UNUSED_PRESERVE | PRESERVE       | Preserve unused bits

The modifier SEXT (applied by using `sext(operand)`) apply sign extension
(fill bits after part) to source operand while for source operand was not
selected whole dword (SDWA_DWORD not choosen).
