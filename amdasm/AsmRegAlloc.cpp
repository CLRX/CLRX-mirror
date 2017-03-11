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
#include <stack>
#include <vector>
#include <utility>
#include <unordered_map>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

ISAUsageHandler::ISAUsageHandler(const std::vector<cxbyte>& _content) :
            content(_content), lastOffset(0), readOffset(0), instrStructPos(0),
            regUsagesPos(0), regUsages2Pos(0), regVarUsagesPos(0),
            pushedArgs(0), argPos(0), argFlags(0), isNext(false), useRegMode(false)
{ }

ISAUsageHandler::~ISAUsageHandler()
{ }

void ISAUsageHandler::rewind()
{
    readOffset = instrStructPos = 0;
    regUsagesPos = regUsages2Pos = regVarUsagesPos = 0;
    useRegMode = false;
    pushedArgs = 0;
    skipBytesInInstrStruct();
}

void ISAUsageHandler::skipBytesInInstrStruct()
{
    // do not add instruction size if usereg (usereg immediately before instr regusages)
    if ((instrStructPos != 0 || argPos != 0) && !useRegMode)
        readOffset += defaultInstrSize;
    argPos = 0;
    for (;instrStructPos < instrStruct.size() &&
        instrStruct[instrStructPos] > 0x80; instrStructPos++)
        readOffset += (instrStruct[instrStructPos] & 0x7f);
    isNext = (instrStructPos < instrStruct.size());
}

void ISAUsageHandler::putSpace(size_t offset)
{
    if (lastOffset != offset)
    {
        flush(); // flush before new instruction
        // useReg immediately before instruction regusages
        size_t defaultInstrSize = (!useRegMode ? this->defaultInstrSize : 0);
        if (lastOffset > offset)
            throw Exception("Offset before previous instruction");
        if (!instrStruct.empty() && offset - lastOffset < defaultInstrSize)
            throw Exception("Offset between previous instruction");
        size_t toSkip = !instrStruct.empty() ? 
                offset - lastOffset - defaultInstrSize : offset;
        while (toSkip > 0)
        {
            size_t skipped = std::min(toSkip, size_t(0x7f));
            instrStruct.push_back(skipped | 0x80);
            toSkip -= skipped;
        }
        lastOffset = offset;
        argFlags = 0;
        pushedArgs = 0;
    } 
}

void ISAUsageHandler::pushUsage(const AsmRegVarUsage& rvu)
{
    if (lastOffset == rvu.offset && useRegMode)
        flush(); // only flush if useRegMode and no change in offset
    else // otherwise
        putSpace(rvu.offset);
    useRegMode = false;
    if (rvu.regVar != nullptr)
    {
        argFlags |= (1U<<pushedArgs);
        regVarUsages.push_back({ rvu.regVar, rvu.rstart, rvu.rend, rvu.regField,
            rvu.rwFlags, rvu.align });
    }
    else // reg usages
        regUsages.push_back({ rvu.regField,cxbyte(rvu.rwFlags |
                    getRwFlags(rvu.regField, rvu.rstart, rvu.rend)) });
    pushedArgs++;
}

void ISAUsageHandler::pushUseRegUsage(const AsmRegVarUsage& rvu)
{
    if (lastOffset == rvu.offset && !useRegMode)
        flush(); // only flush if useRegMode and no change in offset
    else // otherwise
        putSpace(rvu.offset);
    useRegMode = true;
    if (pushedArgs == 0 || pushedArgs == 256)
    {
        argFlags = 0;
        pushedArgs = 0;
        instrStruct.push_back(0x80); // sign of regvarusage from usereg
        instrStruct.push_back(0);
    }
    if (rvu.regVar != nullptr)
    {
        argFlags |= (1U<<(pushedArgs & 7));
        regVarUsages.push_back({ rvu.regVar, rvu.rstart, rvu.rend, rvu.regField,
            rvu.rwFlags, rvu.align });
    }
    else // reg usages
        regUsages2.push_back({ rvu.rstart, rvu.rend, rvu.rwFlags });
    pushedArgs++;
    if ((pushedArgs & 7) == 0) // just flush per 8 bit
    {
        instrStruct.push_back(argFlags);
        instrStruct[instrStruct.size() - ((pushedArgs+7) >> 3) - 1] = pushedArgs;
        argFlags = 0;
    }
}

