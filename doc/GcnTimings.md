## GCN ISA Instruction Timings

### Preliminary explanations

Almost all instructions (scalar and vector) are executed within 4 cycles. Hence, to
achieve maximum performance, 4 wavefronts should be executed per compute unit.

NOTE: a simple single dword (4-byte) instruction is executed in 4 cycles (thanks to fast
dispatching from cache). However, a 2 dword (8-byte) instruction may require 4 extra cycles
for execution due to bigger size in memory and limits of instruction dispatching.
To achieve best performance, we recommend to use single dword instructions.

A DPFACTOR term is present in some tables; it indicates that the number of cycles depends
on the model of the GPU as follows:

 DPFACTOR     | DP speed | GPU subfamily
--------------|----------|----------------------------
 1            | 1/2      | professional Hawaii
 2            | 1/4      | Highend Tahiti: Radeon HD7970
 4            | 1/8      | Highend Hawaii: R9 290
 8            | 1/16     | Other GPU's
 

### Occupancy table

Waves | SGPRs | VGPRs | LdsW/I | Issue
------|-------|-------|--------|---------
1     | 128   | 256   | 64     | 1
2     | 128   | 128   | 32     | 2
3     | 128   | 84    | 21     | 3
4     | 128   | 64    | 16     | 4
5     | 96    | 48    | 12     | 5
6     | 80    | 40    | 10     | 5
7     | 72    | 36    | 9      | 5
8     | 64    | 32    | 8      | 5
9     | 56    | 28    | 7      | 5
10    | 48    | 24    | 6      | 5

Waves - number of concurrent waves that can be computed by a single SIMD unit
SGPRs - number of maximum SGPRs that can be allocated at that occupancy
VPGRs - number of maximum VGPRs that can be allocated at that occupancy
LdsW/I - maximum amount of LDS space per vector lane per wavefront in dwords
Issue - maximum number of instructions per clock

Each compute unit is partitioned into four SIMD units. So, the maximum number of waves per
compute unit is 40.

### Instruction alignment

Aligmnent Rules for 2-dword instructions (GCN 1.0/1.1):

* any penalty costs 4 cycles
* program divided by in 32-byte blocks
* only the first 3 dwords in the 32-byte block incur no penalty. Any 2-dword
instruction outside these first 3 dwords adds a single penalty.
* if the instructions is longer (more than four cycles) then the last cycles/4 dwords are free
* if 16 or more cycle 2-dword instruction and 2-dword instruction in 4 dword, then there is
no penalty for the second 2-dword instruction.
* best place to jump is the 5 first dwords in the 32-byte block. Jump to rest of the dwords causes
1-3 penalties, depending on number of dwords (N-4, where N is the dword number). This rule
does not apply to backward jumps (???)
* any conditional jump instruction should be in first half of the 32-byte block, otherwise
1-4 penalties are added if jump is not taken, depending on dword number (N-3, where N is dword number).

IMPORTANT: If the occupancy is greater than 1 wave per compute unit, then the penalties,
branches, and scalar instructions will be masked while executing
more waves than 4\*CUs. For best results is recommended to execute many waves
(multiple of 4\*CUs) with occupancy greater than 1.

The GCN 1.2 always execute instruction with full speed if these are in instruction cache.
GCN 1.2 can fetch double dword instructions in full speed.

### Instruction scheduling

* if many wavefronts are executed in a single CU (if many wavefronts) then scalar, vector and
data-share, memory (???) execution units can run independently in parallel,
achieving many instructions per cycles.
* between any integer V_ADD\*, V_SUB\*, V_FIRSTREADLINE_B32, V_READLANE_B32 operations
and any scalar ALU instructions there is 16-cycle delay. Masked if there are more waves than 4*CUs.
* any conditional jump that directly checks VCCZ or EXECZ after an instruction that changes
VCC or EXEC adds a single penalty (4 cycles). Masked if there are more waves than 4*CUs.
* any conditional jump that directly checks SCC after an instruction that changes SCC,
EXEC, VCC adds a single penalty (4 cycles). Masked if there are more waves than 4*CUs.

### SOP2 Instruction timings

