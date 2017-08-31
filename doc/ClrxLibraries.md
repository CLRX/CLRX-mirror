## CLRX Libraries

The assembler and disassembler program are linked with CLRadeonExtender libraries.
These libraries can be used easily to embed an assembler or a disassembler in user/developer
programs and libraries. CLRadeonExtender provides three libraries:

* CLRXAmdAsm - main assembler and disassembler library
* CLRXAmdBin - main library to read and write program binaries
* CLRXUtils - utility library

The CLRXAmdAsm library needs CLRXAmdBin and CLRXUtils libraries. The CLRXAmdBin
library needs CLRXUtils libraries.

In Linux/Unix systems program that uses CLRX libraries need to be linked with `dl` library,
thread (pthread) library and C++11 standard STL library.

In Windows systems program that uses CLRX libraries need to be linked with `shell32.lib` library.

