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
typedef AsmRegAllocator::VVarSetEntry VVarSetEntry;

struct LinearDep2
{
    cxbyte align;
    Array<size_t> prevVidxes;
    Array<size_t> nextVidxes;
};

typedef Array<size_t> VVarSetEntry2[4];

struct AsmLivenessesCase
{
    const char* input;
    Array<OutLiveness> livenesses[MAX_REGTYPES_NUM];
    Array<std::pair<size_t, LinearDep2> > linearDepMaps[MAX_REGTYPES_NUM];
    Array<std::pair<size_t, VVarSetEntry2> > varRoutineMap;
    Array<std::pair<size_t, VVarSetEntry2> > varCallMap;
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
        { },  // varRoutineMap
        { },  // varCallMap
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
        { },  // varRoutineMap
        { },  // varCallMap
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
                { 2, { 2, { }, { 3 } } },  // sa[2]'0
                { 3, { 0, { 2 }, { } } },  // sa[3]'0
                { 4, { 2, { }, { 6 } } },  // sa[4]'0
                { 5, { 2, { }, { 7 } } },  // sa[4]'1
                { 6, { 0, { 4 }, { } } },  // sa[5]'0
                { 7, { 0, { 5 }, { } } },   // sa[5]'1
                { 9, { 4, { }, { 10 } } }, // sbuf[0]'0
                { 10, { 0, { 9 }, { 11 } } }, // sbuf[1]'0
                { 11, { 0, { 10 }, { 12 } } }, // sbuf[2]'0
                { 12, { 0, { 11 }, { } } }  // sbuf[3]'0
            },
            {   // for VGPRs
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
        { },  // varRoutineMap
        { },  // varCallMap
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
        { },  // varRoutineMap
        { },  // varCallMap
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
        { },  // varRoutineMap
        { },  // varCallMap
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
            { },
            { },
            { },
            { }
        },
        { },  // varRoutineMap
        { },  // varCallMap
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
            { },
            { },
            { },
            { }
        },
        { },  // varRoutineMap
        { },  // varCallMap
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
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
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
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
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
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    },
    {   // 10 - more complex join
        R"ffDXD(.regvar sa:s:12, va:v:8
        s_mov_b32 sa[2], s2             # 0
        s_xor_b32 sa[6], sa[2], s3      # 4
        .cf_jump a1,a2,a3
        s_setpc_b64 s[0:1]              # 8
        
a1:     s_add_u32 sa[2], sa[2], sa[3]   # 12
        s_add_u32 sa[2], sa[2], sa[3]   # 16
        .cf_jump b1,b2
        s_setpc_b64 s[0:1]              # 20
a2:     s_add_u32 sa[2], sa[2], sa[4]   # 24
        s_add_u32 sa[2], sa[2], sa[4]   # 28
        .cf_jump b1,b2
        s_setpc_b64 s[0:1]              # 32
a3:     s_add_u32 sa[2], sa[2], sa[5]   # 36
        s_add_u32 sa[2], sa[2], sa[5]   # 40
        .cf_jump b1,b2
        s_setpc_b64 s[0:1]              # 44
        
b1:     s_add_u32 sa[5], sa[6], sa[4]   # 48
        s_branch end                    # 52
b2:     s_add_u32 sa[6], sa[5], sa[4]   # 56
        s_branch end                    # 60
        
end:    s_xor_b32 sa[2], sa[2], s3      # 64
        s_xor_b32 sa[3], sa[6], s4      # 68
        s_endpgm                        # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 21 }, { 24, 33 }, { 36, 45 } }, // 0: S0
                { { 0, 21 }, { 24, 33 }, { 36, 45 } }, // 1: S1
                { { 0, 1 } }, // 2: S2
                { { 0, 65 } }, // 3: S3
                { { 0, 69 } }, // 4: S4
                { { 1, 13 }, { 24, 25 }, { 36, 37 } }, // 5: sa[2]'0
                { { 13, 17 } }, // 6: sa[2]'1
                { { 17, 24 }, { 29, 36 }, { 41, 65 } }, // 7: sa[2]'2
                { { 65, 66 } }, // 8: sa[2]'3
                { { 25, 29 } }, // 9: sa[2]'4
                { { 37, 41 } }, // 10: sa[2]'5
                { { 0, 17 } }, // 11: sa[3]'0
                { { 69, 70 } }, // 12: sa[3]'1
                { { 0, 49 }, { 56, 57 } }, // 13: sa[4]'0
                { { 0, 48 }, { 56, 57 } }, // 14: sa[5]'0
                { { 49, 50 } }, // 15: sa[5]'0
                { { 5, 56 }, { 57, 69 } }  // 16: sa[6]'0
            },
            { },
            { },
            { }
        },
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    },
    {   // 11 - simple loop
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[0], 0              # 0
        s_mov_b32 sa[1], s10            # 4
        v_mov_b32 va[0], v0             # 8
        v_mov_b32 va[1], v1             # 12
        v_mov_b32 va[3], 0              # 16
        v_mov_b32 va[4], 11             # 20
loop:
        ds_read_b32 va[2], va[3]        # 24
        v_xor_b32 va[0], va[1], va[2]   # 32
        v_not_b32 va[0], va[0]          # 36
        v_xor_b32 va[0], 0xfff, va[0]   # 40
        v_xor_b32 va[4], va[4], va[2]   # 48
        v_not_b32 va[4], va[0]          # 52
        v_xor_b32 va[4], 0xfff, va[0]   # 56
        v_add_u32 va[1], vcc, 1001, va[1]   # 64
        
        s_add_u32 sa[0], sa[0], 1       # 72
        s_cmp_lt_u32 sa[0], sa[1]       # 76
        s_cbranch_scc1 loop             # 80
        
        v_xor_b32 va[0], 33, va[0]      # 84
        s_add_u32 sa[0], 14, sa[0]      # 88
        s_endpgm                        # 92
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // 0: S10
                { { 1, 89 } }, // 1: sa[0]'0
                { { 89, 90 } }, // 2: sa[0]'1
                { { 5, 84 } }  // 3: sa[1]'0
            },
            {   // for VGPRs
                { { 0, 9 } }, // 0: V0
                { { 0, 13 } }, // 1: V1
                { { 9, 10 } }, // va[0]'0
                { { 33, 37 } }, // va[0]'1
                { { 37, 41 } }, // va[0]'2
                { { 41, 85 } }, // va[0]'3
                { { 85, 86 } }, // va[0]'4
                { { 13, 84 } }, // va[1]'0
                { { 25, 49 } }, // va[2]'0
                { { 17, 84 } }, // va[3]'0
                { { 21, 49 }, { 57, 84 } }, // va[4]'0
                { { 49, 50 } }, // va[4]'1
                { { 53, 54 } }  // va[4]'2
            },
            { },
            { }
        },
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    },
    {   // 12 - simple loop with fork (regvar used only in these forks)
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
loop:   s_cbranch_scc1 b1               # 4
        
b0:     s_add_u32 sa[2], sa[2], sa[0]   # 8
        s_branch loopend                # 12
b1:     s_add_u32 sa[2], sa[2], sa[1]   # 16
loopend:
        s_cbranch_scc0 loop             # 20
        s_endpgm                        # 24
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 1 } }, // 0: S4
                { { 0, 24 } }, // 1: sa[0]'0
                { { 0, 24 } }, // 2: sa[1]'0
                { { 1, 24 } }  // 3: sa[2]'0
            },
            { },
            { },
            { }
        },
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    },
    {   // 13 - simple with forks - holes in loop (second case)
        R"ffDXD(.regvar sa:s:8, va:v:8
loop:   s_cbranch_scc1 b1               # 0
        
b0:     s_add_u32 sa[2], sa[2], sa[0]   # 4
        s_add_u32 sa[3], sa[0], sa[1]   # 8
        s_branch loopend                # 12
b1:     s_add_u32 sa[2], sa[0], sa[1]   # 16
        s_add_u32 sa[3], sa[3], sa[1]   # 20
loopend:
        s_cbranch_scc0 loop             # 24
        s_endpgm                        # 28
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 28 } }, // 0: sa[0]'0
                { { 0, 28 } }, // 1: sa[1]'0
                { { 0, 16 }, { 17, 28 } }, // 2: sa[2]'0
                { { 0, 4 }, { 9, 28 } }  // 3: sa[3]'0
            },
            { },
            { },
            { }
        },
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    },
    {   // 14 - two loops, one nested
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b64 sa[0:1], 0            # 0
        v_mov_b32 va[0], v1             # 4
        v_mov_b32 va[1], v2             # 8
        v_mov_b32 va[2], v3             # 12
        
loop0:  v_lshrrev_b32 va[0], va[2], va[0]   # 16
        v_xor_b32 va[0], va[1], va[0]       # 20
        
        s_xor_b32 sa[3], sa[0], 0x5     # 24
        s_cmp_eq_u32 sa[3], 9           # 28
        s_cbranch_scc1 bb0              # 32
        
        v_and_b32 va[0], -15, va[0]         # 36
        v_sub_u32 va[0], vcc, -13, va[0]    # 40
        s_branch loop1start                 # 44
        
bb0:    v_xor_b32 va[0], 15, va[0]          # 48
        v_add_u32 va[0], vcc, 17, va[0]     # 52
loop1start:
        s_mov_b32 sa[1], sa[0]              # 56
        
loop1:  v_add_u32 va[2], vcc, va[2], va[1]  # 60
        v_xor_b32 va[2], 0xffaaaa, va[0]    # 64
        
        s_xor_b32 sa[2], sa[1], 0x5     # 72
        s_cmp_eq_u32 sa[2], 7           # 76
        s_cbranch_scc1 bb1              # 80
        
        v_sub_u32 va[1], vcc, 5, va[1]      # 84
        v_sub_u32 va[2], vcc, 7, va[2]      # 88
        s_branch loop1end                   # 92
        
bb1:    v_xor_b32 va[1], 15, va[1]          # 96
        v_xor_b32 va[2], 17, va[2]          # 100
loop1end:
        
        s_add_u32 sa[1], sa[1], 1       # 104
        s_cmp_lt_u32 sa[1], 52          # 108
        s_cbranch_scc1 loop1            # 112
        
        v_xor_b32 va[0], va[1], va[0]   # 116
        v_xor_b32 va[0], va[2], va[0]   # 120
        v_xor_b32 va[0], sa[0], va[0]   # 124
        
        s_add_u32 sa[0], sa[0], 1       # 128
        s_cmp_lt_u32 sa[0], 33          # 132
        s_cbranch_scc1 loop0            # 136
        
        s_endpgm                        # 140
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 1, 140 } }, // 0: sa[0]'0
                { { 1, 2 } }, // 1: sa[1]'0
                { { 57, 116 } }, // 2: sa[1]'1
                { { 73, 77 } }, // 3: sa[2]'0
                { { 25, 29 } }  // 4: sa[3]'0
            },
            {   // for VGPRs
                { { 0, 5 } }, // 0: V1
                { { 0, 9 } }, // 1: V2
                { { 0, 13 } }, // 2: V3
                { { 5, 17 }, { 125, 140 } }, // 3: va[0]'0
                { { 17, 21 } }, // 4: va[0]'1
                { { 21, 37 }, { 48, 49 } }, // 5: va[0]'2
                { { 37, 41 } }, // 6: va[0]'3
                { { 41, 48 }, { 53, 117 } }, // 7: va[0]'4
                { { 117, 121 } }, // 8: va[0]'5
                { { 121, 125 } }, // 9: va[0]'6
                { { 49, 53 } }, // 10: va[0]'7
                { { 9, 140 } }, // 11: va[1]'0
                { { 13, 61 }, { 89, 96 }, { 101, 140 } }, // 12: va[2]'0
                { { 61, 62 } }, // 13: va[2]'1
                { { 65, 89 }, { 96, 101 } }  // 14: va[2]'2
            },
            { },
            { }
        },
        {   // linearDepMaps
            { // for SGPRs
                { 0, { 2, { }, { 1 } } },
                { 1, { 0, { 0 }, { } } }
            },
            { },
            { },
            { }
        },
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    },
    {   // 15 - trick - SSA replaces beyond visited point
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        
loop:   s_xor_b32 sa[2], sa[2], sa[4]   # 8
        s_cbranch_scc0 end              # 12
        
        s_xor_b32 sa[3], sa[2], sa[4]   # 16
        s_cbranch_scc0 loop             # 20
        
        s_endpgm                        # 24
        
end:    s_xor_b32 sa[3], sa[3], sa[4]   # 28
        s_endpgm                        # 32
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 1 } }, // 0: S4
                { { 0, 5 } }, // 1: S5
                { { 1, 24 } }, // 2: sa[2]'0
                { { 5, 16 }, { 17, 24 }, { 28, 29 } }, // 3: sa[3]'0
                { { 29, 30 } }, // 4: sa[3]'1
                { { 0, 24 }, { 28, 29 } }  // 5: sa[4]'0
            },
            { },
            { },
            { }
        },
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    },
    {   // 16 - trick - SSA replaces beyond visited point
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        
loop:   s_xor_b32 sa[2], sa[2], sa[4]   # 8
        s_cbranch_scc0 end              # 12
        
        s_xor_b32 sa[3], sa[2], sa[4]   # 16
        s_cbranch_scc0 loop             # 20
        
end:    s_xor_b32 sa[3], sa[3], sa[4]   # 24
        s_endpgm                        # 28
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 1 } }, // 0: S4
                { { 0, 5 } }, // 1: S5
                { { 1, 24 } }, // 2: sa[2]'0
                { { 5, 16 }, { 17, 25 } }, // 3: sa[3]'0
                { { 25, 26 } }, // 4: sa[3]'1
                { { 0, 25 } }  // 5: sa[4]'0
            },
            { },
            { },
            { }
        },
        { },  // linearDepMaps
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    },
#if 0
    {   // 17 - simple call
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        
        s_getpc_b64 s[2:3]              # 8
        s_add_u32 s2, s2, routine-.     # 12
        s_add_u32 s3, s3, routine-.+4   # 20
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 28
        
        s_lshl_b32 sa[2], sa[2], 3      # 32
        s_lshl_b32 sa[3], sa[3], 4      # 36
        s_endpgm                        # 40
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 44
        s_xor_b32 sa[3], sa[3], sa[4]   # 48
        .cf_ret
        s_setpc_b64 s[0:1]              # 52
)ffDXD",
        {
            {   // for SGPRs
                { { 29, 32 }, { 44, 53 } }, // 0: S0
                { { 29, 32 }, { 44, 53 } }, // 1: S1
                { { 9, 29 } }, // 2: S2
                { { 9, 29 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 5 } }, // 5: S5
                { { 1, 32 }, { 44, 45 } }, // 6: sa[2]'0
                { }, // 7: sa[2]'1
                { }, // 8: sa[2]'2
                { }, // 9: sa[3]'0
                { }, // 10: sa[3]'1
                { }, // 11: sa[3]'2
                { }  // 12: sa[4]'0
            },
            { },
            { },
            { }
        },
        { }, //
        { },  // varRoutineMap
        { },  // varCallMap
        true, ""
    }
