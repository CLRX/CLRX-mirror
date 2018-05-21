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
#include <string>
#include <fstream>
#include <vector>
#include <utility>
#include <algorithm>
#include <atomic>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

/* about column counting: simple input filters (source) handles correctly column counting
 * Unfortunatelly: macro argument substitution can change column numeration, thus
 * macro input filters counts incorectly columns (correct solution requires
 * cumbersome code changes) */

static std::atomic<size_t> asmSourceCounter(0);

AsmSource::AsmSource(AsmSourceType _type) : uniqueId(asmSourceCounter.fetch_add(1)),
                type(_type)
{ }

AsmSource::~AsmSource()
{ }

AsmFile::~AsmFile()
{ }

AsmMacroSource::~AsmMacroSource()
{ }

AsmRepeatSource::~AsmRepeatSource()
{ }

static std::atomic<size_t> asmMacroSubstCounter(0);

AsmMacroSubst::AsmMacroSubst(RefPtr<const AsmSource> _source, LineNo _lineNo, ColNo _colNo)
            : uniqueId(asmMacroSubstCounter.fetch_add(1)),
              source(_source), lineNo(_lineNo), colNo(_colNo)
{ }

AsmMacroSubst::AsmMacroSubst(RefPtr<const AsmMacroSubst> _parent,
            RefPtr<const AsmSource> _source, LineNo _lineNo, ColNo _colNo)
            : uniqueId(asmMacroSubstCounter.fetch_add(1)),
              parent(_parent), source(_source), lineNo(_lineNo), colNo(_colNo)
{ }

/* Asm Macro */
AsmMacro::AsmMacro(const AsmSourcePos& _pos, const Array<AsmMacroArg>& _args)
        : contentLineNo(0), sourcePos(_pos), args(_args)
{ }

AsmMacro::AsmMacro(const AsmSourcePos& _pos, Array<AsmMacroArg>&& _args)
        : contentLineNo(0), sourcePos(_pos), args(std::move(_args))
{ }

void AsmMacro::addLine(RefPtr<const AsmMacroSubst> macro, RefPtr<const AsmSource> source,
           const std::vector<LineTrans>& colTrans, size_t lineSize, const char* line)
{
    content.insert(content.end(), line, line+lineSize);
    // line can be empty and can be not finished by newline
    if (lineSize==0 || (lineSize > 0 && line[lineSize-1] != '\n'))
        content.push_back('\n');
    colTranslations.insert(colTranslations.end(), colTrans.begin(), colTrans.end());
    if (!macro)
    {
        if (sourceTranslations.empty() || sourceTranslations.back().source != source)
            sourceTranslations.push_back({contentLineNo, source});
    }
    else
    {
        // with macro
        bool doAdd = sourceTranslations.empty();
        /* add macro source information to line source translations if:
         * it is first line at source, if macro source differs against previous */
        if (!doAdd)
        {
            doAdd = sourceTranslations.back().source->type != AsmSourceType::MACRO;
            if (!doAdd)
            {
                RefPtr<const AsmMacroSource> macroSource = sourceTranslations.back()
                            .source.staticCast<const AsmMacroSource>();
                doAdd = macroSource->source!=source || macroSource->macro!=macro;
            }
        }
        if (doAdd)
            sourceTranslations.push_back({contentLineNo, RefPtr<const AsmSource>(
                new AsmMacroSource{macro, source})});
    }
    contentLineNo++;
}

/* Asm Repeat */
AsmRepeat::AsmRepeat(const AsmSourcePos& _pos, uint64_t _repeatsNum)
        : contentLineNo(0), sourcePos(_pos), repeatsNum(_repeatsNum)
{ }

AsmRepeat::~AsmRepeat()
{ }

// add line to repetition
void AsmRepeat::addLine(RefPtr<const AsmMacroSubst> macro, RefPtr<const AsmSource> source,
            const std::vector<LineTrans>& colTrans, size_t lineSize, const char* line)
{
    content.insert(content.end(), line, line+lineSize);
    // line can be empty and can be not finished by newline
    if (lineSize==0 || (lineSize > 0 && line[lineSize-1] != '\n'))
        content.push_back('\n');
    colTranslations.insert(colTranslations.end(), colTrans.begin(), colTrans.end());
    if (sourceTranslations.empty() || sourceTranslations.back().source != source ||
        sourceTranslations.back().macro != macro)
        sourceTranslations.push_back({contentLineNo, macro, source});
    contentLineNo++;
}

AsmFor::AsmFor(const AsmSourcePos& _pos, void* _iterSymEntry,
                AsmExpression* _condExpr, AsmExpression* _nextExpr)
        : AsmRepeat(_pos, 0), iterSymEntry(_iterSymEntry), condExpr(_condExpr),
                    nextExpr(_nextExpr)
{ }

AsmFor::~AsmFor()
{ }

AsmIRP::AsmIRP(const AsmSourcePos& _pos, const CString& _symbolName,
               const Array<CString>& _symValues)
        : AsmRepeat(_pos, _symValues.size()), irpc(false),
          symbolName(_symbolName), symValues(_symValues)
{ }

AsmIRP::AsmIRP(const AsmSourcePos& _pos, const CString& _symbolName,
               Array<CString>&& _symValues)
        : AsmRepeat(_pos, _symValues.size()), irpc(false), symbolName(_symbolName),
          symValues(std::move(_symValues))
{ }

AsmIRP::AsmIRP(const AsmSourcePos& _pos, const CString& _symbolName,
               const CString& _symValString)
        : AsmRepeat(_pos, std::max(_symValString.size(), size_t(1))), irpc(true),
          symbolName(_symbolName), symValues({_symValString})
{ }

/* AsmInputFilter */

AsmInputFilter::~AsmInputFilter()
{ }

LineCol AsmInputFilter::translatePos(size_t position) const
{
    // find in reverse order with reverse comparison:
    // find equal or less element (instead equal or greater)
    auto found = std::lower_bound(colTranslations.rbegin(), colTranslations.rend(),
         LineTrans({ ssize_t(position), 0 }),
         [](const LineTrans& t1, const LineTrans& t2)
         { return t1.position > t2.position; });
    return { found->lineNo, position-found->position+1 };
}

/*
 * AsmStreamInputFilter
 */

static const size_t AsmParserLineMaxSize = 200;

AsmStreamInputFilter::AsmStreamInputFilter(const CString& filename)
    : AsmInputFilter(AsmInputFilterType::STREAM), managed(true),
        stream(nullptr), mode(LineMode::NORMAL), stmtPos(0)
{
    try
    {
        source = RefPtr<const AsmSource>(new AsmFile(filename));
        stream = new std::ifstream(filename.c_str(), std::ios::binary);
        if (!*stream)
            throw AsmException(std::string("Can't open source file '")+
                    filename.c_str()+"'");
        stream->exceptions(std::ios::badbit);
        buffer.reserve(AsmParserLineMaxSize);
    }
    catch(...)
    {
        delete stream;
        throw;
    }
}

AsmStreamInputFilter::AsmStreamInputFilter(std::istream& is, const CString& filename)
    : AsmInputFilter(AsmInputFilterType::STREAM),
      managed(false), stream(&is), mode(LineMode::NORMAL), stmtPos(0)
{
    source = RefPtr<const AsmSource>(new AsmFile(filename));
    stream->exceptions(std::ios::badbit);
    buffer.reserve(AsmParserLineMaxSize);
}

AsmStreamInputFilter::AsmStreamInputFilter(const AsmSourcePos& pos,
           const CString& filename)
    : AsmInputFilter(AsmInputFilterType::STREAM),
      managed(true), stream(nullptr), mode(LineMode::NORMAL), stmtPos(0)
{
    try
    {
        if (!pos.macro)
            source = RefPtr<const AsmSource>(new AsmFile(pos.source, pos.lineNo,
                             pos.colNo, filename));
        else // if inside macro
            source = RefPtr<const AsmSource>(new AsmFile(
                RefPtr<const AsmSource>(new AsmMacroSource(pos.macro, pos.source)),
                     pos.lineNo, pos.colNo, filename));
        
        // open file
        stream = new std::ifstream(filename.c_str(), std::ios::binary);
        if (!*stream)
            throw AsmException(std::string("Can't open source file '")+
                        filename.c_str()+"'");
        stream->exceptions(std::ios::badbit);
        buffer.reserve(AsmParserLineMaxSize);
    }
    catch(...)
    {
        delete stream;
        throw;
    }
}

