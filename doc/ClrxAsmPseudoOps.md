## CLRadeonExtender Assembler Pseudo-Operations

The CLRX assembler accepts the almost pseudo-operations from GNU as.
This chapter lists and explains standard pseudo-operations.

A CLRX assembler stores values greater than byte in the little-endian ordering.

## List of the pseudo-operations

### .abort

Aborts compilation.

### .amd

This pseudo-operation should to be at begin of source.
Choose AMD Catalyst OpenCL program binary format.

### .align, .balign

Syntax: .align ALIGNMENT[, VALUE[, LIMIT]]  
Syntax: .balign ALIGNMENT[, VALUE[, LIMIT]]

Align current position to value of the first expression.
Value of that expression must be a power of two.
By default if section is writeable, fill 0's.
Second expression determines byte value that will be stored inside filling.
If third expression is not given, assembler accepts any length of the skip.
Third expression limits skip to own value. If any alignment needs to skip number
of the bytes greater than that value, then alignment will not be done.
If aligment will be done in `.text` section and second expresion will not be given, then
assembler fills no-operation instructions in that hole.

### .arch

Syntax: .arch ARCHITECTURE

This pseudo-operation should to be at begin of source. Set GPU architecture.
One of following architecture can be set: GCN1.0, GCN1.1, GCN1.2.

### .ascii

Syntax: .ascii STRING,....

Put ASCII string into current section. This pseudo-operations does not add the
null-terminated character. If more than one string will be given then all given
string will be concatenated.

### .asciz

Syntax: .asciz STRING,....

Put ASCII string into current section. This pseudo-operations adds the
null-terminated character. If more than one string will be given then all given
string will be concatenated.

### .balignw, .balignl

Syntax: .balignw ALIGNMENT[, VALUE[, LIMIT]]  
Syntax: .balignl ALIGNMENT[, VALUE[, LIMIT]]

Refer to `.align`. `.balignw` treats fill value as 2-byte word. `.balignl` treats
fill value as 4-byte word.

### .byte

Syntax: .byte ABS-EXPR,....

Put byte values into current section. If any expression is empty then an assembler stores
0 and warns about empty expression. If expression will give a value that can not be stored
in byte then an assembler warn about that.

### .data

Syntax: .data

Go to `.data` section or if that section doesn't exists create it.

### .double

Syntax: .double DOUBLE-VAL,...

Put double-precision floating point values into current section.
If no value between comma then an assembler stores 0 and warn about no value.
This pseudo-operation accepts only double precision floating point literals.

### .else

Part of the if-endif clause. Refer to `.if` pseudo-operation.
Code between `.else` and `.endif` will be performed if all previous conditions was not
satisified. Otherwise code will be skipped.

### .elseifXXX

Syntax: .elseif ABS-EXPR  
Syntax: .elseif32  
Syntax: .elseif64  
Syntax: .elseifarch ARCHITECTURE  
Syntax: .elseifb [PART-OF-LINE]  
Syntax: .elseifc STR1, STR2  
Syntax: .elseifdef SYMBOL  
Syntax: .elseifeq ABS-EXPR  
Syntax: .elseifeqs STRING  
Syntax: .elseiffmt BINFMT  
Syntax: .elseifge ABS-EXPR  
Syntax: .elseifgpu GPUDEVICE  
Syntax: .elseifgt ABS-EXPR  
Syntax: .elseifle ABS-EXPR  
Syntax: .elseiflt ABS-EXPR  
Syntax: .elseifnarch ARCHITECTURE  
Syntax: .elseifnb [PART-OF-LINE]  
Syntax: .elseifnc STR1, STR2  
Syntax: .elseifndef SYMBOL  
Syntax: .elseifne ABS-EXPR  
Syntax: .elseifnes STRING  
Syntax: .elseifnfmt BINFMT  
Syntax: .elseifngpu GPUDEVICE  
Syntax: .elseifnotdef SYMBOL