#endif
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
    
    // checking linearDepMaps
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
    
    // checking varCallMap
    const std::unordered_map<size_t, VVarSetEntry>& varCallMap = regAlloc.getVarCallMap();
    assertValue("testAsmLivenesses", testCaseName + "varCallMap.size",
            varCallMap.size(), testCase.varCallMap.size());
    
    for (size_t j = 0; j < varCallMap.size(); j++)
    {
        std::ostringstream vOss;
        vOss << "varCallMap#" << j;
        vOss.flush();
        std::string vcname(vOss.str());
        
        auto vcit = varCallMap.find(testCase.varCallMap[j].first);
        std::ostringstream kOss;
        kOss << testCase.varCallMap[j].first;
        kOss.flush();
        assertTrue("testAsmLivenesses", testCaseName + vcname +".key=" + kOss.str(),
                    vcit != varCallMap.end());
        
        const VVarSetEntry2& expEntry = testCase.varCallMap[j].second;
        const VVarSetEntry& resEntry = vcit->second;
        for (cxuint r = 0; r < MAX_REGTYPES_NUM; r++)
        {
            std::ostringstream vsOss;
            vsOss << ".vs#" << r;
            vsOss.flush();
            std::string vsname = vcname + vsOss.str();
            
            assertValue("testAsmLivenesses", testCaseName + vsname +".size",
                    expEntry[r].size(), resEntry.vvars[r].size());
            
            for (size_t k = 0; k < expEntry[r].size(); k++)
            {
                std::ostringstream vskOss;
                vskOss << ".elem#" << r << "=" << expEntry[r][k];
                vskOss.flush();
                std::string vskname = vsname + vskOss.str();
                
                assertTrue("testAsmLivenesses", testCaseName + vskname +".size",
                        resEntry.vvars[r].find(lvIndexCvtTables[r][expEntry[r][k]]) !=
                        resEntry.vvars[r].end());
            }
        }
    }
    
    const std::unordered_map<size_t, VVarSetEntry>& varRoutineMap =
                regAlloc.getVarRoutineMap();
    assertValue("testAsmLivenesses", testCaseName + "varRoutineMap.size",
            varRoutineMap.size(), testCase.varRoutineMap.size());
    
    for (size_t j = 0; j < varRoutineMap.size(); j++)
    {
        std::ostringstream vOss;
        vOss << "varRoutineMap#" << j;
        vOss.flush();
        std::string vcname(vOss.str());
        
        auto vcit = varRoutineMap.find(testCase.varRoutineMap[j].first);
        std::ostringstream kOss;
        kOss << testCase.varRoutineMap[j].first;
        kOss.flush();
        assertTrue("testAsmLivenesses", testCaseName + vcname +".key=" + kOss.str(),
                    vcit != varRoutineMap.end());
        
        const VVarSetEntry2& expEntry = testCase.varRoutineMap[j].second;
        const VVarSetEntry& resEntry = vcit->second;
        for (cxuint r = 0; r < MAX_REGTYPES_NUM; r++)
        {
            std::ostringstream vsOss;
            vsOss << ".vs#" << r;
            vsOss.flush();
            std::string vsname = vcname + vsOss.str();
            
            assertValue("testAsmLivenesses", testCaseName + vsname +".size",
                    expEntry[r].size(), resEntry.vvars[r].size());
            
            for (size_t k = 0; k < expEntry[r].size(); k++)
            {
                std::ostringstream vskOss;
                vskOss << ".elem#" << r << "=" << expEntry[r][k];
                vskOss.flush();
                std::string vskname = vsname + vskOss.str();
                
                assertTrue("testAsmLivenesses", testCaseName + vskname +".size",
                        resEntry.vvars[r].find(lvIndexCvtTables[r][expEntry[r][k]]) !=
                        resEntry.vvars[r].end());
            }
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
