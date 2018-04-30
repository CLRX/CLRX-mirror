/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include "../TestUtils.h"
#include "AsmRegAlloc.h"

using namespace CLRX;

typedef AsmRegAllocator::OutLiveness OutLiveness;
typedef AsmRegAllocator::LinearDep LinearDep;
typedef AsmRegAllocator::VarIndexMap VarIndexMap;

struct LinearDep2
{
    cxbyte align;
    Array<size_t> prevVidxes;
    Array<size_t> nextVidxes;
};

struct AsmLivenessesCase
{
    const char* input;
    Array<OutLiveness> livenesses[MAX_REGTYPES_NUM];
    Array<std::pair<size_t, LinearDep2> > linearDepMaps[MAX_REGTYPES_NUM];
    bool good;
    const char* errorMessages;
};

static const AsmLivenessesCase createLivenessesCasesTbl[] =
{
    {   // 0 - simple case
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[4], s3
        v_xor_b32 va[4], va[2], v3
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // S3
                { { 0, 1 } }, // sa[2]'0
                { { 1, 5 } }, // sa[4]'0
                { { 5, 6 } }  // sa[4]'1
            },
            {   // for VGPRs
                { { 0, 9 } }, // V3
                { { 0, 9 } }, // va[2]'0
                { { 9, 10 } } // va[4]'0 : out of range code block
            },
            { },
            { }
        },
        { },  // linearDepMaps
        true, ""
    },
    {   // 1 - simple case 2
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[4], s3
        v_xor_b32 va[4], va[2], v3
        s_endpgm
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // S3
                { { 0, 1 } }, // sa[2]'0
                { { 1, 5 } }, // sa[4]'0
                { { 5, 6 } }  // sa[4]'1
            },
            {   // for VGPRs
                { { 0, 9 } }, // V3
                { { 0, 9 } }, // va[2]'0
                { { 9, 10 } } // va[4]'0 : out of range code block
            },
            { },
            { }
        },
        { },  // linearDepMaps
        true, ""
    },
    {   // 2 - simple case (linear dep)
        R"ffDXD(.regvar sa:s:8, va:v:10
        .regvar sbuf:s:4, rbx4:v:6
        s_mov_b64 sa[4:5], sa[2:3]  # 0
        s_and_b64 sa[4:5], sa[4:5], s[4:5]
        v_add_f64 va[4:5], va[2:3], v[3:4]
        buffer_load_dwordx4 rbx4[1:5], va[6], sbuf[0:3], sa[7] idxen offset:603 tfe
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // S4
                { { 0, 5 } }, // S5
                { { 0, 1 } }, // sa[2]'0
                { { 0, 1 } }, // sa[3]'0
                { { 1, 5 } }, // sa[4]'0
                { { 5, 6 } }, // sa[4]'1
                { { 1, 5 } }, // sa[5]'0
                { { 5, 6 } }, // sa[5]'1
                { { 0, 17 } }, // sa[7]'0
                { { 0, 17 } }, // sbuf[0]'0
                { { 0, 17 } }, // sbuf[1]'0
                { { 0, 17 } }, // sbuf[2]'0
                { { 0, 17 } }, // sbuf[3]'0
            },
            {   // for VGPRs
                { { 0, 9 } }, // V3
                { { 0, 9 } }, // V4
                { { 17, 18 } }, // rbx4[1]'0
                { { 17, 18 } }, // rbx4[2]'0
                { { 17, 18 } }, // rbx4[3]'0
                { { 17, 18 } }, // rbx4[4]'0
                { { 0, 17 } }, // rbx4[5]'0: tfe - read before write
                { { 0, 9 } }, // va[2]'0
                { { 0, 9 } }, // va[3]'0
                { { 9, 10 } }, // va[4]'0 : out of range code block
                { { 9, 10 } }, // va[5]'0 : out of range code block
                { { 0, 17 } } // va[6]'0
            },
            { },
            { }
        },
        {   // linearDepMaps
            {   // for SGPRs
                { 0, { 0, { }, { 1 } } },  // S4
                { 1, { 0, { 0 }, { } } },  // S5
                { 2, { 2, { }, { 3 } } },  // sa[2]'0
                { 3, { 0, { 2 }, { } } },  // sa[3]'0
                { 4, { 2, { }, { 6 } } },  // sa[4]'0
                { 5, { 2, { }, { 7, 7 } } },  // sa[4]'1
                { 6, { 0, { 4 }, { } } },  // sa[5]'0
                { 7, { 0, { 5, 5 }, { } } },   // sa[5]'1
                { 9, { 4, { }, { 10 } } }, // sbuf[0]'0
                { 10, { 0, { 9 }, { 11 } } }, // sbuf[1]'0
                { 11, { 0, { 10 }, { 12 } } }, // sbuf[2]'0
                { 12, { 0, { 11 }, { } } }  // sbuf[3]'0
            },
            {   // for VGPRs
                { 0, { 0, { }, { 1 } } },  // V3
                { 1, { 0, { 0 }, { } } },  // V4
                { 2, { 1, { }, { 3 } } }, // rbx4[1]'0
                { 3, { 0, { 2 }, { 4 } } }, // rbx4[2]'0
                { 4, { 0, { 3 }, { 5 } } }, // rbx4[3]'0
                { 5, { 0, { 4 }, { 6 } } }, // rbx4[4]'0
                { 6, { 0, { 5 }, { } } }, // rbx4[5]'0
                { 7, { 1, { }, { 8 } } },  // va[2]'0
                { 8, { 0, { 7 }, { } } },  // va[3]'0
                { 9, { 1, { }, { 10 } } },  // va[4]'0
                { 10, { 0, { 9 }, { } } }  // va[5]'0
            },
            { },
            { }
        },
        true, ""
    },
    {   // 3 - next simple case
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[4], s3
        v_xor_b32 va[4], va[2], v3
        v_mov_b32 va[3], 1.45s
        v_mov_b32 va[6], -7.45s
        v_lshlrev_b32 va[5], sa[4], va[4]
        v_mac_f32 va[3], va[6], v0
        s_mul_i32 sa[7], sa[2], sa[3]
        v_mac_f32 va[3], va[7], v0
        s_endpgm
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // 0: S3
                { { 0, 37 } }, // 1: sa[2]'0
                { { 0, 37 } }, // 2: sa[3]'0
                { { 1, 5 } }, // 3: sa[4]'0
                { { 5, 29 } }, // 4: sa[4]'1
                { { 37, 38 } }  // 5: sa[7]'0
            },
            {   // for VGPRs
                { { 0, 41 } }, // 0: v0
                { { 0, 9 } }, // 1: v3
                { { 0, 9 } }, // 2: va[2]'0
                { { 13, 41 } }, // 3: va[3]'0
                { { 9, 29 } }, // 4: va[4]'0
                { { 29, 30 } }, // 5: va[5]'0
                { { 21, 33 } }, // 6: va[6]'0
                { { 0, 41 } }  // 7: va[7]'0
            },
            { },
            { }
        },
        { }, // linearDepMaps
        true, ""
    },
    {   // 4 - next test case
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[4], s3  # 4
        
        v_xor_b32 v4, va[2], v3         # 8
        v_xor_b32 va[5], v4, va[5]      # 12
        
        s_sub_u32 sa[4], sa[4], sa[3]   # 16
        
        v_xor_b32 v4, va[5], v3         # 20
        v_xor_b32 va[6], v4, va[6]      # 24
        
        s_mul_i32 sa[4], sa[4], sa[1]   # 28
        s_mul_i32 sa[4], sa[4], sa[0]   # 32
        
        v_xor_b32 v4, va[6], v3         # 36
        v_xor_b32 va[7], v4, va[7]      # 40
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } },   // 0: S3
                { { 0, 33 } },  // 1: sa[0]'0
                { { 0, 29 } },  // 2: sa[1]'0
                { { 0, 1 } },   // 3: sa[2]'0
                { { 0, 17 } },  // 4: sa[3]'0
                { { 1, 5 } },   // 5: sa[4]'0
                { { 5, 17 } },  // 6: sa[4]'1
                { { 17, 29 } }, // 7: sa[4]'2
                { { 29, 33 } }, // 8: sa[4]'3
                { { 33, 34 } }  // 9: sa[4]'4
            },
            {   // for VGPRs
                { { 0, 37 } },   // 0: v3
                { { 9, 13 }, { 21, 25 }, { 37, 41 } }, // 1: v4
                { { 0, 9 } },    // 2: va[2]'0
                { { 0, 13 } },   // 3: va[5]'0
                { { 13, 21 } },  // 4: va[5]'1
                { { 0, 25 } },   // 5: va[6]'0
                { { 25, 37 } },  // 6: va[6]'1
                { { 0, 41 } },   // 7: va[7]'0
                { { 41, 42 } }   // 8: va[7]'1
            },
            { },
            { }
        },
        { }, // linearDepMaps
        true, ""
    },
    {   // 5 - blocks
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]          # 0
        s_add_u32 sa[4], sa[4], s3      # 4
        v_mad_f32 va[0], va[1], va[2], v0  # 8
        .cf_jump a0,a1,a2
        s_setpc_b64 s[0:1]              # 16
        
