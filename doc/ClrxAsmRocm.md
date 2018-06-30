## CLRadeonExtender Assembler ROCm handling

The ROCm platform is new an open-source  environment created by AMD for Radeon GPU
(especially designed for HPC and their proffesional products). This platform uses HSACO
binary object file format to store compiled code for GPU's.

The ROCm binary format implementation and this documentation based on source:
[ROCm-ComputeABI-Doc](https://github.com/RadeonOpenCompute/ROCm-ComputeABI-Doc).

## Binary format

The binary file is stored in ELF file. The symbol table holds kernels and data's symbols.
Main `.text` section contains all code for all kernels. Data
(for example global constant datas) also stored in `.text' section.
Kernel symbols points to configuration for kernel. Special offset field in configuration
data's points where is kernel code.

The assembler source code divided to three parts:

* kernel configuration
* kernel code and data (in `.text` section)

Order of these parts doesn't matter.

Kernel function should to be aligned to 256 byte boundary.

Additional kernel informations and binary informations are in metadata ELF note.
It holds informations about `printf` calls, kernel configuration and its arguments.

## Register usage setup

The CLRX assembler automatically sets number of used VGPRs and number of used SGPRs.
This setup can be replaced by pseudo-ops '.sgprsnum' and '.vgprsnum'.

## Scalar register allocation

An assembler for ROCm format counts all SGPR registers and add extra registers
(FLAT_SCRATCH, XNACK_MASK). Special fields determines
what extra SGPR registers (FLAT_SCRATCH, VCC and XNACK_MASK) has been added.
The VCC register is included by default.

The `.sgprsnum` set number of all SGPRs including VCC, FLAT_SCRATCH and XNACK_MASK.

## Expression with sections

An assembler can calculate difference between symbols which present in one of three sections:
globaldata (rodata) section, code section and GOT (Global Offset Table) section.
For example, an expression `.-globaldata1` (if globaldata is defined in global data section)
calculates distance between current position and `globaldata1` place.
An assembler automcatically found section where symbol points to between code,
globaldata and GOT. Because, layout of the sections is not known while assemblying,
section differences are possible in places where expression can be evaluated later:
in `.int` or similar pseudo-ops, in the literal values in instructions,
in the symbol assignments, etc.

## List of the specific pseudo-operations

### .arch_minor

Syntax: .arch_minor ARCH_MINOR

Set architecture minor number.

### .arch_stepping

Syntax: .arch_minor ARCH_STEPPING

Set architecture stepping number.

### .arg

Syntax arg: .arg [NAME]\[, "TYPENAME"], SIZE, [ALIGN], VALUEKIND, VALUETYPE[,POINTEEALIGN]\[, ADDRSPACE]\[,ACCQUAL]\[,ACTACCQUAL] \[FLAG1\] \[FLAG2\]...

This pseudo-op must be inside kernel configuration (`.config`).
Define kernel argument in metadata info. The argument name, type name, alignment are
optional. The ADDRSPACE is address space and it present only if value kind is
`globalbuf` or `dynshptr`. The POINTEEALIGN is pointee alignment in bytes and it present
only if value kind is `dynshptr`. The ACCQUAL defines access qualifier and it present
only if value kind is `image` or `pipe`. The ACTACCQUAL defines actual access qualifier
and it present only if value kind is `image`, `pipe` or `globalbuf`.
The FLAGS is list of flags delimited by spaces.

The list of value kinds:

* complact - hidden competion action
* defqueue -hidden default command queue
* dynshptr - dynamic shared pointer (local, private)
* globalbuf - global buffer
* gox, globaloffsetx - hidden global offset x
* goy, globaloffsety - hidden global offset y
* goz, globaloffsetz - hidden global offset z
* image - image object
* none - hidden none to make space between arguments
* pipe - OpenCL 2.0 pipe object
* printfbuf - hidden printf buffer
* queue - command queue
* sampler - image sampler
* value - ByValue - argument holds value (integer, floats)

The list of value types:

* i8, char - signed 8-bit integer
* i16, short - signed 16-bit integer
* i32, int - signed 32-bit integer
* i64, long - signed 64-bit integer
* u8, uchar - unsigned 8-bit integer
* u16, ushort - unsigned 16-bit integer
* u32, uint - unsigned 32-bit integer
* u64, ulong - unsigned 64-bit integer
* f16, half - 16-bit half floating point
* f32, float - 32-bit single floating point
* f64, double - 64-bit double floating point
* struct - structure

The list of address spaces:

* constant - constant space (???)
* generic - generic (global or scratch or local)
* global - global memory
* local - local memory
* private - private memory
* region - ???

This list of access qualifiers:

* default - default access qualifier
* read_only, rdonly - read only
* read_write, rdwr - read and write
* write_only, wronly - write only

This list of flags:

* const - constant value (only for global buffer)
* restrict - restrict value (only for global buffer)
* volatile - volatile (only for global buffer)
* pipe - only for pipe value kind

### .call_convention

Syntax: .call_convention CALL_CONV

This pseudo-op must be inside kernel configuration (`.config`).
Set call convention for kernel.

### .codeversion

Syntax .codeversion MAJOR, MINOR

This pseudo-op must be inside kernel configuration (`.config`). Set AMD code version.

### .config

Open kernel configuration. Must be inside kernel.

The kernel metadata info config pseudo-ops:

* .arg - add kernel argument
* .md_language - kernel language
* .cws, .reqd_work_group_size - reqd_work_group_size
* .work_group_size_hint - work_group_size_hint
* .fixed_work_group_size - fixed work group size
* .max_flat_work_group_size - max flat work group size
* .vectypehint - vector type hint
* .runtime_handle - runtime handle symbol name
* .md_kernarg_segment_align - kernel argument segment alignment
* .md_kernarg_segment_size - kernel argument segment size
* .md_group_segment_fixed_size - group segment fixed size
* .md_private_segment_fixed_size - private segment fixed size
* .md_symname - kernel symbol name
* .md_sgprsnum - number of SGPRs
* .md_vgprsnum - number of VGPRs
* .spilledsgprs - number of spilled SGPRs
* .spilledvgprs - number of spilled VGPRs
* .md_wavefront_size - wavefront size

### .control_directive

Open control directive section. This section must be 128 bytes. The content of this
section will be stored in control_directive field in kernel configuration.
Must be defined inside kernel.

### .cws, .reqd_work_group_size

Syntax: .cws [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]  
Syntax: .reqd_work_group_size [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]

This pseudo-operation must be inside any kernel configuration.
Set reqd_work_group_size hint for this kernel in metadata info.

### .debug_private_segment_buffer_sgpr

Syntax: .debug_private_segment_buffer_sgpr SGPRREG

This pseudo-op must be inside kernel configuration (`.config`). Set
`debug_private_segment_buffer_sgpr` field in kernel configuration.

### .debug_wavefront_private_segment_offset_sgpr

Syntax: .debug_wavefront_private_segment_offset_sgpr SGPRREG

This pseudo-op must be inside kernel configuration (`.config`). Set
`debug_wavefront_private_segment_offset_sgpr` field in kernel configuration.

### .debugmode

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the DEBUG_MODE.

### .default_hsa_features

This pseudo-op must be inside kernel configuration (`.config`).
It sets default HSA kernel features and register features (extra SGPR registers usage).
These default features are `.use_private_segment_buffer`, `.use_dispatch_ptr`,
`.use_kernarg_segment_ptr`, `.use_ptr64` and private_elem_size to 4 bytes.

### .dims

Syntax: .dims DIMENSIONS  
Syntax: .dims GID_DIMS, LID_DIMS

This pseudo-op must be inside kernel configuration (`.config`). Define what dimensions
(from list: x, y, z) will be used to determine space of the kernel execution.
In second syntax form, the dimensions are given for group_id (GID_DIMS) and for local_id
(LID_DIMS) separately.

### .dx10clamp

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the DX10_CLAMP.

### .eflags

Syntax: .eflags EFLAGS

Set value of ELF header e_flags field.

### .exceptions

Syntax: .exceptions EXCPMASK

This pseudo-op must be inside kernel configuration (`.config`).
Set exception mask in PGMRSRC2 register value. Value should be 7-bit.

### .fixed_work_group_size

Syntax: .fixed_work_group_size [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]

This pseudo-operation must be inside any kernel configuration.
Set fixed_work_group_size for this kernel in metadata info.

### .fkernel

Mark given kernel as function in ROCm. Must be inside kernel.

### .floatmode

Syntax: .floatmode BYTE-VALUE

This pseudo-op must be inside kernel configuration (`.config`). Define float-mode.
Set floatmode (FP_ROUND and FP_DENORM fields of the MODE register). Default value is 0xc0.

### .gds_segment_size

Syntax: .gds_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`). Set
`gds_segment_size` field in kernel configuration.

