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
* best place to jump is 5 first dwords in 32-byte block. Jump to rest of dwords causes
1-3 penalties, depending on number of dword (N-4, where N is number of dword). This rule
does not apply to backward jumps (???)
* any conditional jump instruction should be in first half of 32-byte block, otherwise
1-4 penalties will be added if jump was not taken, depending on number of dword
(N-3, where N is number of dword).

### Instruction scheduling

* between any integer V_ADD\*, V_SUB\*, V_FIRSTREADLINE_B32, V_READLANE_B32 operation
and any scalar ALU instruction is 16-cycle delay.
* any conditional jump directly that checks VCCZ or EXECZ after instruction that changes
VCC or EXEC adds single penalty (4 cycles)
* any conditional jump directly that checks SCC after instruction that changes SCC,
EXEC, VCC adds single penalty (4 cycles)

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
