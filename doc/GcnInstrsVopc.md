## GCN ISA VOPC/VOP3 instructions

VOPC instructions can be encoded in the VOPC encoding and the VOP3A encoding.
List of fields for VOPC encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-8   | SRC0     | First (scalar or vector) source operand
9-16  | VSRC1    | Second (scalar or vector) source operand
17-24 | OPCODE   | Operation code
25-31 | ENCODING | Encoding type. Must be 0b0111110

Syntax: INSTRUCTION VCC, SRC0, VSRC1

List of fields for VOP3A/VOP3B encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | SDST     | Scalar destination operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
11    | CLAMP    | CLAMP modifier (VOP3A)
17-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

List of fields for VOP3A/VOP3B encoding (GCN 1.2/1.4):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | SDST     | Scalar destination operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
11-14 | OP_SEL   | Operand selection (VOP3A) (GCN 1.4)
15    | CLAMP    | CLAMP modifier
16-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

Syntax: INSTRUCTION SDST(2), SRC0, SRC1 [MODIFIERS]

Modifiers:

* -SRC - negate floating point value from source operand. Applied after ABS modifier.
* ABS(SRC), |SRC| - apply absolute value to source operand
* OP_SEL:VALUE|[B0,...] - operand half selection (0 - lower 16-bits, 1 - bits)

NOTE: ABS and negation is applied to source operand for any instruction.

Negation and absolute value can be combined: `-ABS(V0)`.

Operand half selection (OP_SEL) take value with bits number depends of number operands.
Last bit control destination operand. Zero in bit choose lower 16-bits in dword,
one choose higher 16-bits. Example: op_sel:[0,1,1] - higher 16-bits in second source and
in destination. List of bits of OP_SEL field:

Bit | Operand | Description
----|---------|----------------------
 11 | SRC0    | Choose part of SRC0 (first source operand)
 12 | SRC1    | Choose part of SRC1 (second source operand)
 13 | SRC2    | Choose part of SRC2 (third source operand)
 14 | VDST    | Choose part of VDST (destination)

Limitations for operands:

* only one SGPR can be read by instruction. Multiple occurrences of this same
SGPR is allowed
* only one literal constant can be used, and only when a SGPR or M0 is not used in
source operands
* only SRC0 can holds LDS_DIRECT

Unaligned pairs of SGPRs are allowed in source and destination operands.

VOPC opcodes (0-255) and VOP3 opcodes are same.

NOTE: Bits for inactive threads in the 64-bit scalar destination
(VCC or pair of SGPRs) are always zeroed.

### Tables of opcodes and their descriptions

Table of floating point comparison instructions by opcode (GCN 1.0/1.1):

Opcode range        | Instruction        | Description
--------------------|--------------------|---------------------------
0-15 (0x00-0x0f)    | V_CMP_{OP16}_F32   | Signal on sNAN input only. Single FP values.
16-31 (0x10-0x1f)   | V_CMPX_{OP16}_F32  | Signal on sNAN input only. Also write result to EXEC. Single FP values.
32-47 (0x20-0x2f)   | V_CMP_{OP16}_F64   | Signal on sNAN input only. Double FP values.
48-63 (0x30-0x3f)   | V_CMPX_{OP16}_F64  | Signal on sNAN input only. Also write result to EXEC. Double FP values.
64-79 (0x40-0x4f)   | V_CMPS_{OP16}_F32  | Signal on any sNAN. Single FP values.
80-95 (0x50-0x5f)   | V_CMPSX_{OP16}_F32 | Signal on any sNAN. Also write result to EXEC. Single FP values.
96-111 (0x60-0x6f)  | V_CMPS_{OP16}_F64  | Signal on any sNAN. Double FP values.
112-127 (0x70-0x7f) | V_CMPSX_{OP16}_F64 | Signal on any sNAN. Also write result to EXEC. Double FP values.

Table of floating point comparison instructions by opcode (GCN 1.2/1.4):

Opcode range        | Instruction        | Description
--------------------|--------------------|---------------------------
32-47 (0x20-0x2f)   | V_CMP_{OP16}_F16   | Signal on sNAN input only. Half FP values.
48-63 (0x30-0x3f)   | V_CMPX_{OP16}_F16  | Signal on sNAN input only. Also write result to EXEC. Half FP values.
64-79 (0x40-0x4f)   | V_CMP_{OP16}_F32   | Signal on sNAN input only. Single FP values.
80-95 (0x50-0x5f)   | V_CMPX_{OP16}_F32  | Signal on sNAN input only. Also write result to EXEC. Single FP values.
96-111 (0x60-0x6f)  | V_CMP_{OP16}_F64   | Signal on sNAN input only. Double FP values.
112-127 (0x70-0x7f) | V_CMPX_{OP16}_F64  | Signal on sNAN input only. Also write result to EXEC. Double FP values.

