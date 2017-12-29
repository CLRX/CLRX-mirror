## GCN ISA DS instructions

These instructions access to local or global data share (LDS/GDS) memory.

List of fields for the DS encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-7   | OFFSET0  | 8-bit offset 0 for VDATA0
8-15  | OFFSET1  | 8-bit offset 1 for VDATA1
0-15  | OFFSET   | 16-bit offset for single DATA
17    | GDS      | Access to GDS instead LDS
18-25 | OPCODE   | Operation code (GCN 1.0/1.1)
17-24 | OPCODE   | Operation code (GCN 1.2)
26-31 | ENCODING | Encoding type. Must be 0b110110
32-39 | ADDR     | Vector register that holds byte address
40-47 | VDATA0   | Source data 0 vector register
48-55 | VDATA1   | Source data 1 vector register
56-63 | VDST     | Destination vector register

Syntax: INSTRUCTION ADDR, VDATA0 [offset:OFFSET] [GDS]  
Syntax: INSTRUCTION ADDR, VDATA0, VDATA1 [offset0:OFFSET0] [offset1:OFFSET1] [GDS]  
VDST syntax: INSTRUCTION VDST, ADDR [offset:OFFSET] [GDS]

NOTE: Any operation on LDS requires correctly set M0 register, prior to execution.
The M0 register holds maximum size of a LDS memory, that can be accessed by wavefront.
To set no limits, just set a M0 register to 0xffffffff.

NOTE: Any operation on GDS requires correctly set M0 register, prior to execution.
The M0 register holds maximum size of a GDS memory in bits 0-15,
that can be accessed by kernel, and a GDS base offset (in bytes) in bits 16-31.

Any DS instruction return data in order (including DS_SWIZZLE_B32) and increments LGKM_CNT.
Any operation increments LGKM by one, and decremented by one if it will be finished.

List of the instructions by opcode (GCN 1.0/1.1):

 Opcode     |GCN 1.0|GCN 1.1| Mnemonic
------------|-------|-------|------------------------
 0 (0x0)    |   ✓   |   ✓   | DS_ADD_U32
 1 (0x1)    |   ✓   |   ✓   | DS_SUB_U32
 2 (0x2)    |   ✓   |   ✓   | DS_RSUB_U32
 3 (0x3)    |   ✓   |   ✓   | DS_INC_U32
 4 (0x4)    |   ✓   |   ✓   | DS_DEC_U32
 5 (0x5)    |   ✓   |   ✓   | DS_MIN_I32
 6 (0x6)    |   ✓   |   ✓   | DS_MAX_I32
 7 (0x7)    |   ✓   |   ✓   | DS_MIN_U32
 8 (0x8)    |   ✓   |   ✓   | DS_MAX_U32
 9 (0x9)    |   ✓   |   ✓   | DS_AND_B32
 10 (0xa)   |   ✓   |   ✓   | DS_OR_B32
 11 (0xb)   |   ✓   |   ✓   | DS_XOR_B32
 12 (0xc)   |   ✓   |   ✓   | DS_MSKOR_B32
 13 (0xd)   |   ✓   |   ✓   | DS_WRITE_B32
 14 (0xe)   |   ✓   |   ✓   | DS_WRITE2_B32
 15 (0xf)   |   ✓   |   ✓   | DS_WRITE2ST64_B32
 16 (0x10)  |   ✓   |   ✓   | DS_CMPST_B32
 17 (0x11)  |   ✓   |   ✓   | DS_CMPST_F32
 18 (0x12)  |   ✓   |   ✓   | DS_MIN_F32
 19 (0x13)  |   ✓   |   ✓   | DS_MAX_F32
 20 (0x14)  |       |   ✓   | DS_NOP
 24 (0x18)  |       |   ✓   | DS_GWS_SEMA_RELEASE_ALL
 25 (0x19)  |   ✓   |   ✓   | DS_GWS_INIT
 26 (0x1a)  |   ✓   |   ✓   | DS_GWS_SEMA_V
 27 (0x1b)  |   ✓   |   ✓   | DS_GWS_SEMA_BR
 28 (0x1c)  |   ✓   |   ✓   | DS_GWS_SEMA_P
 29 (0x1d)  |   ✓   |   ✓   | DS_GWS_BARRIER
 30 (0x1e)  |   ✓   |   ✓   | DS_WRITE_B8
 31 (0x1f)  |   ✓   |   ✓   | DS_WRITE_B16
 32 (0x20)  |   ✓   |   ✓   | DS_ADD_RTN_U32
 33 (0x21)  |   ✓   |   ✓   | DS_SUB_RTN_U32
 34 (0x22)  |   ✓   |   ✓   | DS_RSUB_RTN_U32
 35 (0x23)  |   ✓   |   ✓   | DS_INC_RTN_U32
 36 (0x24)  |   ✓   |   ✓   | DS_DEC_RTN_U32
 37 (0x25)  |   ✓   |   ✓   | DS_MIN_RTN_I32
 38 (0x26)  |   ✓   |   ✓   | DS_MAX_RTN_I32
 39 (0x27)  |   ✓   |   ✓   | DS_MIN_RTN_U32
 40 (0x28)  |   ✓   |   ✓   | DS_MAX_RTN_U32
 41 (0x29)  |   ✓   |   ✓   | DS_AND_RTN_B32
 42 (0x2a)  |   ✓   |   ✓   | DS_OR_RTN_B32
 43 (0x2b)  |   ✓   |   ✓   | DS_XOR_RTN_B32
 44 (0x2c)  |   ✓   |   ✓   | DS_MSKOR_RTN_B32
 45 (0x2d)  |   ✓   |   ✓   | DS_WRXCHG_RTN_B32
 46 (0x2e)  |   ✓   |   ✓   | DS_WRXCHG2_RTN_B32
 47 (0x2f)  |   ✓   |   ✓   | DS_WRXCHG2ST64_RTN_B32
 48 (0x30)  |   ✓   |   ✓   | DS_CMPST_RTN_B32
 49 (0x31)  |   ✓   |   ✓   | DS_CMPST_RTN_F32
 50 (0x32)  |   ✓   |   ✓   | DS_MIN_RTN_F32
 51 (0x33)  |   ✓   |   ✓   | DS_MAX_RTN_F32
 52 (0x34)  |       |   ✓   | DS_WRAP_RTN_B32
 53 (0x35)  |   ✓   |   ✓   | DS_SWIZZLE_B32
 54 (0x36)  |   ✓   |   ✓   | DS_READ_B32
 55 (0x37)  |   ✓   |   ✓   | DS_READ2_B32
 56 (0x38)  |   ✓   |   ✓   | DS_READ2ST64_B32
 57 (0x39)  |   ✓   |   ✓   | DS_READ_I8
 58 (0x3a)  |   ✓   |   ✓   | DS_READ_U8
 59 (0x3b)  |   ✓   |   ✓   | DS_READ_I16
 60 (0x3c)  |   ✓   |   ✓   | DS_READ_U16
 61 (0x3d)  |   ✓   |   ✓   | DS_CONSUME
 62 (0x3e)  |   ✓   |   ✓   | DS_APPEND
 63 (0x3f)  |   ✓   |   ✓   | DS_ORDERED_COUNT (???)
 64 (0x40)  |   ✓   |   ✓   | DS_ADD_U64
 65 (0x41)  |   ✓   |   ✓   | DS_SUB_U64
 66 (0x42)  |   ✓   |   ✓   | DS_RSUB_U64
 67 (0x43)  |   ✓   |   ✓   | DS_INC_U64
 68 (0x44)  |   ✓   |   ✓   | DS_DEC_U64
 69 (0x45)  |   ✓   |   ✓   | DS_MIN_I64
 70 (0x46)  |   ✓   |   ✓   | DS_MAX_I64
 71 (0x47)  |   ✓   |   ✓   | DS_MIN_U64
 72 (0x48)  |   ✓   |   ✓   | DS_MAX_U64
 73 (0x49)  |   ✓   |   ✓   | DS_AND_B64
 74 (0x4a)  |   ✓   |   ✓   | DS_OR_B64
 75 (0x4b)  |   ✓   |   ✓   | DS_XOR_B64
 76 (0x4c)  |   ✓   |   ✓   | DS_MSKOR_B64
 77 (0x4d)  |   ✓   |   ✓   | DS_WRITE_B64
 78 (0x4e)  |   ✓   |   ✓   | DS_WRITE2_B64
 79 (0x4f)  |   ✓   |   ✓   | DS_WRITE2ST64_B64
 80 (0x50)  |   ✓   |   ✓   | DS_CMPST_B64
 81 (0x51)  |   ✓   |   ✓   | DS_CMPST_F64
 82 (0x52)  |   ✓   |   ✓   | DS_MIN_F64
 83 (0x53)  |   ✓   |   ✓   | DS_MAX_F64
 96 (0x60)  |   ✓   |   ✓   | DS_ADD_RTN_U64
 97 (0x61)  |   ✓   |   ✓   | DS_SUB_RTN_U64
 98 (0x62)  |   ✓   |   ✓   | DS_RSUB_RTN_U64
 99 (0x63)  |   ✓   |   ✓   | DS_INC_RTN_U64
 100 (0x64) |   ✓   |   ✓   | DS_DEC_RTN_U64
 101 (0x65) |   ✓   |   ✓   | DS_MIN_RTN_I64
 102 (0x66) |   ✓   |   ✓   | DS_MAX_RTN_I64
 103 (0x67) |   ✓   |   ✓   | DS_MIN_RTN_U64
 104 (0x68) |   ✓   |   ✓   | DS_MAX_RTN_U64
 105 (0x69) |   ✓   |   ✓   | DS_AND_RTN_B64
 106 (0x6a) |   ✓   |   ✓   | DS_OR_RTN_B64
 107 (0x6b) |   ✓   |   ✓   | DS_XOR_RTN_B64
 108 (0x6c) |   ✓   |   ✓   | DS_MSKOR_RTN_B64
 109 (0x6d) |   ✓   |   ✓   | DS_WRXCHG_RTN_B64
 110 (0x6e) |   ✓   |   ✓   | DS_WRXCHG2_RTN_B64
 111 (0x6f) |   ✓   |   ✓   | DS_WRXCHG2ST64_RTN_B64
 112 (0x70) |   ✓   |   ✓   | DS_CMPST_RTN_B64
 113 (0x71) |   ✓   |   ✓   | DS_CMPST_RTN_F64
 114 (0x72) |   ✓   |   ✓   | DS_MIN_RTN_F64
 115 (0x73) |   ✓   |   ✓   | DS_MAX_RTN_F64
 118 (0x76) |   ✓   |   ✓   | DS_READ_B64
 119 (0x77) |   ✓   |   ✓   | DS_READ2_B64
 120 (0x78) |   ✓   |   ✓   | DS_READ2ST64_B64
 126 (0x7e) |       |   ✓   | DS_CONDXCHG32_RTN_B64
 128 (0x80) |   ✓   |   ✓   | DS_ADD_SRC2_U32
 129 (0x81) |   ✓   |   ✓   | DS_SUB_SRC2_U32
 130 (0x82) |   ✓   |   ✓   | DS_RSUB_SRC2_U32
 131 (0x83) |   ✓   |   ✓   | DS_INC_SRC2_U32
 132 (0x84) |   ✓   |   ✓   | DS_DEC_SRC2_U32
 133 (0x85) |   ✓   |   ✓   | DS_MIN_SRC2_I32
 134 (0x86) |   ✓   |   ✓   | DS_MAX_SRC2_I32
 135 (0x87) |   ✓   |   ✓   | DS_MIN_SRC2_U32
 136 (0x88) |   ✓   |   ✓   | DS_MAX_SRC2_U32
 137 (0x89) |   ✓   |   ✓   | DS_AND_SRC2_B32
 138 (0x8a) |   ✓   |   ✓   | DS_OR_SRC2_B32
 139 (0x8b) |   ✓   |   ✓   | DS_XOR_SRC2_B32
 141 (0x8d) |   ✓   |   ✓   | DS_WRITE_SRC2_B32
 146 (0x92) |   ✓   |   ✓   | DS_MIN_SRC2_F32
 147 (0x93) |   ✓   |   ✓   | DS_MAX_SRC2_F32
 192 (0xc0) |   ✓   |   ✓   | DS_ADD_SRC2_U64
 193 (0xc1) |   ✓   |   ✓   | DS_SUB_SRC2_U64
 194 (0xc2) |   ✓   |   ✓   | DS_RSUB_SRC2_U64
 195 (0xc3) |   ✓   |   ✓   | DS_INC_SRC2_U64
 196 (0xc4) |   ✓   |   ✓   | DS_DEC_SRC2_U64
 197 (0xc5) |   ✓   |   ✓   | DS_MIN_SRC2_I64
 198 (0xc6) |   ✓   |   ✓   | DS_MAX_SRC2_I64
 199 (0xc7) |   ✓   |   ✓   | DS_MIN_SRC2_U64
 200 (0xc8) |   ✓   |   ✓   | DS_MAX_SRC2_U64
 201 (0xc9) |   ✓   |   ✓   | DS_AND_SRC2_B64
 202 (0xca) |   ✓   |   ✓   | DS_OR_SRC2_B64
 203 (0xcb) |   ✓   |   ✓   | DS_XOR_SRC2_B64
 205 (0xcd) |   ✓   |   ✓   | DS_WRITE_SRC2_B64
 210 (0xd2) |   ✓   |   ✓   | DS_MIN_SRC2_F64
 211 (0xd3) |   ✓   |   ✓   | DS_MAX_SRC2_F64
 222 (0xde) |       |   ✓   | DS_WRITE_B96
 223 (0xdf) |       |   ✓   | DS_WRITE_B128
 253 (0xfd) |       |   ✓   | DS_CONDXCHG32_RTN_B128
 254 (0xfe) |       |   ✓   | DS_READ_B96
 255 (0xff) |       |   ✓   | DS_READ_B128

