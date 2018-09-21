## CLRadeonExtender Disassembler

The CLRadeonExtender provides a disassembler that can disassemble code
for the Radeon GPU's based on the GCN 1.0/1.1/1.2/1.4 (AMD VEGA) architecture.
Program is called `clrxdisasm`.

Disassembler can handle the AMD Catalyst(tm) OpenCL(tm) kernel binaries and the
GalliumCompute kernel binaries. It displays instructions of the code and optionally
structure of the binaries (kernels and their configuration). Output of that program
can be used as input to the CLRX assembler if option '--all' will be used.

A disassembler can detect automatically binary format, bitness of the binary.

### Invoking a disassembler

The `clrxdisasm` can be invoked in following way:

clrxdisasm [-mdcCfsHLhar?] [-g GPUDEVICE] [-a ARCH] [-t VERSION] [--metadata] [--data]
[--calNotes] [--config] [--floats] [--hexcode] [--setup] [--HSAConfig] [--HSALayout]
[--all] [--raw] [--gpuType=GPUDEVICE] [--arch=ARCH] [--driverVersion=VERSION]
[--llvmVersion=VERSION] [--buggyFPLit] [--help] [--usage] [--version] [file...]

### Program Options

Following options `clrxdisasm` can recognize:

* **<-m>**, **--metadata>**

    Print metadata from AMD Catalyst binaries to output. For a AMD Catalyst binaries,
disassembler prints internal metadata. For a GalliumCompute binaries disassembler
prints argument of the kernel and proginfo entries.

* **-d**, **--data**

    Print data section from binaries. For AMD Catalyst binaries disassembler prints
global constant data, and '.data' section for particular kernel executables.
For GalliumCompute binaries disassembler prints a global constant data.

* **-c**, **--calNotes**

    Print list of the ATI CAL notes and their content from AMD Catalyst binaries to output.

* **-C**, **--config**

    Print human-readable configuration instead of metadatas, headers and ATI CAL notes.
    
* **-f**, **--float**

    Print floating point literals in instructions if instructions accept float point values
and their has a constant literal. Floating point values will be inside comment.

* **-h**, **--hexcode**

    Print hexadecimal code before disassembled instruction in comment. Hexadecimal code
will be printed in 4-byte words.

* **-s**, **--setup**

    Print AMD OpenCL 2.0 kernel setup data.

* **-H**, **--HSAConfig**

    Print AMD OpenCL 2.0 kernel setup configuration as AMD HSA configuration.

* **-L**, **--HSALayout**

    Print AMD OpenCL 2.0 code like Gallium or ROCm: print text section as program code
with kernel names.

* **-a**, **--all**

    Enable all options -mdcfh (except -C).

* **-r**, **--raw**

    Treat input as raw code. By default, disassembler assumes that input code is for
the GCN1.0 architecture.

* **-g GPUDEVICE**, **--gpuType=GPUDEVICE**

    Choose device type. Device type name is case-insensitive.
List of supported GPUs: 
Baffin, Bonaire, CapeVerde, Carrizo, Dummy, Ellesmere, Fiji, GFX700, GFX701, GFX801,
GFX802, GFX803, GFX804, GFX900, GFX901, GFX902, GFX903, GFX904, GFX905, GFX906, GFX907,
Goose, Hainan, Hawaii, Horse, Iceland, Kalindi, Mullins, Oland, Pitcairn, Polaris10,
Polaris11, Polaris12, Polaris20, Polaris21, Polaris22, Raven, Spectre, Spooky, Stoney,
Tahiti, Tonga, Topaz, Vega10, Vega11, Vega12 and Vega20.

* **-A ARCH**, **--arch=ARCH**

    Choose device architecture. Architecture name is case-insensitive.
List of supported architectures:
SI, VI, CI, VEGA, VEGA20, GFX6, GFX7, GFX8, GFX9, GFX906, GCN1.0, GCN1.1, GCN1.2,
GCN1.4 and GCN1.4.1.

* **-t VERSION**, **--driverVersion=VERSION**

    Choose AMD Catalyst OpenCL driver version for which binaries are generated. 
Version can retrieved from clinfo program that display field 'Driver version'
where version is. Version is number in that form: MajorVersion*100 + MinorVersion.
Used for AMD OpenCL 2.0 binaries.

* **--llvmVersion=VERSION**

    Choose LLVM version that generates binaries.
Version is number in that form: MajorVersion*100 + MinorVersion.


* **--buggyFPLit**

    Choose old and buggy floating point literals rules (to 0.1.2 version)
for compatibility.

* **-?**, **--help**

    Print help and list of the options.

* **--usage**

    Print usage for this program

* **--version**

    Print version

### Output

`clrxdisasm` prints a disassembled code to standard output and errors to
standard error output. `clrxdisasm` returns 0 if succeeded, otherwise it returns 1
and prints the error messages to stderr
    
### Sample usages

Following sample usages:

* `clrxdisasm -aC source.clo`

    Disassemble binary file source.clo. Print addresess, opcodes, metadata in human readable form.

* `clrxdisasm -a source.clo`

    Disassemble binary file source.clo. Print addresess, opcodes, metadata in machine readable form
(enough rarely used).

* `clrxdisasm -aC -t240400 source.clo`

    Disassemble binary file source.clo including AMD driver version 240400.
Print addresess, opcodes, metadata in human readable form.

* `clrxdisasm -aCHL -t240400 source.clo`

    Disassemble binary file AMD OpenCL 2.0 source.clo including AMD driver version 240400 in
new HSA layout form (like ROCm).
Print addresess, opcodes, metadata in human readable HSA config form.

* `clrxdisasm -aC -gBonaire source.clo`

    Disassemble binary file source.clo for Bonaire GPU device. It can be used while
disassemblying GalliumCompute binaries. Print addresess, opcodes, metadata in human readable form.

* `clrxdisasm -aC -t170000 --llvmVersion=40000 -gBonaire source.clo`

    Disassemble new GalliumCompute (for new MesaOpenCL 17.0.0 or later and LLVM 4.0.0 or later)
binary file source.clo for Bonaire GPU device.
Print addresess, opcodes, metadata in human readable form.
