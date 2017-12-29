## Gallium ABI description

This chapter describes how kernel gets its argument, how access to constant data.

In this chapter, size is given in dwords. Dword is 4-byte value.

### Argument passing

Arguments are stored in memory which address is stored in s[0:1].
Arguments begins from 9 dword. First 9 dwords are:

* 0-2 - number of groups for each dimension
* 3-5 - global size for each dimension
* 6-8 - local size for each dimension

An argument griddim holds number of dimensions. Argument gridoffset holds 3 values of the
global offset.

Userdata tooks 4 first scalar registers and holds:

* s[0:1] - address to argument list and kernel setup
* s[2:3] - scratch address

### Other data and resources

The section '.rodata' ('.globaldata') hold constant data for kernels.
The constant data is placed after code of kernels. Use PC pointer to get this data.

## Gallium ABI description AMDHSA

### Argument passing

Arguments are stored in memory which address is stored in s[6:7]. Arguments begins from
the first dword in this memory. After kernel arguments are kernel dimensions.
List of data (number is dword offset after kernel arguments):

* 0 - number of dimensions
* 1-3 - global offsets for each dimensions

Local sizes and other kernel setup is in the memory which address is stored in s[4:5]. 
List of data (number is dword offset after kernel argument):

* 1 - low 16-bits is global local size for X dimension, higher 16-bits is for Y dimension
* 2 - low 16-bits is global size for Z dimension
* 3-5 - global size for each dimension

Userdata tooks 8 first scalar registers and holds:

* s[0:3] - scratch buffer resource
* s[4:5] - kernel setup
* s[6:7] - address to argument list and kernel dimensions and global offsets
