## GCN ISA SOPP instructions

The basic encoding of the SOPP instructions needs 4 bytes (dword). List of fields:

Bits  | Name     | Description
------|----------|------------------------------
0-15  | SIMM16   | Signed or unsigned 16-bit immediate value
16-22 | OPCODE   | Operation code
23-31 | ENCODING | Encoding type. Must be 0b101111111

Syntax for almost instructions: INSTRUCTION SIMM16

SIMM16 - signed 16-bit immediate. IMM16 - unsigned 16-bit immediate.  
RELADDR - relative offset to this instruction (can be label or relative expresion).
RELADDR = NEXTPC + SIMM16, NEXTPC - PC for next instruction.

List of the instructions by opcode:

 Opcode     |GCN 1.0|GCN 1.1|GCN 1.2|GCN 1.4| Mnemonic
------------|-------|-------|-------|-------|-----------------
 0 (0x0)    |   ✓   |   ✓   |   ✓   |   ✓   | S_NOP
 1 (0x1)    |   ✓   |   ✓   |   ✓   |   ✓   | S_ENDPGM
 2 (0x2)    |   ✓   |   ✓   |   ✓   |   ✓   | S_BRANCH
 3 (0x3)    |       |       |   ✓   |   ✓   | S_WAKEUP
 4 (0x4)    |   ✓   |   ✓   |   ✓   |   ✓   | S_CBRANCH_SCC0
 5 (0x5)    |   ✓   |   ✓   |   ✓   |   ✓   | S_CBRANCH_SCC1
 6 (0x6)    |   ✓   |   ✓   |   ✓   |   ✓   | S_CBRANCH_VCCZ
 7 (0x7)    |   ✓   |   ✓   |   ✓   |   ✓   | S_CBRANCH_VCCNZ
 8 (0x8)    |   ✓   |   ✓   |   ✓   |   ✓   | S_CBRANCH_EXECZ
 9 (0x9)    |   ✓   |   ✓   |   ✓   |   ✓   | S_CBRANCH_EXECNZ
 10 (0xa)   |   ✓   |   ✓   |   ✓   |   ✓   | S_BARRIER
 11 (0xb)   |       |   ✓   |   ✓   |   ✓   | S_SETKILL
 12 (0xc)   |   ✓   |   ✓   |   ✓   |   ✓   | S_WAITCNT
 13 (0xd)   |   ✓   |   ✓   |   ✓   |   ✓   | S_SETHALT
 14 (0xe)   |   ✓   |   ✓   |   ✓   |   ✓   | S_SLEEP
 15 (0xf)   |   ✓   |   ✓   |   ✓   |   ✓   | S_SETPRIO
 16 (0x10)  |   ✓   |   ✓   |   ✓   |   ✓   | S_SENDMSG
 17 (0x11)  |   ✓   |   ✓   |   ✓   |   ✓   | S_SENDMSGHALT
 18 (0x12)  |   ✓   |   ✓   |   ✓   |   ✓   | S_TRAP
 19 (0x13)  |   ✓   |   ✓   |   ✓   |   ✓   | S_ICACHE_INV
 20 (0x14)  |   ✓   |   ✓   |   ✓   |   ✓   | S_INCPERFLEVEL
 21 (0x15)  |   ✓   |   ✓   |   ✓   |   ✓   | S_DECPERFLEVEL
 22 (0x16)  |   ✓   |   ✓   |   ✓   |   ✓   | S_TTRACEDATA
 23 (0x17)  |       |   ✓   |   ✓   |   ✓   | S_CBRANCH_CDBGSYS
 24 (0x18)  |       |   ✓   |   ✓   |   ✓   | S_CBRANCH_CDBGUSER
 25 (0x19)  |       |   ✓   |   ✓   |   ✓   | S_CBRANCH_CDBGSYS_OR_USER
 26 (0x1a)  |       |   ✓   |   ✓   |   ✓   | S_CBRANCH_CDBGSYS_AND_USER
 27 (0x1b)  |       |       |   ✓   |   ✓   | S_ENDPGM_SAVED
 28 (0x1c)  |       |       |   ✓   |   ✓   | S_SET_GPR_IDX_OFF
 29 (0x1d)  |       |       |   ✓   |   ✓   | S_SET_GPR_IDX_MODE
 30 (0x1e)  |       |       |       |   ✓   | S_ENDPGM_ORDERED_PS_DONE

### Instruction set

Alphabetically sorted instruction list:

#### S_BARRIER

Opcode: 10 (0xa)  
Syntax: S_BARRIER  
Description: Synchronize waves within workgroup.

#### S_BRANCH

