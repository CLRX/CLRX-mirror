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
#include <iterator>
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
typedef AsmRegAllocator::VIdxSetEntry VIdxSetEntry;

struct LinearDep2
{
    cxbyte align;
    Array<size_t> prevVidxes;
    Array<size_t> nextVidxes;
};

struct VIdxSetEntry2
{
    Array<size_t> vs[4];
};

struct AsmLivenessesCase
{
    const char* input;
    Array<OutLiveness> livenesses[MAX_REGTYPES_NUM];
    Array<std::pair<size_t, LinearDep2> > linearDepMaps[MAX_REGTYPES_NUM];
    Array<std::pair<size_t, VIdxSetEntry2> > vidxRoutineMap;
    Array<std::pair<size_t, VIdxSetEntry2> > vidxCallMap;
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        v_xor_b32 va[3], va[0], va[1]     # 48
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        v_xor_b32 va[3], va[0], va[1]       # 84
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        v_xor_b32 va[1], 1001, va[1]   # 64
        
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        v_xor_b32 va[0], -13, va[0]         # 40
        s_branch loop1start                 # 44
        
bb0:    v_xor_b32 va[0], 15, va[0]          # 48
        v_and_b32 va[0], 17, va[0]          # 52
loop1start:
        s_mov_b32 sa[1], sa[0]              # 56
        
loop1:  v_and_b32 va[2], va[2], va[1]       # 60
        v_xor_b32 va[2], 0xffaaaa, va[0]    # 64
        
        s_xor_b32 sa[2], sa[1], 0x5     # 72
        s_cmp_eq_u32 sa[2], 7           # 76
        s_cbranch_scc1 bb1              # 80
        
        v_xor_b32 va[1], 5, va[1]      # 84
        v_xor_b32 va[2], 7, va[2]      # 88
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
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
        { },  // vidxRoutineMap
        { },  // vidxCallMap
        true, ""
    },
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
        {   // livenesses
            {   // for SGPRs
                { { 29, 32 }, { 44, 53 } }, // 0: S0
                { { 29, 32 }, { 44, 53 } }, // 1: S1
                { { 9, 29 } }, // 2: S2
                { { 9, 29 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 5 } }, // 5: S5
                { { 1, 32 }, { 44, 45 } }, // 6: sa[2]'0
                { { 32, 33 }, { 45, 56 } }, // 7: sa[2]'1
                { { 33, 34 } }, // 8: sa[2]'2
                { { 5, 32 }, { 44, 49 } }, // 9: sa[3]'0
                { { 32, 37 }, { 49, 56 } }, // 10: sa[3]'1
                { { 37, 38 } }, // 11: sa[3]'2
                { { 0, 32 }, { 44, 49 } }  // 12: sa[4]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 9, 10, 12 }, { }, { }, { } } } }
        },
        { },  // vidxCallMap
        true, ""
    },
    {   // 18 - simple call, more complex routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        s_getpc_b64 s[2:3]              # 12
        s_add_u32 s2, s2, routine-.     # 16
        s_add_u32 s3, s3, routine-.+4   # 24
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 32
        
        s_lshl_b32 sa[2], sa[2], 3      # 36
        s_lshl_b32 sa[3], sa[3], 4      # 40
        s_lshl_b32 sa[5], sa[5], 5      # 44
        s_endpgm                        # 48
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 52
        s_xor_b32 sa[3], sa[3], sa[4]   # 56
        s_cbranch_scc1 bb1              # 60
        
        s_min_u32 sa[2], sa[2], sa[4]   # 64
        s_min_u32 sa[3], sa[3], sa[4]   # 68
        .cf_ret
        s_setpc_b64 s[0:1]              # 72
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 76
        s_and_b32 sa[3], sa[3], sa[4]   # 80
        .cf_ret
        s_setpc_b64 s[0:1]              # 84
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 33, 36 }, { 52, 73 }, { 76, 85 } }, // 0: S0
                { { 33, 36 }, { 52, 73 }, { 76, 85 } }, // 1: S1
                { { 13, 33 } }, // 2: S2
                { { 13, 33 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 1, 36 }, { 52, 53 } }, // 6: sa[2]'0
                { { 53, 65 }, { 76, 77 } }, // 7: sa[2]'1
                { { 36, 37 }, { 65, 76 }, { 77, 88 } }, // 8: sa[2]'2
                { { 37, 38 } }, // 9: sa[2]'3
                { { 5, 36 }, { 52, 57 } }, // 10: sa[3]'0
                { { 57, 69 }, { 76, 81 } }, // 11: sa[3]'1
                { { 36, 41 }, { 69, 76 }, { 81, 88 } }, // 12: sa[3]'2
                { { 41, 42 } }, // 13: sa[3]'3
                { { 0, 36 }, { 52, 69 }, { 76, 81 } }, // 14: sa[4]'0
                { { 9, 45 } }, // 15: sa[5]'0
                { { 45, 46 } }  // 16: sa[5]'1
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 8, 10, 11, 12, 14 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 15 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 19 - simple call, more complex routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        s_getpc_b64 s[2:3]              # 12
        s_add_u32 s2, s2, routine-.     # 16
        s_add_u32 s3, s3, routine-.+4   # 24
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 32
        
        s_lshl_b32 sa[2], sa[2], 3      # 36
        s_lshl_b32 sa[3], sa[3], sa[5]  # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 48
        s_cbranch_scc1 bb1              # 52
        
        s_min_u32 sa[2], sa[2], sa[4]   # 56
        s_xor_b32 sa[3], sa[3], sa[4]   # 60
        .cf_ret
        s_setpc_b64 s[0:1]              # 64
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 68
        .cf_ret
        s_setpc_b64 s[0:1]              # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 33, 36 }, { 48, 65 }, { 68, 73 } }, // 0: S0
                { { 33, 36 }, { 48, 65 }, { 68, 73 } }, // 1: S1
                { { 13, 33 } }, // 2: S2
                { { 13, 33 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 1, 36 }, { 48, 49 } }, // 6: sa[2]'0
                { { 49, 57 }, { 68, 69 } }, // 7: sa[2]'1
                { { 36, 37 }, { 57, 68 }, { 69, 76 } }, // 8: sa[2]'2
                { { 37, 38 } }, // 9: sa[2]'3
                { { 5, 41 }, { 48, 76 } }, // 10: sa[3]'0
                { { 41, 42 } }, // 11: sa[3]'1
                { { 0, 36 }, { 48, 61 }, { 68, 69 } }, // 12: sa[4]'0
                { { 9, 41 } }  // 13: sa[5]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 8, 10, 12 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 13 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 20 - multiple call of routine
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
        
        s_getpc_b64 s[2:3]              # 40
        s_add_u32 s2, s2, routine-.     # 44
        s_add_u32 s3, s3, routine-.+4   # 52
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 60
        
        s_ashr_i32 sa[2], sa[2], 3      # 64
        s_ashr_i32 sa[2], sa[2], 3      # 68
        s_ashr_i32 sa[3], sa[3], 4      # 72
        s_ashr_i32 sa[3], sa[3], 4      # 76
        
        s_getpc_b64 s[2:3]              # 80
        s_add_u32 s2, s2, routine-.     # 84
        s_add_u32 s3, s3, routine-.+4   # 92
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 100
        
        s_ashr_i32 sa[2], sa[2], 3      # 104
        s_ashr_i32 sa[3], sa[3], 3      # 108
        s_endpgm                        # 112
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 116
        s_xor_b32 sa[3], sa[3], sa[4]   # 120
        s_cbranch_scc1 bb1              # 124
        
        s_min_u32 sa[2], sa[2], sa[4]   # 128
        .cf_ret
        s_setpc_b64 s[0:1]              # 132
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 136
        .cf_ret
        s_setpc_b64 s[0:1]              # 140
)ffDXD",
        {   // livenesses
            {
                { { 29, 32 }, { 61, 64 }, { 101, 104 },
                        { 116, 133 }, { 136, 141 } }, // 0: S0
                { { 29, 32 }, { 61, 64 }, { 101, 104 },
                        { 116, 133 }, { 136, 141 } }, // 1: S1
                { { 9, 29 }, { 41, 61 }, { 81, 101 } }, // 2: S2
                { { 9, 29 }, { 41, 61 }, { 81, 101 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 5 } }, // 5: S5
                { { 1, 32 }, { 33, 64 }, { 69, 104 }, { 116, 117 } }, // 6: sa[2]'0
                { { 117, 129 }, { 136, 137 } }, // 7: sa[2]'1
                { { 32, 33 }, { 64, 65 }, { 104, 105 },
                        { 129, 136 }, { 137, 144 } }, // 8: sa[2]'2
                { { 65, 69 } }, // 9: sa[2]'3
                { { 105, 106 } }, // 10: sa[2]'4
                { { 5, 32 }, { 37, 64 }, { 77, 104 }, { 116, 121 } }, // 11: sa[3]'0
                { { 32, 37 }, { 64, 73 }, { 104, 109 }, { 121, 144 } }, // 12: sa[3]'1
                { { 73, 77 } }, // 13: sa[3]'2
                { { 109, 110 } }, // 14: sa[3]'3
                { { 0, 104 }, { 116, 144 } }  // 15: sa[4]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 4, { { { 0, 1, 6, 7, 8, 11, 12, 15 }, { }, { }, { } } } }
        },
        { }, // vidxCallMap
        true, ""
    },
    {   // 21 - many nested routines - path penetration
        R"ffDXD(.regvar sa:s:8, va:v:8
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]         # 0
        
        s_add_u32 sa[7], sa[7], sa[1]       # 4
        s_add_u32 sa[7], sa[7], sa[2]       # 8
        s_add_u32 sa[7], sa[7], sa[3]       # 12
        s_add_u32 sa[7], sa[7], sa[4]       # 16
        s_add_u32 sa[7], sa[7], sa[5]       # 20
        s_add_u32 sa[7], sa[7], sa[6]       # 24
        v_xor_b32 va[7], va[7], va[0]       # 28
        v_xor_b32 va[7], va[7], va[1]       # 32
        v_xor_b32 va[7], va[7], va[2]       # 36
        v_xor_b32 va[7], va[7], va[3]       # 40
        s_endpgm                            # 44
        
