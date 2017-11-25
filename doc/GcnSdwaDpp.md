## GCN 1.2 VOP_SDWA and VOP_DPP encodings

The GCN 1.2 architecture provides two new VOP encoding enhancements which adds new features.
The VOP_SDWA adds possibility to choosing and interesting bytes, words in operands.
The VOP_DPP adds new parallel features to manipulating wavefronts (moving data between
threads).

### VOP_SDWA

The VOP_SDWA encoding is enabled by setting 0xf9 in VSRC0 field in VOP1/VOP2/VOPC encoding.
In an assembler's syntax you can force this encoding by using `SDWA` modifier in instruction.
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
of destination dword. This make operation `(RESULT & PARTMASK) << PARTSHIFT`.
The SRC0_SEL and SRC1_SEL determines which part of source dword will be placed to first
part of this source dword. This make operation `(SOURCE>>PARTSHIFT) & PARTMASK`.
Possible part selection for DST_SEL, SRC0_SEL and SRC1_SEL:

Value | Name        | Assembler names    | Description
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

Value | Name                 | Assembler name | Descrption
------|----------------------|----------------|--------------------------------------
 0    | SDWA_UNUSED_PAD      | PAD            | Fill by zeroes
 1    | SDWA_UNUSED_SEXT     | SEXT           | Fill by last bit of parts (sign), zero pad lower bits
 2    | SDWA_UNUSED_PRESERVE | PRESERVE       | Preserve unused bits

The modifier SEXT (applied by using `sext(operand)`) apply sign extension
(fill bits after part) to source operand while for source operand was not
selected whole dword (SDWA_DWORD not choosen).

Examples:  
```
v_xor_b32 v1,v2,v3 dst_sel:byte_1 src0_sel:byte1 src1_sel:word1
v_xor_b32 v1,v2,v3 dst_sel:b1 src0_sel:b1 src1_sel:w1
v_xor_b32 v1,v2,v3 dst_sel:byte_1 src0_sel:byte1 src1_sel:word1 dst_unused:preserve
v_xor_b32 v1,v2,v3 dst_sel:byte_1 src0_sel:byte1 src1_sel:word1 dst_unused:sext
v_xor_b32 v1,sext(v2),v3 dst_sel:byte_1 src0_sel:byte1 src1_sel:word1
```