Opcode: 2 (0x2)  
Syntax: S_BRANCH RELADDR  
Description: Jump to address RELADDR (store RELADDR to PC).  
Operation:  
```
PC = RELADDR
```

#### S_CBRANCH_CDBGSYS

Opcode: 23 (0x17) for GCN 1.1/1.2/1.4  
Syntax: S_CBRANCH_CDBGSYS RELADDR  
Description: Jump to address RELADDR if COND_DBG_SYS status bit is set.

#### S_CBRANCH_CDBGSYS_AND_USER

Opcode: 26 (0x1a) for GCN 1.1/1.2/1.4  
Syntax: S_CBRANCH_CDBGSYS_AND_USER RELADDR  
Description: Jump to address RELADDR if COND_DBG_SYS and COND_DBG_USER status bit is set.

#### S_CBRANCH_CDBGSYS_OR_USER

Opcode: 25 (0x19) for GCN 1.1/1.2/1.4  
Syntax: S_CBRANCH_CDBGSYS_OR_USER RELADDR  
Description: Jump to address RELADDR if COND_DBG_SYS or COND_DBG_USER status bit is set.

#### S_CBRANCH_CDBGUSER

Opcode: 24 (0x18) for GCN 1.1/1.2/1.4  
Syntax: S_CBRANCH_CDBGUSER RELADDR  
Description: Jump to address RELADDR if COND_DBG_USER status bit is set.

#### S_CBRANCH_EXECNZ

Opcode: 9 (0x9)  
Syntax: S_CBRANCH_EXECNZ RELADDR  
Description: If EXEC is not zero then jump to RELADDR, otherwise jump to next instruction.  
Operation:  
```
PC = EXEC!=0 ? RELADDR : PC+4
```

#### S_CBRANCH_EXECZ

Opcode: 8 (0x8)  
Syntax: S_CBRANCH_EXECZ RELADDR  
Description: If EXEC is zero then jump to RELADDR, otherwise jump to next instruction.  
Operation:  
```
PC = EXEC==0 ? RELADDR : PC+4
```

#### S_CBRANCH_SCC0

Opcode: 4 (0x4)  
Syntax: S_CBRANCH_SCC0 RELADDR  
Description: If SCC is zero then jump to RELADDR, otherwise jump to next instruction.  
Operation:  
```
PC = SCC0==0 ? RELADDR : PC+4
```

#### S_CBRANCH_SCC1

Opcode: 5 (0x5)  
Syntax: S_CBRANCH_SCC1 RELADDR  
Description: If SCC is one then jump to RELADDR, otherwise jump to next instruction.  
Operation:  
```
PC = SCC0==1 ? RELADDR : PC+4
```

#### S_CBRANCH_VCCNZ

Opcode: 7 (0x7)  
Syntax: S_CBRANCH_VCCNZ RELADDR  
Description: If VCC is not zero then jump to RELADDR, otherwise jump to next instruction.  
Operation:  
```
PC = VCC!=0 ? RELADDR : PC+4
```

#### S_CBRANCH_VCCZ

Opcode: 6 (0x6)  
Syntax: S_CBRANCH_VCCZ RELADDR  
Description: If VCC is zero then jump to RELADDR, otherwise jump to next instruction.  
Operation:  
```
PC = VCC==0 ? RELADDR : PC+4
```

#### S_DECPERFLEVEL

Opcode: 21 (0x15)  
Syntax: S_DECPERFLEVEL SIMM16  
Description: Decrement performance counter specified in SIMM16&15 by 1.  
```
PERFCNT[SIMM16 & 15]--
```

#### S_ENDPGM

Opcode: 1 (0x1)  
Syntax: S_ENDPGM  
Description: End program.

#### S_ENDPGM_ORDERED_PS_DONE

Opcode: 30 (0x1e) only for GCN 1.4  
Description: End of program; signal that a wave has exited its POPS critical section
and terminate wavefront.  The hardware implicitly
executes S_WAITCNT 0 before executing this instruction. This
instruction is an optimization that combines
S_SENDMSG(MSG_ORDERED_PS_DONE) and S_ENDPGM. (from ISA manual)

#### S_ENDPGM_SAVED

Opcode: 27 (0x1b) only for GCN 1.2/1.4  
Syntax: S_ENDPGM_SAVED  
Description: End of program; signal that a wave has been saved by the context-switch trap handler, and
terminate wavefront. The hardware implicitly executes S_WAITCNT 0 before executing this
instruction. Use S_ENDPGM in all cases unless you are executing the context-switch save
handler. (from ISA manual)

#### S_ICACHE_INV

Opcode: 19 (0x13)  
Syntax: S_ICACHE_INV  
Description: Invalidate entire L1 instruction cache.