a0:     s_mul_i32 sa[3], sa[4], s3      # 20
        s_xor_b32 s4, sa[2], s4         # 24
        s_endpgm                        # 28
        
a1:     v_add_f32 va[2], sa[5], va[0]   # 32
        v_mul_f32 va[3], va[2], v0      # 36
        s_endpgm                        # 40
        
a2:     s_cselect_b32 sa[2], sa[4], sa[3]   # 44
        v_cndmask_b32 va[3], va[0], va[1], vcc     # 48
        s_endpgm                        # 52
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 17 } }, // 0: S0
                { { 0, 17 } }, // 1: S1
                { { 0, 21 } }, // 2: S3
                { { 0, 26 } }, // 3: S4
                { { 0, 25 } }, // 4: sa[2]'0
                { { 45, 46 } }, // 5: sa[2]'1
                { { 0, 20 }, { 44, 45 } }, // 6: sa[3]'0
                { { 21, 22 } }, // 7: sa[3]'1
                { { 1, 5 } }, // 8: sa[4]'0
                { { 5, 21 }, { 44, 45 } }, // 9: sa[4]'1
                { { 0, 20 }, { 32, 33 } }  // 10: sa[5]
            },
            {   // for VGPRs
                { { 0, 20 }, { 32, 37 } }, // 0: V0
                { { 9, 20 }, { 32, 33 }, { 44, 49 } }, // 1: va[0]'0
                { { 0, 20 }, { 44, 49 } }, // 2: va[1]'0
                { { 0, 9 } }, // 3: va[2]'0
                { { 33, 37 } }, // 4: va[2]'1
                { { 37, 38 } }, // 5: va[3]'0
                { { 49, 50 } }  // 6: va[3]'1
            },
            { },
            { }
        },
        {   // linearDepMaps
            {   // for SGPRs
                { 0, { 0, { }, { 1 } } },
                { 1, { 0, { 0 }, { } } }
            },
            { },
            { },
            { }
        },
        true, ""
    },
    {   // 6 - blocks
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]              # 0
        s_add_u32 sa[4], sa[4], s3          # 4
        v_mad_f32 va[0], va[1], va[2], v0   # 8
        s_xor_b32 sa[6], sa[2], sa[4]       # 16
        .cf_jump a0,a1,a2
        s_setpc_b64 s[0:1]                  # 20
        
