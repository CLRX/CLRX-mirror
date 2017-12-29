## AMD Catalyst OpenCL ABI description

This chapter describes how kernel gets its argument, how access to constant data.

In this chapter, size is given in dwords. Dword is 4-byte value.

### User data classes

User data is stored in first scalar registers. Data class indicates what a data are stored.
Following data classes:

* IMM_RESOURCE - data for read_only image descriptors. ApiSlot determines uavid.
* IMM_UAV - data for UAV (global/constant buffer descriptor or
write only image descriptor). Holds 4 or 8 registers. ApiSlot determines uavid.
* IMM_SAMPLER - data for sampler (4 registers). ApiSlot determines sampler entry index
* IMM_CONST_BUFFER - const buffer (4 registers). See below.
ApiSlot determines const buffer id.
* PTR_RESOURCE_TABLE - pointer to resource table (2 registers).
Each entry holds 8 dwords. Count from zero.
Table can be accessed by using SMRD (s_load_dwordxx) instructions.
Resource table holds read-only image descriptors (8 dwords).
* PTR_SAMPLER_TABLE - pointer to sampler table (2 pointers).
Resource table holds sampler descriptors (4 dwords).
* PTR_UAV_TABLE - pointer to uav table (2 registers).
Each entry holds 8 dwords. Count from zero.
Table can be accessed by using SMRD (s_load_dwordxx) instructions.
Uav table holds UAV for global buffer, constant buffer (since 1384 driver)
and write only images (8 dwords descriptors).
* PTR_CONST_BUFFER_TABLE - pointer to const buffer table (2 registers).
Each entry have 4 dwords. For older drivers than 1348.05, global constant buffer
(third entry) and argument constant buffers descriptors stored in this table.
* PTR_INTERNAL_GLOBAL_TABLE - pointer to internal global table (2 registers).
Each entry have 4 dwords.
* IMM_SCRATCH_BUFFER - doesn't work (???)

### About resource passing

All global pointers resource descriptors stored in the UAV table begin from
UAVID+1 id. By default UAVID=11 (or for driver older than 1384.xx UAVID=9).
By default 10th entry is reserved for global data constant buffer.
9th entry is reserved for printf buffer.
First eight entries is write only image descriptors if defined.

Read only image descriptors stored in resource table.
Constant buffer descriptors (0 and 1) stored in const buffer tables

### Argument passing and kernel setup

First const buffer (id=0) holds:

* 0-2 dwords - global size for each dimension
* 3 dword - number of dimensions
* 4-6 dwords - local size for each dimension
* 8-10 dwords - number of groups for each dimension
* 24-26 dwords - global offset for each dimension
* 27 dword - get_global_offset(0)\*(workDim>=1?get_global_offset(1):1)\*
            (workDim==2?get_global_offset(2):1)
* 32 dword (32-bit binary) - global constant buffer offset
* 32-33 dword (64-bit binary) - global constant buffer offset
* 36-38 dwords (32-bit binary) - global offset for each dimension
* 37-39 dwords (64-bit binary) - global offset for each dimension

Second const buffer (id=1) holds arguments aligned to 4 dwords.

Global pointers holds vector offset (64-bit for 64-bit binary) to the memory.
Local pointers holds its offset in bytes (1 dword).

### Image arguments

Image arguments needs 8 dwords.

* 0 dword - width
* 1 dword - height
* 2 dword - depth
* 3 dword - OpenCL image format data type
* 7 dword - OpenCL image component order

### Sampler arguments

Sampler argument holds sampler value:

* 0 bit - for normalized coords is 1, zero for other
* 1-3 bits - addressing mode:
    0 - none, 1 - repeat, 2 - clamp_to_edge, 3 - clamp, 4 - mirrored_repeat
* 4-5 bits - filtering: 0 - none, 1 - nearest, 2 - linear

### Scratch buffer access

Second entry in the internal global table holds scratch buffer descriptor.
Refer to [GCN Machine State](GcnState) to learn about vector and scalar initial registers.
