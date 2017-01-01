## CLRadeonExtender Assembler AMD Catalyst OpenCL 2.0 handling

The AMD Catalyst driver provides own OpenCL implementation that can generates
own binaries of the OpenCL programs. The CLRX assembler supports both OpenCL 1.2
and OpenCL 2.0 binary format. This chapter describes Amd OpenCL 2.0 binary format.

## Binary format

An AMD Catalyst binary format for OpenCL 2.0 support significantly differs from
prevbious binary format for OpenCL 1.2. The Kernel codes are in single text inner binary.
Instead of AMD CAL notes and ProgInfo entries, the kernel setup is in special
format structure. Metadatas mainly holds arguments definitions of kernels.

A CLRadeonExtender supports two versions of binary formats for OpenCL 2.0: newer (since 
AMD OpenCL 1912.05) and older (before 1912.05 driver version).

Special section to define global data for all kernels:

* `rodata`, `.globaldata` - read-only constant (global) data
* `.rwdata`, `.data` - read-write global data
* `.bss`, `.bssdata` - allocatable read-write data

## Relocations

An CLRX assembler handles relocations to symbol at global data, global rwdata and
global bss data in kernel code. These relocations can be applied to places that accepts
32-bit literal immediates. Only two types of relocations is allowed:

* `place`, `place&0xffffffff`, `place%0x10000000`, `place%%0x10000000` -
low 32 bits of value
* `place>>32`, `place/0x100000000`, `place//0x100000000` - high 32 bits of value

The `place` indicates an expression that result points to some place in one of
allowed sections.

Examples:

```
s_mov_b32       s13, (gdata+152)>>32
s_mov_b32       s12, (gdata+152)&0xffffffff
s_mov_b32       s15, (gdata+160)>>32
s_mov_b32       s14, (gdata+160)&0xffffffff
```

## Layout of the source code

The CLRX assembler allow to use one of two ways to configure kernel setup:
for human (`.config`) and for quick recompilation (kernel setup, stub, metadata content).

## Scalar register allocation

Depend on configuration options, an assembler add VCC and FLAT_SCRATCH
(if `.useenqueue` or `.usegeneric` enabled).

## List of the specific pseudo-operations

### .acl_version

Syntax: .acl_version "STRING"

Set ACL version string.

### .arch_minor

Syntax: .arch_minor ARCH_MINOR

Set architecture minor number.

### .arch_stepping

Syntax: .arch_minor ARCH_STEPPING

Set architecture stepping number.

### .arg