All SOP2 instructions (S_CBRANCH_G_FORK not checked) take 4 cycles.

### SOPK Instruction timings

All SOPK instructions (S_CBRANCH_I_FORK  not checked) take 4 cycles.
S_SETREG_B32 and S_SETREG_IMM32_B32 take 8 cycles.

### SOP1 Instruction timings

The S_*_SAVEEXEC_B64 instructions take 8 cycles. Other ALU instructions (except
S_MOV_REGRD_B32, S_CBRANCH_JOIN, S_RFE_B64) take 4 cycles.

### SOPC Instruction timings

All comparison and bit checking instructions take 4 cycles.

### SOPP Instruction timings

Jumps cost 4 cycle (no jump) or 20 cycles (???) if jump is performed.

### SMRD Instruction timings

Timings of SMRD instructions includes only time to fetch and execute instruction without
loading data from memory. Timings of SMRD instructions are in this table:

 Instruction           | Cycles        | Instruction           | Cycles
-----------------------|---------------|-----------------------|---------------
 S_BUFFER_LOAD_DWORD   | 4             | S_LOAD_DWORD          | 4
 S_BUFFER_LOAD_DWORDX2 | 4             | S_LOAD_DWORDX2        | 4
 S_BUFFER_LOAD_DWORDX4 | 4             | S_LOAD_DWORDX4        | 4
 S_BUFFER_LOAD_DWORDX8 | 8             | S_LOAD_DWORDX8        | 8
 S_BUFFER_LOAD_DWORDX16 | 16-24        | S_LOAD_DWORDX16       | 16-24
 S_DCACHE_INV          | 4             | S_MEMTIME             | 4
 S_DCACHE_INV_VOL      | 4             |

### VOP2 Instruction timings

All VOP2 instructions take 4 cycles. All instruction can achieve throughput 1 instruction
per cycle.

### VOP1 Instruction timings

Maximum throughput of these instructions can be calculated by using the expression
`(1/(CYCLES/4))` - for 4 cycles it is 1 instruction per cycle, for 8 cycles it is 1/2 instruction
per cycle, etc.
Timings of VOP1 instructions are in this table:

 Instruction           | Cycles        | Instruction           | Cycles