Part of the if-endif clause. Refer to `.if` pseudo-operation.
Code between `.else` and `.endif` will be performed if all previous conditions was not
satisified and condition to this pseudo-operation was satisfied. Otherwise code will be
skipped.

### .end

Ends source code compilation at this point.

### .endif

Finish if-endif clause.

### .endm

Finish macro definition

### .endr

Finish code of repetition.

### .equ, .set

Syntax: .equ SYMBOL, EXPR  
Syntax: .set SYMBOL, EXPR

Define symbol with specified value of the expression given in second operand. Symbol
defined by using these pseudo-operations can be redefined many times.

### .equiv

Syntax: .equiv SYMBOL, EXPR

Define symbol with specified value of the expression given in second operand.
Symbol defined by using `.equiv` can not be redefined. If symbol was already defined
this pseudo-operations causes an error.

### .eqv

Syntax: .eqv SYMBOL, EXPR

Define symbol with specified expression given in second operand.
The expression of symbol will be evaluated any time when symbol will be used.
This feature allow to change value of symbol indirectly by changing value of the symbols
used in expression. Example:

```
a = 3
b = 5
.eqv currentValue, a+b  # now currentValue is equal to 8
.int currentValue
b = 7
.int currentValue  # we changed b to 7, so we put 10
```

### .err

This pseudo-operation causes error at point where is encountered.

### .error

Syntax: .error STRING

This pseudo-operation causes error and print error given in string.

### .exitm

This pseudo-operation can be used only inside macro definitions. It does early exiting
from macro. Useful while using conditions inside a macros.

### .extern

Syntax: .extern SYMBOL,...

Set symbol as external. Ignored by GNU as and CLRX assembler.

### .fail

Syntax: .fail ABS-EXPR

If value of the expression is greater or equal to 500 then assembler print warnings.
Otherwise assembler print error and fails.

### .file

This pseudo-operation is ignored by CLRX assembler.

### .fill, .fillq

Syntax: .fill REPEAT[, SIZE[, VALUE]]  
Syntax: .fillq REPEAT[, SIZE[, VALUE]]

Store value many times. First expression defines how many times value will be stored.
Second expression defines how long is value. Third expression defines value to be stored.
If second expression is not given, then assembler assumes that is byte value.
If third expression is not given then assembler stores 0's. Assembler takes only
lowest 4-bytes of the value, and if size of value is greater than 4 then more significant
bytes will be assumed to be 0. The `.fillq` takes all bytes of the 64-bit value.
If value doesn't fit to size of value then it will be truncated.
Size of value can be 0. First expression too.

### .float, .single

Syntax: .float FLOAT-VAL,...  
Syntax: .single FLOAT-VAL,...

Put single-precision floating point values into current section.
If no value between comma then an assembler stores 0 and warn about no value.
This pseudo-operation accepts only single precision floating point literals.

### .format

Syntax: .format BINFORMAT

This pseudo-operation should to be at begin of source.
Choose binary format. Binary can be one of following list:

* `amd`, `catalyst` - AMD Catalyst OpenCL binary format
* `gallium` - the GalliumCompute binary format
* `raw` - rawcode (raw program instructions)

### .global, .globl

Syntax: .global SYMBOL,...  
Syntax: .globl SYMBOL,...

Indicates that symbols will be a global. Global symbol can be used
across many object files. The CLRX assembler can expose global symbols at output binary if
adding sybols will be enabled.

### .gpu GPUDEVICE

Syntax: .gpu GPUDEVICE

This pseudo-operation should to be at begin of source. Set GPU device.
One of following device type can be set:
CapeVerde, Pitcairn, Tahiti, Oland, Bonaire, Spectre, Spooky, Kalindi,
Hainan, Hawaii, Iceland, Tonga, Mullins, Fiji and Carrizo.

### .half

Syntax: .half HALF-VAL,...

Put half-precision floating point values into current section.
If no value between comma then an assembler stores 0 and warn about no value.
This pseudo-operation accepts only half precision floating point literals.

### .hword

Syntax: .hword ABS-EXPR,....