routine:
        v_add_f32 va[3], va[3], va[1]       # 48
        s_cbranch_scc0 r1_2                 # 52
        
r1_1:   s_add_u32 sa[2], sa[2], sa[0]       # 56
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]         # 60
        v_add_f32 va[3], va[3], va[1]       # 64
        .cf_ret
        s_setpc_b64 s[0:1]                  # 68
        
r1_2:   s_add_u32 sa[6], sa[6], sa[0]       # 72
        .cf_call routine3
        s_swappc_b64 s[0:1], s[2:3]         # 76
        v_add_f32 va[2], va[2], va[1]       # 80
        .cf_ret
        s_setpc_b64 s[0:1]                  # 84

routine2:
        s_add_u32 sa[2], sa[2], sa[0]       # 88
        s_cbranch_scc0 r2_2                 # 92
        
r2_1:   s_add_u32 sa[2], sa[2], sa[0]       # 96
        .cf_ret
        s_setpc_b64 s[0:1]                  # 100
r2_2:   s_add_u32 sa[3], sa[3], sa[0]       # 104
        .cf_ret
        s_setpc_b64 s[0:1]                  # 108

routine3:
        s_add_u32 sa[4], sa[4], sa[0]       # 112
        s_cbranch_scc0 r3_2                 # 116
        
r3_1:   s_add_u32 sa[4], sa[4], sa[0]       # 120
        .cf_ret
        s_setpc_b64 s[0:1]                  # 124
r3_2:   s_add_u32 sa[5], sa[5], sa[0]       # 128
        s_cbranch_scc0 r3_4                 # 132
r3_3:   v_add_f32 va[0], va[0], va[1]       # 136
        .cf_ret
        s_setpc_b64 s[0:1]                  # 140
r3_4:
        v_add_f32 va[0], va[0], va[1]       # 144
        .cf_ret
        s_setpc_b64 s[0:1]                  # 148
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 1, 2 }, { 61, 69 }, { 77, 85 }, { 88, 152 } }, // 0: S0
                { { 1, 2 }, { 61, 69 }, { 77, 85 }, { 88, 152 } }, // 1: S1
                { { 0, 4 }, { 48, 61 }, { 72, 77 } }, // 2: S2
                { { 0, 4 }, { 48, 61 }, { 72, 77 } }, // 3: S3
                { { 0, 4 }, { 48, 64 }, { 72, 80 }, { 88, 97 },
                    { 104, 105 }, { 112, 121 }, { 128, 129 } }, // 4: sa[0]'0
                { { 0, 5 } }, // 5: sa[1]'0
                { { 0, 9 }, { 48, 57 }, { 64, 88 }, { 89, 112 } }, // 6: sa[2]'0
                { { 57, 64 }, { 88, 89 } }, // 7: sa[2]'1
                { { 0, 13 }, { 48, 112 } }, // 8: sa[3]'0
                { { 0, 17 }, { 48, 88 }, { 112, 152 } }, // 9: sa[4]'0  // ???
                { { 0, 21 }, { 48, 88 }, { 112, 152 } }, // 10: sa[5]'0
                { { 0, 25 }, { 48, 88 } }, // 11: sa[6]'0
                { { 0, 5 } }, // 12: sa[7]'0
                { { 5, 9 } }, // 13: sa[7]'1
                { { 9, 13 } }, // 14: sa[7]'2
                { { 13, 17 } }, // 15: sa[7]'3
                { { 17, 21 } }, // 16: sa[7]'4
                { { 21, 25 } }, // 17: sa[7]'5
                { { 25, 26 } }  // 18: sa[7]'6
            },
            {   // for VGPRs
                { { 0, 29 }, { 48, 88 }, { 112, 152 } }, // 0: va[0]'0
                { { 0, 33 }, { 48, 88 }, { 112, 152 } }, // 1: va[1]'0
                { { 0, 37 }, { 48, 88 } }, // 2: va[2]'0
                { { 0, 4 }, { 48, 49 } }, // 3: va[3]'0
                { { 4, 41 }, { 49, 88 } }, // 4: va[3]'1
                { { 0, 29 } }, // 5: va[7]'0
                { { 29, 33 } }, // 6: va[7]'1
                { { 33, 37 } }, // 7: va[7]'2
                { { 37, 41 } }, // 8: va[7]'3
                { { 41, 42 } }  // 9: va[7]'4
            },
            { },
            { }
        },
        { }, // linearDepMaps
        {
            { 2, { {
                // SGPRs
                { 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11 },
                // VGPRs
                { 0, 1, 2, 3, 4 }, { }, { } } } },
            { 7, { { { 0, 1, 4, 6, 7, 8 }, { }, { }, { } } } },
            { 10, { {
                // SGPRs
                { 0, 1, 4, 9, 10 },
                // VGPRs
                { 0, 1 }, { }, { } } } }
        }, // vidxRoutineMap
        {
            { 0, { { { 5, 12 }, { 5 }, { }, { } } } },
            { 3, { { { 9, 10, 11 }, { 0, 1, 2, 4 }, { }, { } } } },
            { 5, { { { 6, 8, 11 }, { 2, 4 }, { }, { } } } }
        }, // vidxCallMap
        true, ""
    },
    {   // 22 - simple call, more complex routine (first unused return)
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        s_getpc_b64 s[2:3]              # 12
        s_add_u32 s2, s2, routine-.     # 16
        s_add_u32 s3, s3, routine-.+4   # 24
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 32
        
        s_lshl_b32 sa[2], sa[2], 3      # 36
        s_lshl_b32 sa[3], sa[3], sa[5]  # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 48
        s_cbranch_scc1 bb1              # 52
        
        s_min_u32 sa[2], sa[2], sa[4]   # 56
        .cf_ret
        s_setpc_b64 s[0:1]              # 60
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 64
        s_xor_b32 sa[3], sa[3], sa[4]   # 68
        .cf_ret
        s_setpc_b64 s[0:1]              # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 33, 36 }, { 48, 61 }, { 64, 73 } }, // 0: S0
                { { 33, 36 }, { 48, 61 }, { 64, 73 } }, // 1: S1
                { { 13, 33 } }, // 2: S2
                { { 13, 33 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 1, 36 }, { 48, 49 } }, // 6: sa[2]'0
                { { 49, 57 }, { 64, 65 } }, // 7: sa[2]'1
                { { 36, 37 }, { 57, 64 }, { 65, 76 } }, // 8: sa[2]'2
                { { 37, 38 } }, // 9: sa[2]'3
                { { 5, 41 }, { 48, 76 } }, // 10: sa[3]'0
                { { 41, 42 } }, // 11: sa[3]'1
                { { 0, 36 }, { 48, 57 }, { 64, 69 } }, // 12: sa[4]'0
                { { 9, 41 } }  // 13: sa[5]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 8, 10, 12 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 13 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 23 - simple nested call (checking filling unused path)
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 12
        
        s_lshl_b32 sa[2], sa[2], 3      # 16
        s_lshl_b32 sa[3], sa[3], sa[5]  # 20
        s_endpgm                        # 24
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 28
        s_cbranch_scc1 bb1              # 32
        
        s_min_u32 sa[2], sa[2], sa[4]   # 36
        .cf_ret
        s_setpc_b64 s[0:1]              # 40
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 44
        
        .cf_call routine2
        s_swappc_b64 s[4:5], s[6:7]     # 48
        
        .cf_ret
        s_setpc_b64 s[0:1]              # 52
        
routine2:
        s_xor_b32 sa[3], sa[3], sa[4]   # 56
        .cf_ret
        s_setpc_b64 s[4:5]              # 60
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 13, 16 }, { 28, 41 }, { 44, 53 } }, // 0: S0
                { { 13, 16 }, { 28, 41 }, { 44, 53 } }, // 1: S1
                { { 0, 13 } }, // 2: S2
                { { 0, 13 } }, // 3: S3
                { { 0, 1 }, { 49, 52 }, { 56, 61 } }, // 4: S4
                { { 0, 9 }, { 49, 52 }, { 56, 61 } }, // 5: S5
                { { 0, 16 }, { 28, 36 }, { 44, 49 } }, // 6: S6
                { { 0, 16 }, { 28, 36 }, { 44, 49 } }, // 7: S7
                { { 1, 16 }, { 28, 29 } }, // 8: sa[2]'0
                { { 29, 37 }, { 44, 45 } }, // 9: sa[2]'1
                { { 16, 17 }, { 37, 44 }, { 45, 56 } }, // 10: sa[2]'2
                { { 17, 18 } }, // 11: sa[2]'3
                { { 5, 21 }, { 28, 64 } }, // 12: sa[3]'0
                { { 21, 22 } }, // 13: sa[3]'1
                { { 0, 16 }, { 28, 37 }, { 44, 52 }, { 56, 57 } }, // 14: sa[4]'0
                { { 9, 21 } }  // 15: sa[5]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 4, 5, 6, 7, 8, 9, 10, 12, 14 }, { }, { }, { } } } },
            { 6, { { { 4, 5, 12, 14 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 15 }, { }, { }, { } } } },
            { 4, { { { 0, 1, 10 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 24 - simple call, more complex routine (sa[3] and sa[6] not used after call)
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        s_getpc_b64 s[2:3]              # 12
        s_add_u32 s2, s2, routine-.     # 16
        s_add_u32 s3, s3, routine-.+4   # 24
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 32
        
        s_lshl_b32 sa[2], sa[2], sa[5]  # 36
        s_nop 7                         # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 48
        s_xor_b32 sa[6], sa[6], sa[4]   # 52
        s_cbranch_scc1 bb1              # 56
        
        s_min_u32 sa[2], sa[2], sa[4]   # 60
        s_xor_b32 sa[3], sa[3], sa[4]   # 64
        .cf_ret
        s_setpc_b64 s[0:1]              # 68
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 72
        .cf_ret
        s_setpc_b64 s[0:1]              # 76
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 33, 36 }, { 48, 69 }, { 72, 77 } }, // 0: S0
                { { 33, 36 }, { 48, 69 }, { 72, 77 } }, // 1: S1
                { { 13, 33 } }, // 2: S2
                { { 13, 33 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 1, 36 }, { 48, 49 } }, // 6: sa[2]'0
                { { 49, 61 }, { 72, 73 } }, // 7: sa[2]'1
                { { 36, 37 }, { 61, 72 }, { 73, 80 } }, // 8: sa[2]'2
                { { 37, 38 } }, // 9: sa[2]'3
                { { 5, 36 }, { 48, 65 } }, // 10: sa[3]'0
                { { 65, 66 } }, // 11: sa[3]'1
                { { 0, 36 }, { 48, 65 }, { 72, 73 } }, // 12: sa[4]'0
                { { 9, 37 } },  // 13: sa[5]'0
                { { 0, 36 }, { 48, 53 } }, // 14: sa[6]'0
                { { 53, 54 } }  // 15: sa[6]'1
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 8, 10, 11, 12, 14, 15 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 13 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 25 - many routines and many calls. all vars in all return paths
        R"ffDXD(.regvar sa:s:8, va:v:8
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]         # 0
        
        s_add_u32 sa[7], sa[7], sa[1]       # 4
        s_add_u32 sa[7], sa[7], sa[2]       # 8
        s_add_u32 sa[7], sa[7], sa[3]       # 12
        s_add_u32 sa[7], sa[7], sa[4]       # 16
        v_xor_b32 va[7], va[7], va[1]       # 20
        v_xor_b32 va[7], va[7], va[2]       # 24
        v_xor_b32 va[7], va[7], va[3]       # 28
        s_endpgm                            # 32
        
routine:
        s_add_u32 sa[2], sa[2], sa[0]       # 36
        s_cbranch_scc0 r1_2                 # 40
r1_1:
        v_add_f32 va[2], va[2], va[0]       # 44
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]         # 48
        v_add_f32 va[1], va[1], va[0]       # 52
        s_add_u32 sa[1], sa[1], sa[0]       # 56
        v_add_f32 va[3], va[3], va[0]       # 60
        .cf_ret
        s_setpc_b64 s[0:1]                  # 64
