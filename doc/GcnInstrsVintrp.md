## GCN ISA VINTRP instructions

The VINTRP instructions was designed to interpolate vertex attributes inside
vertex shaders. However, it is possible to use these instructions inside OpenCL or
any GPU program environment. Instructions requires access to LDS (local memory).

List of fields for VINTRP encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VSRC     | Source vector register or interpolation parameter
8-9   | ATTRCHAN | Attribute channel
10-15 | ATTR     | Attribute to interpolate
16-17 | OPCODE   | Operation code
18-25 | VDST     | Destination vector register
26-31 | ENCODING | Encoding type.

Encoding type is 0b110010 for GCN 1.0/1.1 or 0b110101 for GCN 1.2/1.4.

Syntax: INSTRUCTION VDST, VSRC, ATTR{ATTR_NUM}.{ATTRCHAN}

* ATTR - attribute number in range 0-32
* ATTRCHAN - attribute channel: X - 0, Y - 1, Z - 2, W - 3

Attribute and attribute channel are case-insensitive.

List of the instructions by opcode:

 Opcode      | Mnemonic
-------------|-----------------------
 0 (0x0)     | V_INTERP_P1_F32
 1 (0x1)     | V_INTERP_P2_F32
 2 (0x2)     | V_INTERP_MOV_F32

### How is working interpolation

The VINTRP instructions does the barycentric interpolation by using formulae:
`P0 + I*P10 + J*P20`, where `I` and 'J' are coordinates; P0, P10 and P20 are parameters.
'I' and  'J' supplied in instruction source operand. P0, P10, and P20 are loaded from
LDS (local memory).

Initial configuration is specified in the M0 registers in form:

* 0-15 bits - local memory offset
* 16-30 bits - newprimmask - mask that specify what quad of lane process what primitive

The newprimmask specify primitives for all wavefront lanes. Four lanes can process
single primitive interpolation, so maximum number of primitives is 16. One in bit
newprimmask begins new primitive from lane with number (BIT+1)*4. NewPrimMask skips first
lane, because always begins new primitive. Number of set bits in newprimmask determines
number of primitives plus one. Examples of newprimmask:

The newprimmask 0b1010011:

Lane ids | Processed primitive
---------|---------------------------------
 0-3     | 0
 4-7     | 1
 8-11    | 2
 12-15   | 2
 16-19   | 2
 20-23   | 3
 24-27   | 3
 28-63   | 4

The layout in local memory is three-level. Primitive holds 12 floating points values for
P0, P10, P20 for each channel. Each attribute holds parameters all primitives.
A parameters are stored in order:

Dword | Parameter and channel
------|-------------------------------
 0    | P0.X
 1    | P10.X
 2    | P0.Y
 3    | P10.Y
 4    | P0.Z
 5    | P10.Z
 6    | P0.W
 7    | P10.W
 8    | P20.X
 9    | P20.Y
 10   | P20.Z
 11   | P20.W

Example layout for newprimmask 0b1010011:

 Offset      | Primitive     | Primitive     | Primitive     | Primitive
-------------|---------------|---------------|---------------|-------------------
 0 (0x0)     | ATTR#0 PRIM#0 | ATTR#0 PRIM#1 | ATTR#0 PRIM#2 | ATTR#0 PRIM#3
 192 (0xc0)  | ATTR#0 PRIM#4 | ATTR#1 PRIM#0 | ATTR#1 PRIM#1 | ATTR#1 PRIM#2
 384 (0x180) | ATTR#1 PRIM#3 | ATTR#1 PRIM#4 | ATTR#2 PRIM#0 | ATTR#2 PRIM#1
 576 (0x240) | ATTR#2 PRIM#2 | ATTR#2 PRIM#3 | ATTR#2 PRIM#4 | .......
 
The offset does not include offset given in M0 register (you must add to it).

### Instruction set

Alphabetically sorted instruction list:

#### V_INTERP_MOV_F32

Opcode: 2 (0x2)  
Syntax: V_INTERP_MOV_F32 VDST, PARAMTYPE, ATTR.ATTRCHAN  
Description: Move parameter value into VDST. The PARAMTYPE is P0, P10 or P20.  
NOTE: The indices in LDS is dword indices.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
if (PARAMTYPE==P0)
    VDST[LANEID] = ASFLOAT(LDS[S + ATTRCHAN*2])
else if (PARAMTYPE==P10)
    VDST[LANEID] = ASFLOAT(LDS[S + ATTRCHAN*2 + 1])
else if (PARAMTYPE==P20)
    VDST[LANEID] = ASFLOAT(LDS[S + ATTRCHAN + 8])
```

#### V_INTERP_P1_F32

Opcode: 0 (0x0)  
Syntax: V_INTERP_P1_F32 VDST, VSRC, ATTR.ATTRCHAN  
Description: Instruction does the first step of the interpolation (P0 + P10*I). The I
coordinate given in VSRC register.  
NOTE: The indices in LDS is dword indices.  
NOTE: VDST and VSRC registers must not be same.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
FLOAT P0[LANEID] = ASFLOAT(LDS[S + ATTRCHAN*2])
FLOAT P10[LANEID] = ASFLOAT(LDS[S + ATTRCHAN*2 + 1])
VDST[LANEID] = P0[LANEID] + ASFLOAT(VSRC[LANEID]) * P10[LANEID]
```

#### V_INTERP_P2_F32

Opcode: 1 (0x1)  
Syntax: V_INTERP_P1_F32 VDST, VSRC, ATTR.ATTRCHAN  
Description: Instruction does the second step of the interpolation (P20*J + D). The J
coordinate given in VSRC register.  
NOTE: The indices in LDS is dword indices.  
NOTE: VDST and VSRC registers must not be same.  
Operation:  
```
UINT S = 12*(ATTR*NUMPRIM + PRIMID(LANEID>>2))
FLOAT P20[LANEID] = ASFLOAT(LDS[S + ATTRCHAN + 8])
VDST[LANEID] = ASFLOAT(VDST[LANEID]) + ASFLOAT(VSRC[LANEID]) * P20[LANEID]
```
