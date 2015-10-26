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

Syntax for scalar: .arg ARGNAME[[, "ARGTYPENAME"], ARGTYPE[, unused]]  
Syntax for structure: .arg [[, "ARGTYPENAME"], ARGTYPE[, STRUCTSIZE[, unused]]]  
Syntax for image: .arg ARGNAME[[, "ARGTYPENAME"], ARGTYPE[, [ACCESS] [, RESID[, unused]]]]  
Syntax for counter32: .arg ARGNAME[[, "ARGTYPENAME"], ARGTYPE[, RESID[, unused]]]  
Syntax for global pointer: .arg ARGNAME[[, "ARGTYPENAME"], 
ARGTYPE[[, STRUCTSIZE], PTRSPACE[, [ACCESS] [, RESID[, unused]]]]]  
Syntax for local pointer: .arg ARGNAME[[, "ARGTYPENAME"], 
ARGTYPE[[, STRUCTSIZE], PTRSPACE[, [ACCESS] [, unused]]]]  
Syntax for constant pointer: .arg ARGNAME[[, "ARGTYPENAME"], 
ARGTYPE[[, STRUCTSIZE], PTRSPACE[, [ACCESS] [, [CONSTSIZE] [, RESID[, unused]]]]]]

Adds kernel argument definition. Must be inside kernel configuration. First argument is
argument name from OpenCL kernel definition. Next optional argument is argument type name
from OpenCL kernel definition. Next arugment is argument type:

* char, uchar, short, ushort, int, uint, ulong, long, float, double - simple scalar types
* charX, ucharX, shortX, ushortX, intX, uintX, ulongX, longX, floatX, doubleX - vector types
(X indicates number of elements: 2, 3, 4, 8 or 16)
* counter32 - 32-bit counter type
* structure - structure
* image, image1d, image1d_array, image1d_buffer, image2d, image2d_array, image3d -
image types
* sampler - sampler
* type* - pointer to data

Rest of the argument depends on type of the kernel argument. STRUCTSIZE determines size of
structure. ACCESS for image determines can be one of the: `read_only` or `write_only`.
PTRSPACE determines space where pointer points to.
It can be one of: `local`, `constant` or `global`.
ACCESS for pointers can be: `const`, `restrict` and `volatile`.
CONSTSIZE determines maximum size in bytes for constant buffer.
RESID determines resource id.

* for global or constant pointers is UAVID, range is in 8-1023.
* for constant pointers (driver older than 1348.X), range is in 1-159.
* for read only images range is in 0-127.
* For write only images or counters range is in 0-7.

The last argument `unused` indicates that argument will not be used by kernel.

Sample usage:

```
.arg v1,"double_t",double
.arg v2,double2
.arg v3,double3
.arg v23,image2d,
.arg v30,image2d,,5
.arg v41,ulong16  *,global
.arg v42,ulong16  *,global, restrict
.arg v57,structure*,82,global
```

### .boolconsts

This pseudo-operation must be inside kernel.
Open ATI_BOOL32CONSTS CAL note. Next occurrence in this same kernel, add new CAL note.

### .calnote

Syntax: .calnote CALNOTEID

This pseudo-operation must be inside kernel. Open ATI CAL note.

### .cbid

Syntax: .cbid
Syntax: .cbid VALUE

If this pseudo-operation inside ATI_CONSTANT_BUFFERS CAL note then
it adds entry into ATI_CONSTANT_BUFFERS CAL note.
If this pseudo-operation in kernel configuration then set constant buffer id.

### .cbmask

Syntax: .cbmask INDEX, SIZE

This pseudo-operation must be in ATI_CONSTANT_BUFFERS CAL note.
Add entry into ATI_CONSTANT_BUFFERS CAL note.

### .compile_options

Syntax: .compile_options "STRING"

Set compile options for this binary.

### .condout

Syntax: .condout [VALUE]  
Syntax: .condout VALUE

If this pseudo-operation inside kernel then it open ATI_CONDOUT CAL note.
Next occurrence in this same kernel, add new CAL note.
Optional argument add 4-byte value to content of this CAL note.
If this pseudo-operation in kernel configuration then set CONDOUT value.

### .config

Open kernel configuration.

### .constantbuffers

This pseudo-operation must be inside kernel.
Open ATI_CONSTANT_BUFFERS CAL note. Next occurrence in this same kernel,
add new CAL note.

### .cws

Syntax: .cws SIZEHINT[, SIZEHINT[, SIZEHINT]]

This pseudo-operation must be inside kernel configuration.
Set reqd_work_group_size hint for this kernel.

### .dims

Syntax: .dims DIMENSIONS

This pseudo-operation must be inside kernel configuration. Defines what dimensions
(from list: x, y, z) will be used to determine space of the kernel execution.

### .driver_info

Syntax: .driver_info "INFO"

Set driver info for this binary.

### .driver_version

Syntax: .driver_version VERSION

Set driver version for this binary. Version in form: MajorVersion*100+MinorVersion.

### .earlyexit

Syntax: .earlyexit [VALUE]  
Syntax: .earlyexit VALUE

If this pseudo-operation inside kernel then it open ATI_EARLY_EXIT CAL note.
Next occurrence in this same kernel, add new CAL note.
Optional argument add 4-byte value to content of this CAL note.
If this pseudo-operation in kernel configuration then set EARLY_EXIT value.

### .entry

Syntax: .entry UAVID, F1, F2, TYPE  
Syntax: .entry VALUE1, VALUE2

This pseudo-operation must be in ATI_UAV or ATI_PROGINFO CAL note.
Add entry into CAL note. For ATI_UAV, pseudo-operation accepts 4 32-bit values.
For ATI_PROGINFO, accepts 2 32-bit values.

### .floatconsts

This pseudo-operation must be inside kernel.
Open ATI_FLOATCONSTS CAL note. Next occurrence in this same kernel, add new CAL note.

### .floatmode

Syntax: .floatmode VALUE

This pseudo-operation must be inside kernel configuration.
Set floatmode. Value shall to be byte value.

### .globalbuffers

This pseudo-operation must be inside kernel.
Open ATI_GLOBAL_BUFFERS CAL note. Next occurrence in this same kernel, add new CAL note.

### .globaldata

Go to constant global data section.