## CLRadeonExtender Assembler Gallium handling

The GalliumCompute is an open-source the OpenCL implementation for the Mesa3D
drivers. It divided into three components: CLover, libclc, LLVM AMDGPU. Since LLVM v3.6
and Mesa3D v10.5, GalliumCompute binary format with native code. CLRadeonExtender
supports only these binaries.

## Binary format

The binary format contains: kernel informations and the main binary in the ELF format.
Main `.text` section contains all code for all kernels. Optionally,
section `.rodata` contains constant global data for all kernels.
Main binary have the kernel configuration (ProgInfo) in the `.AMDGPU.config` section.
ProgInfo holds three addresses and values that describes runtime environment for kernel:
floating point setup, register usage, local data usage and rest.

The assembler source code divided to three parts:

* kernel configuration
* kernel constant data (in `.rodata` section)
* kernel code (in `.text` section)

Order of these parts doesn't matter.

Kernel function should to be aligned to 256 byte boundary.

## Scalar register allocation

Assembler for GalliumCompute format counts all SGPR registers and add extra registers
(VCC, FLAT_SCRATCH, XNACK_MASK) if any used to register pool. By default no extra register
is added.

## List of the specific pseudo-operations

### .arg

Syntax: .arg ARGTYPE, SIZE[, TARGETSIZE[, ALIGNMENT[, NUMEXT[, SEMANTIC]]]]

Adds kernel argument definition. Must be inside argument configuration.
First argument is type:

* scalar - scalar value (including vector values likes uint4)
* contant - constant pointer (32-bit ???)
* global - global pointer (64-bit)
* local - local pointer
* image2d_rdonly - ??
* image2d_wronly - ??
* image3d_rdonly - ??
* image3d_wronly - ??
* sampler - ??
* griddim - shortcut for griddim argument definition
* gridoffset - shortcut for gridoffset argument definition

Second argument is size of argument. Third argument is targetSize which
should be a multiplier of 4. Fourth argument is target alignment. By default target
alignment is power of 2 not less than size.
Fifth argument determines how extend numeric value to larger target size:
`sext` - signed, `zext` - zero extend. If argument is smaller than 4 byte,
then `sext` can be to define signed integer, `zext` to unsigned integer.
Sixth argument is semantic:

* general - general argument
* griddim - griddim argument
* gridoffset - gridoffset argument
* imgsize - image size
* imgformat - image format

Example argument definition:

```
.arg scalar, 4, 4, 4, zext, general
.arg global, 8, 8, 8, zext, general
.arg scalar, 2, 4, 4, sext, general # short
.arg scalar, 16, 16, 16, zext, general # uint4 or double2
.arg scalar, 4, 4, 4, zext, griddim # shortcut: .arg griddim
.arg scalar, 4, 4, 4, zext, gridoffset # shortcut .arg gridoffset
```

Last two arguments (griddim, gridoffset) shall to be defined in any kernel definition.

### .args

Open kernel argument configuration. Must be inside kernel.

### .config

Open kernel configuration. Must be inside kernel. Kernel configuration can not be
defined if proginfo configuration was defined (by using `.proginfo`).
Following pseudo-ops can be inside kernel config:

* .debugmode - enables using of DEBUG_MODE
* .dims DIMS - choose dimensions used by kernel function. Can be: x,y,z.
* .dx10clamp - enables using of DX10_CLAMP
* .floatmode VALUE - choose float mode for kernel (byte value).
Default value is 0xc0
* .ieeemode - choose IEEE mode for kernel
* .localsize SIZE - initial local data size for kernel in bytes
* .pgmrsrc2 VALUE - value of the PGMRSRC2 (only bits that is not set by other pseudo-ops)
* .priority VALUE - set priority for kernel (0-3). Default value is 0.
* .privmode - enables using of PRIV (privileged mode)
* .scratchbuffer SIZE - size of scratchbuffer (???). Default value is 0.
* .sgprsnum NUMBER - number of SGPR registers used by kernel (excluding VCC,FLAT_SCRATCH).
By default, automatically computed by assembler.
* .vgprsnum NUMBER - number of VGPR registers used by kernel.
By default, automatically computed by assembler.
* .userdatanum NUMBER - number of USERDATA used by kernel (0-16). Default value is 4.
* .tgsize - enables using of TG_SIZE_EN (we recommend to add this always)

Example configuration:

```
.config
    .dims xyz
    .tgsize
```

### .debugmode

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the DEBUG_MODE.

### .dims

Syntax: .dims DIMENSIONS

This pseudo-op must be inside kernel configuration (`.config`). Defines what dimensions
(from list: x, y, z) will be used to determine space of the kernel execution.