-----------------------|---------------|-----------------------|---------------
 V_BFREV_B32           | 4             | V_FREXP_EXP_I32_F64   | DPFACTOR*4
 V_CEIL_F16            | 4             | V_FREXP_MANT_F16      | 4
 V_CEIL_F32            | 4             | V_FREXP_MANT_F32      | 4
 V_CEIL_F64            | DPFACTOR*4    | V_FREXP_MANT_F64      | DPFACTOR*4
 V_CLREXCP             | 4             | V_LOG_CLAMP_F32       | 16
 V_COS_F16             | 16            | V_LOG_F16             | 16
 V_COS_F32             | 16            | V_LOG_F32             | 16
 V_CVT_F16_F32         | 4             | V_LOG_LEGACY_F32      | 16
 V_CVT_F16_I16         | 4             | V_MBCNT_LO_U32_B32    | 4
 V_CVT_F16_U16         | 4             | V_MBCNT_HI_U32_B32    | 4
 V_CVT_F32_F16         | 4             | V_MOVRELD_B32         | 4
 V_CVT_F32_F64         | DPFACTOR*4    | V_MOVRELSD_B32        | 4
 V_CVT_F32_I32         | 4             | V_MOVRELS_B32         | 4
 V_CVT_F32_U32         | 4             | V_MOV_B32             | 4
 V_CVT_F32_UBYTE0      | 4             | V_MOV_FED_B32         | 4
 V_CVT_F32_UBYTE1      | 4             | V_MOV_PRSV_B32        | 4
 V_CVT_F32_UBYTE2      | 4             | V_NOP                 | 4
 V_CVT_F32_UBYTE3      | 4             | V_NOT_B32             | 4
 V_CVT_F64_F32         | DPFACTOR*4    | V_RCP_CLAMP_F32       | 16
 V_CVT_F64_I32         | DPFACTOR*4    | V_RCP_CLAMP_F64       | DPFACTOR*8
 V_CVT_F64_U32         | DPFACTOR*4    | V_RCP_F16             | 16
 V_CVT_FLR_I32_F32     | 4             | V_RCP_F32             | 16
 V_CVT_I16_F16         | 4             | V_RCP_F64             | DPFACTOR*8
 V_CVT_I32_F32         | 4             | V_RCP_IFLAG_F32       | 16
 V_CVT_I32_F64         | DPFACTOR*4    | V_RCP_LEGACY_F32      | 16
 V_CVT_NORM_I16_F16    | 4             | V_READFIRSTLANE_B32   | 4
 V_CVT_NORM_U16_F16    | 4             | V_RNDNE_F16           | 4
 V_CVT_OFF_F32_I4      | 4             | V_RNDNE_F32           | 4
 V_CVT_RPI_I32_F32     | 4             | V_RNDNE_F64           | DPFACTOR*4
 V_CVT_U16_F16         | 4             | V_RSQ_CLAMP_F32       | 16
 V_CVT_U32_F32         | 4             | V_RSQ_CLAMP_F64       | DPFACTOR*8
 V_CVT_U32_F64         | DPFACTOR*4    | V_RSQ_F16             | 16
 V_EXP_F16             | 16            | V_RSQ_F32             | 16
 V_EXP_F32             | 16            | V_RSQ_F64             | DPFACTOR*8
 V_EXP_LEGACY_F32      | 16            | V_RSQ_LEGACY_F32      | 16
 V_FFBH_I32            | 4             | V_SAT_PK_U8_I16       | 4
 V_FFBH_U32            | 4             | V_SCREEN_PARTITION_4SE_B32 | 4
 V_FFBL_B32            | 4             | V_SIN_F16             | 16
 V_FLOOR_F16           | 4             | V_SIN_F32             | 16
 V_FLOOR_F32           | 4             | V_SQRT_F16            | 16 
 V_FLOOR_F64           | DPFACTOR*4    | V_SQRT_F32            | 16
 V_FRACT_F16           | 4             | V_SQRT_F64            | DPFACTOR*8
 V_FRACT_F32           | 4             | V_SWAP_B32            | 8
 V_FRACT_F64           | DPFACTOR*4    | V_TRUNC_F16           | 4
 V_FREXP_EXP_I16_F16   | 4             | V_TRUNC_F32           | 4
 V_FREXP_EXP_I32_F32   | 4             | V_TRUNC_F64           | DPFACTOR*4

### VOPC Instruction timings

Maximum throughput of these instructions can be calculated by using expression
`(1/(CYCLES/4))` - for 4 cycles it is 1 instruction per cycle, for 8 cycles it is 1/2
instruction per cycle, etc.
All 16-bit and 32-bit comparison instructions take 4 cycles.
All 64-bit comparison instructions take DPFACTOR*4 cycles.

### VOP3 Instruction timings

Maximum throughput of these instructions can be calculated by using expression
`(1/(CYCLES/4))` - for 4 cycles it is 1 instruction per cycle, for 8 cycles it is 1/2
instruction per cycle and etc.

Timings of VOP3 instructions are in this table:

 Instruction           | Cycles        | Instruction           | Cycles
