/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <CLRX/amdasm/Assembler.h>

using namespace CLRX;

struct AsmExprParseCase
{
    const char* expression;
    const char* rpnString;
    bool evaluated;
    uint64_t value;
    const char* errors;
    const char* extra;
};

static AsmExprParseCase asmExprParseCases[] =
{
    /* literals */
    { "", "", true, 0, "", "" }, /* empty string is illegal */
    { "1", "1", true, 1, "", "" },
    { "3", "3", true, 3, "", "" },
    { "1234893", "1234893", true, 1234893, "", "" },
    { "187378932789789", "187378932789789", true, 187378932789789UL, "", "" },
    { "01234", "668", true, 668, "", "" },
    { "0xaddab", "712107", true, 712107, "", "" },
    { "0b11", "3", true, 3, "", "" },
    /* character literals */
    { "'a'", "97", true, 97, "", "" },
    { "'d'", "100", true, 100, "", "" },
    { "'A'", "65", true, 65, "", "" },
    { "'E'", "69", true, 69, "", "" },
    { "'\\a'", "7", true, 7, "", "" },
    { "'\\n'", "10", true, 10, "", "" },
    { "'\\b'", "8", true, 8, "", "" },
    { "'\\t'", "9", true, 9, "", "" },
    { "'\\t'", "9", true, 9, "", "" },
    { "'\\132'", "90", true, 90, "", "" },
    { "'\\x9A'", "154", true, 154, "", "" },
    { "'\\xaa9A'", "154", true, 154, "", "" }, // changed hex literal parsing
    { "'\\T'", "84", true, 84, "", "" },
    { "'\\\\'", "92", true, 92, "", "" },
    { "'\\''", "39", true, 39, "", "" },
    /* symbols */
    { "xyz", "xyz", false, 0, "", "" },
    { ".sometest", ".sometest", false, 0, "", "" },
    /* local labels */
    { "555f + 556f", "555f 556f +", false, 0, "", "", },
    /* simple expressions */
    { "-3", "3 !-", true, -3ULL, "", "" },
    { "+3", "3", true, 3, "", "" }, /* no plus operator, because is skipped */
    { "4-7+8", "4 7 - 8 +", true, 5, "", "" },
    { "2*9 + 11*5", "2 9 * 11 5 * +", true, 73, "", "" },
    /* operator ordering */
    { "--~-~x", "x ~ !- ~ !- !-", false, 0, "", "" },
    { "x+-y+~z+!w", "x y !- + z ~ + w ! +", false, 0, "", "" },
    { "a+b*x", "a b x * +", false, 0, "", "" },
    { "a-b*x", "a b x * -", false, 0, "", "" },
    { "a&b*x", "a b x * &", false, 0, "", "" },
    { "a|b*x", "a b x * |", false, 0, "", "" },
    { "a^b*x", "a b x * ^", false, 0, "", "" },
    { "a!b*x", "a b x * !!", false, 0, "", "" },
    { "a+b/x", "a b x / +", false, 0, "", "" },
    { "a-b/x", "a b x / -", false, 0, "", "" },
    { "a&b/x", "a b x / &", false, 0, "", "" },
    { "a|b/x", "a b x / |", false, 0, "", "" },
    { "a^b/x", "a b x / ^", false, 0, "", "" },
    { "a!b/x", "a b x / !!", false, 0, "", "" },
    { "a+b%x", "a b x % +", false, 0, "", "" },
    { "a-b%x", "a b x % -", false, 0, "", "" },
    { "a&b%x", "a b x % &", false, 0, "", "" },
    { "a|b%x", "a b x % |", false, 0, "", "" },
    { "a^b%x", "a b x % ^", false, 0, "", "" },
    { "a!b%x", "a b x % !!", false, 0, "", "" },
    { "a+b//x", "a b x // +", false, 0, "", "" },
    { "a-b//x", "a b x // -", false, 0, "", "" },
    { "a&b//x", "a b x // &", false, 0, "", "" },
    { "a|b//x", "a b x // |", false, 0, "", "" },
    { "a^b//x", "a b x // ^", false, 0, "", "" },
    { "a!b//x", "a b x // !!", false, 0, "", "" },
    { "a+b%%x", "a b x %% +", false, 0, "", "" },
    { "a-b%%x", "a b x %% -", false, 0, "", "" },
    { "a&b%%x", "a b x %% &", false, 0, "", "" },
    { "a|b%%x", "a b x %% |", false, 0, "", "" },
    { "a^b%%x", "a b x %% ^", false, 0, "", "" },
    { "a!b%%x", "a b x %% !!", false, 0, "", "" },
    { "a+b<<x", "a b x << +", false, 0, "", "" },
    { "a-b<<x", "a b x << -", false, 0, "", "" },
    { "a&b<<x", "a b x << &", false, 0, "", "" },
    { "a|b<<x", "a b x << |", false, 0, "", "" },
    { "a^b<<x", "a b x << ^", false, 0, "", "" },
    { "a!b<<x", "a b x << !!", false, 0, "", "" },
    { "a+b>>x", "a b x >> +", false, 0, "", "" },
    { "a-b>>x", "a b x >> -", false, 0, "", "" },
    { "a&b>>x", "a b x >> &", false, 0, "", "" },
    { "a|b>>x", "a b x >> |", false, 0, "", "" },
    { "a^b>>x", "a b x >> ^", false, 0, "", "" },
    { "a!b>>x", "a b x >> !!", false, 0, "", "" },
    { "a+b>>>x", "a b x >>> +", false, 0, "", "" },
    { "a-b>>>x", "a b x >>> -", false, 0, "", "" },
    { "a&b>>>x", "a b x >>> &", false, 0, "", "" },
    { "a|b>>>x", "a b x >>> |", false, 0, "", "" },
    { "a^b>>>x", "a b x >>> ^", false, 0, "", "" },
    { "a!b>>>x", "a b x >>> !!", false, 0, "", "" },
    { "a+b&x", "a b x & +", false, 0, "", "" },
    { "a-b&x", "a b x & -", false, 0, "", "" },
    { "a+b|x", "a b x | +", false, 0, "", "" },
    { "a-b|x", "a b x | -", false, 0, "", "" },
    { "a+b^x", "a b x ^ +", false, 0, "", "" },
    { "a-b^x", "a b x ^ -", false, 0, "", "" },
    { "a+b!x", "a b x !! +", false, 0, "", "" },
    { "a-b!x", "a b x !! -", false, 0, "", "" },
    { "a+b==x", "a b + x ==", false, 0, "", "" },
    { "a-b==x", "a b - x ==", false, 0, "", "" },
    { "a+b!=x", "a b + x !=", false, 0, "", "" },
    { "a-b!=x", "a b - x !=", false, 0, "", "" },
    { "a+b<>x", "a b + x !=", false, 0, "", "" },
    { "a-b<>x", "a b - x !=", false, 0, "", "" },
    { "a+b<x", "a b + x <", false, 0, "", "" },
    { "a-b<x", "a b - x <", false, 0, "", "" },
    { "a+b<=x", "a b + x <=", false, 0, "", "" },
    { "a-b<=x", "a b - x <=", false, 0, "", "" },
    { "a+b>x", "a b + x >", false, 0, "", "" },
    { "a-b>x", "a b - x >", false, 0, "", "" },
    { "a+b>=x", "a b + x >=", false, 0, "", "" },
    { "a-b>=x", "a b - x >=", false, 0, "", "" },
    { "a+b<@x", "a b + x <@", false, 0, "", "" },
    { "a-b<@x", "a b - x <@", false, 0, "", "" },
    { "a+b<=@x", "a b + x <=@", false, 0, "", "" },
    { "a-b<=@x", "a b - x <=@", false, 0, "", "" },
    { "a+b>@x", "a b + x >@", false, 0, "", "" },
    { "a-b>@x", "a b - x >@", false, 0, "", "" },
    { "a+b>=@x", "a b + x >=@", false, 0, "", "" },
    { "a-b>=@x", "a b - x >=@", false, 0, "", "" },
    { "a+b&&x", "a b + x &&", false, 0, "", "" },
    { "a-b&&x", "a b - x &&", false, 0, "", "" },
    { "a+b||x", "a b + x ||", false, 0, "", "" },
    { "a-b||x", "a b - x ||", false, 0, "", "" },
    { "a&&b==x", "a b x == &&", false, 0, "", "" },
    { "a||b==x", "a b x == ||", false, 0, "", "" },
    { "a&&b!=x", "a b x != &&", false, 0, "", "" },
    { "a||b!=x", "a b x != ||", false, 0, "", "" },
    { "a&&b<>x", "a b x != &&", false, 0, "", "" },
    { "a||b<>x", "a b x != ||", false, 0, "", "" },
    { "a&&b<x", "a b x < &&", false, 0, "", "" },
    { "a||b<x", "a b x < ||", false, 0, "", "" },
    { "a&&b<=x", "a b x <= &&", false, 0, "", "" },
    { "a||b<=x", "a b x <= ||", false, 0, "", "" },
    { "a&&b>x", "a b x > &&", false, 0, "", "" },
    { "a||b>x", "a b x > ||", false, 0, "", "" },
    { "a&&b>=x", "a b x >= &&", false, 0, "", "" },
    { "a||b>=x", "a b x >= ||", false, 0, "", "" },
    { "a&&b<@x", "a b x <@ &&", false, 0, "", "" },
    { "a||b<@x", "a b x <@ ||", false, 0, "", "" },
    { "a&&b<=@x", "a b x <=@ &&", false, 0, "", "" },
    { "a||b<=@x", "a b x <=@ ||", false, 0, "", "" },
    { "a&&b>@x", "a b x >@ &&", false, 0, "", "" },
    { "a||b>@x", "a b x >@ ||", false, 0, "", "" },
    { "a&&b>=@x", "a b x >=@ &&", false, 0, "", "" },
    { "a||b>=@x", "a b x >=@ ||", false, 0, "", "" },
    { "a&b==x", "a b & x ==", false, 0, "", "" },
    { "a|b==x", "a b | x ==", false, 0, "", "" },
    { "a^b==x", "a b ^ x ==", false, 0, "", "" },
    { "a!b==x", "a b !! x ==", false, 0, "", "" },
    { "a&b!=x", "a b & x !=", false, 0, "", "" },
    { "a|b!=x", "a b | x !=", false, 0, "", "" },
    { "a^b!=x", "a b ^ x !=", false, 0, "", "" },
    { "a!b!=x", "a b !! x !=", false, 0, "", "" },
    { "a&b<>x", "a b & x !=", false, 0, "", "" },
    { "a|b<>x", "a b | x !=", false, 0, "", "" },
    { "a^b<>x", "a b ^ x !=", false, 0, "", "" },
    { "a!b<>x", "a b !! x !=", false, 0, "", "" },
    { "a&b<x", "a b & x <", false, 0, "", "" },
    { "a|b<x", "a b | x <", false, 0, "", "" },
    { "a^b<x", "a b ^ x <", false, 0, "", "" },
    { "a!b<x", "a b !! x <", false, 0, "", "" },
    { "a&b<=x", "a b & x <=", false, 0, "", "" },
    { "a|b<=x", "a b | x <=", false, 0, "", "" },
    { "a^b<=x", "a b ^ x <=", false, 0, "", "" },
    { "a!b<=x", "a b !! x <=", false, 0, "", "" },
    { "a&b>x", "a b & x >", false, 0, "", "" },
    { "a|b>x", "a b | x >", false, 0, "", "" },
    { "a^b>x", "a b ^ x >", false, 0, "", "" },
    { "a!b>x", "a b !! x >", false, 0, "", "" },
    { "a&b>=x", "a b & x >=", false, 0, "", "" },
    { "a|b>=x", "a b | x >=", false, 0, "", "" },
    { "a^b>=x", "a b ^ x >=", false, 0, "", "" },
    { "a!b>=x", "a b !! x >=", false, 0, "", "" },
    { "a&b<@x", "a b & x <@", false, 0, "", "" },
    { "a|b<@x", "a b | x <@", false, 0, "", "" },
    { "a^b<@x", "a b ^ x <@", false, 0, "", "" },
    { "a!b<@x", "a b !! x <@", false, 0, "", "" },
    { "a&b<=@x", "a b & x <=@", false, 0, "", "" },
    { "a|b<=@x", "a b | x <=@", false, 0, "", "" },
    { "a^b<=@x", "a b ^ x <=@", false, 0, "", "" },
    { "a!b<=@x", "a b !! x <=@", false, 0, "", "" },
    { "a&b>@x", "a b & x >@", false, 0, "", "" },
    { "a|b>@x", "a b | x >@", false, 0, "", "" },
    { "a^b>@x", "a b ^ x >@", false, 0, "", "" },
    { "a!b>@x", "a b !! x >@", false, 0, "", "" },
    { "a&b>=@x", "a b & x >=@", false, 0, "", "" },
    { "a|b>=@x", "a b | x >=@", false, 0, "", "" },
    { "a^b>=@x", "a b ^ x >=@", false, 0, "", "" },
    { "a!b>=@x", "a b !! x >=@", false, 0, "", "" },
    { "a&&x||b<=c>=d-e+f>>g*h/-!~i", "a x && b c <= d e - f g >> h * i ~ ! !- / + >= ||",
      false, 0, "", "" },
    /* ?: test suite */
    { "a?b:c", "a b c ?", false, 0, "", "" },
    { "a?b:c?d:e", "a b c d e ? ?", false, 0, "", "" },
    { "a?b:c?d:e?f:g", "a b c d e f g ? ? ?", false, 0, "", "" },
    { "a?b?c:d:e", "a b c d ? e ?", false, 0, "", "" },
    { "a?b?c?d:e:f?g:h:i", "a b c d e ? f g h ? ? i ?", false, 0, "", "" },
    { "a?b?(c?d:e):f?g:h:i", "a b c d e ? f g h ? ? i ?", false, 0, "", "" },
    { "a==b ? c && d ? e<b : g : h", "a b == c d && e b < g ? h ?", false, 0, "", "" },
    /* parentheses */
    { "(a)", "a", false, 0, "", "" },
    { "((((a))))", "a", false, 0, "", "" },
    { "a+(b+c)", "a b c + +", false, 0, "", "" },
    { "a+(b+(c+d))", "a b c d + + +", false, 0, "", "" },
    { "a-(b+(c-d))", "a b c d - + -", false, 0, "", "" },
    { "(a+b)*c", "a b + c *", false, 0, "", "" },
    { "a*(b+c)", "a b c + *", false, 0, "", "" },
    { "(a-b)*c", "a b - c *", false, 0, "", "" },
    { "a*(b-c)", "a b c - *", false, 0, "", "" },
    { "(a+b)/c", "a b + c /", false, 0, "", "" },
    { "a/(b+c)", "a b c + /", false, 0, "", "" },
    { "(a-b)/c", "a b - c /", false, 0, "", "" },
    { "a/(b-c)", "a b c - /", false, 0, "", "" },
    { "(a&b)*c", "a b & c *", false, 0, "", "" },
    { "a*(b&c)", "a b c & *", false, 0, "", "" },
    { "(a|b)*c", "a b | c *", false, 0, "", "" },
    { "a*(b|c)", "a b c | *", false, 0, "", "" },
    { "(a&b)/c", "a b & c /", false, 0, "", "" },
    { "a/(b&c)", "a b c & /", false, 0, "", "" },
    { "(a|b)/c", "a b | c /", false, 0, "", "" },
    { "a/(b|c)", "a b c | /", false, 0, "", "" },
    { "(a+b)&c", "a b + c &", false, 0, "", "" },
    { "a&(b+c)", "a b c + &", false, 0, "", "" },
    { "(a-b)&c", "a b - c &", false, 0, "", "" },
    { "a&(b-c)", "a b c - &", false, 0, "", "" },
    { "(a+b)|c", "a b + c |", false, 0, "", "" },
    { "a|(b+c)", "a b c + |", false, 0, "", "" },
    { "(a-b)|c", "a b - c |", false, 0, "", "" },
    { "a|(b-c)", "a b c - |", false, 0, "", "" },
    { "(a==b)*c", "a b == c *", false, 0, "", "" },
    { "a*(b==c)", "a b c == *", false, 0, "", "" },
    { "(a!=b)*c", "a b != c *", false, 0, "", "" },
    { "a*(b!=c)", "a b c != *", false, 0, "", "" },
    { "(a&&b)*c", "a b && c *", false, 0, "", "" },
    { "a*(b&&c)", "a b c && *", false, 0, "", "" },
    { "(a||b)*c", "a b || c *", false, 0, "", "" },
    { "a*(b||c)", "a b c || *", false, 0, "", "" },
    { "a+(b*c)", "a b c * +", false, 0, "", "" },
    { "(a*b)+c", "a b * c +", false, 0, "", "" },
    { "a+(b*(d+e)-x*(f-g)*(a==4))/(x<>y?~j<<k:!l)",
      "a b d e + * x f g - * a 4 == * - x y != j ~ k << l ! ? / +", false, 0, "", "" },
    { "a*~(c&b) + ~d&-(e==x)", "a c b & ~ * d ~ e x == !- & +", false, 0, "", "" },
    { "a?b?c:(d?e:f):g", "a b c d e f ? ? g ?", false, 0, "", "" },
    { "a?b?c:(x?y:d?e:f):g", "a b c x y d e f ? ? ? g ?", false, 0, "", "" },
    
    /** errors handling **/
    { "1 / 0", "1 0 /", false, 0, "<stdin>:1:3: Error: Division by zero\n", "" },
    { "1 // 0", "1 0 //", false, 0, "<stdin>:1:3: Error: Division by zero\n", "" },
    { "1 % 0", "1 0 %", false, 0, "<stdin>:1:3: Error: Division by zero\n", "" },
    { "1 %% 0", "1 0 %%", false, 0, "<stdin>:1:3: Error: Division by zero\n", "" },
    { "1<<64", "1 64 <<", true, 0,
      "<stdin>:1:2: Warning: Shift count out of range (between 0 and 63)\n", "" },
    { "1>>64", "1 64 >>", true, 0,
      "<stdin>:1:2: Warning: Shift count out of range (between 0 and 63)\n", "" },
    { "1>>>64", "1 64 >>>", true, 0,
      "<stdin>:1:2: Warning: Shift count out of range (between 0 and 63)\n", "" },
    /* literals */
    { "0xx", "", false, 0, "<stdin>:1:1: Error: Missing number\n", "x" },
    { "0bx", "", false, 0, "<stdin>:1:1: Error: Missing number\n", "x" },
    { "'", "", false, 0, "<stdin>:1:1: Error: Unterminated character literal\n", "" },
    { "'\\", "", false, 0, "<stdin>:1:1: Error: Unterminated character literal\n", "" },
    { "'\\x", "", false, 0, "<stdin>:1:1: Error: Unterminated character literal\n", "" },
    { "'\\400", "", false, 0, "<stdin>:1:1: Error: Octal code out of range\n", "0" },
    { "'\\xda", "", false, 0, "<stdin>:1:1: Error: Missing ''' at end of literal\n", "" },
    { "'\\39'", "", false, 0,
        "<stdin>:1:1: Error: Expected octal character code\n", "9'" },
    /* operators */
    { "+", "", false, 0, "<stdin>:1:2: Error: Unterminated expression\n", "" },
    { "-", "", false, 0, "<stdin>:1:2: Error: Unterminated expression\n", "" },
    { "~", "", false, 0, "<stdin>:1:2: Error: Unterminated expression\n", "" },
    { "!", "", false, 0, "<stdin>:1:2: Error: Unterminated expression\n", "" },
    { "*", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "/", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "//", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "%", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "&", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "|", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "^", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "!", "", false, 0, "<stdin>:1:2: Error: Unterminated expression\n", "" },
    { "<<", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { ">>>", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "!=", "", false, 0, "<stdin>:1:2: Error: "
            "Expected primary expression before operator\n"
            "<stdin>:1:3: Error: Unterminated expression\n", "" },
    { "<", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "<=", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { ">", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { ">=", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "<@", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { "<=@", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { ">@", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    { ">=@", "", false, 0, "<stdin>:1:1: Error: "
            "Expected primary expression before operator\n", "" },
    /* operators at end of expression */
    { "1+6+", "", false, 0, "<stdin>:1:5: Error: Unterminated expression\n", "" },
    { "1+6-", "", false, 0, "<stdin>:1:5: Error: Unterminated expression\n", "" },
    { "1+6~", "", false, 0, "<stdin>:1:4: Error: "
        "Expected non-unary operator, '(', or end of expression\n", "" },
    { "1+6/", "", false, 0, "<stdin>:1:5: Error: Unterminated expression\n", "" },
    { "1+6//", "", false, 0, "<stdin>:1:6: Error: Unterminated expression\n", "" },
    { "1+6%%", "", false, 0, "<stdin>:1:6: Error: Unterminated expression\n", "" },
    { "1+6<<", "", false, 0, "<stdin>:1:6: Error: Unterminated expression\n", "" },
    { "1+6>>>", "", false, 0, "<stdin>:1:7: Error: Unterminated expression\n", "" },
    { "1+6!=", "", false, 0, "<stdin>:1:6: Error: Unterminated expression\n", "" },
    { "1+6<", "", false, 0, "<stdin>:1:5: Error: Unterminated expression\n", "" },
    { "1+6>", "", false, 0, "<stdin>:1:5: Error: Unterminated expression\n", "" },
    /* parentheses */
    { "1+(8*9", "", false, 0, "<stdin>:1:7: Error: Missing ')'\n", "" },
    { "1+(((8*9", "", false, 0, "<stdin>:1:9: Error: Missing ')'\n", "" },
    { "()", "", false, 0, "<stdin>:1:2: Error: Expected operator or value or symbol\n"
        "<stdin>:1:3: Error: Missing ')'\n", "" },
    { "4+7+()", "", false, 0, "<stdin>:1:6: Error: Expected operator or value or symbol\n"
        "<stdin>:1:7: Error: Missing ')'\n"
        "<stdin>:1:7: Error: Unterminated expression\n", "" },
    { "4+7<<()", "", false, 0, "<stdin>:1:7: Error: Expected operator or value or symbol\n"
        "<stdin>:1:8: Error: Missing ')'\n"
        "<stdin>:1:8: Error: Unterminated expression\n", "" },
    { "1)+8*9", "1", true, 1, "", ")+8*9" }, // no error
    { "1(+8*9", "", false, 0, "<stdin>:1:2: Error: Expected operator\n", "" },
    { "1+8(*9", "", false, 0, "<stdin>:1:4: Error: Expected operator\n", "" },
    { "1+8*9;", "1 8 9 * +", true, 73, "", ";" }, // no error
    { "1+8*9:", "1 8 9 * +", true, 73, "", ":" }, // no error
    { "1+8*9    :  ", "1 8 9 * +", true, 73, "", ":  " }, // no error
    { "1+8*9'", "1 8 9 * +", true, 73, "", "'" }, // no error
    { "1+8*9#", "1 8 9 * +", true, 73, "", "" }, // no error
    { "1+8*9/* */", "1 8 9 * +", true, 73, "", "" }, // no error
    { "0x + 0x", "", false, 0, "<stdin>:1:1: Error: Missing number\n"
        "<stdin>:1:6: Error: Number is too short\n", "" },
    { "(a+b * (b-c", "", false, 0, "<stdin>:1:12: Error: Missing ')'\n", "" },
    /* out of range */
    { "123445555556666666667", "", false, 0,
        "<stdin>:1:1: Error: Number out of range\n", "7" },
    { "1234455553236556666666667", "", false, 0,
        "<stdin>:1:1: Error: Number out of range\n", "66667" },
    /* error with ?: */
    { "a?b+c", "", false, 0, "<stdin>:1:2: Error: Missing ':' for '?'\n", "" },
    { "a*(a?b+c)", "", false, 0, "<stdin>:1:5: Error: Missing ':' for '?'\n", "" },
    { "a*(a?b+c:)", "", false, 0,
        "<stdin>:1:10: Error: Expected operator or value or symbol\n"
        "<stdin>:1:11: Error: Missing ')'\n"
        "<stdin>:1:11: Error: Unterminated expression\n", "" },
    { "a*a?(b+c:x)", "", false, 0, "<stdin>:1:9: Error: Missing '?' before ':'\n"
        "<stdin>:1:4: Error: Missing ':' for '?'\n", "" },
    { "a*(a?b+c)+x", "", false, 0, "<stdin>:1:5: Error: Missing ':' for '?'\n", "" },
    { "a?a+(:c", "", false, 0, "<stdin>:1:6: Error: Expected primary expression "
        "before operator\n<stdin>:1:8: Error: Missing ')'\n"
        "<stdin>:1:2: Error: Missing ':' for '?'\n", "" },
    /* random */
    { "a+?5:cd:4%2Qf:hab<;<@", "", false, 0, 
        "<stdin>:1:3: Error: Expected primary expression before operator\n",
        ":cd:4%2Qf:hab<;<@" },
    { "( ala + .,. )", "", false, 0, "<stdin>:1:11: Error: Garbages at end of expression\n"
        "<stdin>:1:12: Error: Garbages at end of expression\n", "" },
    /* with ',' */
    { "123+45*,", "", false, 0, "<stdin>:1:8: Error: Unterminated expression\n", "," }
};

static std::string rpnExpression(const AsmExpression* expr)
{
    std::ostringstream oss;
    const AsmExprArg* args = expr->getArgs();
    bool first = true;
    for (AsmExprOp op: expr->getOps())
    {
        if (!first)
            oss << ' ';
        first = false;
        if (op == AsmExprOp::ARG_VALUE)
        {
            oss << args->value;
            args++;
            continue;
        }
        else if (op == AsmExprOp::ARG_SYMBOL)
        {
            oss << args->symbol->first;
            args++;
            continue;
        }
        
        switch (op)
        {
            case AsmExprOp::ADDITION:
                oss << '+';
                break;
            case AsmExprOp::PLUS:
                oss << "!+";
                break;
            case AsmExprOp::SUBTRACT:
                oss << '-';
                break;
            case AsmExprOp::NEGATE:
                oss << "!-";
                break;
            case AsmExprOp::MULTIPLY:
                oss << '*';
                break;
            case AsmExprOp::DIVISION:
                oss << "//";
                break;
            case AsmExprOp::SIGNED_DIVISION:
                oss << '/';
                break;
            case AsmExprOp::MODULO:
                oss << "%%";
                break;
            case AsmExprOp::SIGNED_MODULO:
                oss << '%';
                break;
            case AsmExprOp::BIT_AND:
                oss << '&';
                break;
            case AsmExprOp::BIT_OR:
                oss << '|';
                break;
            case AsmExprOp::BIT_XOR:
                oss << '^';
                break;
            case AsmExprOp::BIT_ORNOT:
                oss << "!!";
                break;
            case AsmExprOp::BIT_NOT:
                oss << '~';
                break;
            case AsmExprOp::SHIFT_LEFT:
                oss << "<<";
                break;
            case AsmExprOp::SHIFT_RIGHT:
                oss << ">>";
                break;
            case AsmExprOp::SIGNED_SHIFT_RIGHT:
                oss << ">>>";
                break;
            case AsmExprOp::LOGICAL_AND:
                oss << "&&";
                break;
            case AsmExprOp::LOGICAL_OR:
                oss << "||";
                break;
            case AsmExprOp::LOGICAL_NOT:
                oss << '!';
                break;
            case AsmExprOp::CHOICE:
                oss << '?';
                break;
            case AsmExprOp::EQUAL:
                oss << "==";
                break;
            case AsmExprOp::NOT_EQUAL:
                oss << "!=";
                break;
            case AsmExprOp::LESS:
                oss << '<';
                break;
            case AsmExprOp::LESS_EQ:
                oss << "<=";
                break;
            case AsmExprOp::GREATER:
                oss << '>';
                break;
            case AsmExprOp::GREATER_EQ:
                oss << ">=";
                break;
            case AsmExprOp::BELOW:
                oss << "<@";
                break;
            case AsmExprOp::BELOW_EQ:
                oss << "<=@";
                break;
            case AsmExprOp::ABOVE:
                oss << ">@";
                break;
            case AsmExprOp::ABOVE_EQ:
                oss << ">=@";
                break;
            default:
                break;
        }
    }
    return oss.str();
}

class MyAssembler: public Assembler
{
public:
    explicit MyAssembler(std::istream& input, std::ostream& msgOut)
        : Assembler("", input, ASM_WARNINGS,
                    BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, msgOut)
    { readLine(); }
};

static void testAsmExprParse(cxuint i, const AsmExprParseCase& testCase, bool makeBase)
{
    std::istringstream iss(testCase.expression);
    std::ostringstream resultErrorsOut;
    MyAssembler assembler(iss, resultErrorsOut);
    size_t linePos = 0;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(assembler, linePos, makeBase));
    std::string resultRpnExpr;
    std::string resultExtra = testCase.expression+linePos;
    uint64_t resultValue = 0;
    bool resultEvaluated = false;
    if (expr)
    {
        resultRpnExpr = rpnExpression(expr.get());
        cxuint sectionId;
        if (expr->getSymOccursNum() == 0)
            resultEvaluated = expr->evaluate(assembler, resultValue, sectionId);
        else
            resultEvaluated = false;
    }
    /* compare */
    std::string resultErrors = resultErrorsOut.str();
    if (::strcmp(testCase.rpnString, resultRpnExpr.c_str()) ||
        testCase.evaluated != resultEvaluated || testCase.value != resultValue ||
        ::strcmp(testCase.errors, resultErrors.c_str()) ||
        ::strcmp(testCase.extra, resultExtra.c_str()))
    {
        std::ostringstream oss;
        oss << "FAILED for parseExpr#" << i << " snapshot=" << makeBase << "\n"
                "Result: rpnString='" << resultRpnExpr <<
                "', value=" << resultValue << ", evaluated=" << int(resultEvaluated) <<
                ", errors='" << resultErrors << "'"
                ", extra='" << resultExtra << "'.\n"
                "Expected: rpnString='" << testCase.rpnString <<
                "', value=" << testCase.value <<
                ", evaluated=" << int(testCase.evaluated) <<
                ", errors='" << testCase.errors << "'"
                ", extra='" << testCase.extra << "'.\n";
        throw Exception(oss.str());
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(asmExprParseCases)/sizeof(AsmExprParseCase); i++)
    {
        try
        { testAsmExprParse(i, asmExprParseCases[i], false); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
        try
        { testAsmExprParse(i, asmExprParseCases[i], true); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    }
    return retVal;
}
