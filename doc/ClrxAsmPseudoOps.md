## CLRadeonExtender Assembler Pseudo-Operations

The CLRX assembler accepts the almost pseudo-operations from GNU assembler.
This chapter lists and explains standard pseudo-operations.

A CLRX assembler stores values greater than byte in the little-endian ordering.

## List of the pseudo-operations

### .32bit

This pseudo-operation should to be at begin of source.
Choose 32-bit binaries (it have meaningful for the AMD Catalyst and GalliumCompute
binary format). For AMD Catalyst OpenCL 1.2 and 2.0 format, it determines
bitness of address. For GalliumCompute, it determines bitness of inner ELF binary.
For ROCm binary format, bitness is ignored.

### .64bit

This pseudo-operation should to be at begin of source.
Choose 64-bit binaries (it have meaningful for the AMD Catalyst and GalliumCompute
binary format). For AMD Catalyst OpenCL 1.2 and 2.0 format, it determines
bitness of address. For GalliumCompute, it determines bitness of inner ELF binary.
For ROCm binary format, bitness is ignored.

### .abort

Aborts compilation.

### .align, .balign

Syntax: .align ALIGNMENT[, [VALUE] [, LIMIT]]  
Syntax: .balign ALIGNMENT[, [VALUE] [, LIMIT]]

Align current position to value of the first expression.
Value of that expression must be a power of two.
By default if section is writeable, fill 0's.
Second expression determines byte value that will be stored inside filling.
If third expression is not given, assembler accepts any length of the skip.
Third expression limits skip to own value. If any alignment needs to skip number
of the bytes greater than that value, then alignment will not be done.
If aligment will be done in `.text` section and second expresion will not be given, then
assembler fills no-operation instructions in that hole.

### .altmacro

Enable alternate macro syntax. This mode enables following features:

* new macro substitution without backslash:

```
.macro test1 a b
    .int a, b
.endm
test1 12,34     # put 12 and 34 integer value
```

* evaluating expression as string in macro arguments:

```
.macro stringize a,b,c
    .string "a, b, c"
.endm
stringize %12|33, %43*5, %12-65 # generate string "45, 215, -53"
```