AsmStreamInputFilter::AsmStreamInputFilter(const AsmSourcePos& pos, std::istream& is,
        const CString& filename) : AsmInputFilter(AsmInputFilterType::STREAM),
        managed(false), stream(&is), mode(LineMode::NORMAL), stmtPos(0)
{
    if (!pos.macro)
        source = RefPtr<const AsmSource>(new AsmFile(pos.source, pos.lineNo,
                             pos.colNo, filename));
    else // if inside macro
        source = RefPtr<const AsmSource>(new AsmFile(
            RefPtr<const AsmSource>(new AsmMacroSource(pos.macro, pos.source)),
                 pos.lineNo, pos.colNo, filename));
    stream->exceptions(std::ios::badbit);
    buffer.reserve(AsmParserLineMaxSize);
}

AsmStreamInputFilter::~AsmStreamInputFilter()
{
    if (managed)
        delete stream;
}

const char* AsmStreamInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    colTranslations.clear();
    bool endOfLine = false;
    size_t lineStart = pos;
    size_t joinStart = pos; // join Start - physical line start
    size_t destPos = pos;
    size_t backslash = false;
    bool prevAsterisk = false;
    bool asterisk = false;
    colTranslations.push_back({ssize_t(-stmtPos), lineNo});
    while (!endOfLine)
    {
        switch(mode)
        {
            case LineMode::NORMAL:
            {
                if (pos < buffer.size() && !isSpace(buffer[pos]) && buffer[pos] != ';')
                {
                    // putting regular string (no spaces)
                    do {
                        backslash = (buffer[pos] == '\\');
                        if (buffer[pos] == '*' &&
                            destPos > 0 && buffer[destPos-1] == '/')
                        {
                            // if long comment
                            prevAsterisk = false;
                            asterisk = false;
                            buffer[destPos-1] = ' ';
                            buffer[destPos++] = ' ';
                            mode = LineMode::LONG_COMMENT;
                            pos++;
                            break;
                        }
                        if (buffer[pos] == '#')
                        {
                            // if line comment
                            buffer[destPos++] = ' ';
                            mode = LineMode::LINE_COMMENT;
                            pos++;
                            break;
                        }
                        
                        const char old = buffer[pos];
                        buffer[destPos++] = buffer[pos++];
                        
                        if (old == '"')
                        {
                            // if string opened
                            mode = LineMode::STRING;
                            break;
                        }
                        else if (old == '\'')
                        {
                            // if string opened
                            mode = LineMode::LSTRING;
                            break;
                        }
                        
                    } while (pos < buffer.size() && !isSpace(buffer[pos]) &&
                                buffer[pos] != ';');
                }
                if (pos < buffer.size() && mode!=LineMode::LINE_COMMENT)
                {
                    // ignore for single line comment - because empty single comment
                    // will be moved to next line!
                    if (buffer[pos] == '\n')
                    {
                        lineNo++;
                        endOfLine = (!backslash);
                        if (backslash) 
                        {
                            destPos--;
                            /* delete col translation at this position (if exists) and
                             * add new with new lineNo */
                            if (ssize_t(destPos-lineStart) ==
                                colTranslations.back().position)
                                colTranslations.pop_back();
                            colTranslations.push_back(
                                {ssize_t(destPos-lineStart), lineNo});
                        }
                        stmtPos = 0;
                        pos++;
                        joinStart = pos;
                        backslash = false;
                        break;
                    }
                    else if (buffer[pos] == ';' && mode == LineMode::NORMAL)
                    {
                        /* treat statement as separate line */
                        endOfLine = true;
                        pos++;
                        stmtPos += pos-joinStart;
                        joinStart = pos;
                        backslash = false;
                        break;
                    }
                    else if (mode == LineMode::NORMAL)
                    {
                        /* replace space character by 0x20 (space) */
                        backslash = false;
                        do {
                            buffer[destPos++] = ' ';
                            pos++;
                        } while (pos < buffer.size() && buffer[pos] != '\n' &&
                            isSpace(buffer[pos]));
                    }
                }
                break;
            }
            case LineMode::LINE_COMMENT:
            {
                while (pos < buffer.size() && buffer[pos] != '\n')
                {
                    // skipping bytes until newline or buffer end
                    backslash = (buffer[pos] == '\\');
                    pos++;
                    buffer[destPos++] = ' ';
                }
                if (pos < buffer.size())
                {
                    lineNo++;
                    endOfLine = (!backslash);
                    if (backslash)
                    {
                        // continue comment after line splitting
                        destPos--;
                        if (ssize_t(destPos-lineStart) == colTranslations.back().position)
                            colTranslations.pop_back();
                        colTranslations.push_back({ssize_t(destPos-lineStart), lineNo});
                    }
                    else
                        mode = LineMode::NORMAL;
                    pos++;
                    joinStart = pos;
                    backslash = false;
                    stmtPos = 0;
                }
                break;
            }
            case LineMode::LONG_COMMENT:
            {
                // go to end of long comment '*/' or to end of line
                while (pos < buffer.size() && buffer[pos] != '\n' &&
                    (!asterisk || buffer[pos] != '/'))
                {
                    backslash = (buffer[pos] == '\\');
                    prevAsterisk = asterisk;
                    asterisk = (buffer[pos] == '*');
                    pos++;
                    buffer[destPos++] = ' ';
                }
                if (pos < buffer.size())
                {
                    if ((asterisk && buffer[pos] == '/'))
                    {
                        // end of multi line comment, set normal mode
                        pos++;
                        buffer[destPos++] = ' ';
                        mode = LineMode::NORMAL;
                    }
                    else
                    {
                        // newline
                        lineNo++;
                        endOfLine = (!backslash);
                        if (backslash)
                        {
                            asterisk = prevAsterisk;
                            prevAsterisk = false;
                            destPos--;
                            /* delete col translation at this position (if exists) and
                             * add new with new lineNo */
                            if (ssize_t(destPos-lineStart) ==
                                colTranslations.back().position)
                                colTranslations.pop_back();
                            colTranslations.push_back(
                                {ssize_t(destPos-lineStart), lineNo});
                        }
                        pos++;
                        joinStart = pos;
                        backslash = false;
                        stmtPos = 0;
                    }
                }
                break;
            }
            case LineMode::STRING:
            case LineMode::LSTRING:
            {
                const char quoteChar = (mode == LineMode::STRING)?'"':'\'';
                // go to end of string '"' or "'" or to new line
                while (pos < buffer.size() && buffer[pos] != '\n' &&
                    ((backslash&1) || buffer[pos] != quoteChar))
                {
                    if (buffer[pos] == '\\')
                        backslash++;
                    else
                        backslash = 0;
                    buffer[destPos++] = buffer[pos];
                    pos++;
                }
                if (pos < buffer.size())
                {
                    if ((backslash&1)==0 && buffer[pos] == quoteChar)
                    {
                        // if qoutation character exists and it not escaped, we ends string
                        pos++;
                        mode = LineMode::NORMAL;
                        buffer[destPos++] = quoteChar;
                    }
                    else
                    {
                        lineNo++;
                        endOfLine = ((backslash&1)==0);
                        if (backslash&1)
                        {
                            destPos--; // ignore last backslash
                            colTranslations.push_back(
                                {ssize_t(destPos-lineStart), lineNo});
                        }
                        else
                            assembler.printWarning({lineNo, pos-joinStart+stmtPos+1},
                                        "Unterminated string: newline inserted");
                        pos++;
                        joinStart = pos;
                        stmtPos = 0;
                    }
                    backslash = false;
                }
                break;
            }
            default:
                break;
        }
        
        if (endOfLine)
            break;
        
        if (pos >= buffer.size())
        {
            /* get from buffer */
            if (lineStart != 0)
            {
                // use backward copying for moving buffer content back to begin
                std::copy_backward(buffer.begin()+lineStart, buffer.begin()+pos,
                       buffer.begin() + pos-lineStart);
                destPos -= lineStart;
                joinStart -= pos-destPos;
                pos = destPos;
                lineStart = 0;
            }
            if (pos == buffer.size())
                buffer.resize(std::max(AsmParserLineMaxSize, (pos>>1)+pos));
            
            stream->read(buffer.data()+pos, buffer.size()-pos);
            const size_t readed = stream->gcount();
            buffer.resize(pos+readed);
            if (readed == 0)
            {
                // end of file. check comments
                if (mode == LineMode::LONG_COMMENT && lineStart!=pos)
                    assembler.printError({lineNo, pos-joinStart+stmtPos+1},
                           "Unterminated multi-line comment");
                if (destPos-lineStart == 0)
                {
                    lineSize = 0;
                    return nullptr;
                }
                break;
            }
        }
    }
    lineSize = destPos-lineStart;
    return buffer.data()+lineStart;
}

