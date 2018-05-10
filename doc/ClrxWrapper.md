## CLRadeonExtender CLRXWrapper

CLRXWrapper embeds and integrates the assembler into AMD Catalyst OpenCL implementation.
It make possible to call an assembler from OpenCL applications. Assembler will be called
when special build options will be added: `-xasm` or `-x asm`.

### Installation

By default, CLRXWrapper is not enabled. To enable CLRX wrapper few step should be done:

* remove `amdocl32.icd` or `amdocl64.icd` (if 64-bit systems) in `/etc/OpenCL/vendors`
directory
* write `clrx.icd` with content `libCLRXWrapper.so` in  `/etc/OpenCL/vendors`
directory

Before below steps, we recommend to make copy of `/etc/OpenCL/vendors` directory.

Installation on Windows:

* copy all CLRX libraries and CLRXWrapper into AMDAPP directory or OpenCL directory
* run RegEdit and go to `HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\OpenCL\Vendors\`
* remove `amdocl32.dll` or `amdocl64.dll` key
* add a DWORD `CLRXWrapper.dll` key with value 0
* additionally, set system environment variable to path to CLRX_AMDOCL_PATH dynamic library

WARNING: CLRXWrapper has been tested under Linux. This part of CLRadeonExtender can behave
very unexpectedly under Windows.

### Additional build options

* **-xasm**, **-x asm**

    compile program by using CLRX assembler

* **-D SYMBOL[=VALUE]**, **-defsym=SYMBOL[=VALUE]**

    Define symbol. Value is optional and if it is not given then assembler set 0 by default.
This option can be occurred many times to defining many symbols.

* **-I PATH**, **-includePath=PATH**

    Add an include path to search path list. Assembler begins search from current directory
and follows to next include paths.
This option can be occurred many times to adding many include paths.

* **-forceAddSymbols**

    Add all non-local symbols to binaries. By default any assembler does not add any symbols
to keep compatibility with original format.

* **-buggyFPLit**

    Choose old and buggy floating point literals rules (to 0.1.2 version)
for compatibility.

* **-oldModParam**

    Choose old modifier parametrization that accepts only 0 and 1 values (to 0.1.5 version)
for compatibility.

* **-noMacroCase**
    Do not ignore letter's case in macro names (by default is ignored).

* **-w**
    Do not print all warnings.

* **-legacy**
    Force use legacy AMD Catalyst OpenCL 1.2 binary format instead AMD OpemCL 2.0.

* **-policy=VERSION**
    Set CLRX assembler policy version.

### Environment variables

* CLRX_FORCE_ORIGINAL_AMDOCL=1|0 - enable forcing of the original AMDOCL
* CLRX_AMDOCL_PATH=PATH - set path to AMDOCL library

### Usage

Sample call: `clBuildProgram(program, num_devices, devices, "-xasm", NULL, NULL);`
