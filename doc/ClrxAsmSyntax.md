## CLRadeonExtender Assembler syntax

The CLRX assembler is compatible with GNU assembler syntax. In the many cases code for
GNU assembler can be easily ported to CLRX assembler, ofcourse except processor's
instructions.

## Layout of the source

The assembler accepts plain text that contains lines. Lines contains one of more
statements. Statement can be the symbol's assignment, assembler's pseudo-operation
(directive) or processor's instruction.

Pseudo-operations begins from `.` character. Symbol assignment is in following form:
`symbolName=expression`.

If line is too long, it can be splitted into smaller parts by using `\` at end of line,
likewise as in C/C++ language. 

Statement can be separated in single line by semicolon `;`. Like that:

```
.int 1,2,3; v_nop; nop_count = nop_count+1
```

Single comment begins from `#`. Multiline comment is same as in C/C++ language:
begins from `/*` and terminates at `*/`.

Names of pseudo-operations, macro names (if option macrocase enabled),
processors instructions and other names
(for example: argument type, gpu device type) are case-insensitive. Symbol names,
kernel names,section names and scope names are case-sensitive.

### Symbols

CRLX assembler operates on the symbols. The symbol is value that can be a absolute value or
it can refer to some place in binary code. Special symbol that is always defined to
refers to current place of a binary code.
This is `.` and is called in this manual as output counter.
Symbol names can contains alphanumeric characters, `.` and `_`. First character
must not be a digit. This same rules concerns a labels.

Label is symbol that can not be redefined.
Labels precedes statement and can occurred many times. Like that:

```
label1: init: v_add_i32 v1, v2
end:  s_endpgm
```

Special kind of the label is local labels. They can be used only locally. The identifier
of local labels can have only digits. In contrast, local labels can to be
redefined many times.
In source code reference can be to previous or next local label by
adding `b` or `f` suffix.

```
v_add_i32 v32,3f-3b,v2  # 3b is previous `3` label, 3f is next `3` label
```

CLRX assembler accepts assignment register or register's range to symbols.
Register or register's range shall to be preceded by '%' at assignment.
Register symbol can be used for instruction operand or other register assignment.
Register subranges or just single register can be extracted from parent register ranges
by using indexing as well as regular register pools. Example:

```
regpool = %v[16:31]
reg1 = %s[0:1]
s_and_b64 reg1, s[2:3], s[4:5]  # output as s[0:1]
s_cmp_lt_i32 reg1[0], s2        # compare s0 with s2
v_xor_b32 regpool[4], regpool[7], regpool[9]    # v_xor_b32 v20, v23, v25
zx = 10 # zx symbol
v_xor_b32 regpool[zx+1], regpool[zx+5], regpool[zx+7]    # v_xor_b32 v27, v31, v33
```

Additionally, an assembler accepts constant literals in symreganges like `zz=%1` or
`xx=%0.5`. These symreganges are permitted in fields that accepts a scalar sources and
are not permitted in a scalar destination.

Special operator 'lit' force literal encoding for operand immediates:

```
s_add_u32 s1,s2,lit(4)      # encode 4 as literal (two 32-bit words)
s_add_u32 s1,s2,lit(4.0)    # encode 4.0 as literal (two 32-bit words)
```

### Scopes

New feature is the visibility's scopes. The scopes concerns symbols, labels
(except local labels), regvars. The macros, kernels and sections are still global.
 At start, the assembler create the global scope, that
is root of next defined scopes. The scope can be opened by using `.scope` pseudo-op and
they can be closed by using `.ends` or `.endscope`. We distinguish scope to two types:
normal and temporary scopes.
The temporary scopes doesn't have name and they exists until first close.

If scope will be opened, any object in this scope will directly available (by simple name).
Next available object is in used scopes (declared by `.using` pseudo-op)
begins from last 'using' to 'first'.

The scopes are organized in tree where global scope is root of tree.
This feature, allow to nest scopes (even named scopes inside temporary scopes).
During searching object, an assembler begins from
top (current) scope and ends at global scope. In every scope, it is possible to
start using object from other scopes (by `.using` pseudo-op).
While searching at scope stack level, an assembler firstly search that scope and
if not found then search object through 'usings'.