AsmMacroInputFilter::AsmMacroInputFilter(RefPtr<const AsmMacro> _macro,
         const AsmSourcePos& pos, const MacroArgMap& _argMap, uint64_t _macroCount,
         bool _alternateMacro)
        : AsmInputFilter(AsmInputFilterType::MACROSUBST), macro(_macro),
          argMap(_argMap), macroCount(_macroCount), contentLineNo(0), sourceTransIndex(0),
          realLinePos(0), alternateMacro(_alternateMacro)
{
    if (macro->getSourceTransSize()!=0)
        source = macro->getSourceTrans(0).source;
    macroSubst = RefPtr<const AsmMacroSubst>(new AsmMacroSubst(pos.macro,
                   pos.source, pos.lineNo, pos.colNo));
    curColTrans = macro->getColTranslations().data();
    buffer.reserve(AsmParserLineMaxSize);
    lineNo = !macro->getColTranslations().empty() ? curColTrans[0].lineNo : 0;
    if (!macro->getColTranslations().empty())
        realLinePos = -curColTrans[0].position;
}

AsmMacroInputFilter::AsmMacroInputFilter(RefPtr<const AsmMacro> _macro,
         const AsmSourcePos& pos, MacroArgMap&& _argMap, uint64_t _macroCount,
         bool _alternateMacro)
        : AsmInputFilter(AsmInputFilterType::MACROSUBST), macro(_macro),
          argMap(std::move(_argMap)), macroCount(_macroCount),
          contentLineNo(0), sourceTransIndex(0), realLinePos(0),
          alternateMacro(_alternateMacro)
{
    if (macro->getSourceTransSize()!=0)
        source = macro->getSourceTrans(0).source;
    macroSubst = RefPtr<const AsmMacroSubst>(new AsmMacroSubst(pos.macro,
                   pos.source, pos.lineNo, pos.colNo));
    curColTrans = macro->getColTranslations().data();
    buffer.reserve(AsmParserLineMaxSize);
    lineNo = !macro->getColTranslations().empty() ? curColTrans[0].lineNo : 0;
    if (!macro->getColTranslations().empty())
        realLinePos = -curColTrans[0].position;
}

