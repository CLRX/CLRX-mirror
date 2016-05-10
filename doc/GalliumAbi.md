## Gallium ABI description

This chapter describes how kernel gets its argument, how access to constant data.

In this chapter, size is given in dwords. Dword is 4-byte value.

### Argument passing

Argument is stored in memory which address is stored in s[0:1].
Argument begins from 9 dword. First 9 dwords are:

* 0-2 - number of groups for each dimension
* 3-5 - global size for each dimension
* 6-8 - local size for each dimension

Argument griddim holds number of dimensions. Argument gridoffset holds 3 values of the
global offset.

Userdata tooks 4 first scalar registers and holds:

* s[0:1] - address to argument list and kernel setup
* s[2:3] - scratch address

### Other data and resources

Section '.rodata' ('.globaldata') hold constant data for kernels.
Constant data is placed after code of kernels. Use PC pointer to get this data.