Operation code:  
```
// SRC0_SRC = source SRC0, SRC1_SRC = source SRC1, DST_SRC = VDST source
// SRC0_DST = dest. SRC0, SRC1_DST = dest. SRC1, DST_DST = VDST dest.
// OPERATION(SRC0, SRC1) - instruction operation, VDST - VDST register before instruction
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
    case SDWA_BYTE_3:
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
        case SDWA_BYTE_3:
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
DST_SRC = OPERATION(SRC0_DST,SRC1_DST)
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

### VOP_DPP

The VOP_DPP encoding is enabled by setting 0xfa in SRC0 field in VOP1/VOP2/VOPC encoding.
In an assembler's syntax you can force this encoding by using `DPP` modifier in instruction.
List of fields:

Bits  | Name       | Description
------|------------|------------------------------
0-7   | SRC0       | First source vector operand
8-16  | DPP_CTRL   | Data parallel primitive control
19    | BOUND_CTRL | Specifies behaviour when shared data is invalid
20    | SRC0_NEG   | Negation modifier for SRC0
21    | SRC0_ABS   | Absolute value for SRC0
22    | SRC1_NEG   | Negation modifier for SRC1
23    | SRC1_ABS   | Absolute value for SRC1
24-27 | BANK_MASK  | Bank enable mask
28-31 | ROW_MASK   | Row enable mask

The operation on wavefronts applied to SRC0 operand in VOP instruction.
The wavefront contains 4 rows (1 row - 16 threads), and each row contains 4 banks
(1 bank - 4 threads). The DPP_CTRL choose which operation will be applied to SRC0.
List of data parallel operations:

Value        | Name                 | Modifier | Description
-------------|----------------------|----------|-------------------
0x00-0xff    | DPP_QUAD_PERM{00:ff} | quad_perm:[A,B,C,D]  | Full permute of 4 threads
0x101-0x10f  | DPP_ROW_SL{1:15}     | row_shl:N | Row shift left by N threads
0x111-0x11f  | DPP_ROW_SR{1:15}     | row_shr:N | Row shift right by N threads
0x121-0x12f  | DPP_ROW_RR{1:15}     | row_ror:N | Row rotate right by N threads
0x130        | DPP_WF_SL1           | wave_shl:1 | Wave shift left by 1 thread
0x134        | DPP_WF_RL1           | wave_rol:1 | Wave rotate left by 1 thread
0x138        | DPP_WF_SR1           | wave_shr:1 | Wave shift right by 1 thread
0x13c        | DPP_WF_RR1           | wave_ror:1 | Wave rotate right by 1 thread
0x140        | DPP_ROW_MIRROR       | row_mirror | Mirror threads within row
0x141        | DPP_ROW_HALF_MIRROR  | row_half_mirror | Mirror threads within half row
0x142        | DPP_ROW_BCAST15      | row_bcast:15 | Broadcast 15 thread of each row to next row
0x143        | DPP_ROW_BCAST31      | row_bcast:31 | Broadcast 31 thread to row 2 and row 3

The BOUND_CTRL flag (modifier `bound_ctrl` or `bound_ctrl:0`) control how to fill invalid
threads (for example that last threads after left shifting). Zero value (no modifier)
do not perform operation in thread that source threads are invalid.
One value (with modifier) fills invalid threads by 0 value.

The field BANK_MASK (modifier `bank_mask:value`) choose which banks will be enabled during
data parallel operation in each enabled row. The Nth bit represents Nth bank in each row.
Threads in disabled banks do not perform operation.

The field ROW_MASK (modifier `row_mask:value`) choose which rows will be enabled during
data parallel operation. The Nth bit represents Nth row.
Threads in disabled rows do not perform operation.

Examples:  
```
v_xor_b32 v1,v2,v3 quad_perm:[2,3,0,1]
v_xor_b32 v1,v2,v3 row_shl:5
v_xor_b32 v1,v2,v3 row_shr:7
v_xor_b32 v1,v2,v3 row_ror:8
v_xor_b32 v1,v2,v3 wave_shl:1
v_xor_b32 v1,v2,v3 wave_shl
v_xor_b32 v1,v2,v3 wave_shr:1
v_xor_b32 v1,v2,v3 wave_shr
v_xor_b32 v1,v2,v3 wave_rol:1
v_xor_b32 v1,v2,v3 wave_rol
v_xor_b32 v1,v2,v3 wave_ror:1
v_xor_b32 v1,v2,v3 wave_ror
v_xor_b32 v1,v2,v3 row_mirror
v_xor_b32 v1,v2,v3 row_half_mirror
v_xor_b32 v1,v2,v3 row_bcast:15
v_xor_b32 v1,v2,v3 row_bcast:31
v_xor_b32 v1,v2,v3 row_shr:7 bound_ctrl
v_xor_b32 v1,v2,v3 row_shr:7 bound_ctrl:0
v_xor_b32 v1,v2,v3 row_shl:5 row_mask:0b1100
v_xor_b32 v1,v2,v3 row_shl:5 bank_mask:0b0101
```

Operation code:  
```
// SRC0_SRC[X] - original VSRC0 value from thread X
// SRC0_DST[X] - destination VSRC0 value from thread X
// OPERATION(SRC0, SRC1) - instruction operation, VDST - VDST register before instruction
BYTE invalid = 0
BYTE srcLane
if (DPP_CTRL>=DPP_QUAD_PERM00 && DPP_CTRL<=DPP_QUAD_PERMFF)
{
    BYTE p0 = DPP_CTRL&3
    BYTE p1 = (DPP_CTRL>>2)&3
    BYTE p2 = (DPP_CTRL>>4)&3
    BYTE p3 = (DPP_CTRL>>6)&3
    BYTE curL4 = LANEID&~3
    if (LANEID&3==0)
        srcLane = curL4 + p0
    else if (LANEID&3==1)
        srcLane = curL4 + p1
    else if (LANEID&3==2)
        srcLane = curL4 + p2
    else if (LANEID&3==3)
        srcLane = curL4 + p3    
}
else if (DPP_CTRL>=DPP_ROW_SL1 && DPP_CTRL<=DPP_ROW_SL15)
{
    BYTE shift = DPP_CTRL&15
    BYTE slid = LANEID&15
    BYTE curR = LANEID&~15
    if (slid+shift<=15)
        srcLane = curR + slid + shift
    else
        srcLane = LANESNUM
}
else if (DPP_CTRL>=DPP_ROW_SR1 && DPP_CTRL<=DPP_ROW_SR15)
{
    BYTE shift = DPP_CTRL&15
    BYTE slid = LANEID&15
    BYTE curR = LANEID&~15
    if (slid>=shift)
        srcLane = curR + slid - shift
    else
        srcLane = LANESNUM
}
else if (DPP_CTRL>=DPP_ROW_RR1 && DPP_CTRL<=DPP_ROW_RR15)
{
    BYTE shift = DPP_CTRL&15
    BYTE slid = LANEID&15
    BYTE curR = LANEID&~15
    srcLane = curR + ((16+slid - shift)&15)
}
else if (DPP_CTRL==DPP_WF_SL1)
    srcLane = LANEID+1
else if (DPP_CTRL==DPP_WF_SR1)
    srcLane = LANEID-1
else if (DPP_CTRL==DPP_WF_RL1)
    srcLane = (LANEID+1)&63
else if (DPP_CTRL==DPP_WF_RR1)
    srcLane = (LANEID-1)&63
else if (DPP_CTRL==DPP_ROW_MIRROR)
{
    BYTE curR = LANEID&~15
    srcLane = curR + ((LANEID&15)^15)
}
else if (DPP_CTRL==DPP_ROW_HALF_MIRROR)
{
    BYTE curR = LANEID&~7
    srcLane = curR + ((LANEID&7)^7)
}
else if (DPP_CTRL==DPP_BCAST_15)
{
    BYTE curR = LANEID&~15
    if (LANEID<15)
        srcLane = LANEID
    else
        srcLane = ((LANEID-16)&~15)+15
}
else if (DPP_CTRL==DPP_BCAST_31)
{
    BYTE curR = LANEID&~31
    if (LANEID<31)
        srcLane = LANEID
    else
        srcLane = ((LANEID-31)&~31)+31
}
if (srcLane < LANESNUM)
    SRC0_DST[LANEID] = SRC0_SRC[srcLane]
else if (BOUND_CTRL==0)
    SRC0_DST[LANEID] = 0
else
    invalid = 1
if ((ROW_MASK & (1U<<(LANEID>>4)))==0)
    invalid = 1
if ((BANK_MASK & (1U<<((LANEID>>2)&3)))==0)
    invalid = 1
if (!invalid)
    VDST = OPERATION(SRC0_DST,SRC1)
```