Example of using scopes:

```
.scope ala  # open scope 'ala', parent is global scope
    sym1 = 4
    sym2 = 11
    .byte sym1 # put 4
    .scope child   # open scope child, parent is 'ala'
        sym1 = 5
        .byte sym1 # put 5
        .byte sym2 # put 11, sym2 in 'ala' scope
    .ends
    .scope      # open temporary scope
        sym1 = 8
        .byte sym1 # put 8
    .ends       # close scope, now is doesn't exists
    .byte sym1 # put 4
.ends       # close scope 'ala'
```

Example of 'usings':

```
.scope ala  # open scope 'ala', parent is global scope
    sym2 = 4
.ends
.scope another  # open scope 'another', parent is global scope
    sym2 = 6
    sym3 = 15
.ends
.using ala      # start using 'ala'
.byte sym2      # put 4, sym2 from scope 'ala'
.scope ula
    .using another  # start using 'another'
    .byte sym2  # put 6, sym2 from scope 'another'
.ends
.byte sym2      # put 4, sym1 from scope 'ala'
.scope ula
    .using ala # start using 'ala'
    .byte sym2  # put 4, sym2 from scope 'ala', because 'ala' is last declared
    .byte sym3  # put 15, sym3 from scope 'another'
.ends
::ala::sym2 = 7 # redefine sym2 in scope 'ala'
```

The names of the object can have the scope path. Scope path is way to particular scope in
tree. If searching scope should start from global scope, a scope path should be begins
from `::`. The `::` is separator (likes `/` in file system path) for path elements.

```
sym1 = 9
.scope ala  # open scope 'ala', parent is global scope
    sym1 = 4
    .scope child   # open scope child, parent is 'ala'
        sym1 = 7
    .ends
.ends
.byte ala::sym1   # put 4, symbol from 'ala' scope
.byte ala::child::sym1   # put 7, symbol from 'child' scope in 'ala' scope
.scope ala
    .byte ::sym1        # put 9, sym1 from global scope
.ends
```

The setting symbols, labels, if simple name is given (without scope path) always
create object in the current scope. Any call of object (even if not defined) always
start searching through scope tree. It is possible to call to symbols
in scope which doesn't already exists (just they will be created with object while calling).
After that call, symbol can be defined.

The algorithm of searching the object is bit sophisticated:

1. Search scope.  
1.1. If simple name is given the begin at current scope of tree.  
1.2. If scope path is only `::`, then search only at global scope  
1.3. If scope begins from `::`, then first scope element in global scope  
1.4. Otherwise, find scope element begins from current scope going to shallower
level of tree (finally to global scope).  
1.5. If scopes are not found, then create then at global scope (if scope path begins `::`)
or current scope.  
2. Find object in that scope, if not found:  
2.1. Find in 'usings' begins from last and ends at first.  
3. Go to parent scope if not global scope and no scope path.
If global scope the end searching.

The special symbol `.` is always global. Any `.` in any place always calls this
same counter.

### Sections

Section is some part of the binary that contains some data. Type of the data depends on
type of the section. Main program code is in the `.text` section which holds
program's instructions. Section `.rodata` holds read-only data (mainly constant data)
that can be used by program. Section can be divided by type of the access.
The most sections are writeable (any data can be put into them) and
addressable (we can define symbols inside these sections or move forward).

Absolute section is only addressable section. It can be used for defining structures.
In absolute section output counter can be moved backward (this is special exception
for absolute section).

Any symbol that refer to some code place refer to sections. Between switching kernels or
global layout, the last used sections are stored. Next usage of kernel or global layout
causes switching to this last section.

Special type of sections are configuration's sections. These section does not hold any
content (bytes), instead they stores configuration of the kernel or program which
is defined by specific pseudo operations. Any putting data to these sections is illegal.

### Literals

CLRX assembler treats any constant literals as 64-bit value. Assembler honors
C/C++ literal syntax. Special kind of literal are floating point literals.
They can be used only in `.half`, `.single`, `.float`, `.double` pseudo-operations
or as operand of the instruction that accepts floating point literals.

Literal types:

