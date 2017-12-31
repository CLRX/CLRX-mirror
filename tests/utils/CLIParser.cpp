/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
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
#include <sstream>
#include <cstring>
#include <initializer_list>
#include <CLRX/utils/CLIParser.h>
#include "../TestUtils.h"

using namespace CLRX;

static const CLIOption programOptions1[] =
{
    { nullptr, 'X', CLIArgType::NONE, false, false, "buru help", nullptr },
    { "param0", '0', CLIArgType::INT, false, false, "some param0 for program", "PARAM0" },
    { "param1", '1', CLIArgType::UINT, true, false, "some param1 for program", nullptr },
    { "boolX", 'B', CLIArgType::BOOL, false, false, "boolean setup", "BOOLX" },
    { "bigNum", 'N', CLIArgType::UINT64, false, false, "bignum", "UBIGNUM" },
    { "sbigNum", 0, CLIArgType::INT64, false, false, "bignum", "SBIGNUM" },
    { "size", 's', CLIArgType::SIZE, false, false, "sizeof", "SIZEOF" },
    { "distance", 'd', CLIArgType::FLOAT, false, false,
        "distance between two points", "DIST" },
    { "dpval", 'D', CLIArgType::DOUBLE, false, false, "double precision", "DPVAL" },
    { "text", 't', CLIArgType::STRING, false, false, "text for some item", "TEXT" },
    { "name", 'n', CLIArgType::TRIMMED_STRING, false, false, "trimmed name", "NAME" },
    { "text2", 'T', CLIArgType::STRING, true, false, "text for some item 2", "TEXT2" },
    /* array parsing */
    { "boolArrX", 'b', CLIArgType::BOOL_ARRAY, false, false, "boolean arry", "BOOLARR" },
    { "intset", 'i', CLIArgType::INT_ARRAY, false, false, "set of numbers", "INTARR" },
    { "uintset", 'u', CLIArgType::UINT_ARRAY, false, false,
      "set of unsigned numbers", "UINTARR" },
    { "int64set", 0, CLIArgType::INT64_ARRAY, false, false, "set of bigints", "INTARR64" },
    { "uint64set", 0, CLIArgType::UINT64_ARRAY, false, false,
        "set of biguints", "UINTARR64" },
    { nullptr, 'Y', CLIArgType::SIZE_ARRAY, false, false, "set of sizeofs", "SIZEARR" },
    { "positions", 'p', CLIArgType::FLOAT_ARRAY, false, false, "positions", "POSITIONS" },
    { "powers", 'P', CLIArgType::DOUBLE_ARRAY, false, false, "powers", "POWERS" },
    { "titles", 0, CLIArgType::STRING_ARRAY, false, false, "set of titles", "STRINGS" },
    { "names", 0, CLIArgType::TRIMMED_STRING_ARRAY, false, false,
        "set of names", "NAMES" },
    CLRX_CLI_AUTOHELP
    { nullptr, 0 }
};

static const char* programArgv1_1[] =
{
    "prog1",
    "-X", "-0-12", "-1", "35", "-BON", "-N111111333311111",
    "--sbigNum=-11234511656565", "-s133", "-d2.5", "-D2.5",
    "-tala", "-n   ala ", "-Tkot", "-b1010100", "-i-12,55,-677", "-u42,64,677",
    "--int64set=12344444444444444,-5644,-4455445545454,2232344343",
    "--uint64set=1235555555555555,5644,4455445545454,2232344343",
    "-Y11,33,454", "-p1.5,1.5,-5.25", "-P3.5,18.5,-15.25",
    "--titles=ala,ma,kota,latek", "--names=   linux   , somebody,xxx,radeon   ",
    // doubled (memory leak check)
    "-X", "-0-12", "-1", "35", "-BON", "-N1111213331111111",
    "--sbigNum=-11234511656565", "-s133", "-d2.5", "-D12.5",
    "-takla", "-n   ala ", "-Tkot", "-b1010100", "-i-12,55,-677", "-u42,64,677",
    "--int64set=12344444444444444,-5644,-4455445545454,2232344343",
    "--uint64set=1235555555555555,5644,4455445545454,2232344343",
    "-Y11,33,454,55,59", "-p0.5,0.25,1.5,1.5,-5.25", "-P3.5,18.5,-15.25",
    "--titles=ala,ma,kotka,lateks", "--names=   linux   , somebody,xxx,radeon   ",
    nullptr
};