const char* AsmMacroInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    buffer.clear();
    colTranslations.clear();
    const std::vector<LineTrans>& macroColTrans = macro->getColTranslations();
    const LineTrans* colTransEnd = macroColTrans.data()+ macroColTrans.size();
    const size_t contentSize = macro->getContent().size();
    if (pos == contentSize)
    {
        lineSize = 0;
        return nullptr;
    }
    
    const char* content = macro->getContent().data();
    
    size_t nextLinePos = pos;
    while (nextLinePos < contentSize && content[nextLinePos] != '\n')
        nextLinePos++;
    
    const size_t linePos = pos;
    size_t destPos = 0;
    size_t toCopyPos = pos;
    // determine start of destination line (real line, with real newline)
    size_t destLineStart = 0;
    // first curColTrans
    colTranslations.push_back({ ssize_t(-realLinePos), curColTrans->lineNo});
    // colTransThreshold is position of next column translation (e.g line splitting)
    size_t colTransThreshold = (curColTrans+1 != colTransEnd) ?
            (curColTrans[1].position>0 ? curColTrans[1].position + linePos :
                    nextLinePos) : SIZE_MAX;
    
    const char* localStmtStart = nullptr;
    std::vector<std::pair<CString, const char*> > localNames;
    const char* stmtStartPtr = content + pos;
    /* parse local statement */
    if (alternateMacro)
    {
        // try to parse local statement
        const char* linePtr = stmtStartPtr;
        const char* end = content+nextLinePos;
        // before we skip labels at current statement
        skipSpacesAndLabels(linePtr, end);
        localStmtStart = linePtr;
        if (linePtr+6 < end && ::strncasecmp(linePtr, "local", 5)==0 && linePtr[5]==' ')
        {
            // if local
            linePtr+=5;
            while (true)
            {
                skipSpacesToEnd(linePtr, end);
                if (linePtr == end)
                    break; // end of list
                const char* symNamePlace = linePtr;
                const CString localName = extractSymName(linePtr, end, false);
                if (!localName.empty())
                {
                    localNames.push_back({localName, symNamePlace});
                    skipSpacesToEnd(linePtr, end);
                    if (linePtr!=end && *linePtr==',')
                        linePtr++; // skip ','
                }
                else // is not symbol
                    break;
            }
            
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end)
                localStmtStart = nullptr; // this is not local stmt
        }
        else
            localStmtStart = nullptr; // this is not local stmt
    }
    
    // indicate length of name to copy to buffer (skip)
    size_t wordSkip = 0;
    /* loop move position to backslash. if backslash encountered then copy content
     * to content and handles backsash with substitutions */
    while (pos < contentSize && content[pos] != '\n')
    {
        if (alternateMacro && localStmtStart!=nullptr && content+pos == localStmtStart)
        {
            /* if end of labels in 'local' statement',
             * we just replaces character by spaces */
            // put remaining text from source (to local stmt start position)
            buffer.resize(destPos + pos-toCopyPos);
            std::copy(content + toCopyPos, content + pos, buffer.begin() + destPos);
            destPos += pos-toCopyPos;
            toCopyPos = pos;
            // put col translations and spaces to buffer
            while (pos < contentSize && content[pos] != '\n')
            {
                if (pos >= colTransThreshold)
                {
                    // put column translation
                    curColTrans++;
                    colTranslations.push_back({ssize_t(destPos + pos-toCopyPos),
                                curColTrans->lineNo});
                    if (curColTrans->position >= 0)
                    {
                        /// real new line, reset real line position
                        realLinePos = 0;
                        destLineStart = destPos + pos-toCopyPos;
                    }
                    colTransThreshold = (curColTrans+1 != colTransEnd) ?
                            (curColTrans[1].position>0 ? curColTrans[1].position + linePos :
                                    nextLinePos) : SIZE_MAX;
                }
                pos++;
                buffer.push_back(' '); // put space instead of character to ignore later
            }
            toCopyPos = pos;
            break; // end of line of statement if local statement occurrent
        }
        
        bool tryParseSubstition = false;
        bool altMacroSyntax = false; // indicate if currently used altmacro syntax
        if (content[pos] != '\\')
        {
            if (pos >= colTransThreshold)
            {
                curColTrans++;
                colTranslations.push_back({ssize_t(destPos + pos-toCopyPos),
                            curColTrans->lineNo});
                if (curColTrans->position >= 0)
                {
                    /// real new line, reset real line position
                    realLinePos = 0;
                    destLineStart = destPos + pos-toCopyPos;
                }
                colTransThreshold = (curColTrans+1 != colTransEnd) ?
                        (curColTrans[1].position>0 ? curColTrans[1].position + linePos :
                                nextLinePos) : SIZE_MAX;
            }
            if (alternateMacro && wordSkip==0 && pos < contentSize &&
                (isAlpha(content[pos]) || content[pos]=='.' || content[pos]=='$' ||
                 content[pos]=='_'))
            {
                // try parse substitution (altmacro mode)
                tryParseSubstition = true;
                altMacroSyntax = true; // disables '@' and '()' use altmacro
            }
            else
            {
                /* otherwise consume one character */
                if (wordSkip!=0)
                    // after unmatched alternate substitution, we copy unmatched name
                    wordSkip--;
                pos++;
            }
        }
        else
        {
            // backslash
            if (pos >= colTransThreshold)
            {
                // put column translation
                curColTrans++;
                colTranslations.push_back({ssize_t(destPos + pos-toCopyPos),
                            curColTrans->lineNo});
                if (curColTrans->position >= 0)
                {
                    /// real new line, reset real line position
                    realLinePos = 0;
                    destLineStart = destPos + pos-toCopyPos;
                }
                colTransThreshold = (curColTrans+1 != colTransEnd) ?
                        (curColTrans[1].position>0 ? curColTrans[1].position + linePos :
                                nextLinePos) : SIZE_MAX;
            }
            // copy chars to buffer, fast copy regular content of macro (no substitution)
            if (pos > toCopyPos)
            {
                buffer.resize(destPos + pos-toCopyPos);
                std::copy(content + toCopyPos, content + pos, buffer.begin() + destPos);
                destPos += pos-toCopyPos;
            }
            pos++;
            tryParseSubstition = true;
        }
        if (tryParseSubstition)
        {
            bool skipColTransBetweenMacroArg = true;
            if (pos < contentSize)
            {
                if (!altMacroSyntax &&
                    content[pos] == '(' && pos+1 < contentSize && content[pos+1]==')')
                    pos += 2;   // skip this separator
                else
                {
                    // extract argName
                    const char* thisPos = content + pos;
                    const CString symName = extractSymName(
                                thisPos, content+contentSize, false);
                    auto it = argMap.end();
                    auto localIt = localMap.end();
                    if (!symName.empty()) // find only if not empty symName
                    {
                        it = binaryMapFind(argMap.begin(), argMap.end(), symName);
                        if (it == argMap.end())
                            localIt = localMap.find(symName);
                    }
                    if (altMacroSyntax && (it!=argMap.end() || localIt!=localMap.end()))
                    {
                        /* copy previous content, missing copy before alternate
                         * substitution */
                        buffer.resize(destPos + pos-toCopyPos);
                        std::copy(content + toCopyPos, content + pos,
                                  buffer.begin() + destPos);
                        destPos += pos-toCopyPos;
                    }
                    
                    if (it != argMap.end())
                    {
                        // if found
                        buffer.insert(buffer.end(), it->second.begin(),
                              it->second.begin() + it->second.size());
                        destPos += it->second.size();
                        pos = thisPos-content;
                    }
                    else if (localIt != localMap.end())
                    {
                        // substitute local definition
                        char buf[32];
                        ::memcpy(buf, ".LL", 3);
                        size_t bufSize = itocstrCStyle(localIt->second, buf+3, 29)+3;
                        buffer.insert(buffer.end(), buf, buf+bufSize);
                        destPos += bufSize;
                        pos = thisPos-content;
                    }
                    else if (!altMacroSyntax && content[pos] == '@')
                    {
                        char numBuf[32];
                        const size_t numLen = itocstrCStyle(macroCount, numBuf, 32);
                        pos++;
                        buffer.insert(buffer.end(), numBuf, numBuf+numLen);
                        destPos += numLen;
                    }
                    else
                    {
                        if (!altMacroSyntax)
                        {
                            buffer.push_back('\\');
                            destPos++;
                        }
                        else // continue consuming to copy
                        {
                            // and set name to copy to buffer (skip)
                            wordSkip = symName.size();
                            continue;
                        }
                        // do not skip column translation, because no substitution!
                        skipColTransBetweenMacroArg = false;
                    }
                }
            }
            toCopyPos = pos;
            // skip colTrans between macroarg or separator
            if (skipColTransBetweenMacroArg)
            {
                while (pos > colTransThreshold)
                {
                    curColTrans++;
                    if (curColTrans->position >= 0)
                    {
                        /// real new line, reset real line position
                        realLinePos = 0;
                        destLineStart = destPos + pos-toCopyPos;
                    }
                    colTransThreshold = (curColTrans+1 != colTransEnd) ?
                            curColTrans[1].position : SIZE_MAX;
                }
            }
        }
    }
    // last copying of content of line (rest of line)
    if (pos > toCopyPos)
    {
        buffer.resize(destPos + pos-toCopyPos);
        std::copy(content + toCopyPos, content + pos, buffer.begin() + destPos);
        destPos += pos-toCopyPos;
    }
    lineSize = buffer.size();
    /// if not end of content (just newline)
    if (pos < contentSize)
    {
        if (curColTrans+1 != colTransEnd)
        {
            curColTrans++;
            if (curColTrans->position >= 0) /// real new line, reset real line position
                realLinePos = 0;
            else    // otherwise determine position in destination source
                realLinePos += lineSize - destLineStart+1;
        }
        pos++; // skip newline
    }
    lineNo = curColTrans->lineNo;
    // move to next source translation
    if (sourceTransIndex+1 < macro->getSourceTransSize())
    {
        const AsmMacro::SourceTrans& fpos = macro->getSourceTrans(sourceTransIndex+1);
        if (fpos.lineNo == contentLineNo)
        {
            source = fpos.source;
            sourceTransIndex++;
        }
    }
    contentLineNo++;
    if (localStmtStart!=nullptr)
    {
        // if really is local statement, we add local defs to map
        for (const auto& elem: localNames)
            if (!addLocal(elem.first, assembler.localCount))
                // error report error if duplicate
                assembler.printError(getSourcePos(elem.second-stmtStartPtr),
                     (std::string("Name '")+elem.first.c_str()+
                     "' was already used by local or macro argument").c_str());
            else
                assembler.localCount++;
    }
    return (!buffer.empty()) ? buffer.data() : "";
}

bool AsmMacroInputFilter::addLocal(const CString& name, uint64_t localNo)
{
    if (binaryMapFind(argMap.begin(), argMap.end(), name) != argMap.end())
        return false; // if found in argument list
    return localMap.insert(std::make_pair(name, localNo)).second;
}

/*
 * AsmRepeatInputFilter
 */

AsmRepeatInputFilter::AsmRepeatInputFilter(const AsmRepeat* _repeat) :
          AsmInputFilter(AsmInputFilterType::REPEAT), repeat(_repeat),
          repeatCount(0), contentLineNo(0), sourceTransIndex(0)
{
    if (_repeat->getSourceTransSize()!=0)
    {
        source = RefPtr<const AsmSource>(new AsmRepeatSource(
                    _repeat->getSourceTrans(0).source, 0, _repeat->getRepeatsNum()));
        macroSubst = _repeat->getSourceTrans(0).macro;
    }
    else
        source = RefPtr<const AsmSource>(new AsmRepeatSource(
                    RefPtr<const AsmSource>(), 0, _repeat->getRepeatsNum()));
    curColTrans = _repeat->getColTranslations().data();
    lineNo = !_repeat->getColTranslations().empty() ? curColTrans[0].lineNo : 0;
}