-----------------------|---------------|-----------------------|---------------
 V_ADD3_U32            | 4             |  V_MAD_LEGACY_U16      | 4
 V_ADD_F64             | DPFACTOR*4    |  V_MAD_U16             | 4
 V_ADD_LSHL_U32        | 4             |  V_MAD_U32_U16         | 4
 V_ALIGNBIT_B32        | 4             |  V_MAD_U32_U24         | 4
 V_ALIGNBYTE_B32       | 4             |  V_MAD_U64_U32         | 16
 V_AND_OR_B32          | 4             |  V_MAX3_F16            | 4
 V_ASHR_I64            | DPFACTOR*4    |  V_MAX3_F32            | 4
 V_ASHRREV_I64         | DPFACTOR*4    |  V_MAX3_I16            | 4
 V_BFE_I32             | 4             |  V_MAX3_I32            | 4
 V_BFE_U32             | 4             |  V_MAX3_U16            | 4
 V_BFI_B32             | 4             |  V_MAX3_U32            | 4
 V_CUBEID_F32          | 4             |  V_MAX_F64             | DPFACTOR*4
 V_CUBEMA_F32          | 4             |  V_MED3_F16            | 4
 V_CUBESC_F32          | 4             |  V_MED3_F32            | 4
 V_CUBETC_F32          | 4             |  V_MED3_I16            | 4
 V_CVT_PK_U8_F32       | 4             |  V_MED3_I32            | 4
 V_DIV_FIXUP_F16       | 4             |  V_MED3_U16            | 4
 V_DIV_FIXUP_F32       | 16            |  V_MED3_U32            | 4
 V_DIV_FIXUP_F64       | DPFACTOR*4    |  V_MIN3_F16            | 4
 V_DIV_FMAS_F32        | 16            |  V_MIN3_F32            | 4
 V_DIV_FMAS_F64        | DPFACTOR*8    |  V_MIN3_I16            | 4
 V_DIV_SCALE_F32       | 16            |  V_MIN3_I32            | 4
 V_DIV_SCALE_F64       | DPFACTOR*4    |  V_MIN3_U16            | 4
 V_MAD_F16             | 4             |  V_MIN3_U32            | 4
 V_FMA_F32             | 4 or 16 (1)   |  V_MIN_F64             | DPFACTOR*4
 V_FMA_F64             | DPFACTOR*8    |  V_MQSAD_PK_U16_U8     | 16
 V_FMA_LEGACY_F16      | 4             |  V_MQSAD_U32_U8        | 16
 V_LDEXP_F64           | DPFACTOR*4    |  V_MQSAD_U8            | 16
 V_LERP_U8             | 4             |  V_MSAD_U8             | 4
 V_LSHL_ADD_U32        | 4             |  V_MULLIT_F32          | 4
 V_LSHL_B64            | DPFACTOR*4    |  V_MUL_F64             | DPFACTOR*8
 V_LSHL_OR_B32         | 4             |  V_MUL_HI_I32          | 16
 V_LSHLREV_B64         | DPFACTOR*4    |  V_MUL_HI_U32          | 16
 V_LSHR_B64            | DPFACTOR*4    |  V_MUL_LO_I32          | 16
 V_LSHRREV_B64         | DPFACTOR*4    |  V_MUL_LO_U32          | 16
 V_MAD_F16             | 4             |  V_OR3_B32             | 4
 V_MAD_F32             | 4             |  V_QSAD_PK_U16_U8      | 16
 V_MAD_I16             | 4             |  V_QSAD_U8             | 16
 V_MAD_I32_I16         | 4             |  V_SAD_HI_U8           | 4
 V_MAD_I32_I24         | 4             |  V_SAD_U16             | 4
 V_MAD_I64_I32         | 16            |  V_SAD_U32             | 4
 V_MAD_LEGACY_F16      | 4             |  V_SAD_U8              | 4
 V_MAD_LEGACY_F32      | 4             |  V_TRIG_PREOP_F64      | DPFACTOR*8
 V_MAD_LEGACY_I16      | 4             |  V_XAD_U32             | 4

(1) - for device with DP speed 1/2, 1/4 or 1/8 is 4 cycles, for other devices is 16 cycles

### VOP3P Instruction timings

All VOP3P instructions take 4 cycles. All instruction can achieve throughput 1 instruction
per cycle.

### DS Instruction timings

Timings of DS instructions includes only execution without waiting for completing
LDS/GDS memory access on a single wavefront. Throughput indicates maximal possible
throughput that excludes any other delays and penalties.
Timings of DS instructions are in this table:

 Instruction            | Cycles | Throughput