### .globaldata

Go to constant global data section (`.rodata`).

### .gotsym

Syntax: .gotsym SYMBOL[, OUTSYMBOL]

Add GOT entry for SYMBOL. A SYMBOL must be defined in global scope. Optionally, pseudo-op
set position of the GOT entry to OUTSYMBOL if symbol was given. A GOT entry take 8 bytes.

### .group_segment_align

Syntax: .group_segment_align ALIGN

This pseudo-op must be inside kernel configuration (`.config`). Set
`group_segment_align` field in kernel configuration.

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

This pseudo-op must be inside kernel configuration (`.config`). Set
`kernarg_segment_alignment` field in kernel configuration. Value must be a power of two.

### .kernarg_segment_size

Syntax: .kernarg_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`). Set
`kernarg_segment_byte_size` field in kernel configuration.

### .kernel_code_entry_offset

Syntax: .kernel_code_entry_offset OFFSET

This pseudo-op must be inside kernel configuration (`.config`). Set
`kernel_code_entry_byte_offset` field in kernel configuration. This field
store offset between configuration and kernel code. By default is 256.

### .kernel_code_prefetch_offset

Syntax: .kernel_code_prefetch_offset OFFSET

This pseudo-op must be inside kernel configuration (`.config`). Set
`kernel_code_prefetch_byte_offset` field in kernel configuration.

### .kernel_code_prefetch_size

Syntax: .kernel_code_prefetch_size OFFSET

This pseudo-op must be inside kernel configuration (`.config`). Set
`kernel_code_prefetch_byte_size` field in kernel configuration.

### .localsize

Syntax: .localsize SIZE

This pseudo-op must be inside kernel configuration (`.config`). Define initial
local memory size used by kernel.

### .machine

Syntax: .machine KIND, MAJOR, MINOR, STEPPING

This pseudo-op must be inside kernel configuration (`.config`). Set
machine version fields in kernel configuration.

### .max_flat_work_group_size

Syntax: .max_flat_work_group_size SIZE

This pseudo-op must be inside kernel configuration (`.config`).
Set max flat work group size in metadata info.

### .max_scratch_backing_memory

Syntax: .max_scratch_backing_memory SIZE

This pseudo-op must be inside kernel configuration (`.config`). Set
`max_scratch_backing_memory_byte_size` field in kernel configuration.

### .md_group_segment_fixed_size

Syntax: .md_group_segment_fixed_size SIZE

This pseudo-op must be inside kernel configuration (`.config`).
Set group segment fixed size in metadata info.

### .md_kernarg_segment_align

Syntax: .md_kernarg_segment_align ALIGNMENT

This pseudo-op must be inside kernel configuration (`.config`).
Set kernel argument segment alignment in metadata info.

### .md_kernarg_segment_size

Syntax: .md_kernarg_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`).
Set kernel argument segment size in metadata info.