Table of OP16 (compare operations) for floating point values comparisons:

Opcode offset | OP16 name | Description
--------------|-----------|--------------------------
0 (0x0)       | F         | SDST(LANEID) = 0
1 (0x1)       | LT        | SDST(LANEID) = ASTYPE(SRC0) < ASTYPE(SRC1_
2 (0x2)       | EQ        | SDST(LANEID) = ASTYPE(SRC0) == ASTYPE(SRC1)
3 (0x3)       | LE        | SDST(LANEID) = ASTYPE(SRC0) <= ASTYPE(SRC1)
4 (0x4)       | GT        | SDST(LANEID) = ASTYPE(SRC0) > ASTYPE(SRC1)
5 (0x5)       | LG        | SDST(LANEID) = ASTYPE(SRC0) != ASTYPE(SRC1)
6 (0x6)       | GE        | SDST(LANEID) = ASTYPE(SRC0) >= ASTYPE(SRC1)
7 (0x7)       | O         | SDST(LANEID) = (!ISNAN(ASTYPE(SRC0)) && !ISNAN(ASTYPE(SRC1))
8 (0x8)       | U         | SDST(LANEID) = (!ISNAN(ASTYPE(SRC0)) || !ISNAN(ASTYPE(SRC1)))
9 (0x9)       | NGE       | SDST(LANEID) = !(ASTYPE(SRC0) >= ASTYPE(SRC1))
10 (0xa)      | NLG       | SDST(LANEID) = !(ASTYPE(SRC0) != ASTYPE(SRC1))
11 (0xb)      | NGT       | SDST(LANEID) = !(ASTYPE(SRC0) > ASTYPE(SRC1))
12 (0xc)      | NLE       | SDST(LANEID) = !(ASTYPE(SRC0) <= ASTYPE(SRC1))
13 (0xd)      | NEQ       | SDST(LANEID) = !(ASTYPE(SRC0) == ASTYPE(SRC1))
14 (0xe)      | NLT       | SDST(LANEID) = !(ASTYPE(SRC0) < ASTYPE(SRC1))
15 (0xf)      | TRU, T    | SDST(LANEID) = 1

NOTE: Comparison operators (<,<=,!=,==) compares only non NaN values. If any operand is NaN
then returns false. By contrast, negations of comparisons (NLT, NGT) returns true
if any operand is NaN value. This feature distinguish for example NGE from LT.  

LANEID in description is lane id. ASTYPE function that treat compared values
as value of type (ASFLOAT for _FP32, ASDOUBLE for _FP64).

Sample instructions:  
```
V_CMPX_LT_F32 VCC, V0, V1  # V0<V1
V_CMPSX_EQ_F32 VCC, V0, V1 # V0==V1, store result to EXEC, signal for any sNaN
V_CMPX_LT_F64 VCC, V[2:3], V[4:5]  # V[2:3]<V[4:5]
```

Table of integer comparison instructions by opcode (GCN 1.0/1.1):

Opcode range        | Instruction        | Description
--------------------|--------------------|---------------------------
128-135 (0x80-0x87) | V_CMP_{OP8}_I32    | Signed 32-bit values.
144-151 (0x90-0x97) | V_CMPX_{OP8}_I32   | Also write result to EXEC. Signed 32-bit values.
160-167 (0xa0-0xa7) | V_CMP_{OP8}_I64    | Signed 64-bit values.
176-183 (0xb0-0xb7) | V_CMPX_{OP8}_I64   | Also write result to EXEC. Signed 64-bit values.
192-199 (0xc0-0xc7) | V_CMP_{OP8}_U32    | Unsigned 32-bit values.
208-215 (0xd0-0xd7) | V_CMPX_{OP8}_U32   | Also write result to EXEC. Unsigned 32-bit values.
224-231 (0xe0-0xe7) | V_CMP_{OP8}_U64    | Unsigned 64-bit values.
240-247 (0xf0-0xf7) | V_CMPX_{OP8}_U64   | Also write result to EXEC. Unsigned 64-bit values.

Table of integer comparison instructions by opcode (GCN 1.2/1.4):

Opcode range        | Instruction        | Description
--------------------|--------------------|---------------------------
160-167 (0xa0-0xa7) | V_CMP_{OP8}_I16    | Signed 16-bit values.
168-175 (0xa8-0xaf) | V_CMP_{OP8}_U16    | Unsigned 16-bit values.
176-183 (0xb0-0xb7) | V_CMPX_{OP8}_I16   | Also write result to EXEC. Signed 16-bit values.
184-191 (0xb8-0xbf) | V_CMPX_{OP8}_U16   | Also write result to EXEC. Unsigned 16-bit values.
192-199 (0xc0-0xc7) | V_CMP_{OP8}_I32    | Signed 32-bit values.
200-207 (0xc8-0xcf) | V_CMP_{OP8}_U32    | Unsigned 32-bit values.
208-215 (0xd0-0xd7) | V_CMPX_{OP8}_I32   | Also write result to EXEC. Signed 32-bit values.
216-223 (0xd8-0xdf) | V_CMPX_{OP8}_U32   | Also write result to EXEC. Unsigned 32-bit values.
224-231 (0xe0-0xe7) | V_CMP_{OP8}_I64    | Signed 64-bit values.
232-239 (0xe8-0xef) | V_CMP_{OP8}_U64    | Unsigned 64-bit values.
240-247 (0xf0-0xf7) | V_CMPX_{OP8}_I64   | Also write result to EXEC. Signed 64-bit values.
248-255 (0xf8-0xff) | V_CMPX_{OP8}_U64   | Also write result to EXEC. Unsigned 64-bit values.

Table of OP16 (compare operations) for integer values comparisons:

Opcode offset | OP8 name | Description
--------------|----------|--------------------------
0 (0x0)       | F        | SDST(LANEID) = 0
1 (0x1)       | LT       | SDST(LANEID) = (TYPE)SRC0 < (TYPE)SRC1
2 (0x2)       | EQ       | SDST(LANEID) = (TYPE)SRC0 == (TYPE)SRC1
3 (0x3)       | LE       | SDST(LANEID) = (TYPE)SRC0 <= (TYPE)SRC1
4 (0x4)       | GT       | SDST(LANEID) = (TYPE)SRC0 > (TYPE)SRC1
5 (0x5)       | LG, NE   | SDST(LANEID) = (TYPE)SRC0 != (TYPE)SRC1
6 (0x6)       | GE       | SDST(LANEID) = (TYPE)SRC0 >= (TYPE)SRC1
7 (0x7)       | TRU, T   | SDST(LANEID) = 1

LANEID in description is lane id. TYPE is type of compared values (UINT32 for _U32,
INT32 for _I32,...).

Sample instructions:  
```
V_CMP_LT_U32 VCC, V0, V1  # V0<V1
V_CMPX_EQ_U32 VCC, V0, V1 # V0==V1, store result to EXEC, signal for any sNaN
```

NOTE: In GCN 1.4 architecture, CLAMP modifier enabled signalling exception when NaN in
inputs was encountered.

### Class instructions

List of class instructions:

#### V_CMP_CLASS_F16

Opcode: 20 (0x14) for GCN 1.2/1.4  
Syntax VOPC: V_CMP_CLASS_F16 VCC, SRC0, SRC1  
Syntax VOP3: V_CMP_CLASS_F16 SDST, SRC0, SRC1  
Operation: Check whether SRC0 half floating point value belongs to one of specified class.
Classes are specified as set bits in SRC1. If that condition is satisfied then store
1 to bit of SDST with number of current lane id, otherwise clear that bit.
This instruction doesn't raise exception under any circumstances.
No flushing denormalized values for SRC0. List of classes:

Bit | Description
----|----------------------------
0   | Signaling NaN
1   | quiet Nan
2   | -INF
3   | negative normalized value
4   | negative dernormalized value
5   | negative zero
6   | positive zero
7   | positive denormalized value
8   | positive normalized value
9   | +INF

#### V_CMPX_CLASS_F16

Opcode: 21 (0x15) for GCN 1.2/1.4  
Syntax VOPC: V_CMPX_CLASS_F16 VCC, SRC0, SRC1  
Syntax VOP3: V_CMPX_CLASS_F16 SDST, SRC0, SRC1  
Operation: Check whether SRC0 half floating point value belongs to one of specified class.
Classes are specified as set bits in SRC1. If that condition is satisfied then store
1 to bit of SDST and EXEC with number of current lane id, otherwise clear that bit.
This instruction doesn't raise exception under any circumstances.
No flushing denormalized values for SRC0. List of classes:

Bit | Description
----|----------------------------
0   | Signaling NaN
1   | quiet Nan
2   | -INF
3   | negative normalized value
4   | negative dernormalized value
5   | negative zero
6   | positive zero
7   | positive denormalized value
8   | positive normalized value
9   | +INF


#### V_CMP_CLASS_F32

Opcode: 136 (0x88) for GCN 1.0/1.1; 16 (0x10) for GCN 1.2/1.4  
Syntax VOPC: V_CMP_CLASS_F32 VCC, SRC0, SRC1  
Syntax VOP3: V_CMP_CLASS_F32 SDST, SRC0, SRC1  
Operation: Check whether SRC0 single floating point value belongs to one of specified class.
Classes are specified as set bits in SRC1. If that condition is satisfied then store
1 to bit of SDST with number of current lane id, otherwise clear that bit.
This instruction doesn't raise exception under any circumstances.
No flushing denormalized values for SRC0. List of classes:

Bit | Description
----|----------------------------
0   | Signaling NaN
1   | quiet Nan
2   | -INF
3   | negative normalized value
4   | negative dernormalized value
5   | negative zero
6   | positive zero
7   | positive denormalized value
8   | positive normalized value
9   | +INF

#### V_CMPX_CLASS_F32

Opcode: 152 (0x98) for GCN 1.0/1.1; 17 (0x11) for GCN 1.2/1.4  
Syntax VOPC: V_CMPX_CLASS_F32 VCC, SRC0, SRC1  
Syntax VOP3: V_CMPX_CLASS_F32 SDST, SRC0, SRC1  
Operation: Check whether SRC0 single floating point value belongs to one of specified class.
Classes are specified as set bits in SRC1. If that condition is satisfied then store
1 to bit of SDST and EXEC with number of current lane id, otherwise clear that bit.
This instruction doesn't raise exception under any circumstances.
No flushing denormalized values for SRC0. List of classes:

Bit | Description
----|----------------------------
0   | Signaling NaN
1   | quiet Nan
2   | -INF
3   | negative normalized value
4   | negative dernormalized value
5   | negative zero
6   | positive zero
7   | positive denormalized value
8   | positive normalized value
9   | +INF

#### V_CMP_CLASS_F64

Opcode: 168 (0xa8) for GCN 1.0/1.1; 18 (0x12) for GCN 1.2/1.4  
Syntax VOPC: V_CMP_CLASS_F64 VCC, SRC0, SRC1(2)  
Syntax VOP3: V_CMP_CLASS_F64 SDST, SRC0(2), SRC1(2)  
Operation: Check whether SRC0 double floating point value belongs to one of specified class.
Classes are specified as set bits in SRC1. If that condition is satisfied then store
1 to bit of SDST with number of current lane id, otherwise clear that bit.
This instruction doesn't raise exception under any circumstances.
No flushing denormalized values for SRC0. List of classes:

Bit | Description
----|----------------------------
0   | Signaling NaN
1   | quiet Nan
2   | -INF
3   | negative normalized value
4   | negative dernormalized value
5   | negative zero
6   | positive zero
7   | positive denormalized value
8   | positive normalized value
9   | +INF

#### V_CMPX_CLASS_F64

Opcode: 184 (0xb8) for GCN 1.01/1.1; 19 (0x13) for GCN 1.2/1.4  
Syntax VOPC: V_CMPX_CLASS_F64 VCC, SRC0(2), SRC1(2)  
Syntax VOP3: V_CMPX_CLASS_F64 SDST, SRC0(2), SRC1(2)  
Operation: Check whether SRC0 double floating point value belongs to one of specified class.
Classes are specified as set bits in SRC1. If that condition is satisfied then store
1 to bit of SDST and EXEC with number of current lane id, otherwise clear that bit.
This instruction doesn't raise exception under any circumstances.
No flushing denormalized values for SRC0. List of classes:

Bit | Description
----|----------------------------
0   | Signaling NaN
1   | quiet Nan
2   | -INF
3   | negative normalized value
4   | negative dernormalized value
5   | negative zero
6   | positive zero
7   | positive denormalized value
8   | positive normalized value
9   | +INF