void ISAUsageHandler::flush()
{
    if (pushedArgs != 0)
    {
        if (!useRegMode)
        {   // normal regvarusages
            instrStruct.push_back(argFlags);
            if ((argFlags & (1U<<(pushedArgs-1))) != 0)
                regVarUsages.back().rwFlags |= 0x80;
            else // reg usages
                regUsages.back().rwFlags |= 0x80;
        }
        else // use reg regvarusages
        {
            if ((pushedArgs & 7) != 0) //if only not pushed args remains
                instrStruct.push_back(argFlags);
            instrStruct[instrStruct.size() - ((pushedArgs+7) >> 3) - 1] = pushedArgs;
        }
    }
}

AsmRegVarUsage ISAUsageHandler::nextUsage()
{
    if (!isNext)
        throw Exception("No reg usage in this code");
    AsmRegVarUsage rvu;
    // get regvarusage
    bool lastRegUsage = false;
    rvu.offset = readOffset;
    if (!useRegMode && instrStruct[instrStructPos] == 0x80)
    {   // useRegMode (begin fetching useregs)
        useRegMode = true;
        argPos = 0;
        instrStructPos++;
        // pushedArgs - numer of useregs, 0 - 256 useregs
        pushedArgs = instrStruct[instrStructPos++];
        argFlags = instrStruct[instrStructPos];
    }
    
    if ((instrStruct[instrStructPos] & (1U << (argPos&7))) != 0)
    {   // regvar usage
        const AsmRegVarUsageInt& inRVU = regVarUsages[regVarUsagesPos++];
        rvu.regVar = inRVU.regVar;
        rvu.rstart = inRVU.rstart;
        rvu.rend = inRVU.rend;
        rvu.regField = inRVU.regField;
        rvu.rwFlags = inRVU.rwFlags & ASMRVU_ACCESS_MASK;
        rvu.align = inRVU.align;
        if (!useRegMode)
            lastRegUsage = ((inRVU.rwFlags&0x80) != 0);
    }
    else if (!useRegMode)
    {   // simple reg usage
        const AsmRegUsageInt& inRU = regUsages[regUsagesPos++];
        rvu.regVar = nullptr;
        const std::pair<uint16_t, uint16_t> regPair =
                    getRegPair(inRU.regField, inRU.rwFlags);
        rvu.rstart = regPair.first;
        rvu.rend = regPair.second;
        rvu.rwFlags = (inRU.rwFlags & ASMRVU_ACCESS_MASK);
        rvu.regField = inRU.regField;
        rvu.align = 0;
        lastRegUsage = ((inRU.rwFlags&0x80) != 0);
    }
    else
    {   // use reg (simple reg usage, second structure)
        const AsmRegUsage2Int& inRU = regUsages2[regUsages2Pos++];
        rvu.regVar = nullptr;
        rvu.rstart = inRU.rstart;
        rvu.rend = inRU.rend;
        rvu.rwFlags = inRU.rwFlags;
        rvu.regField = ASMFIELD_NONE;
        rvu.align = 0;
    }
    argPos++;
    if (useRegMode)
    {   // if inside useregs
        if (argPos == (pushedArgs&0xff))
        {
            instrStructPos++; // end
            skipBytesInInstrStruct();
            useRegMode = false;
        }
        else if ((argPos & 7) == 0) // fetch new flag
        {
            instrStructPos++;
            argFlags = instrStruct[instrStructPos];
        }
    }
    // after instr
    if (lastRegUsage)
    {
        instrStructPos++;
        skipBytesInInstrStruct();
    }
    return rvu;
}

AsmRegAllocator::AsmRegAllocator(Assembler& _assembler) : assembler(_assembler)
{ }

