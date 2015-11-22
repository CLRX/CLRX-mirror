## AMD GCN Instruction Set Architecture

This chapter describes an instruction set of the GCN architecture, their addressing modes
and features.

The GPU architectures differs significantly from CPU architectures. Main pressure in the GPU
architectures is the parallelism and an efficient hiding memory latencies.
The most CPU architectures provides an unified memory access approach. By contrast,
the most GPU's have few different resource types for which access is different. Hence,
few instruction's kinds: scalar, vector, main memory access instructions.

### Language that describes operation.

In 'Operation' field, this document describes operation in specific computer language.
This language is very similar to C/C++ and uses this same expresion's syntax
(these same operators and their precedence). In this language, we use types there are
to similar C/C++ types:

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

By default, any register value is treated as unsigned integer.

---

* [GCN Operands](GcnOperands)
* [SOP2 instructions](GcnInstrsSop2)
* [SOPK instructions](GcnInstrsSopk)
* [SOP1 instructions](GcnInstrsSop1)
* [SOPC instructions](GcnInstrsSopc)
* [SOPP instructions](GcnInstrsSopp)
* [SMRD instructions](GcnInstrsSmrd)
* [VOP2 instructions](GcnInstrsVop2)

---