/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
};

static AsmExprParseCase asmExprParseCases[] =
{
    /* literals */
    { "", "0", true, 0, "" },
    { "1", "1", true, 1, "" },
    { "1234893", "1234893", true, 1234893, "" },
    { "187378932789789", "187378932789789", true, 187378932789789UL, "" },
    { "01234", "668", true, 668, "" },
    { "0xaddab", "712107", true, 712107, "" },
    /* character literals */
    { "'a'", "97", true, 97, "" },
    { "'d'", "100", true, 100, "" },
    { "'A'", "65", true, 65, "" },
    { "'E'", "69", true, 69, "" },
    { "'\\a'", "7", true, 7, "" },
    { "'\\n'", "10", true, 10, "" },
    { "'\\b'", "8", true, 8, "" },
    { "'\\t'", "9", true, 9, "" },
    { "'\\t'", "9", true, 9, "" },
    { "'\\132'", "90", true, 90, "" },
    { "'\\x9A'", "154", true, 154, "" },
    /* symbols */
    { "xyz", "xyz", false, 0, "" },
    { ".sometest", ".sometest", false, 0, "" },
    /* simple expressions */
    { "-3", "3 !-", true, -3ULL, "" },
    { "+3", "3", true, 3, "" }, /* no plus operator, because is skipped */
    { "4-7+8", "4 7 - 8 +", true, 5, "" },
    { "2*9 + 11*5", "2 9 * 11 5 * +", true, 73, "" },
    /* operator ordering */
    { "--~-~x", "x ~ !- ~ !- !-", false, 0, "" },
    { "x+-y+~z+!w", "x y !- + z ~ + w ! +", false, 0, "" },
    { "a+b*x", "a b x * +", false, 0, "" },
    { "a-b*x", "a b x * -", false, 0, "" },
    { "a+b/x", "a b x / +", false, 0, "" },
    { "a-b/x", "a b x / -", false, 0, "" },
    { "a+b%x", "a b x % +", false, 0, "" },
    { "a-b%x", "a b x % -", false, 0, "" },
    { "a+b//x", "a b x // +", false, 0, "" },
    { "a-b//x", "a b x // -", false, 0, "" },
    { "a+b%%x", "a b x %% +", false, 0, "" },
    { "a-b%%x", "a b x %% -", false, 0, "" },
};

static std::string rpnExpression(const AsmExpression* expr)
{
    std::ostringstream oss;
    const AsmExprArg* args = expr->args.get();
    bool first = true;
    for (AsmExprOp op: expr->ops)
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
                : Assembler("", input, 0, msgOut)
    { readLine(); }
};

static void testAsmExprParse(cxuint i, const AsmExprParseCase& testCase)
{
    std::istringstream iss(testCase.expression);
    std::ostringstream resultErrorsOut;
    MyAssembler assembler(iss, resultErrorsOut);
    size_t linePos;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(assembler, 0, linePos));
    std::string resultRpnExpr;
    uint64_t resultValue = 0;
    bool resultEvaluated = false;
    if (expr)
    {
        resultRpnExpr = rpnExpression(expr.get());
        if (expr->symOccursNum == 0)
            resultEvaluated = expr->evaluate(assembler, resultValue);
        else
            resultEvaluated = false;
    }
    /* compare */
    std::string resultErrors = resultErrorsOut.str();
    if (::strcmp(testCase.rpnString, resultRpnExpr.c_str()) ||
        testCase.evaluated != resultEvaluated || testCase.value != resultValue ||
        ::strcmp(testCase.errors, resultErrors.c_str()))
    {
        std::ostringstream oss;
        oss << "FAILED for parseExpr#" << i << "\nResult: rpnString='" << resultRpnExpr <<
                "', value=" << resultValue << ", evaluated=" << int(resultEvaluated) <<
                ", errors='" << resultErrors << "'.\n"
                "Expected: rpnString='" << testCase.rpnString <<
                "', value=" << testCase.value <<
                ", evaluated=" << int(testCase.evaluated) <<
                ", errors='" << testCase.errors << "'";
        throw Exception(oss.str());
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(asmExprParseCases)/sizeof(AsmExprParseCase); i++)
        try
        { testAsmExprParse(i, asmExprParseCases[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