### .md_private_segment_fixed_size

Syntax: .md_private_segment_fixed_size SIZE

This pseudo-op must be inside kernel configuration (`.config`).
Set private segment fixed size in metadata info.

### .md_symname

Syntax: .md_symname "SYMBOLNAME"

This pseudo-op must be inside kernel configuration (`.config`).
Set kernel symbol name in metadata info. It should be in format "NAME@kd".

### .md_language

Syntax .md_language "LANGUAGE"[, MAJOR, MINOR]

This pseudo-op must be inside kernel configuration (`.config`).
Set kernel language and its version in metadata info. The language name is as string.

### .md_sgprsnum

Syntax: .md_sgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`).
Define number of scalar registers for kernel in metadata info.

### .md_version

Syntax: .md_version MAJOR, MINOR

This pseudo-ops defines metadata format version.

### .md_wavefront_size

Syntax: .md_wavefront_size SIZE

This pseudo-op must be inside kernel configuration (`.config`).
Define wavefront size in metadata info. If not specified then value get from HSA config.

### .md_vgprsnum

Syntax: .md_vgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`).
Define number of vector registers for kernel in metadata info.

### .metadata

This pseudo-operation must be inside kernel. Go to metadata (metadata ELF note) section.

### .newbinfmt

This pseudo-op set new binary format.