r1_2:
        s_add_u32 sa[3], sa[3], sa[0]       # 68
        s_add_u32 sa[1], sa[1], sa[0]       # 72
        .cf_call routine3
        s_swappc_b64 s[0:1], s[2:3]         # 76
        s_add_u32 sa[4], sa[4], sa[0]       # 80
        .cf_ret
        s_setpc_b64 s[0:1]                  # 84
        
routine2:
        s_add_u32 sa[3], sa[3], sa[0]       # 88
        s_cbranch_scc0 r2_2                 # 92
r2_1:
        s_add_u32 sa[4], sa[4], sa[0]       # 96
        .cf_ret
        s_setpc_b64 s[0:1]                  # 100
r2_2:
        s_add_u32 sa[4], sa[4], sa[0]       # 104
        .cf_ret
        s_setpc_b64 s[0:1]                  # 108
        
routine3:
        v_add_f32 va[1], va[1], va[0]       # 112
        v_add_f32 va[3], va[3], va[0]       # 116
        s_cbranch_scc0 r3_2                 # 120
r3_1:
        v_add_f32 va[2], va[2], va[0]       # 124
        .cf_ret
        s_setpc_b64 s[0:1]                  # 128
r3_2:
        v_add_f32 va[2], va[2], va[0]       # 132
        s_cbranch_scc1 r3_3                 # 136
        .cf_ret
        s_setpc_b64 s[0:1]                  # 140
r3_3:
        v_add_f32 va[3], va[3], va[0]       # 144
        .cf_ret
        s_setpc_b64 s[0:1]                  # 148
)ffDXD",
        {   // livenesses
            {   // SGPRs
                { { 1, 2 }, { 49, 65 }, { 77, 85 }, { 88, 152 } }, // 0: S0
                { { 1, 2 }, { 49, 65 }, { 77, 85 }, { 88, 152 } }, // 1: S1
                { { 0, 4 }, { 36, 49 }, { 68, 77 } }, // 2: S2
                { { 0, 4 }, { 36, 49 }, { 68, 77 } }, // 3: S3
                { { 0, 4 }, { 36, 57 }, { 68, 81 }, { 88, 112 } }, // 4: sa[0]'0
                { { 0, 4 }, { 36, 57 }, { 68, 73 } }, // 5: sa[1]'0
                { { 4, 5 }, { 57, 68 }, { 73, 88 } }, // 6: sa[1]'1
                { { 0, 4 }, { 36, 37 } }, // 7: sa[2]'0
                { { 4, 9 }, { 37, 88 } }, // 8: sa[2]'1
                { { 0, 4 }, { 36, 52 }, { 68, 69 }, { 88, 89 } }, // 9: sa[3]'0
                { { 4, 13 }, { 52, 68 }, { 69, 88 }, { 89, 112 } }, // 10: sa[3]'1
                { { 0, 4 }, { 36, 52 }, { 68, 81 },
                        { 88, 97 }, { 104, 105 } }, // 11: sa[4]'0
                { { 4, 17 }, { 52, 68 }, { 81, 88 },
                        { 97, 104 }, { 105, 112 } }, // 12: sa[4]'1
                { { 0, 5 } }, // 13: sa[7]'0
                { { 5, 9 } }, // 14: sa[7]'1
                { { 9, 13 } }, // 15: sa[7]'2
                { { 13, 17 } }, // 16: sa[7]'3
                { { 17, 18 } }  // 17: sa[7]'4
            },
            {   // for VPGRs
                { { 0, 4 }, { 36, 61 }, { 68, 80 },
                        { 112, 125 }, { 132, 140 }, { 144, 145 } }, // 0: va[0]'0
                { { 0, 4 }, { 36, 53 }, { 68, 80 }, { 112, 113 } }, // 1: va[1]'0
                { { 4, 21 }, { 53, 68 }, { 80, 88 }, { 113, 152 } }, // 2: va[1]'1
                { { 0, 4 }, { 36, 45 }, { 68, 80 },
                        { 112, 125 }, { 132, 133 } }, // 3: va[2]'0
                { { 4, 25 }, { 45, 68 }, { 80, 88 },
                        { 125, 132 }, { 133, 152 } }, // 4: va[2]'1
                { { 0, 4 }, { 36, 61 }, { 68, 80 }, { 112, 117 } }, // 5: va[3]'0
                { { 4, 29 }, { 61, 68 }, { 80, 88 }, { 117, 152 } }, // 6: va[3]'1
                { { 0, 21 } }, // 7: va[7]'0
                { { 21, 25 } }, // 8: va[7]'1
                { { 25, 29 } }, // 9: va[7]'2
                { { 29, 30 } }  // 10: va[7]'3
            },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { {
                { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 }, // for SGPRs
                { 0, 1, 2, 3, 4, 5, 6 }, // for VGPRs
                { }, { } } } },
            { 7, { {
                { 0, 1, 4, 9, 10, 11, 12 }, // for SGPRs
                { }, { }, { } } } },
            { 10, { { { 0, 1 },  // for SGPRs
                { 0, 1, 2, 3, 4, 5, 6 }, // for VGPRs
                { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 13 }, { 7 }, { }, { } } } },
            { 3, { { { 5, 8 }, { 0, 1, 4, 5 }, { }, { } } } },
            { 5, { { { 4, 6, 8, 10, 11 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 26 - routine with loop
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4             # 0
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 4
        
        s_xor_b32 sa[2], sa[2], sa[0]   # 8
        s_endpgm                        # 12
        
routine:
        s_and_b32 sa[2], sa[2], sa[1]   # 16
loop0:
        s_cbranch_vccz b1               # 20
b0:     s_cbranch_scc0 loop0            # 24
        s_branch ret2                   # 28
        
b1:     s_and_b32 sa[2], sa[0], sa[1]   # 32
        s_cbranch_scc0 loop0            # 36
        .cf_ret
        s_setpc_b64 s[0:1]              # 40
ret2:
        .cf_ret
        s_setpc_b64 s[0:1]              # 44
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 5, 8 }, { 16, 41 }, { 44, 45 } }, // 0: S0
                { { 5, 8 }, { 16, 41 }, { 44, 45 } }, // 1: S1
                { { 0, 5 } }, // 2: S2
                { { 0, 5 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 }, { 16, 48 } }, // 5: sa[0]'0
                { { 0, 8 }, { 16, 28 }, { 32, 40 } }, // 6: sa[1]'0
                { { 1, 8 }, { 16, 17 } }, // 7: sa[2]'0
                { { 8, 9 }, { 17, 32 }, { 33, 48 } }, // 8: sa[2]'1
                { { 9, 10 } }  // 9: sa[2]'2
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 5, 6, 7, 8 }, { }, { }, { } } } }
        },
        { }, // vidxCallMap
        true, ""
    },
    {   // 27 - routine with loop (normal register instead regvar)
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 s6, s4                # 0
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 4
        
        s_xor_b32 s6, s6, sa[0]         # 8
        s_endpgm                        # 12
        
routine:
        s_and_b32 s6, s6, sa[1]         # 16
loop0:
        s_cbranch_vccz b1               # 20
b0:     s_cbranch_scc0 loop0            # 24
        s_branch ret2                   # 28
        
b1:     s_and_b32 s6, sa[0], sa[1]      # 32
        s_cbranch_scc0 loop0            # 36
        .cf_ret
        s_setpc_b64 s[0:1]              # 40
ret2:
        .cf_ret
        s_setpc_b64 s[0:1]              # 44
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 5, 8 }, { 16, 41 }, { 44, 45 } }, // 0: S0
                { { 5, 8 }, { 16, 41 }, { 44, 45 } }, // 1: S1
                { { 0, 5 } }, // 2: S2
                { { 0, 5 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 1, 10 }, { 16, 32 }, { 33, 48 } }, // 5: s6
                { { 0, 9 }, { 16, 48 } }, // 6: sa[0]'0
                { { 0, 8 }, { 16, 28 }, { 32, 40 } } // 7: sa[1]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 5, 6, 7 }, { }, { }, { } } } }
        },
        { }, // vidxCallMap
        true, ""
    },
    {   // 28 - two routines with var sharing (output-input)
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4             # 0
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 4
        
        s_sub_u32 sa[3], sa[2], sa[1]   # 8
        .cf_jump a0, a1, a2
        s_setpc_b64 s[2:3]              # 12
        
a0:     .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]     # 16
        s_xor_b32 sa[2], sa[2], sa[0]   # 20
        s_endpgm                        # 24