* new string quoting in macro arguments (by triagular brackets '<' and '>' or by single
quote ' or double quote "). Also, enables new string escaping by '!'.

```
test1 <this is test !<!>>  # put "this is test <>" string to first macro argument
test1 "this is test <>"  # put "this is test <>" string to first macro argument
test1 'this is test !''  # put "this is test '" string to first macro argument
```

* local symbol names. 'local name' defines new unique symbol name for name. If any name
will be occurred then that unique name will substituted.

```
local myName    # myName substitutes new unique name
myName:     # define new label with unique name
```

An alternate macro syntax does not disable any standard macro syntax features likes
macro substitution via backslashes, '\@'.

### .amd

This pseudo-operation should to be at begin of source.
Choose AMD Catalyst OpenCL 1.2 program binary format.

### .amdcl2

This pseudo-operation should to be at begin of source.
Choose AMD Catalyst new (introduced for OpenCL 2.0 support) program binary format.

### .arch

Syntax: .arch ARCHITECTURE

This pseudo-operation should to be at begin of source. Set GPU architecture.
One of following architecture can be set:
SI, VI, CI, VEGA, VEGA20, GFX6, GFX7, GFX8, GFX9, GFX906, GCN1.0, GCN1.1, GCN1.2,
GCN1.4 and GCN1.4.1.

### .ascii

Syntax: .ascii "STRING",....

Emit ASCII string. This pseudo-operations does not add the
null-terminated character. If more than one string will be given then all given
string will be concatenated.

### .asciz

Syntax: .asciz "STRING",....

Emit ASCII string. This pseudo-operations adds the
null-terminated character. If more than one string will be given then all given
string will be concatenated.

### .balignw, .balignl

Syntax: .balignw ALIGNMENT[, [VALUE] [, LIMIT]]  
Syntax: .balignl ALIGNMENT[, [VALUE] [, LIMIT]]

Refer to `.align`. `.balignw` treats fill value as 2-byte word. `.balignl` treats
fill value as 4-byte word.

### .buggyfplit

Enable old and buggy behavior for floating point literals and constants.

### .byte

Syntax: .byte ABS-EXPR,....

Emit byte values. If any expression is empty then an assembler stores
0 and warns about empty expression. If expression will give a value that can not be stored
in byte then an assembler warn about that.

### .data

Go to `.data` section. If this section doesn't exist assembler create it.

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
Syntax: .elseifb STRING  
Syntax: .elseifc STRING1, STRING2  
Syntax: .elseifdef SYMBOL  
Syntax: .elseifeq ABS-EXPR  
Syntax: .elseifeqs "STRING1","STRING2"  
Syntax: .elseiffmt BINFMT  
Syntax: .elseifge ABS-EXPR  
Syntax: .elseifgpu GPUDEVICE  
Syntax: .elseifgt ABS-EXPR  
Syntax: .elseifle ABS-EXPR  
Syntax: .elseiflt ABS-EXPR  
Syntax: .elseifnarch ARCHITECTURE  
Syntax: .elseifnb STRING  
Syntax: .elseifnc STRING1, STRING2  
Syntax: .elseifndef SYMBOL  
Syntax: .elseifne ABS-EXPR  
Syntax: .elseifnes "STRING1","STRING2"  
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

### .endm, .endmacro

Finish macro definition

### .endr, .endrept

Finish code of repetition.

### .ends, .endscope

Close visibility's scope.

### .enum

Syntax: .enum [>[STARTPOS],] SYMBOL,....

Simplify defining the enumerations. For every symbol,
define symbol with value of enumeration counter and increase an enumeration counter.
Defined symbols can not be assigned later. Optional `>` with optional STARTPOS sets
enumeration counter to STARTPOS value or zero (if no STARTPOS given).
Every scope has own enumeration counter. This features simplify
joining enumerations with scopes.

Examples:

```
.enum OK,BADFD,FATAL        # define OK=0, BADFD=1, FATAL=2
.enum >8, BitOne,BitTwo     # define BitOne=8, BitTwo=9
.enum HALO                  # define HALO=10
.scope Result
    # enum counter is zero in this scope
    .enum NONE,FULL,INVALID   # NONE=0, FULL=1, INVALID=2
.ends
.enum >, myzero             # myzero=0
```

### .equ, .set

Syntax: .equ SYMBOL, EXPR|REG  
Syntax: .set SYMBOL, EXPR|REG

Define symbol with specified value of the expression or register or register range
given in second operand. Symbol defined by using these pseudo-operations can be
redefined many times.

### .equiv

Syntax: .equiv SYMBOL, EXPR|REG

Define symbol with specified value of the expression or register or register range
given in second operand given in second operand.
Symbol defined by using `.equiv` can not be redefined. If symbol was already defined
this pseudo-operations causes an error.

### .eqv

Syntax: .eqv SYMBOL, EXPR|REG

Define symbol with specified expression or register or register range
given in second operand given in second operand.
Symbol defined by using `.eqv` can not be redefined.
If symbol was already defined this pseudo-operations causes an error.
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

Syntax: .error "STRING"

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

Syntax: .fill REPEAT[, [SIZE] [, VALUE]]  
Syntax: .fillq REPEAT[, [SIZE] [, VALUE]]

Emit value many times. First expression defines how many times value will be stored.
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

Emit single-precision floating point values. If no value between comma then an
assembler stores 0 and warn about no value.
This pseudo-operation accepts only single precision floating point literals.

### .for

Syntax: .for SYMBOL=INITVALUE, COND-EXPR, NEXT-EXPR

Open 'for' repetition. Before repetition, SYMBOL is initialized by INITVALUE.
For every repetition SYMBOL value will be replaced by value evaluted by NEXT-EXPR.
The code between this pseudo-operation and `.endr` will be repeated
until COND-EXPR returns zero. Example:

```
.for x=1,x<16,x+x
    .int x
.endr
```

generates:

```
.int x      # x=1
.int x      # x=2
.int x      # x=4
.int x      # x=8
```

### .format

Syntax: .format BINFORMAT

This pseudo-operation should to be at begin of source.
Choose binary format. Binary can be one of following list:

* `amd`, `catalyst` - AMD Catalyst OpenCL 1.2 binary format
* `amdcl2` - AMD Catalyst new (introduced for OpenCL 2.0) binary format
* `gallium` - the GalliumCompute binary format
* `rocm` - the ROCm binary format
* `raw` - rawcode (raw program instructions)

### .gallium

This pseudo-operation should to be at begin of source.
Choose GalliumCompute OpenCL program binary format.

### .get_64bit

Syntax: .get_64bit SYMBOL

Store 64-bitness in specified symbol. Store 1 to symbol if 64-bit mode enabled.

### .get_arch

Syntax: .get_arch SYMBOL

Store GPU architecture identifier to symbol. List of architecture ids:

Id | Description
---|-------------------------------------------
 0 | GCN1.0 (Pitcairn, Tahiti)
 1 | GCN1.1 (Bonaire, Hawaii)
 2 | GCN1.2 (Tonga, Fiji, Ellesmere)
 3 | VEGA (AMD RX VEGA)
 4 | VEGA20 (GFX906)

### .get_format

Syntax: .get_format SYMBOL

Store binary format identifier to symbol. List of format ids:

Id | Description
---|-------------------------------------------
 0 | AMD OpenCL 1.2 binary format
 1 | Gallium Compute binary format
 2 | Raw code
 3 | AMD OpenCL 2.0 (new driver) binary format
 4 | ROCm binary format

### .get_gpu

Syntax: .get_gpu SYMBOL

Store GPU device identifier to symbol. List of GPU device ids:

Id  | Description
----|-------------------------------------------
 0  | Cape Verde (Radeon HD 7700)
 1  | Pitcairn (Radeon HD 7850)
 2  | Tahiti  (Radeon HD 7900)
 3  | Oland
 4  | Bonaire  (Radeon R7 260)
 5  | Spectre
 6  | Spooky
 7  | Kalindi
 8  | Hainan
 9  | Hawaii (Radeon R9 290)
 10 | Iceland
 11 | Tonga (Radeon R9 285)
 12 | Mullins
 13 | Fiji (Radeon Fury)
 14 | Carrizo
 15 | Dummy
 16 | Goose
 17 | Horse
 18 | Stoney
 19 | Ellesmere (Radeon RX 470/480)
 20 | Baffin (Radeon 460)
 21 | gfx804
 22 | gfx900 (Radeon RX VEGA)
 23 | gfx901 (Radeon RX VEGA)
 24 | gfx902 (Radeon RX VEGA)
 25 | gfx903 (Radeon RX VEGA)
 26 | gfx904 (Radeon RX VEGA)
 27 | gfx905 (Radeon RX VEGA)
 28 | gfx906 (Radeon VEGA 20)
 29 | gfx907 (Radeon VEGA 20 ???)

### .get_policy

Syntax: .get_version SYMBOL

Store current CLRX policy version to symbol. Version stored as integer in form:
`major_version*10000 + minor_version*100 + micro_version`.

### .get_version

Syntax: .get_version SYMBOL

Store CLRadeonExtender version to symbol. Version stored as integer in form:
`major_version*10000 + minor_version*100 + micro_version`.

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
Baffin, Bonaire, CapeVerde, Carrizo, Dummy, Ellesmere, Fiji, GFX700, GFX701, GFX801,
GFX802, GFX803, GFX804, GFX900, GFX901, GFX902, GFX903, GFX904, GFX905, GFX906, GFX907,
Goose, Hainan, Hawaii, Horse, Iceland, Kalindi, Mullins, Oland, Pitcairn, Polaris10,
Polaris11, Polaris12, Polaris20, Polaris21, Polaris22, Raven, Spectre, Spooky, Stoney,
Tahiti, Tonga, Topaz, Vega10, Vega11, Vega12 and Vega20.

### .half

Syntax: .half HALF-VAL,...

Emit half-precision floating point values.
If no value between comma then an assembler stores 0 and warn about no value.
This pseudo-operation accepts only half precision floating point literals.

### .hword, .short

Syntax: .hword ABS-EXPR,....
Syntax: .short ABS-EXPR,....

Emit 2-byte word values. If any expression is empty then an assembler
stores 0 and warns about empty expression. If expression will give a value that can not be
stored in 2-byte word then an assembler warn about that.

### .ifXXX

Syntax: .if ABS-EXPR  
Syntax: .if32  
Syntax: .if64  
Syntax: .ifarch ARCHITECTURE  
Syntax: .ifb STRING  
Syntax: .ifc STRING1, STRING2  
Syntax: .ifdef SYMBOL  
Syntax: .ifeq ABS-EXPR  
Syntax: .ifeqs "STRING1","STRING2"  
Syntax: .iffmt BINFMT  
Syntax: .ifge ABS-EXPR  
Syntax: .ifgpu GPUDEVICE  
Syntax: .ifgt ABS-EXPR  
Syntax: .ifle ABS-EXPR  
Syntax: .iflt ABS-EXPR  
Syntax: .ifnarch ARCHITECTURE  
Syntax: .ifnb STRING  
Syntax: .ifnc STRING1, STRING2  
Syntax: .ifndef SYMBOL  
Syntax: .ifne ABS-EXPR  
Syntax: .ifnes "STRING1","STRING2"  
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

NOTE: For `ifarch` or `.ifnarch`:
The assembler assumes that VEGA20 is this same ISA as VEGA10 (GFX9)
and if you set VEGA20 GPU architecture and if you put GFX9 architecture in `.ifarch`
then code for this clause will be assembled (or will not be assembled for `.ifnarch`).

### .incbin

Syntax: .incbin FILENAME[, [OFFSET] [, COUNT]]

Append the binary file into currenct section. If file not found in the current directory
then assembler searches file in the include paths. If file not found again then assembler
prints error. Second optional argument defines offset (how many bytes should to be skipped).
By default assembler begin appending from first byte.
Third argument defines maximum number bytes to append. By default all data from binary
will be appended.

### .include

Syntax: .include "FILENAME"

Include new source code file and begins assemblying from this file.
An assembler automatically returns to previous file if encounters end of the that file.
If file not found in the current directory then assembler searches file in the
include paths. If file not found again then assembler prints error.

### .irp

Syntax: .irp NAME, STRING,...  
Syntax: .irp NAME STRING ...

Begin repetition with the named variable. Nth occurrence of name will be replaced by
nth string. The name of variable must be preceded by `\` character.

Sample usage:

```
.irp reg v0,v1,v4,v5
    v_mul_f32 v10,\reg
.endr
```

produces:

```
    v_mul_f32 v10,v0
    v_mul_f32 v10,v1
    v_mul_f32 v10,v4
    v_mul_f32 v10,v5
```

Alternate syntax does not require a comma separator between strings and variable name.
Rules regarding to substituting variables are same as in macro substitution. Refer to
`.macro` pseudo-operation.

### .irpc

Syntax: .irpc NAME, STRING

Begin repetition with the named variable.Nth occurrence of name will be replaced by
nth character of the string (second operand).
The name of variable must be preceded by `\` character.

```
.irp reg 0145
    v_mul_f32 v10,v\reg
.endr
```

produces:

```
    v_mul_f32 v10,v0
    v_mul_f32 v10,v1
    v_mul_f32 v10,v4
    v_mul_f32 v10,v5
```

Rules regarding to substituting variables are same as in macro substitution. Refer to
`.macro` pseudo-operation.

### .int, .long

Syntax: .int ABS-EXPR,....  
Syntax: .long ABS-EXPR,....

Emit 4-byte word values. If any expression is empty then an assembler
stores 0 and warns about empty expression. If expression will give a value that can not be
stored in 4-byte word then an assembler warn about that.

### .kernel

Syntax: .kernel KERNELNAME

Create new kernel with default section or move to an existing kernel and
its default section. Types of section which can to be belonging to kernel depends on
the binary format.

### .lflags

This pseudo-operation is ignored by CLRX assembler.

### .ln, .line

These pseudo-operations are ignored by CLRX assembler.

### .local

Syntax: .local SYMBOL,...

Indicates that symbols will be a local. Local symbol are not visible in other
binary objects and they can be used only locally.

### .macro

Syntax: .macro MACRONAME, ARG,...  
Syntax: .macro MACRONAME ARG ...

Begin macro definition. The macro is recorded code that can be used later in source code.
Macro can accepts one or more arguments that will be substituted in its body.
Occurrence of the argument must be preceded by `\` character.

Special case of substitution is `\@` that replaced by number of the macro substitution.
Sometimes substitution must be joined with other name. In this case '\()` can be used to
split argument occurrence and other name. Example:

```
.macro macro1, x
.set \x\()Symbol, 10 # set symbol Xsymbol to value 10
.endm
macro1 Linux # define LinuxSymbol=10
```

That substitution is useful to create labels and symbols that can not be redefined.
Value of the argument is string. Optionally, argument can have the default value
which will be used if no argument value is not given in a macro call.

List of the argument definition:

* `arg1` - simple argument
* `arg1=value` - argument with default value
* `arg1:req` - required argument
* `arg:vararg` - variadic argument, get rest of the arguments and treats them as
single (must be last)
* `arg1:vararg:req` - required argument and variadic argument

A macro calling is easy:

```
somemacro
somemacro1 1,2,4
somemacro1 1 2 3
/* use variadic argument */
.macro putHeaderAndData x,data:vararg
    .byte \x&0x3f, \data
.endm
putHeaderAndData 0xfe,1,4,6,7,110
```

Some simple examples:

```
.macro putTwoBytes value1,value2
    .byte \value1+\value2, \value2-\value1
.endm
putTwoBytes 4,7 # generates .byte 11,3
.macro putTwoBytes2 value1=100,value2
    .byte \value1+\value2, \value2-\value1
.endm
putTwoBytes2 ,7 # generates .byte 107,93
```

The argument substitution works inside double-quoted strings:

```
.macro putVersionInfo major,minor,micro
.ascii "CLRadeonExtender \major\().\minor\().\micro"
.endm
putVersionInfo 1,2,3 # generates string "CLRadeonExtender 1.2.3"
```

It is possible to create the nested definitions like that:

```
.macro generators m,value1,value2
    .macro gen\m x1,x2  # define genM with two arguments
        /* use first and second operand of parent macro, and use two
         * operands from this macro */
        .int (\x1+\x2)*(\value1+\value2) 
    .endm    
.endm
/* define genF1 */
generators F1,2,3
/* use genF1, generates (10+12)*(2+3) -> 22*5 -> 110 */
genF1 10,12
```

If macro argument starts with '\%' then rest of macro argument will be evaluated as
expression and its results will be stored in string form (likewise '%EXPR' evaluation if
alternate macro syntax is enabled).

### .macrocase

Enable ignoring letter's case in macro names (default behaviour).

### .main

Go to main binary over binary of the kernel.

### .noaltmacro

Disables alternate macro syntax.

### .nobuggyfplit

Disable old and buggy behavior for floating point literals and constants.

### .nomacrocase

Disable ignoring letter's case in macro names.

### .nooldmodparam

Disable old modifier parametrization that accepts only 0 and 1 values (to 0.1.5 version).

### .octa

Syntax: .octa OCTA-LITERAL,...

Emit 128-bit word values. If no value between comma then an assembler stores 0 and warn
about no value. This pseudo-operation accepts only 128-bit word literals.

### .offset, .struct

Syntax: .offset ABS-EXPR  
Syntax: .struct ABS-EXPR

Set the output counter to some place in absolute section. Useful to defining
fields of the structures.

### .oldmodparam

Enable old modifier parametrization that accepts only 0 and 1 values (to 0.1.5 version)
for compatibility.

### .org

Syntax: .org EXPRESSION

Set the output counter to place defined by expression. Expression can point to some
place in the code or some place to absolute section (absolute value). If a expression
points to some place of a code, moving backwards is illegal.

### .p2align

Syntax: .p2align POWOF2ALIGN[, [VALUE] [, LIMIT]]

Refer to `.align`. First argument is power of two of the alignment instead of
same alignment.

### .policy

Syntax: .policy VERSION

Set current CLRX assembler's policy version. Version stored as integer in form:
`major_version*10000 + minor_version*100 + micro_version`. This number controls
behaviour of the same assembler for some things like  SGPRs counting. Refer to
[Asssembler policy](ClrxAsmPolicy).

### .print

Syntax: .print "STRING"

Print string into standard output (or defined print output).

### .purgem

Syntax: .purgem MACRONAME

Undefine macro MACRONAME.

### .quad

Syntax: .quad ABS-EXPR,....

Emit 8-byte word values. If any expression is empty then an assembler
stores 0 and warns about empty expression.

### .rawcode

This pseudo-operation should to be at begin of source.
Choose raw code (same processor's instructions).

### .regvar

Syntax: .regvar REGVAR:REGTYPE:REGSNUM, ...

Define new register variable (UNIMPLEMENTED).

### .rept

Syntax: .rept ABS-EXPR

Open repetition. The code between this pseudo-operation and `.endr` will be repeated
number given in first argument. Zero value is legal in first argument. Example:

```
.rept 3
v_nop
.endr
```

generates:

```
v_nop
v_nop
v_nop
```

### .rocm

This pseudo-operation should to be at begin of source.
Choose ROCm program binary format.

### .rodata

Go to `.rodata` section. If this section doesn't exist assembler create it.

### .sbttl, .tittle, .version

Syntax: .sbttl "STRING"  
Syntax: .title "STRING"  
Syntax: .version "STRING"

These pseudo-operations are ignored by CLRX assembler.

### .scope

Syntax .scope [SCOPENAME]

Open visbility's scope (if no name specified, then temporary scope).
The labels (except local labels), symbols, scopes and regvars are defined
inside scopes and visible inside them. The assembler create always global scope at begin.
If scope doesn't exists then will be created. The nested scopes are allowed even
if parent scope is temporary scope. The opened scope have parent that is previous scope.
Temporary scopes exists until first close.

### .section

Syntax: .section SECTIONNAME[, "FLAGS"[, @TYPE]] [align=ALIGN]

Go to specified section SECTIONNAME. If section doesn't exist assembler create it.
Second optional argument set flags of the section. Can be from list:

* `a` - allocatable section
* `w` - writeable section in file
* `x` - executable section

Third optional argument set type of the section. Default section's type depends on
the binary format. Type can be one of following type:

* `progbits` - program data
* `note` - informations about program or other things
* `nobits` - doesn't contain data (only occupies space)

The last attribute called 'align' set up section aligmnent.

### .size

Syntax: .size SYMBOL, ABS-EXPR

Set size of symbol SYMBOL. Currently, this feature of symbol is not used by
the CLRX assembler.

### .skip, .space

Syntax: .skip SIZE-EXPR[, VALUE-EXPR]
Syntax: .space SIZE-EXPR[, VALUE-EXPR]

Likewise as `.fill`, this pseudo-operation emits value (but byte) many times.
First expression determines how many values should to be emitted, second expression
determines what byte value should to be stored. If second expression is not given
then assembler stores 0's.

### .string, .string16, .string32, .string64

Syntax: .string "STRING",....  
Syntax: .string16 "STRING",....  
Syntax: .string32 "STRING",....  
Syntax: .string64 "STRING",....

Emit ASCII string. This pseudo-operations adds the
null-terminated character. If more than one string will be given then all given
string will be concatenated.
`.string16` emits string with 2-byte characters.
`.string32` emits string with 4-byte characters.
`.string64` emits string with 8-byte characters.
Characters longer than 1 byte will be zero expanded.

### .text

Go to `.text` section. If this section doesn't exist assembler create it.

### .undef

Syntax: .undef SYMBOL

Undefine symbol. If symbol already doesn't exist then assembler warns about that.

### .unusing

Syntax: .unusing [SCOPEPATH]

Stop using all objects (labels, symbols, regvar) in the specified scope or all
previously used scopes (by '.using').

### .using

Syntax: .using SCOPEPATH

Start use all objects (labels, symbols, scopes, regvar) in the specified scope.
All objects (if they names is not hidden by object in current scope) will be visible
directly including usings from specified scope. Searching object in current level of
scope's stack is recursive and it traverse through 'usings' until object will be found.
Assembler start searching object begins from the last declared 'using' to first 'using'.

### .warning

Syntax: .warning "STRING"

Print warning message specified in first argument.

### .weak

Syntax: .weak SYMBOL,...

Indicates that symbols will be a weak. Currently, unused feature of the symbol by
the CLRX assembler.

### .while

Syntax: .while COND-EXPR

Open 'while' repetition.The code between this pseudo-operation and `.endr` will be repeated
until COND-EXPR returns zero. Example:

```
x=1
.while x<16
    .int x
    x=x+x
.endr
```

generates:

```
.int x      # x=1
.int x      # x=2
.int x      # x=4
.int x      # x=8
```

### .word

Syntax: .word ABS-EXPR,....

Emit processor's word values. If any expression is empty then
an assembler stores 0 and warns about empty expression. If expression will give a value
that can not be stored in processor's word then an assembler warn about that.

Processor's word have always 4 bytes.