### .nosectdiffs

This pseudo-op disable section difference resolving. After disabling it, the global data
and GOT sections are absolute addressable. This is old ROCm mode for compatibility with
older an assembler's versions.

### .pgmrsrc1

Syntax: .pgmrsrc1 VALUE

This pseudo-op must be inside kernel configuration (`.config`).
Define value of the PGMRSRC1.

### .pgmrsrc2

Syntax: .pgmrsrc2 VALUE

This pseudo-op must be inside kernel configuration (`.config`).
Define value of the PGMRSRC2. If dimensions is set then bits that controls dimension setup
will be ignored. SCRATCH_EN bit will be ignored.

### .printf

Syntax: .printf [ID]\[,ARGSIZE,....],"FORMAT"

This pseudo-op must be inside kernel configuration (`.config`).
Adds new printf info entry to metadata info. The first argument is ID (must be unique)
and is optional. Next arguments are argument size for printf call. The last argument
is format string.

### .priority

Syntax: .priority PRIORITY

This pseudo-op must be inside kernel configuration (`.config`). Define priority (0-3).

### .private_elem_size

Syntax: .private_elem_size ELEMSIZE

This pseudo-op must be inside kernel configuration (`.config`). Set `private_element_size`
field in kernel configuration. Must be a power of two between 2 and 16.

### .private_segment_align

Syntax: .private_segment ALIGN

This pseudo-op must be inside kernel configuration (`.config`). Set
`private_segment_alignment` field in kernel configuration. Value must be a power of two.

### .privmode

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the PRIV (privileged mode).

### .reserved_sgprs

Syntax: .reserved_sgprs FIRSTREG, LASTREG

This pseudo-op must be inside kernel configuration (`.config`). Set
`reserved_sgpr_first` and `reserved_sgpr_count` fields in kernel configuration.
`reserved_sgpr_count` filled by number of registers (LASTREG-FIRSTREG+1).

### .reserved_vgprs

Syntax: .reserved_vgprs FIRSTREG, LASTREG

This pseudo-op must be inside kernel configuration (`.config`). Set
`reserved_vgpr_first` and `reserved_vgpr_count` fields in kernel configuration.
`reserved_vgpr_count` filled by number of registers (LASTREG-FIRSTREG+1).

### .runtime_handle

Syntax: .runtime_handle "SYMBOLNAME"

This pseudo-op must be inside kernel configuration (`.config`).
Set runtime handle in metadata info

### .runtime_loader_kernel_symbol

Syntax: .runtime_loader_kernel_symbol ADDRESS

This pseudo-op must be inside kernel configuration (`.config`). Set
`runtime_loader_kernel_symbol` field in kernel configuration.

### .scratchbuffer

Syntax: .scratchbuffer SIZE

This pseudo-op must be inside kernel configuration (`.config`). Define scratchbuffer size.

### .sgprsnum

Syntax: .sgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of scalar
registers which can be used during kernel execution.
It counts SGPR registers including VCC, FLAT_SCRATCH and XNACK_MASK.

### .spilledsgprs

Syntax: .spilledsgprs REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of scalar
registers to spill in scratch buffer (in metadata info).

### .spilledvgprs

Syntax: .spilledvgprs REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of vector
registers to spill in scratch buffer (in metadata info).

### .target

Syntax: .target "TARGET"

Set LLVM target with device name. For example: "amdgcn-amd-amdhsa-amdgizcl-gfx803".

### .tgsize

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the TG_SIZE_EN.

### .tripple

Syntax: .tripple "TRIPPLE"

Set LLVM target without device name. For example "amdgcn-amd-amdhsa-amdgizcl" with
Fiji device generates target "amdgcn-amd-amdhsa-amdgizcl-gfx803".

### .use_debug_enabled

