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

## Relocations

A CLRX assembler handles relocations to scratch symbol (`.scratchsym` pseudo-op).
These relocations can be applied to places that accepts
32-bit literal immediates. Only two types of relocations is allowed:

* `place`, `place&0xffffffff`, `place%0x10000000`, `place%%0x10000000` -
low 32 bits of value
* `place>>32`, `place/0x100000000`, `place//0x100000000` - high 32 bits of value

The `place` indicates an expression with scratch symbol. Additional offsets
are not accepted (only same scratch symbol).

Examples:

```
s_mov_b32       s13, scratchsym>>32
s_mov_b32       s12, scratchsym&0xffffffff
```

## Register usage setup

The CLRX assembler automatically sets number of used VGPRs and number of used SGPRs.
This setup can be replaced by pseudo-ops '.sgprsnum' and '.vgprsnum'.

## Scalar register allocation

Assembler for GalliumCompute format counts all SGPR registers and add extra registers
(VCC, FLAT_SCRATCH, XNACK_MASK) if any used to register pool.
 The VCC register is included by default.
In AMDHSA configuration (LLVM >= 4.0.0) then special fields determines
what extra SGPR registers (FLAT_SCRATCH, VCC and XNACK_MASK) has been added.

The `.sgprsnum` set number of all SGPRs including VCC, FLAT_SCRATCH and XNACK_MASK.

## List of the specific pseudo-operations

### .arch_minor

Syntax: .arch_minor ARCH_MINOR

Set architecture minor number. Used only if LLVM version is 4.0.0 or later.

### .arch_stepping

Syntax: .arch_minor ARCH_STEPPING

Set architecture stepping number. Used only if LLVM version is 4.0.0 or later.

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

### .call_convention

Syntax: .call_convention CALL_CONV

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set call convention for kernel.

### .codeversion

Syntax .codeversion MAJOR, MINOR

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set AMD code version.

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
* .spillesgprs - number of scalar registers to spill
* .spillevgprs - number of vector registers to spill
* AMDHSA pseudo-ops

Example configuration:

```
.config
    .dims xyz
    .tgsize
```

### .control_directive

Open control directive section. This section must be 128 bytes. The content of this
section will be stored in control_directive field in kernel configuration.
Must be defined inside kernel. Can ben used only if LLVM version is 4.0.0 or later

### .debug_private_segment_buffer_sgpr

Syntax: .debug_private_segment_buffer_sgpr SGPRREG

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `debug_private_segment_buffer_sgpr` field in
kernel configuration.

### .debug_wavefront_private_segment_offset_sgpr

Syntax: .debug_wavefront_private_segment_offset_sgpr SGPRREG

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `debug_wavefront_private_segment_offset_sgpr` field in
kernel configuration.

### .debugmode

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the DEBUG_MODE.

### .default_hsa_features

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. It sets default HSA kernel features and register features
(extra SGPR registers usage). These default features are `.use_private_segment_buffer`,
`.use_dispatch_ptr`, `.use_kernarg_segment_ptr`, `.use_ptr64` and
private_elem_size to 4 bytes.

### .dims

Syntax: .dims DIMENSIONS  
Syntax: .dims GID_DIMS, LID_DIMS

This pseudo-op must be inside kernel configuration (`.config`). Define what dimensions
(from list: x, y, z) will be used to determine space of the kernel execution.
In second syntax form, the dimensions are given for group_id (GID_DIMS) and for local_id
(LID_DIMS) separately.

### .driver_version

Syntax: .driver_version VERSION

Set driver (Mesa3D) version for this binary. Version in form: MajorVersion*100+MinorVersion.
This pseudo-op replaces driver info.

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

This pseudo-op must be inside kernel configuration (`.config`). Define float-mode.
Set floatmode (FP_ROUND and FP_DENORM fields of the MODE register). Default value is 0xc0.

### .gds_segment_size

Syntax: .gds_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `gds_segment_size` field in kernel configuration.

### .get_driver_version

Syntax: .get_driver_version SYMBOL

Store current driver version to SYMBOL. Version in form:
`major_version*10000 + minor_version*100 + micro_version`.

### .get_llvm_version

Syntax: .get_llvm_version SYMBOL

Store current LLVM compiler version to SYMBOL. Version in form:
`major_version*10000 + minor_version*100 + micro_version`.

### .globaldata

Go to constant global data section (`.rodata`).

### .group_segment_align

Syntax: .group_segment_align ALIGN

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `group_segment_align` field in kernel configuration.

### .hsa_debugmode

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable usage of the DEBUG_MODE in kernel HSA configuration.

### .hsa_dims

Syntax: .hsa_dims DIMENSIONS

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Define what dimensions (from list: x, y, z) will be used
to determine space of the kernel execution in kernel HSA configuration.

### .hsa_dx10clamp

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable usage of the DX10_CLAMP in kernel HSA configuration.

### .hsa_exceptions

Syntax: .hsa_exceptions EXCPMASK

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set exception mask in PGMRSRC2 register value in
kernel HSA configuration. Value should be 7-bit.

