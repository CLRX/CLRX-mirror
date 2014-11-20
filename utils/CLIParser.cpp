/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
#include <algorithm>
#include <cstring>
#include <string>
#include <climits>
#include <map>
#include <CLRX/Utilities.h>
#include <CLRX/CLIParser.h>

using namespace CLRX;

CLIParser::CLIParser(const char* programName, const CLIOption* options,
        cxuint argc, const char** argv) : leftOverArgsNum(0), leftOverArgs(nullptr)
{
    this->programName = programName;
    this->options = options;
    this->argc = argc;
    this->argv = argv;
}

CLIParser::~CLIParser()
{
    delete[] leftOverArgs;
}

void CLIParser::handleExceptionsForGetOptArg(cxuint optionId, CLIArgType argType)
{
    if (optionId < optionEntries.size())
        throw Exception("No such command line option!");
    if (!optionEntries[optionId].isArg)
        throw Exception("Command line option doesn't have argument!");
    if (argType != options[optionId].argType)
        throw Exception("Argument type of command line option mismatch!");
}

typedef std::map<const char*, cxuint, CLRX::CStringLess> CLIOptionsMap;

void CLIParser::parse()
{
    CLIOptionsMap longNameMap;
    cxuint shortNameMap[256];
    
    std::fill(shortNameMap, shortNameMap+256, UINT_MAX);
    for (cxuint i = 0; options[i].longName != nullptr && options[i].shortName != 0; i++)
    {
        if (options[i].longName != nullptr)
            longNameMap.insert(std::make_pair(options[i].longName, i));
        if (options[i].shortName != 0)
            shortNameMap[cxuchar(options[i].shortName)] = i;
    }
    /* parse args */
    for (cxuint i = 1; i < argc; i++)
    {
    }
}
