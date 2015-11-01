## AMD Catalyst OpenCL ABI description

This chapter describes how kernel gets its argument, how access to constant data.

### User data classes

User data is stored in first scalar registers. Data class indicates what data are stored.
Following data classes:

* IMM_UAV - data to uav. Holds 4 registers. ApiSlot determines uavid.
* IMM_SAMPLER - data for sampler (4 registers).
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
Each entry have 4 dwords.

### Argument passing and kernel setup

First const buffer (id=0) holds:

* 0-2 dwords - global size for each dimensions
* 3 dword - number of dimensions
* 4-6 dwords - local size for each dimensions
* 8-10 dwords - number of groups for each dimensions
* 24-26 dwords - global offset for each dimensions
* 27 dword - get_global_offset(0)\*(workDim>=1?get_global_offset(1):1)\*
            (workDim==2?get_global_offset(2):1)
* 36-38 dwords (32-bit binary) - global offset for each dimensions
* 37-39 dwords (64-bit binary) - global offset for each dimensions

Second const buffer (id=1) holds:

arguments aligned to 4 dwords.

### Other data and resources

Scalar register after userdata holds (n - userdatanum):

* s[n:n+2] - group id for each dimension

First three vector registers holds local ids for each dimensions.