This pseudo-op must be inside kernel configuration (`.config`). Enable `is_debug_enabled`
field in kernel configuration.

### .use_dispatch_id

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_sgpr_dispatch_id` field in kernel configuration.

### .use_dispatch_ptr

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_sgpr_dispatch_ptr` field in kernel configuration.

### .use_dynamic_call_stack

This pseudo-op must be inside kernel configuration (`.config`). Enable
`is_dynamic_call_stack` field in kernel configuration.

### .use_flat_scratch_init

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_sgpr_flat_scratch_init` field in kernel configuration.

### .use_grid_workgroup_count

Syntax: .use_grid_workgroup_count DIMENSIONS

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_sgpr_grid_workgroup_count_X`, `enable_sgpr_grid_workgroup_count_Y`
and `enable_sgpr_grid_workgroup_count_Z` fields in kernel configuration,
respectively by given dimensions.

### .use_kernarg_segment_ptr

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_sgpr_kernarg_segment_ptr` field in kernel configuration.

### .use_ordered_append_gds

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_ordered_append_gds` field in kernel configuration.

### .use_private_segment_buffer

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_sgpr_private_segment_buffer` field in kernel configuration.

### .use_private_segment_size

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_sgpr_private_segment_size` field in kernel configuration.

### .use_ptr64

This pseudo-op must be inside kernel configuration (`.config`). Enable `is_ptr64` field
in kernel configuration.

### .use_queue_ptr

This pseudo-op must be inside kernel configuration (`.config`). Enable
`enable_sgpr_queue_ptr` field in kernel configuration.

### .use_xnack_enabled

This pseudo-op must be inside kernel configuration (`.config`). Enable
`is_xnack_enabled` field in kernel configuration.

### .userdatanum

Syntax: .userdatanum NUMBER

This pseudo-op must be inside kernel configuration (`.config`). Set number of
registers for USERDATA.

### .vectypehint

Syntax: .vectypehint "OPENCLTYPE"

This pseudo-op must be inside kernel configuration (`.config`).
Set vectypehint for kernel in metadata info. The argument is OpenCL type.

### .vgprsnum

Syntax: .vgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of vector
registers which can be used during kernel execution.

### .wavefront_sgpr_count

Syntax: .wavefront_sgpr_count REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set
`wavefront_sgpr_count` field in kernel configuration.

### .wavefront_size

Syntax: .wavefront_size POWEROFTWO

This pseudo-op must be inside kernel configuration (`.config`). Set `wavefront_size`
field in kernel configuration. Value must be a power of two.

### .work_group_size_hint

Syntax: .work_group_size_hint [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]

This pseudo-operation must be inside any kernel configuration.
Set work_group_size_hint for this kernel in metadata info.

### .workgroup_fbarrier_count

Syntax: .workgroup_fbarrier_count COUNT

This pseudo-op must be inside kernel configuration (`.config`). Set
`workgroup_fbarrier_count` field in kernel configuration.

### .workgroup_group_segment_size

Syntax: .workgroup_group_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`). Set
`workgroup_group_segment_byte_size` in kernel configuration.

### .workitem_private_segment_size

Syntax: .workitem_private_segment_size SIZE

This pseudo-op must be inside kernel configuration (`.config`). Set
`workitem_private_segment_byte_size` field in kernel configuration.

### .workitem_vgpr_count

Syntax: .workitem_vgpr_count REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set
`workitem_vgpr_count` field in kernel configuration.

## Sample code

This is sample example of the kernel setup:

```
.rocm
.gpu Carrizo
.arch_minor 0
.arch_stepping 1
.kernel test1
.kernel test2
.text
test1:
        .byte 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .byte 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00
        .byte 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .fill 24, 1, 0x00
        .byte 0x41, 0x00, 0x2c, 0x00, 0x90, 0x00, 0x00, 0x00
        .byte 0x0b, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00
        .fill 8, 1, 0x00
        .byte 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .byte 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x07, 0x00
        .fill 8, 1, 0x00
        .byte 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x06
        .fill 152, 1, 0x00