### .dx10clamp

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the DX10_CLAMP.

### .entry

Syntax: .entry ADDRESS, VALUE

Add entry of proginfo. Must be inside proginfo configuration. Sample proginfo:

```
.entry 0x0000b848, 0x000c0080
.entry 0x0000b84c, 0x00001788
.entry 0x0000b860, 0x00000000
```

### .exceptions

Syntax: .exceptions EXCPMASK

This pseudo-op must be inside kernel configuration (`.config`).
Set exception mask in PGMRSRC2 register value. Value should be 7-bit.

### .floatmode

Syntax: .floatmode BYTE-VALUE

This pseudo-op must be inside kernel configuration (`.config`). Defines float-mode.
Set floatmode (FP_ROUND and FP_DENORM fields of the MODE register). Default value is 0xc0.

### .globaldata

Go to constant global data section (`.rodata`).

### .ieeemode

Syntax: .ieeemode

This pseudo-op must be inside kernel configuration (`.config`). Set ieee-mode.

### .kcode

Syntax: .kcode KERNEL1,....  
Syntax: .kcode +

Open code that will be belonging to specified kernels. By default any code between
two consecutive kernel labels belongs to the kernel with first label name.
This pseudo-operation can change membership of the code to specified kernels.
You can nest this `.kcode` any times. Just next .kcode adds or remove membership code
to kernels. The most important reason why this feature has been added is register usage
calculation. Any kernel given in this pseudo-operation must be already defined.

Sample usage:

```
.kcode + # this code belongs to all kernels
.kcodeend
.kcode kernel1, kernel2 #  this code belongs to kernel1, kernel2
    .kcode -kernel1 #  this code belongs only to kernel2 (kernel1 removed)
    .kcodeend
.kcodeend
```

### .kcodeend

Close `.kcode` clause. Refer to `.kcode`.

### .localsize

Syntax: .localsize SIZE

This pseudo-op must be inside kernel configuration (`.config`). Defines initial
local memory size used by kernel.


### .pgmrsrc1

Syntax: .pgmrsrc1 VALUE

This pseudo-op must be inside kernel configuration (`.config`).
Defines value of the PGMRSRC1.

### .pgmrsrc2

Syntax: .pgmrsrc2 VALUE

This pseudo-op must be inside kernel configuration (`.config`).
Defines value of the PGMRSRC2. If dimensions is set then bits that controls dimension setup
will be ignored. SCRATCH_EN bit will be ignored.

### .priority

Syntax: .priority PRIORITY

This pseudo-op must be inside kernel configuration (`.config`). Defines priority (0-3).

### .privmode

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the PRIV (privileged mode).

### .proginfo

Open progInfo definition. Must be inside kernel.
ProgInfo shall to be containing 3 entries. ProgInfo can not be defined if kernel config
was defined (by using `.config`).

### .scratchbuffer

Syntax: .scratchbuffer SIZE

This pseudo-op must be inside kernel configuration (`.config`). Defines scratchbuffer size.

### .sgprsnum

Syntax: .sgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of scalar
registers which can be used during kernel execution.

### .tgsize

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the TG_SIZE_EN. Should be set.

### .userdatanum

Syntax: .userdatanum NUMBER

This pseudo-op must be inside kernel configuration (`.config`). Set number of
registers for USERDATA.

### .vgprsnum

Syntax: .vgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of vector
registers which can be used during kernel execution.

## Sample code

This is sample example of the kernel setup:

```
.kernel DCT
    .args
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg local, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, griddim
        .arg scalar, 4, 4, 4, zext, gridoffset
    .proginfo
        .entry 0x0000b848, 0x000c0183
        .entry 0x0000b84c, 0x00001788
        .entry 0x0000b860, 0x00000000
```

with kernel configuration:

```
    .args
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg local, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, griddim
        .arg scalar, 4, 4, 4, zext, gridoffset
    .config
        .dims xyz
        .tgsize
```

All code:

```
.gallium
.gpu CapeVerde
.kernel DCT
    .args
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg local, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, general
        .arg scalar, 4, 4, 4, zext, griddim
        .arg scalar, 4, 4, 4, zext, gridoffset
    .proginfo
        .entry 0x0000b848, 0x000c0183
        .entry 0x0000b84c, 0x00001788
        .entry 0x0000b860, 0x00000000
.text
DCT:
/*c0030106         */ s_load_dword    s6, s[0:1], 0x6
/*c0038107         */ s_load_dword    s7, s[0:1], 0x7
/* we skip rest of instruction to demonstrate how to write GalliumCompute program */
/*bf810000         */ s_endpgm
```