Put 2-byte word values into current section. If any expression is empty then an assembler
stores 0 and warns about empty expression. If expression will give a value that can not be
stored in 2-byte word then an assembler warn about that.

### .ifXXX

Syntax: .if ABS-EXPR  
Syntax: .if32  
Syntax: .if64  
Syntax: .ifarch ARCHITECTURE  
Syntax: .ifb [PART-OF-LINE]  
Syntax: .ifc STR1, STR2  
Syntax: .ifdef SYMBOL  
Syntax: .ifeq ABS-EXPR  
Syntax: .ifeqs STRING  
Syntax: .iffmt BINFMT  
Syntax: .ifge ABS-EXPR  
Syntax: .ifgpu GPUDEVICE  
Syntax: .ifgt ABS-EXPR  
Syntax: .ifle ABS-EXPR  
Syntax: .iflt ABS-EXPR  
Syntax: .ifnarch ARCHITECTURE  
Syntax: .ifnb [PART-OF-LINE]  
Syntax: .ifnc STR1, STR2  
Syntax: .ifndef SYMBOL  
Syntax: .ifne ABS-EXPR  
Syntax: .ifnes STRING  
Syntax: .ifnfmt BINFMT  
Syntax: .ifngpu GPUDEVICE  
Syntax: .ifnotdef SYMBOL

Open if-endif clause. After that pseudo-op can encounter `.elseifXX` or `.else`
pseudo-operations. If condition of that pseudo-operations is satisfied then
code will be performed.

List of the `.if` kinds:

* `.if` - perform code if value is not zero.
* `.if32` - perform code if 32-bit binary.
* `.if64` - perform code if 64-bit binary.
* `.ifarch` - perform code if specified architecture was set.
* `.ifb` - perform code if all character after name of this pseudo-op is blank.
* `.ifc` - compare two unquoted strings. If are equal then perform code.
* `.ifdef` - perform code if symbol was defined
* `.ifeq` - perform code if value is zero.
* `.ifeqs` - perform code if two quoted string are equal.
* `.iffmt` - perform code if specified binary format was set.
* `.ifge` - perform code if value is greater or equal to zero.
* `.ifgpu` - perform code if specified GPU device type was set.
* `.ifgt` - perform code if value is greater than zero.
* `.ifle` - perform code if value is less or equal to zero.
* `.iflt` - perform code if value is less than zero.
* `.ifnarch` - perform code if specified architecture was not set.
* `.ifnb` - perform code if any character after name of this pseudo-op is not blank.
* `.ifnc` - compare two unquoted strings. If are not equal then perform code.
* `.ifndef` - perform code if symbol was not defined.
* `.ifne` - perform code if value is not zero.
* `.ifnes` - perform code if two quoted string are not equal.
* `.ifnfmt` - perform code if specified binary format was not set.
* `.ifngpu` - perform code if specified GPU device type was not set.
* `.ifnotdef` - perform code if symbol was not defined.

### .incbin

Syntax: .incbin FILENAME[, OFFSET[, COUNT]]

Append the binary file into currenct section. If file not found in the current directory
then assembler searches file in the include paths. If file not found again then assembler
prints error. Second optional argument defines offset (how many bytes should to be skipped).
By default assembler begin appending from first byte.
Third argument defines maximum number bytes to append. By default all data from binary
will be appended.

### .include

Syntax: .include FILENAME

Include new source code file and begins assemblying from this file.
An assembler automatically returns to previous file if encounters end of the that file.
If file not found in the current directory then assembler searches file in the
include paths. If file not found again then assembler prints error.

### .irp

Syntax: .irp NAME, STRING ...

### .int, .long

Syntax: .int ABS-EXPR,....  
Syntax: .long ABS-EXPR,....

Put 4-byte word values into current section. If any expression is empty then an assembler
stores 0 and warns about empty expression. If expression will give a value that can not be
stored in 4-byte word then an assembler warn about that.

### .ln, .line

These pseudo-operations are ignored by CLRX assembler.

### .local