void AsmRegAllocator::createCodeStructure(const std::vector<AsmCodeFlowEntry>& codeFlow,
             size_t codeSize, const cxbyte* code)
{
    ISAAssembler* isaAsm = assembler.isaAssembler;
    std::vector<size_t> splits;
    std::vector<size_t> codeStarts;
    std::vector<size_t> codeEnds;
    codeStarts.push_back(0);
    codeEnds.push_back(codeSize);
    for (const AsmCodeFlowEntry& entry: codeFlow)
    {
        size_t instrAfter = 0;
        if (entry.type == AsmCodeFlowType::JUMP || entry.type == AsmCodeFlowType::CJUMP)
            instrAfter = entry.offset + isaAsm->getInstructionSize(
                        codeSize - entry.offset, code + entry.offset);
        
        switch(entry.type)
        {
            case AsmCodeFlowType::START:
                codeStarts.push_back(entry.offset);
                break;
            case AsmCodeFlowType::END:
                codeEnds.push_back(entry.offset);
                break;
            case AsmCodeFlowType::JUMP:
                splits.push_back(entry.target);
                codeEnds.push_back(instrAfter);
                break;
            case AsmCodeFlowType::CJUMP:
                splits.push_back(entry.target);
                splits.push_back(instrAfter);
                break;
            case AsmCodeFlowType::CALL:
                splits.push_back(entry.target);
                break;
            case AsmCodeFlowType::RETURN:
                codeEnds.push_back(instrAfter);
                break;
            default:
                break;
        }
    }
    std::sort(splits.begin(), splits.end());
    splits.resize(std::unique(splits.begin(), splits.end()) - splits.begin());
    std::sort(codeEnds.begin(), codeEnds.end());
    codeEnds.resize(std::unique(codeEnds.begin(), codeEnds.end())
                - codeEnds.begin());
    // add next codeStarts
    auto splitIt = splits.begin();
    for (size_t codeEnd: codeEnds)
    {
        auto it = std::lower_bound(splitIt, splits.end(), codeEnd);
        if (it != splits.end())
        {
            codeStarts.push_back(*it);
            splitIt = it;
        }
        else // if end
            break;
    }
    std::sort(codeStarts.begin(), codeStarts.end());
    codeStarts.resize(std::unique(codeStarts.begin(), codeStarts.end())
                - codeStarts.begin());
    // divide to blocks
    for (size_t codeStart: codeStarts)
    {
        size_t codeEnd = *std::lower_bound(codeEnds.begin(), codeEnds.end(), codeStart);
        splitIt = std::lower_bound(splitIt, splits.end(), codeStart);
        if (splitIt == splits.end())
            break; // end of code blocks
        for (size_t start = *splitIt; start < codeEnd;)
        {
            start = *splitIt;
            ++splitIt;
            size_t end = codeEnd;
            if (splitIt != splits.end())
                end = std::min(end, *splitIt);
            codeBlocks.push_back({ start, end, { } });
        }
        if (splitIt == splits.end())
            break; // end of code blocks
    }
    // flags = 1 - have call, 2 - have return, 4 - have jump
    struct CodeBlockInfo
    {
        cxbyte flags;
        std::vector<bool> nextCalls; // true if next is call
        size_t nextRet; // next block with return
        CodeBlockInfo() : flags(0), nextRet(SIZE_MAX)
        { }
    };
    std::vector<CodeBlockInfo> codeBlockInfos(codeBlocks.size());
    
    // construct flow-graph
    for (const AsmCodeFlowEntry& entry: codeFlow)
        if (entry.type == AsmCodeFlowType::CALL || entry.type == AsmCodeFlowType::JUMP ||
            entry.type == AsmCodeFlowType::CJUMP || entry.type == AsmCodeFlowType::RETURN)
        {
            auto it = binaryFind(codeBlocks.begin(), codeBlocks.end(),
                    CodeBlock{ entry.target },
                    [](const CodeBlock& c1, const CodeBlock& c2)
                    { return c1.start < c2.end; });
            
            if (entry.type == AsmCodeFlowType::RETURN)
            {   // if block have return
                if (it == codeBlocks.end())
                    continue;
                codeBlockInfos[it - codeBlocks.begin()].flags = 2;
                continue;
            }
            if (it != codeBlocks.end())
                codeBlockInfos[it - codeBlocks.begin()].flags = 
                    entry.type == AsmCodeFlowType::CALL ? 1 : 4;
            
            size_t instrAfter = entry.offset + isaAsm->getInstructionSize(
                        codeSize - entry.offset, code + entry.offset);
            auto it2 = binaryFind(codeBlocks.begin(), codeBlocks.end(),
                    CodeBlock{ instrAfter },
                    [](const CodeBlock& c1, const CodeBlock& c2)
                    { return c1.start < c2.end; });
            if (it == codeBlocks.end() || it2 == codeBlocks.end())
                continue; // error!
            it->nexts.push_back(it2 - codeBlocks.begin());
            codeBlockInfos[it - codeBlocks.begin()].nextCalls.push_back(
                    entry.type == AsmCodeFlowType::CALL);
            if (entry.type != AsmCodeFlowType::JUMP) // add next next block
                it->nexts.push_back(it - codeBlocks.begin() + 1);
        }
    // add direct next
    for (size_t i = 0; i < codeBlocks.size(); i++)
    {
    }
    // add return nexts
    std::vector<bool> visited(codeBlocks.size());
    struct CallStackEntry
    {
        size_t callBlock; // index
        size_t callNextIndex; // index of call next
        std::vector<size_t> returns; // returns
    };
    std::stack<CallStackEntry> callStack;
    std::unordered_map<size_t, Array<size_t> > routineMap;
    
    struct FlowStackEntry
    {
        size_t blockIndex;
        size_t nextIndex;
    };
    std::stack<FlowStackEntry> flowStack;
    flowStack.push({ 0, 0 });
    
    /// collect returns next
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.top();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        if (entry.nextIndex == 0)
        {
            if (!visited[entry.blockIndex])
                visited[entry.blockIndex] = true;
            else
            {   // back, already visited
                flowStack.pop();
                continue;
            }
        }
        if (entry.nextIndex < cblock.nexts.size())
        {
            flowStack.push({ cblock.nexts[entry.nextIndex], 0 });
            entry.nextIndex++;
        }
        else // back
            flowStack.pop();
    }
    
    visited.clear();
    // real resolving returns nexts
    flowStack.push({ 0, 0 });
    while (!flowStack.empty())
    {
        FlowStackEntry& entry = flowStack.top();
        const CodeBlock& cblock = codeBlocks[entry.blockIndex];
        if (entry.nextIndex == 0)
        { // process current block
            if (!visited[entry.blockIndex])
            {
                visited[entry.blockIndex] = true;
                if (callStack.empty())
                    continue;
                CallStackEntry& centry = callStack.top();
                if ((codeBlockInfos[entry.blockIndex].flags & 2) != 0)
                    // add return to stack
                    centry.returns.push_back(entry.blockIndex);
                else if ((codeBlockInfos[entry.blockIndex].flags & 1) != 0 &&
                    entry.blockIndex == centry.callBlock &&
                    entry.nextIndex-1 == centry.callNextIndex)
                {   // insert new routine
                    routineMap.insert(std::make_pair(
                        codeBlocks[centry.callBlock].nexts[centry.callNextIndex],
                        Array<size_t>(centry.returns.begin(), centry.returns.end())));
                    
                    for (size_t retIndex: centry.returns)
                        codeBlocks[retIndex].nexts.push_back(entry.blockIndex+1);
                    callStack.pop(); // just return from call
                }
            }
            else
            {   // back, already visited
                flowStack.pop();
                continue;
            }
        }
        if (entry.nextIndex < cblock.nexts.size())
        {
            if (codeBlockInfos[entry.blockIndex].nextCalls[entry.nextIndex])
            {
                auto it = routineMap.find(cblock.nexts[entry.nextIndex]);
                if (it == routineMap.end())
                    // add return block to callstack
                    callStack.push({ entry.blockIndex, entry.nextIndex });
                else
                {   // if routine already visited
                    auto it = routineMap.find(cblock.nexts[entry.nextIndex]);
                    if (it != routineMap.end())
                        for (size_t retIndex: it->second)
                            codeBlocks[retIndex].nexts.push_back(entry.blockIndex+1);
                    entry.nextIndex++;
                    continue;
                }
            }
            flowStack.push({ cblock.nexts[entry.nextIndex], 0 });
            entry.nextIndex++;
        }
        else // back
            flowStack.pop();
    }
}

void AsmRegAllocator::allocateRegisters(cxuint sectionId)
{   // before any operation, clear all
    codeBlocks.clear();
    // set up
    const AsmSection& section = assembler.sections[sectionId];
    createCodeStructure(section.codeFlow, section.content.size(), section.content.data());
}