List of the instructions by opcode (GCN 1.2/1.4):

 Opcode     |GCN 1.2|GCN 1.4| Mnemonic
------------|-------|-------|-----------------------------
 0 (0x0)    |   ✓   |   ✓   | DS_ADD_U32
 1 (0x1)    |   ✓   |   ✓   | DS_SUB_U32
 2 (0x2)    |   ✓   |   ✓   | DS_RSUB_U32
 3 (0x3)    |   ✓   |   ✓   | DS_INC_U32
 4 (0x4)    |   ✓   |   ✓   | DS_DEC_U32
 5 (0x5)    |   ✓   |   ✓   | DS_MIN_I32
 6 (0x6)    |   ✓   |   ✓   | DS_MAX_I32
 7 (0x7)    |   ✓   |   ✓   | DS_MIN_U32
 8 (0x8)    |   ✓   |   ✓   | DS_MAX_U32
 9 (0x9)    |   ✓   |   ✓   | DS_AND_B32
 10 (0xa)   |   ✓   |   ✓   | DS_OR_B32
 11 (0xb)   |   ✓   |   ✓   | DS_XOR_B32
 12 (0xc)   |   ✓   |   ✓   | DS_MSKOR_B32
 13 (0xd)   |   ✓   |   ✓   | DS_WRITE_B32
 14 (0xe)   |   ✓   |   ✓   | DS_WRITE2_B32
 15 (0xf)   |   ✓   |   ✓   | DS_WRITE2ST64_B32
 16 (0x10)  |   ✓   |   ✓   | DS_CMPST_B32
 17 (0x11)  |   ✓   |   ✓   | DS_CMPST_F32
 18 (0x12)  |   ✓   |   ✓   | DS_MIN_F32
 19 (0x13)  |   ✓   |   ✓   | DS_MAX_F32
 20 (0x14)  |   ✓   |   ✓   | DS_NOP
 21 (0x15)  |   ✓   |   ✓   | DS_ADD_F32
 29 (0x1d)  |       |   ✓   | DS_WRITE_ADDTID_B32
 30 (0x1e)  |   ✓   |   ✓   | DS_WRITE_B8
 31 (0x1f)  |   ✓   |   ✓   | DS_WRITE_B16
 32 (0x20)  |   ✓   |   ✓   | DS_ADD_RTN_U32
 33 (0x21)  |   ✓   |   ✓   | DS_SUB_RTN_U32
 34 (0x22)  |   ✓   |   ✓   | DS_RSUB_RTN_U32
 35 (0x23)  |   ✓   |   ✓   | DS_INC_RTN_U32
 36 (0x24)  |   ✓   |   ✓   | DS_DEC_RTN_U32
 37 (0x25)  |   ✓   |   ✓   | DS_MIN_RTN_I32
 38 (0x26)  |   ✓   |   ✓   | DS_MAX_RTN_I32
 39 (0x27)  |   ✓   |   ✓   | DS_MIN_RTN_U32
 40 (0x28)  |   ✓   |   ✓   | DS_MAX_RTN_U32
 41 (0x29)  |   ✓   |   ✓   | DS_AND_RTN_B32
 42 (0x2a)  |   ✓   |   ✓   | DS_OR_RTN_B32
 43 (0x2b)  |   ✓   |   ✓   | DS_XOR_RTN_B32
 44 (0x2c)  |   ✓   |   ✓   | DS_MSKOR_RTN_B32
 45 (0x2d)  |   ✓   |   ✓   | DS_WRXCHG_RTN_B32
 46 (0x2e)  |   ✓   |   ✓   | DS_WRXCHG2_RTN_B32
 47 (0x2f)  |   ✓   |   ✓   | DS_WRXCHG2ST64_RTN_B32
 48 (0x30)  |   ✓   |   ✓   | DS_CMPST_RTN_B32
 49 (0x31)  |   ✓   |   ✓   | DS_CMPST_RTN_F32
 50 (0x32)  |   ✓   |   ✓   | DS_MIN_RTN_F32
 51 (0x33)  |   ✓   |   ✓   | DS_MAX_RTN_F32
 52 (0x34)  |   ✓   |   ✓   | DS_WRAP_RTN_B32
 53 (0x35)  |   ✓   |   ✓   | DS_ADD_RTN_F32
 54 (0x36)  |   ✓   |   ✓   | DS_READ_B32
 55 (0x37)  |   ✓   |   ✓   | DS_READ2_B32
 56 (0x38)  |   ✓   |   ✓   | DS_READ2ST64_B32
 57 (0x39)  |   ✓   |   ✓   | DS_READ_I8
 58 (0x3a)  |   ✓   |   ✓   | DS_READ_U8
 59 (0x3b)  |   ✓   |   ✓   | DS_READ_I16
 60 (0x3c)  |   ✓   |   ✓   | DS_READ_U16
 61 (0x3d)  |   ✓   |   ✓   | DS_SWIZZLE_B32
 62 (0x3e)  |   ✓   |   ✓   | DS_PERMUTE_B32
 63 (0x3f)  |   ✓   |   ✓   | DS_BPERMUTE_B32
 64 (0x40)  |   ✓   |   ✓   | DS_ADD_U64
 65 (0x41)  |   ✓   |   ✓   | DS_SUB_U64
 66 (0x42)  |   ✓   |   ✓   | DS_RSUB_U64
 67 (0x43)  |   ✓   |   ✓   | DS_INC_U64
 68 (0x44)  |   ✓   |   ✓   | DS_DEC_U64
 69 (0x45)  |   ✓   |   ✓   | DS_MIN_I64
 70 (0x46)  |   ✓   |   ✓   | DS_MAX_I64
 71 (0x47)  |   ✓   |   ✓   | DS_MIN_U64
 72 (0x48)  |   ✓   |   ✓   | DS_MAX_U64
 73 (0x49)  |   ✓   |   ✓   | DS_AND_B64
 74 (0x4a)  |   ✓   |   ✓   | DS_OR_B64
 75 (0x4b)  |   ✓   |   ✓   | DS_XOR_B64
 76 (0x4c)  |   ✓   |   ✓   | DS_MSKOR_B64
 77 (0x4d)  |   ✓   |   ✓   | DS_WRITE_B64
 78 (0x4e)  |   ✓   |   ✓   | DS_WRITE2_B64
 79 (0x4f)  |   ✓   |   ✓   | DS_WRITE2ST64_B64
 80 (0x50)  |   ✓   |   ✓   | DS_CMPST_B64
 81 (0x51)  |   ✓   |   ✓   | DS_CMPST_F64
 82 (0x52)  |   ✓   |   ✓   | DS_MIN_F64
 83 (0x53)  |   ✓   |   ✓   | DS_MAX_F64
 84 (0x54)  |       |   ✓   | DS_WRITE_B8_D16_HI
 85 (0x55)  |       |   ✓   | DS_WRITE_B16_D16_HI
 86 (0x56)  |       |   ✓   | DS_READ_U8_D16
 87 (0x57)  |       |   ✓   | DS_READ_U8_D16_HI
 88 (0x58)  |       |   ✓   | DS_READ_I8_D16
 89 (0x59)  |       |   ✓   | DS_READ_I8_D16_HI
 90 (0x5a)  |       |   ✓   | DS_READ_U16_D16
 91 (0x5b)  |       |   ✓   | DS_READ_U16_D16_HI
 96 (0x60)  |   ✓   |   ✓   | DS_ADD_RTN_U64
 97 (0x61)  |   ✓   |   ✓   | DS_SUB_RTN_U64
 98 (0x62)  |   ✓   |   ✓   | DS_RSUB_RTN_U64
 99 (0x63)  |   ✓   |   ✓   | DS_INC_RTN_U64
 100 (0x64) |   ✓   |   ✓   | DS_DEC_RTN_U64
 101 (0x65) |   ✓   |   ✓   | DS_MIN_RTN_I64
 102 (0x66) |   ✓   |   ✓   | DS_MAX_RTN_I64
 103 (0x67) |   ✓   |   ✓   | DS_MIN_RTN_U64
 104 (0x68) |   ✓   |   ✓   | DS_MAX_RTN_U64
 105 (0x69) |   ✓   |   ✓   | DS_AND_RTN_B64
 106 (0x6a) |   ✓   |   ✓   | DS_OR_RTN_B64
 107 (0x6b) |   ✓   |   ✓   | DS_XOR_RTN_B64
 108 (0x6c) |   ✓   |   ✓   | DS_MSKOR_RTN_B64
 109 (0x6d) |   ✓   |   ✓   | DS_WRXCHG_RTN_B64
 110 (0x6e) |   ✓   |   ✓   | DS_WRXCHG2_RTN_B64
 111 (0x6f) |   ✓   |   ✓   | DS_WRXCHG2ST64_RTN_B64
 112 (0x70) |   ✓   |   ✓   | DS_CMPST_RTN_B64
 113 (0x71) |   ✓   |   ✓   | DS_CMPST_RTN_F64
 114 (0x72) |   ✓   |   ✓   | DS_MIN_RTN_F64
 115 (0x73) |   ✓   |   ✓   | DS_MAX_RTN_F64
 118 (0x76) |   ✓   |   ✓   | DS_READ_B64
 119 (0x77) |   ✓   |   ✓   | DS_READ2_B64
 120 (0x78) |   ✓   |   ✓   | DS_READ2ST64_B64
 126 (0x7e) |   ✓   |   ✓   | DS_CONDXCHG32_RTN_B64
 128 (0x80) |   ✓   |   ✓   | DS_ADD_SRC2_U32
 129 (0x81) |   ✓   |   ✓   | DS_SUB_SRC2_U32
 130 (0x82) |   ✓   |   ✓   | DS_RSUB_SRC2_U32
 131 (0x83) |   ✓   |   ✓   | DS_INC_SRC2_U32
 132 (0x84) |   ✓   |   ✓   | DS_DEC_SRC2_U32
 133 (0x85) |   ✓   |   ✓   | DS_MIN_SRC2_I32
 134 (0x86) |   ✓   |   ✓   | DS_MAX_SRC2_I32
 135 (0x87) |   ✓   |   ✓   | DS_MIN_SRC2_U32
 136 (0x88) |   ✓   |   ✓   | DS_MAX_SRC2_U32
 137 (0x89) |   ✓   |   ✓   | DS_AND_SRC2_B32
 138 (0x8a) |   ✓   |   ✓   | DS_OR_SRC2_B32
 139 (0x8b) |   ✓   |   ✓   | DS_XOR_SRC2_B32
 141 (0x8d) |   ✓   |   ✓   | DS_WRITE_SRC2_B32
 146 (0x92) |   ✓   |   ✓   | DS_MIN_SRC2_F32
 147 (0x93) |   ✓   |   ✓   | DS_MAX_SRC2_F32
 149 (0x95) |   ✓   |   ✓   | DS_ADD_SRC2_F32
 152 (0x98) |   ✓   |   ✓   | DS_GWS_SEMA_RELEASE_ALL
 153 (0x99) |   ✓   |   ✓   | DS_GWS_INIT
 154 (0x9a) |   ✓   |   ✓   | DS_GWS_SEMA_V
 155 (0x9b) |   ✓   |   ✓   | DS_GWS_SEMA_BR
 156 (0x9c) |   ✓   |   ✓   | DS_GWS_SEMA_P
 157 (0x9d) |   ✓   |   ✓   | DS_GWS_BARRIER
 182 (0xb6) |       |   ✓   | DS_READ_ADDTID_B32
 189 (0xbd) |   ✓   |   ✓   | DS_CONSUME
 190 (0xbe) |   ✓   |   ✓   | DS_APPEND
 191 (0xbf) |   ✓   |   ✓   | DS_ORDERED_COUNT
 192 (0xc0) |   ✓   |   ✓   | DS_ADD_SRC2_U64
 193 (0xc1) |   ✓   |   ✓   | DS_SUB_SRC2_U64
 194 (0xc2) |   ✓   |   ✓   | DS_RSUB_SRC2_U64
 195 (0xc3) |   ✓   |   ✓   | DS_INC_SRC2_U64
 196 (0xc4) |   ✓   |   ✓   | DS_DEC_SRC2_U64
 197 (0xc5) |   ✓   |   ✓   | DS_MIN_SRC2_I64
 198 (0xc6) |   ✓   |   ✓   | DS_MAX_SRC2_I64
 199 (0xc7) |   ✓   |   ✓   | DS_MIN_SRC2_U64
 200 (0xc8) |   ✓   |   ✓   | DS_MAX_SRC2_U64
 201 (0xc9) |   ✓   |   ✓   | DS_AND_SRC2_B64
 202 (0xca) |   ✓   |   ✓   | DS_OR_SRC2_B64
 203 (0xcb) |   ✓   |   ✓   | DS_XOR_SRC2_B64
 205 (0xcd) |   ✓   |   ✓   | DS_WRITE_SRC2_B64
 210 (0xd2) |   ✓   |   ✓   | DS_MIN_SRC2_F64
 211 (0xd3) |   ✓   |   ✓   | DS_MAX_SRC2_F64
 222 (0xde) |   ✓   |   ✓   | DS_WRITE_B96
 223 (0xdf) |   ✓   |   ✓   | DS_WRITE_B128
 253 (0xfd) |   ✓   |   ✓   | DS_CONDXCHG32_RTN_B128
 254 (0xfe) |   ✓   |   ✓   | DS_READ_B96
 255 (0xff) |   ✓   |   ✓   | DS_READ_B128