------------------------|--------|------------
 DS_ADD_RTN_U32         | 8      | 1/4
 DS_ADD_RTN_U64         | 12     | 1/6
 DS_ADD_SRC2_U32        | 4      | 1/4
 DS_ADD_SRC2_U64        | 8      | 1/8
 DS_ADD_U32             | 8      | 1/4
 DS_ADD_U64             | 12     | 1/6
 DS_AND_B32             | 8      | 1/4
 DS_AND_B64             | 12     | 1/6
 DS_AND_RTN_B32         | 8      | 1/4
 DS_AND_RTN_B64         | 12     | 1/6
 DS_AND_SRC2_B32        | 4      | 1/4
 DS_AND_SRC2_B64        | 8      | 1/8
 DS_APPEND              | 4      | ?
 DS_CMPST_B32           | 12     | 1/6
 DS_CMPST_B64           | 20     | 1/10
 DS_CMPST_F32           | 12     | 1/6
 DS_CMPST_F64           | 20     | 1/10
 DS_CMPST_RTN_B32       | 12     | 1/6
 DS_CMPST_RTN_B64       | 20     | 1/10
 DS_CMPST_RTN_F32       | 12     | 1/6
 DS_CMPST_RTN_F64       | 20     | 1/10
 DS_CONDXCHG32_RTN_B128 | ?      | ?
 DS_CONDXCHG32_RTN_B64  | ?      | ?
 DS_CONSUME             | 4      | ?
 DS_DEC_RTN_U32         | 8      | 1/4
 DS_DEC_RTN_U64         | 12     | 1/6
 DS_DEC_SRC2_U32        | 4      | 1/4
 DS_DEC_SRC2_U64        | 8      | 1/8
 DS_DEC_U32             | 8      | 1/4
 DS_DEC_U64             | 12     | 1/6
 DS_GWS_BARRIER         | ?      | ?
 DS_GWS_INIT            | ?      | ?
 DS_GWS_SEMA_BR         | ?      | ?
 DS_GWS_SEMA_P          | ?      | ?
 DS_GWS_SEMA_RELEASE_ALL| ?      | ?
 DS_GWS_SEMA_V          | ?      | ?
 DS_INC_RTN_U32         | 8      | 1/4
 DS_INC_RTN_U64         | 12     | 1/6
 DS_INC_SRC2_U32        | 4      | 1/4
 DS_INC_SRC2_U64        | 8      | 1/8
 DS_INC_U32             | 8      | 1/4
 DS_INC_U64             | 12     | 1/6
 DS_MAX_F32             | 8      | 1/4
 DS_MAX_F64             | 12     | 1/6
 DS_MAX_I32             | 8      | 1/4
 DS_MAX_I64             | 12     | 1/6
 DS_MAX_RTN_F32         | 8      | 1/4
 DS_MAX_RTN_F64         | 12     | 1/6
 DS_MAX_RTN_I32         | 8      | 1/4
 DS_MAX_RTN_I64         | 12     | 1/6
 DS_MAX_RTN_U32         | 8      | 1/4
 DS_MAX_RTN_U64         | 12     | 1/6
 DS_MAX_SRC2_F32        | 4      | 1/4
 DS_MAX_SRC2_F64        | 8      | 1/8
 DS_MAX_SRC2_I32        | 4      | 1/4
 DS_MAX_SRC2_I64        | 8      | 1/8
 DS_MAX_SRC2_U32        | 4      | 1/4
 DS_MAX_SRC2_U64        | 8      | 1/8
 DS_MAX_U32             | 8      | 1/4
 DS_MAX_U64             | 12     | 1/6
 DS_MIN_F32             | 8      | 1/4
 DS_MIN_F64             | 12     | 1/6
 DS_MIN_I32             | 8      | 1/4
 DS_MIN_I64             | 12     | 1/6
 DS_MIN_RTN_F32         | 8      | 1/4
 DS_MIN_RTN_F64         | 12     | 1/6
 DS_MIN_RTN_I32         | 8      | 1/4
 DS_MIN_RTN_I64         | 12     | 1/6
 DS_MIN_RTN_U32         | 8      | 1/4
 DS_MIN_RTN_U64         | 12     | 1/6
 DS_MIN_SRC2_F32        | 4      | 1/4
 DS_MIN_SRC2_F64        | 8      | 1/8
 DS_MIN_SRC2_I32        | 4      | 1/4
 DS_MIN_SRC2_I64        | 8      | 1/8
 DS_MIN_SRC2_U32        | 4      | 1/4
 DS_MIN_SRC2_U64        | 8      | 1/8
 DS_MIN_U32             | 8      | 1/4
 DS_MIN_U64             | 12     | 1/6
 DS_MSKOR_B32           | 12     | 1/6
 DS_MSKOR_B64           | 20     | 1/10
 DS_MSKOR_RTN_B32       | 12     | 1/6
 DS_MSKOR_RTN_B64       | 20     | 1/10
 DS_NOP                 | 4      | ?
 DS_ORDERED_COUNT (???) | ?      | ?
 DS_OR_B32              | 8      | 1/4
 DS_OR_B64              | 12     | 1/6
 DS_OR_RTN_B32          | 8      | 1/4
 DS_OR_RTN_B64          | 12     | 1/6
 DS_OR_SRC2_B32         | 4      | 1/4
 DS_OR_SRC2_B64         | 8      | 1/8
 DS_READ2ST64_B32       | 8      | 1/4
 DS_READ2ST64_B64       | 16     | 1/8
 DS_READ2_B32           | 8      | 1/4
 DS_READ2_B64           | 16     | 1/8
 DS_READ_B128           | 16     | 1/8
 DS_READ_B32            | 4      | 1/2
 DS_READ_B64            | 8      | 1/4
 DS_READ_B96            | 16     | 1/8
 DS_READ_I16            | 4      | 1/2
 DS_READ_I8             | 4      | 1/2
 DS_READ_U16            | 4      | 1/2
 DS_READ_U8             | 4      | 1/2
 DS_RSUB_RTN_U32        | 8      | 1/4
 DS_RSUB_RTN_U64        | 12     | 1/6
 DS_RSUB_SRC2_U32       | 4      | 1/4
 DS_RSUB_SRC2_U64       | 8      | 1/8
 DS_RSUB_U32            | 8      | 1/4
 DS_RSUB_U64            | 12     | 1/6
 DS_SUB_RTN_U32         | 8      | 1/4
 DS_SUB_RTN_U64         | 12     | 1/6
 DS_SUB_SRC2_U32        | 4      | 1/4
 DS_SUB_SRC2_U64        | 8      | 1/8
 DS_SUB_U32             | 8      | 1/4
 DS_SUB_U64             | 12     | 1/6
 DS_SWIZZLE_B32         | 4      | 1/2
 DS_WRAP_RTN_B32        | ?      | ?
 DS_WRITE2ST64_B32      | 12     | 1/6
 DS_WRITE2ST64_B64      | 20     | 1/10
 DS_WRITE2_B32          | 12     | 1/6
 DS_WRITE2_B64          | 20     | 1/10
 DS_WRITE_B128          | 20     | 1/10
 DS_WRITE_B16           | 8      | 1/4
 DS_WRITE_B32           | 8      | 1/4
 DS_WRITE_B64           | 12     | 1/8
 DS_WRITE_B8            | 8      | 1/4
 DS_WRITE_B96           | 16     | 1/10
 DS_WRITE_SRC2_B32      | 12     | 1/4
 DS_WRITE_SRC2_B64      | 20     | 1/8
 DS_WRXCHG2ST64_RTN_B32 | 12     | 1/6
 DS_WRXCHG2ST64_RTN_B64 | 20     | 1/12
 DS_WRXCHG2_RTN_B32     | 12     | 1/6
 DS_WRXCHG2_RTN_B64     | 20     | 1/12
 DS_WRXCHG_RTN_B32      | 8      | 1/4
 DS_WRXCHG_RTN_B64      | 12     | 1/6
 DS_XOR_B32             | 8      | 1/4
 DS_XOR_B64             | 12     | 1/6
 DS_XOR_RTN_B32         | 8      | 1/4
 DS_XOR_RTN_B64         | 12     | 1/6
 DS_XOR_SRC2_B32        | 4      | 1/4
 DS_XOR_SRC2_B64        | 8      | 1/8