const char* AsmRepeatInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    colTranslations.clear();
    const std::vector<LineTrans>& repeatColTrans = repeat->getColTranslations();
    const LineTrans* colTransEnd = repeatColTrans.data()+ repeatColTrans.size();
    const size_t contentSize = repeat->getContent().size();
    if (pos == contentSize)
    {
        repeatCount++;
        if (repeatCount == repeat->getRepeatsNum() || contentSize==0)
        {
            lineSize = 0;
            return nullptr;
        }
        sourceTransIndex = 0;
        curColTrans = repeat->getColTranslations().data();
        pos = 0;
        contentLineNo = 0;
        source = RefPtr<const AsmSource>(new AsmRepeatSource(
            repeat->getSourceTrans(0).source, repeatCount, repeat->getRepeatsNum()));
    }
    const char* content = repeat->getContent().data();
    size_t oldPos = pos;
    while (pos < contentSize && content[pos] != '\n')
        pos++;
    
    lineSize = pos - oldPos; // set new linesize
    if (pos < contentSize)
        pos++; // skip newline
    
    const LineTrans* oldCurColTrans = curColTrans;
    curColTrans++;
    while (curColTrans != colTransEnd && curColTrans->position > 0)
        curColTrans++;
    colTranslations.assign(oldCurColTrans, curColTrans);
    
    lineNo = (curColTrans != colTransEnd) ? curColTrans->lineNo : repeatColTrans[0].lineNo;
    if (sourceTransIndex+1 < repeat->getSourceTransSize())
    {
        const AsmRepeat::SourceTrans& fpos = repeat->getSourceTrans(sourceTransIndex+1);
        if (fpos.lineNo == contentLineNo)
        {
            macroSubst = fpos.macro;
            sourceTransIndex++;
            source = RefPtr<const AsmSource>(new AsmRepeatSource(
                fpos.source, repeatCount, repeat->getRepeatsNum()));
        }
    }
    contentLineNo++;
    return content + oldPos;
}

AsmForInputFilter::AsmForInputFilter(const AsmFor* forRpt) :
        AsmRepeatInputFilter(forRpt)
{ }

const char* AsmForInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    colTranslations.clear();
    const std::vector<LineTrans>& repeatColTrans = repeat->getColTranslations();
    const AsmFor* asmFor = static_cast<const AsmFor*>(repeat.get());
    const LineTrans* colTransEnd = repeatColTrans.data()+ repeatColTrans.size();
    const size_t contentSize = repeat->getContent().size();
    if (pos == contentSize)
    {
        repeatCount++;
        uint64_t value = 0;
        cxuint sectionId = ASMSECT_ABS;
        bool good = true;
        if (asmFor->getNextExpr() != nullptr)
        {
            // create next expression to evaluate
            std::unique_ptr<AsmExpression> nextEvExpr(asmFor->getNextExpr()->
                        createExprToEvaluate(assembler));
            uint64_t nextValue = 0;
            cxuint nextSectionId = ASMSECT_ABS;
            if (nextEvExpr != nullptr &&
                nextEvExpr->evaluate(assembler, nextValue, nextSectionId))
                // set symbol if evaluated without errors
                assembler.setSymbol(*(AsmSymbolEntry*)asmFor->getIterSymEntry(),
                                    nextValue, nextSectionId);
            else
                good = false;
        }
        if (good)
        {
            // create conditional expression to evaluate
            std::unique_ptr<AsmExpression> condEvExpr(asmFor->getCondExpr()->
                        createExprToEvaluate(assembler));
            if (condEvExpr==nullptr || !condEvExpr->evaluate(assembler, value, sectionId))
                // zeroing condition if evaluation failed
                value = 0;
            else if (sectionId != ASMSECT_ABS)
            {
                // failed if no absolute value
                assembler.printError(asmFor->getCondExpr()->getSourcePos(),
                        "Value of conditional expression is not absolute");
                value = 0;
                good = false;
            }
        }
        
        if (!good || value==0 || contentSize==0)
        {
            lineSize = 0;
            return nullptr;
        }
        
        sourceTransIndex = 0;
        curColTrans = repeat->getColTranslations().data();
        pos = 0;
        contentLineNo = 0;
        source = RefPtr<const AsmSource>(new AsmRepeatSource(
            repeat->getSourceTrans(0).source, repeatCount, repeat->getRepeatsNum()));
    }
    const char* content = repeat->getContent().data();
    size_t oldPos = pos;
    while (pos < contentSize && content[pos] != '\n')
        pos++;
    
    lineSize = pos - oldPos; // set new linesize
    if (pos < contentSize)
        pos++; // skip newline
    
    const LineTrans* oldCurColTrans = curColTrans;
    curColTrans++;
    while (curColTrans != colTransEnd && curColTrans->position > 0)
        curColTrans++;
    colTranslations.assign(oldCurColTrans, curColTrans);
    
    lineNo = (curColTrans != colTransEnd) ? curColTrans->lineNo : repeatColTrans[0].lineNo;
    if (sourceTransIndex+1 < repeat->getSourceTransSize())
    {
        const AsmRepeat::SourceTrans& fpos = repeat->getSourceTrans(sourceTransIndex+1);
        if (fpos.lineNo == contentLineNo)
        {
            macroSubst = fpos.macro;
            sourceTransIndex++;
            source = RefPtr<const AsmSource>(new AsmRepeatSource(
                fpos.source, repeatCount, repeat->getRepeatsNum()));
        }
    }
    contentLineNo++;
    return content + oldPos;
}

AsmIRPInputFilter::AsmIRPInputFilter(const AsmIRP* _irp) :
        AsmInputFilter(AsmInputFilterType::REPEAT), irp(_irp),
        repeatCount(0), contentLineNo(0), sourceTransIndex(0), realLinePos(0)
{
    if (_irp->getSourceTransSize()!=0)
    {
        source = RefPtr<const AsmSource>(new AsmRepeatSource(
                    _irp->getSourceTrans(0).source, 0, _irp->getRepeatsNum()));
        macroSubst = _irp->getSourceTrans(0).macro;
    }
    else
        source = RefPtr<const AsmSource>(new AsmRepeatSource(
                    RefPtr<const AsmSource>(), 0, _irp->getRepeatsNum()));
    curColTrans = _irp->getColTranslations().data();
    lineNo = !_irp->getColTranslations().empty() ? curColTrans[0].lineNo : 0;
    
    if (!_irp->getColTranslations().empty())
        realLinePos = -curColTrans[0].position;
    buffer.reserve(AsmParserLineMaxSize);
}