static const char* programArgv1_2[] =
{
    "prog1",
    "-X", "-0-12", "-1", "35", "-BON", "-N111111333311111",
    "--sbigNum=-11234511656565", "-s133", "-d2.5", "-D2.5",
    "-tala", "-n   ala ", "-Tkot", "-b1010100", "-i-12,55,-677", "-u42,64,677",
    "--int64set=12344444444444444,-5644,-4455445545454,2232344343",
    "--uint64set=1235555555555555,5644,4455445545454,2232344343",
    "-Y11,33,454", "-p1.5,1.5,-5.25", "-P3.5,18.5,-15.25",
    "--titles=ala,ma,kota,latek", "--names=   linux   , somebody,xxx,radeon   ",
    // doubled (memory leak check)
    "-X", "-0", "-12", "-1", "35", "-B", "ON", "-N", "1111213331111111",
    "--sbigNum", "-11234511656565", "-s", "  133", "-d", "2.5", "-D", "12.5",
    "-t", "akla", "-n", "   ala ", "-T", "kot", "-b", "1010100", "-i", "-12,55,-677",
    "-u", "42,64,677", "--int64set", "12344444444444444,-5644,-4455445545454,2232344343",
    "--uint64set", "1235555555555555,5644,4455445545454,2232344343",
    "-Y", "11,33,454,55,59", "-p", "0.5,0.25,1.5,1.5,-5.25", "-P", "3.5,18.5,-15.25",
    "--titles", "ala,ma,kotka,lateks", "--names", "   linux   , somebody,xxx,radeon   ",
    nullptr
};

static const char* programArgv1_3[] =
{
    "prog1",
    "-X", "-0-12", "-1", "35", "-BON", "-N111111333311111",
    "--sbigNum=-11234511656565", "-s133", "-d2.5", "-D2.5",
    "-tala", "-n   ala ", "-Tkot", "-b1010100", "-i-12,55,-677", "-u42,64,677",
    "--int64set=12344444444444444,-5644,-4455445545454,2232344343",
    "--uint64set=1235555555555555,5644,4455445545454,2232344343",
    "-Y11,33,454", "-p1.5,1.5,-5.25", "-P3.5,18.5,-15.25",
    "--titles=ala,ma,kota,latek", "--names=   linux   , somebody,xxx,radeon   ",
    // doubled (memory leak check)
    "-X", "-0=-12", "-1", "35", "-B=ON", "-N=1111213331111111",
    "--sbigNum=-11234511656565", "-s=133", "-d=2.5", "-D=12.5",
    "-t=akla", "-n=   ala ", "-T=kot", "-b=1010100", "-i=-12,55,-677", "-u=42,64,677",
    "--int64set=12344444444444444,-5644,-4455445545454,2232344343",
    "--uint64set=1235555555555555,5644,4455445545454,2232344343",
    "-Y=11,33,454,55,59", "-p=0.5,0.25,1.5,1.5,-5.25", "-P=3.5,18.5,-15.25",
    "--titles=ala,ma,kotka,lateks", "--names=   linux   , somebody,xxx,radeon   ",
    nullptr
};

static const char* programArgv1_4[] =
{
    "prog1",
    "-X", "-0=   -12", "-1", "35", "-B= \t  ON", "-N=\t 1111213331111111  ",
    "--sbigNum=    -11234511656565", "-s= 133", "-d=    2.5", "-D= \t  12.5   ",
    "-t=akla", "-n=   ala ", "-T=kot", "-b=     1010100",
    "-i=     -12,55    , -677   ", "-u=  42 ,64 \t,\t677",
    "--int64set=  12344444444444444  \t,-5644,-4455445545454,2232344343",
    "--uint64set=      1235555555555555,    5644,4455445545454   ,2232344343",
    "-Y=  11,33,454,55,59", "-p=    0.5,0.25,1.5,1.5,-5.25", "-P=    3.5,   18.5,-15.25",
    "--titles=ala,ma,kotka,lateks", "--names=   linux   , somebody,xxx,radeon   ",
    nullptr
};

