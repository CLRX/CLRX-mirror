## CLRadeonExtender Assembler AMD Catalyst handling

The AMD Catalyst driver provides own OpenCL implementation that can generates
own binaries of the OpenCL programs. The CLRX assembler supports only OpenCL 1.2
binary format.

## Binary format

The AMD OpenCL binaries contains constant global data, the device and compilation
informations and embedded kernel binaries. Kernel binaries are inside `.text` section.
Program code are separate for each kernel and no shared machine code between kernels.
Each kernel binary have the metadata string, ATI CAL notes and program code.
The metadata strings describes the kernel arguments, settings of the
input/output buffers, constant buffers, read only and write only images, local data.
ATI CAL notes are special small data fragments that describes features of the kernel.
The most important ATI CAL note is PROGINFO that holds important data for runtime execution,
like register usage, UAV usage, floating point setup.

## Layout of the source code

The CLRX assembler allow to use one of two ways to configure kernel setup:
for human (`.config`) and for quick recompilation (ATI CALNotes and the metadata string).

## List of the specific pseudo-operations

### .arg

### .calnote

### .compile_options

### .config