const char* AsmIRPInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    buffer.clear();
    colTranslations.clear();
    const std::vector<LineTrans>& macroColTrans = irp->getColTranslations();
    const LineTrans* colTransEnd = macroColTrans.data()+ macroColTrans.size();
    const size_t contentSize = irp->getContent().size();
    if (pos == contentSize)
    {
        repeatCount++;
        if (repeatCount == irp->getRepeatsNum() || contentSize==0)
        {
            lineSize = 0;
            return nullptr;
        }
        sourceTransIndex = 0;
        curColTrans = irp->getColTranslations().data();
        realLinePos = -curColTrans[0].position;
        pos = contentLineNo = 0;
        source = RefPtr<const AsmSource>(new AsmRepeatSource(
            irp->getSourceTrans(0).source, repeatCount, irp->getRepeatsNum()));
    }
    
    const CString& expectedSymName = irp->getSymbolName();
    const CString& symValue = !irp->isIRPC() ? irp->getSymbolValue(repeatCount) :
            irp->getSymbolValue(0);
    size_t symValueSize = symValue.size();
    const char* content = irp->getContent().data();
    
    size_t nextLinePos = pos;
    while (nextLinePos < contentSize && content[nextLinePos] != '\n')
        nextLinePos++;
    
    const size_t linePos = pos;
    size_t destPos = 0;
    size_t toCopyPos = pos;
    // determine start of destination line (real line, with real newline)
    size_t destLineStart = 0;
    colTranslations.push_back({ ssize_t(-realLinePos), curColTrans->lineNo});
    // colTransThreshold is position of next column translation (e.g line splitting)
    size_t colTransThreshold = (curColTrans+1 != colTransEnd) ?
            (curColTrans[1].position>0 ? curColTrans[1].position + linePos :
                    nextLinePos) : SIZE_MAX;
    
    /* loop move position to backslash. if backslash encountered then copy content
     * to content and handles backsash with substitutions */
    while (pos < contentSize && content[pos] != '\n')
    {
        if (content[pos] != '\\')
        {
            if (pos >= colTransThreshold)
            {
                // put column translation
                curColTrans++;
                colTranslations.push_back({ssize_t(destPos + pos-toCopyPos),
                            curColTrans->lineNo});
                if (curColTrans->position >= 0)
                {
                    /// real new line, reset real line position
                    realLinePos = 0;
                    destLineStart = destPos + pos-toCopyPos;
                }
                colTransThreshold = (curColTrans+1 != colTransEnd) ?
                        (curColTrans[1].position>0 ? curColTrans[1].position + linePos :
                                nextLinePos) : SIZE_MAX;
            }
            pos++;
        }
        else
        {
            // backslash
            if (pos >= colTransThreshold)
            {
                // put column translation
                curColTrans++;
                colTranslations.push_back({ssize_t(destPos + pos-toCopyPos),
                            curColTrans->lineNo});
                if (curColTrans->position >= 0)
                {
                    /// real new line, reset real line position
                    realLinePos = 0;
                    destLineStart = destPos + pos-toCopyPos;
                }
                colTransThreshold = (curColTrans+1 != colTransEnd) ?
                        (curColTrans[1].position>0 ? curColTrans[1].position + linePos :
                                nextLinePos) : SIZE_MAX;
            }
            // copy chars to buffer
            if (pos > toCopyPos)
            {
                buffer.resize(destPos + pos-toCopyPos);
                std::copy(content + toCopyPos, content + pos, buffer.begin() + destPos);
                destPos += pos-toCopyPos;
            }
            pos++;
            bool skipColTransBetweenMacroArg = true;
            if (pos < contentSize)
            {
                if (content[pos] == '(' && pos+1 < contentSize && content[pos+1]==')')
                    pos += 2;   // skip this separator
                else
                {
                    // extract argName
                    const char* thisPos = content+pos;
                    const CString symName = extractSymName(
                                thisPos, content+contentSize, false);
                    if (expectedSymName == symName)
                    {
                        // if found
                        if (!irp->isIRPC())
                        {
                            buffer.insert(buffer.end(), symValue.begin(),
                                  symValue.begin() + symValueSize);
                            destPos += symValueSize;
                        }
                        else if (!symValue.empty())
                        {
                            buffer.push_back(symValue[repeatCount]);
                            destPos++;
                        }
                        pos = thisPos-content;
                    }
                    else
                    {
                        buffer.push_back('\\');
                        destPos++;
                        // do not skip column translation, because no substitution!
                        skipColTransBetweenMacroArg = false;
                    }
                }
            }
            toCopyPos = pos;
            // skip colTrans between macroarg or separator
            if (skipColTransBetweenMacroArg)
            {
                while (pos > colTransThreshold)
                {
                    curColTrans++;
                    if (curColTrans->position >= 0)
                    {
                        /// real new line, reset real line position
                        realLinePos = 0;
                        destLineStart = destPos + pos-toCopyPos;
                    }
                    colTransThreshold = (curColTrans+1 != colTransEnd) ?
                            curColTrans[1].position : SIZE_MAX;
                }
            }
        }
    }
    if (pos > toCopyPos)
    {
        buffer.resize(destPos + pos-toCopyPos);
        std::copy(content + toCopyPos, content + pos, buffer.begin() + destPos);
        destPos += pos-toCopyPos;
    }
    lineSize = buffer.size();
    // not content end (just newline at end of line)
    if (pos < contentSize)
    {
        if (curColTrans != colTransEnd)
        {
            curColTrans++;
            if (curColTrans != colTransEnd)
            {
                if (curColTrans->position >= 0)
                    /// real new line, reset real line position
                    realLinePos = 0;
                else // otherwise determine position in destination source
                    realLinePos += lineSize - destLineStart+1;
            }
        }
        pos++; // skip newline
    }
    lineNo = (curColTrans != colTransEnd) ? curColTrans->lineNo : macroColTrans[0].lineNo;
    // move to next source translation
    if (sourceTransIndex+1 < irp->getSourceTransSize())
    {
        const AsmRepeat::SourceTrans& fpos = irp->getSourceTrans(sourceTransIndex+1);
        if (fpos.lineNo == contentLineNo)
        {
            macroSubst = fpos.macro;
            sourceTransIndex++;
            source = RefPtr<const AsmSource>(new AsmRepeatSource(
                fpos.source, repeatCount, irp->getRepeatsNum()));
        }
    }
    contentLineNo++;
    return (!buffer.empty()) ? buffer.data() : "";
}

/*
 * source pos
 */

static void printIndent(std::ostream& os, cxuint indentLevel)
{
    for (; indentLevel != 0; indentLevel--)
        os.write("    ", 4);
}

static RefPtr<const AsmSource> printAsmRepeats(std::ostream& os,
           RefPtr<const AsmSource> source, cxuint indentLevel)
{
    bool firstDepth = true;
    while (source->type == AsmSourceType::REPT)
    {
        auto sourceRept = source.staticCast<const AsmRepeatSource>();
        printIndent(os, indentLevel);
        os.write((firstDepth)?"In repetition ":"              ", 14);
        // print: REPEATSCOUNT/REPEATSNUM:
        char numBuf[64];
        size_t size = itocstrCStyle(sourceRept->repeatCount+1, numBuf, 32);
        numBuf[size++] = '/';
        if (sourceRept->repeatsNum!=0)
            size += itocstrCStyle(sourceRept->repeatsNum, numBuf+size, 32-size);
        else // print '?'. we don't know where is end
            numBuf[size++] = '?';
        numBuf[size++] = ':';
        numBuf[size++] = '\n';
        os.write(numBuf, size);
        source = sourceRept->source;
        firstDepth = false;
    }
    return source;
}

