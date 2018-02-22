## About binary formats

Because the history of the OpenCL for AMD GPU's is enough complicated and chaotic,
I decided to write about used binary formats for the OpenCL programs.

All of these binary formats based on the Unix ELF binary format which widely used in
the Linux/Unix systems for storing executables, code objects and shared libraries.

### GPU drivers

* Windows Catalyst/Crimson driver

    This is standard propetariary driver for Windows. It provides the OpenCL 1.0-2.0
implementation both 32-bit and 64-bit.

* Linux Catalyst/Crimson driver (also, called as fglrx)

    This is standard propetariary driver for Windows. It provides the OpenCL 1.0-2.0
implementation both 32-bit and 64-bit.

* Linux AMDGPU-PRO hybrid driver

    This is mix of opensource driver and propetariary Linux driver. Curretly,
it provides OpenCL 1.2 implementation. 

* Radeon/RadeonSI/AMDGPU driver

    This is the opensource driver. The OpenCL implementation consists of the Mesa3D Clover,
libclc OpenCL library and LLVM compiler. Currently, the development of this driver was
abandoned by AMD. The ROCm OpenCL implementation used in new AMDGPU-PRO drivers to handle
Radeon VEGA devices.

* ROCm platform

  The new opensource (the almost components) platform for High Performance Computing.
Mainly, It is designed for high end hardware. This platform requires PCIExpress with
PCIE atomics.

### Binary formats

Now, we have four the various binary formats in usage:

* AMD OpenCL 1.2 binary format

    Used in Windows Catalyst/Crimson drivers and Linux Catalyst/Crimson and
Linux AMDGPU-PRO drivers. This is a legacy binary format that was used in older
Catalyst drivers (both Windows and Linux versions). Currently, this format used in
standard way only for first GCN generation (GCN 1.0) devices: Radeon HD 7xxx.
This format can be choosen by using '-legacy' option while building the OpenCL program.

* AMD OpenCL 2.0 binary format

    In the first Crimson drivers (both Windows and Linux) designed to handle the OpenCL 2.0
standard. However, this format was used for the OpenCL 1.2 programs in later drivers
(Windows Crimson and AMDGPU-PRO) for the next GCN generation devices
(Radeon Rx 2XX/3XX, Radeon R 4XX). In the first drivers this format is choosen
by setting OpenCL 2.0 standard (option '-cl-std=CL2.0'). For Windows Crimson drivers
this format is used for Radeon VEGA GPU's. This format uses AMD HSA binary code format
with AMD HSA kernel configuration.

* GalliumCompute (Mesa3D/Clover) binary format

    This is format for the opensource drivers in Mesa3D package for Linux and Unix systems.
It is used by Mesa3D Clover the OpenCL implementation. It is used for all AMD GPU's
since the Evergreen generation (Radeon HD 5xxx). We distinguish to kinds of this format:
old AMDGPU binary format with plain code, and AMD HSA binary format with AMD HSA kernel
configuration in the program code. For these kinds, various ABI are used.
The AMD HSA was introduced by LLVM 4.0.0.

* ROCm(-OpenCL) binary format

    Used in the ROCm platform and the ROCm-OpenCL. This format uses AMD HSA code format with
AMD HSA kernel configurations. In this format, the kernel informations about
kernel arguments and the kernel configurations is stored in the AMDGPU (ROCM) Metadata
written in the YAML data format.

### Bitness of the binary formats

Because these format can be used in both 32-bit and 64-bit environment, we describe
what these bitness for particular binary format means. Table of the bitness:

Binary format          | Format bitness  | Memory access bitness
-----------------------|-----------------|----------------------------------
AMD OpenCL 1.2         | 32/64-bit       | 32/64-bit
AMD OpenCL 2.0         | 32/64-bit       | 32/64-bit
GalliumCompute         | 32/64-bit       | 64-bit
ROCm                   | 64-bit          | 64-bit

Format bitness - bitness of the same binary format.  
Memory address bitness - bitness of the memory access.

### The kernel's call conventions (ABI)

The various drivers uses various kernel's call convention. The kernel call convetion
encompass the kernel argument's passinhg, the kernel setup
(global offset, work size, work group size) passing and way to pass constant data.
We call this as ABI.
The all drivers uses various call conventions. For example the machine code (only
instructions) for the AMD OpenCL 1.2 does not work under AMD OpenCL 2.0.
Moreover the old GalliumCompute does not work under new GalliumCompute with AMD HSA.

This is list of call conventions (ABI):

* AMD OpenCL 1.2 - used with AMD OpenCL 1.2 binary format
* AMD OpenCL 2.0 - used with AMD OpenCL 2.0 binary format
* old GalliumCompute - used with old GalliumCompute with plain code
* new GalliumCompute - used with new GalliumCompute with AMD HSA
* ROCm - used with ROCm binary format

### CLRXCLHelper

The additional library CLRXCLHelper automatically determines the binary format,
the bitness, the GPU device and the driver version from given the OpenCL device.