### .hsa_floatmode

Syntax: .hsa_floatmode BYTE-VALUE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Define float-mode in kernel HSA configuration.
Set floatmode (FP_ROUND and FP_DENORM fields of the MODE register). Default value is 0xc0.

### .hsa_ieeemode

Syntax: .hsa_ieeemode

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set ieee-mode in kernel HSA configuration.

### .hsa_localsize

Syntax: .hsa_localsize SIZE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Define initial local memory size used by kernel in
kernel HSA configuration.

### .hsa_pgmrsrc1

Syntax: .hsa_pgmrsrc1 VALUE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Define value of the PGMRSRC1 in kernel HSA configuration.

### .hsa_pgmrsrc2

Syntax: .hsa_pgmrsrc2 VALUE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Define value of the PGMRSRC2 in kernel HSA configration.
If dimensions is set then bits that controls dimension setup will be ignored.
SCRATCH_EN bit will be ignored.

### .hsa_priority

Syntax: .hsa_priority PRIORITY

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Define priority (0-3) in kernel HSA configuration.

### .hsa_privmode

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable usage of the PRIV (privileged mode) in
kernel HSA configuration.

### .hsa_scratchbuffer

Syntax: .hsa_scratchbuffer SIZE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Define scratchbuffer size in kernel HSA configuration.

### .hsa_sgprsnum

Syntax: .hsa_sgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set number of scalar registers which can be used during
kernel execution in kernel HSA configuration.

### .hsa_tgsize

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable usage of the TG_SIZE_EN in kernel HSA configuration.

### .hsa_userdatanum

Syntax: .hsa_userdatanum NUMBER

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set number of registers for USERDATA in
kernel HSA configuration.

### .hsa_vgprsnum

Syntax: .hsa_vgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`) can ben used only if
LLVM version is 4.0.0 or later. Set number of vector registers which can be used during
kernel execution in kernel HSA configuration.

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

### .kernarg_segment_align

Syntax: .kernarg_segment_align ALIGN

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `kernarg_segment_alignment` field in
kernel configuration. Value must be a power of two.

### .kernarg_segment_size

Syntax: .kernarg_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `kernarg_segment_byte_size` field in
kernel configuration.

### .kernel_code_entry_offset

Syntax: .kernel_code_entry_offset OFFSET

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `kernel_code_entry_byte_offset` field in
kernel configuration. This field store offset between configuration and kernel code.
By default is 256.

### .kernel_code_prefetch_offset

Syntax: .kernel_code_prefetch_offset OFFSET

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `kernel_code_prefetch_byte_offset` field in kernel
configuration.

### .kernel_code_prefetch_size

Syntax: .kernel_code_prefetch_size OFFSET

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `kernel_code_prefetch_byte_size` field in kernel configuration.

### .llvm_version

Syntax: .llvm_version VERSION

Set LLVM compiler version for this binary. Version in form: MajorVersion*100+MinorVersion.
This pseudo-op replaces driver info.

### .localsize

Syntax: .localsize SIZE

This pseudo-op must be inside kernel configuration (`.config`). Define initial
local memory size used by kernel.

### .machine

Syntax: .machine KIND, MAJOR, MINOR, STEPPING

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set machine version fields in kernel configuration.

### .max_scratch_backing_memory

Syntax: .max_scratch_backing_memory SIZE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `max_scratch_backing_memory_byte_size` field
in kernel configuration.

### .pgmrsrc1

Syntax: .pgmrsrc1 VALUE

This pseudo-op must be inside kernel configuration (`.config`).
Define value of the PGMRSRC1.

### .pgmrsrc2

Syntax: .pgmrsrc2 VALUE

This pseudo-op must be inside kernel configuration (`.config`).
Define value of the PGMRSRC2. If dimensions is set then bits that controls dimension setup
will be ignored. SCRATCH_EN bit will be ignored.

### .priority

Syntax: .priority PRIORITY

This pseudo-op must be inside kernel configuration (`.config`). Define priority (0-3).

### .private_elem_size

Syntax: .private_elem_size ELEMSIZE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `private_element_size` field in kernel configuration.
Must be a power of two between 2 and 16.

### .private_segment_align

Syntax: .private_segment ALIGN

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `private_segment_alignment` field in kernel
configuration. Value must be a power of two.

### .privmode

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the PRIV (privileged mode).

### .proginfo

Open progInfo definition. Must be inside kernel.
ProgInfo shall to be containing 3 entries. ProgInfo can not be defined if kernel config
was defined (by using `.config`).

### .reserved_sgprs

Syntax: .reserved_sgprs FIRSTREG, LASTREG

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `reserved_sgpr_first` and `reserved_sgpr_count`
fields in kernel configuration. `reserved_sgpr_count` filled by number of registers
(LASTREG-FIRSTREG+1).

### .reserved_vgprs

Syntax: .reserved_vgprs FIRSTREG, LASTREG

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `reserved_vgpr_first` and `reserved_vgpr_count`
fields in kernel configuration. `reserved_vgpr_count` filled by number of registers
(LASTREG-FIRSTREG+1).

### .runtime_loader_kernel_symbol

Syntax: .runtime_loader_kernel_symbol ADDRESS

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `runtime_loader_kernel_symbol` field in kernel
configuration.

### .scratchbuffer

Syntax: .scratchbuffer SIZE

This pseudo-op must be inside kernel configuration (`.config`). Define scratchbuffer size.

### .scratchsym

Syntax: .scratchsym SYMBOL

Set symbol as scratch symbol. This symbol points to scratch buffer offset an will be used
while generating scratch buffer relocations.

### .sgprsnum

Syntax: .sgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of scalar
registers which can be used during kernel execution.
It counts SGPR registers including VCC, FLAT_SCRATCH and XNACK_MASK.

### .spilledsgprs

Syntax: .spilledsgprs REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of scalar
registers to spill in scratch buffer. It have meaning for LLVM 3.9 or later.

### .spilledvgprs

Syntax: .spilledvgprs REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of vector
registers to spill in scratch buffer. It have meaning for LLVM 3.9 or later. 

### .tgsize

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the TG_SIZE_EN. Should be set.

### .use_debug_enabled

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `is_debug_enabled` field in kernel configuration.

### .use_dispatch_id

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_sgpr_dispatch_id` field in kernel
configuration.