Syntax for scalar: .arg ARGNAME \[, "ARGTYPENAME"], ARGTYPE[, unused]  
Syntax for structure: .arg ARGNAME, \[, "ARGTYPENAME"], ARGTYPE[, STRUCTSIZE[, unused]]  
Syntax for image: .arg ARGNAME\[, "ARGTYPENAME"], ARGTYPE[, [ACCESS] [, RESID[, unused]]]  
Syntax for sampler: .arg ARGNAME\[, "ARGTYPENAME"], ARGTYPE[, RESID[, unused]]  
Syntax for global pointer: .arg ARGNAME\[, "ARGTYPENAME"], 
ARGTYPE\[\[, STRUCTSIZE], PTRSPACE[, [ACCESS] [, unused]]]  
Syntax for local pointer: .arg ARGNAME\[, "ARGTYPENAME"], 
ARGTYPE\[\[, STRUCTSIZE], PTRSPACE[, [ACCESS] [, unused]]]  
Syntax for constant pointer: .arg ARGNAME\[, "ARGTYPENAME"], 
ARGTYPE\[\[, STRUCTSIZE], PTRSPACE\[, [ACCESS] [, [CONSTSIZE] [, unused]]]

Adds kernel argument definition. Must be inside kernel configuration. First argument is
argument name from OpenCL kernel definition. Next optional argument is argument type name
from OpenCL kernel definition. Next arugment is argument type:

* char, uchar, short, ushort, int, uint, ulong, long, float, double - simple scalar types
* charX, ucharX, shortX, ushortX, intX, uintX, ulongX, longX, floatX, doubleX - vector types
(X indicates number of elements: 2, 3, 4, 8 or 16)
* structure - structure
* image, image1d, image1d_array, image1d_buffer, image2d, image2d_array, image3d -
image types
* sampler - sampler
* queue - command queue
* clkevent - clkevent
* type* - pointer to data

Rest of the argument depends on type of the kernel argument. STRUCTSIZE determines size of
structure. ACCESS for image determines can be one of the: `read_only`, `rdonly` or
`write_only`, `wronly`.
PTRSPACE determines space where pointer points to.
It can be one of: `local`, `constant` or `global`.
ACCESS for pointers can be: `const`, `restrict` and `volatile`.
CONSTSIZE determines maximum size in bytes for constant buffer.
RESID determines resource id (only for samplers and images).

* for read only images range is in 0-127.
* for other images is in 0-63.
* for samplers is in 0-15.

The last argument `unused` indicates that argument will not be used by kernel. In this
argument can be given 'rdonly' (argument used for read-only) and 'wronly'
(argument used for write-only).

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

### .bssdata

Syntax: .bssdata [align=ALIGNMENT]

Go to global data bss section. Optional argument sets alignment of section.

### .config

Open kernel configuration. Must be inside kernel. Kernel configuration can not be
defined if any isametadata, metadata or stub was defined.
Following pseudo-ops can be inside kernel config:

* .arg
* .cws
* .debugmode
* .dims
* .dx10clamp
* .exceptions
* .localsize
* .ieeemode
* .pgmrsrc1
* .pgmrsrc2
* .priority
* .privmode
* .sampler
* .scratchbuffer
* .setupargs
* .sgprsnum
* .tgsize
* .uavid
* .useargs
* .useenqueue
* .usegeneric
* .usesetup
* .vgprsnum

### .cws

Syntax: .cws SIZEHINT[, SIZEHINT[, SIZEHINT]]

This pseudo-operation must be inside kernel configuration.
Set reqd_work_group_size hint for this kernel.

### .debugmode

This pseudo-operation must be inside kernel configuration.
Enable usage of the DEBUG_MODE.

### .dims

Syntax: .dims DIMENSIONS

This pseudo-operation must be inside kernel configuration. Defines what dimensions
(from list: x, y, z) will be used to determine space of the kernel execution.

### .driver_version

Syntax: .driver_version VERSION

Set driver version for this binary. Version in form: MajorVersion*100+MinorVersion.
This pseudo-op replaces driver info.

### .dx10clamp

This pseudo-operation must be inside kernel configuration.
Enable usage of the DX10_CLAMP.

### .exceptions

Syntax: .exceptions EXCPMASK

This pseudo-operation must be inside kernel configuration.
Set exception mask in PGMRSRC2 register value. Value should be 7-bit.

### .get_driver_version

Syntax: .get_driver_version SYMBOL

Store current driver version to SYMBOL.

### .globaldata

Go to constant global data section.

### .ieeemode

This pseudo-op must be inside kernel configuration. Set ieee-mode.

### .inner

Go to inner binary place. By default assembler is in main binary.

### .isametadata

This pseudo-operation must be inside kernel. Go to ISA metadata content
(only older driver binaries).

### .localsize

Syntax: .localsize SIZE

This pseudo-operation must be inside kernel configuration. Set the initial
local data size.

### .metadata

This pseudo-operation must be inside kernel. Go to metadata content.

### .pgmrsrc1

Syntax: .pgmrsrc1 VALUE

This pseudo-operation must be inside kernel.
Defines value of the PGMRSRC1.


### .pgmrsrc2

Syntax: .pgmrsrc2 VALUE

This pseudo-operation must be inside kernel configuration. Set PGMRSRC2 value.
If dimensions is set then bits that controls dimension setup will be ignored.
SCRATCH_EN bit will be ignored.

### .priority

Syntax: .priority PRIORITY

This pseudo-operation must be inside kernel. Defines priority (0-3).

### .privmode

This pseudo-operation must be inside kernel.
Enable usage of the PRIV (privileged mode).

### .rwdata

Go to read-write global data section.

### .sampler

Syntax: .sampler VALUE,...

Inside main and inner binary: add sampler definitions.
Only legal when no samplerinit section. Inside kernel configuration:
add samplers to kernel (values are sampler ids).

### .samplerinit

Go to samplerinit content section. Only legal if no sampler definitions.

### .samplerreloc

Syntax: .samplerreloc OFFSET, SAMPLERID

Add sampler relocation that points to constant global data (rodata).

### .scratchbuffer

Syntax: .scratchbuffer SIZE

This pseudo-operation must be inside kernel configuration.
Set scratchbuffer size.

### .setup

Go to kernel setup content section.

### .setupargs

This pseudo-op must be inside kernel configuration. Add first kernel setup arguments.
This pseudo-op must be before any other arguments.

### .sgprsnum

Syntax: .sgprsnum REGNUM

This pseudo-op must be inside kernel configuration. Set number of scalar
registers which can be used during kernel execution.

### .stub

Go to kernel stub content section. Only allowed for older driver version binaries.

### .tgsize

This pseudo-op must be inside kernel configuration.
Enable usage of the TG_SIZE_EN.

### .useargs

This pseudo-op must be inside kernel configuration. Indicate that kernel uses arguments.

### .useenqueue

This pseudo-op must be inside kernel configuration. Indicate that kernel uses
enqueue mechanism.

### .usegeneric

This pseudo-op must be inside kernel configuration. Indicate that kernel uses
generic pointers mechanism (FLAT instructions).

### .usesetup

This pseudo-op must be inside kernel configuration. Indicate that kernel uses
setup data (global sizes, local sizes, work groups num).

### .vgprsnum

Syntax: .vgprsnum REGNUM

This pseudo-op must be inside kernel configuration. Set number of vector
registers which can be used during kernel execution.

## Sample code

This is sample example of the kernel setup:

```
.amdcl2
.64bit
.gpu Bonaire
.driver_version 191205
.compile_options "-I ./ -cl-std=CL2.0"
.acl_version "AMD-COMP-LIB-v0.8 (0.0.SC_BUILD_NUMBER)"
.kernel DCT
    .metadata
        .byte 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        ...,
    .setup
        .byte 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
        .byte 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        ....
    .text
/*c0000501         */ s_load_dword    s0, s[4:5], 0x1
....
/*bf810000         */ s_endpgm
```

This is sample of the kernel with configuration:

```
.amdcl2
.64bit
.gpu Bonaire
.driver_version 191205
.compile_options "-I ./ -cl-std=CL2.0"
.acl_version "AMD-COMP-LIB-v0.8 (0.0.SC_BUILD_NUMBER)"
.kernel DCT
    .config
        .dims xy
        .useargs
        .usesetup
        .setupargs
        .arg output,float*
        .arg input,float*
        .arg dct8x8,float*
        .arg dct8x8_trans,float*
        .arg inter,float*,local
        .arg width,uint
        .arg blockWidth,uint
        .arg inverse,uint
        .......
    .text
/*c0000501         */ s_load_dword    s0, s[4:5], 0x1
....
/*bf810000         */ s_endpgm
```