static const char* programArgv1_5[] =
{
    "prog1",
    "-X", "--param0=-12", "--param1", "35", "--boolX=ON", "--bigNum=1111213331111111",
    "--sbigNum=-11234511656565", "--size=133", "--distance=2.5", "--dpval=12.5",
    "--text=akla", "--name=   ala ", "--text2=kot", "--boolArrX=1010100",
    "--intset=-12,55,-677", "--uintset=42,64,677",
    "--int64set=12344444444444444,-5644,-4455445545454,2232344343",
    "--uint64set=1235555555555555,5644,4455445545454,2232344343",
    "-Y11,33,454,55,59", "--positions=0.5,0.25,1.5,1.5,-5.25", "--powers=3.5,18.5,-15.25",
    "--titles=ala,ma,kotka,lateks", "--names=   linux   , somebody,xxx,radeon   ",
    nullptr
};

// testing CLI parser
static void testCLI1(const char* name, int argc, const char** argv)
{
    std::ostringstream oss;
    oss << "testCLI1#" << name;
    oss.flush();
    const std::string testName = oss.str();
    CLIParser cli("CLIParserTest", programOptions1, argc, argv);
    cli.parse();
    assertTrue(testName, "hasOption(-X)", cli.hasOption(0));
    assertTrue(testName, "hasNoArg(-X)", !cli.hasOptArg(0));
    assertTrue(testName, "hasOption(-0)", cli.hasOption(1));
    assertTrue(testName, "hasArg(-0)", cli.hasOptArg(1));
    assertValue(testName, "getOptArg(-0)", int32_t(-12), cli.getOptArg<int32_t>(1));
    assertTrue(testName, "hasOption(-1)", cli.hasOption(2));
    assertTrue(testName, "hasArg(-1)", cli.hasOptArg(2));
    assertValue(testName, "getOptArg(-1)", uint32_t(35), cli.getOptArg<uint32_t>(2));
    assertTrue(testName, "hasOption(-B)", cli.hasOption(3));
    assertTrue(testName, "hasArg(-B)", cli.hasOptArg(3));
    assertValue(testName, "getOptArg(-B)", true, cli.getOptArg<bool>(3));
    assertTrue(testName, "hasOption(-N)", cli.hasOption(4));
    assertTrue(testName, "hasArg(-N)", cli.hasOptArg(4));
    assertValue(testName, "getOptArg(-N)", uint64_t(1111213331111111),
            cli.getOptArg<uint64_t>(4));
    assertTrue(testName, "hasOption(--sbigNum)", cli.hasOption(5));
    assertTrue(testName, "hasArg(--sbigNum)", cli.hasOptArg(5));
    assertValue(testName, "getOptArg(--sbigNum)", int64_t(-11234511656565),
            cli.getOptArg<int64_t>(5));
    assertTrue(testName, "hasOption(-s)", cli.hasOption(6));
    assertTrue(testName, "hasArg(-s)", cli.hasOptArg(6));
    assertValue(testName, "getOptArg(-s)", size_t(133), cli.getOptArg<size_t>(6));
    assertTrue(testName, "hasOption(-d)", cli.hasOption(7));
    assertTrue(testName, "hasArg(-d)", cli.hasOptArg(7));
    assertValue(testName, "getOptArg(-d)", 2.5f, cli.getOptArg<float>(7));
    assertTrue(testName, "hasOption(-D)", cli.hasOption(8));
    assertTrue(testName, "hasArg(-D)", cli.hasOptArg(8));
    assertValue(testName, "getOptArg(-D)", 12.5, cli.getOptArg<double>(8));
    assertTrue(testName, "hasOption(-t)", cli.hasOption(9));
    assertTrue(testName, "hasArg(-t)", cli.hasOptArg(9));
    assertString(testName, "getOptArg(-t)", "akla", cli.getOptArg<const char*>(9));
    assertTrue(testName, "hasOption(-n)", cli.hasOption(10));
    assertTrue(testName, "hasArg(-n)", cli.hasOptArg(10));
    assertString(testName, "getOptArg(-n)", "ala", cli.getOptArg<const char*>(10));
    assertTrue(testName, "hasOption(-T)", cli.hasOption(11));
    assertTrue(testName, "hasArg(-T)", cli.hasOptArg(11));
    assertString(testName, "getOptArg(-T)", "kot", cli.getOptArg<const char*>(11));
    assertTrue(testName, "hasOption(-b)", cli.hasOption(12));
    assertTrue(testName, "hasArg(-b)", cli.hasOptArg(12));
    size_t boolArrLength;
    const bool* boolArr = cli.getOptArgArray<bool>(12, boolArrLength);
    assertArray(testName, "getOptArg(-b)",
        { true, false, true, false, true, false, false }, boolArrLength, boolArr);
    
    assertTrue(testName, "hasOption(-i)", cli.hasOption(13));
    assertTrue(testName, "hasArg(-i)", cli.hasOptArg(13));
    size_t intArrLength;
    const int32_t* intArr = cli.getOptArgArray<int32_t>(13, intArrLength);
    assertArray(testName, "getOptArg(-i)", { int32_t(-12), int32_t(55),
                int32_t(-677) }, intArrLength, intArr);
    
    assertTrue(testName, "hasOption(-u)", cli.hasOption(14));
    assertTrue(testName, "hasArg(-u)", cli.hasOptArg(14));
    size_t uintArrLength;
    const uint32_t* uintArr = cli.getOptArgArray<uint32_t>(14, uintArrLength);
    assertArray(testName, "getOptArg(-u)", { uint32_t(42), uint32_t(64),
                uint32_t(677) }, uintArrLength, uintArr);
    
    assertTrue(testName, "hasOption(--int64set)", cli.hasOption(15));
    assertTrue(testName, "hasArg(--int64set)", cli.hasOptArg(15));
    size_t int64ArrLength;
    const int64_t* int64Arr = cli.getOptArgArray<int64_t>(15, int64ArrLength);
    assertArray(testName, "getOptArg(--int64set)",
            { int64_t(12344444444444444LL), int64_t(-5644LL),int64_t(-4455445545454LL),
                int64_t(2232344343LL) }, int64ArrLength, int64Arr);
    
    assertTrue(testName, "hasOption(--uint64set)", cli.hasOption(16));
    assertTrue(testName, "hasArg(--uint64set)", cli.hasOptArg(16));
    size_t uint64ArrLength;
    const uint64_t* uint64Arr = cli.getOptArgArray<uint64_t>(16, uint64ArrLength);
    assertArray(testName, "getOptArg(--uint64set)",
        { uint64_t(1235555555555555ULL), uint64_t(5644ULL), uint64_t(4455445545454ULL),
            uint64_t(2232344343ULL) }, uint64ArrLength, uint64Arr);
    
    assertTrue(testName, "hasOption(-Y)", cli.hasOption(17));
    assertTrue(testName, "hasArg(-Y)", cli.hasOptArg(17));
    size_t sizeArrLength;
    const size_t* sizeArr = cli.getOptArgArray<size_t>(17, sizeArrLength);
    assertArray(testName, "getOptArg(-Y)", { size_t(11),size_t(33),size_t(454),
            size_t(55),size_t(59) }, sizeArrLength, sizeArr);
    
    assertTrue(testName, "hasOption(-p)", cli.hasOption(18));
    assertTrue(testName, "hasArg(-p)", cli.hasOptArg(18));
    size_t floatArrLength;
    const float* floatArr = cli.getOptArgArray<float>(18, floatArrLength);
    assertArray(testName, "getOptArg(-p)", { 0.5f,0.25f,1.5f,1.5f,-5.25f },
                floatArrLength, floatArr);
    
    assertTrue(testName, "hasOption(-P)", cli.hasOption(19));
    assertTrue(testName, "hasArg(-P)", cli.hasOptArg(19));
    size_t doubleArrLength;
    const double* doubleArr = cli.getOptArgArray<double>(19, doubleArrLength);
    assertArray(testName, "getOptArg(-P)", { 3.5,18.5,-15.25 },
                doubleArrLength, doubleArr);
    
    assertTrue(testName, "hasOption(--titles)", cli.hasOption(20));
    assertTrue(testName, "hasArg(--titles)", cli.hasOptArg(20));
    size_t strArrLength;
    const char* const* strArr = cli.getOptArgArray<const char*>(20, strArrLength);
    assertStrArray(testName, "getOptArg(--titles)", { "ala","ma","kotka","lateks" },
                strArrLength, (const char**)strArr);
    
    assertTrue(testName, "hasOption(--names)", cli.hasOption(21));
    assertTrue(testName, "hasArg(--names)", cli.hasOptArg(21));
    size_t namesArrLength;
    const char* const* namesArr = cli.getOptArgArray<const char*>(21, namesArrLength);
    assertStrArray(testName, "getOptArg(--names)", { "linux", "somebody",
            "xxx", "radeon" }, namesArrLength, (const char**)namesArr);
}

