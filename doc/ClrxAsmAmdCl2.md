## CLRadeonExtender Assembler AMD Catalyst OpenCL 2.0 handling

The AMD Catalyst driver provides own OpenCL implementation that can generates
own binaries of the OpenCL programs. The CLRX assembler supports both OpenCL 1.2
and OpenCL 2.0 binary format. This chapter describes Amd OpenCL 2.0 binary format.
The first Catalyst drivers uses this format for OpenCL 2.0 programs.
Current AMD drivers uses this format for OpenCL 1.2 and OpenCL 2.0 programs for
GCN 1.1 and later architectures.

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

A CLRX assembler handles relocations to symbol at global data, global rwdata and
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

In the HSA layout mode the kernel codes in single main code section and no code section
for each kernel. The HSA layout mode can be enabled only for newer binary format.

## Register usage setup

The CLRX assembler automatically sets number of used VGPRs and number of used SGPRs.
This setup can be replaced by pseudo-ops '.sgprsnum' and '.vgprsnum'.

## Scalar register allocation

Depend on configuration options, an assembler add VCC, FLAT_SCRATCH and XNACK_MASK
(if `.useenqueue` or `.usegeneric` enabled).
In HSA configuration mode, a special fields determines
what extra SGPR registers (FLAT_SCRATCH, VCC and XNACK_MASK) has been added.

While using HSA kernel configuration (`.hsaconfig`) the `.sgprsnum` set number of all SGPRs
including VCC, FLAT_SCRATCH and XNACK_MASK.
While using kernel configuration (`.config`) the `.sgprsnum` set number of all SGPRs
except VCC and FLAT_SCRATCH and XNACK_MASK (rule from AMD binary format support).
Since CLRX policy 0.2 (200) a `.sgprsnum` set number of all SGPRs including
VCC, FLAT_SCRATCH and XNACK_MASK (extra registers) for both old style configuration and
HSA configuration.

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

Adds kernel argument definition. Must be inside any kernel configuration. First argument is
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
structure. ACCESS for image determines can be one of the: `read_only`, `rdonly`,
`write_only`, `wronly` or 'read_write', 'rdwr'.
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

### .call_convention

Syntax: .call_convention CALL_CONV

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`).
Set call convention for kernel.

### .codeversion

Syntax .codeversion MAJOR, MINOR

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`).
Set AMD code version.

### .compile_options

Syntax: .compile_options "STRING"

Set compile options for this binary.

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

### .control_directive

Open control directive section. This section must be 128 bytes. The content of this
section will be stored in control_directive field in kernel configuration.
Must be defined inside kernel.

### .cws, .reqd_work_group_size

Syntax: .cws [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]  
Syntax: .reqd_work_group_size [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]

This pseudo-operation must be inside any kernel configuration.
Set reqd_work_group_size hint for this kernel.
In versions earlier than 0.1.7 this pseudo-op has been broken and this pseudo-op
set zeroes in two last component instead ones. We recomment to fill all components.

### .debug_private_segment_buffer_sgpr

Syntax: .debug_private_segment_buffer_sgpr SGPRREG

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`debug_private_segment_buffer_sgpr` field in kernel configuration.

### .debug_wavefront_private_segment_offset_sgpr

Syntax: .debug_wavefront_private_segment_offset_sgpr SGPRREG

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`debug_wavefront_private_segment_offset_sgpr` field in kernel configuration.

### .debugmode

This pseudo-operation must be inside any kernel configuration.
Enable usage of the DEBUG_MODE.

