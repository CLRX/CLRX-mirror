## GCN ISA VOP3P instructions (GCN 1.4)

The VOP3P encoding is derivative of VOP3 encoding. It needs two dwords to one instruction
and give many modifiers to control behaviour of instruction. This encoding has been
designed for packed half-floating point and packed 16-bit arithmetic instructions.

List of fields for VOP3P encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Vector destination operand
8-10  | NEG_HI   | Negation modifier for high part of source operands
11-13 | OP_SEL   | Operand selection for lower part
14,59,60 | OP_SEL_HI | Operand selection for high part
15    | CLAMP    | CLAMP modifier
16-22 | OPCODE   | Operation code
23-31 | ENCODING | Encoding type. Must be 0b110100111
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
61-63 | NEG      | Negation modifier for lower part of source operands

Typical syntax: INSTRUCTION VDST, SRC0, SRC1, SRC2 [MODifIERS]

Modifiers:

* CLAMP - clamps destination floating point value in range 0.0-1.0
* -SRC - negate floating point value from source operand. Applied after ABS modifier.
* OP_SEL:VALUE|[B0,...] - operand lower half selection (0 - lower 16-bits, 1 - bits)
* OP_SEL_HI:VALUE|[B0,...] - operand higher half selection (0 - lower 16-bits, 1 - bits)
* NEG - negate floating point value from lower part.
* NEG_HI - negate floating point value from higher part.

Operand half selection (OP_SEL) take value with bits number depends of number operands.
Zero in bit choose lower 16-bits in dword, one choose higher 16-bits.
Example: op_sel:[0,1,1] - higher 16-bits in second source and in third source.
List of bits of OP_SEL field:

OP_SEL Bit | OP_SEL_HI Bit | Operand | Description
-----------|---------------|---------|----------------------
 11        | 14            | SRC0    | Choose part of SRC0 (first source operand)
 12        | 59            | SRC1    | Choose part of SRC1 (second source operand)
 13        | 60            | SRC2    | Choose part of SRC2 (third source operand)

Limitations for operands:

* only one SGPR can be read by instruction. Multiple occurrences of this same
SGPR is allowed
* only one literal constant can be used, and only when a SGPR or M0 is not used in
source operands
* only SRC0 can holds LDS_DIRECT

List of the instructions by opcode:

 Opcode    | Mnemonic
-----------|-----------------------------------
 0 (0x0)   | V_PK_MAD_I16
 1 (0x1)   | V_PK_MUL_LO_U16
 2 (0x2)   | V_PK_ADD_I16
 3 (0x3)   | V_PK_SUB_I16
 4 (0x4)   | V_PK_LSHLREV_B16
 5 (0x5)   | V_PK_LSHRREV_B16
 6 (0x6)   | V_PK_ASHRREV_I16
 7 (0x7)   | V_PK_MAX_I16
 8 (0x8)   | V_PK_MIN_I16
 9 (0x9)   | V_PK_MAD_U16
 10 (0xa)  | V_PK_ADD_U16
 11 (0xb)  | V_PK_SUB_U16
 12 (0xc)  | V_PK_MAX_U16
 13 (0xd)  | V_PK_MIN_U16
 14 (0xe)  | V_PK_FMA_F16
 15 (0xf)  | V_PK_ADD_F16
 16 (0x10) | V_PK_MUL_F16
 17 (0x11) | V_PK_MIN_F16
 18 (0x12) | V_PK_MAX_F16
 32 (0x20) | V_MAD_MIX_F32
 33 (0x21) | V_MAD_MIXLO_F16
 34 (0x22) | V_MAD_MIXHI_F16

### Instruction set

Alphabetically sorted instruction list:

#### V_MAD_MIX_F32

Opcode: 32 (0x20)  
Syntax: V_MAD_MIX_F32 VDST, SRC0, SRC1, SRC2  
Description: Multiply single FP value from SRC0 by single FP value SRC1 and add
single FP value from SRC2, and store result to VDST.
OP_SEL and OP_SEL_HI controls type and place of sources:

OP_SEL | OP_SEL_HI  | Meaning
-------|------------|---------------------
  0    |  0         | FP32
  1    |  0         | FP32
  0    |  1         | FP16 in lower part
  1    |  1         | FP32 in higher part

