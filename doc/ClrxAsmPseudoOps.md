## CLRadeonExtender Assembler syntax

The CLRX assembler accepts the almost pseudo-operations from GNU as.
This chapter lists and explain standard pseudo-operations.

### .abort

Aborts compilation.

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
0 and warn about empty expression. If expression will give a value that can not be stored
in bytes then an assembler warn about that.

### .data

Syntax: .data

Go to `.data` section or if that section doesn't exists create it.

### .double

Syntax: .double DOUBLE-VAL,...

Put double-precision floating point values into current section.
If no value between comma then an assembler stores 0 and warn about no value.
This pseudo-operation accepts only double precision floating point literals.

### .end

Ends source code compilation at this point.

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

### .err

This pseudo-operation causes error at point where is encountered.

### .error

Syntax: .error STRING

This pseudo-operation causes error and print error given in string.

### .extern

Syntax: .extern SYMBOL,...

Set symbol as external. Ignored by GNU as and CLRX assembler.

### .fail

Syntax: .fail ABS-EXPR

If value of the expression is greater or equal to 500 then assembler print warnings.
Otherwise assembler print error and fails.

### .file

This pseudo-operations is ignored by CLRX assembler.

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

Syntax: .float DOUBLE-VAL,...

Syntax: .single DOUBLE-VAL,...

Put single-precision floating point values into current section.
If no value between comma then an assembler stores 0 and warn about no value.
This pseudo-operation accepts only single precision floating point literals.
