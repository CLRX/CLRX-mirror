## CLRadeonExtender

This is mirror of the CLRadeonExtender project.

Original site is here [https://clrx.nativeboinc.org](https://clrx.nativeboinc.org) or
[http://clrx.nativeboinc.org](http://clrx.nativeboinc.org)
if SSL certificate doesn't work.

CLRadeonExtender provides tools to develop software in low-level for the Radeon GPU's
compatible with GCN 1.0/1.1/1.2/1.4 (AMD VEGA) architecture. Since version 0.1.8 also AMD VEGA
Deep Learning extensions has been supported.
Currently, this project have two tools to develop that software:

* clrxasm - the GCN assembler
* clrxdisasm - the GCN disassembler

Both tools can operate on four binary formats:

* the AMD Catalyst OpenCL program binaries
* the GalliumCompute (Mesa) program binaries
* the AMD Catalyst OpenCL 2.0 (new AMD format) program binaries
* the ROCm (RadeonOpenCompute) program binaries

CLRadeonExtender not only provides basic tools to low-level development, but also
allow to embed own assembler with AMD Catalyst driver through CLwrapper.
An embedded assembler can be called from `clBuildProgram` OpenCL call
with specified option `-xasm`. Refer to README and INSTALL to learn about CLRXWrapper.

### System requirements

CLRadeonExtender requires:

* C++11 compliant compiler (Clang++ or GCC 4.7 or later, MSVC 2015 or later)
* any build system supported by CMake (GNU make, NMake, Visual Studio, ...)
* CMake system (2.8 or later)
* Threads support (for Linux, recommended NPTL)
* Unix-like (Linux or BSD) system or Windows system

Optionally, CLRXWrapper requires:

* libOpenCL.so or OpenCL.dll
* OpenCL headers
* OpenGL headers (to 0.1.5 version)
* OpenCL ICD (for example from AMD Catalyst driver)
* AMD Catalyst driver or AMDGPU-PRO driver.

and documentation requires:

* pod2man utility for Unix manuals
* markdown_py utility for CLRX Documentation
* Doxygen for CLRX API Documentation

### Compilation

To build system you should create a build directory in source code package:

```
mkdir build
```

and run:

```
cmake .. [cmake options]
```

Optional CMake configuration options for build:

* CMAKE_BUILD_TYPE - type of build (Release, Debug, GCCSan, GCCSSP).
* CMAKE_INSTALL_PREFIX - prefix for installation (for example '/usr/local')
* BUILD_32BIT - build 32-bit binaries (works only in the Unix/Linux 64-bit environment)
* BUILD_TESTS - build all tests
* BUILD_SAMPLES - build OpenCL samples
* BUILD_DOCUMENTATION - build project documentation (doxygen, unix manuals, user doc)
* BUILD_DOXYGEN - build doxygen documentation
* BUILD_MANUAL - build Unix manual pages
* BUILD_CLRXDOC - build CLRX user documentation
* BUILD_STATIC_EXE - build with statically linked executables
* GCC5CXX11NEWABI - build with new GCC5 C++11 ABI (required if GCC uses old ABI by default)
* NO_STATIC - no static libraries
* NO_CLWRAPPER - do not build CLRXWrapper
* CPU_ARCH - target CPU architecture (in GCC parameter to -march, for MSVC
  parameter to /arch:)
* OPENCL_DIST_DIR - an OpenCL directory distribution installation (optional)

You can just add one or many of these options to cmake command:

```
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
```

or for Microsoft Visual C++ (for NMake):

```
cmake .. -G "NMake Makefiles" [cmake configuration options]
```

or for Microsoft Visual C++ (for same Visual Studio)

```
cmake .. -G "Visual Studio XXXXXX [arch]" [cmake configuration options]
```

where XXXX - version of Visual Studio (7, 14 2015, ...).
arch - architecture (Win64, ARM)  (optional).

After creating Makefiles scripts you can compile project:

`make` or `make -jX` - where X is number of processors.

or (for NMake)

`nmake`

or just execute build option in Visual Studio.

After building you can check whether project is working (if you will build tests):

```
ctest
```

Creating documentation will be done by this command
(if you will enable a building documentation, required for version 0.1):

```
make Docs
```

or (for NMake)

```
nmake Docs
```

#### FreeBSD

Due to unknown reasons, the compilation under clang++ will be failed. We recommend to use
gcc compiler to build the CLRadeonExtender. You should prepend cmake command by `CXX=g++`:
`CXX=g++ cmake .. ....`.


### Installation

Installation is easy. Just run command:

```
make install
```

or (for NMake):

```
nmake install
```

### Usage of libraries in binaries

The default (without '-gcc5' in name) binary libraries for Linux are compiled
for C++11 old ABI, hence you must add option -D_GLIBCXX_USE_CXX11_ABI=0 to
compiler commands if you are using GCC 5.0 or higher or compiler that by default
uses new C++11 ABI.

### Usage

Usage of the clrxasm is easy:

```
clrxasm [-o outputFile] [options] [file ...]
```

If no file specified clrxasm read source from standard input.

Useful options:

* -g DEVICETYPE - device type ('pitcairn', 'bonaire'...)
* -A ARCH - architecture ('gcn1.0', 'gcn1.1', 'gcn1.2' or 'gcn1.4')
* -b BINFMT - binary format ('amd', 'amdcl2', 'gallium', 'rocm', 'rawcode')
* -t VERSION - driver version for which a binary will be generated
* -w - suppress warnings

Usage of the clrxdisasm:

```
clrxdisasm [options] [file ...]
```

and clrxdisasm will print a disassembled code to standard output.

Useful options for clrxdisasm:

* -a - print everything (not only code, but also kernels and their metadatas)
* -f - print floating points
* -h - print hexadecimal instruction codes
* -C - print configuration dump instead metadatas, CALnotes and setup data
* -g DEVICETYPE - device type ('pitcairn', 'bonaire'...)
* -A ARCH - architecture ('gcn1.0', 'gcn1.1', 'gcn1.2' or 'gcn1.4')
* -t VERSION - driver version for which a binary will be generated

A CLRX assembler accepts source from disassembler.
