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

CLIException::CLIException(const std::string& message)
{
    this->message = message;
}

CLIException::CLIException(const std::string& message, char shortName)
{
    this->message = "Option '-";
    this->message += shortName;
    this->message += "': ";
    this->message += message;
}

CLIException::CLIException(const std::string& message,
               const std::string& longName)
{
    this->message = "Option '--";
    this->message += longName;
    this->message += "': ";
    this->message += message;
}

CLIParser::CLIParser(const char* programName, const CLIOption* options,
        cxuint argc, const char** argv)
try
{
    shortNameMap = nullptr;
    this->programName = programName;
    this->options = options;
    this->argc = argc;
    this->argv = argv;
    
    shortNameMap = new cxuint[256];
    std::fill(shortNameMap, shortNameMap+256, UINT_MAX); // fill as unused
    
    cxuint i = 0;
    for (; options[i].longName != nullptr && options[i].shortName != 0; i++)
    {
        if (options[i].shortName != 0)
        {
            if (options[i].shortName == '-' || options[i].shortName == '=' ||
                cxuchar(options[i].shortName) <= 0x20)
                throw CLIException("Illegal short name");
            shortNameMap[cxuchar(options[i].shortName)] = i;
        }
        
        if (options[i].longName != nullptr)
        {
            if (::strchr(options[i].longName, '=') != nullptr)
                throw CLIException("Illegal long name");
            longNameMap.insert(std::make_pair(options[i].longName, i));
        }   
    }
    optionEntries.resize(i); // resize to number of options
}
catch(...)
{
    delete[] shortNameMap;
}

CLIParser::~CLIParser()
{
    delete[] shortNameMap;
}

void CLIParser::handleExceptionsForGetOptArg(cxuint optionId, CLIArgType argType)
{
    if (optionId < optionEntries.size())
        throw CLIException("No such command line option!");
    if (!optionEntries[optionId].isArg)
        throw CLIException("Command line option doesn't have argument!");
    if (argType != options[optionId].argType)
        throw CLIException("Argument type of command line option mismatch!");
}

void CLIParser::parse()
{   /* parse args */
    bool isLeftOver = false;
    for (cxuint i = 1; i < argc; i++)
    {
        if (argv[i] == nullptr)
            throw CLIException("Null argument!");
        const char* arg = argv[i];
        if (!isLeftOver && arg[0] == '-')
        {
            
            if (arg[1] == '-')
            {   // longNames
                if (arg[2] == 0)
                {   // if '--'
                    isLeftOver = true;
                    continue;
                }
                /* find option from long name */
                auto it = longNameMap.lower_bound(arg+2);
                const char* optLongName = nullptr;
                bool found = false;
                size_t optLongNameLen;
                if (it != longNameMap.end())
                {   /* check this same or greater key */
                    optLongName = options[it->second].longName;
                    optLongNameLen = ::strlen(optLongName);
                    found = (::strncmp(arg+2, optLongName, optLongNameLen) == 0 &&
                        (arg[2+optLongNameLen] == 0 || arg[2+optLongNameLen] == '='));
                }
                
                if (!found && it != longNameMap.begin())
                {   /* check previous (lower) entry in longNameMap */
                    --it;
                    optLongName = options[it->second].longName;
                    optLongNameLen = ::strlen(optLongName);
                    found = (::strncmp(arg+2, optLongName, optLongNameLen) == 0 &&
                        (arg[2+optLongNameLen] == 0 || arg[2+optLongNameLen] == '='));
                }
                
                if (!found) // unknown option
                    throw CLIException("Unknown command line option", arg+2);
                
                const cxuint optionId = it->second;
                const CLIOption& option = options[optionId];
                OptionEntry& optionEntry =  optionEntries[optionId];
                optionEntry.isSet = true;
                if (option.argType != CLIArgType::NONE)
                {
                    const char* optArg = nullptr;
                    if (arg[optLongNameLen+2] == '=' && arg[optLongNameLen+3] != 0)
                        // if option arg in this same arg elem
                        optArg = arg+optLongNameLen+3;
                    else if (i+1 < argc)
                    {
                        if (argv[i+1] != nullptr &&
                            (argv[i+1][0] != '-' || !option.argIsOptional))
                        {
                            i++; // next elem
                            if (argv[i] == nullptr)
                                throw CLIException("Null argument!");
                            optArg = argv[i];
                        }
                    }
                    else if (!option.argIsOptional)
                        throw CLIException("Missing argument", option.longName);
                    
                    if (optArg != nullptr)
                    {   /* parse option argument */
                    }
                }
            }
            else
            {   // short names
                arg++;
                for (; *arg != 0; arg++)
                {
                    const cxuint optionId = shortNameMap[cxuchar(*arg)];
                    if (optionId != UINT_MAX)
                    {
                        const CLIOption& option = options[optionId];
                        OptionEntry& optionEntry =  optionEntries[optionId];
                        optionEntry.isSet = true;
                        
                        if (option.argType != CLIArgType::NONE)
                            break;
                        {
                            const char* optArg = nullptr;
                            if (arg[1] == '=' && arg[2] != 0)
                                optArg = arg+2; // if after '='
                            else if (arg[1] != 0) // if option arg in this same arg elem
                                optArg = arg+1;
                            else if (i+1 < argc)
                            {
                                if (argv[i+1] != nullptr && 
                                    (argv[i+1][0] != '-' || !option.argIsOptional))
                                {
                                    i++; // next elem
                                    if (argv[i] == nullptr)
                                        throw CLIException("Null argument!");
                                    optArg = argv[i];
                                }
                            }
                            else if (!option.argIsOptional)
                                throw CLIException("Missing argument", option.shortName);
                            
                            if (optArg != nullptr)
                            {   /* parse option argument */
                            }
                            break; // end break parsing
                        }
                    }
                    else // if option is not known
                        throw CLIException("Unknown command line option", *arg);
                }
            }
        }
        else // left over args
            leftOverArgs.push_back(arg);
    }
}

void CLIParser::printHelp(std::ostream& os) const
{
}

void CLIParser::printUsage(std::ostream& os) const
{
}
