## GCN ISA Instruction Timings

### Preliminary explanations

The almost instructions are executed within 4 cycles (scalar and vector). Hence, to
achieve maximum performance, 4 wavefront per compute units must be ran. 

NOTE: Simple single dword (4-byte) instruction is executed in 4 cycles (thanks fast
dispatching from cache). However, 2 dword instruction can require 4 extra cycles
to execution due to bigger size in memory and limits of instruction dispatching.
To achieve best performance, we recommend to use single dword instructions.

The 'Delay' column contains instruction's delays (how many cycles needed to execute
instruction). The 'Throughput' contains instruction's throughputs (maximum number of
instructions per cycle).

### Instruction alignment

Aligmnent Rules for 2-dword instructions (GCN 1.0):

* any penalty costs 4 cycles
* program divided by in 32-byte blocks
* only first 3 places (dwords) in 32-byte block is free (no penalty). Any 2-dword
instruction outside these first 3 dwords adds single penalty.
* if instructions is longer (more than four cycles) then last cycles/4 dwords are free
* if 16 or more cycle 2-dword instruction and 2 dword instruction in 4 dword, then
no penalty for second 2-dword instruction.

### Instruction scheduling

Between any vector operation that operates on VCC and any scalar ALU instruction is
16-cycle delay.

### SOP2 Instruction timings

 Instruction      | Delay  | Throughput
------------------|--------|-------------
 S_ABSDIFF_I32    | 4      | 1
 S_ADDC_U32       | 4      | 1
 S_ADD_I32        | 4      | 1
 S_ADD_U32        | 4      | 1
 S_ANDN2_B32      | 4      | 1
 S_ANDN2_B64      | 4      | 1
 S_AND_B32        | 4      | 1
 S_AND_B64        | 4      | 1
 S_ASHR_I32       | 4      | 1
 S_ASHR_I64       | 4      | 1
 S_BFE_I32        | 4      | 1
 S_BFE_I64        | 4      | 1
 S_BFE_U32        | 4      | 1
 S_BFE_U64        | 4      | 1
 S_BFM_B32        | 4      | 1
 S_BFM_B64        | 4      | 1
 S_CBRANCH_G_FORK |        |
 S_CSELECT_B32    | 4      | 1
 S_CSELECT_B64    | 4      | 1
 S_LSHL_B32       | 4      | 1
 S_LSHL_B64       | 4      | 1
 S_LSHR_B32       | 4      | 1
 S_LSHR_B64       | 4      | 1
 S_MAX_I32        | 4      | 1
 S_MAX_U32        | 4      | 1
 S_MIN_I32        | 4      | 1
 S_MIN_U32        | 4      | 1
 S_MUL_I32        | 4      | 1
 S_NAND_B32       | 4      | 1
 S_NAND_B64       | 4      | 1
 S_NOR_B32        | 4      | 1
 S_NOR_B64        | 4      | 1
 S_ORN2_B32       | 4      | 1
 S_ORN2_B64       | 4      | 1
 S_OR_B32         | 4      | 1
 S_OR_B64         | 4      | 1
 S_SUBB_U32       | 4      | 1
 S_SUB_I32        | 4      | 1
 S_SUB_U32        | 4      | 1
 S_XNOR_B32       | 4      | 1
 S_XNOR_B64       | 4      | 1
 S_XOR_B32        | 4      | 1
 S_XOR_B64        | 4      | 1

### SOPK Instruction timings

 Instruction       | Delay  | Throughput
-------------------|--------|-------------
 S_ADDK_I32        | 4      | 1
 S_CBRANCH_I_FORK  |        |
 S_CMOVK_I32       | 4      | 1
 S_CMPK_EQ_I32     | 4      | 1
 S_CMPK_EQ_U32     | 4      | 1
 S_CMPK_GE_I32     | 4      | 1
 S_CMPK_GE_U32     | 4      | 1
 S_CMPK_GT_I32     | 4      | 1
 S_CMPK_GT_U32     | 4      | 1
 S_CMPK_LE_I32     | 4      | 1
 S_CMPK_LE_U32     | 4      | 1
 S_CMPK_LG_I32     | 4      | 1
 S_CMPK_LG_U32     | 4      | 1
 S_CMPK_LT_I32     | 4      | 1
 S_CMPK_LT_U32     | 4      | 1
 S_GETREG_B32      |        |
 S_GETREG_REGRD_B32 |       |
 S_MOVK_I32        | 4      | 1
 S_MULK_I32        | 4      | 1
 S_SETREG_B32      |        |
 S_SETREG_IMM32_B32 |       |

 
### SOP1 Instruction timings

The S_*_SAVEEXEC_B64 instructions takes 8 cycles. Other ALU instructions (expects
S_MOV_REGRD_B32, S_CBRANCH_JOIN, S_RFE_B64) take 4 cycles.

### SOPC Instruction timings

All comparison and bit checking instructions take 4 cycles.