### .use_dispatch_ptr

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_sgpr_dispatch_ptr` field in kernel
configuration.

### .use_dynamic_call_stack

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `is_dynamic_call_stack` field in
kernel configuration.

### .use_flat_scratch_init

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_sgpr_flat_scratch_init` field in
kernel configuration.

### .use_grid_workgroup_count

Syntax: .use_grid_workgroup_count DIMENSIONS

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_sgpr_grid_workgroup_count_X`,
`enable_sgpr_grid_workgroup_count_Y` and `enable_sgpr_grid_workgroup_count_Z` fields
in kernel configuration, respectively by given dimensions.

### .use_kernarg_segment_ptr

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_sgpr_kernarg_segment_ptr` field in
kernel configuration.

### .use_ordered_append_gds

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_ordered_append_gds` field in
kernel configuration.

### .use_private_segment_buffer

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_sgpr_private_segment_buffer` field in
kernel configuration.

### .use_private_segment_size

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_sgpr_private_segment_size` field in
kernel configuration.

### .use_ptr64

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `is_ptr64` field in kernel configuration.

### .use_queue_ptr

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `enable_sgpr_queue_ptr` field in
kernel configuration.

### .use_xnack_enabled

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Enable `is_xnack_enabled` field in kernel configuration.

### .userdatanum

Syntax: .userdatanum NUMBER

This pseudo-op must be inside kernel configuration (`.config`). Set number of
registers for USERDATA.

### .vgprsnum

Syntax: .vgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of vector
registers which can be used during kernel execution.

### .wavefront_sgpr_count

Syntax: .wavefront_sgpr_count REGNUM

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `wavefront_sgpr_count` field in kernel configuration.

### .wavefront_size

Syntax: .wavefront_size POWEROFTWO

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `wavefront_size` field in kernel configuration.
Value must be a power of two.

### .workgroup_fbarrier_count

Syntax: .workgroup_fbarrier_count COUNT

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `workgroup_fbarrier_count` field in
kernel configuration.

### .workgroup_group_segment_size

Syntax: .workgroup_group_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `workgroup_group_segment_byte_size` in
kernel configuration.

### .workitem_private_segment_size

Syntax: .workitem_private_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `workitem_private_segment_byte_size` field in
kernel configuration.

### .workitem_vgpr_count

Syntax: .workitem_vgpr_count REGNUM

This pseudo-op must be inside kernel configuration (`.config`) and can ben used only if
LLVM version is 4.0.0 or later. Set `workitem_vgpr_count` field in kernel configuration.


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

This sample for new Gallium format (LLVM>=4.0, Mesa>=17.0.0) with two kernels:

```
.gallium
.llvm_version 40000         # set LLVM version 4.0.0
.driver_version 170000      # set Mesa version 17.0.0
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
    .config
        .dims xyz
        .tgsize
.kernel DCT2
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
.text
DCT:
.skip 256   # skip HSA configuration
/*c0030106         */ s_load_dword    s6, s[0:1], 0x6
/*c0038107         */ s_load_dword    s7, s[0:1], 0x7
/* we skip rest of instruction to demonstrate how to write GalliumCompute program */
/*bf810000         */ s_endpgm
.p2align 8        # important alignment to 256-byte boundary
DCT2:
.skip 256   # skip HSA configuration
/*c0030106         */ s_load_dword    s6, s[0:1], 0x6
/*c0038107         */ s_load_dword    s7, s[0:1], 0x7
/* we skip rest of instruction to demonstrate how to write GalliumCompute program */
/*bf810000         */ s_endpgm
```
