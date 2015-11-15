### Operand encoding

The GCN1.0/1.1 delivers maximum 104 registers (with VCC). Basic list of destination
scalar operands have 128 entries. Source operands codes is in range 0-255.

**Important**: Two SGPR's must be aligned to 2. Four or more SGPR's must be aligned to 4.

Following list describes all operand codes values:

Code     | Name              | Description
---------|-------------------|------------------------
0-103    | S0 - S103         | SGPR's (GCN1.0/1.1)
0-101    | S0 - S102         | SGPR's (GCN1.2)
104-105  | FLAT_SCRATCH      | FLAT_SCRATCH register (GCN1.1)
104      | FLAT_SCRATCH_LO   | Low half of FLAT_SCRATCH register (GCN1.1)
105      | FLAT_SCRATCH_HI   | High half of FLAT_SCRATCH register (GCN1.1)
102-103  | FLAT_SCRATCH      | FLAT_SCRATCH register (GCN1.2)
102      | FLAT_SCRATCH_LO   | Low half of FLAT_SCRATCH register (GCN1.2)
103      | FLAT_SCRATCH_HI   | High half of FLAT_SCRATCH register (GCN1.2)
104-105  | XNACK_MASK        | XNACK_MASK register
104      | XNACK_MASK_LO     | Low half of XNACK_MASK register
105      | XNACK_MASK_HI     | High half of XNACK_MASK register
106-107  | VCC               | VCC (vector carry register) two last SGPR's
106      | VCC_LO            | Low half of VCC
107      | VCC_HI            | High half of VCC
108-109  | TBA               | Trap handler base address
108      | TBA_LO            | Low half of TBA register
109      | TBA_HI            | High half of TBA register
110-111  | TMA               | Pointer to data in memory used by trap handler
110      | TMA_LO            | Low half of TMA register
111      | TMA_HI            | High half of TMA register
112-123  | TTMP0 - TTMP11    | Trap handler temporary registers
124      | M0                | M0. Memory register
125      | -                 | reserved
126-127  | EXEC              | EXEC register
126      | EXEC_LO           | Low half of EXEC register
127      | EXEC_HI           | High half of EXEC register
128      | 0                 | 0
129-192  | 1-64              | 1 to 64 constant value
193-208  | -1 - -16          | -1 to -16 constant value
209-239  | -                 | reserved
240      | 0.5               | 0.5 floating point value
241      | -0.5              | -0.5 floating point value
242      | 1.0               | 1.0 floating point value
243      | -1.0              | -1.0 floating point value
244      | 2.0               | 2.0 floating point value
245      | -2.0              | -2.0 floating point value
246      | 4.0               | 4.0 floating point value
247      | -4.0              | -4.0 floating point value
248      | 1/(2*PI)          | 1/(2*PI)
249      | --                | SDWA dword (GCN1.2)
250      | --                | DPP dword (GCN1.2)
251      | VCCZ              | VCCZ register
252      | EXECZ             | EXECZ register
253      | SCC               | SCC register
255      | 255               | Literal constant (follows instruction dword)
256-511  | V0-V255           | VGPR's (only VOP3 encoding operands)

### Operand syntax

Single operands can be given by their name: `s0`, `v54`. CLRX assemblers accepts syntax with
brackets: `s[0]`, `s[z]`, `v[66]`. In many instructions operands are
64-bit, 96-bit or even 128-bit. These operands consists several registers that can be
expressed by ranges: `v[3:4]`, `s[8:11]`, `s[16:23]`, where second value is
last register's number.

Names of the registers are case-insensitive.

Constant values are automatically resolved if expression have already value.
The 1/(2*PI), 1.0, -2.0 and other floating point constant values will be
resolved if that accurate floating point value will be given.

In instruction syntax, operands are listed by name of the encoding field. Optionally, in
parentheses is given number of the registers. Example:

Syntax: S_SUB_I32 SDST, SSRC0, SSRC1  
Syntax: S_AND_B64 SDST(2), SSRC0(2), SSRC1(2)  

### Hardware registers

These register could be read or written by S_GETREG_\* and S_SETREG_\* instruction.

List of hardware registers:

* GPR_ALLOC, HWREG_GPR_ALLOC - 
* HW_ID, HWREG_HW_ID - 
* IB_DBG0, HWREG_DBG0 - 
* IB_STS, HWREG_IB_STS -
* INST_DW0, HWREG_INST_DW0 -
* INST_DW1, HWREG_INST_DW1 -
* LDS_ALLOC, HWREG_LDS_ALLOC -
* MODE, HWREG_MODE -
* PC_HI, HWREG_PC_HI -
* PC_LO, HWREG_PC_LO -
* STATUS, HWREG_STATUS -
* TRAPSTS, HWREG_TRAPSTS -