// check type in option
template<typename T1>
bool isTypeMatch(const CLIParser& cli, cxuint optId)
{
    try
    { cli.getOptArg<T1>(optId); }
    catch(const CLIException& ex)
    {
        if (::strcmp(ex.what(), "Argument type of option mismatch!") == 0)
            return false;
        throw; // other error
    }
    return true;
}

// testin type matching
static void testCLI1TypeMatch(const char* name, int argc, const char** argv)
{
    std::ostringstream oss;
    oss << "testCLI1TypeMismatch#" << name;
    oss.flush();
    const std::string testName = oss.str();
    CLIParser cli("CLIParserTest", programOptions1, argc, argv);
    cli.parse();
    assertTrue(testName, "-O(int)!=(uint)", !isTypeMatch<uint32_t>(cli, 1));
    assertTrue(testName, "-O(int)==(int)", isTypeMatch<int32_t>(cli, 1));
    assertTrue(testName, "-O(int)!=(int64)", !isTypeMatch<int64_t>(cli, 1));
    assertTrue(testName, "-B(bool)!=(uint)", !isTypeMatch<uint32_t>(cli, 3));
    assertTrue(testName, "-B(bool)==(bool)", isTypeMatch<bool>(cli, 3));
    assertTrue(testName, "-t(char*)==(char*)", isTypeMatch<const char*>(
                cli, cli.findOption('t')));
    assertTrue(testName, "-n(char*)==(char*)", isTypeMatch<const char*>(
                cli, cli.findOption('n')));
    assertTrue(testName, "--titles(char**)==(char**)", isTypeMatch<const char**>(
                cli, cli.findOption("titles")));
    assertTrue(testName, "--names(char**)==(char**)", isTypeMatch<const char**>(
                cli, cli.findOption("names")));
}