a1:     .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]     # 28
        s_xor_b32 sa[2], sa[2], sa[0]   # 32
        s_endpgm                        # 36
        
a2:     .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]     # 40
        s_xor_b32 sa[2], sa[2], sa[0]   # 44
        s_endpgm                        # 48
        
routine:
        s_and_b32 sa[2], sa[2], sa[1]   # 52
        .cf_ret
        s_setpc_b64 s[0:1]              # 56
        
routine2:
        s_cbranch_vccz r2_1             # 60
r2_0:   s_and_b32 sa[2], sa[2], sa[1]   # 64
        .cf_ret
        s_setpc_b64 s[0:1]              # 68
r2_1:   s_and_b32 sa[2], sa[2], sa[1]   # 72
        s_xor_b32 sa[3], sa[3], sa[1]   # 76
        .cf_ret
        s_setpc_b64 s[0:1]              # 80
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 5, 8 }, { 17, 20 }, { 29, 32 }, { 41, 44 },
                    { 52, 57 }, { 60, 69 }, { 72, 81 } }, // 0: S0
                { { 5, 8 }, { 17, 20 }, { 29, 32 }, { 41, 44 },
                    { 52, 57 }, { 60, 69 }, { 72, 81 } }, // 1: S1
                { { 0, 17 }, { 28, 29 }, { 40, 41 } }, // 2: S2
                { { 0, 17 }, { 28, 29 }, { 40, 41 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 21 }, { 28, 33 }, { 40, 45 } }, // 5: sa[0]'0
                { { 0, 20 }, { 28, 32 }, { 40, 44 },
                    { 52, 65 }, { 72, 77 } }, // 6: sa[1]'0
                { { 1, 8 }, { 52, 53 } }, // 7: sa[2]'0
                { { 8, 20 }, { 28, 32 }, { 40, 44 },
                    { 53, 65 }, { 72, 73 } }, // 8: sa[2]'1
                { { 20, 21 }, { 32, 33 }, { 44, 45 },
                    { 65, 72 }, { 73, 84 } }, // 9: sa[2]'2
                { { 21, 22 } }, // 10: sa[2]'3
                { { 33, 34 } }, // 11: sa[2]'4
                { { 45, 46 } }, // 12: sa[2]'5
                { { 9, 20 }, { 28, 32 }, { 40, 44 },
                    { 60, 64 }, { 72, 77 } }, // 13: sa[3]'0
                { { 77, 78 } }  // 14: sa[3]'1
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 8, { { { 0, 1, 6, 7, 8 }, { }, { }, { } } } },
            { 9, { { { 0, 1, 6, 8, 9, 13, 14 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 2, 3, 5 }, { }, { }, { } } } },
            { 2, { { { 5 }, { }, { }, { } } } },
            { 4, { { { 5 }, { }, { }, { } } } },
            { 6, { { { 5 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 29 - two routines with var sharing (output-input) 2
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4             # 0
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 4
        
        s_sub_u32 sa[3], sa[2], sa[1]   # 8
        .cf_jump a0, a1, a2
        s_setpc_b64 s[2:3]              # 12
        
a0:     s_branch mp                     # 16
        
a1:     s_branch mp                     # 20
        
a2:     s_branch mp                     # 24
        
mp:     .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]     # 28
        s_xor_b32 sa[2], sa[2], sa[0]   # 32
        s_endpgm                        # 36
        
routine:
        s_and_b32 sa[2], sa[2], sa[1]   # 40
        .cf_ret
        s_setpc_b64 s[0:1]              # 44
        
routine2:
        s_cbranch_vccz r2_1             # 48
r2_0:   s_and_b32 sa[2], sa[2], sa[1]   # 52
        .cf_ret
        s_setpc_b64 s[0:1]              # 56
r2_1:   s_and_b32 sa[2], sa[2], sa[1]   # 60
        s_xor_b32 sa[3], sa[3], sa[1]   # 64
        .cf_ret
        s_setpc_b64 s[0:1]              # 68
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 5, 8 }, { 29, 32 }, { 40, 45 }, { 48, 57 }, { 60, 69 } }, // 0: S0
                { { 5, 8 }, { 29, 32 }, { 40, 45 }, { 48, 57 }, { 60, 69 } }, // 1: S1
                { { 0, 29 } }, // 2: S2
                { { 0, 29 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 33 } }, // 5: sa[0]'0
                { { 0, 32 }, { 40, 53 }, { 60, 65 } }, // 6: sa[1]'0
                { { 1, 8 }, { 40, 41 } }, // 7: sa[2]'0
                { { 8, 32 }, { 41, 53 }, { 60, 61 } }, // 8: sa[2]'1
                { { 32, 33 }, { 53, 60 }, { 61, 72 } }, // 9: sa[2]'2
                { { 33, 34 } }, // 10: sa[2]'3
                { { 9, 32 }, { 48, 52 }, { 60, 65 } }, // 11: sa[3]'0
                { { 65, 66 } }  // 12: sa[3]'1
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 7, { { { 0, 1, 6, 7, 8 }, { }, { }, { } } } },
            { 8, { { { 0, 1, 6, 8, 9, 11, 12 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 2, 3, 5 }, { }, { }, { } } } },
            { 5, { { { 5 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 30 - two routines with var sharing (output-input) 3
        // (shared var in called routine3)
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4             # 0
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 4
        
        s_sub_u32 sa[3], sa[2], sa[1]   # 8
        .cf_jump a0, a1, a2
        s_setpc_b64 s[2:3]              # 12
        
a0:     s_branch mp                     # 16
        
a1:     s_branch mp                     # 20
        
a2:     s_branch mp                     # 24
        
mp:     .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]     # 28
        s_xor_b32 sa[2], sa[2], sa[0]   # 32
        s_endpgm                        # 36
        
routine:
        s_and_b32 sa[2], sa[2], sa[1]   # 40
        .cf_ret
        s_setpc_b64 s[0:1]              # 44
        
routine2:
        s_cbranch_vccz r2_1             # 48
r2_0:   .cf_call routine3
        s_swappc_b64 s[6:7], s[0:1]     # 52
        .cf_ret
        s_setpc_b64 s[0:1]              # 56
r2_1:   .cf_call routine3
        s_swappc_b64 s[6:7], s[0:1]     # 60
        s_xor_b32 sa[3], sa[3], sa[1]   # 64
        .cf_ret
        s_setpc_b64 s[0:1]              # 68
routine3:
        s_and_b32 sa[2], sa[2], sa[1]   # 72
        .cf_ret
        s_setpc_b64 s[0:1]              # 76
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 5, 8 }, { 29, 32 }, { 40, 45 },
                    { 48, 57 }, { 60, 69 }, { 72, 80 } }, // 0: S0
                { { 5, 8 }, { 29, 32 }, { 40, 45 },
                    { 48, 57 }, { 60, 69 }, { 72, 80 } }, // 1: S1
                { { 0, 29 } }, // 2: S2
                { { 0, 29 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 53, 54 }, { 61, 62 } }, // 5: S6
                { { 53, 54 }, { 61, 62 } }, // 6: S7
                { { 0, 33 } }, // 7: sa[0]'0
                { { 0, 32 }, { 40, 56 }, { 60, 65 }, { 72, 80 } }, // 8: sa[1]'0
                { { 1, 8 }, { 40, 41 } }, // 9: sa[2]'0
                { { 8, 32 }, { 41, 56 }, { 60, 64 }, { 72, 73 } }, // 10: sa[2]'1
                { { 32, 33 }, { 56, 60 }, { 64, 72 }, { 73, 80 } }, // 11: sa[2]'2
                { { 33, 34 } }, // 12: sa[2]'3
                { { 9, 32 }, { 48, 52 }, { 60, 65 } }, // 13: sa[3]'0
                { { 65, 66 } }  // 14: sa[3]'1
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 7, { { { 0, 1, 8, 9, 10 }, { }, { }, { } } } },
            { 8, { { { 0, 1, 8, 10, 11, 13, 14 }, { }, { }, { } } } },
            { 13, { { { 0, 1, 8, 10, 11 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 2, 3, 7 }, { }, { }, { } } } },
            { 5, { { { 7 }, { }, { }, { } } } },
            { 11, { { { 13 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 31 - routine with ends (s_endpgm, no return) 1
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        s_getpc_b64 s[2:3]              # 12
        s_add_u32 s2, s2, routine-.     # 16
        s_add_u32 s3, s3, routine-.+4   # 24
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 32
        
        s_lshl_b32 sa[2], sa[2], 3      # 36
        s_lshl_b32 sa[3], sa[3], sa[5]  # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 48
        s_cbranch_scc1 bb1              # 52
        
        s_min_u32 sa[2], sa[2], sa[4]   # 56
        s_xor_b32 sa[3], sa[3], sa[4]   # 60
        s_endpgm                        # 64
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 68
        .cf_ret
        s_setpc_b64 s[0:1]              # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 33, 36 }, { 48, 56 }, { 68, 73 } }, // 0: S0
                { { 33, 36 }, { 48, 56 }, { 68, 73 } }, // 1: S1
                { { 13, 33 } }, // 2: S2
                { { 13, 33 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 1, 36 }, { 48, 49 } }, // 6: sa[2]'0
                { { 49, 57 }, { 68, 69 } }, // 7: sa[2]'1
                { { 57, 58 } }, // 8: sa[2]'2
                { { 36, 37 }, { 69, 76 } }, // 9: sa[2]'3
                { { 37, 38 } }, // 10: sa[2]'4
                { { 5, 41 }, { 48, 61 }, { 68, 76 } }, // 11: sa[3]'0
                { { 61, 62 } }, // 12: sa[3]'1
                { { 41, 42 } }, // 13: sa[3]'2
                { { 0, 36 }, { 48, 61 }, { 68, 69 } }, // 14: sa[4]'0
                { { 9, 41 } }  // 15: sa[5]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 8, 9, 11, 12, 14 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 15 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 32 - routine with ends (s_endpgm, no return) 2
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        s_getpc_b64 s[2:3]              # 12
        s_add_u32 s2, s2, routine-.     # 16
        s_add_u32 s3, s3, routine-.+4   # 24
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 32
        
        s_lshl_b32 sa[2], sa[2], 3      # 36
        s_lshl_b32 sa[3], sa[3], sa[5]  # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 48
        s_cbranch_scc1 bb1              # 52
        
        s_min_u32 sa[2], sa[2], sa[4]   # 56
        s_xor_b32 sa[3], sa[3], sa[4]   # 60
        .cf_ret
        s_setpc_b64 s[0:1]              # 64
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 68
        s_endpgm                        # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 33, 36 }, { 48, 65 } }, // 0: S0
                { { 33, 36 }, { 48, 65 } }, // 1: S1
                { { 13, 33 } }, // 2: S2
                { { 13, 33 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 1, 36 }, { 48, 49 } }, // 6: sa[2]'0
                { { 49, 57 }, { 68, 69 } }, // 7: sa[2]'1
                { { 36, 37 }, { 57, 68 } }, // 8: sa[2]'2
                { { 69, 70 } }, // 9: sa[2]'3
                { { 37, 38 } }, // 10: sa[2]'4
                { { 5, 36 }, { 48, 61 } }, // 11: sa[3]'0
                { { 36, 41 }, { 61, 68 } }, // 12: sa[3]'1
                { { 41, 42 } }, // 13: sa[3]'2
                { { 0, 36 }, { 48, 61 }, { 68, 69 } }, // 14: sa[4]'0
                { { 9, 41 } }  // 15: sa[5]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 8, 9, 11, 12, 14 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 15 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 33 - routine with ends (s_endpgm, no return) 3
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        s_getpc_b64 s[2:3]              # 12
        s_add_u32 s2, s2, routine-.     # 16
        s_add_u32 s3, s3, routine-.+4   # 24
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 32
        
        s_lshl_b32 sa[2], sa[2], 3      # 36
        s_lshl_b32 sa[3], sa[3], sa[5]  # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 48
        s_cbranch_scc1 bb1              # 52
        
        s_min_u32 sa[2], sa[2], sa[4]   # 56
        s_endpgm                        # 60
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 64
        s_xor_b32 sa[3], sa[3], sa[4]   # 68
        .cf_ret
        s_setpc_b64 s[0:1]              # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 33, 36 }, { 48, 56 }, { 64, 73 } }, // 0: S0
                { { 33, 36 }, { 48, 56 }, { 64, 73 } }, // 1: S1
                { { 13, 33 } }, // 2: S2
                { { 13, 33 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 1, 36 }, { 48, 49 } }, // 6: sa[2]'0
                { { 49, 57 }, { 64, 65 } }, // 7: sa[2]'1
                { { 57, 58 } }, // 8: sa[2]'2
                { { 36, 37 }, { 65, 76 } }, // 9: sa[2]'3
                { { 37, 38 } }, // 10: sa[2]'4
                { { 5, 36 }, { 48, 56 }, { 64, 69 } }, // 11: sa[3]'0
                { { 36, 41 }, { 69, 76 } }, // 12: sa[3]'1
                { { 41, 42 } }, // 13: sa[3]'2
                { { 0, 36 }, { 48, 57 }, { 64, 69 } }, // 14: sa[4]'0
                { { 9, 41 } }  // 15: sa[5]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 8, 9, 11, 12, 14 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 15 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 34 - routine with ends (s_endpgm, no return) 4
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[5], s5             # 8
        
        s_getpc_b64 s[2:3]              # 12
        s_add_u32 s2, s2, routine-.     # 16
        s_add_u32 s3, s3, routine-.+4   # 24
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 32
        
        s_lshl_b32 sa[2], sa[2], 3      # 36
        s_lshl_b32 sa[3], sa[3], sa[5]  # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 48
        s_cbranch_scc1 bb1              # 52
        
        s_min_u32 sa[2], sa[2], sa[4]   # 56
        .cf_ret
        s_setpc_b64 s[0:1]              # 60
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]   # 64
        s_xor_b32 sa[3], sa[3], sa[4]   # 68
        s_endpgm                        # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 33, 36 }, { 48, 61 } }, // 0: S0
                { { 33, 36 }, { 48, 61 } }, // 1: S1
                { { 13, 33 } }, // 2: S2
                { { 13, 33 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 1, 36 }, { 48, 49 } }, // 6: sa[2]'0
                { { 49, 57 }, { 64, 65 } }, // 7: sa[2]'1
                { { 36, 37 }, { 57, 64 } }, // 8: sa[2]'2
                { { 65, 66 } }, // 9: sa[2]'3
                { { 37, 38 } }, // 10: sa[2]'4
                { { 5, 41 }, { 48, 69 } }, // 11: sa[3]'0
                { { 69, 70 } }, // 12: sa[3]'1
                { { 41, 42 } }, // 13: sa[3]'2
                { { 0, 36 }, { 48, 57 }, { 64, 69 } }, // 14: sa[4]'0
                { { 9, 41 } }  // 15: sa[5]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 6, 7, 8, 9, 11, 12, 14 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 15 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 35 - routine with loop
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4             # 0
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 4
        
        s_xor_b32 sa[2], sa[2], sa[0]   # 8
        s_endpgm                        # 12
        
routine:
        s_and_b32 sa[2], sa[2], sa[1]   # 16
loop0:
        s_cbranch_vccz b1               # 20
b0:     s_cbranch_scc0 loop0            # 24
        s_branch ret2                   # 28
        
b1:     s_xor_b32 sa[2], sa[2], sa[0]   # 32
        s_xor_b32 sa[2], sa[2], sa[0]   # 36
        s_and_b32 sa[2], sa[2], sa[1]   # 40
        s_and_b32 sa[2], sa[2], sa[1]   # 44
        s_cbranch_scc0 loop0            # 48
        .cf_ret
        s_setpc_b64 s[0:1]              # 52
ret2:
        .cf_ret
        s_setpc_b64 s[0:1]              # 56
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 5, 8 }, { 16, 53 }, { 56, 57 } }, // 0: S0
                { { 5, 8 }, { 16, 53 }, { 56, 57 } }, // 1: S1
                { { 0, 5 } }, // 2: S2
                { { 0, 5 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 9 }, { 16, 60 } }, // 5: sa[0]'0
                { { 0, 8 }, { 16, 28 }, { 32, 52 } }, // 6: sa[1]'0
                { { 1, 8 }, { 16, 17 } }, // 7: sa[2]'0
                { { 8, 9 }, { 17, 33 }, { 45, 60 } }, // 8: sa[2]'1
                { { 33, 37 } }, // 9: sa[2]'2
                { { 37, 41 } }, // 10: sa[2]'3
                { { 41, 45 } }, // 11: sa[2]'4
                { { 9, 10 } }  // 12: sa[2]'5
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 5, 6, 7, 8, 9, 10, 11 }, { }, { }, { } } } }
        },
        { }, // vidxCallMap
        true, ""
    },
    {   // 36 - two routines with common code
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        s_mov_b32 sa[4], s6             # 8
        s_mov_b32 sa[5], s7             # 12
        s_mov_b32 sa[6], s8             # 16
        s_mov_b32 xa[0], s9             # 20
        s_mov_b32 xa[1], s10            # 24
        
        .cf_call routine1
        s_swappc_b64 s[0:1], s[2:3]     # 28
        
        s_xor_b32 sa[5], sa[5], sa[0]   # 32
        s_xor_b32 sa[2], sa[2], sa[0]   # 36
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]     # 40
        
        s_xor_b32 sa[5], sa[5], sa[0]   # 44
        s_xor_b32 sa[2], sa[2], sa[0]   # 48
        s_xor_b32 sa[6], sa[6], sa[0]   # 52
        s_xor_b32 xa[0], xa[0], sa[0]   # 56
        s_xor_b32 xa[1], xa[1], sa[0]   # 60
        s_endpgm                        # 64
        