/*c0020082 00000004*/ s_load_dword    s2, s[4:5], 0x4
/*c0060003 00000000*/ s_load_dwordx2  s[0:1], s[6:7], 0x0
....
```

with kernel configuration:

```
.rocm
.gpu Carrizo
.arch_minor 0
.arch_stepping 1
.kernel test1
    .config
        .dims x
        .sgprsnum 16
        .vgprsnum 8
        .dx10clamp
        .floatmode 0xc0
        .priority 0
        .userdatanum 8
        .pgmrsrc1 0x002c0041
        .pgmrsrc2 0x00000090
        .codeversion 1, 0
        .machine 1, 8, 0, 1
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_size 8
        .wavefront_sgpr_count 15
        .workitem_vgpr_count 7
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive          # optional
        .fill 128, 1, 0x00
.text
test1:
.skip 256           # skip ROCm kernel configuration (required)
/*c0020082 00000004*/ s_load_dword    s2, s[4:5], 0x4
/*c0060003 00000000*/ s_load_dwordx2  s[0:1], s[6:7], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8602ff02 0000ffff*/ s_and_b32       s2, s2, 0xffff
/*92020802         */ s_mul_i32       s2, s2, s8
/*32000002         */ v_add_u32       v0, vcc, s2, v0
/*2202009f         */ v_ashrrev_i32   v1, 31, v0
/*d28f0001 00020082*/ v_lshlrev_b64   v[1:2], 2, v[0:1]
/*32060200         */ v_add_u32       v3, vcc, s0, v1
...
```

The sample with metadata info:

```
.rocm
.gpu Fiji
.arch_minor 0
.arch_stepping 4
.eflags 2
.newbinfmt
.tripple "amdgcn-amd-amdhsa-amdgizcl"
.md_version 1, 0
.kernel vectorAdd
    .config
        .dims x
        .codeversion 1, 1
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
    .control_directive
        .fill 128, 1, 0x00
    .config
        .md_language "OpenCL", 1, 2
        .arg n, "uint", 4, , value, u32
        .arg a, "float*", 8, , globalbuf, f32, global, default const volatile
        .arg b, "float*", 8, , globalbuf, f32, global, default const
        .arg c, "float*", 8, , globalbuf, f32, global, default
        .arg , "", 8, , gox, i64
        .arg , "", 8, , goy, i64
        .arg , "", 8, , goz, i64
        .arg , "", 8, , printfbuf, i8
.text
vectorAdd:
.skip 256           # skip ROCm kernel configuration (required)
...
```

The sample with metadata info with two kernels:

```
.rocm
.gpu Fiji
.arch_minor 0
.arch_stepping 4
.eflags 2
.newbinfmt
.tripple "amdgcn-amd-amdhsa-amdgizcl"
.md_version 1, 0
.kernel vectorAdd
    .config
        .dims x
        .codeversion 1, 1
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
    .control_directive
        .fill 128, 1, 0x00
    .config
        .md_language "OpenCL", 1, 2
        .arg n, "uint", 4, , value, u32
        .arg a, "float*", 8, , globalbuf, f32, global, default const volatile
        .arg b, "float*", 8, , globalbuf, f32, global, default const
        .arg c, "float*", 8, , globalbuf, f32, global, default
        .arg , "", 8, , gox, i64
        .arg , "", 8, , goy, i64
        .arg , "", 8, , goz, i64
        .arg , "", 8, , printfbuf, i8
.kernel vectorAdd2
    .config
        .dims x
        .codeversion 1, 1
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
    .control_directive
        .fill 128, 1, 0x00
    .config
        .md_language "OpenCL", 1, 2
        .arg n, "uint", 4, , value, u32
        .arg a, "float*", 8, , globalbuf, f32, global, default const volatile
        .arg b, "float*", 8, , globalbuf, f32, global, default const
        .arg c, "float*", 8, , globalbuf, f32, global, default
        .arg , "", 8, , gox, i64
        .arg , "", 8, , goy, i64
        .arg , "", 8, , goz, i64
        .arg , "", 8, , printfbuf, i8
.text
vectorAdd:
.skip 256           # skip ROCm kernel configuration (required)
            s_mov_b32 s8, s1
...
...
            s_endpgm
.p2align 8      # important alignment to 256-byte boundary
vectorAdd2
.skip 256
            s_mov_b32 s8, s1
...
...
            s_endpgm
```