void AsmSourcePos::print(std::ostream& os, cxuint indentLevel) const
{
    if ((!macro && !source) || colNo==0 || lineNo==0)
        return; // do not print asm source
    if (indentLevel == 10)
    {
        printIndent(os, indentLevel);
        os.write("Can't print all tree trace due to too big depth level\n", 53);
        return;
    }
    const AsmSourcePos* thisPos = this;
    bool exprFirstDepth = true;
    char numBuf[32];
    // print expression source positions
    while (thisPos->exprSourcePos!=nullptr)
    {
        AsmSourcePos sourcePosToPrint = *(thisPos->exprSourcePos);
        sourcePosToPrint.exprSourcePos = nullptr;
        printIndent(os, indentLevel);
        if (sourcePosToPrint.source->type == AsmSourceType::FILE)
        {
            RefPtr<const AsmFile> file = sourcePosToPrint.source.
                        staticCast<const AsmFile>();
            if (!file->parent)
            {
                os.write((exprFirstDepth) ? "Expression evaluation from " :
                        "                      from ", 27);
                os.write(file->file.c_str(), file->file.size());
                // print: :LINENO:COLNO: (COLNO is optional)
                numBuf[0] = ':';
                size_t size = 1+itocstrCStyle(sourcePosToPrint.lineNo, numBuf+1, 31);
                os.write(numBuf, size);
                if (colNo != 0)
                {
                    numBuf[0] = ':';
                    size = 1+itocstrCStyle(sourcePosToPrint.colNo, numBuf+1, 29);
                    numBuf[size++] = ':';
                    os.write(numBuf, size);
                }
                os.put('\n');
                exprFirstDepth = false;
                thisPos = thisPos->exprSourcePos;
                continue;
            }
        }
        exprFirstDepth = true;
        os.write("Expression evaluation from\n", 27);
        sourcePosToPrint.print(os, indentLevel+1);
        os.put('\n');
        thisPos = thisPos->exprSourcePos;
    }
    /* print macro tree */
    RefPtr<const AsmMacroSubst> curMacro = macro;
    RefPtr<const AsmMacroSubst> parentMacro;
    bool firstDepth = true;
    while(curMacro)
    {
        parentMacro = curMacro->parent;
        
        if (curMacro->source->type != AsmSourceType::MACRO)
        {
            /* if file */
            RefPtr<const AsmFile> curFile = curMacro->source.staticCast<const AsmFile>();
            if (curMacro->source->type == AsmSourceType::REPT || curFile->parent)
            {
                if (firstDepth)
                {
                    printIndent(os, indentLevel);
                    os.write("In macro substituted from\n", 26);
                }
                AsmSourcePos nextLevelPos = { RefPtr<const AsmMacroSubst>(),
                    curMacro->source, curMacro->lineNo, curMacro->colNo };
                nextLevelPos.print(os, indentLevel+1);
                os.write((parentMacro) ? ";\n" : ":\n", 2);
                firstDepth = true;
            }
            else
            {
                printIndent(os, indentLevel);
                os.write((firstDepth) ? "In macro substituted from " :
                        "                     from ", 26);
                // leaf
                curFile = curMacro->source.staticCast<const AsmFile>();
                if (!curFile->file.empty())
                    os.write(curFile->file.c_str(), curFile->file.size());
                else // stdin
                    os.write("<stdin>", 7);
                // print: :LINENO:COLNO or :LINENO:COLNO;
                numBuf[0] = ':';
                size_t size = 1+itocstrCStyle(curMacro->lineNo, numBuf+1, 29);
                numBuf[size++] = ':';
                os.write(numBuf, size);
                size = itocstrCStyle(curMacro->colNo, numBuf, 29);
                numBuf[size++] = (parentMacro) ? ';' : ':';
                numBuf[size++] = '\n';
                os.write(numBuf, size);
                firstDepth = false;
            }
        }
        else
        {
            // if macro
            printIndent(os, indentLevel);
            os.write("In macro substituted from macro content:\n", 41);
            RefPtr<const AsmMacroSource> curMacroPos =
                    curMacro->source.staticCast<const AsmMacroSource>();
            AsmSourcePos macroPos = { curMacroPos->macro, curMacroPos->source,
                curMacro->lineNo, curMacro->colNo };
            macroPos.print(os, indentLevel+1);
            os.write((parentMacro) ? ";\n" : ":\n", 2);
            firstDepth = true;
        }
        
        curMacro = parentMacro;
    }
    /* print source tree */
    RefPtr<const AsmSource> curSource = source;
    while (curSource->type == AsmSourceType::REPT)
        curSource = curSource.staticCast<const AsmRepeatSource>()->source;
    
    if (curSource->type != AsmSourceType::MACRO)
    {
        // if file
        RefPtr<const AsmFile> curFile = curSource.staticCast<const AsmFile>();
        if (curFile->parent)
        {
            RefPtr<const AsmSource> parentSource;
            bool firstDepth = true;
            while (curFile->parent)
            {
                parentSource = curFile->parent;
                
                parentSource = printAsmRepeats(os, parentSource, indentLevel);
                if (!firstDepth)
                    firstDepth = (curFile->parent != parentSource); // if repeats
                
                printIndent(os, indentLevel);
                if (parentSource->type != AsmSourceType::MACRO)
                {
                    RefPtr<const AsmFile> parentFile =
                            parentSource.staticCast<const AsmFile>();
                    os.write(firstDepth ? "In file included from " :
                            "                 from ", 22);
                    if (!parentFile->file.empty())
                        os.write(parentFile->file.c_str(), parentFile->file.size());
                    else // stdin
                        os.write("<stdin>", 7);
                    
                    // print: :LINENO:COLNO: or :LINENO:COLNO,
                    numBuf[0] = ':';
                    size_t size = 1+itocstrCStyle(curFile->lineNo, numBuf+1, 29);
                    numBuf[size++] = ':';
                    os.write(numBuf, size);
                    size = itocstrCStyle(curFile->colNo, numBuf, 30);
                    curFile = parentFile.staticCast<const AsmFile>();
                    numBuf[size++] = curFile->parent ? ',' : ':';
                    numBuf[size++] = '\n';
                    os.write(numBuf, size);
                    firstDepth = false;
                }
                else
                {
                    /* if macro */
                    os.write("In file included from macro content:\n", 37);
                    RefPtr<const AsmMacroSource> curMacroPos =
                            parentSource.staticCast<const AsmMacroSource>();
                    AsmSourcePos macroPos = { curMacroPos->macro, curMacroPos->source,
                        curFile->lineNo, curFile->colNo };
                    macroPos.print(os, indentLevel+1);
                    os.write(":\n", 2);
                    break;
                }
            }
        }
        // leaf
        printAsmRepeats(os, source, indentLevel);
        printIndent(os, indentLevel);
        if (!curSource.staticCast<const AsmFile>()->file.empty())
            os.write(curSource.staticCast<const AsmFile>()->file.c_str(),
                     curSource.staticCast<const AsmFile>()->file.size());
        else // stdin
            os.write("<stdin>", 7);
        numBuf[0] = ':';
        size_t size = 1+itocstrCStyle(lineNo, numBuf+1, 31);
        os.write(numBuf, size);
        if (colNo != 0)
        {
            numBuf[0] = ':';
            size = 1+itocstrCStyle(colNo, numBuf+1, 31);
            os.write(numBuf, size);
        }
    }
    else
    {
        // if macro
        printAsmRepeats(os, source, indentLevel);
        printIndent(os, indentLevel);
        os.write("In macro content:\n", 18);
        RefPtr<const AsmMacroSource> curMacroPos =
                curSource.staticCast<const AsmMacroSource>();
        AsmSourcePos macroPos = { curMacroPos->macro, curMacroPos->source, lineNo, colNo };
        macroPos.print(os, indentLevel+1);
    }
}

/*
 * AsmSourcePosHandler for sections
 */

/* STtrans byte format:
 * 0x00 - 0x3f - byte offset change
 * 0x40 - 0x7f - line position change ( - 0x40)
 * 0x80 - 0xbf - column position change ( - 0x40)
 * 0xc0 - 0xf7 - line and byte offset change
 *   0-2bit - offset change + 1
 *   3-5bit - linePos change + 1
 * 0xff - change macro substitition
 * 0xfe - change source
 * 0xfd - change source and macro substitution
 * 0xfc - no offset change
 * 0xfb - encode longer lineNo diff
 * 0xfa - encode longer colNo diff
 * 0xf9 - encode longer offset diff
 */

AsmSourcePosHandler::AsmSourcePosHandler() : sourcesPos(0), macroSubstsPos(0),
        stTransPos(0), oldLineNo(0), oldColNo(0), oldOffset(0)
{ }

template<typename T>
static inline void pushDiff(std::vector<cxbyte>& out, T diff, cxbyte smallCode,
            cxbyte longCode)
{
    if (diff <= 64)
        out.push_back(cxbyte(diff-1) | smallCode);
    else
    {
        out.push_back(longCode);
        // push colNo value
        const T value = diff-65;
        for (cxuint bit = 0; bit < sizeof(T)*8 && (value >> bit) != 0; bit += 7)
        {
            if (bit != 0)
                out.back() |= 0x80;
            out.push_back((value >> bit) & 0x7f);
        }
        if (value == 0)
            out.push_back(0); // if zero
    }
}

template<typename T>
static inline T getDiff(size_t& pos, const std::vector<cxbyte>& in, cxbyte smallCode,
            cxbyte longCode)
{
    if ((in[pos] & 0xc0) == smallCode)
        return T(in[pos++] & 0x3f) + 1;
    else if (in[pos] == longCode)
    {
        pos++;
        // read columnNo value
        T value = 0;
        cxuint bit = 0;
        do {
            value |= size_t(in[pos++] & 0x7f) << bit;
            bit += 7;
        } while ((in[pos-1] & 0x80) != 0);
        return value + 65;
    }
    return 0;
}