routine1:
        s_add_u32 sa[2], sa[2], sa[0]   # 68
        s_add_u32 sa[3], sa[3], sa[1]   # 72
        
        s_add_u32 sa[4], sa[3], sa[0]   # 76
        s_add_u32 sa[5], sa[3], sa[5]   # 80
        s_add_u32 sa[6], sa[6], sa[1]   # 84
        
        s_xor_b32 xa[1], xa[1], sa[0]   # 88
        
common:
        s_add_u32 sa[5], sa[5], sa[0]   # 92
        s_add_u32 sa[3], sa[3], sa[1]   # 96
        s_xor_b32 sa[4], sa[4], sa[2]   # 100
        .cf_ret
        s_setpc_b64 s[0:1]              # 104

routine2:
        s_add_u32 sa[2], sa[2], sa[0]   # 108
        s_add_u32 sa[3], sa[3], sa[1]   # 112
        
        s_add_u32 sa[4], sa[1], sa[2]   # 116
        s_add_u32 sa[5], sa[3], sa[5]   # 120
        
        s_xor_b32 xa[0], xa[0], sa[0]   # 124
        s_branch common                 # 128
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 29, 32 }, { 41, 44 }, { 68, 105 }, { 108, 132 } }, // 0: S0
                { { 29, 32 }, { 41, 44 }, { 68, 105 }, { 108, 132 } }, // 1: S1
                { { 0, 41 } }, // 2: S2
                { { 0, 41 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 5 } }, // 5: S5
                { { 0, 9 } }, // 6: S6
                { { 0, 13 } }, // 7: S7
                { { 0, 17 } }, // 8: S8
                { { 0, 21 } }, // 9: S9
                { { 0, 25 } }, // 10: S10
                { { 0, 61 }, { 68, 132 } }, // 11: sa[0]'0
                { { 0, 44 }, { 68, 132 } }, // 12: sa[1]'0
                { { 1, 32 }, { 68, 69 } }, // 13: sa[2]'0
                { { 32, 37 }, { 44, 49 }, { 69, 108 }, { 109, 132 } }, // 14: sa[2]'1
                { { 37, 44 }, { 108, 109 } }, // 15: sa[2]'2
                { { 49, 50 } }, // 16: sa[2]'3
                { { 5, 32 }, { 68, 73 } }, // 17: sa[3]'0
                { { 73, 97 }, { 113, 132 } }, // 18: sa[3]'1
                { { 32, 44 }, { 97, 113 } }, // 19: sa[3]'2
                { { 9, 10 } }, // 20: sa[4]'0
                { { 77, 102 }, { 117, 132 } }, // 21: sa[4]'1
                { { 13, 32 }, { 68, 81 } }, // 22: sa[5]'0
                { { 81, 93 }, { 121, 132 } }, // 23: sa[5]'1
                { { 32, 33 }, { 44, 45 }, { 93, 108 } }, // 24: sa[5]'2
                { { 33, 44 }, { 108, 121 } }, // 25: sa[5]'3
                { { 45, 46 } }, // 26: sa[5]'4
                { { 17, 32 }, { 68, 85 } }, // 27: sa[6]'0
                { { 32, 53 }, { 85, 108 } }, // 28: sa[6]'1
                { { 53, 54 } }, // 29: sa[6]'2
                { { 21, 44 }, { 108, 125 } }, // 30: xa[0]'0
                { { 44, 57 }, { 92, 108 }, { 125, 132 } }, // 31: xa[0]'1
                { { 57, 58 } }, // 32: xa[0]'2
                { { 25, 32 }, { 68, 89 } }, // 33: xa[1]'0
                { { 32, 61 }, { 89, 108 } }, // 34: xa[1]'1
                { { 61, 62 } }  // 35: xa[1]'2
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 3, { {
                { 0, 1, 11, 12, 13, 14, 17, 18, 19, 21, 22, 23, 24, 27, 28, 33, 34 },
                { }, { }, { } } } },
            { 5, { { { 0, 1, 11, 12, 14, 15, 18, 19, 21, 23, 24, 25, 30, 31 },
                { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 2, 3, 30 }, { }, { }, { } } } },
            { 1, { { { 28, 34 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 37 - many routines in calls
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s5             # 4
        
        .cf_call routine, routine2
        s_swappc_b64 s[0:1], s[2:3]     # 8
        
        s_lshl_b32 sa[2], sa[2], 3      # 12
        s_lshl_b32 sa[3], sa[3], 4      # 16
        
        .cf_call routine2, routine3
        s_swappc_b64 s[0:1], s[2:3]     # 20
        
        s_lshr_b32 sa[2], sa[2], 3      # 24
        s_lshr_b32 sa[3], sa[3], 4      # 28
        s_endpgm                        # 32
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]   # 36
        s_xor_b32 sa[3], sa[3], sa[4]   # 40
        .cf_ret
        s_setpc_b64 s[0:1]              # 44
        
routine2:
        s_xor_b32 sa[2], sa[2], sa[4]   # 48
        s_xor_b32 sa[3], sa[3], sa[4]   # 52
        s_xor_b32 sa[3], sa[3], sa[1]   # 56
        .cf_ret
        s_setpc_b64 s[0:1]              # 60
        
routine3:
        s_xor_b32 sa[2], sa[2], sa[4]   # 64
        s_xor_b32 sa[3], sa[2], sa[0]   # 68
        
        .cf_call routine2, routine4
        s_swappc_b64 s[0:1], s[2:3]     # 72
        
        s_xor_b32 sa[2], sa[2], sa[1]   # 76
        s_xor_b32 sa[3], sa[3], sa[0]   # 80
        .cf_ret
        s_setpc_b64 s[0:1]              # 84
        
routine4:
        s_xor_b32 sa[3], sa[3], sa[4]   # 88
        s_xor_b32 sa[2], sa[3], sa[6]   # 92
        .cf_ret
        s_setpc_b64 s[0:1]              # 96
)ffDXD",
        {
            {   // for SGPRs
                { { 9, 12 }, { 21, 24 }, { 36, 45 },
                    { 48, 64 }, { 73, 85 }, { 88, 100 } }, // 0: S0
                { { 9, 12 }, { 21, 24 }, { 36, 45 },
                    { 48, 64 }, { 73, 85 }, { 88, 100 } }, // 1: S1
                { { 0, 24 }, { 64, 73 } }, // 2: S2
                { { 0, 24 }, { 64, 73 } }, // 3: S3
                { { 0, 1 } }, // 4: S4
                { { 0, 5 } }, // 5: S5
                { { 0, 24 }, { 64, 81 } }, // 6: sa[0]'0
                { { 0, 24 }, { 48, 77 } }, // 7: sa[1]'0
                { { 1, 12 }, { 13, 24 }, { 36, 37 },
                    { 48, 49 }, { 64, 76 } }, // 8: sa[2]'0
                { { 12, 13 }, { 24, 25 }, { 37, 48 },
                    { 49, 64 }, { 76, 88 }, { 93, 100 } }, // 9: sa[2]'1
                { { 25, 26 } }, // 10: sa[2]'2
                { { 5, 12 }, { 17, 24 }, { 36, 41 },
                    { 48, 53 }, { 69, 76 }, { 88, 89 } }, // 11: sa[3]'0
                { { 12, 17 }, { 24, 29 }, { 41, 48 },
                    { 57, 64 }, { 76, 88 }, { 89, 100 } }, // 12: sa[3]'1
                { { 53, 57 } }, // 13: sa[3]'2
                { { 29, 30 } }, // 14: sa[3]'3
                { { 0, 24 }, { 36, 76 }, { 88, 89 } }, // 15: sa[4]'0
                { { 0, 24 }, { 64, 76 }, { 88, 93 } }  // 16: sa[6]'0
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 3, { { { 0, 1, 8, 9, 11, 12, 15 }, { }, { }, { } } } },
            { 4, { { { 0, 1, 7, 8, 9, 11, 12, 13, 15 }, { }, { }, { } } } },
            { 5, { { { 0, 1, 2, 3, 6, 7, 8, 9, 11, 12, 13, 15, 16 }, { }, { }, { } } } },
            { 7, { { { 0, 1, 9, 11, 12, 15, 16 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 2, 3, 6, 7, 16 }, { }, { }, { } } } },
            { 5, { { { 6, 7 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 38 - simple joins (same normal registers)
        R"ffDXD(.regvar sa:s:12, va:v:8
        s_mov_b32 s2, s12       # 0
        .cf_jump a1,a2,a3
        s_setpc_b64 s[10:11]    # 4
        
a1:     s_add_u32 s2, s2, s3    # 8
        s_add_u32 s2, s2, s3    # 12
        s_branch end            # 16
a2:     s_add_u32 s2, s2, s4    # 20
        s_add_u32 s2, s2, s4    # 24
        s_branch end            # 28
a3:     s_add_u32 s2, s2, s5    # 32
        s_add_u32 s2, s2, s5    # 36
        s_branch end            # 40

end:    s_xor_b32 s2, s2, s13   # 44
        s_endpgm                # 48
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 1, 46 } }, // 0: S2
                { { 0, 13 } }, // 0: S3
                { { 0, 8 }, { 20, 25 } }, // 0: S4
                { { 0, 8 }, { 32, 37 } }, // 0: S5
                { { 0, 5 } }, // 0: S10
                { { 0, 5 } }, // 0: S11
                { { 0, 1 } }, // 0: S12
                { { 0, 45 } } // 0: S13
            },
            { },
            { },
            { }
        },
        { },  // linearDepMaps
        { },  // vidxRoutineMap
        { },  // vidxCallMap
        true, ""
    },
    {   // 39 - simple call, more complex routine (same normal registers)
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 s2, s14               # 0
        s_mov_b32 s3, s15               # 4
        s_mov_b32 s5, s15               # 8
        
        s_getpc_b64 s[12:13]            # 12
        s_add_u32 s12, s12, routine-.   # 16
        s_add_u32 s13, s13, routine-.+4 # 24
        .cf_call routine
        s_swappc_b64 s[10:11], s[12:13] # 32
        
        s_lshl_b32 s2, s2, 3            # 36
        s_lshl_b32 s3, s3, s5           # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 s2, s2, s4            # 48
        s_cbranch_scc1 bb1              # 52
        
        s_min_u32 s2, s2, s4            # 56
        s_xor_b32 s3, s3, s4            # 60
        .cf_ret
        s_setpc_b64 s[10:11]            # 64
        
bb1:    s_and_b32 s2, s2, s4            # 68
        .cf_ret
        s_setpc_b64 s[10:11]            # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 1, 38 }, { 48, 76 } }, // 0: S2
                { { 5, 42 }, { 48, 76 } }, // 1: S3
                { { 0, 36 }, { 48, 61 }, { 68, 69 } }, // 2: S4
                { { 9, 41 } }, // 3: S5
                { { 33, 36 }, { 48, 65 }, { 68, 73 } }, // 4: S10
                { { 33, 36 }, { 48, 65 }, { 68, 73 } }, // 5: S11
                { { 13, 33 } }, // 6: S12
                { { 13, 33 } }, // 7: S13
                { { 0, 1 } }, // 8: S14
                { { 0, 9 } }  // 9: S15
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 2, 4, 5 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 0, { { { 3 }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 40 - first recursion testcase
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s4             # 4
        s_mov_b32 sa[6], s7             # 8
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 12
        
        s_add_u32 sa[2], sa[2], sa[0]   # 16
        s_add_u32 sa[3], sa[3], sa[0]   # 20
        s_add_u32 sa[6], sa[6], sa[0]   # 24
        s_endpgm                        # 28
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]   # 32
        s_xor_b32 sa[3], sa[3], sa[1]   # 36
        s_cbranch_vccnz b0              # 40
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 44
        
        s_xor_b32 sa[3], sa[3], sa[1]   # 48
        s_xor_b32 sa[6], sa[6], sa[1]   # 52
        .cf_ret
        s_setpc_b64 s[0:1]              # 56
        
b0:     s_xor_b32 sa[3], sa[3], sa[0]   # 60
        s_xor_b32 sa[2], sa[2], sa[0]   # 64
        s_xor_b32 sa[6], sa[6], sa[0]   # 68
        .cf_ret
        s_setpc_b64 s[0:1]              # 72
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 13, 16 }, { 32, 44 }, { 45, 76 } }, // 0: S0
                { { 13, 16 }, { 32, 44 }, { 45, 76 } }, // 1: S1
                { { 0, 16 }, { 32, 48 } }, // 2: S2
                { { 0, 16 }, { 32, 48 } }, // 3: S3
                { { 0, 5 } }, // 4: S4
                { { 0, 9 } }, // 5: S7
                { { 0, 25 }, { 32, 76 } }, // 6: sa[0]'0
                { { 0, 16 }, { 32, 76 } }, // 7: sa[1]'0
                { { 1, 17 }, { 32, 76 } }, // 8: sa[2]'0
                { { 17, 18 } }, // 9: sa[2]'1
                { { 5, 21 }, { 32, 76 } }, // 10: sa[3]'0
                { { 21, 22 } }, // 11: sa[3]'1
                { { 9, 25 }, { 32, 76 } }, // 12: sa[6]'0
                { { 25, 26 } }  // 13: sa[6]'1
            }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { { { 0, 1, 2, 3, 6, 7, 8, 10, 12 }, { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 3, { { { }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 41 - second recursion testcase
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4             # 0
        s_mov_b32 sa[3], s4             # 4
        s_mov_b32 sa[4], s5             # 8
        s_mov_b32 sa[5], s6             # 12
        s_mov_b32 sa[6], s7             # 16
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 20
        
        s_add_u32 sa[2], sa[2], sa[0]   # 24
        s_add_u32 sa[3], sa[3], sa[0]   # 28
        s_add_u32 sa[4], sa[4], sa[1]   # 32
        s_add_u32 sa[5], sa[5], sa[1]   # 36
        s_add_u32 sa[6], sa[6], sa[1]   # 40
        s_endpgm                        # 44
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]   # 48
        s_xor_b32 sa[3], sa[3], sa[1]   # 52
        s_cbranch_vccnz b0              # 56
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]     # 60
        
        s_xor_b32 sa[3], sa[3], sa[1]   # 64
        s_xor_b32 sa[6], sa[6], sa[1]   # 68
        s_xor_b32 sa[5], sa[5], sa[0]   # 72
        .cf_ret
        s_setpc_b64 s[0:1]              # 76
        
b0:     s_xor_b32 sa[3], sa[3], sa[0]   # 80
        s_xor_b32 sa[2], sa[2], sa[0]   # 84
        s_xor_b32 sa[6], sa[6], sa[0]   # 88
        .cf_ret
        s_setpc_b64 s[0:1]              # 92
        
routine2:
        s_xor_b32 sa[2], sa[2], sa[0]   # 96
        s_xor_b32 sa[3], sa[3], sa[1]   # 100
        s_cbranch_vccnz b1              # 104
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]     # 108
        
        s_xor_b32 sa[3], sa[3], sa[1]   # 112
        s_xor_b32 sa[6], sa[6], sa[1]   # 116
        s_xor_b32 sa[4], sa[4], sa[0]   # 120
        .cf_ret
        s_setpc_b64 s[0:1]              # 124
        
b1:     s_xor_b32 sa[3], sa[3], sa[0]   # 128
        s_xor_b32 sa[2], sa[2], sa[0]   # 132
        s_xor_b32 sa[6], sa[6], sa[0]   # 136
        .cf_ret
        s_setpc_b64 s[0:1]              # 140
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 21, 24 }, { 48, 60 }, { 61, 108 }, { 109, 144 } }, // 0: S0
                { { 21, 24 }, { 48, 60 }, { 61, 108 }, { 109, 144 } }, // 1: S1
                { { 0, 24 }, { 48, 64 }, { 96, 112 } }, // 2: S2
                { { 0, 24 }, { 48, 64 }, { 96, 112 } }, // 3: S3
                { { 0, 5 } }, // 4: S4
                { { 0, 9 } }, // 5: S5
                { { 0, 13 } }, // 6: s6
                { { 0, 17 } }, // 7: S7
                { { 0, 29 }, { 48, 144 } }, // 8: sa[0]'0
                { { 0, 41 }, { 48, 144 } }, // 9: sa[1]'0
                //{ { 1, 25 }, { 48, 96 }, { 97, 144 } }, // 10: sa[2]'0
                { { 1, 25 }, { 48, 49 }, { 64, 80 },
                    { 85, 96 }, { 97, 144 } }, // 10: sa[2]'1
                { { 49, 64 }, { 80, 85 }, { 96, 97 } }, // 11: sa[2]'2
                { { 25, 26 } }, // 12: sa[2]'3
                { { 5, 29 }, { 48, 53 }, { 65, 80 },
                    { 81, 96 }, { 101, 113 }, { 128, 129 } }, // 13: sa[3]'0
                { { 53, 65 }, { 80, 81 }, { 96, 101 },
                    { 113, 128 }, { 129, 144 } }, // 14: sa[3]'1
                { { 29, 30 } }, // 15: sa[3]'2
                { { 9, 33 }, { 48, 144 } }, // 16: sa[4]'0
                { { 33, 34 } }, // 17: sa[4]'1
                { { 13, 37 }, { 48, 144 } }, // 18: sa[5]'0
                { { 37, 38 } }, // 19: sa[5]'1
                { { 17, 41 }, { 48, 144 } }, // 20: sa[6]'0
                { { 41, 42 } }  // 21: sa[6]'1
            },
            { },
            { },
            { }
        },
        { }, // linearDepMaps
        {   // vidxRoutineMap
            { 2, { {
                { 0, 1, 2, 3, 8, 9, 10, 11, 13, 14, 16, 18, 20 },
                { }, { }, { } } } },
            { 6, { {
                { 0, 1, 2, 3, 8, 9, 10, 11, 13, 14, 16, 18, 20 },
                { }, { }, { } } } }
        },
        {   // vidxCallMap
            { 3, { { { 18 }, { }, { }, { } } } }, // ?
            { 7, { { { }, { }, { }, { } } } }
        },
        true, ""
    },
    {   // 42 - testing linear deps
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b64 sa[2:3], s[4:5]               # 0
        s_mov_b64 sa[5:6], s[6:7]               # 4
        s_cbranch_scc0 b1                       # 8
        
b0:     s_xor_b32 sa[2], sa[2], sa[7]           # 12
        s_xor_b32 sa[3], sa[3], sa[7]           # 16
        s_xor_b64 sa[2:3], sa[2:3], sa[0:1]     # 20
        s_xor_b32 sa[2], sa[2], sa[7]           # 24
        s_xor_b32 sa[3], sa[3], sa[7]           # 28
        s_xor_b64 sa[2:3], sa[5:6], sa[0:1]     # 32
        s_xor_b64 sa[5:6], sa[2:3], sa[0:1]     # 36
        s_endpgm                                # 40
        
b1:     s_xor_b32 sa[2], sa[2], sa[7]           # 44
        s_xor_b32 sa[3], sa[3], sa[7]           # 48
        s_xor_b64 sa[2:3], sa[2:3], sa[0:1]     # 52
        s_xor_b64 sa[2:3], sa[5:6], sa[0:1]     # 56
        s_xor_b64 sa[5:6], sa[2:3], sa[0:1]     # 60
        s_xor_b32 sa[5], sa[5], sa[7]           # 64
        s_xor_b32 sa[6], sa[6], sa[7]           # 68
        s_xor_b64 sa[5:6], sa[2:3], sa[0:1]     # 72
        s_endpgm                                # 76
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 1 } }, // 0: S4
                { { 0, 1 } }, // 1: S5
                { { 0, 5 } }, // 2: S6
                { { 0, 5 } }, // 3: S7
                { { 0, 37 }, { 44, 73 } }, // 4: sa[0]'0
                { { 0, 37 }, { 44, 73 } }, // 5: sa[1]'0
                { { 1, 13 }, { 44, 45 } }, // 6: sa[2]'0
                { { 13, 21 } }, // 7: sa[2]'1
                { { 21, 25 } }, // 8: sa[2]'2
                { { 25, 26 } }, // 9: sa[2]'3
                { { 33, 37 } }, // 10: sa[2]'4
                { { 45, 53 } }, // 11: sa[2]'5
                { { 53, 54 } }, // 12: sa[2]'6
                { { 57, 73 } }, // 13: sa[2]'7
                { { 1, 17 }, { 44, 49 } }, // 14: sa[3]'0
                { { 17, 21 },}, // 15: sa[3]'1
                { { 21, 29 } }, // 16: sa[3]'2
                { { 29, 30 } }, // 17: sa[3]'3
                { { 33, 37 } }, // 18: sa[3]'4
                { { 49, 53 } }, // 19: sa[3]'5
                { { 53, 54 } }, // 20: sa[3]'6
                { { 57, 73 } }, // 21: sa[3]'7
                { { 5, 33 }, { 44, 57 } }, // 22: sa[5]'0
                { { 37, 38 } }, // 23: sa[5]'1
                { { 61, 65 } }, // 24: sa[5]'2
                { { 65, 66 } }, // 25: sa[5]'3
                { { 73, 74 } }, // 26: sa[5]'4
                { { 5, 33 }, { 44, 57 } }, // 27: sa[6]'0
                { { 37, 38 } }, // 28: sa[6]'1
                { { 61, 69 } }, // 29: sa[6]'2
                { { 69, 70 } }, // 30: sa[6]'3
                { { 73, 74 } }, // 31: sa[6]'4
                { { 0, 29 }, { 44, 69 } }  // 32: sa[7]'0
            }
        },
        {   // linearDeps
            {   // for SGPRs
                { 4, { 2, { }, { 5 } } },
                { 5, { 0, { 4 }, { } } },
                { 6, { 2, { }, { 14 } } },
                { 7, { 2, { }, { 15 } } },
                { 8, { 2, { }, { 16 } } },
                { 10, { 2, { }, { 18 } } },
                { 11, { 2, { }, { 19 } } },
                { 12, { 2, { }, { 20 } } },
                { 13, { 2, { }, { 21 } } },
                { 14, { 0, { 6 }, { } } },
                { 15, { 0, { 7 }, { } } },
                { 16, { 0, { 8 }, { } } },
                { 18, { 0, { 10 }, { } } },
                { 19, { 0, { 11 }, { } } },
                { 20, { 0, { 12 }, { } } },
                { 21, { 0, { 13 }, { } } },
                { 22, { 2, { }, { 27 } } },
                { 23, { 2, { }, { 28 } } },
                { 24, { 2, { }, { 29 } } },
                { 26, { 2, { }, { 31 } } },
                { 27, { 0, { 22 }, { } } },
                { 28, { 0, { 23 }, { } } },
                { 29, { 0, { 24 }, { } } },
                { 31, { 0, { 26 }, { } } }
            },
            { },
            { },
            { }
        },
        { }, // vidxRoutineMap
        { }, // vidxCallMap
        true, ""
    },
    {   // 43 - user define linear deps (by '.rvlin')
        R"ffDXD(.regvar sa:s:8, va:v:8
        .rvlin_once sa[2:7]
        .usereg sa[2:7]:r
        s_mov_b32 sa[2], s4     # 0
        s_cbranch_scc0 b1       # 4
b0:     .rvlin_once sa[2:7]
        .usereg sa[2:7]:r
        s_mov_b32 s1, s1        # 8
        s_endpgm                # 12
b1:     .rvlin_once va[3:6]
        .usereg va[3:6]:w
        s_mov_b32 s1, s1        # 16
        s_endpgm                # 20
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 10 }, { 16, 18 } }, // 0: S1
                { { 0, 1 } }, // 1: S4
                { { 0, 1 } }, // 2: sa[2]'0
                { { 1, 9 } }, // 3: sa[2]'1
                { { 0, 9 } }, // 4: sa[3]'0
                { { 0, 9 } }, // 5: sa[4]'0
                { { 0, 9 } }, // 5: sa[5]'0
                { { 0, 9 } }, // 6: sa[6]'0
                { { 0, 9 } }  // 7: sa[7]'0
            },
            {   // for VGPRs
                { { 0, 8 }, { 16, 17 } }, // 0: sa[3]'0
                { { 0, 8 }, { 16, 17 } }, // 0: sa[4]'0
                { { 0, 8 }, { 16, 17 } }, // 0: sa[5]'0
                { { 0, 8 }, { 16, 17 } }  // 0: sa[6]'0
            },
            { },
            { }
        },
        {   // linearDepMaps
            {   // for SGPRs
                { 2, { 0, { }, { 4 } } },
                { 3, { 0, { }, { 4 } } },
                { 4, { 0, { 3, 2 }, { 5 } } },
                { 5, { 0, { 4 }, { 6 } } },
                { 6, { 0, { 5 }, { 7 } } },
                { 7, { 0, { 6 }, { 8 } } },
                { 8, { 0, { 7 }, { } } }
            },
            {   // for VGPRs
                { 0, { 0, { }, { 1 } } },
                { 1, { 0, { 0 }, { 2 } } },
                { 2, { 0, { 1 }, { 3 } } },
                { 3, { 0, { 2 }, { } } }
            },
            { },
            { }
        },
        { }, // vidxRoutineMap
        { }, // vidxCallMap
        true, ""
    },
    {   // 44 - simple case (first rvu in next instruction)
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_nop 4
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[4], s3
        v_xor_b32 va[4], va[2], v3
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 9 } }, // S3
                { { 0, 5 } }, // sa[2]'0
                { { 5, 9 } }, // sa[4]'0
                { { 9, 10 } }  // sa[4]'1
            },
            {   // for VGPRs
                { { 0, 13 } }, // V3
                { { 0, 13 } }, // va[2]'0
                { { 13, 14 } } // va[4]'0 : out of range code block
            },
            { },
            { }
        },
        { },  // linearDepMaps
        { },  // vidxRoutineMap
        { },  // vidxCallMap
        true, ""
    },
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