NEG_HI changes meaning to absolute-value modifier.  
```
FLOAT getSource(UINT32 S, BYTE OP_SEL, BYTE OP_SEL_HI, SRCINDEX)
{
    BYTE mask = 1<<SRCINDEX
    if ((OP_SEL_HI&mask) == 0)
        return ASFLOAT(S)
    if ((OP_SEL&mask) == 0 && (OP_SEL_HI&mask) == 1)
        return (FLOAT)ASHALF(S&0xffff)
    else
        return (FLOAT)ASHALF(S>>16)
}
FLOAT SS0 = getSource(SRC0, OP_SEL, OP_SEL_HI, 0)
FLOAT SS1 = getSource(SRC1, OP_SEL, OP_SEL_HI, 1)
FLOAT SS2 = getSource(SRC2, OP_SEL, OP_SEL_HI, 2)
FLOAT S0 = NEG_HI&1 ? ABS(SS0) : SS0
FLOAT S1 = NEG_HI&2 ? ABS(SS1) : SS1
FLOAT S2 = NEG_HI&4 ? ABS(SS2) : SS2
VDST = S0 * S1 + S2
```

#### V_MAD_MIXLO_F16

Opcode: 33 (0x21)  
Syntax: V_MAD_MIXLO_F16 VDST, SRC0, SRC1, SRC2  
Description: Multiply FP value from SRC0 by FP value SRC1 and add
half FP value from SRC2, and store result to lower 16-bit of VDST.
OP_SEL and OP_SEL_HI controls type and place of sources:

OP_SEL | OP_SEL_HI  | Meaning
-------|------------|---------------------
  0    |  0         | FP32
  1    |  0         | FP32
  0    |  1         | FP16 in lower part
  1    |  1         | FP32 in higher part

NEG_HI changes meaning to absolute-value modifier.  
```
FLOAT getSource(UINT32 S, BYTE OP_SEL, BYTE OP_SEL_HI, SRCINDEX)
{
    BYTE mask = 1<<SRCINDEX
    if ((OP_SEL_HI&mask) == 0)
        return ASFLOAT(S)
    if ((OP_SEL&mask) == 0 && (OP_SEL_HI&mask) == 1)
        return (FLOAT)ASHALF(S&0xffff)
    else
        return (FLOAT)ASHALF(S>>16)
}
FLOAT SS0 = getSource(SRC0, OP_SEL, OP_SEL_HI, 0)
FLOAT SS1 = getSource(SRC1, OP_SEL, OP_SEL_HI, 1)
FLOAT SS2 = getSource(SRC2, OP_SEL, OP_SEL_HI, 2)
FLOAT S0 = NEG_HI&1 ? ABS(SS0) : SS0
FLOAT S1 = NEG_HI&2 ? ABS(SS1) : SS1
FLOAT S2 = NEG_HI&4 ? ABS(SS2) : SS2
VDST = (ASUINT32((HALF)(S0 * S1 + S2))&0xfff) | (VDST&0xffff0000)
```

#### V_MAD_MIXHI_F16

Opcode: 34 (0x22)  
Syntax: V_MAD_MIXHI_F16 VDST, SRC0, SRC1, SRC2  
Description: Multiply half FP value from SRC0 by half FP value SRC1 and add
half FP value from SRC2, and store result to higher 16-bit part of VDST.
OP_SEL and OP_SEL_HI controls type and place of sources:

OP_SEL | OP_SEL_HI  | Meaning
-------|------------|---------------------
  0    |  0         | FP32
  1    |  0         | FP32
  0    |  1         | FP16 in lower part
  1    |  1         | FP32 in higher part

