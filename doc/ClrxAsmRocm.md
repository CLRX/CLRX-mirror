## CLRadeonExtender Assembler ROCm handling

The ROCm platform is new an open-source  environment created by AMD for Radeon GPU
(especially designed for HPC and their proffesional products). This platform uses HSACO
binary object file format to store compiled code for GPU's.

## Binary format

The binary file is stored in ELF file. The symbol table holds kernels and data's symbols.
Main `.text` section contains all code for all kernels. Data
(for example global constant datas) also stored in `.text' section.
Kernel symbols points to configuration for kernel. Special offset field in configuration
data's points where is kernel code.

The assembler source code divided to three parts:

* kernel configuration
* kernel code and data (in `.text` section`)

Order of these parts doesn't matter.

Kernel function should to be aligned to 256 byte boundary.

## Scalar register allocation

Assembler for ROCm format counts all SGPR registers and add extra registers
(VCC, FLAT_SCRATCH, XNACK_MASK) if any used to register pool. Special fields determines
what extra SGPR extra has been added.

## List of the specific pseudo-operations

### .arch_minor

Syntax: .arch_minor ARCH_MINOR

Set architecture minor number.

### .arch_stepping

Syntax: .arch_minor ARCH_STEPPING

Set architecture stepping number.

### .call_convention

Syntax: .call_convention CALL_CONV

This pseudo-op must be inside kernel configuration (`.config`).
Set call convention for kernel.

### .codeversion

Syntax .codeversion MAJOR, MINOR

This pseudo-op must be inside kernel configuration (`.config`). Set AMD code version.

### .config

Open kernel configuration. Must be inside kernel.

### .control_directive

Open control directive section. This section must be 128 bytes. The content of this
section will be stored in control_directive field in kernel configuration.
Must be defined inside kernel.

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

### .dims

Syntax: .dims DIMENSIONS

This pseudo-op must be inside kernel configuration (`.config`). Defines what dimensions
(from list: x, y, z) will be used to determine space of the kernel execution.

### .dx10clamp

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the DX10_CLAMP.

### .exceptions

Syntax: .exceptions EXCPMASK

This pseudo-op must be inside kernel configuration (`.config`).
Set exception mask in PGMRSRC2 register value. Value should be 7-bit.

### .fkernel

Mark given kernel as function in ROCm. Must be inside kernel.

### .floatmode

Syntax: .floatmode BYTE-VALUE

This pseudo-op must be inside kernel configuration (`.config`). Defines float-mode.
Set floatmode (FP_ROUND and FP_DENORM fields of the MODE register). Default value is 0xc0.

### .gds_segment_size

This pseudo-op must be inside kernel configuration (`.config`).

### .group_segment_align

This pseudo-op must be inside kernel configuration (`.config`). Set
`gds_segment_byte_size` field in kernel configuration.

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

This pseudo-op must be inside kernel configuration (`.config`). Defines initial
local memory size used by kernel.

### .machine

Syntax: .machine KIND, MAJOR, MINOR, STEPPING

This pseudo-op must be inside kernel configuration (`.config`). Set
machine version fields in kernel configuration.

### .max_scratch_backing_memory

Syntax: .max_scratch_backing_memory SIZE

This pseudo-op must be inside kernel configuration (`.config`). Set
`max_scratch_backing_memory_byte_size` field in kernel configuration.

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

### .runtime_loader_kernel_symbol

Syntax: .runtime_loader_kernel_symbol ADDRESS

This pseudo-op must be inside kernel configuration (`.config`). Set
`runtime_loader_kernel_symbol` field in kernel configuration.

### .scratchbuffer

Syntax: .scratchbuffer SIZE

This pseudo-op must be inside kernel configuration (`.config`). Defines scratchbuffer size.

### .sgprsnum

Syntax: .sgprsnum REGNUM

This pseudo-op must be inside kernel configuration (`.config`). Set number of scalar
registers which can be used during kernel execution.

### .tgsize

This pseudo-op must be inside kernel configuration (`.config`).
Enable usage of the TG_SIZE_EN.

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