static const CLIOption programOptionsLong[] =
{
    { "x", '1', CLIArgType::INT, false, false, "parameter x", "PARAM" },
    { "x=x", '2', CLIArgType::INT, false, false, "parameter x=x", "PARAM" },
    { "x=x=x", '3', CLIArgType::INT, false, false, "parameter x=x=x", "PARAM" },
    { "x=x=x=x", '4', CLIArgType::INT, false, false, "parameter x=x=x=x", "PARAM" },
    { "x=x=x=x=x", '5', CLIArgType::INT, false, false, "parameter x=x=x=x=x", "PARAM" },
    { "x=x=x=x=x=x", '6', CLIArgType::INT, false, false,
                "parameter x=x=x=x=x=x", "PARAM" },
    CLRX_CLI_AUTOHELP
    { nullptr, 0 }
};

static const char* programArgvLong_1[] =
{ "prog1", "-11246", "-21331", "-3232", "-4781", "-5832", "-6637", nullptr };

static const char* programArgvLong_2[] =
{ "prog1", "-1=1246", "-2=1331", "-3=232", "-4=781", "-5=832", "-6=637", nullptr };

static const char* programArgvLong_3[] =
{
    "prog1", "--x=1246", "--x=x=1331", "--x=x=x=232", "--x=x=x=x=781", "--x=x=x=x=x=832",
    "--x=x=x=x=x=x=637", nullptr
};