### .default_hsa_features

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`).
It sets default HSA kernel features and register features (extra SGPR registers usage).
These default features are `.use_private_segment_buffer`, `.use_kernarg_segment_ptr`,
`.use_ptr64` (if 64-bit binaries) and private_elem_size is 4 bytes.

### .dims

Syntax: .dims DIMENSIONS  
Syntax: .dims GID_DIMS, LID_DIMS

This pseudo-operation must be inside any kernel configuration. Define what dimensions
(from list: x, y, z) will be used to determine space of the kernel execution.
In second syntax form, the dimensions are given for group_id (GID_DIMS) and for local_id
(LID_DIMS) separately. If you use first syntax with single argument and if your kernel use
enqueue (`.useenqueue`) then all dimensions for local_id will be always enabled,
if you do not want it then use second syntax.

### .driver_version

Syntax: .driver_version VERSION

Set driver version for this binary. Version in form: MajorVersion*100+MinorVersion.
This pseudo-op replaces driver info.

### .dx10clamp

This pseudo-operation must be inside any kernel configuration.
Enable usage of the DX10_CLAMP.

### .exceptions

Syntax: .exceptions EXCPMASK

This pseudo-operation must be inside any kernel configuration.
Set exception mask in PGMRSRC2 register value. Value should be 7-bit.

### .gds_segment_size

Syntax: .gds_segment_size SIZE

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`gds_segment_size` field in kernel configuration.

### .gdssize

Syntax: .gdssize SIZE

This pseudo-operation must be inside any kernel configuration. Set the GDS
(global data share) size.

### .get_driver_version

Syntax: .get_driver_version SYMBOL

Store current driver version to SYMBOL. Version in form `version*100 + revision`.

### .globaldata

Go to constant global data section.

### .group_segment_align

Syntax: .group_segment_align ALIGN

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`group_segment_align` field in kernel configuration.

### .hsaconfig

Open kernel HSA configuration. Must be inside kernel. Kernel configuration can not be
defined if any isametadata, metadata or stub was defined. Do not mix with `.config`.

### .hsalayout

This pseudo-op enabled HSA layout mode (source code layout similar to Gallium binary format
layout or ROCm layout) where code of the kernels is in single main code section and
kernels are aligned and kernel setup is skipped in section code.
Only allowed for newer driver version binaries.

### .ieeemode

This pseudo-op must be inside any kernel configuration. Set ieee-mode.

### .inner

Go to inner binary place. By default assembler is in main binary.

### .isametadata

This pseudo-operation must be inside kernel. Go to ISA metadata content
(only older driver binaries).

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

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`kernarg_segment_alignment` field in kernel configuration. Value must be a power of two.

### .kernarg_segment_size

Syntax: .kernarg_segment_size SIZE

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`kernarg_segment_byte_size` field in kernel configuration.

### .kernel_code_entry_offset

Syntax: .kernel_code_entry_offset OFFSET

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`kernel_code_entry_byte_offset` field in kernel configuration. This field
store offset between configuration and kernel code. By default is 256.

### .kernel_code_prefetch_offset

Syntax: .kernel_code_prefetch_offset OFFSET

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`kernel_code_prefetch_byte_offset` field in kernel configuration.

### .kernel_code_prefetch_size

Syntax: .kernel_code_prefetch_size OFFSET

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`kernel_code_prefetch_byte_size` field in kernel configuration.

### .localsize

Syntax: .localsize SIZE

This pseudo-operation must be inside any kernel configuration. Set the initial
local data size.

### .machine

Syntax: .machine KIND, MAJOR, MINOR, STEPPING

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
machine version fields in kernel configuration.

### .max_scratch_backing_memory

Syntax: .max_scratch_backing_memory SIZE

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`max_scratch_backing_memory_byte_size` field in kernel configuration.

### .metadata

This pseudo-operation must be inside kernel. Go to metadata content.

### .pgmrsrc1

Syntax: .pgmrsrc1 VALUE

This pseudo-operation must be inside kernel.
Define value of the PGMRSRC1.


### .pgmrsrc2

Syntax: .pgmrsrc2 VALUE

This pseudo-operation must be inside any kernel configuration. Set PGMRSRC2 value.
If dimensions is set then bits that controls dimension setup will be ignored.
SCRATCH_EN bit will be ignored.

### .priority

Syntax: .priority PRIORITY

This pseudo-operation must be inside kernel. Define priority (0-3).

### .private_elem_size

Syntax: .private_elem_size ELEMSIZE

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`).
Set `private_element_size` field in kernel configuration.
Must be a power of two between 2 and 16.

### .private_segment_align

Syntax: .private_segment ALIGN

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`private_segment_alignment` field in kernel configuration. Value must be a power of two.

### .privmode

This pseudo-operation must be inside kernel.
Enable usage of the PRIV (privileged mode).

### .reserved_sgprs

Syntax: .reserved_sgprs FIRSTREG, LASTREG

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`reserved_sgpr_first` and `reserved_sgpr_count` fields in kernel configuration.
`reserved_sgpr_count` filled by number of registers (LASTREG-FIRSTREG+1).

### .reserved_vgprs

Syntax: .reserved_vgprs FIRSTREG, LASTREG

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`reserved_vgpr_first` and `reserved_vgpr_count` fields in kernel configuration.
`reserved_vgpr_count` filled by number of registers (LASTREG-FIRSTREG+1).

### .runtime_loader_kernel_symbol

Syntax: .runtime_loader_kernel_symbol ADDRESS

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`runtime_loader_kernel_symbol` field in kernel configuration.

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

This pseudo-operation must be inside any kernel configuration.
Set scratchbuffer size.

### .setup

Go to kernel setup content section.

### .setupargs

This pseudo-op must be inside any kernel configuration. Add first kernel setup arguments.
This pseudo-op must be before any other arguments.

### .sgprsnum

Syntax: .sgprsnum REGNUM