void AsmSourcePosHandler::pushSourcePos(size_t offset, const AsmSourcePos& sourcePos)
{
    bool doSetPos = false;
    const bool thisSameSource = !sources.empty() && sources.back() == sourcePos.source &&
            (!sourcePos.source || sources.back()->uniqueId  == sourcePos.source->uniqueId);
    const bool thisSameMacro = !macroSubsts.empty() &&
            macroSubsts.back() == sourcePos.macro && (!sourcePos.macro ||
             macroSubsts.back()->uniqueId  == sourcePos.macro->uniqueId);
    
    const size_t diffOffset = offset - oldOffset;
    
    if (!thisSameMacro && !thisSameSource)
    {
        // change macro and source
        macroSubsts.push_back(sourcePos.macro);
        sources.push_back(sourcePos.source);
        stTrans.push_back(0xfd);
        doSetPos = true;
    }
    else if (!thisSameMacro)
    {
        // change subst
        macroSubsts.push_back(sourcePos.macro);
        stTrans.push_back(0xff);
        doSetPos = true;
    }
    else if (!thisSameSource ||
            // fix for 64 change in offset and no change in colNo and lineNo
            (!stTrans.empty() && stTrans.back()==0x3f && sourcePos.lineNo == oldLineNo &&
             sourcePos.colNo == oldColNo && (diffOffset & 63) == 0))
    {
        // change source
        sources.push_back(sourcePos.source);
        stTrans.push_back(0xfe);
        doSetPos = true;
    }
    
    // change line and column
    bool noDiffOffset = false;
    if (!doSetPos)
    {
        const LineNo diffLineNo = sourcePos.lineNo - oldLineNo;
        ColNo diffColNo = sourcePos.colNo - oldColNo;
        if (diffColNo == 0 && diffLineNo!=0 && diffOffset!=0 &&
            diffLineNo <= 7 && diffOffset <= 8)
        {
            stTrans.push_back((cxbyte((diffLineNo-1)<<3) | cxbyte(diffOffset-1) | 0xc0));
            noDiffOffset = true;
        }
        else
        {
            // put lineNo and colNo differences
            if (diffLineNo != 0)
            {
                pushDiff(stTrans, diffLineNo, 0x40, 0xfb);
                diffColNo = sourcePos.colNo - 1;
            }
            if (diffColNo != 0)
                pushDiff(stTrans, diffColNo, 0x80, 0xfa);
        }
    }
    else
    {
        // push lineNo value
        stTrans.push_back(sourcePos.lineNo&0xff);
        stTrans.push_back((sourcePos.lineNo>>8) & 0x7f);
        if (sourcePos.lineNo > 0x7fffU)
        {
            stTrans.back() |= 0x80;
            stTrans.push_back((sourcePos.lineNo>>15) & 0xff);
            stTrans.push_back((sourcePos.lineNo>>23) & 0x7f);
            if (sourcePos.lineNo > 0x3fffffffU)
            {
                stTrans.back() |= 0x80;
                stTrans.push_back((sourcePos.lineNo>>30) & 0xff);
                stTrans.push_back((sourcePos.lineNo>>38) & 0x7f);
                if (sourcePos.lineNo > 0x1fffffffffffULL)
                {
                    stTrans.back() |= 0x80;
                    stTrans.push_back((sourcePos.lineNo>>45) & 0xff);
                    stTrans.push_back((sourcePos.lineNo>>53) & 0xff);
                }
            }
        }
        // push colNo value
        for (cxuint bit = 0; bit < sizeof(size_t)*8 &&
                    (sourcePos.colNo >> bit) != 0; bit += 7)
        {
            if (bit != 0)
                stTrans.back() |= 0x80;
            stTrans.push_back((sourcePos.colNo >> bit) & 0x7f);
        }
        if (sourcePos.colNo == 0)
            stTrans.push_back(0);
    }
    // push offset difference
    if (!noDiffOffset)
    {
        if (diffOffset != 0)
            pushDiff(stTrans, diffOffset, 0x0, 0xf9);
        else
            stTrans.push_back(0xfc); // no offset change
    }
    oldLineNo = sourcePos.lineNo;
    oldColNo = sourcePos.colNo;
    oldOffset = offset;
}

void AsmSourcePosHandler::rewind()
{
    sourcesPos = macroSubstsPos = stTransPos = 0;
    oldOffset = 0;
    oldLineNo = 0;
    oldColNo = 0;
}

std::pair<size_t, AsmSourcePos> AsmSourcePosHandler::nextSourcePos()
{
    if (stTransPos >= stTrans.size())
        throw Exception("No source pos available");
    const cxbyte code = stTrans[stTransPos];
    
    AsmSourcePos sourcePos{ };
    if (macroSubstsPos != 0)
        sourcePos.macro = macroSubsts[macroSubstsPos-1];
    if (sourcesPos != 0)
        sourcePos.source = sources[sourcesPos-1];
    
    bool doReadPos = false;
    if (code == 0xfd) // source and macro
    {
        sourcePos.macro = macroSubsts[macroSubstsPos++];
        sourcePos.source = sources[sourcesPos++];
        doReadPos = true;
        stTransPos++;
    }
    else if (code == 0xfe)
    {
        sourcePos.source = sources[sourcesPos++];
        doReadPos = true;
        stTransPos++;
    }
    else if (code == 0xff)
    {
        sourcePos.macro = macroSubsts[macroSubstsPos++];
        doReadPos = true;
        stTransPos++;
    }
    
    LineNo lineNo = oldLineNo;
    bool offsetAlreadyChanged = false;
    if (!doReadPos)
    {
        if ((code & 0xc0) == 0xc0 && (code < 0xf8))
        {
            lineNo += ((code&0x3f)>>3) + 1;
            oldOffset += (code&7) + 1;
            offsetAlreadyChanged = true;
            stTransPos++;
        }
        else
        {
            // apply differences for lineNo
            lineNo += getDiff<LineNo>(stTransPos, stTrans, 0x40, 0xfb);
            
            if (lineNo != oldLineNo) // line has changed
                oldColNo = 1; // we assume that column no is 1
            
            // apply differencees for colNo
            oldColNo += getDiff<ColNo>(stTransPos, stTrans, 0x80, 0xfa);
        }
        oldLineNo = lineNo;
    }
    else
    {
        // read lineNo value
        oldLineNo = stTrans[stTransPos++];
        oldLineNo |= uint64_t(stTrans[stTransPos++] & 0x7f) << 8;
        if ((stTrans[stTransPos-1] & 0x80) != 0)
        {
            oldLineNo |= uint64_t(stTrans[stTransPos++]) << 15;
            oldLineNo |= uint64_t(stTrans[stTransPos++] & 0x7f) << 23;
            if ((stTrans[stTransPos-1] & 0x80) != 0)
            {
                oldLineNo |= uint64_t(stTrans[stTransPos++]) << 30;
                oldLineNo |= uint64_t(stTrans[stTransPos++] & 0x7f) << 38;
                if ((stTrans[stTransPos-1] & 0x80) != 0)
                {
                    oldLineNo |= uint64_t(stTrans[stTransPos++]) << 45;
                    oldLineNo |= uint64_t(stTrans[stTransPos++]) << 53;
                }
            }
        }
        // read columnNo value
        oldColNo = 0;
        cxuint bit = 0;
        do {
            oldColNo |= size_t(stTrans[stTransPos++] & 0x7f) << bit;
            bit += 7;
        } while ((stTrans[stTransPos-1] & 0x80) != 0);
    }
    
    if (!offsetAlreadyChanged)
    {
        // if offset is not already changed
        if (stTransPos > stTrans.size() || stTrans[stTransPos] != 0xfc)
            oldOffset += getDiff<size_t>(stTransPos, stTrans, 0x0, 0xf9);
        else if (stTrans[stTransPos] == 0xfc)
            stTransPos++; // skip 0xfc
    }
    
    sourcePos.lineNo = oldLineNo;
    sourcePos.colNo = oldColNo;
    return std::make_pair(oldOffset, sourcePos);
}