* decimal literals: `100, 12, 4323`
* hexadecimal literals: `0x354, 0x3da, 0xDAB`
* octal literals: `0246, 077`
* binary literals: `0b10010101, 0b11011`
* character literals: `'a', 'b', '-', '\n', '\t', '\v', '\xab', '\123`
* floating point literals: `10.2, .45, +1.5e, 100e-6, 0x1a2.4b5p5`
* string literals: `"ala ma kota", "some\n"

For character literals and string literals, escape can be used to put special characters
likes newline, tab. List of the escapes:

Escape   |  Description    | Value
:-------:|-----------------|-------------
 `\a`    | Alarm           | 7
 `\b`    | Backspace       | 8
 `\t`    | Tab             | 9
 `\n`    | Newline         | 10
 `\v`    | Vertical tab    | 11
 `\f`    | Form feed       | 12
 `\r`    | Carriage return | 13
 `\\`    | Backslash       | 92
 `\"`    | Double-quote    | 34
 `\'`    | Qoute           | 39
 `\aaa`  | Octal code      | Various
 `\HHH..`|Hexadecimal code | Various

The floating point literals in instruction operands can have the suffix ('l', 'h' or 's').
Suffix 's' indicates that given value is single floating point value.
Suffix 'h' indicates that given value is half floating point value.
Suffix 'l' indicates that given value is double floating point value.

### Expressions

The CLRX assembler get this same the operator ordering as in GNU as.
CLRX assembler treat any literal or symbol's value as 64-bit integer value.
List of the operators:

Type  | Operator | Order | Description
------|:--------:|:-----:|--------------------
Unary |    -     |   1   | Negate value
Unary |    ~     |   1   | Binary NOT
Unary |    !     |   1   | Logical NOT
Unary |    +     |   1   | Plus (doing nothing)
Binary|    *     |   2   | Multiplication
Binary|    /     |   2   | Signed division
Binary|    //    |   2   | Unsigned division
Binary|    %     |   2   | Signed remainder
Binary|    %%    |   2   | Unsigned remainder
Binary|    <<    |   2   | Left shift
Binary|    >>    |   2   | Unsigned right shift
Binary|    >>>   |   2   | Signed right shift
Binary|    &     |   3   | Binary AND
Binary| vert-line|   3   | Binary OR
Binary|    ^     |   3   | Binary XOR
Binary|    !     |   3   | Binary ORNOT (performs A OR ~B)
Binary|    +     |   3   | Addition
Binary|    -     |   3   | Subtraction
Binary|    ==    |   4   | Equal to
Binary| !=,<>    |   4   | Not equal to
Binary|   <      |   4   | Less than (signed)
Binary|   <=     |   4   | Less or equal (signed)
Binary|   >      |   4   | Greater than (signed)
Binary|   >=     |   4   | Greater or equal (signed)
Binary|   <@     |   4   | Less than (unsigned)
Binary|   <=@    |   4   | Less or equal (unsigned)
Binary|   >@     |   4   | Greater than (unsigned)
Binary|   >=@    |   4   | Greater or equal (unsigned)
Binary|   &&     |   5   | Logical AND
Binary|dbl-vert-line|   5   | Logical OR
Binary|   ?:     |   6   | Choice (this same as in C++)

'vert-line' is `|`, and 'dbl-vert-line' is `||`.

The `?:` operator have this same meanigful as in C/C++ and performed from
right to left side. 

**Important note**: Comparison operators return all ones (-1) value instead 1.

Symbol refering to some place can be added, subtracted, compared or negated if
final result of the expression can be represented as place of the code or absolute value
(without refering to any place). An assembler performs this same operations
on the sections during evaluating an expression. Division, modulo,
binary operations (except negation), logical operations is not legal.

### Instruction operands

Instruction operand can be one of list:

* GCN register or register range
* absolute expression
* float literal
* in VOP3 encoding operand modifier: abs, neg

An expression can be preceded by '@' to ensure that a following text will be treated as
an expression:

```
v_add_f32 v0, @v0, v4       # second operand is expression: 'v0' instead of v0 register
```

Alternatively, any expression can be inscribed in parentheses to ensure that result.