### Instruction set

Alphabetically sorted instruction list:

#### DS_ADD_F32

Opcode: 21 (0x15) for GCN 1.2/1.4  
Syntax: DS_ADD_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Add single float value from LDS/GDS at address (ADDR+OFFSET) & ~3 and
VDATA0, and store result back to LDS/GDS at this address as single float value.
Operation is atomic.  
Operation:  
```
FLOAT* V = (FLOAT*)(DS + ((ADDR+OFFSET)&~3))
*V = *V + ASFLOAT(VDATA0)  // atomic operation
```

#### DS_ADD_RTN_U32

Opcode: 32 (0x20)  
Syntax: DS_ADD_RTN_U32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Add unsigned integer value from LDS/GDS at address (ADDR+OFFSET) & ~3 and
VDATA0, and store result back to LDS/GDS at this address. Previous value from
LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = *V + VDATA0  // atomic operation
```

#### DS_ADD_RTN_U64

Opcode: 96 (0x60)  
Syntax: DS_ADD_RTN_U64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Add unsigned 64-bit integer value from LDS/GDS at address (ADDR+OFFSET) & ~7
and VDATA0, and store result back to LDS/GDS at this address. Previous value from
LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = *V + VDATA0  // atomic operation
```

#### DS_ADD_SRC2_U32

Opcode: 128 (0x80)  
Syntax: DS_ADD_SRC2_U32 ADDR [OFFSET:OFFSET]  
Description: Add unsigned integer value from LDS/GDS at address A and B, and
store result back to LDS/GDS at address A. Refer to listing to learn about addressing.
Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = *V + *(UINT32*)(DS + B) // atomic operation
```

#### DS_ADD_SRC2_U64

Opcode: 192 (0xc0)  
Syntax: DS_ADD_SRC2_U64 ADDR [OFFSET:OFFSET]  
Description: Add unsigned 64-bit integer value from LDS/GDS at address A and B, and
store result back to LDS/GDS at address A. Refer to listing to learn about addressing.
Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = *V + *(UINT64*)(DS + B) // atomic operation
```

#### DS_ADD_U32

Opcode: 0 (0x0)  
Syntax: DS_ADD_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Add unsigned integer value from LDS/GDS at address (ADDR+OFFSET) & ~3 and
VDATA0, and store result back to LDS/GDS at this address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = *V + VDATA0  // atomic operation
```

#### DS_ADD_U64

Opcode: 64 (0x40)  
Syntax: DS_ADD_U64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Add unsigned integer 64-bit value from LDS/GDS at address (ADDR+OFFSET) & ~7
and VDATA0, and store result back to LDS/GDS at this address. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = *V + VDATA0  // atomic operation
```

#### DS_AND_B32

Opcode: 9 (0x9)  
Syntax: DS_AND_B32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise AND operation on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = *V & VDATA0  // atomic operation
```

#### DS_AND_B64

Opcode: 73 (0x49)  
Syntax: DS_AND_B64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Do bitwise AND operation on 64-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~7, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = *V & VDATA0  // atomic operation
```

#### DS_AND_RTN_B32

Opcode: 41 (0x29)  
Syntax: DS_AND_RTN_B32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise AND operation on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = *V & VDATA0  // atomic operation
```

#### DS_AND_RTN_B64

Opcode: 105 (0x69)  
Syntax: DS_AND_RTN_B64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Do bitwise AND operation on 64-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~7, and value from VDATA0; and store result to LDS/GDS at this same
address. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = *V & VDATA0  // atomic operation
```

#### DS_AND_SRC2_B32

Opcode: 137 (0x89)  
Syntax: DS_AND_SRC2_B32 ADDR [OFFSET:OFFSET]  
Description: Do bitwise AND operation on 32-bit value from LDS/GDS at address
A, and at address B; and store result to LDS/GDS at address A. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = *V & *(UINT32*)(DS + B) // atomic operation
```

#### DS_AND_SRC2_B64

Opcode: 201 (0xc9)  
Syntax: DS_AND_SRC2_B64 ADDR [OFFSET:OFFSET]  
Description: Do bitwise AND operation on 64-bit value from LDS/GDS at address
A, and at address B; and store result to LDS/GDS at address A. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = *V & *(UINT64*)(DS + B) // atomic operation
```

#### DS_APPEND

Opcode: 62 (0x3e) for GCN 1.0/1.1; 190 (0xbe) GCN 1.2/1.4  
Syntax: DS_APPEND VDST [OFFSET:OFFSET]  
Description: Append entries to buffer. This instruction increments 32-bit value in
LDS/GDS at address OFFSET&~3 by number of the active threads, and
store previous value from LDS/GDS at this same address into VDST.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (OFFSET&~3))
VDST = *V   // scalar operation
*V += BITCOUNT(EXEC)  // scalar operation
```

#### DS_BPERMUTE_B32

Opcode: 63 (0x3f) for GCN 1.2/1.4  
Syntax: DS_BPERMUTE_B32 DST, ADDR, SRC [OFFSET:OFFSET]  
Description: Backward permutation for wave. Put value of SRC0 from 
lane id calculated from `ADDR[(LANEID + (OFFSET>>2)) & 63]`,
to DST register in LANEID. The ADDR holds lane id is multiplied by 4 (size of dword).
Realizes pop semantic: "read data from lane i".
Operation:  
```
UINT tmp[64]
for (BYTE i = 0; i < 64; i++)
{
    UINT32 laneid = ADDR[(i + (OFFSET>>2)) & 63]
    tmp[i] = (EXEC & (1ULL<<laneid)!=0) ?  SRC[laneid] : 0
}
for (BYTE i = 0; i < 64; i++)
    if (EXEC & (1ULL<<i)!=0)
        DST[i] = tmp[i]