About bank conflicts: The LDS memory is partitioned in 32 banks. The bank number is in
bits 2-6 of the address. A bank conflict occurs when two addresses hit the same
bank, but the addresses are different starting from the 7bit
(the first 2 bits of the address doesn't matter).
Any bank conflict adds penalty to timing and throughput. In the worst case, the throughput
can be not greater 1/32 requests per cycle.
 
### MUBUF Instruction timings

Timings of MUBUF instructions includes only execution without waiting for completing
main memory access on a single wavefront. Additional GLCX adds X cycles to instruction
if the instruction uses the GLC modifier. Timings of MUBUF instructions are in this table:

 Instruction                | Cycles
----------------------------|-----------
 BUFFER_ATOMIC_ADD          | 16+GLC1
 BUFFER_ATOMIC_ADD_X2       | 16+GLC2
 BUFFER_ATOMIC_AND          | 16+GLC1
 BUFFER_ATOMIC_AND_X2       | 16
 BUFFER_ATOMIC_CMPSWAP      | 32
 BUFFER_ATOMIC_CMPSWAP_X2   | 32
 BUFFER_ATOMIC_DEC          | 16+GLC1
 BUFFER_ATOMIC_DEC_X2       | 16+GLC2
 BUFFER_ATOMIC_FCMPSWAP     | 32
 BUFFER_ATOMIC_FCMPSWAP_X2  | 32
 BUFFER_ATOMIC_FMAX         | 16+GLC1
 BUFFER_ATOMIC_FMAX_X2      | 16+GLC2
 BUFFER_ATOMIC_FMIN         | 16+GLC1
 BUFFER_ATOMIC_FMIN_X2      | 16+GLC2
 BUFFER_ATOMIC_INC          | 16+GLC1
 BUFFER_ATOMIC_INC_X2       | 16+GLC2
 BUFFER_ATOMIC_OR           | 16+GLC1
 BUFFER_ATOMIC_OR_X2        | 16+GLC2
 BUFFER_ATOMIC_RSUB         | 16+GLC1
 BUFFER_ATOMIC_RSUB_X2      | 16+GLC2
 BUFFER_ATOMIC_SMAX         | 16+GLC1
 BUFFER_ATOMIC_SMAX_X2      | 16+GLC2
 BUFFER_ATOMIC_SMIN         | 16+GLC1
 BUFFER_ATOMIC_SMIN_X2      | 16+GLC2
 BUFFER_ATOMIC_SUB          | 16+GLC1
 BUFFER_ATOMIC_SUB_X2       | 16+GLC2
 BUFFER_ATOMIC_SWAP         | 16+GLC1
 BUFFER_ATOMIC_SWAP_X2      | 16+GLC2
 BUFFER_ATOMIC_UMAX         | 16+GLC1
 BUFFER_ATOMIC_UMAX_X2      | 16+GLC2
 BUFFER_ATOMIC_UMIN         | 16+GLC1
 BUFFER_ATOMIC_UMIN_X2      | 16+GLC2
 BUFFER_ATOMIC_XOR          | 16+GLC1
 BUFFER_ATOMIC_XOR_X2       | 16+GLC2
 BUFFER_LOAD_DWORD          | 8
 BUFFER_LOAD_DWORDX2        | 18
 BUFFER_LOAD_DWORDX3        | 16
 BUFFER_LOAD_DWORDX4        | 16
 BUFFER_LOAD_FORMAT_X       | 8
 BUFFER_LOAD_FORMAT_XY      | 18?
 BUFFER_LOAD_FORMAT_XYZ     | 16
 BUFFER_LOAD_FORMAT_XYZW    | 16
 BUFFER_LOAD_SBYTE          | 8
 BUFFER_LOAD_SSHORT         | 8
 BUFFER_LOAD_UBYTE          | 8
 BUFFER_LOAD_USHORT         | 8
 BUFFER_STORE_BYTE          | 16
 BUFFER_STORE_DWORD         | 16
 BUFFER_STORE_DWORDX2       | 16
 BUFFER_STORE_DWORDX3       | 16
 BUFFER_STORE_DWORDX4       | 16
 BUFFER_STORE_FORMAT_X      | 16
 BUFFER_STORE_FORMAT_XY     | 16
 BUFFER_STORE_FORMAT_XYZ    | 16
 BUFFER_STORE_FORMAT_XYZW   | 16
 BUFFER_STORE_SHORT         | 16
 BUFFER_WBINVL1             | ?
 BUFFER_WBINVL1_SC          | ?