static const char* programArgvLong_4[] =
{
    "prog1", "--x", "1246", "--x=x", "1331", "--x=x=x", "232", "--x=x=x=x", "781",
    "--x=x=x=x=x", "832", "--x=x=x=x=x=x", "637", nullptr
};

// testing long options
static void testCLILong(const char* name, int argc, const char** argv)
{
    std::ostringstream oss;
    oss << "testCLILong#" << name;
    oss.flush();
    const std::string testName = oss.str();
    CLIParser cli("CLIParserTest", programOptionsLong, argc, argv);
    cli.parse();
    assertTrue(testName, "hasOption(-1)", cli.hasOption(0));
    assertTrue(testName, "hasNoArg(-1)", cli.hasOptArg(0));
    assertValue(testName, "getOptArg(-1)", int32_t(1246), cli.getOptArg<int32_t>(0));
    assertTrue(testName, "hasOption(-2)", cli.hasOption(1));
    assertTrue(testName, "hasNoArg(-2)", cli.hasOptArg(1));
    assertValue(testName, "getOptArg(-2)", int32_t(1331), cli.getOptArg<int32_t>(1));
    assertTrue(testName, "hasOption(-3)", cli.hasOption(2));
    assertTrue(testName, "hasNoArg(-3)", cli.hasOptArg(2));
    assertValue(testName, "getOptArg(-3)", int32_t(232), cli.getOptArg<int32_t>(2));
    assertTrue(testName, "hasOption(-4)", cli.hasOption(3));
    assertTrue(testName, "hasNoArg(-4)", cli.hasOptArg(3));
    assertValue(testName, "getOptArg(-4)", int32_t(781), cli.getOptArg<int32_t>(3));
    assertTrue(testName, "hasOption(-5)", cli.hasOption(4));
    assertTrue(testName, "hasNoArg(-5)", cli.hasOptArg(4));
    assertValue(testName, "getOptArg(-5)", int32_t(832), cli.getOptArg<int32_t>(4));
    assertTrue(testName, "hasOption(-6)", cli.hasOption(5));
    assertTrue(testName, "hasNoArg(-6)", cli.hasOptArg(5));
    assertValue(testName, "getOptArg(-6)", int32_t(637), cli.getOptArg<int32_t>(5));
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testCLI1, "1", sizeof(programArgv1_1)/sizeof(const char*)-1,
               programArgv1_1);
    retVal |= callTest(testCLI1, "2", sizeof(programArgv1_2)/sizeof(const char*)-1,
               programArgv1_2);
    retVal |= callTest(testCLI1, "3", sizeof(programArgv1_3)/sizeof(const char*)-1,
               programArgv1_3);
    retVal |= callTest(testCLI1, "4", sizeof(programArgv1_4)/sizeof(const char*)-1,
               programArgv1_4);
    retVal |= callTest(testCLI1, "5", sizeof(programArgv1_5)/sizeof(const char*)-1,
               programArgv1_5);
    retVal |= callTest(testCLI1TypeMatch, "1",
           sizeof(programArgv1_1)/sizeof(const char*)-1, programArgv1_1);
    
    retVal |= callTest(testCLILong, "1", sizeof(programArgvLong_1)/sizeof(const char*)-1,
               programArgvLong_1);
    retVal |= callTest(testCLILong, "2", sizeof(programArgvLong_2)/sizeof(const char*)-1,
               programArgvLong_2);
    retVal |= callTest(testCLILong, "3", sizeof(programArgvLong_3)/sizeof(const char*)-1,
               programArgvLong_3);
    retVal |= callTest(testCLILong, "4", sizeof(programArgvLong_4)/sizeof(const char*)-1,
               programArgvLong_4);
    return retVal;
}