This pseudo-op must be inside any kernel configuration. Set number of scalar
registers which can be used during kernel execution. In old-config style,
it counts SGPR registers excluding VCC, FLAT_SCRATCH and XNACK_MASK.
In HSA-config style, it counts SGPR registers including VCC, FLAT_SCRATCH and XNACK_MASK
(like ROCm).

### .stub

Go to kernel stub content section. Only allowed for older driver version binaries.

### .tgsize

This pseudo-op must be inside any kernel configuration.
Enable usage of the TG_SIZE_EN.

### .use_debug_enabled

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`is_debug_enabled` field in kernel configuration.

### .use_dispatch_id

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_sgpr_dispatch_id` field in kernel configuration.

### .use_dispatch_ptr

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_sgpr_dispatch_ptr` field in kernel configuration.

### .use_dynamic_call_stack

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`is_dynamic_call_stack` field in kernel configuration.

### .use_flat_scratch_init

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_sgpr_flat_scratch_init` field in kernel configuration.

### .use_grid_workgroup_count

Syntax: .use_grid_workgroup_count DIMENSIONS

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_sgpr_grid_workgroup_count_X`, `enable_sgpr_grid_workgroup_count_Y`
and `enable_sgpr_grid_workgroup_count_Z` fields in kernel configuration,
respectively by given dimensions.

### .use_kernarg_segment_ptr

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_sgpr_kernarg_segment_ptr` field in kernel configuration.

### .use_ordered_append_gds

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_ordered_append_gds` field in kernel configuration.

### .use_private_segment_buffer

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_sgpr_private_segment_buffer` field in kernel configuration.

### .use_private_segment_size

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_sgpr_private_segment_size` field in kernel configuration.

### .use_ptr64

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`).
Enable `is_ptr64` field in kernel configuration.

### .use_queue_ptr

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`enable_sgpr_queue_ptr` field in kernel configuration.

### .use_xnack_enabled

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Enable
`is_xnack_enabled` field in kernel configuration.

### .useargs

This pseudo-op must be inside any kernel (non-HSA) configuration.
Indicate that kernel uses arguments.

### .useenqueue

This pseudo-op must be inside any kernel (non-HSA) configuration.
Indicate that kernel uses enqueue mechanism.

### .usegeneric

This pseudo-op must be inside any kernel (non-HSA) configuration.
Indicate that kernel uses generic pointers mechanism (FLAT instructions).

### .usesetup

This pseudo-op must be inside any kernel (non-HSA) configuration.
Indicate that kernel uses setup data (global sizes, local sizes, work groups num).

### .userdatanum

Syntax: .userdatanum NUMBER

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set number of
registers for USERDATA.

### .vectypehint

Syntax: .vectypehint OPENCLTYPE

This pseudo-operation must be inside any kernel configuration.
Set vectypehint for kernel. The argument is OpenCL type.

### .vgprsnum

Syntax: .vgprsnum REGNUM

This pseudo-op must be inside any kernel configuration. Set number of vector
registers which can be used during kernel execution.

### .wavefront_sgpr_count

Syntax: .wavefront_sgpr_count REGNUM

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`wavefront_sgpr_count` field in kernel configuration.

### .wavefront_size

Syntax: .wavefront_size POWEROFTWO

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`).
Set `wavefront_size` field in kernel configuration. Value must be a power of two.

### .work_group_size_hint

Syntax: .work_group_size_hint [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]

This pseudo-operation must be inside any kernel configuration.
Set work_group_size_hint for this kernel.

### .workgroup_fbarrier_count

Syntax: .workgroup_fbarrier_count COUNT

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`workgroup_fbarrier_count` field in kernel configuration.

### .workgroup_group_segment_size

Syntax: .workgroup_group_segment_size SIZE

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`workgroup_group_segment_byte_size` in kernel configuration.

### .workitem_private_segment_size

Syntax: .workitem_private_segment_size SIZE

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`workitem_private_segment_byte_size` field in kernel configuration.

### .workitem_vgpr_count

Syntax: .workitem_vgpr_count REGNUM

This pseudo-op must be inside kernel HSA configuration (`.hsaconfig`). Set
`workitem_vgpr_count` field in kernel configuration.

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

This is sample of two kernels with configuration in HSA layout mode:

```
.amdcl2
.64bit
.gpu Bonaire
.driver_version 191205
.hsalayout
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
.kernel DCT2
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
DCT:
.skip 256   # setup kernel skip
/*c0000501         */ s_load_dword    s0, s[4:5], 0x1
....
/*bf810000         */ s_endpgm
.p2align 8            # important alignment to 256-byte boundary
DCT2:
.skip 256   # setup kernel skip
/*c0000501         */ s_load_dword    s0, s[4:5], 0x1
....
/*bf810000         */ s_endpgm
```