a0:     s_mul_i32 sa[3], sa[4], s3          # 24
        s_xor_b32 s4, sa[2], s4             # 28
        s_cbranch_scc1 a01                  # 32
a00:    s_add_u32 sa[4], sa[4], s4          # 36
        s_endpgm                            # 40
a01:    s_add_u32 sa[6], sa[6], s4          # 44
        s_endpgm                            # 48
        
a1:     v_add_f32 va[2], sa[5], va[0]       # 52
        v_mul_f32 va[3], va[2], v0          # 56
        s_cbranch_scc1 a11                  # 60
a10:    s_add_u32 sa[5], sa[7], s7          # 64
        s_endpgm                            # 68
a11:    v_add_f32 va[5], va[3], va[1]       # 72
        s_endpgm                            # 76
        
a2:     s_cselect_b32 sa[2], sa[4], sa[3]   # 80
        v_cndmask_b32 va[3], va[0], va[1], vcc  # 84
        s_cbranch_scc1 a21                  # 88
a20:    v_add_f32 va[1], va[3], va[4]       # 92
        s_endpgm                            # 96
a21:    v_add_f32 va[2], va[1], va[0]       # 100
        s_endpgm                            # 104
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 21 } }, // 0: S0
                { { 0, 21 } }, // 1: S1
                { { 0, 25 } }, // 2: S3
                { { 0, 37 }, { 44, 45 } }, // 3: S4
                { { 0, 24 }, { 52, 65 } }, // 4: S7
                { { 0, 29 } }, // 5: sa[2]'0
                { { 81, 82 } }, // 6: sa[2]'1
                { { 0, 24 }, { 80, 81 } }, // 7: sa[3]'0
                { { 25, 26 } }, // 8: sa[3]'1
                { { 1, 5 } }, // 9: sa[4]'0
                { { 5, 37, }, { 80, 81 } }, // 10 sa[4]'1
                { { 37, 38 } }, // 11: sa[4]'2
                { { 0, 24 }, { 52, 53 } }, // 12: sa[5]'0
                { { 65, 66 } }, // 13: sa[5]'1
                { { 17, 36 }, { 44, 45 } }, // 14: sa[6]'0
                { { 45, 46 } }, // 15: sa[6]'1
                { { 0, 24 }, { 52, 65 }  }  // 16: sa[7]'0
            },
            {   // for VGPRs
                { { 0, 24 }, { 52, 57 } }, // 1: V0
                { { 9, 24 }, { 52, 53 }, { 80, 92 }, { 100, 101 } }, // 1: va[0]'0
                { { 0, 24 }, { 52, 64 }, { 72, 73 },
                            { 80, 92 }, { 100, 101 } }, // 2: va[1]'0
                { { 93, 94 } }, // 3: va[1]'1
                { { 0, 9 } }, // 4: va[2]'0
                { { 53, 57 } }, // 5: va[2]'1
                { { 101, 102 } }, // 6: va[2]'2
                { { 57, 64 }, { 72, 73 } }, // 7: va[3]'0
                { { 85, 93 } }, // 8: va[3]'1
                { { 0, 24 }, { 80, 93 } }, // 8: va[4]'0
                { { 73, 74 } }  // 10: va[5]'0
            },
            { },
            { }
        },
        {   // linearDepMaps
            {   // for SGPRs
                { 0, { 0, { }, { 1 } } },
                { 1, { 0, { 0 }, { } } }
            },
            { },
            { },
            { }
        },
        true, ""
    },
    {   // 7 - empty blocks
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[0], sa[2]              # 0
        s_add_u32 sa[1], sa[3], s3          # 4
        v_mad_f32 va[0], va[1], va[2], v0   # 8
        s_xor_b32 sa[2], sa[4], sa[3]       # 16
        .cf_jump a0,a1
        s_setpc_b64 s[0:1]                  # 20
        
a0:     s_mul_i32 sa[1], sa[2], s3          # 24
        s_nop 7                             # 28
        s_cbranch_scc1 a01                  # 32
        s_nop 7                             # 36
        s_cbranch_execz a02                 # 40
        s_nop 7                             # 44
        s_cbranch_vccz a03                  # 48
        s_nop 7                             # 52
        s_cbranch_vccnz a04                 # 56
        s_nop 7                             # 60
        s_mul_i32 s2, sa[4], sa[2]          # 64
        s_cbranch_scc0 a05                  # 68
        s_endpgm                            # 72
        
a01:    s_mul_i32 sa[0], s2, s3             # 76
        s_endpgm                            # 80
        
a02:    s_mul_i32 sa[1], s2, sa[4]          # 84
        s_endpgm                            # 88
        
a03:    v_add_f32 va[3], va[1], va[0]       # 92
        s_endpgm                            # 96
        
a04:    v_add_f32 va[3], va[3], va[4]       # 100
        s_endpgm                            # 104
        
a05:    s_mul_i32 sa[5], s4, sa[6]          # 108
        s_endpgm                            # 112

a1:     v_mul_f32 va[1], sa[1], va[1]       # 116
        v_nop                               # 120
        s_cbranch_scc1 a11                  # 124
        v_nop                               # 128
        s_cbranch_execz a12                 # 132
        v_nop                               # 136
        s_cbranch_vccz a13                  # 140
        v_nop                               # 144
        s_cbranch_vccnz a14                 # 148
        v_nop                               # 152
        s_mul_i32 s2, sa[4], sa[2]          # 156
        s_cbranch_scc0 a15                  # 160
        s_endpgm                            # 164

a11:    v_add_f32 v2, va[2], va[3]          # 168
        s_endpgm                            # 172
        
a12:    v_add_f32 v2, va[0], va[3]          # 176
        s_endpgm                            # 180
        
a13:    v_nop                               # 184
        s_endpgm                            # 188
        
a14:    v_nop                               # 192
        s_endpgm                            # 196
        
a15:    s_mul_i32 sa[5], s4, sa[1]          # 200
        s_endpgm                            # 204
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 21 } }, // 0: S0
                { { 0, 21 } }, // 1: S1
                { { 0, 44 }, { 65, 66 }, { 76, 77 },
                        { 84, 85 }, { 157, 158 } }, // 2: S2
                { { 0, 36 }, { 76, 77 } }, // 3: S3
                { { 0, 72 }, { 108, 109 }, { 116, 164 }, { 200, 201 } }, // 4: S4
                { { 1, 2 } }, // 5: sa[0]'0
                { { 77, 78 } }, // 6: sa[0]'1
                { { 5, 24 }, { 116, 164 }, { 200, 201 } }, // 7: sa[1]'0
                { { 25, 26 } }, // 8: sa[1]'1
                { { 85, 86 } }, // 9: sa[1]'2
                { { 0, 1 } }, // 10: sa[2]'0
                { { 17, 65 }, { 116, 157 } }, // 11: sa[2]'1
                { { 0, 17 } }, // 12: sa[3]'0
                { { 0, 65 }, { 84, 85 }, { 116, 157 } }, // 13: sa[4]'0
                { { 109, 110 } }, // 14: sa[5]'0
                { { 201, 202 } }, // 15: sa[5]'1
                { { 0, 72 }, { 108, 109 } }  // 16: sa[6]'0
            },
            {   // for VGPRs
                { { 0, 9 } }, // 0: V0
                { { 169, 170 }, { 177, 178 } }, // 1: V2
                { { 9, 52 }, { 92, 93 }, { 116, 136 }, { 176, 177 } }, // 2: va[0]'0
                { { 0, 52 }, { 92, 93 }, { 116, 117 } }, // 3: va[1]'0
                { { 117, 118 } }, // 4: va[1]'1
                { { 0, 24 }, { 116, 128 }, { 168, 169 } }, // 5: va[2]'0
                { { 0, 60 }, { 100, 101 }, { 116, 136 },
                        { 168, 169 }, { 176, 177 } }, // 6: va[3]'0
                { { 101, 102 } }, // 7: va[3]'1
                { { 93, 94 } }, // 8: va[3]'2
                { { 0, 60 }, { 100, 101 } }  // 9: va[4]'0
            },
            { },
            { }
        },
        {   // linearDepMaps
            {   // for SGPRs
                { 0, { 0, { }, { 1 } } },
                { 1, { 0, { 0 }, { } } }
            },
            { },
            { },
            { }
        },
        true, ""
    },
    {   // 8 - empty blocks
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[0], sa[2]              # 0
        s_add_u32 sa[1], sa[3], s3          # 4
        v_mad_f32 va[0], va[1], va[2], v0   # 8
        s_xor_b32 sa[2], sa[4], sa[3]       # 16
        .cf_jump a0
        s_setpc_b64 s[0:1]                  # 20
        
a0:     s_mul_i32 sa[1], sa[2], s3          # 24
        s_nop 7                             # 28
        s_cbranch_scc1 a01                  # 32
        s_nop 7                             # 36
        s_cbranch_execz a02                 # 40
        s_nop 7                             # 44
        s_cbranch_vccz a03                  # 48
        s_nop 7                             # 52
        s_cbranch_vccnz a04                 # 56
        s_nop 7                             # 60
        s_nop 1                             # 64
        s_cbranch_scc0 a05                  # 68
        s_endpgm                            # 72
        
a01:    s_mul_i32 sa[0], s2, s3             # 76
        s_endpgm                            # 80
        
a02:    s_mul_i32 sa[1], s2, sa[4]          # 84
        s_endpgm                            # 88
        
a03:    v_add_f32 va[3], va[1], va[0]       # 92
        s_endpgm                            # 96
        
a04:    v_add_f32 va[3], va[3], va[4]       # 100
        s_endpgm                            # 104
        
a05:    s_mul_i32 sa[5], s4, sa[6]          # 108
        s_endpgm                            # 112
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 21 } }, // 0: S0
                { { 0, 21 } }, // 1: S1
                { { 0, 44 }, { 76, 77 }, { 84, 85 } }, // 2: S2
                { { 0, 36 }, { 76, 77 } }, // 3: S3
                { { 0, 72 }, { 108, 109 } }, // 4: S4
                { { 1, 2 } }, // 5: sa[0]'0
                { { 77, 78 } }, // 6: sa[0]'1
                { { 5, 6 } }, // 7: sa[1]'0
                { { 25, 26 } }, // 8: sa[1]'1
                { { 85, 86 } }, // 9: sa[1]'2
                { { 0, 1 } }, // 10: sa[2]'0
                { { 17, 25 } }, // 11: sa[2]'1
                { { 0, 17 } }, // 12: sa[3]'0
                { { 0, 44 }, { 84, 85 } }, // 13: sa[4]'0
                { { 109, 110 } }, // 14: sa[5]'0
                { { 0, 72 }, { 108, 109 } }  // 16: sa[6]'0
            },
            {   // for VGPRs
                { { 0, 9 } }, // 0: V0
                { { 9, 52 }, { 92, 93 } }, // 2: va[0]'0
                { { 0, 52 }, { 92, 93 } }, // 4: va[1]'1
                { { 0, 9 } }, // 5: va[2]'0
                { { 0, 60 }, { 100, 101 } }, // 6: va[3]'0
                { { 101, 102 } }, // 7: va[3]'1
                { { 93, 94 } }, // 8: va[3]'2
                { { 0, 60 }, { 100, 101 } }  // 9: va[4]'0
            },
            { },
            { }
        },
        {   // linearDepMaps
            {   // for SGPRs
                { 0, { 0, { }, { 1 } } },
                { 1, { 0, { 0 }, { } } }
            },
            { },
            { },
            { }
        },
        true, ""
    },
    {   // 9 - simple joins
        R"ffDXD(.regvar sa:s:12, va:v:8
        s_mov_b32 sa[2], s2     # 0
        .cf_jump a1,a2,a3
        s_setpc_b64 s[0:1]      # 4
        
a1:     s_add_u32 sa[2], sa[2], sa[3]   # 8
        s_add_u32 sa[2], sa[2], sa[3]   # 12
        s_branch end                    # 16
a2:     s_add_u32 sa[2], sa[2], sa[4]   # 20
        s_add_u32 sa[2], sa[2], sa[4]   # 24
        s_branch end                    # 28
a3:     s_add_u32 sa[2], sa[2], sa[5]   # 32
        s_add_u32 sa[2], sa[2], sa[5]   # 36
        s_branch end                    # 40

end:    s_xor_b32 sa[2], sa[2], s3      # 44
        s_endpgm                        # 48
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // 0: S0
                { { 0, 5 } }, // 1: S1
                { { 0, 1 } }, // 2: S2
                { { 0, 45 } }, // 3: S3
                { { 1, 9 }, { 20, 21 }, { 32, 33 } }, // 4: sa[2]'0
                { { 9, 13 } }, // 5: sa[2]'1
                { { 13, 20 }, { 25, 32 }, { 37, 45 } }, // 6: sa[2]'2
                { { 45, 46 } }, // 7: sa[2]'3
                { { 21, 25 } }, // 8: sa[2]'4
                { { 33, 37 } }, // 9: sa[2]'5
                { { 0, 13 } }, // 10: sa[3]'0
                { { 0, 8 }, { 20, 25 } }, // 11: sa[4]'0
                { { 0, 8 }, { 32, 37 } }  // 12: sa[5]'0
            },
            { },
            { },
            { }
        },
        {   // linearDepMaps
            { // for SGPRs
                { 0, { 0, { }, { 1 } } },
                { 1, { 0, { 0 }, { } } }
            },
            { },
            { },
            { }
        },
        true, ""
    }
};