static void checkVIdxSetEntries(const std::string& testCaseName, const char* vvarSetName,
        const Array<std::pair<size_t, VIdxSetEntry2> >& expVIdxRoutineMap,
        const std::unordered_map<size_t,VIdxSetEntry>& vidxRoutineMap,
        const std::vector<size_t>* revLvIndexCvtTables)
{
    assertValue("testAsmLivenesses", testCaseName + vvarSetName + ".size",
            expVIdxRoutineMap.size(), vidxRoutineMap.size());
    
    for (size_t j = 0; j < vidxRoutineMap.size(); j++)
    {
        std::ostringstream vOss;
        vOss << vvarSetName << "#" << j;
        vOss.flush();
        std::string vcname(vOss.str());
        
        auto vcit = vidxRoutineMap.find(expVIdxRoutineMap[j].first);
        std::ostringstream kOss;
        kOss << expVIdxRoutineMap[j].first;
        kOss.flush();
        assertTrue("testAsmLivenesses", testCaseName + vcname +".key=" + kOss.str(),
                    vcit != vidxRoutineMap.end());
        
        const Array<size_t>* expEntry = expVIdxRoutineMap[j].second.vs;
        const VIdxSetEntry& resEntry = vcit->second;
        for (cxuint r = 0; r < MAX_REGTYPES_NUM; r++)
        {
            std::ostringstream vsOss;
            vsOss << ".vs#" << r;
            vsOss.flush();
            std::string vsname = vcname + vsOss.str();
            
            assertValue("testAsmLivenesses", testCaseName + vsname +".size",
                    expEntry[r].size(), resEntry.vs[r].size());
            
            std::vector<size_t> resVVars;
            std::transform(resEntry.vs[r].begin(), resEntry.vs[r].end(),
                    std::back_inserter(resVVars),
                    [&r,&revLvIndexCvtTables](size_t v)
                    { return revLvIndexCvtTables[r][v]; });
            std::sort(resVVars.begin(), resVVars.end());
            assertArray("testAsmLivenesses", testCaseName + vsname,
                        expEntry[r], resVVars);
        }
    }
}

static void testCreateLivenessesCase(cxuint i, const AsmLivenessesCase& testCase)
{
    std::cout << "-----------------------------------------------\n"
    "           Test " << i << "\n"
                "------------------------------------------------\n";
    
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
    regAlloc.createSSAData(*section.usageHandler, *section.linearDepHandler);
    regAlloc.applySSAReplaces();
    regAlloc.createLivenesses(*section.usageHandler, *section.linearDepHandler);
    
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
    
    // checking vidxRoutineMap
    checkVIdxSetEntries(testCaseName, "vidxRoutineMap", testCase.vidxRoutineMap,
                regAlloc.getVIdxRoutineMap(), revLvIndexCvtTables);
    
    // checking vidxCallMap
    checkVIdxSetEntries(testCaseName, "vidxCallMap", testCase.vidxCallMap,
                regAlloc.getVIdxCallMap(), revLvIndexCvtTables);
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