```

#### DS_CONSUME

Opcode: 61 (0x3d) for GCN 1.0/1.1; 189 (0xbd) GCN 1.2/1.4  
Syntax: DS_CONSUME VDST [OFFSET:OFFSET]  
Description: Consume entries to buffer. This instruction increments 32-bit value in
LDS/GDS at address OFFSET&~3 by number of the active threads, and
store previous value from LDS/GDS at this same address into VDST.  
Operation:  
```
UINT32* V = (UINT32*)(DS + (OFFSET&~3))
VDST = *V   // scalar operation
*V -= BITCOUNT(EXEC)  // scalar operation
```

#### DS_CMPST_B32

Opcode: 16 (0x10)  
Syntax: DS_CMPST_B32 ADDR, VDATA0, VDATA1 [OFFSET:OFFSET]  
Description: Compare values from VDATA0 and from LDS/GDS at address (ADDR+OFFSET) & ~3.
If values are equal, store VDATA1 to LDS/GDS at this same address, otherwise do nothing.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = (*V==VDATA0) ? VDATA1 : *V  // atomic operation
```

#### DS_CMPST_B64

Opcode: 80 (0x50)  
Syntax: DS_CMPST_B64 ADDR, VDATA0(2), VDATA1(2) [OFFSET:OFFSET]  
Description: Compare 64-bit values from VDATA0 and from LDS/GDS at address
(ADDR+OFFSET) & ~7. If values are equal, store VDATA1 to LDS/GDS at this same address,
otherwise do nothing. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = (*V==VDATA0) ? VDATA1 : *V  // atomic operation
```

#### DS_CMPST_F32

Opcode: 17 (0x11)  
Syntax: DS_CMPST_F32 ADDR, VDATA0, VDATA1 [OFFSET:OFFSET]  
Description: Compare single floating point values from VDATA0 and from LDS/GDS at address
(ADDR+OFFSET) & ~3. If values are equal, store VDATA1 to LDS/GDS at this same address,
otherwise do nothing. Operation is atomic.  
Operation:  
```
FLOAT* V = (FLOAT*)(DS + ((ADDR+OFFSET)&~3))
*V = (*V==ASFLOATR(VDATA0)) ? ASFLOAT(VDATA1) : *V  // atomic operation
```

#### DS_CMPST_F64

Opcode: 81 (0x51)  
Syntax: DS_CMPST_F64 ADDR, VDATA0(2), VDATA1(2) [OFFSET:OFFSET]  
Description: Compare double floating point values from VDATA0 and from LDS/GDS at address
(ADDR+OFFSET) & ~7. If values are equal, store VDATA1 to LDS/GDS at this same address,
otherwise do nothing. Operation is atomic.  
Operation:  
```
DOUBLE* V = (DOUBLE*)(DS + ((ADDR+OFFSET)&~7))
*V = (*V==ASDOUBLE(VDATA0)) ? ASDOUBLE(VDATA1) : *V  // atomic operation
```

#### DS_CMPST_RTN_B32

Opcode: 48 (0x30)  
Syntax: DS_CMPST_RTN_B32 VDST, ADDR, VDATA0, VDATA1 [OFFSET:OFFSET]  
Description: Compare values from VDATA0 and from LDS/GDS at address (ADDR+OFFSET) & ~3.
If values are equal, store VDATA1 to LDS/GDS at this same address, otherwise do nothing.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = VDST; *V = (*V==VDATA0) ? VDATA1 : *V  // atomic operation
```

#### DS_CMPST_RTN_B64

Opcode: 112 (0x70)  
Syntax: DS_CMPST_RTN_B64 VDST(2), ADDR, VDATA0(2), VDATA1(2) [OFFSET:OFFSET]  
Description: Compare 64-bit values from VDATA0 and from LDS/GDS at address
(ADDR+OFFSET) & ~7. If values are equal, store VDATA1 to LDS/GDS at this same address,
otherwise do nothing. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = VDST; *V = (*V==VDATA0) ? VDATA1 : *V  // atomic operation
```

#### DS_CMPST_RTN_F32

Opcode: 49 (0x31)  
Syntax: DS_CMPST_RTN_F32 VDST, ADDR, VDATA0, VDATA1 [OFFSET:OFFSET]  
Description: Compare single floating point values from VDATA0 and from LDS/GDS at address
(ADDR+OFFSET) & ~3. If values are equal, store VDATA1 to LDS/GDS at this same address,
otherwise do nothing. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
FLOAT* V = (FLOAT*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = (*V==ASFLOAT(VDATA0)) ? ASFLOAT(VDATA1) : *V  // atomic operation
```

#### DS_CMPST_RTN_F64

