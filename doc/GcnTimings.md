## GCN ISA Instruction Timings

### Preliminary explanations

The almost instructions are executed within 4 cycles (scalar and vector). Hence, to
achieve maximum performance, 4 wavefront per compute units must be ran.

NOTE: Simple single dword (4-byte) instruction is executed in 4 cycles (thanks fast
dispatching from cache). However, 2 dword instruction can require 4 extra cycles
to execution due to bigger size in memory and limits of instruction dispatching.
To achieve best performance, we recommend to use single dword instructions.

In some tables present DPFACTOR term. This term indicates that number of cycles depends
on the model of GPU as follows:

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

Waves - number of concurrent waves that can be computed by single SIMD unit  
SGPRs - number of maximum SGPRs that can be allocated that occupancy  
VPGRs - number of maximum VGPRs that can be allocated that occupancy  
LdsW/I - Maximum amount of LDS space per vector lane per wavefront in dwords  
Issue - number of maximum instruction per clock that can be processed  

Each compute unit partitioned into four SIMD units. So, maximum number of waves per
compute unit is 40.

### Instruction alignment

Aligmnent Rules for 2-dword instructions (GCN 1.0/1.1):

* any penalty costs 4 cycles
* program divided by in 32-byte blocks
* only first 3 places (dwords) in 32-byte block is free (no penalty). Any 2-dword
instruction outside these first 3 dwords adds single penalty.
* if instructions is longer (more than four cycles) then last cycles/4 dwords are free
* if 16 or more cycle 2-dword instruction and 2 dword instruction in 4 dword, then
no penalty for second 2-dword instruction.
* best place to jump is 5 first dwords in 32-byte block. Jump to rest of dwords causes
1-3 penalties, depending on number of dword (N-4, where N is number of dword). This rule
does not apply to backward jumps (???)
* any conditional jump instruction should be in first half of 32-byte block, otherwise
1-4 penalties will be added if jump was not taken, depending on number of dword
(N-3, where N is number of dword).

IMPORTANT: If occupancy is greater than 1 wave per compute unit, then penalties,
branches, and scalar instructions will be masked while executing
more waves than 4\*CUs. For best results is recommended to execute many waves
(multiple of 4\*CUs) with occupancy greater than 1.

### Instruction scheduling

* between any integer V_ADD\*, V_SUB\*, V_FIRSTREADLINE_B32, V_READLANE_B32 operation
and any scalar ALU instruction is 16-cycle delay. Masked if more waves than 4*CUs
* any conditional jump directly that checks VCCZ or EXECZ after instruction that changes
VCC or EXEC adds single penalty (4 cycles). Masked if more waves than 4*CUs
* any conditional jump directly that checks SCC after instruction that changes SCC,
EXEC, VCC adds single penalty (4 cycles). Masked if more waves than 4*CUs

### SOP2 Instruction timings

All SOP2 instructions (S_CBRANCH_G_FORK not checked) takes 4 cycles.

### SOPK Instruction timings

All SOPK instructions (S_CBRANCH_I_FORK  not checked) takes 4 cycles.
S_SETREG_B32 and S_SETREG_IMM32_B32 takes 8 cycles.
 
### SOP1 Instruction timings

The S_*_SAVEEXEC_B64 instructions takes 8 cycles. Other ALU instructions (except
S_MOV_REGRD_B32, S_CBRANCH_JOIN, S_RFE_B64) take 4 cycles.

### SOPC Instruction timings

All comparison and bit checking instructions take 4 cycles.

### SOPP Instruction timings

Jumps costs 4 (no jump) or 20 cycles (???) if jump will performed.

### VOP2 Instruction timings

All VOP2 instructions takes 4 cycles.

### VOP1 Instruction timings

Timings of VOP1 instructions is in this table:

 Instruction           | Cycles        | Instruction           | Cycles
-----------------------|---------------|-----------------------|---------------
 V_BFREV_B32           | 4             | V_FREXP_EXP_I32_F32   | 4
 V_CEIL_F32            | 4             | V_FREXP_EXP_I32_F64   | DPFACTOR*4
 V_CEIL_F64            | DPFACTOR*4    | V_FREXP_MANT_F32      | 4
 V_CLREXCP             | 4             | V_FREXP_MANT_F64      | DPFACTOR*4
 V_COS_F32             | 16            | V_LOG_CLAMP_F32       | 16
 V_CVT_F16_F32         | 4             | V_LOG_F32             | 16
 V_CVT_F32_F16         | 4             | V_LOG_LEGACY_F32      | 16
 V_CVT_F32_F64         | DPFACTOR*4    | V_MOVRELD_B32         | 4
 V_CVT_F32_I32         | 4             | V_MOVRELSD_B32        | 4
 V_CVT_F32_U32         | 4             | V_MOVRELS_B32         | 4
 V_CVT_F32_UBYTE0      | 4             | V_MOV_B32             | 4
 V_CVT_F32_UBYTE1      | 4             | V_MOV_FED_B32         | 4
 V_CVT_F32_UBYTE2      | 4             | V_NOP                 | 4
 V_CVT_F32_UBYTE3      | 4             | V_NOT_B32             | 4
 V_CVT_F64_F32         | DPFACTOR*4    | V_RCP_CLAMP_F32       | 16
 V_CVT_F64_I32         | DPFACTOR*4    | V_RCP_CLAMP_F64       | DPFACTOR*8
 V_CVT_F64_U32         | DPFACTOR*4    | V_RCP_F32             | 16
 V_CVT_FLR_I32_F32     | 4             | V_RCP_F64             | DPFACTOR*8
 V_CVT_I32_F32         | 4             | V_RCP_IFLAG_F32       | 16
 V_CVT_I32_F64         | DPFACTOR*4    | V_RCP_LEGACY_F32      | 16
 V_CVT_OFF_F32_I4      | 4             | V_READFIRSTLANE_B32   | 4
 V_CVT_RPI_I32_F32     | 4             | V_RNDNE_F32           | 4
 V_CVT_U32_F32         | 4             | V_RNDNE_F64           | DPFACTOR*4
 V_CVT_U32_F64         | DPFACTOR*4    | V_RSQ_CLAMP_F32       | 16
 V_EXP_F32             | 16            | V_RSQ_CLAMP_F64       | DPFACTOR*8
 V_EXP_LEGACY_F32      | 16            | V_RSQ_F32             | 16
 V_FFBH_I32            | 4             | V_RSQ_F64             | DPFACTOR*8
 V_FFBH_U32            | 4             | V_RSQ_LEGACY_F32      | 16
 V_FFBL_B32            | 4             | V_SIN_F32             | 16
 V_FLOOR_F32           | 4             | V_SQRT_F32            | 16
 V_FLOOR_F64           | DPFACTOR*4    | V_SQRT_F64            | DPFACTOR*8
 V_FRACT_F32           | 4             | V_TRUNC_F32           | 4
 V_FRACT_F64           | DPFACTOR*4    | V_TRUNC_F64           | DPFACTOR*4

### VOPC Instruction timings

All 32-bit comparison instructions takes 4 cycles. All 64-bit comparison instructions takes
DPFACTOR*4 cycles.