static TestSingleVReg getTestSingleVReg(const AsmSingleVReg& vr,
        const std::unordered_map<const AsmRegVar*, CString>& rvMap)
{
    if (vr.regVar == nullptr)
        return { "", vr.index };
    
    auto it = rvMap.find(vr.regVar);
    if (it == rvMap.end())
        throw Exception("getTestSingleVReg: RegVar not found!!");
    return { it->second, vr.index };
}

static void testCreateLivenessesCase(cxuint i, const AsmLivenessesCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input,
                    (ASM_ALL&~ASM_ALTMACRO) | ASM_TESTRUN | ASM_TESTRESOLVE,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " testAsmLivenesses#" << i;
        throw Exception(oss.str());
    }
    const AsmSection& section = assembler.getSections()[0];
    
    AsmRegAllocator regAlloc(assembler);
    
    regAlloc.createCodeStructure(section.codeFlow, section.getSize(),
                            section.content.data());
    regAlloc.createSSAData(*section.usageHandler);
    regAlloc.applySSAReplaces();
    regAlloc.createLivenesses(*section.usageHandler);
    
    std::ostringstream oss;
    oss << " testAsmLivenesses case#" << i;
    const std::string testCaseName = oss.str();
    
    assertValue<bool>("testAsmLivenesses", testCaseName+".good",
                      testCase.good, good);
    assertString("testAsmLivenesses", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
    
    std::unordered_map<const AsmRegVar*, CString> regVarNamesMap;
    for (const auto& rvEntry: assembler.getRegVarMap())
        regVarNamesMap.insert(std::make_pair(&rvEntry.second, rvEntry.first));
    
    // generate livenesses indices conversion table
    const VarIndexMap* vregIndexMaps = regAlloc.getVregIndexMaps();
    
    std::vector<size_t> lvIndexCvtTables[MAX_REGTYPES_NUM];
    std::vector<size_t> revLvIndexCvtTables[MAX_REGTYPES_NUM];
    for (size_t r = 0; r < MAX_REGTYPES_NUM; r++)
    {
        const VarIndexMap& vregIndexMap = vregIndexMaps[r];
        Array<std::pair<TestSingleVReg, const std::vector<size_t>*> > outVregIdxMap(
                    vregIndexMap.size());
        
        size_t j = 0;
        for (const auto& entry: vregIndexMap)
        {
            TestSingleVReg vreg = getTestSingleVReg(entry.first, regVarNamesMap);
            outVregIdxMap[j++] = std::make_pair(vreg, &entry.second);
        }
        mapSort(outVregIdxMap.begin(), outVregIdxMap.end());
        
        std::vector<size_t>& lvIndexCvtTable = lvIndexCvtTables[r];
        std::vector<size_t>& revLvIndexCvtTable = revLvIndexCvtTables[r];
        // generate livenessCvt table
        for (const auto& entry: outVregIdxMap)
            for (size_t v: *entry.second)
                if (v != SIZE_MAX)
                {
                    /*std::cout << "lvidx: " << v << ": " << entry.first.name << ":" <<
                            entry.first.index << std::endl;*/
                    size_t j = lvIndexCvtTable.size();
                    lvIndexCvtTable.push_back(v);
                    if (v+1 > revLvIndexCvtTable.size())
                        revLvIndexCvtTable.resize(v+1);
                    revLvIndexCvtTable[v] = j;
                }
    }
    
    const Array<OutLiveness>* resLivenesses = regAlloc.getOutLivenesses();
    for (size_t r = 0; r < MAX_REGTYPES_NUM; r++)
    {
        std::ostringstream rOss;
        rOss << "live.regtype#" << r;
        rOss.flush();
        std::string rtname(rOss.str());
        
        assertValue("testAsmLivenesses", testCaseName + rtname + ".size",
                    testCase.livenesses[r].size(), resLivenesses[r].size());
        
        for (size_t li = 0; li < resLivenesses[r].size(); li++)
        {
            std::ostringstream lOss;
            lOss << ".liveness#" << li;
            lOss.flush();
            std::string lvname(rtname + lOss.str());
            const OutLiveness& expLv = testCase.livenesses[r][li];
            const OutLiveness& resLv = resLivenesses[r][lvIndexCvtTables[r][li]];
            
            // checking liveness
            assertValue("testAsmLivenesses", testCaseName + lvname + ".size",
                    expLv.size(), resLv.size());
            for (size_t k = 0; k < resLv.size(); k++)
            {
                std::ostringstream regOss;
                regOss << ".reg#" << k;
                regOss.flush();
                std::string regname(lvname + regOss.str());
                assertValue("testAsmLivenesses", testCaseName + regname + ".first",
                    expLv[k].first, resLv[k].first);
                assertValue("testAsmLivenesses", testCaseName + regname + ".second",
                    expLv[k].second, resLv[k].second);
            }
        }
    }
    
    const std::unordered_map<size_t, LinearDep>* resLinearDepMaps =
                regAlloc.getLinearDepMaps();
    for (size_t r = 0; r < MAX_REGTYPES_NUM; r++)
    {
        std::ostringstream rOss;
        rOss << "lndep.regtype#" << r;
        rOss.flush();
        std::string rtname(rOss.str());
        
        assertValue("testAsmLivenesses", testCaseName + rtname + ".size",
                    testCase.linearDepMaps[r].size(), resLinearDepMaps[r].size());
        
        for (size_t di = 0; di < testCase.linearDepMaps[r].size(); di++)
        {
            std::ostringstream lOss;
            lOss << ".lndep#" << di;
            lOss.flush();
            std::string ldname(rtname + lOss.str());
            const auto& expLinearDepEntry = testCase.linearDepMaps[r][di];
            auto rlit = resLinearDepMaps[r].find(
                            lvIndexCvtTables[r][expLinearDepEntry.first]);
            
            std::ostringstream vOss;
            vOss << expLinearDepEntry.first;
            vOss.flush();
            assertTrue("testAsmLivenesses", testCaseName + ldname + ".key=" + vOss.str(),
                        rlit != resLinearDepMaps[r].end());
            const LinearDep2& expLinearDep = expLinearDepEntry.second;
            const LinearDep& resLinearDep = rlit->second;
            
            assertValue("testAsmLivenesses", testCaseName + ldname + ".align",
                        cxuint(expLinearDep.align), cxuint(resLinearDep.align));
            
            Array<size_t> resPrevVidxes(resLinearDep.prevVidxes.size());
            // convert to res ssaIdIndices
            for (size_t k = 0; k < resLinearDep.prevVidxes.size(); k++)
                resPrevVidxes[k] = revLvIndexCvtTables[r][resLinearDep.prevVidxes[k]];
            
            assertArray("testAsmLivenesses", testCaseName + ldname + ".prevVidxes",
                        expLinearDep.prevVidxes, resPrevVidxes);
            
            Array<size_t> resNextVidxes(resLinearDep.nextVidxes.size());
            // convert to res ssaIdIndices
            for (size_t k = 0; k < resLinearDep.nextVidxes.size(); k++)
                resNextVidxes[k] = revLvIndexCvtTables[r][resLinearDep.nextVidxes[k]];
            
            assertArray("testAsmLivenesses", testCaseName + ldname + ".nextVidxes",
                        expLinearDep.nextVidxes, resNextVidxes);
        }
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(createLivenessesCasesTbl)/sizeof(AsmLivenessesCase); i++)
        try
        { testCreateLivenessesCase(i, createLivenessesCasesTbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