Opcode: 113 (0x71)  
Syntax: DS_CMPST_RTN_F64 VDST(2), ADDR, VDATA0(2), VDATA1(2) [OFFSET:OFFSET]  
Description: Compare double floating point values from VDATA0 and from LDS/GDS at address
(ADDR+OFFSET) & ~7. If values are equal, store VDATA1 to LDS/GDS at this same address,
otherwise do nothing. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
DOUBLE* V = (DOUBLE*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = (*V==ASDOUBLE(VDATA0)) ? ASDOUBLE(VDATA1) : *V  // atomic operation
```

#### DS_DEC_RTN_U32

Opcode: 36 (0x24)  
Syntax: DS_DEC_RTN_U32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Load unsigned value from LDS/GDS at  address (ADDR+OFFSET) & ~3, and
compare with unsigned value from VDATA0. If VDATA0 is greater or equal and loaded
unsigned value is not zero, then increment value from LDS/GDS, otherwise store
VDATA0 to LDS/GDS. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = (VDATA0 >= *V && *V!=0) ? *V-1 : VDATA0  // atomic operation
```

#### DS_DEC_RTN_U64

Opcode: 100 (0x64)  
Syntax: DS_DEC_RTN_U64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Load unsigned 64-bit value from LDS/GDS at  address (ADDR+OFFSET) & ~7, and
compare with unsigned value from VDATA0. If VDATA0 is greater or equal and loaded
unsigned value is not zero, then decrement value from LDS/GDS, otherwise store
VDATA0 to LDS/GDS. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = (VDATA0 >= *V && *V!=0) ? *V-1 : VDATA0  // atomic operation
```

#### DS_DEC_SRC2_U32

Opcode: 132 (0x84)  
Syntax: DS_DEC_SRC2_U32 ADDR [OFFSET:OFFSET]  
Description: Load unsigned value from LDS/GDS at address A, and compare with unsigned
value at address B. If value at address B is greater or equal and loaded unsigned value
is not zero, then decrement value from LDS/GDS, otherwise store value at address B to
LDS/GDS at address A. Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
UINT32* VB = (UINT32*)(DS + B)
*V = (*VB >= *V && *V!=0) ? *V-1 : *VB // atomic operation
```

#### DS_DEC_SRC2_U64

Opcode: 196 (0xc4)  
Syntax: DS_DEC_SRC2_U64 ADDR [OFFSET:OFFSET]  
Description: Load unsigned 64-bit value from LDS/GDS at address A, and compare with
unsigned 64-bit value at address B. If value at address B is greater or equal and loaded
unsigned value is not zero, then decrement value from LDS/GDS, otherwise store value
at address B to LDS/GDS at address A. Refer to listing to learn about addressing.
Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
UINT64* VB = (UINT64*)(DS + B)
*V = (*VB >= *V && *V!=0) ? *V-1 : *VB // atomic operation
```

#### DS_DEC_U32

Opcode: 4 (0x4)  
Syntax: DS_DEC_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Load unsigned value from LDS/GDS at  address (ADDR+OFFSET) & ~3, and
compare with unsigned value from VDATA0. If VDATA0 is greater or equal and loaded
unsigned value is not zero, then decrement value from LDS/GDS, otherwise store
VDATA0 to LDS/GDS. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = (VDATA0 >= *V && *V!=0) ? *V-1 : VDATA0  // atomic operation
```

#### DS_DEC_U64

Opcode: 68 (0x44)  
Syntax: DS_DEC_U64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Load unsigned 64-bit value from LDS/GDS at  address (ADDR+OFFSET) & ~7, and
compare with unsigned value from VDATA0. If VDATA0 is greater or equal and loaded
unsigned value is not zero, then decrement value from LDS/GDS, otherwise store
VDATA0 to LDS/GDS. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = (VDATA0 >= *V && *V!=0) ? *V-1 : VDATA0  // atomic operation
```

#### DS_INC_RTN_U32

Opcode: 35 (0x23)  
Syntax: DS_INC_RTN_U32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Load unsigned value from LDS/GDS at address (ADDR+OFFSET) & ~3, and
compare with unsigned value from VDATA0. If VDATA0 greater, then increment value
from LDS/GDS, otherwise store 0 to LDS/GDS.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = (VDATA0 > *V) ? *V+1 : 0  // atomic operation
```

#### DS_INC_RTN_U64

Opcode: 99 (0x63)  
Syntax: DS_INC_RTN_U64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Load unsigned 64-bit value from LDS/GDS at address (ADDR+OFFSET) & ~7, and
compare with unsigned value from VDATA0. If VDATA0 is greater, then increment value
from LDS/GDS, otherwise store 0 to LDS/GDS.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = (VDATA0 > *V) ? *V+1 : 0  // atomic operation
```

#### DS_INC_SRC2_U32

Opcode: 131 (0x83)  
Syntax: DS_INC_SRC2_U32 ADDR [OFFSET:OFFSET]  
Description: Load unsigned value from LDS/GDS at address A, and
compare with unsigned value at address B. If value at address B is greater,
then increment value from LDS/GDS, otherwise store 0 to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = (*(UINT32*)(DS + B) > *V) ? *V+1 : 0  // atomic operation
```

#### DS_INC_SRC2_U64

Opcode: 195 (0xc3)  
Syntax: DS_INC_SRC2_U64 ADDR [OFFSET:OFFSET]  
Description: Load unsigned 64-bit value from LDS/GDS at address A, and
compare with unsigned value at address B. If value at address B is greater,
then increment value from LDS/GDS, otherwise store 0 to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = (*(UINT64*)(DS + B) > *V) ? *V+1 : 0  // atomic operation
```

#### DS_INC_U32

Opcode: 3 (0x3)  
Syntax: DS_INC_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Load unsigned value from LDS/GDS at address (ADDR+OFFSET) & ~3, and
compare with unsigned value from VDATA0. If VDATA0 is greater, then increment value
from LDS/GDS, otherwise store 0 to LDS/GDS. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = (VDATA0 > *V) ? *V+1 : 0  // atomic operation
```

#### DS_INC_U64

Opcode: 67 (0x43)  
Syntax: DS_INC_U64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Load unsigned 64-bit value from LDS/GDS at address (ADDR+OFFSET) & ~7, and
compare with unsigned value from VDATA0. If VDATA0 is greater, then increment value
from LDS/GDS, otherwise store 0 to LDS/GDS. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = (VDATA0 > *V) ? *V+1 : 0  // atomic operation
```

#### DS_MAX_F32

Opcode: 19 (0x13)  
Syntax: DS_MAX_F32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose greatest single floating point value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
FLOAT* V = (FLOAT*)(DS + ((ADDR+OFFSET)&~3))
*V = MAX(*V, ASFLOAT(VDATA0)) // atomic operation
```

#### DS_MAX_F64

Opcode: 83 (0x53)  
Syntax: DS_MAX_F64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose greatest double floating point value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
DOUBLE* V = (DOUBLE*)(DS + ((ADDR+OFFSET)&~7))
*V = MAX(*V, ASDOUBLE(VDATA0)) // atomic operation
```

#### DS_MAX_I32

Opcode: 6 (0x6)  
Syntax: DS_MAX_I32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose greatest signed integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
INT32* V = (INT32*)(DS + ((ADDR+OFFSET)&~3))
*V = MAX(*V, (INT32)VDATA0) // atomic operation
```

#### DS_MAX_I64

Opcode: 70 (0x46)  
Syntax: DS_MAX_I64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose greatest signed 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
INT64* V = (INT64*)(DS + ((ADDR+OFFSET)&~7))
*V = MAX(*V, (INT64)VDATA0) // atomic operation
```

#### DS_MAX_RTN_F32

Opcode: 51 (0x33)  
Syntax: DS_MAX_F32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest single floating point value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
FLOAT* V = (FLOAT*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = MAX(*V, ASFLOAT(VDATA0)) // atomic operation
```

#### DS_MAX_RTN_F64

Opcode: 115 (0x73)  
Syntax: DS_MAX_F64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose smallest double floating point value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
DOUBLE* V = (DOUBLE*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = MAX(*V, ASDOUBLE(VDATA0)) // atomic operation
```

#### DS_MAX_RTN_I32

Opcode: 38 (0x26)  
Syntax: DS_MAX_RTN_I32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose greatest signed integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
INT32* V = (INT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = MAX(*V, (INT32)VDATA0) // atomic operation
```

#### DS_MAX_RTN_I64

Opcode: 102 (0x66)  
Syntax: DS_MAX_RTN_I64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose greatest signed 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
INT64* V = (INT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = MAX(*V, (INT64)VDATA0) // atomic operation
```

#### DS_MAX_RTN_U32

Opcode: 40 (0x28)  
Syntax: DS_MAX_RTN_U32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose greatest unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = MAX(*V, VDATA0) // atomic operation
```

#### DS_MAX_RTN_U64

Opcode: 104 (0x68)  
Syntax: DS_MAX_RTN_U64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose greatest unsigned 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = MAX(*V, VDATA0) // atomic operation
```

#### DS_MAX_SRC2_F32

Opcode: 147 (0x93)  
Syntax: DS_MAX_SRC2_F32 ADDR [OFFSET:OFFSET]  
Description: Choose greatest single floating point value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
FLOAT* V = (FLOAT*)(DS + A)
*V = MAX(*V, *(FLOAT*)(DS + B)) // atomic operation
```

#### DS_MAX_SRC2_F64

Opcode: 211 (0xd3)  
Syntax: DS_MAX_SRC2_F64 ADDR [OFFSET:OFFSET]  
Description: Choose greatest double floating point value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
DOUBLE* V = (DOUBLE*)(DS + A)
*V = MAX(*V, *(DOUBLE*)(DS + B)) // atomic operation
```

#### DS_MAX_SRC2_I32

Opcode: 134 (0x86)  
Syntax: DS_MAX_SRC2_I32 ADDR [OFFSET:OFFSET]  
Description: Choose greatest signed integer value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
INT32* V = (INT32*)(DS + A)
*V = MAX(*V, *(INT32*)(DS + B)) // atomic operation
```

#### DS_MAX_SRC2_I64

Opcode: 198 (0x86)  
Syntax: DS_MAX_SRC2_I64 ADDR [OFFSET:OFFSET]  
Description: Choose greatest signed 64-bit integer value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
INT64* V = (INT64*)(DS + A)
*V = MAX(*V, *(INT64*)(DS + B)) // atomic operation
```

#### DS_MAX_SRC2_U32

Opcode: 136 (0x88)  
Syntax: DS_MAX_SRC2_U32 ADDR [OFFSET:OFFSET]  
Description: Choose greatest unsigned integer value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = MAX(*V, *(UINT32*)(DS + B)) // atomic operation
```

#### DS_MAX_SRC2_U64

Opcode: 200 (0xc8)  
Syntax: DS_MAX_SRC2_U64 ADDR [OFFSET:OFFSET]  
Description: Choose greatest unsigned 64-bit integer value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = MAX(*V, *(UINT64*)(DS + B)) // atomic operation
```

#### DS_MAX_U32

Opcode: 8 (0x8)  
Syntax: DS_MAX_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose greatest unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = MAX(*V, VDATA0) // atomic operation
```

#### DS_MAX_U64

Opcode: 72 (0x48)  
Syntax: DS_MAX_U64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose greatest unsigned 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = MAX(*V, VDATA0) // atomic operation
```

#### DS_MIN_F32

Opcode: 18 (0x12)  
Syntax: DS_MIN_F32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest single floating point value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
FLOAT* V = (FLOAT*)(DS + ((ADDR+OFFSET)&~3))
*V = MIN(*V, ASFLOAT(VDATA0)) // atomic operation
```

#### DS_MIN_F64

Opcode: 82 (0x52)  
Syntax: DS_MIN_F64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose smallest double floating point value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
DOUBLE* V = (DOUBLE*)(DS + ((ADDR+OFFSET)&~7))
*V = MIN(*V, ASDOUBLE(VDATA0)) // atomic operation
```

#### DS_MIN_I32

Opcode: 5 (0x5)  
Syntax: DS_MIN_I32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest signed integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
INT32* V = (INT32*)(DS + ((ADDR+OFFSET)&~3))
*V = MIN(*V, (INT32)VDATA0) // atomic operation
```

#### DS_MIN_I64

Opcode: 69 (0x45)  
Syntax: DS_MIN_I64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose smallest signed 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
INT64* V = (INT64*)(DS + ((ADDR+OFFSET)&~7))
*V = MIN(*V, (INT64)VDATA0) // atomic operation
```

#### DS_MIN_RTN_F32

Opcode: 50 (0x32)  
Syntax: DS_MIN_F32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest single floating point value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
FLOAT* V = (FLOAT*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = MIN(*V, ASFLOAT(VDATA0)) // atomic operation
```

#### DS_MIN_RTN_F64

Opcode: 114 (0x72)  
Syntax: DS_MIN_F64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose smallest double floating point value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
DOUBLE* V = (FLOAT*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = MIN(*V, ASDOUBLE(VDATA0)) // atomic operation
```

#### DS_MIN_RTN_I32

Opcode: 37 (0x25)  
Syntax: DS_MIN_RTN_I32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest signed integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
INT32* V = (INT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = MIN(*V, (INT32)VDATA0) // atomic operation
```

#### DS_MIN_RTN_I64

Opcode: 101 (0x65)  
Syntax: DS_MIN_RTN_I64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose smallest signed 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
INT64 V = (INT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = MIN(*V, (INT64)VDATA0) // atomic operation
```

#### DS_MIN_RTN_U32

Opcode: 39 (0x27)  
Syntax: DS_MIN_RTN_U32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = MIN(*V, VDATA0) // atomic operation
```

#### DS_MIN_RTN_U64

Opcode: 103 (0x67)  
Syntax: DS_MIN_RTN_U64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose smallest unsigned 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = MIN(*V, VDATA0) // atomic operation
```

#### DS_MIN_SRC2_F32

Opcode: 146 (0x92)  
Syntax: DS_MIN_SRC2_F32 ADDR [OFFSET:OFFSET]  
Description: Choose smallest single floating point value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
FLOAT* V = (FLOAT*)(DS + A)
*V = MIN(*V, *(FLOAT*)(DS + B)) // atomic operation
```

#### DS_MIN_SRC2_F64

Opcode: 210 (0xd2)  
Syntax: DS_MIN_SRC2_F64 ADDR [OFFSET:OFFSET]  
Description: Choose smallest double floating point value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
DOUBLE* V = (DOUBLE*)(DS + A)
*V = MIN(*V, *(DOUBLE*)(DS + B)) // atomic operation
```

#### DS_MIN_SRC2_I32

Opcode: 133 (0x85)  
Syntax: DS_MIN_SRC2_I32 ADDR [OFFSET:OFFSET]  
Description: Choose smallest signed integer value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
INT32* V = (INT32*)(DS + A)
*V = MIN(*V, *(INT32*)(DS + B)) // atomic operation
```

#### DS_MIN_SRC2_I64

Opcode: 197 (0xc5)  
Syntax: DS_MIN_SRC2_I64 ADDR [OFFSET:OFFSET]  
Description: Choose smallest signed 64-bit integer value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = A + (((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
INT64* V = (INT64*)(DS + A)
*V = MIN(*V, *(INT64*)(DS + B)) // atomic operation
```

#### DS_MIN_SRC2_U32

Opcode: 135 (0x87)  
Syntax: DS_MIN_SRC2_U32 ADDR [OFFSET:OFFSET]  
Description: Choose smallest unsigned integer value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = MIN(*V, *(UINT32*)(DS + B)) // atomic operation
```

#### DS_MIN_SRC2_U64

Opcode: 199 (0xc7)  
Syntax: DS_MIN_SRC2_U64 ADDR [OFFSET:OFFSET]  
Description: Choose smallest unsigned 64-bit integer value from LDS/GDS at address
A and at address B, and store result to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = MIN(*V, *(UINT64*)(DS + B)) // atomic operation
```

#### DS_MIN_U32

Opcode: 7 (0x7)  
Syntax: DS_MIN_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Choose smallest unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = MIN(*V, VDATA0) // atomic operation
```

#### DS_MIN_U64

Opcode: 71 (0x47)  
Syntax: DS_MIN_U64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Choose smallest unsigned 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 and VDATA0, and store result to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = MIN(*V, VDATA0) // atomic operation
```

#### DS_MSKOR_B32

Opcode: 12 (0xc)  
Syntax: DS_MSKOR_U32 ADDR, VDATA0, VDATA1 [OFFSET:OFFSET]  
Description: Load first argument from LDS/GDS at address (ADDR+OFFSET) & ~3.
Second and third arguments are from VDATA0 and VDATA1. The instruction keeps all bits
that is zeroed in second argument, and set all bits from third argument.
Result is stored in LDS/GDS at this same address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = (*V & ~VDATA0) | VDATA1 // atomic operation
```

#### DS_MSKOR_B64

Opcode: 76 (0x4c)  
Syntax: DS_MSKOR_U64 ADDR, VDATA0(2), VDATA1(2) [OFFSET:OFFSET]  
Description: Load first argument from LDS/GDS at address (ADDR+OFFSET) & ~7.
Second and third arguments are from VDATA0 and VDATA1. All three arguments are 64-bit.
The instruction keeps all bits that is zeroed in second argument, and set all bits from
third argument. Result is stored in LDS/GDS at this same address. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = (*V & ~VDATA0) | VDATA1 // atomic operation
```

#### DS_MSKOR_RTN_B32

Opcode: 44 (0x2c)  
Syntax: DS_MSKOR_RTN_U32 VDST, ADDR, VDATA0, VDATA1 [OFFSET:OFFSET]  
Description: Load first argument from LDS/GDS at address (ADDR+OFFSET) & ~3.
Second and third arguments are from VDATA0 and VDATA1. The instruction keeps all bits
that is zeroed in second argument, and set all bits from third argument.
Result is stored in LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = (*V & ~VDATA0) | VDATA1 // atomic operation
```

#### DS_MSKOR_RTN_B64

Opcode: 108 (0x6c)  
Syntax: DS_MSKOR_RTN_U64 VDST(2), ADDR, VDATA0(2), VDATA1(2) [OFFSET:OFFSET]  
Description: Load first argument from LDS/GDS at address (ADDR+OFFSET) & ~7.
Second and third arguments are from VDATA0 and VDATA1. All three arguments are 64-bit.
The instruction keeps all bits that is zeroed in second argument, and set all bits from
third argument. Result is stored in LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = (*V & ~VDATA0) | VDATA1 // atomic operation
```

#### DS_NOP

Opcode: 20 (0x14) for GCN 1.1/1.2/1.4  
Syntax: DS_NOP VADDR  
Description: Do nothing.

#### DS_OR_B32

Opcode: 10 (0xa)  
Syntax: DS_OR_B32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise OR operation on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = *V | VDATA0  // atomic operation
```

#### DS_OR_B64

Opcode: 74 (0x4a)  
Syntax: DS_OR_B64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Do bitwise OR operation on 64-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~7, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = *V | VDATA0  // atomic operation
```

#### DS_OR_RTN_B32

Opcode: 42 (0x2a)  
Syntax: DS_OR_RTN_B32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise OR operation on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = *V | VDATA0  // atomic operation
```

#### DS_OR_RTN_B64

Opcode: 106 (0x6a)  
Syntax: DS_OR_RTN_B64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Do bitwise OR operation on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~7, and value from VDATA0; and store result to LDS/GDS at this same
address. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = *V | VDATA0  // atomic operation
```

#### DS_OR_SRC2_B32

Opcode: 138 (0x8a)  
Syntax: DS_OR_SRC2_B32 ADDR [OFFSET:OFFSET]  
Description: Do bitwise OR operation on 32-bit value from LDS/GDS at address
A, and at address B; and store result to LDS/GDS at address A. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = *V | *(UINT32*)(DS + B) // atomic operation
```

#### DS_OR_SRC2_B64

Opcode: 202 (0xca)  
Syntax: DS_OR_SRC2_B64 ADDR [OFFSET:OFFSET]  
Description: Do bitwise OR operation on 64-bit value from LDS/GDS at address
A, and at address B; and store result to LDS/GDS at address A. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = *V | *(UINT64*)(DS + B) // atomic operation
```

#### DS_PERMUTE_B32

Opcode: 62 (0x3e) for GCN 1.2/1.4  
Syntax: DS_PERMUTE_B32 DST, ADDR, SRC [OFFSET:OFFSET]  
Description: Forward permutation for wave. Put value of SRC0 from LANEID to DST register in
lane id calculated from `ADDR[(LANEID + (OFFSET>>2)) & 63]`.
The ADDR holds lane id multiplied by 4 (size of dword). Realizes push semantic:
"put my lane data in lane i".
Operation:  
```
UINT32 TMP[64]
for (BYTE i = 0; i < 64; i++)
    tmp[ADDR[(i + (OFFSET>>2)) & 63]] = (EXEC & (1ULL<<i) != 0) ? SRC[i] : 0
for (BYTE i = 0; i < 64; i++)
    if (EXEC & (1ULL<<i) != 0)
        DST[i] = tmp[i]
```

#### DS_READ_ADDTID_B32

Opcode: 182 (0xb6) for GCN 1.4  
Syntax: DS_READ_ADDTID_B32 VDST [OFFSET:OFFSET]  
Operation: Read single dword from LDS/GDS at address (M0&0xffff) + OFFSET + LANEID*4 and
store into VDST.  
Operation:  
```
VDST = *(UINT32*)(DS + (M0&0xffff) + OFFSET + LANEID*4)
```

#### DS_READ_B128

Opcode: 255 (0xff) for GCN 1.1/1.2/1.4  
Syntax: DS_READ_B128 VDST(4), ADDR [OFFSET:OFFSET]  
Description: Read four dwords from LDS/GDS at address (ADDR+OFFSET) & ~15,
store into VDST.  
Operation:  
```
VDST[0:1] = *(UINT64*)(DS + ((ADDR+OFFSET)&~15))
VDST[2:3] = *(UINT64*)(DS + ((ADDR+OFFSET)&~15) + 8)
```

#### DS_READ_B32

Opcode: 54 (0x36)  
Syntax: DS_READ_B32 VDST, ADDR [OFFSET:OFFSET]  
Description: Read dword from LDS/GDS at address (ADDR+OFFSET) & ~3 or
(ADDR+OFFSET) for GCN 1.2/1.4, store into VDST.  
Operation:  
```
if (GCN14)
    VDST = *(UINT32*)(DS + (ADDR+OFFSET))
else
    VDST = *(UINT32*)(DS + ((ADDR+OFFSET)&~3))
```

#### DS_READ_B64

Opcode: 118 (0x76)  
Syntax: DS_READ_B64 VDST(2), ADDR [OFFSET:OFFSET]  
Description: Read two dwords from LDS/GDS at address (ADDR+OFFSET) & ~7 or
(ADDR+OFFSET) for GCN 1.4, store into VDST.  
Operation:  
```
if (GCN14)
    VDST = *(UINT64*)(DS + (ADDR+OFFSET))
else
    VDST = *(UINT64*)(DS + ((ADDR+OFFSET)&~7))
```

#### DS_READ_B96

Opcode: 254 (0xfe) for GCN 1.1/1.2/1.4  
Syntax: DS_READ_B96 VDST(3), ADDR [OFFSET:OFFSET]  
Description: Read three dwords from LDS/GDS at address (ADDR+OFFSET) & ~15,
store into VDST.  
Operation:  
```
VDST[0:1] = *(UINT64*)(DS + ((ADDR+OFFSET)&~15))
VDST[2] = *(UINT32*)(DS + ((ADDR+OFFSET)&~15) + 8)
```

#### DS_READ_I16

Opcode: 59 (0x3b)  
Syntax: DS_READ_I16 VDST, ADDR [OFFSET:OFFSET]  
Description: Read signed 16-bit word from LDS/GDS at address (ADDR+OFFSET) & ~1 or
(ADDR+OFFSET) for GCN 1.4, store into VDST. The value's sign will be extended to higher bits.  
Operation:  
```
if (GCN14)
    VDST = (INT32)*(INT16*)(DS + (ADDR+OFFSET))
else
    VDST = (INT32)*(INT16*)(DS + ((ADDR+OFFSET)&~1))
```

#### DS_READ_I8_D16

Opcode: 88 (0x58) for GCN 1.4  
Syntax: DS_READ_I8_D16 VDST, ADDR [OFFSET:OFFSET]  
Description: Read signed byte from LDS/GDS at address (ADDR+OFFSET) and store as
16-bit unsigned value to lower 16-bits of VDST.  
Operation:  
```
VDST = SEXT16(*(INT8*)(DS + (ADDR+OFFSET))) | (VDST&0xffff0000)
```

#### DS_READ_I8_D16_HI

Opcode: 88 (0x59) for GCN 1.4  
Syntax: DS_READ_I8_D16_HI VDST, ADDR [OFFSET:OFFSET]  
Description: Read signed byte from LDS/GDS at address (ADDR+OFFSET) and store as
16-bit unsigned value to higher16-bits of VDST.  
Operation:  
```
VDST = (((UINT32)SEXT16(*(INT8*)(DS + (ADDR+OFFSET))))<<16) | (VDST&0xffff)
```

#### DS_READ_I8

Opcode: 57 (0x39)  
Syntax: DS_READ_I8 VDST, ADDR [OFFSET:OFFSET]  
Description: Read signed byte from LDS/GDS at address (ADDR+OFFSET), store into VDST.
The value's sign will be extended to higher bits.  
Operation:  
```
VDST = (INT32)*(INT8*)(DS + (ADDR+OFFSET))
```

#### DS_READ_U16

Opcode: 60 (0x3c)  
Syntax: DS_READ_U16 VDST, ADDR [OFFSET:OFFSET]  
Description: Read unsigned 16-bit word from LDS/GDS at address (ADDR+OFFSET) & ~1 or
(ADDR+OFFSET) for GCN 1.4, store into VDST.  
Operation:  
```
if (GCN14)
    VDST = *(UINT16*)(DS + (ADDR+OFFSET))
else
    VDST = *(UINT16*)(DS + ((ADDR+OFFSET)&~1))
```

#### DS_READ_U16_D16

Opcode: 90 (0x5a) for GCN 1.4  
Syntax: DS_READ_U16_D16 VDST, ADDR [OFFSET:OFFSET]  
Description Read unsigned 16-bit word from LDS/DDS at address ADDR+OFFSET and store
into lower part of VDST.  
Operation:  
```
VDST = *(UINT16*)(DS + (ADDR+OFFSET)) | (VDST & 0xffff0000)
```

#### DS_READ_U16_D16_HI

Opcode: 91 (0x5b) for GCN 1.4  
Syntax: DS_READ_U16_D16_HI VDST, ADDR [OFFSET:OFFSET]  
Description Read unsigned 16-bit word from LDS/DDS at address ADDR+OFFSET and store
into higher part of VDST.  
Operation:  
```
VDST = (((UINT32)*(UINT16*)(DS + (ADDR+OFFSET)))<<16) | (VDST & 0xffff)
```

#### DS_READ_U8

Opcode: 58 (0x3a)  
Syntax: DS_READ_U8 VDST, ADDR [OFFSET:OFFSET]  
Description: Read unsigned byte from LDS/GDS at address (ADDR+OFFSET), store into VDST.  
Operation:  
```
VDST = *(UINT8*)(DS + (ADDR+OFFSET))
```

#### DS_READ_U8_D16

Opcode: 86 (0x56) for GCN 1.4  
Syntax: DS_READ_U8_D16 VDST, ADDR [OFFSET:OFFSET]  
Description: Read unsigned byte from LDS/GDS at address (ADDR+OFFSET) and store as
16-bit unsigned value to lower 16-bits of VDST.  
Operation:  
```
VDST = (UINT16)(*(UINT8*)(DS + (ADDR+OFFSET))) | (VDST&0xffff0000)
```

#### DS_READ_U8_D16_HI

Opcode: 87 (0x57) for GCN 1.4  
Syntax: DS_READ_U8_D16_HI VDST, ADDR [OFFSET:OFFSET]  
Description: Read unsigned byte from LDS/GDS at address (ADDR+OFFSET) and store as
16-bit unsigned value to higher16-bits of VDST.  
Operation:  
```
VDST = (((UINT32)(*(UINT8*)(DS + (ADDR+OFFSET))))<<16) | (VDST&0xffff)
```

#### DS_READ2_B32

Opcode: 55 (0x37)  
Syntax: DS_READ2_B32 VDST(2), ADDR [OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Read dword from LDS/GDS at address (ADDR+OFFSET0\*4) & ~3, and second dword
at address (ADDR+OFFSET1\*4) & ~3, and store these dwords into VDST.  
Operation:  
```
UINT32* V0 = (UINT32*)(DS + ((ADDR + OFFSET0*4)&~3)) 
UINT32* V1 = (UINT32*)(DS + ((ADDR + OFFSET1*4)&~3))
VDST = *V0 | (UINT64(*V1)<<32)
```

#### DS_READ2_B64

Opcode: 119 (0x77)  
Syntax: DS_READ2_B64 VDST(4), ADDR [OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Read 64-bit value from LDS/GDS at address (ADDR+OFFSET0\*8) & ~7,
and second value at address (ADDR+OFFSET1\*8) & ~7, and store these values into VDST.  
Operation:  
```
UINT64* V0 = (UINT64*)(DS + (ADDR + OFFSET0*8)&~7) 
UINT64* V1 = (UINT64*)(DS + (ADDR + OFFSET1*8)&~7)
VDST = *V0 | (UINT128(*V1)<<64)
```

#### DS_READ2ST64_B32

Opcode: 56 (0x38)  
Syntax: DS_READ2ST64_B32 VDST(2), ADDR [OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Read dword from LDS/GDS at address (ADDR+OFFSET0\*256) & ~3, and second dword
at address (ADDR+OFFSET1\*256) & ~3, and store these values into VDST.  
Operation:  
```
UINT32* V0 = (UINT32*)(DS + ((ADDR + OFFSET0*256)&~3)) 
UINT32* V1 = (UINT32*)(DS + ((ADDR + OFFSET1*256)&~3))
VDST = *V0 | (UINT64(*V1)<<32)
```

#### DS_READ2ST64_B64

Opcode: 120 (0x78)  
Syntax: DS_READ2ST64_B64 VDST(4), ADDR [OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Read 64-bit value from LDS/GDS at address (ADDR+OFFSET0\*512) & ~7, and
second value at address (ADDR+OFFSET1\*512) & ~7, and store these values into VDST.  
Operation:  
```
UINT64* V0 = (UINT64*)(DS + ((ADDR + OFFSET0*512)&~7))
UINT64* V1 = (UINT64*)(DS + ((ADDR + OFFSET1*512)&~7))
VDST = *V0 | (UINT128(*V1)<<64)
```

#### DS_RSUB_RTN_U32

Opcode: 34 (0x22)  
Syntax: DS_RSUB_RTN_U32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Subtract unsigned integer value from LDS/GDS at address (ADDR+OFFSET) & ~3
from value in VDATA0, and store result back to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST= *V; *V = VDATA0 - *V  // atomic operation
```

#### DS_RSUB_RTN_U64

Opcode: 98 (0x62)  
Syntax: DS_RSUB_RTN_U64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Subtract unsigned 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 from value in VDATA0, and store result back to LDS/GDS at this
same address. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST= *V; *V = VDATA0 - *V  // atomic operation
```

#### DS_RSUB_SRC2_U32

Opcode: 130 (0x82)  
Syntax: DS_RSUB_SRC2_U32 ADDR [OFFSET:OFFSET]  
Description: Subtract unsigned integer value from LDS/GDS at address A from
value at address B, and store result back to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = *(UINT32*)(DS + B) - *V // atomic operation
```

#### DS_RSUB_SRC2_U64

Opcode: 194 (0xc2)  
Syntax: DS_RSUB_SRC2_U64 ADDR [OFFSET:OFFSET]  
Description: Subtract unsigned integer value from LDS/GDS at address A from
value at address B, and store result back to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = *(UINT64*)(DS + B) - *V // atomic operation
```

#### DS_RSUB_U32

Opcode: 2 (0x2)  
Syntax: DS_RSUB_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Subtract unsigned integer value from LDS/GDS at address (ADDR+OFFSET) & ~3
from value in VDATA0, and store result back to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = VDATA0 - *V  // atomic operation
```

#### DS_RSUB_U64

Opcode: 66 (0x42)  
Syntax: DS_RSUB_U64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Subtract unsigned 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7 from value in VDATA0, and store result back to LDS/GDS at this
same address. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = VDATA0 - *V  // atomic operation
```

#### DS_SUB_RTN_U32

Opcode: 33 (0x21)  
Syntax: DS_SUB_RTN_U32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Subtract VDATA0 from unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and store result back to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = *V - VDATA0  // atomic operation
```

#### DS_SUB_RTN_U64

Opcode: 97 (0x61)  
Syntax: DS_SUB_RTN_U64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Subtract VDATA0 from unsigned 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7, and store result back to LDS/GDS at this same address.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = *V - VDATA0  // atomic operation
```

#### DS_SUB_SRC2_U32

Opcode: 129 (0x81)  
Syntax: DS_SUB_SRC2_U32 ADDR [OFFSET:OFFSET]  
Description: Subtract unsigned integer value from LDS/GDS at address B from
value at address A, and store result back to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = *V - *(UINT32*)(DS + B) // atomic operation
```

#### DS_SUB_SRC2_U64

Opcode: 193 (0xc1)  
Syntax: DS_SUB_SRC2_U64 ADDR [OFFSET:OFFSET]  
Description: Subtract unsigned 64-bit integer value from LDS/GDS at address B from
value at address A, and store result back to LDS/GDS at address A.
Refer to listing to learn about addressing. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = *V - *(UINT64*)(DS + B) // atomic operation
```

#### DS_SUB_U32

Opcode: 1 (0x1)  
Syntax: DS_SUB_U32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Subtract VDATA0 from unsigned integer value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and store result back to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = *V - VDATA0  // atomic operation
```

#### DS_SUB_U64

Opcode: 65 (0x41)  
Syntax: DS_SUB_U64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Subtract VDATA0 from unsigned 64-bit integer value from LDS/GDS at address
(ADDR+OFFSET) & ~7, and store result back to LDS/GDS at this same address.
Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = *V - VDATA0  // atomic operation
```

#### DS_SWIZZLE_B32

Opcode: 53 (0x35) for GCN 1.0/1.1; 61 (0x3d) for GCN 1.2/1.4  
Syntax: DS_SWIZZLE_B32 VDST, ADDR [OFFSET:OFFSET]  
Description: Move between lanes ADDR value storing in VDST. Refer to operation's listing to
learn about how this instruction calcutes input lane. If input lane is not valid then
store zero to VDST[LANEID]. This instruction doesn't finish immediately and it increments
LGKMCNT counter (you must use `S_WAITCNT LGKMCNT(X)` to wait for its results).  
Operation:  
```
BYTE INLANEID = 0
if (OFFSET&0x8000)
    INLANEID = (LANEID&0x3c) + ((OFFSET >> (2*(LANEID&3)) & 3)
else
{
    BYTE ANDMASK = (OFFSET&31)
    BYTE ORMASK = (OFFSET>>5)&31
    BYTE XORMASK = OFFSET>>10
    INLANEID = (LANEID&32) + ((((LANEID&31) & ANDMASK) | ORMASK) ^ XORMASK)
}
VDST[LANEID] = (EXEC & (1ULL<<INLANEID)) ? ADDR[INLANEID] : 0
```

#### DS_WRAP_RTN_B32

Opcode: 52 (0x34) for GCN 1.1/1.2/1.4  
Syntax: DS_WRAP_RTN_B32 VDST, ADDR, VDATA0, VDATA1 [OFFSET:OFFSET]  
Description: Compare unsigned 32-bit value from LDS/GDS at address A and D0. If value from
LDS/GDS is not less than VDATA0 value, then subtract VDATA0 from this value and store
result into LDS/GDS. Otherwise add VDATA1 to LDS/GDS value and store into LDS/GDS.
Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32 A = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = (*V >= VDATA0) ? (*V-VDATA0) : (*V+VDATA1) // atomic operation
```

#### DS_WRITE_ADDTID_B32

Opcode 29 (0x1d) for GCN 1.4  
Syntax: DS_WRITE_ADDTID_B32 VDATA0 [OFFSET:OFFSET]  
Description: Store single dword value from VDATA0 into LDS/GDS at address
((M0&0xffff) + OFFSET + 4*LANEID).  
Operation:  
```
(UINT32*)(DS + (M0&0xffff) + OFFSET + LANEID*4) = VDATA0
```

#### DS_WRITE_B128

Opcode: 223 (0xdf)for GCN 1.1/1.2/1.4  
Syntax: DS_WRITE_B128 ADDR, VDATA0(4) [OFFSET:OFFSET]  
Description: Store four dwords value from VDATA0 into LDS/GDS at address
(ADDR+OFFSET) & ~15.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~15))
*V = VDATA0[0:1]
*(V+1) = VDATA0[2:3]
```

#### DS_WRITE_B16

Opcode: 31 (0x1f)  
Syntax: DS_WRITE_B16 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Store low 16 bits from VDATA0 into LDS/GDS at address (ADDR+OFFSET) & 1
or (ADDR+OFFSET) for GCN 1.4.  
Operation:  
```
UINT16* V
if (GCN14)
    V = (UINT16*)(DS + ADDR+OFFSET)
else
    V = (UINT16*)(DS + (ADDR+OFFSET)&~1)
*V = VDATA0&0xffff
```

#### DS_WRITE_B16_D16_HI

Opcode: 85 (0x55) for GCN 1.4  
Syntax: DS_WRITE_B16_D16_HI ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Store high 16 bits from VDATA0 into LDS/GDS at address (ADDR+OFFSET).  
```
V = (UINT16*)(DS + ADDR+OFFSET)
*V = VDATA0>>16
```

#### DS_WRITE_B32

Opcode: 13 (0xd)  
Syntax: DS_WRITE_B32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Store value from VDATA0 into LDS/GDS at address (ADDR+OFFSET) & ~3 or
(ADDR+OFFSET) if GCN 1.4.  
Operation:  
```
UINT32* V
if (GCN1.4)
    V = (UINT32*)(DS + (ADDR+OFFSET))
else
    V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = VDATA0
```

#### DS_WRITE_B64

Opcode: 77 (0x4d)  
Syntax: DS_WRITE_B64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Store 64-bit value from VDATA0 into LDS/GDS at address (ADDR+OFFSET) & ~7 or
ADDR+OFFSET for GCN 1.4.  
Operation:  
```
UINT64* V
if (GCN1.4)
    UINT64* V = (UINT64*)(DS + (ADDR+OFFSET)) // ???
else
    UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = VDATA0
```

#### DS_WRITE_B8

Opcode: 30 (0x1e)  
Syntax: DS_WRITE_B8 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Store low 8 bits from VDATA0 into LDS/GDS at address (ADDR+OFFSET).  
Operation:  
```
UINT8* V = (UINT8*)(DS + (ADDR+OFFSET))
*V = VDATA0&0xff
```

#### DS_WRITE_B8_D16_HI

Opcode: 84 (0x54) for GCN 1.4  
Syntax: DS_WRITE_B8_D16_HI ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Store high 16-23 bits from VDATA0 into LDS/GDS at address (ADDR+OFFSET).  
```
V = (UINT8*)(DS + ADDR+OFFSET)
*V = (VDATA0>>16)&0xff
```

#### DS_WRITE_B96

Opcode: 222 (0xde) for GCN 1.1/1.2/1.4  
Syntax: DS_WRITE_B96 ADDR, VDATA0(3) [OFFSET:OFFSET]  
Description: Store three dwords value from VDATA0 into LDS/GDS at address
(ADDR+OFFSET) & ~15.  
Operation:  
```
UINT32* V = (UINT64*)(DS + ((ADDR+OFFSET)&~15))
*(UINT64*)V = VDATA0[0:1]
*(V+2) = VDATA0[2]
```

#### DS_WRITE_SRC2_B32

Opcode: 141 (0x8d)  
Syntax: DS_WRITE_SRC2_B32 ADDR [OFFSET:OFFSET]  
Description: Store value from LDS/GDS at address B into LDS/GDS at address A.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
*(UINT32*)(DS + A) = *(UINT32*)(DS + B)
```

#### DS_WRITE_SRC2_B64

Opcode: 205 (0xcd)  
Syntax: DS_WRITE_SRC2_B64 ADDR [OFFSET:OFFSET]  
Description: Store 64-bit value from LDS/GDS at address B into LDS/GDS at address A.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
*(UINT64*)(DS + A) = *(UINT64*)(DS + B)
```

#### DS_WRITE2_B32

Opcode: 14 (0xe)  
Syntax: DS_WRITE2_B32 ADDR, VDATA0, VDATA1 [OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Store one dword from VDATA0 into LDS/GDS at address (ADDR+OFFSET0\*4) & ~3,
and second into LDS/GDS at address (ADDR+OFFSET1\*4) & ~3.  
Operation:  
```
UINT32* V0 = (UINT32*)(DS + ((ADDR + OFFSET0*4)&~3)) 
UINT32* V1 = (UINT32*)(DS + ((ADDR + OFFSET1*4)&~3))
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_WRITE2_B64

Opcode: 78 (0x4e)  
Syntax: DS_WRITE2_B64 ADDR, VDATA0(2), VDATA1(2) [OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Store one 64-bit value from VDATA0 into LDS/GDS at address
(ADDR+OFFSET0\*8) & ~7, and second into LDS/GDS at address (ADDR+OFFSET1\*8) & ~7.  
Operation:  
```
UINT64* V0 = (UINT64*)(DS + (ADDR + OFFSET0*8)&~7) 
UINT64* V1 = (UINT64*)(DS + (ADDR + OFFSET1*8)&~7)
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_WRITE2ST64_B32

Opcode: 15 (0xf)  
Syntax: DS_WRITE2ST64_B32 ADDR, VDATA0, VDATA1 [OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Store one dword from VDATA0 into LDS/GDS at address (ADDR+OFFSET0\*256) & ~3,
and second into LDS/GDS at address (ADDR+OFFSET1\*256) & ~3.  
Operation:  
```
UINT32* V0 = (UINT32*)(DS + ((ADDR + OFFSET0*256)&~3)) 
UINT32* V1 = (UINT32*)(DS + ((ADDR + OFFSET1*256)&~3))
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_WRITE2ST64_B64

Opcode: 79 (0x4f)  
Syntax: DS_WRITE2ST64_B64 ADDR, VDATA0(2), VDATA1(2) [OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Store one 64-bit dword from VDATA0 into LDS/GDS at address
(ADDR+OFFSET0\*512) & ~7, and second into LDS/GDS at address (ADDR+OFFSET1\*512) & ~7.  
Operation:  
```
UINT64* V0 = (UINT64*)(DS + ((ADDR + OFFSET0*512)&~7)) 
UINT64* V1 = (UINT64*)(DS + ((ADDR + OFFSET1*512)&~7))
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_WRXCHG_RTN_B32

Opcode: 45 (0x2d)  
Syntax: DS_WRXCHG_B32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Store value from VDATA0 into LDS/GDS at address (ADDR+OFFSET) & ~3.
Previous value from LDS/GDS are stored in VDST.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = VDATA0 // atomic operation
```

#### DS_WRXCHG_RTN_B64

Opcode: 109 (0x6d)  
Syntax: DS_WRXCHG_B64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Store 64-bit value from VDATA0 into LDS/GDS at address (ADDR+OFFSET) & ~7.
Previous value from LDS/GDS are stored in VDST.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = VDATA0 // atomic operation
```

#### DS_WRXCHG2_RTN_B32

Opcode: 46 (0x2e)  
Syntax: DS_WRXCHG2_RTN_B32 VDST(2), ADDR, VDATA0, VDATA1
[OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Store one dword from VDATA0 into LDS/GDS at address (ADDR+OFFSET0\*4) & ~3,
and second into LDS/GDS at address (ADDR+OFFSET1\*4) & ~3.
Previous values from LDS/GDS are stored in VDST.  
Operation:  
```
UINT32* V0 = (UINT32*)(DS + ((ADDR + OFFSET0*4)&~3))
UINT32* V1 = (UINT32*)(DS + ((ADDR + OFFSET1*4)&~3))
VDST = (*V0) | (UINT64(*V1)<<32)
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_WRXCHG2_RTN_B64

Opcode: 110 (0x6e)  
Syntax: DS_WRXCHG2_RTN_B64 VDST(4), ADDR, VDATA0(2), VDATA1(2)
[OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Store one 64-bit value from VDATA0 into LDS/GDS at address
(ADDR+OFFSET0\*8) & ~7, and second into LDS/GDS at address (ADDR+OFFSET1\*8) & ~7.
Previous values from LDS/GDS are stored in VDST.  
Operation:  
```
UINT64* V0 = (UINT64*)(DS + ((ADDR + OFFSET0*8)&~7))
UINT64* V1 = (UINT64*)(DS + ((ADDR + OFFSET1*8)&~7))
VDST = (*V0) | (UINT128(*V1)<<64)
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_WRXCHG2ST64_RTN_B32

Opcode: 47 (0x2f)  
Syntax: DS_WRXCHG2ST64_RTN_B32 VDST(2), ADDR, VDATA0, VDATA1
[OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Store one dword from VDATA0 into LDS/GDS at address (ADDR+OFFSET0\*256) & ~3,
and second into LDS/GDS at address (ADDR+OFFSET1\*256) & ~3.
Previous values from LDS/GDS are stored in VDST.  
Operation:  
```
UINT32* V0 = (UINT32*)(DS + ((ADDR + OFFSET0*256)&~3)) 
UINT32* V1 = (UINT32*)(DS + ((ADDR + OFFSET1*256)&~3))
VDST = (*V0) | (UINT64(*V1)<<32)
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_WRXCHG2ST64_RTN_B64

Opcode: 111 (0x6f)  
Syntax: DS_WRXCHG2ST64_RTN_B64 VDST(4), ADDR, VDATA0(2), VDATA1(2)
[OFFSET0:OFFSET0] [OFFSET1:OFFSET1]  
Description: Store one 64-bit value from VDATA0 into LDS/GDS at address
(ADDR+OFFSET0\*512) & ~7, and second into LDS/GDS at address (ADDR+OFFSET1\*512) & ~7.
Previous values from LDS/GDS are stored in VDST.  
Operation:  
```
UINT64* V0 = (UINT64*)(DS + ((ADDR + OFFSET0*512)&~7)) 
UINT64* V1 = (UINT64*)(DS + ((ADDR + OFFSET1*512)&~7))
VDST = (*V0) | (UINT128(*V1)<<64)
*V0 = VDATA0
*V1 = VDATA1
```

#### DS_XOR_B32

Opcode: 11 (0xb)  
Syntax: DS_XOR_B32 ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise XOR operation on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
*V = *V ^ VDATA0  // atomic operation
```

#### DS_XOR_B64

Opcode: 75 (0x4b)  
Syntax: DS_XOR_B64 ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Do bitwise XOR operation on 64-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~7, and value from VDATA0; and store result to LDS/GDS at this same
address. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
*V = *V ^ VDATA0  // atomic operation
```

#### DS_XOR_RTN_B32

Opcode: 43 (0x2b)  
Syntax: DS_XOR_RTN_B32 VDST, ADDR, VDATA0 [OFFSET:OFFSET]  
Description: Do bitwise XOR operation on 32-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~3, and value from VDATA0; and store result to LDS/GDS at this same
address. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT32* V = (UINT32*)(DS + ((ADDR+OFFSET)&~3))
VDST = *V; *V = *V ^ VDATA0  // atomic operation
```

#### DS_XOR_RTN_B64

Opcode: 107 (0x6b)  
Syntax: DS_XOR_RTN_B64 VDST(2), ADDR, VDATA0(2) [OFFSET:OFFSET]  
Description: Do bitwise XOR operation on 64-bit value from LDS/GDS at address
(ADDR+OFFSET) & ~7, and value from VDATA0; and store result to LDS/GDS at this same
address. Previous value from LDS/GDS are stored in VDST. Operation is atomic.  
Operation:  
```
UINT64* V = (UINT64*)(DS + ((ADDR+OFFSET)&~7))
VDST = *V; *V = *V ^ VDATA0  // atomic operation
```

#### DS_XOR_SRC2_B32

Opcode: 139 (0x8b)  
Syntax: DS_XOR_SRC2_B32 ADDR [OFFSET:OFFSET]  
Description: Do bitwise XOR operation on 32-bit value from LDS/GDS at address
A, and at address B; and store result to LDS/GDS at address A. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fffc : ADDR&~3
UINT32 B = A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4
UINT32* V = (UINT32*)(DS + A)
*V = *V ^ *(UINT32*)(DS + B) // atomic operation
```

#### DS_XOR_SRC2_B64

Opcode: 203 (0xcb)  
Syntax: DS_XOR_SRC2_B64 ADDR [OFFSET:OFFSET]  
Description: Do bitwise XOR operation on 64-bit value from LDS/GDS at address
A, and at address B; and store result to LDS/GDS at address A. Operation is atomic.  
Operation:  
```
UINT32 A = (OFFSET&0x8000) ? ADDR&0x1fff8 : ADDR&~7
UINT32 B = (A + ((OFFSET&0x8000) ? \
            ((ADDR>>17) | ((ADDR>>16)&0x8000)) : \
            ((OFFSET&0x7fff) | (OFFSET<<1)&0x8000)) * 4)&~7
UINT64* V = (UINT64*)(DS + A)
*V = *V ^ *(UINT64*)(DS + B) // atomic operation
```
