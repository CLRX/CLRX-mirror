## CLRadeonExtender Assembler AMD Catalyst handling

The AMD Catalyst driver provides own OpenCL implementation that can generates
own binaries of the OpenCL programs. The CLRX assembler supports both OpenCL 1.2
and OpenCL 2.0 binary format. This chapter describes Amd OpenCL 1.2 binary format.

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

A `.data` section inside kernel is usable section and holds same zeroes.

## Layout of the source code

The CLRX assembler allow to use one of two ways to configure kernel setup:
for human (`.config`) and for quick recompilation (ATI CALNotes and the metadata string).

## Register usage setup

The CLRX assembler automatically sets number of used VGPRs and number of used SGPRs.
This setup can be replaced by pseudo-ops '.sgprsnum' and '.vgprsnum'.

## Scalar register allocation

To used scalar registers, assembler add 2 additional registers for handling VCC.
The `.sgprsnum` set number of all SGPRs except VCC. Since CLRX policy 0.2 (200)
a `.sgprsnum` set number of all SGPRs including VCC.

## List of the specific pseudo-operations

### .arg

Syntax for scalar: .arg ARGNAME \[, "ARGTYPENAME"], ARGTYPE[, unused]  
Syntax for structure: .arg ARGNAME, \[, "ARGTYPENAME"], ARGTYPE[, STRUCTSIZE[, unused]]  
Syntax for image: .arg ARGNAME\[, "ARGTYPENAME"], ARGTYPE[, [ACCESS] [, RESID[, unused]]]  
Syntax for counter32: .arg ARGNAME\[, "ARGTYPENAME"], ARGTYPE[, RESID[, unused]]  
Syntax for global pointer: .arg ARGNAME\[, "ARGTYPENAME"], 
ARGTYPE\[\[, STRUCTSIZE], PTRSPACE[, [ACCESS] [, RESID[, unused]]]]  
Syntax for local pointer: .arg ARGNAME\[, "ARGTYPENAME"], 
ARGTYPE\[\[, STRUCTSIZE], PTRSPACE[, [ACCESS] [, unused]]]  
Syntax for constant pointer: .arg ARGNAME\[, "ARGTYPENAME"], 
ARGTYPE\[\[, STRUCTSIZE], PTRSPACE\[, [ACCESS] [, [CONSTSIZE] [, RESID[, unused]]]]

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
structure. ACCESS for image determines can be one of the: `read_only`, `rdonly` or
`write_only`, `wronly`.
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

Open kernel configuration. Must be inside kernel. Kernel configuration can not be
defined if any CALNote, metadata or header was defined.
Following pseudo-ops can be inside kernel config:

* .arg
* .cbid
* .condout
* .cws
* .dims
* .earlyexit
* .hwlocal
* .hwregion
* .ieeemode
* .localsize
* .pgmrsrc2
* .printfid
* .privateid
* .sampler
* .scratchbuffer
* .sgprsnum
* .tgsize
* .uavid
* .uavprivate
* .useconstdata
* .useprintf
* .userdata
* .vgprsnum

### .constantbuffers

This pseudo-operation must be inside kernel.
Open ATI_CONSTANT_BUFFERS CAL note. Next occurrence in this same kernel,
add new CAL note.

### .cws, .reqd_work_group_size

Syntax: .cws [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]  
Syntax: .reqd_work_group_size [SIZEHINT][, [SIZEHINT][, [SIZEHINT]]]

This pseudo-operation must be inside kernel configuration.
Set reqd_work_group_size hint for this kernel.
In versions earlier than 0.1.7 this pseudo-op has been broken and this pseudo-op
set zeroes in two last component instead ones. We recomment to fill all components.

### .dims

Syntax: .dims DIMENSIONS  
Syntax: .dims GID_DIMS, LID_DIMS

This pseudo-operation must be inside kernel configuration. Define what dimensions
(from list: x, y, z) will be used to determine space of the kernel execution.
In second syntax form, the dimensions are given for group_id (GID_DIMS) and for local_id
(LID_DIMS) separately.

### .driver_info

Syntax: .driver_info "INFO"

Set driver info for this binary.

### .driver_version

Syntax: .driver_version VERSION

Set driver version for this binary. Version in form: MajorVersion*100+MinorVersion.
This pseudo-op replaces driver info.

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

### .exceptions

Syntax: .exceptions EXCPMASK

This pseudo-operation must be inside kernel configuration.
Set exception mask in PGMRSRC2 register value. Value should be 7-bit.

### .floatconsts

This pseudo-operation must be inside kernel.
Open ATI_FLOAT32CONSTS CAL note. Next occurrence in this same kernel, add new CAL note.

### .floatmode

Syntax: .floatmode VALUE

This pseudo-operation must be inside kernel configuration.
Set floatmode (FP_ROUND and FP_DENORM fields of the MODE register).
Value shall to be byte value. Default value is 0xc0.

### .get_driver_version

Syntax: .get_driver_version SYMBOL

Store current driver version to SYMBOL. Version in form `version*100 + revision`.

### .globalbuffers

This pseudo-operation must be inside kernel.
Open ATI_GLOBAL_BUFFERS CAL note. Next occurrence in this same kernel, add new CAL note.

### .globaldata

Go to constant global data section.

### .header

Go to main header of the binary.

### .hwlocal, .localsize

Syntax: .hwlocal SIZE  
Syntax: .localsize SIZE

This pseudo-operation must be inside kernel configuration. Set HWLOCAL value, the initial
local data size.

### .hwregion

Syntax: .hwregion VALUE

This pseudo-operation must be inside kernel configuration. Set HWREGION value.

### .ieeemode

Syntax: .ieeemode

This pseudo-op must be inside kernel configuration. Set ieee-mode.

### .inputs

This pseudo-operation must be inside kernel.
Open ATI_INPUTS CAL note. Next occurrence in this same kernel, add new CAL note.

### .inputsamplers

This pseudo-operation must be inside kernel.
Open ATI_INPUT_SAMPLERS CAL note. Next occurrence in this same kernel, add new CAL note.

### .intconsts

This pseudo-operation must be inside kernel.
Open ATI_INT32CONSTS CAL note. Next occurrence in this same kernel, add new CAL note.

### .metadata

This pseudo-operation must be inside kernel.
Go to metadata content.

### .outputs

This pseudo-operation must be inside kernel.
Open ATI_OUTPUTS CAL note. Next occurrence in this same kernel, add new CAL note.

### .persistentbuffers

This pseudo-operation must be inside kernel.
Open ATI_PERSISTENT_BUFFERS CAL note. Next occurrence in this same kernel,
add new CAL note.

### .pgmrsrc2

Syntax: .pgmrsrc2 VALUE

This pseudo-operation must be inside kernel configuration. Set PGMRSRC2 value.
If dimensions is set then bits that controls dimension setup will be ignored.
SCRATCH_EN bit will be ignored.

### .printfid

Syntax: .printfid RESID

This pseudo-operation must be inside kernel configuration. Set printfid.

### .privateid

Syntax: .privateid RESID

This pseudo-operation must be inside kernel configuration. Set privateid.

### .proginfo

This pseudo-operation must be inside kernel.
Open ATI_PROGINFO CAL note. Next occurrence in this same kernel, add new CAL note.

### .sampler

Syntax: .sampler INPUT, SAMPLER  
Syntax: .sampler RESID,....

If this pseudo-operation is in ATI_SAMPLER CAL note, then it adds sampler entry.
If this  pseudo-operation is in kernel configuration, then it adds samplers with specified
resource ids.

### .scratchbuffer

Syntax: .scratchbuffer SIZE

This pseudo-operation must be inside kernel configuration.
Set scratchbuffer size.

### .scratchbuffers

This pseudo-operation must be inside kernel.
Open ATI_SCRATCH_BUFFERS CAL note. Next occurrence in this same kernel, add new CAL note.

### .segment

Syntax: .segment OFFSET, SIZE

This pseudo-operation must be in ATI_BOOL32CONSTS, ATI_INT32CONSTS or
ATI_FLOAT32CONSTS CAL note. Add entry into CAL note.

### .sgprsnum

Syntax: .sgprsnum REGNUM

This pseudo-op must be inside kernel configuration. Set number of scalar
registers which can be used during kernel execution. It counts SGPR registers excluding
VCC, FLAT_SCRATCH and XNACK_MASK.

### .subconstantbuffers

This pseudo-operation must be inside kernel.
Open ATI_SUB_CONSTANT_BUFFERS CAL note. Next occurrence in this same kernel,
add new CAL note.

### .tgsize

This pseudo-op must be inside kernel configuration.
Enable usage of the TG_SIZE_EN.

### .uav

This pseudo-operation must be inside kernel.
Open ATI_UAV CAL note. Next occurrence in this same kernel,
add new CAL note.

### .uavid

Syntax: .uavid UAVID

This pseudo-op must be inside kernel configuration. Set UAVId value.

### .uavmailboxsize

Syntax: .uavmailboxsize [VALUE]

This pseudo-operation must be inside kernel.
Open ATI_UAV_MAILBOX_SIZE CAL note. Next occurrence in this same kernel,
add new CAL note. If first argument is given, then 32-bit value will be added to content.

### .uavopmask

Syntax: .uavopmask [VALUE]

This pseudo-operation must be inside kernel.
Open ATI_UAV_OP_MASK CAL note. Next occurrence in this same kernel,
add new CAL note. If first argument is given, then 32-bit value will be added to content.

### .uavprivate

Syntax: .uavprivate VALUE

This pseudo-op must be inside kernel configuration. Set uav private value.

### .useconstdata

Eanble using of the const data.

### .useprintf

Eanble using of the printf mechanism.

### .userdata

Syntax: .userdata DATACLASS, APISLOT, REGSTART, REGSIZE

This pseudo-op must be inside kernel configuration. Add USERDATA entry. First argument is
data class. It can be one of the following:

* IMM_RESOURCE
* IMM_SAMPLER
* IMM_CONST_BUFFER
* IMM_VERTEX_BUFFER
* IMM_UAV
* IMM_ALU_FLOAT_CONST
* IMM_ALU_BOOL32_CONST
* IMM_GDS_COUNTER_RANGE
* IMM_GDS_MEMORY_RANGE
* IMM_GWS_BASE
* IMM_WORK_ITEM_RANGE
* IMM_WORK_GROUP_RANGE
* IMM_DISPATCH_ID
* IMM_SCRATCH_BUFFER
* IMM_HEAP_BUFFER
* IMM_KERNEL_ARG
* SUB_PTR_FETCH_SHADER
* PTR_RESOURCE_TABLE
* PTR_INTERNAL_RESOURCE_TABLE
* PTR_SAMPLER_TABLE
* PTR_CONST_BUFFER_TABLE
* PTR_VERTEX_BUFFER_TABLE
* PTR_SO_BUFFER_TABLE
* PTR_UAV_TABLE
* PTR_INTERNAL_GLOBAL_TABLE
* PTR_EXTENDED_USER_DATA
* PTR_INDIRECT_RESOURCE
* PTR_INDIRECT_INTERNAL_RESOURCE
* PTR_INDIRECT_UAV
* IMM_CONTEXT_BASE
* IMM_LDS_ESGS_SIZE
* IMM_GLOBAL_OFFSET
* IMM_GENERIC_USER_DAT

Second argument is apiSlot.
Third argument determines the first scalar register which will hold userdata.
Fourth argument determines how many scalar register needed to hold userdata.

### .vgprsnum

Syntax: .vgprsnum REGNUM

This pseudo-op must be inside kernel configuration. Set number of vector
registers which can be used during kernel execution.

## Sample code

This is sample example of the kernel setup:

```
/* Disassembling 'DCT_15_5.1' */
.amd
.gpu Pitcairn
.32bit
.compile_options ""
.driver_info "@(#) OpenCL 1.2 AMD-APP (1702.3).  Driver version: 1702.3 (VM)"
.kernel DCT
    .header
        .fill 16, 1, 0x00
        .byte 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
        .fill 8, 1, 0x00
    .metadata
        .ascii ";ARGSTART:__OpenCL_DCT_kernel\n"
        .ascii ";version:3:1:111\n"
        .ascii ";device:pitcairn\n"
        .ascii ";uniqueid:1024\n"
        .ascii ";memory:uavprivate:0\n"
        .ascii ";memory:hwlocal:0\n"
        .ascii ";memory:hwregion:0\n"
        .ascii ";pointer:output:float:1:1:0:uav:12:4:RW:0:0\n"
        .ascii ";pointer:input:float:1:1:16:uav:13:4:RO:0:0\n"
        .ascii ";pointer:dct8x8:float:1:1:32:uav:14:4:RO:0:0\n"
        .ascii ";pointer:inter:float:1:1:48:hl:1:4:RW:0:0\n"
        .ascii ";value:width:u32:1:1:64\n"
        .ascii ";value:blockWidth:u32:1:1:80\n"
        .ascii ";value:inverse:u32:1:1:96\n"
        .ascii ";function:1:1030\n"
        .ascii ";uavid:11\n"
        .ascii ";printfid:9\n"
        .ascii ";cbid:10\n"
        .ascii ";privateid:8\n"
        .ascii ";reflection:0:float*\n"
        .ascii ";reflection:1:float*\n"
        .ascii ";reflection:2:float*\n"
        .ascii ";reflection:3:float*\n"
        .ascii ";reflection:4:uint\n"
        .ascii ";reflection:5:uint\n"
        .ascii ";reflection:6:uint\n"
        .ascii ";ARGEND:__OpenCL_DCT_kernel\n"
    .data
        .fill 4736, 1, 0x00
    .inputs
    .outputs
    .uav
        .entry 12, 4, 0, 5
        .entry 13, 4, 0, 5
        .entry 14, 4, 0, 5
        .entry 11, 4, 0, 5
    .condout 0
    .floatconsts
    .intconsts
    .boolconsts
    .earlyexit 0
    .globalbuffers
    .constantbuffers
        .cbmask 0, 32764
        .cbmask 1, 0
    .inputsamplers
    .scratchbuffers
        .int 0x00000000
    .persistentbuffers
    .proginfo
        .entry 0x80001000, 0x00000003
        .entry 0x80001001, 0x00000017
        .entry 0x80001002, 0x00000000
        .entry 0x80001003, 0x00000002
        .entry 0x80001004, 0x00000002
        .entry 0x80001005, 0x00000002
        .entry 0x80001006, 0x00000000
        .entry 0x80001007, 0x00000004
        .entry 0x80001008, 0x00000004
        .entry 0x80001009, 0x00000002
        .entry 0x8000100a, 0x00000001
        .entry 0x8000100b, 0x00000008
        .entry 0x8000100c, 0x00000004
        .entry 0x80001041, 0x0000000b
        .entry 0x80001042, 0x00000018
        .entry 0x80001863, 0x00000066
        .entry 0x80001864, 0x00000100
        .entry 0x80001043, 0x000000c0
        .entry 0x80001044, 0x00000000
        .entry 0x80001045, 0x00000000
        .entry 0x00002e13, 0x00400998
        .entry 0x8000001c, 0x00000100
        .entry 0x8000001d, 0x00000000
        .entry 0x8000001e, 0x00000000
        .entry 0x80001841, 0x00000000
        .entry 0x8000001f, 0x00007000
        .entry 0x80001843, 0x00007000
        .entry 0x80001844, 0x00000000
        .entry 0x80001845, 0x00000000
        .entry 0x80001846, 0x00000000
        .entry 0x80001847, 0x00000000
        .entry 0x80001848, 0x00000000
        .entry 0x80001849, 0x00000000
        .entry 0x8000184a, 0x00000000
        .entry 0x8000184b, 0x00000000
        .entry 0x8000184c, 0x00000000
        .entry 0x8000184d, 0x00000000
        .entry 0x8000184e, 0x00000000
        .entry 0x8000184f, 0x00000000
        .entry 0x80001850, 0x00000000
        .entry 0x80001851, 0x00000000
        .entry 0x80001852, 0x00000000
        .entry 0x80001853, 0x00000000
        .entry 0x80001854, 0x00000000
        .entry 0x80001855, 0x00000000
        .entry 0x80001856, 0x00000000
        .entry 0x80001857, 0x00000000
        .entry 0x80001858, 0x00000000
        .entry 0x80001859, 0x00000000
        .entry 0x8000185a, 0x00000000
        .entry 0x8000185b, 0x00000000
        .entry 0x8000185c, 0x00000000
        .entry 0x8000185d, 0x00000000
        .entry 0x8000185e, 0x00000000
        .entry 0x8000185f, 0x00000000
        .entry 0x80001860, 0x00000000
        .entry 0x80001861, 0x00000000
        .entry 0x80001862, 0x00000000
        .entry 0x8000000a, 0x00000001
        .entry 0x80000078, 0x00000040
        .entry 0x80000081, 0x00008000
        .entry 0x80000082, 0x00008000
    .subconstantbuffers
    .uavmailboxsize 0
    .uavopmask
        .byte 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .fill 120, 1, 0x00
    .text
/*befc03ff 00008000*/ s_mov_b32       m0, 0x8000
...
/*bf810000         */ s_endpgm
```

with kernel configuration:

```
.amd
.gpu Pitcairn
.32bit
.kernel DCT
    .config
    .dims xy
    .arg output,float*,global
    .arg input,float*,global,const
    .arg dct8x8,float*,global,const
    .arg inter,float*,local
    .arg width,uint
    .arg blockWidth,uint
    .arg inverse,uint
    .userdata PTR_UAV_TABLE,0,2,2
    .userdata IMM_CONST_BUFFER,0,4,4
    .userdata IMM_CONST_BUFFER,1,8,4
    .text
/*befc03ff 00008000*/ s_mov_b32       m0, 0x8000
...
/*bf810000         */ s_endpgm
```
