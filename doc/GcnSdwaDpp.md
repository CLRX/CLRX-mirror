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

Operation code:  
```
// SRC0_SRC = source SRC0, SRC1_SRC = source SRC1, DST_SRC = VDST source
// SRC0_DST = dest. SRC0, SRC1_DST = dest. SRC1, DST_DST = VDST dest.
// OPERATION(SRC0, SRC1) - instruction operation, VDST - VDST register before instruction
if (HAVE_SRC0)
{
    switch(SRC0_SEL)
    {
        case SDWA_BYTE_0:
            SRC0_DST = (SRC0_SEXT) ? INT32(INT8(SRC0_SRC & 0xff)) : SRC0_SRC & 0xff
            break;
        case SDWA_BYTE_1:
            SRC0_DST = (SRC0_SEXT) ? INT32(INT8((SRC0_SRC>>8) & 0xff)) :
                        (SRC0_SRC>>8) & 0xff
            break;
        case SDWA_BYTE_2:
            SRC0_DST = (SRC0_SEXT) ? INT32(INT8((SRC0_SRC>>16) & 0xff)) :
                        (SRC0_SRC>>16) & 0xff
            break;
        case SDWA_BYTE_1:
            SRC0_DST = (SRC0_SEXT) ? INT32(INT8(SRC0_SRC>>24)) : SRC0_SRC>>24
            break;
        case SDWA_WORD_0:
            SRC0_DST = (SRC0_SEXT) ? INT32(INT16(SRC0_SRC & 0xffff)) : SRC0_SRC & 0xffff
            break;
        case SDWA_WORD_1:
            SRC0_DST = (SRC0_SEXT) ? INT32(INT16(SRC0_SRC >> 16)) : SRC0_SRC >> 16
            break;
        case SDWA_DWORD:
            SRC0_DST = SRC0_SRC
            break;
    }
}
if (HAVE_SRC1)
{
    switch(SRC1_SEL)
    {
        case SDWA_BYTE_0:
            SRC1_DST = (SRC1_SEXT) ? INT32(INT8(SRC1_SRC & 0xff)) : SRC1_SRC & 0xff
            break;
        case SDWA_BYTE_1:
            SRC1_DST = (SRC1_SEXT) ? INT32(INT8((SRC1_SRC>>8) & 0xff)) :
                        (SRC1_SRC>>8) & 0xff
            break;
        case SDWA_BYTE_2:
            SRC1_DST = (SRC1_SEXT) ? INT32(INT8((SRC1_SRC>>16) & 0xff)) :
                        (SRC1_SRC>>16) & 0xff
            break;
        case SDWA_BYTE_1:
            SRC1_DST = (SRC1_SEXT) ? INT32(INT8(SRC1_SRC>>24)) : SRC1_SRC>>24
            break;
        case SDWA_WORD_0:
            SRC1_DST = (SRC1_SEXT) ? INT32(INT16(SRC1_SRC & 0xffff)) : SRC1_SRC & 0xffff
            break;
        case SDWA_WORD_1:
            SRC1_DST = (SRC1_SEXT) ? INT32(INT16(SRC1_SRC >> 16)) : SRC1_SRC >> 16
            break;
        case SDWA_DWORD:
            SRC1_DST = SRC1_SRC
            break;
    }
}
DST_SRC = OPERATION(SRC0,SRC1)
UNT32 tmp
switch(DST_SEL)
{
    case SDWA_BYTE_0:
        tmp = DST_SRC & 0xff
        if (DST_UNUSED==SDWA_UNUSED_PAD)
            DST_DST = tmp
        else if (DST_UNUSED==SDWA_UNUSED_SEXT)
            DST_DST = INT32(INT8(tmp))
        else if (DST_UNUSED==SDWA_UNUSED_PRESERVE)
            DST_DST = tmp | (VDST & 0xffffff00)
        break;
    case SDWA_BYTE_1:
        tmp = DST_SRC & 0xff
        if (DST_UNUSED==SDWA_UNUSED_PAD)
            DST_DST = tmp << 8
        else if (DST_UNUSED==SDWA_UNUSED_SEXT)
            DST_DST = INT32(INT8(tmp)) << 8
        else if (DST_UNUSED==SDWA_UNUSED_PRESERVE)
            DST_DST = (tmp<<8) | (VDST & 0xffff00ff)
        break;
    case SDWA_BYTE_2:
        tmp = DST_SRC & 0xff
        if (DST_UNUSED==SDWA_UNUSED_PAD)
            DST_DST = tmp << 16
        else if (DST_UNUSED==SDWA_UNUSED_SEXT)
            DST_DST = INT32(INT8(tmp)) << 16
        else if (DST_UNUSED==SDWA_UNUSED_PRESERVE)
            DST_DST = (tmp<<16) | (VDST & 0xff00ffff)
        break;
    case SDWA_BYTE_3:
        tmp = DST_SRC & 0xff
        if (DST_UNUSED==SDWA_UNUSED_PAD)
            DST_DST = tmp << 24
        else if (DST_UNUSED==SDWA_UNUSED_SEXT)
            DST_DST = INT32(INT8(tmp)) << 24
        else if (DST_UNUSED==SDWA_UNUSED_PRESERVE)
            DST_DST = (tmp<<24) | (VDST & 0x00ffffff)
        break;
    case SDWA_WORD_0:
        tmp = DST_SRC & 0xffff
        if (DST_UNUSED==SDWA_UNUSED_PAD)
            DST_DST = tmp
        else if (DST_UNUSED==SDWA_UNUSED_SEXT)
            DST_DST = INT32(INT16(tmp))
        else if (DST_UNUSED==SDWA_UNUSED_PRESERVE)
            DST_DST = tmp | (VDST & 0xffff0000)
        break;
    case SDWA_WORD_1:
        tmp = DST_SRC & 0xffff
        if (DST_UNUSED==SDWA_UNUSED_PAD)
            DST_DST = tmp << 16
        else if (DST_UNUSED==SDWA_UNUSED_SEXT)
            DST_DST = INT32(INT16(tmp)) << 16
        else if (DST_UNUSED==SDWA_UNUSED_PRESERVE)
            DST_DST = (tmp<<16) | (VDST & 0xffff)
        break;
    case SDWA_DWORD:
        DST_DST = DST_SRC
        break;
}
```