#### S_INCPERFLEVEL

Opcode: 20 (0x14)  
Syntax: S_INCPERFLEVEL SIMM16  
Description: Increment performance counter specified in SIMM16&15 by 1.  
```
PERFCNT[SIMM16 & 15]++
```

#### S_NOP

Opcode: 0 (0x0)  
Syntax: S_NOP SIMM16  
Description: Do nothing by (SIMM16&7) + 1 cycles.  
Operation: nothing

#### S_SENDMSG

Opcode: 16 (0x10)  
Syntax: S_SENDMSG SENDMSG(MSG, GS_OP, STREAMID)  
Description: Send message. List of messages:

* INTERRUPT, MSG_INTERRUPT - interrupt. M0&0xff - carries user data,
IDs also sent (wave_id, cu_id, ...)
* GS, MSG_GS
* GS_DONE, MSG_GS_DONE
* SYSMSG, MSG_SYSMSG, SYSTEM, MSG_SYSTEM

List of the GSOP's:

* NOP, GS_NOP - M0&0xff defines wave id. only GS_DONE
* CUT, GS_CUT - (SIMM16 & 0x300)>>8 - streamid, EXEC also sent, M0&0xff - gs waveID
* EMIT, GS_EMIT - (SIMM16 & 0x300)>>8 - streamid, EXEC also sent, M0&0xff - gs waveID
* EMIT_CUT, GS_EMIT_CUT, EMIT-CUT - (SIMM16 & 0x300)>>8 - streamid, EXEC also sent,
M0&0xff - gs waveID

#### S_SENDMSGHALT

Opcode: 17 (0x11)  
Syntax: S_SENDMSGHALT SENDMSG(MSG, GS_OP, STREAMID)  
Description: Send message and halt.

#### S_SET_GPR_IDX_MODE

Opcode: 29 (0x1d) only for GCN 1.2/1.4  
Syntax: S_SET_GPR_IDX_MODE SIMM16  
Description: Set GPR indexing mode (12-15 bits in MO).  
Operation:  
```
M0 = (M0 & 0xffff0fff) | ((SIMM16 & 15)<<12)
```

#### S_SET_GPR_IDX_OFF

Opcode: 28 (0x1c) only for GCN 1.2/1.4  
Syntax: S_SET_GPR_IDX_OFF  
Description: Disables GPR indexing.  
Operation:  
```
MODE = (MODE & ~(1U<<27))
```

#### S_SETHALT

Opcode: 13 (0xd)  
Syntax: S_SETHALT SIMM16  
Description: Set HALT bit to value SIMM16&1. 1 - halt, 0 - resume.
Halt is ignored while PRIV is 1.  
Operation:  
```
HALT = SIMM16&1
```

#### S_SETKILL

Opcode: 11 (0xb) only for GCN 1.1/1.2/1.4  
Syntax: S_SETKILL SIMM16  
Description: Store SIMM16&1 to KILL.  
Operation:  
```
KILL = SIMM16&1
```

#### S_SETPRIO

Opcode: 15 (0xf)  
Syntax: S_SETPRIO SIMM16  
Description: Set priority to  SIMM16&3.  
Operation:  
```
PRIORITY = SIMM16&3
```

#### S_SLEEP

Opcode: 14 (0xe)  
Syntax: S_SLEEP SIMM16  
Description: Sleep approximately by (SIMM16&0x7)*64 cycles.

#### S_TRAP

Opcode: 18 (0x12)  
Syntax: S_TRAP SIMM16  
Description: Enter the trap handler.

#### S_TTRACEDATA

Opcode: 22 (0x16)  
Syntax: S_TTRACEDATA  
Description: Send M0 as user data to thread-trace.

#### S_WAITCNT

Opcode: 12 (0xc)  
Syntax: S_WAITCNT VMCNT(X)|EXPCNT(X)|LGKMCNT(X) [&....]  
Description: Waits for vector memory operations (VMCNT);
LDS, GDS, memory operations (LGKMCNT); export memory-write-data operations (EXPCNT).
(SIMM16&15) specifies how many VMCNT operations can be unfinished (GCN 1.0/1.1/1.2),
value 0xf - no wait for VMCNT operations.
(SIMM16&15)|((SIMM16>>10)&0x30) specifies how many VMCNT operations can be unfinished
(GCN 1.4), value 0x3f - no wait for VMCNT operations.
(SIMM16>>4)&7 specifies how many EXPCNT operations can be unfinished,
0x7 - no wait for EXPCNT operations.
(SIMM16>>8)&31 specifies how many LGKMCNT operations can be unfinished,
0x1f - no wait for LGKM operations.
