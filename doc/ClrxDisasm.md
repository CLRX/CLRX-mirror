## CLRadeonExtender Disassembler

The CLRadeonExtender provides a disassembler that can disassemble code
for the Radeon GPU's based on the GCN 1.0/1.1/1.2 architecture.
Program is called `clrxdisasm`.

Disassembler can handle the AMD Catalyst(tm) OpenCL(tm) kernel binaries and the
GalliumCompute kernel binaries. It displays instructions of the code and optionally
structure of the binaries (kernels and their configuration). Output of that program
can be used as input to the CLRX assembler if option '--all' will be used.

### Invoking a disassembler

The `clrxdisasm` can be invoked in following way:

clrxdisasm [-mdcCfhar?] [-g GPUDEVICE] [-a ARCH] [-t VERSION] [--metadata] [--data]
[--calNotes] [--config] [--floats] [--hexcode] [--all] [--raw] [--gpuType=GPUDEVICE]
[--arch=ARCH] [--driverVersion=VERSION] [--buggyFPLit] [--help] [--usage] [--version]
[file...]

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

* **-a**, **--all**

    Enable all options -mdcfh (except -C).

* **-r**, **--raw**

    Treat input as raw code. By default, disassembler assumes that input code is for
the GCN1.0 architecture.

* **-g GPUDEVICE**, **--gpuType=GPUDEVICE**

    Choose device type. Device type name is case-insensitive.
Currently is supported: 
CapeVerde, Pitcairn, Tahiti, Oland, Bonaire, Spectre, Spooky, Kalindi,
Hainan, Hawaii, Iceland, Tonga, Mullins, Fiji, Carrizo, Dummy, Goose, Horse, Stoney,
Ellesmere, and Baffin.

* **-A ARCH**, **--arch=ARCH**

    Choose device architecture. Architecture name is case-insensitive.
List of supported architectures:
GCN1.0, GCN1.1 and GCN1.2.

* **-t VERSION**, **--driverVersion=VERSION**

    Choose AMD Catalyst OpenCL driver version for which binaries are generated. 
Version can retrieved from clinfo program that display field 'Driver version'
where version is. Version is number in that form: MajorVersion*100 + MinorVersion.
Used for AMD OpenCL 2.0 binaries.

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
and prints an error messages to stderr
    
### Sample usage

Below is sample usage of the `clrxdisasm`:

```
clrxdisasm -a DCT.amd.0
```
