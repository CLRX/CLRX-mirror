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

This pseudo-op must be inside kernel configuration (`.config`).

### .config

### .control_directive

### .debug_private_segment_buffer_sgpr

This pseudo-op must be inside kernel configuration (`.config`).

### .debug_wavefront_private_segment_offset_sgpr

This pseudo-op must be inside kernel configuration (`.config`).

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

This pseudo-op must be inside kernel configuration (`.config`).

### .fkernel

### .floatmode

This pseudo-op must be inside kernel configuration (`.config`).

### .gds_segment_size

This pseudo-op must be inside kernel configuration (`.config`).

### .group_segment_align

This pseudo-op must be inside kernel configuration (`.config`).

### .ieeemode

This pseudo-op must be inside kernel configuration (`.config`).

### .kcode

### .kcodeend

### .kernarg_segment_align

This pseudo-op must be inside kernel configuration (`.config`).

### .kernarg_segment_size

This pseudo-op must be inside kernel configuration (`.config`).

### .kernel_code_entry_offset

This pseudo-op must be inside kernel configuration (`.config`).

### .kernel_code_prefetch_offset

This pseudo-op must be inside kernel configuration (`.config`).

### .kernel_code_prefetch_size

This pseudo-op must be inside kernel configuration (`.config`).

### .localsize

This pseudo-op must be inside kernel configuration (`.config`).

### .machine

This pseudo-op must be inside kernel configuration (`.config`).

### .max_scratch_backing_memory

This pseudo-op must be inside kernel configuration (`.config`).

### .pgmrsrc1

This pseudo-op must be inside kernel configuration (`.config`).

### .pgmrsrc2

This pseudo-op must be inside kernel configuration (`.config`).

### .priority

This pseudo-op must be inside kernel configuration (`.config`).

### .private_elem_size

This pseudo-op must be inside kernel configuration (`.config`).

### .private_segment_align

This pseudo-op must be inside kernel configuration (`.config`).

### .privmode

This pseudo-op must be inside kernel configuration (`.config`).

### .reserved_sgprs

This pseudo-op must be inside kernel configuration (`.config`).

### .reserved_vgprs

This pseudo-op must be inside kernel configuration (`.config`).

### .runtime_loader_kernel_symbol

This pseudo-op must be inside kernel configuration (`.config`).

### .scratchbuffer

This pseudo-op must be inside kernel configuration (`.config`).

### .sgprsnum

This pseudo-op must be inside kernel configuration (`.config`).

### .tgsize

This pseudo-op must be inside kernel configuration (`.config`).

### .use_debug_enabled

This pseudo-op must be inside kernel configuration (`.config`).

### .use_dispatch_id

This pseudo-op must be inside kernel configuration (`.config`).

### .use_dispatch_ptr

This pseudo-op must be inside kernel configuration (`.config`).

### .use_dynamic_call_stack

This pseudo-op must be inside kernel configuration (`.config`).

### .use_flat_scratch_init

This pseudo-op must be inside kernel configuration (`.config`).

### .use_grid_workgroup_count

This pseudo-op must be inside kernel configuration (`.config`).

### .use_kernarg_segment_ptr

This pseudo-op must be inside kernel configuration (`.config`).

### .use_ordered_append_gds

This pseudo-op must be inside kernel configuration (`.config`).

### .use_private_segment_buffer

This pseudo-op must be inside kernel configuration (`.config`).

### .use_private_segment_size

This pseudo-op must be inside kernel configuration (`.config`).

### .use_ptr64

This pseudo-op must be inside kernel configuration (`.config`).

### .use_queue_ptr

This pseudo-op must be inside kernel configuration (`.config`).

### .use_xnack_enabled

This pseudo-op must be inside kernel configuration (`.config`).

### .userdatanum

This pseudo-op must be inside kernel configuration (`.config`).

### .vgprsnum

This pseudo-op must be inside kernel configuration (`.config`).

### .wavefront_sgpr_count

This pseudo-op must be inside kernel configuration (`.config`).

### .wavefront_size

This pseudo-op must be inside kernel configuration (`.config`).

### .workgroup_fbarrier_count

This pseudo-op must be inside kernel configuration (`.config`).

### .workgroup_group_segment_size

This pseudo-op must be inside kernel configuration (`.config`).

### .workitem_private_segment_size

This pseudo-op must be inside kernel configuration (`.config`).

### .workitem_vgpr_count

This pseudo-op must be inside kernel configuration (`.config`).