NEG_HI changes meaning to absolute-value modifier.  
```
FLOAT getSource(UINT32 S, BYTE OP_SEL, BYTE OP_SEL_HI, SRCINDEX)
{
    BYTE mask = 1<<SRCINDEX
    if ((OP_SEL_HI&mask) == 0)
        return ASFLOAT(S)
    if ((OP_SEL&mask) == 0 && (OP_SEL_HI&mask) == 1)
        return (FLOAT)ASHALF(S&0xffff)
    else
        return (FLOAT)ASHALF(S>>16)
}
FLOAT SS0 = getSource(SRC0, OP_SEL, OP_SEL_HI, 0)
FLOAT SS1 = getSource(SRC1, OP_SEL, OP_SEL_HI, 1)
FLOAT SS2 = getSource(SRC2, OP_SEL, OP_SEL_HI, 2)
FLOAT S0 = NEG_HI&1 ? ABS(SS0) : SS0
FLOAT S1 = NEG_HI&2 ? ABS(SS1) : SS1
FLOAT S2 = NEG_HI&4 ? ABS(SS2) : SS2
VDST = (ASUINT32((HALF)(S0 * S1 + S2))<<16) | (VDST&0xffff)
```

#### V_PK_ADD_F16

Opcode: 15 (0xf)  
Syntax: V_PK_ADD_F16 VDST, SRC0, SRC1  
Description: Add two 16-bit FP values from SRC0 to
16-bit FP values from SRC1, and store result to VDST.  
Operation:  
```
HALF S0_0 = ASHALF(SRC0&0xffff), S0_1 = ASHALF(SRC0>>16)
HALF S1_0 = ASHALF(SRC1&0xffff), S1_1 = ASHALF(SRC1>>16)
HALF temp0 = S0_0 + S1_0
HALF temp1 = S0_1 + S1_1
VDST = ASINT16(temp0) | (ASINT16(temp1)<<16)
```

#### V_PK_ADD_I16

Opcode: 2 (0x2)  
Syntax: V_PK_ADD_I16 VDST, SRC0, SRC1  
Description: Add two 16-bit signed integers from SRC0 to
16-bit signed integers from SRC1, and store result to VDST.
If CLAMP modifier supplied, then results are saturated to 16-bit signed values.  
Operation:  
```
INT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
INT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
INT32 temp0 = S0_0 + S1_0
INT32 temp1 = S0_1 + S1_1
if (CLAMP)
{
    temp0 = (UINT16)MAX(MIN(temp0, 32767), -32768)
    temp1 = (UINT16)MAX(MIN(temp1, 32767), -32768)
}
VDST = (temp0&0xffff) | (temp1<<16)
```

#### V_PK_ADD_U16

Opcode: 10 (0xa)  
Syntax: V_PK_ADD_U16 VDST, SRC0, SRC1  
Description: Add two 16-bit unsigned integers from SRC0 to
16-bit unsigned integers from SRC1, and store result to VDST.
If CLAMP modifier supplied, then results are saturated to 16-bit unsigned values.  
Operation:  
```
UINT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
UINT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT32 temp0 = S0_0 + S1_0
UINT32 temp1 = S0_1 + S1_1
if (CLAMP)
{
    temp0 = MIN(temp0, 65535)
    temp1 = MIN(temp1, 65535)
}
VDST = (temp0&0xffff) | (temp1<<16)
```

#### V_PK_FMA_F16

Opcode: 14 (0xe)  
Syntax: V_PK_FMA_F16 VDST, SRC0, SRC1, SRC2  
Description:  Two fused multiplies-adds on two 16-bit FP values from SRC0, SRC1 and SRC2
and store result to VDST.  
Operation:  
```
HALF S0_0 = ASHALF(SRC0&0xffff), S0_1 = ASHALF(SRC0>>16)
HALF S1_0 = ASHALF(SRC1&0xffff), S1_1 = ASHALF(SRC1>>16)
HALF S2_0 = ASHALF(SRC2&0xffff), S2_1 = ASHALF(SRC2>>16)
HALF temp0 = FMA(S0_0, S1_0, S2_0)
HALF temp1 = FMA(S0_1, S1_1, S2_1)
VDST = ASINT16(temp0) | (ASINT16(temp1)<<16)
```

#### V_PK_ASHRREV_I16

Opcode: 6 (0x6)  
Syntax: V_PK_ASHRREV_I16 VDST, SRC0, SRC1  
Description: Arithmetic shift right two 16-bit signed values from SRC1 by numbers of bits
given in SRC0.  
Operation:  
```
INT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
INT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 temp0 = S1_0 >> (S0_0&15)
UINT16 temp1 = S1_1 >> (S0_1&15)
VDST = temp0 | (temp1<<16)
```

#### V_PK_LSHLREV_B16

