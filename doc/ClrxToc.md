# CLRadeonExtender Manual

CLRadeonExtender is package to low-level development for Radeon GPU's compatible
with GCN1.0/1.1/1.2 architecture. It provides an assembler and disassembler that
can handle the AMD Catalyst(TM) OpenCL program binaries and
the GalliumCompute program binaries.

---

## Table of Contents

* [Disassembler](ClrxDisasm)
* Assembler
    * [Invoking assembler](ClrxAsmInvoke)
    * [Assembler syntax](ClrxAsmSyntax)
    * [Assembler pseudo-operations](ClrxAsmPseudoOps)
    * [AMD Catalyst OpenCL handling](ClrxAsmAmd)
    * [GalliumCompute handling](ClrxAsmGallium)
* [CLRXWrapper](ClrxWrapper)
* [GalliumCompute ABI](GalliumAbi)
* [AMD Catalyst ABI](AmdAbi)
* [AMD GCN Instruction Set](GcnIsa)
    * [GCN Operands](GcnOperands)
    * [SOP2 instructions](GcnInstrsSop2)
    * [SOPK instructions](GcnInstrsSopk)
    * [SOP1 instructions](GcnInstrsSop1)
    * [SOPC instructions](GcnInstrsSopc)
    * [SOPP instructions](GcnInstrsSopp)
    * [SMRD instructions](GcnInstrsSmrd)
    * [VOP2 instructions](GcnInstrsVop2)
    * [VOP1 instructions](GcnInstrsVop1)
    * [VOPC instructions](GcnInstrsVopc)

---
