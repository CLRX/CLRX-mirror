## CLRadeonExtender Assembler syntax

The CLRX assembler is compatible with GNU as syntax. In the many cases code for
GNU as can be easily ported to CLRX assembler, ofcourse except processor instructions.

## Layout of the source

The assembler accepts plain text that contains lines. Lines contains one of more
statements. Statement can be the symbol's assignment, assembler's pseudo-operation
(directive) or processor's instruction.

Pseudo-operations begins from `.` character. Symbol assignment is in following form:
`symbolName=expression`.

If line is too long, it can be splitted into small parts by using `\` at end of line,
likewise as in C/C++ language. 

Statement can be separated in single line by semicolon `;`. Like that:

```
.int 1,2,3; v_nop; nop_count = nop_count+1
```

Single comment begins from `#`. Multiline comment is same as in C/C++ language:
begins from `/*` and terminates at `*/`.

### Symbols

CRLX assembler operates on the symbols. The symbol is value that can be a absolute value or
it can refer to some place in binary code. Special symbol that is always defined refers to
current place of a binary code. This is `.` and is called in this manual as output counter.
Symbol names can contains alphanumeric characters, `.` and `_`. First character
must not be a digit. This same rules concerns a labels.

Label are symbol that can not be redefined.
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

Any symbol that refer to some code place refer to sections.

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
Binary|    !     |   3   | Binary ORNOT (performs A|~B)
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
Binary|   ?:     |   6   | Choice (this same as C++)

'vert-line' is `|`, and 'dbl-vert-line' is `||`.

The `?:` operator have this same meanigful as in C/C++ and performed from
right to left side.

Symbol refering to some place can be added, subtracted, compared or negated if
final result of the expression can be represented as place of the code or absolute value
(without refering to any place). An assembler performs this same operations
on the sections during evaluating an expression. Division, modulo,
binary operations (except negation), logical operations is not legal.