Opcode: 4 (0x4)  
Syntax: V_PK_LSHLREV_B16 VDST, SRC0, SRC1  
Description: Shift left two 16-bit values from SRC1 by numbers of bits given in SRC0.  
Operation:  
```
UINT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
UINT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 temp0 = S1_0 << (S0_0&15)
UINT16 temp1 = S1_1 << (S0_1&15)
VDST = temp0 | (temp1<<16)
```

#### V_PK_LSHRREV_B16

Opcode: 5 (0x5)  
Syntax: V_PK_LSHRREV_B16 VDST, SRC0, SRC1  
Description: Shift right two 16-bit values from SRC1 by numbers of bits given in SRC0.  
Operation:  
```
UINT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
UINT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 temp0 = S1_0 >> (S0_0&15)
UINT16 temp1 = S1_1 >> (S0_1&15)
VDST = temp0 | (temp1<<16)
```

#### V_PK_MAD_I16

Opcode: 0 (0x0)  
Syntax: V_PK_MAD_I16 VDST, SRC0, SRC1, SRC2  
Description: Multiply two 16-bit signed integers from SRC0 by two 16-bit signed integers
from SRC1 and add two 16-bit signed integers from SRC2, and store result to VDST.
If CLAMP modifier supplied, then results are saturated to 16-bit signed values.  
Operation:  
```
INT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
INT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
INT16 S2_0 = SRC2&0xffff, S2_1 = SRC2>>16
INT32 temp0 = S0_0 * S1_0 + S2_0
INT32 temp1 = S0_1 * S1_1 + S2_1
if (CLAMP)
{
    temp0 = (UINT16)MAX(MIN(temp0, 32767), -32768)
    temp1 = (UINT16)MAX(MIN(temp1, 32767), -32768)
}
VDST = (temp0&0xffff) | (temp1<<16)
```

#### V_PK_MAD_U16

Opcode: 9 (0x9)  
Syntax: V_PK_MAD_U16 VDST, SRC0, SRC1, SRC2  
Description: Multiply two 16-bit unsigned integers from SRC0 by two 16-bit unsigned
integers from SRC1 and add two 16-bit unsigned integers from SRC2,
and store result to VDST.
If CLAMP modifier supplied, then results are saturated to 16-bit unsigned values.  
Operation:  
```
UINT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
UINT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 S2_0 = SRC2&0xffff, S2_1 = SRC2>>16
UINT32 temp0 = S0_0 * S1_0 + S2_0
UINT32 temp1 = S0_1 * S1_1 + S2_1
if (CLAMP)
{
    temp0 = MIN((UINT16)temp0, 65535)
    temp1 = MIN((UINT16)temp1, 65535)
}
VDST = (temp0&0xffff) | (temp1<<16)
```

#### V_PK_MAX_F16

Opcode: 18 (0x12)  
Syntax: V_PK_MAX_F16 VDST, SRC0, SRC1  
Description: Choose greatest 16-bit floating point values between values from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
HALF S0_0 = ASHALF(SRC0&0xffff), S0_1 = ASHALF(SRC0>>16)
HALF S1_0 = ASHALF(SRC1&0xffff), S1_1 = ASHALF(SRC1>>16)
HALF temp0 = MAX(S0_0, S1_0)
HALF temp1 = MAX(S0_1, S1_1)
VDST = ASINT16(temp0) | (ASINT16(temp1)<<16)
```

#### V_PK_MAX_I16

Opcode: 7 (0x7)  
Syntax: V_PK_MAX_I16 VDST, SRC0, SRC1  
Description: Choose greatest 16-bit signed integers between values from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
INT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
INT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 temp0 = MAX(S0_0, S1_0)
UINT16 temp1 = MAX(S0_1, S1_1)
VDST = temp0 | (temp1<<16)
```

#### V_PK_MAX_U16

