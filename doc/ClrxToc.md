# CLRadeonExtender Manual

CLRadeonExtender is package to low-level development for Radeon GPU's compatible
with GCN1.0/1.1/1.2/1.4 (AMD VEGA) architecture.
It provides an assembler and disassembler that
can handle the AMD Catalyst(TM), the AMDGPU-PRO OpenCL,
the GalliumCompute and the ROCm-OpenCL program binaries.

---

Copyright (c)  2015-2018  Mateusz Szpakowski.  
    Permission is granted to copy, distribute and/or modify this document
    under the terms of the GNU Free Documentation License, Version 1.2
    or any later version published by the Free Software Foundation;
    with no Invariant Sections, no Front-Cover Texts, and no Back-Cover Texts.
    A copy of the license is included in the section entitled "GNU
    Free Documentation License".

---

## Table of Contents

* [GNU Free Documentation License](DocLicense)
* [Disassembler](ClrxDisasm)
* Assembler
    * [Invoking assembler](ClrxAsmInvoke)
    * [Assembler syntax](ClrxAsmSyntax)
    * [Assembler policy](ClrxAsmPolicy)
    * [Assembler pseudo-operations](ClrxAsmPseudoOps)
    * [AMD Catalyst OpenCL 1.2 handling](ClrxAsmAmd)
    * [AMD Catalyst OpenCL 2.0 handling](ClrxAsmAmdCl2)
    * [GalliumCompute handling](ClrxAsmGallium)
    * [ROCm handling](ClrxAsmRocm)
* [CLRX libraries](ClrxLibraries)
* [CLRXWrapper](ClrxWrapper)
* [Binary formats](BinaryFormats)
* [GalliumCompute ABI](GalliumAbi)
* [AMD Catalyst ABI](AmdAbi)
* [AMD Catalyst OpenCL 2.0 ABI](AmdCl2Abi)
* [AMD GCN Instruction Set](GcnIsa)
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
