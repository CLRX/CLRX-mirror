## AMD GCN Instruction Set Architecture

This chapter describes an instruction set of the GCN architecture, their addressing modes
and features.

The GPU architectures differs significantly from CPU architectures. Main pressure in the GPU
architectures is the parallelism and an efficient hiding memory latencies.
The most CPU architectures provides an unified memory access approach. By contrast,
the most GPU's have few different resource types for which access is different. Hence,
few instruction's kinds: scalar, vector, main memory access instructions.

### GCN architecture versions

List of known GCN versions:

CLRX Version | AMD Version  | Example devices
-------------|--------------|-------------------------------------
 GCN 1.0     | GCN 1        | Pitcairn (HD 7850), Tahiti (HD 7970), Cape verde
 GCN 1.1     | GCN 2        | Bonaire (R7 260), Hawaii (R9 290)
 GCN 1.2     | GCN 3, GCN 4 | Tonga (R9 285), Fiji (Fury X), Ellesmere (RX 480), Baffin
 GCN 1.4     | GCN 5        | VEGA (GFX900) (RX VEGA 64)
 GCN 1.4.1   | GCN 5,VEGA20 | VEGA20 (GFX906) (VEGA with Deep Learning extensions)
 
List of architecture names:

CLRX Version | CLRX names
-------------|----------------------------
 GCN 1.0     | GCN1.0, GFX6, SI
 GCN 1.1     | GCN1.1, GFX7, CI
 GCN 1.2     | GCN1.2, GFX8, VI
 GCN 1.4     | GCN1.4, GFX9, VEGA
 GCN 1.4.1   | GCN1.4.1, GFX906, VEGA20

### Instruction suffixes

Optionally, suffixes can be appended to instruction mnemonic to indicate encoding size.
`_e32` suffix marks that instruction will be encoded in single dword.
`_e64` suffix marks that instruction will be encoded in two dwords.
`_sdwa` suffix marks that instruction uses SDWA encoding.
`_dpp` suffix marks that instruction uses DPP encoding.

### Language that describes operation.

In 'Operation' field, this document describes operation in specific computer language.
This language is very similar to C/C++ and uses this same expresion's syntax
(these same operators and their precedence). In this language, we use types there are
to similar C/C++ types:

* BYTE - unsigned byte
* UINT8, INT8 - unsigned and signed byte
* UINT16, INT16 - unsigned and signed 16-bit word
* UINT32, INT32 - unsigned and signed dword (32-bit word)
* UINT64, INT64 - unsigned and signed 64-bit word
* HALF, FLOAT, DOUBLE - half, single and double precision floating point

Special variables:

* LANEID - identifier for current thread in wave

Special functions:

* SEXT64(v) - sign extend to 64-bit from any signed value
* ABS(v) - absolute value, if value is maximum negative then returns this value.
* BITCOUNT(v) - count 1's bits in value
* REVBIT(v) - reverse bits (n bit goes to BITS-n-1 bit,
where BITS is number bits in operand).
* MIN(v1, v2) - return smallest value from two values
* MAX(v1, v2) - return largest value from two values
* ASHALF(v) - treat raw 16-bit integer value as IEEE half floating point value
* ASFLOAT(v) - treat raw 32-bit integer value as IEEE floating point value
* ASDOUBLE(v) - treat raw 32-bit integer value as IEEE double floating point value
* ASINT16(v), ASINT32(v), ASINT64(v) - treat raw floating point value as signed integer
* ASUINT16(v), ASUINT32(v), ASUINT64(v) - treat raw floating point value as unsigned integer
* RNDINT(v) - round floating point value to integer with rounding mode from MODE register,
    returns FP value
* ISNAN(v) - return true if value v is NAN value

Shortcuts:

* FP - floating point (default single if not specified)
* FP16 - half floating point
* FP32 - single floating point
* FP64 - double floating point

By default, any register value is treated as unsigned integer.

---

* [GCN Operands](GcnOperands)
* [GCN Machine State](GcnState)
* [SOP2 instructions](GcnInstrsSop2)
* [SOPK instructions](GcnInstrsSopk)
* [SOP1 instructions](GcnInstrsSop1)
* [SOPC instructions](GcnInstrsSopc)
* [SOPP instructions](GcnInstrsSopp)
* [SMEM instructions](GcnInstrsSmem)
* [SMRD instructions](GcnInstrsSmrd)
* [VOP2 instructions](GcnInstrsVop2)
* [VOP1 instructions](GcnInstrsVop1)
* [VOPC instructions](GcnInstrsVopc)
* [VOP3 instructions](GcnInstrsVop3)
* [VOP3P instructions](GcnInstrsVop3p)
* [SDWA and DPP encodings](GcnSdwaDpp)
* [VINTRP instructions](GcnInstrsVintrp)
* [DS instructions](GcnInstrsDs)
* [Main memory handling](GcnMemHandling)
* [MUBUF instructions](GcnInstrsMubuf)
* [MTBUF instructions](GcnInstrsMtbuf)
* [MIMG instructions](GcnInstrsMimg)
* [FLAT instructions](GcnInstrsFlat)
* [GCN Instruction Timings](GcnTimings)

---