Opcode: 12 (0xc)  
Syntax: V_PK_MAX_U16 VDST, SRC0, SRC1  
Description: Choose greatest 16-bit unsigned integers between values from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
UINT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
UINT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 temp0 = MAX(S0_0, S1_0)
UINT16 temp1 = MAX(S0_1, S1_1)
VDST = temp0 | (temp1<<16)
```

#### V_PK_MIN_F16

Opcode: 17 (0x11)  
Syntax: V_PK_MIN_F16 VDST, SRC0, SRC1  
Description: Choose smallest 16-bit floating point values between values from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
HALF S0_0 = ASHALF(SRC0&0xffff), S0_1 = ASHALF(SRC0>>16)
HALF S1_0 = ASHALF(SRC1&0xffff), S1_1 = ASHALF(SRC1>>16)
HALF temp0 = MIN(S0_0, S1_0)
HALF temp1 = MIN(S0_1, S1_1)
VDST = ASINT16(temp0) | (ASINT16(temp1)<<16)
```

#### V_PK_MIN_I16

Opcode: 8 (0x8)  
Syntax: V_PK_MIN_I16 VDST, SRC0, SRC1  
Description: Choose smallest 16-bit signed integers between values from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
INT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
INT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 temp0 = MIN(S0_0, S1_0)
UINT16 temp1 = MIN(S0_1, S1_1)
VDST = temp0 | (temp1<<16)
```

#### V_PK_MIN_U16

Opcode: 13 (0xd)  
Syntax: V_PK_MIN_U16 VDST, SRC0, SRC1  
Description: Choose smallest 16-bit unsigned integers between values from SRC0 and SRC1,
and store result to VDST.  
Operation:  
```
UINT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
UINT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 temp0 = MIN(S0_0, S1_0)
UINT16 temp1 = MIN(S0_1, S1_1)
VDST = temp0 | (temp1<<16)
```

#### V_PK_MUL_F16

Opcode: 16 (0x10)  
Syntax: V_PK_MUL_F16 VDST, SRC0, SRC1  
Description: Multiply two 16-bit FP values from SRC0 by
16-bit FP values from SRC1, and store result to VDST.  
Operation:  
```
HALF S0_0 = ASHALF(SRC0&0xffff), S0_1 = ASHALF(SRC0>>16)
HALF S1_0 = ASHALF(SRC1&0xffff), S1_1 = ASHALF(SRC1>>16)
HALF temp0 = S0_0 * S1_0
HALF temp1 = S0_1 * S1_1
VDST = ASINT16(temp0) | (ASINT16(temp1)<<16)
```

#### V_PK_MUL_LO_U16

Opcode: 1 (0x1)  
Syntax: V_PK_MUL_U16 VDST, SRC0, SRC1  
Description: Multiply two 16-bit unsigned integers from SRC0 by two
16-bit unsigned integers from SRC1, and store result to VDST.  
Operation:  
```
UINT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
UINT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT16 temp0 = S0_0 * S1_0
UINT16 temp1 = S0_1 * S1_1
VDST = temp0 | (temp1<<16)
```

#### V_PK_SUB_I16

Opcode: 3 (0x3)  
Syntax: V_PK_SUB_I16 VDST, SRC0, SRC1  
Description: Subtract two 16-bit signed integers from SRC1 from
16-bit signed integers from SRC0, and store result to VDST.
If CLAMP modifier supplied, then results are saturated to 16-bit signed values.  
Operation:  
```
INT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
INT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
INT32 temp0 = S0_0 - S1_0
INT32 temp1 = S0_1 - S1_1
if (CLAMP)
{
    temp0 = (UINT16)MAX(MIN(temp0, 32767), -32768)
    temp1 = (UINT16)MAX(MIN(temp1, 32767), -32768)
}
VDST = (temp0&0xffff) | (temp1<<16)
```

#### V_PK_SUB_U16

Opcode: 11 (0xb)  
Syntax: V_PK_SUB_U16 VDST, SRC0, SRC1  
Description: Subtract two 16-bit unsigned integers from SRC1 from
16-bit unsigned integers from SRC0, and store result to VDST.
If CLAMP modifier supplied, then results are saturated to 16-bit unsigned values.  
Operation:  
```
UINT16 S0_0 = SRC0&0xffff, S0_1 = SRC0>>16
UINT16 S1_0 = SRC1&0xffff, S1_1 = SRC1>>16
UINT32 temp0 = S0_0 - S1_0
UINT32 temp1 = S0_1 - S1_1
if (CLAMP)
{
    temp0 = MAX(temp0, 0)
    temp1 = MAX(temp1, 0)
}
VDST = (temp0&0xffff) | (temp1<<16)
```
