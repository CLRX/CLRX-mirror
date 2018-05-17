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
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"
#include "AsmAmdInternals.h"
#include "AsmAmdCL2Internals.h"
#include "AsmGalliumInternals.h"
#include "AsmROCmInternals.h"

using namespace CLRX;

// pseudo-ops used while skipping clauses
static const char* offlinePseudoOpNamesTbl[] =
{
    "else", "elseif", "elseif32", "elseif64", "elseifarch",
    "elseifb", "elseifc", "elseifdef",
    "elseifeq", "elseifeqs", "elseiffmt",
    "elseifge", "elseifgpu", "elseifgt",
    "elseifle", "elseiflt", "elseifnarch", "elseifnb", "elseifnc",
    "elseifndef", "elseifne", "elseifnes",
    "elseifnfmt", "elseifngpu", "elseifnotdef",
    "endif", "endm", "endmacro", "endr", "endrept", "for",
    "if", "if32", "if64", "ifarch", "ifb", "ifc", "ifdef", "ifeq",
    "ifeqs", "iffmt", "ifge", "ifgpu", "ifgt", "ifle",
    "iflt", "ifnarch", "ifnb", "ifnc", "ifndef",
    "ifne", "ifnes", "ifnfmt", "ifngpu", "ifnotdef",
    "irp", "irpc", "macro", "rept", "while"
};

/// pseudo-ops not ignored while putting macro content
static const char* macroRepeatPseudoOpNamesTbl[] =
{
    "endm", "endmacro", "endr", "endrept", "for", "irp", "irpc", "macro", "rept", "while"
};

// pseudo-ops used while skipping clauses
enum
{
    ASMCOP_ELSE = 0, ASMCOP_ELSEIF, ASMCOP_ELSEIF32, ASMCOP_ELSEIF64, ASMCOP_ELSEIFARCH,
    ASMCOP_ELSEIFB, ASMCOP_ELSEIFC, ASMCOP_ELSEIFDEF,
    ASMCOP_ELSEIFEQ, ASMCOP_ELSEIFEQS, ASMCOP_ELSEIFFMT,
    ASMCOP_ELSEIFGE, ASMCOP_ELSEIFGPU, ASMCOP_ELSEIFGT,
    ASMCOP_ELSEIFLE, ASMCOP_ELSEIFLT, ASMCOP_ELSEIFNARCH, ASMCOP_ELSEIFNB, ASMCOP_ELSEIFNC,
    ASMCOP_ELSEIFNDEF, ASMCOP_ELSEIFNE, ASMCOP_ELSEIFNES,
    ASMCOP_ELSEIFNFMT, ASMCOP_ELSEIFNGPU, ASMCOP_ELSEIFNOTDEF,
    ASMCOP_ENDIF, ASMCOP_ENDM, ASMCOP_ENDMACRO, ASMCOP_ENDR, ASMCOP_ENDREPT,
    ASMCOP_FOR, ASMCOP_IF, ASMCOP_IF32, ASMCOP_IF64, ASMCOP_IFARCH, ASMCOP_IFB,
    ASMCOP_IFC, ASMCOP_IFDEF, ASMCOP_IFEQ,
    ASMCOP_IFEQS, ASMCOP_IFFMT, ASMCOP_IFGE, ASMCOP_IFGPU, ASMCOP_IFGT, ASMCOP_IFLE,
    ASMCOP_IFLT, ASMCOP_IFNARCH, ASMCOP_IFNB, ASMCOP_IFNC, ASMCOP_IFNDEF,
    ASMCOP_IFNE, ASMCOP_IFNES, ASMCOP_IFNFMT, ASMCOP_IFNGPU, ASMCOP_IFNOTDEF,
    ASMCOP_IRP, ASMCOP_IRPC, ASMCOP_MACRO, ASMCOP_REPT, ASMCOP_WHILE
};

/// pseudo-ops not ignored while putting macro content
enum
{ ASMMROP_ENDM = 0, ASMMROP_ENDMACRO, ASMMROP_ENDR, ASMMROP_ENDREPT,
    ASMMROP_FOR, ASMMROP_IRP, ASMMROP_IRPC, ASMMROP_MACRO, ASMMROP_REPT,
    ASMMROP_WHILE };

/// all main pseudo-ops (sorted by name)
static const char* pseudoOpNamesTbl[] =
{
    "32bit", "64bit", "abort", "align", "altmacro",
    "amd", "amdcl2", "arch", "ascii", "asciz",
    "balign", "balignl", "balignw", "buggyfplit", "byte",
    "cf_call", "cf_cjump", "cf_end",
    "cf_jump", "cf_ret", "cf_start",
    "data", "double", "else",
    "elseif", "elseif32", "elseif64",
    "elseifarch", "elseifb", "elseifc", "elseifdef",
    "elseifeq", "elseifeqs", "elseiffmt",
    "elseifge", "elseifgpu", "elseifgt",
    "elseifle", "elseiflt", "elseifnarch", "elseifnb",
    "elseifnc", "elseifndef", "elseifne", "elseifnes",
    "elseifnfmt", "elseifngpu", "elseifnotdef",
    "end", "endif", "endm", "endmacro",
    "endr", "endrept", "ends", "endscope",
    "enum", "equ", "equiv", "eqv",
    "err", "error", "exitm", "extern",
    "fail", "file", "fill", "fillq",
    "float", "for", "format", "gallium", "get_64bit", "get_arch",
    "get_format", "get_gpu", "get_policy", "get_version", "global",
    "globl", "gpu", "half", "hword", "if", "if32", "if64",
    "ifarch", "ifb", "ifc", "ifdef", "ifeq",
    "ifeqs", "iffmt", "ifge", "ifgpu", "ifgt", "ifle",
    "iflt", "ifnarch", "ifnb", "ifnc", "ifndef",
    "ifne", "ifnes", "ifnfmt", "ifngpu", "ifnotdef", "incbin",
    "include", "int", "irp", "irpc", "kernel", "lflags",
    "line", "ln", "local", "long",
    "macro", "macrocase", "main", "noaltmacro",
    "nobuggyfplit", "nomacrocase", "nooldmodparam", "octa",
    "offset", "oldmodparam", "org",
    "p2align", "policy", "print", "purgem", "quad",
    "rawcode", "regvar", "rept", "rocm", "rodata",
    "rvlin", "rvlin_once", "sbttl", "scope", "section", "set",
    "short", "single", "size", "skip",
    "space", "string", "string16", "string32",
    "string64", "struct", "text", "title",
    "undef", "unusing", "usereg", "using", "version",
    "warning", "weak", "while", "word"
};

// enum for all pseudo-ops
enum
{
    ASMOP_32BIT = 0, ASMOP_64BIT, ASMOP_ABORT, ASMOP_ALIGN, ASMOP_ALTMACRO,
    ASMOP_AMD, ASMOP_AMDCL2, ASMOP_ARCH, ASMOP_ASCII, ASMOP_ASCIZ,
    ASMOP_BALIGN, ASMOP_BALIGNL, ASMOP_BALIGNW, ASMOP_BUGGYFPLIT, ASMOP_BYTE,
    ASMOP_CF_CALL, ASMOP_CF_CJUMP, ASMOP_CF_END,
    ASMOP_CF_JUMP, ASMOP_CF_RET, ASMOP_CF_START,
    ASMOP_DATA, ASMOP_DOUBLE, ASMOP_ELSE,
    ASMOP_ELSEIF, ASMOP_ELSEIF32, ASMOP_ELSEIF64,
    ASMOP_ELSEIFARCH, ASMOP_ELSEIFB, ASMOP_ELSEIFC, ASMOP_ELSEIFDEF,
    ASMOP_ELSEIFEQ, ASMOP_ELSEIFEQS, ASMOP_ELSEIFFMT,
    ASMOP_ELSEIFGE, ASMOP_ELSEIFGPU, ASMOP_ELSEIFGT,
    ASMOP_ELSEIFLE, ASMOP_ELSEIFLT, ASMOP_ELSEIFNARCH, ASMOP_ELSEIFNB,
    ASMOP_ELSEIFNC, ASMOP_ELSEIFNDEF, ASMOP_ELSEIFNE, ASMOP_ELSEIFNES,
    ASMOP_ELSEIFNFMT, ASMOP_ELSEIFNGPU, ASMOP_ELSEIFNOTDEF,
    ASMOP_END, ASMOP_ENDIF, ASMOP_ENDM, ASMOP_ENDMACRO,
    ASMOP_ENDR, ASMOP_ENDREPT, ASMOP_ENDS, ASMOP_ENDSCOPE,
    ASMOP_ENUM, ASMOP_EQU, ASMOP_EQUIV, ASMOP_EQV,
    ASMOP_ERR, ASMOP_ERROR, ASMOP_EXITM, ASMOP_EXTERN,
    ASMOP_FAIL, ASMOP_FILE, ASMOP_FILL, ASMOP_FILLQ,
    ASMOP_FLOAT, ASMOP_FOR, ASMOP_FORMAT, ASMOP_GALLIUM, ASMOP_GET_64BIT, ASMOP_GET_ARCH,
    ASMOP_GET_FORMAT, ASMOP_GET_GPU, ASMOP_GET_POLICY, ASMOP_GET_VERSION, ASMOP_GLOBAL,
    ASMOP_GLOBL, ASMOP_GPU, ASMOP_HALF, ASMOP_HWORD, ASMOP_IF, ASMOP_IF32, ASMOP_IF64,
    ASMOP_IFARCH, ASMOP_IFB, ASMOP_IFC, ASMOP_IFDEF, ASMOP_IFEQ,
    ASMOP_IFEQS, ASMOP_IFFMT, ASMOP_IFGE, ASMOP_IFGPU, ASMOP_IFGT, ASMOP_IFLE,
    ASMOP_IFLT, ASMOP_IFNARCH, ASMOP_IFNB, ASMOP_IFNC, ASMOP_IFNDEF,
    ASMOP_IFNE, ASMOP_IFNES, ASMOP_IFNFMT, ASMOP_IFNGPU, ASMOP_IFNOTDEF, ASMOP_INCBIN,
    ASMOP_INCLUDE, ASMOP_INT, ASMOP_IRP, ASMOP_IRPC, ASMOP_KERNEL, ASMOP_LFLAGS,
    ASMOP_LINE, ASMOP_LN, ASMOP_LOCAL, ASMOP_LONG,
    ASMOP_MACRO, ASMOP_MACROCASE, ASMOP_MAIN, ASMOP_NOALTMACRO,
    ASMOP_NOBUGGYFPLIT, ASMOP_NOMACROCASE, ASMOP_NOOLDMODPARAM, ASMOP_OCTA,
    ASMOP_OFFSET, ASMOP_OLDMODPARAM, ASMOP_ORG,
    ASMOP_P2ALIGN, ASMOP_POLICY, ASMOP_PRINT, ASMOP_PURGEM, ASMOP_QUAD,
    ASMOP_RAWCODE, ASMOP_REGVAR, ASMOP_REPT, ASMOP_ROCM, ASMOP_RODATA,
    ASMOP_RVLIN, ASMOP_RVLIN_ONCE, ASMOP_SBTTL, ASMOP_SCOPE, ASMOP_SECTION, ASMOP_SET,
    ASMOP_SHORT, ASMOP_SINGLE, ASMOP_SIZE, ASMOP_SKIP,
    ASMOP_SPACE, ASMOP_STRING, ASMOP_STRING16, ASMOP_STRING32,
    ASMOP_STRING64, ASMOP_STRUCT, ASMOP_TEXT, ASMOP_TITLE,
    ASMOP_UNDEF, ASMOP_UNUSING, ASMOP_USEREG, ASMOP_USING, ASMOP_VERSION,
    ASMOP_WARNING, ASMOP_WEAK, ASMOP_WHILE, ASMOP_WORD
};

namespace CLRX
{

// checking whether name is pseudo-op name
// (checking any extra pseudo-op provided by format handler)
bool AsmPseudoOps::checkPseudoOpName(const CString& string)
{
    if (string.empty() || string[0] != '.')
        return false;
    const size_t pseudoOp = binaryFind(pseudoOpNamesTbl, pseudoOpNamesTbl +
                    sizeof(pseudoOpNamesTbl)/sizeof(char*), string.c_str()+1,
                   CStringLess()) - pseudoOpNamesTbl;
    if (pseudoOp < sizeof(pseudoOpNamesTbl)/sizeof(char*))
        return true;
    if (AsmGalliumPseudoOps::checkPseudoOpName(string))
        return true;
    if (AsmAmdPseudoOps::checkPseudoOpName(string))
        return true;
    if (AsmAmdCL2PseudoOps::checkPseudoOpName(string))
        return true;
    if (AsmROCmPseudoOps::checkPseudoOpName(string))
        return true;
    return false;
}

};


void Assembler::parsePseudoOps(const CString& firstName,
       const char* stmtPlace, const char* linePtr)
{
    const size_t pseudoOp = binaryFind(pseudoOpNamesTbl, pseudoOpNamesTbl +
                    sizeof(pseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - pseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case ASMOP_32BIT:
        case ASMOP_64BIT:
            AsmPseudoOps::setBitness(*this, linePtr, pseudoOp == ASMOP_64BIT);
            break;
        case ASMOP_ABORT:
            printError(stmtPlace, "Aborted!");
            endOfAssembly = true;
            break;
        case ASMOP_ALIGN:
        case ASMOP_BALIGN:
            AsmPseudoOps::doAlign(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ALTMACRO:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                alternateMacro = true;
            break;
        case ASMOP_ARCH:
            AsmPseudoOps::setGPUArchitecture(*this, linePtr);
            break;
        case ASMOP_ASCII:
            AsmPseudoOps::putStrings(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ASCIZ:
            AsmPseudoOps::putStrings(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_BALIGNL:
            AsmPseudoOps::doAlignWord<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_BALIGNW:
            AsmPseudoOps::doAlignWord<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_BUGGYFPLIT:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                buggyFPLit = true;
            break;
        case ASMOP_BYTE:
            AsmPseudoOps::putIntegers<cxbyte>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_AMD:
        case ASMOP_AMDCL2:
        case ASMOP_RAWCODE:
        case ASMOP_GALLIUM:
        case ASMOP_ROCM:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
            {
                if (formatHandler!=nullptr)
                    printError(linePtr, "Output format type is already defined");
                else
                    format = (pseudoOp == ASMOP_GALLIUM) ? BinaryFormat::GALLIUM :
                        (pseudoOp == ASMOP_AMD) ? BinaryFormat::AMD :
                        (pseudoOp == ASMOP_AMDCL2) ? BinaryFormat::AMDCL2 :
                        (pseudoOp == ASMOP_ROCM) ? BinaryFormat::ROCM :
                        BinaryFormat::RAWCODE;
            }
            break;
        case ASMOP_CF_CALL:
            AsmPseudoOps::addCodeFlowEntries(*this, stmtPlace, linePtr,
                                  AsmCodeFlowType::CALL);
            break;
        case ASMOP_CF_CJUMP:
            AsmPseudoOps::addCodeFlowEntries(*this, stmtPlace, linePtr,
                                  AsmCodeFlowType::CJUMP);
            break;
        case ASMOP_CF_END:
            AsmPseudoOps::addCodeFlowEntries(*this, stmtPlace, linePtr,
                                  AsmCodeFlowType::END);
            break;
        case ASMOP_CF_JUMP:
            AsmPseudoOps::addCodeFlowEntries(*this, stmtPlace, linePtr,
                                  AsmCodeFlowType::JUMP);
            break;
        case ASMOP_CF_RET:
            AsmPseudoOps::addCodeFlowEntries(*this, stmtPlace, linePtr,
                                  AsmCodeFlowType::RETURN);
            break;
        case ASMOP_CF_START:
            AsmPseudoOps::addCodeFlowEntries(*this, stmtPlace, linePtr,
                                  AsmCodeFlowType::START);
            break;
        case ASMOP_DATA:
            AsmPseudoOps::goToSection(*this, stmtPlace, stmtPlace, true);
            break;
        case ASMOP_DOUBLE:
            AsmPseudoOps::putFloats<uint64_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ELSE:
            AsmPseudoOps::doElse(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ELSEIF:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::NOT_EQUAL, true);
            break;
        case ASMOP_ELSEIF32:
            AsmPseudoOps::doIf64Bit(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIF64:
            AsmPseudoOps::doIf64Bit(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFARCH:
            AsmPseudoOps::doIfArch(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFB:
            AsmPseudoOps::doIfBlank(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFC:
            AsmPseudoOps::doIfCmpStr(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFDEF:
            AsmPseudoOps::doIfDef(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFEQ:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::EQUAL, true);
            break;
        case ASMOP_ELSEIFEQS:
            AsmPseudoOps::doIfStrEqual(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFFMT:
            AsmPseudoOps::doIfFmt(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFGE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::GREATER_EQUAL, true);
            break;
        case ASMOP_ELSEIFGPU:
            AsmPseudoOps::doIfGpu(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFGT:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::GREATER, true);
            break;
        case ASMOP_ELSEIFLE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::LESS_EQUAL, true);
            break;
        case ASMOP_ELSEIFLT:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::LESS, true);
            break;
        case ASMOP_ELSEIFNARCH:
            AsmPseudoOps::doIfArch(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNB:
            AsmPseudoOps::doIfBlank(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNC:
            AsmPseudoOps::doIfCmpStr(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNOTDEF:
        case ASMOP_ELSEIFNDEF:
            AsmPseudoOps::doIfDef(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::NOT_EQUAL, true);
            break;
        case ASMOP_ELSEIFNES:
            AsmPseudoOps::doIfStrEqual(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNFMT:
            AsmPseudoOps::doIfFmt(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNGPU:
            AsmPseudoOps::doIfGpu(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_END:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                endOfAssembly = true;
            break;
        case ASMOP_ENDIF:
            AsmPseudoOps::endIf(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ENDM:
        case ASMOP_ENDMACRO:
            AsmPseudoOps::endMacro(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ENDR:
        case ASMOP_ENDREPT:
            AsmPseudoOps::endRepeat(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ENDS:
        case ASMOP_ENDSCOPE:
            AsmPseudoOps::closeScope(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ENUM:
            AsmPseudoOps::doEnum(*this, linePtr);
            break;
        case ASMOP_EQU:
        case ASMOP_SET:
            AsmPseudoOps::setSymbol(*this, linePtr);
            break;
        case ASMOP_EQUIV:
            AsmPseudoOps::setSymbol(*this, linePtr, false);
            break;
        case ASMOP_EQV:
            AsmPseudoOps::setSymbol(*this, linePtr, false, true);
            break;
        case ASMOP_ERR:
            printError(stmtPlace, ".err encountered");
            break;
        case ASMOP_ERROR:
            AsmPseudoOps::doError(*this, stmtPlace, linePtr);
            break;
        case ASMOP_EXITM:
            AsmPseudoOps::exitMacro(*this, stmtPlace, linePtr);
            break;
        case ASMOP_EXTERN:
            AsmPseudoOps::ignoreExtern(*this, linePtr);
            break;
        case ASMOP_FAIL:
            AsmPseudoOps::doFail(*this, stmtPlace, linePtr);
            break;
        case ASMOP_FILE:
            printWarning(stmtPlace, "'.file' is ignored by this assembler.");
            break;
        case ASMOP_FILL:
            AsmPseudoOps::doFill(*this, stmtPlace, linePtr, false);
            break;
        case ASMOP_FILLQ:
            AsmPseudoOps::doFill(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_FLOAT:
            AsmPseudoOps::putFloats<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_FOR:
            AsmPseudoOps::doFor(*this, stmtPlace, linePtr);
            break;
        case ASMOP_FORMAT:
            AsmPseudoOps::setOutFormat(*this, linePtr);
            break;
        case ASMOP_GET_64BIT:
            AsmPseudoOps::getPredefinedValue(*this, linePtr, AsmPredefined::BIT64);
            break;
        case ASMOP_GET_ARCH:
            AsmPseudoOps::getPredefinedValue(*this, linePtr, AsmPredefined::ARCH);
            break;
        case ASMOP_GET_FORMAT:
            AsmPseudoOps::getPredefinedValue(*this, linePtr, AsmPredefined::FORMAT);
            break;
        case ASMOP_GET_GPU:
            AsmPseudoOps::getPredefinedValue(*this, linePtr, AsmPredefined::GPU);
            break;
        case ASMOP_GET_POLICY:
            AsmPseudoOps::getPredefinedValue(*this, linePtr, AsmPredefined::POLICY);
            break;
        case ASMOP_GET_VERSION:
            AsmPseudoOps::getPredefinedValue(*this, linePtr, AsmPredefined::VERSION);
            break;
        case ASMOP_GLOBAL:
        case ASMOP_GLOBL:
            AsmPseudoOps::setSymbolBind(*this, linePtr, STB_GLOBAL);
            break;
        case ASMOP_GPU:
            AsmPseudoOps::setGPUDevice(*this, linePtr);
            break;
        case ASMOP_HALF:
            AsmPseudoOps::putFloats<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_HWORD:
            AsmPseudoOps::putIntegers<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_IF:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::NOT_EQUAL, false);
            break;
        case ASMOP_IF32:
            AsmPseudoOps::doIf64Bit(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IF64:
            AsmPseudoOps::doIf64Bit(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFARCH:
            AsmPseudoOps::doIfArch(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFB:
            AsmPseudoOps::doIfBlank(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFC:
            AsmPseudoOps::doIfCmpStr(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFDEF:
            AsmPseudoOps::doIfDef(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFEQ:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::EQUAL, false);
            break;
        case ASMOP_IFEQS:
            AsmPseudoOps::doIfStrEqual(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFFMT:
            AsmPseudoOps::doIfFmt(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFGE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::GREATER_EQUAL, false);
            break;
        case ASMOP_IFGPU:
            AsmPseudoOps::doIfGpu(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFGT:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::GREATER, false);
            break;
        case ASMOP_IFLE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::LESS_EQUAL, false);
            break;
        case ASMOP_IFLT:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::LESS, false);
            break;
        case ASMOP_IFNARCH:
            AsmPseudoOps::doIfArch(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNB:
            AsmPseudoOps::doIfBlank(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNC:
            AsmPseudoOps::doIfCmpStr(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNDEF:
        case ASMOP_IFNOTDEF:
            AsmPseudoOps::doIfDef(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::NOT_EQUAL, false);
            break;
        case ASMOP_IFNES:
            AsmPseudoOps::doIfStrEqual(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNFMT:
            AsmPseudoOps::doIfFmt(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNGPU:
            AsmPseudoOps::doIfGpu(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_INCBIN:
            AsmPseudoOps::includeBinFile(*this, stmtPlace, linePtr);
            break;
        case ASMOP_INCLUDE:
            AsmPseudoOps::includeFile(*this, stmtPlace, linePtr);
            break;
        case ASMOP_IRP:
            AsmPseudoOps::doIRP(*this, stmtPlace, linePtr, false);
            break;
        case ASMOP_IRPC:
            AsmPseudoOps::doIRP(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_INT:
        case ASMOP_LONG:
            AsmPseudoOps::putIntegers<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_KERNEL:
            AsmPseudoOps::goToKernel(*this, stmtPlace, linePtr);
            break;
        case ASMOP_LFLAGS:
            printWarning(stmtPlace, "'.lflags' is ignored by this assembler.");
            break;
        case ASMOP_LINE:
        case ASMOP_LN:
            printWarning(stmtPlace, "'.line' is ignored by this assembler.");
            break;
        case ASMOP_LOCAL:
            AsmPseudoOps::setSymbolBind(*this, linePtr, STB_LOCAL);
            break;
        case ASMOP_MACRO:
            AsmPseudoOps::doMacro(*this, stmtPlace, linePtr);
            break;
        case ASMOP_MACROCASE:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                macroCase = true;
            break;
        case ASMOP_MAIN:
            AsmPseudoOps::goToMain(*this, stmtPlace, linePtr);
            break;
        case ASMOP_NOALTMACRO:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                alternateMacro = false;
            break;
        case ASMOP_NOBUGGYFPLIT:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                buggyFPLit = false;
            break;
        case ASMOP_NOMACROCASE:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                macroCase = false;
            break;
        case ASMOP_NOOLDMODPARAM:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                oldModParam = false;
            break;
        case ASMOP_OCTA:
            AsmPseudoOps::putUInt128s(*this, stmtPlace, linePtr);
            break;
        case ASMOP_OFFSET:
            AsmPseudoOps::setAbsoluteOffset(*this, linePtr);
            break;
        case ASMOP_OLDMODPARAM:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                oldModParam = true;
            break;
        case ASMOP_ORG:
            AsmPseudoOps::doOrganize(*this, linePtr);
            break;
        case ASMOP_P2ALIGN:
            AsmPseudoOps::doAlign(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_POLICY:
            AsmPseudoOps::setPolicyVersion(*this, linePtr);
            break;
        case ASMOP_PRINT:
            AsmPseudoOps::doPrint(*this, linePtr);
            break;
        case ASMOP_PURGEM:
            AsmPseudoOps::purgeMacro(*this, linePtr);
            break;
        case ASMOP_QUAD:
            AsmPseudoOps::putIntegers<uint64_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_REGVAR:
            AsmPseudoOps::defRegVar(*this, linePtr);
            break;
        case ASMOP_REPT:
            AsmPseudoOps::doRepeat(*this, stmtPlace, linePtr);
            break;
        case ASMOP_RODATA:
            AsmPseudoOps::goToSection(*this, stmtPlace, stmtPlace, true);
            break;
        case ASMOP_RVLIN:
            AsmPseudoOps::declareRegVarLinearDeps(*this, linePtr, false);
            break;
        case ASMOP_RVLIN_ONCE:
            AsmPseudoOps::declareRegVarLinearDeps(*this, linePtr, true);
            break;
        case ASMOP_SCOPE:
            AsmPseudoOps::openScope(*this, stmtPlace, linePtr);
            break;
        case ASMOP_SECTION:
            AsmPseudoOps::goToSection(*this, stmtPlace, linePtr);
            break;
        case ASMOP_SHORT:
            AsmPseudoOps::putIntegers<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_SINGLE:
            AsmPseudoOps::putFloats<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_SIZE:
            AsmPseudoOps::setSymbolSize(*this, linePtr);
            break;
        case ASMOP_SKIP:
        case ASMOP_SPACE:
            AsmPseudoOps::doSkip(*this, stmtPlace, linePtr);
            break;
        case ASMOP_STRING:
            AsmPseudoOps::putStrings(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_STRING16:
            AsmPseudoOps::putStringsToInts<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_STRING32:
            AsmPseudoOps::putStringsToInts<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_STRING64:
            AsmPseudoOps::putStringsToInts<uint64_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_STRUCT:
            AsmPseudoOps::setAbsoluteOffset(*this, linePtr);
            break;
        case ASMOP_TEXT:
            AsmPseudoOps::goToSection(*this, stmtPlace, stmtPlace, true);
            break;
        case ASMOP_SBTTL:
        case ASMOP_TITLE:
        case ASMOP_VERSION:
            AsmPseudoOps::ignoreString(*this, linePtr);
            break;
        case ASMOP_UNDEF:
            AsmPseudoOps::undefSymbol(*this, linePtr);
            break;
        case ASMOP_UNUSING:
            AsmPseudoOps::stopUsing(*this, linePtr);
            break;
        case ASMOP_USEREG:
            AsmPseudoOps::doUseReg(*this, linePtr);
            break;
        case ASMOP_USING:
            AsmPseudoOps::startUsing(*this, linePtr);
            break;
        case ASMOP_WARNING:
            AsmPseudoOps::doWarning(*this, stmtPlace, linePtr);
            break;
        case ASMOP_WEAK:
            AsmPseudoOps::setSymbolBind(*this, linePtr, STB_WEAK);
            break;
        case ASMOP_WHILE:
            AsmPseudoOps::doWhile(*this, stmtPlace, linePtr);
            break;
        case ASMOP_WORD:
            AsmPseudoOps::putIntegers<uint32_t>(*this, stmtPlace, linePtr);
            break;
        default:
        {
            bool isGalliumPseudoOp = AsmGalliumPseudoOps::checkPseudoOpName(firstName);
            bool isAmdPseudoOp = AsmAmdPseudoOps::checkPseudoOpName(firstName);
            bool isAmdCL2PseudoOp = AsmAmdCL2PseudoOps::checkPseudoOpName(firstName);
            bool isROCmPseudoOp = AsmROCmPseudoOps::checkPseudoOpName(firstName);
            if (isGalliumPseudoOp || isAmdPseudoOp || isAmdCL2PseudoOp || isROCmPseudoOp)
            {
                // initialize only if gallium pseudo-op or AMD pseudo-op
                initializeOutputFormat();
                /// try to parse
                if (!formatHandler->parsePseudoOp(firstName, stmtPlace, linePtr))
                {
                    // check other
                    if (format != BinaryFormat::GALLIUM)
                    {
                        // check gallium pseudo-op
                        if (isGalliumPseudoOp)
                        {
                            printError(stmtPlace, "Gallium pseudo-op can be defined "
                                    "only in Gallium format code");
                            break;
                        }
                    }
                    if (format != BinaryFormat::AMD)
                    {
                        // check amd pseudo-op
                        if (isAmdPseudoOp)
                        {
                            printError(stmtPlace, "AMD pseudo-op can be defined only in "
                                    "AMD format code");
                            break;
                        }
                    }
                    if (format != BinaryFormat::AMDCL2)
                    {
                        // check amd pseudo-op
                        if (isAmdCL2PseudoOp)
                        {
                            printError(stmtPlace, "AMDCL2 pseudo-op can be defined only in "
                                    "AMDCL2 format code");
                            break;
                        }
                    }
                    if (format != BinaryFormat::ROCM)
                    {
                        // check rocm pseudo-op
                        if (isROCmPseudoOp)
                        {
                            printError(stmtPlace, "ROCm pseudo-op can be defined "
                                    "only in ROCm format code");
                            break;
                        }
                    }
                }
            }
            else if (makeMacroSubstitution(stmtPlace) == ParseState::MISSING)
                printError(stmtPlace, "This is neither pseudo-op and nor macro");
            break;
        }
    }
}

/* skipping clauses */
bool Assembler::skipClauses(bool exitm)
{
    const cxuint clauseLevel = clauses.size();
    AsmClauseType topClause = (!clauses.empty()) ? clauses.top().type :
            AsmClauseType::IF;
    const bool isTopIfClause = (topClause == AsmClauseType::IF ||
            topClause == AsmClauseType::ELSEIF || topClause == AsmClauseType::ELSE);
    bool good = true;
    const size_t inputFilterTop = asmInputFilters.size();
    while (exitm || clauses.size() >= clauseLevel)
    {
        if (!readLine())
            break;
        // if exit from macro mode, exit when macro filter exits
        if (exitm && inputFilterTop > asmInputFilters.size())
        {
            // set lineAlreadyRead - next line after skipped region read will be read
            lineAlreadyRead  = true;
            break; // end of macro, 
        }
        
        const char* linePtr = line;
        const char* end = line+lineSize;
        skipSpacesAndLabels(linePtr, end);
        const char* stmtPlace = linePtr;
        if (linePtr == end || *linePtr != '.')
            continue;
        
        CString pseudoOpName = extractSymName(linePtr, end, false);
        toLowerString(pseudoOpName);
        
        const size_t pseudoOp = binaryFind(offlinePseudoOpNamesTbl,
               offlinePseudoOpNamesTbl + sizeof(offlinePseudoOpNamesTbl)/sizeof(char*),
               pseudoOpName.c_str()+1, CStringLess()) - offlinePseudoOpNamesTbl;
        
        // any conditional inside macro or repeat will be ignored
        bool insideMacroOrRepeat = !clauses.empty() && 
            (clauses.top().type == AsmClauseType::MACRO ||
                    clauses.top().type == AsmClauseType::REPEAT);
        switch(pseudoOp)
        {
            case ASMCOP_ENDIF:
                if (!AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                    good = false;   // if endif have garbages
                else if (!insideMacroOrRepeat)
                    if (!popClause(stmtPlace, AsmClauseType::IF))
                        good = false;
                break;
            case ASMCOP_ENDM:
            case ASMCOP_ENDMACRO:
                if (!AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                    good = false;   // if .endm have garbages
                else if (!popClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMCOP_ENDR:
            case ASMCOP_ENDREPT:
                if (!AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                    good = false;   // if .endr have garbages
                else if (!popClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            case ASMCOP_ELSE:
            case ASMCOP_ELSEIFARCH:
            case ASMCOP_ELSEIF:
            case ASMCOP_ELSEIF32:
            case ASMCOP_ELSEIF64:
            case ASMCOP_ELSEIFB:
            case ASMCOP_ELSEIFC:
            case ASMCOP_ELSEIFDEF:
            case ASMCOP_ELSEIFEQ:
            case ASMCOP_ELSEIFEQS:
            case ASMCOP_ELSEIFFMT:
            case ASMCOP_ELSEIFGE:
            case ASMCOP_ELSEIFGPU:
            case ASMCOP_ELSEIFGT:
            case ASMCOP_ELSEIFLE:
            case ASMCOP_ELSEIFLT:
            case ASMCOP_ELSEIFNARCH:
            case ASMCOP_ELSEIFNB:
            case ASMCOP_ELSEIFNC:
            case ASMCOP_ELSEIFNDEF:
            case ASMCOP_ELSEIFNE:
            case ASMCOP_ELSEIFNES:
            case ASMCOP_ELSEIFNFMT:
            case ASMCOP_ELSEIFNGPU:
            case ASMCOP_ELSEIFNOTDEF:
                if (pseudoOp == ASMCOP_ELSE &&
                            !AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                    good = false; // if .else have garbages
                else if (!insideMacroOrRepeat)
                {
                    if (!exitm && clauseLevel == clauses.size() && isTopIfClause)
                    {
                        /* set lineAlreadyRead - next line after skipped region read
                         * will be read */
                        lineAlreadyRead = true; // read
                        return good; // do exit
                    }
                    if (!pushClause(stmtPlace, (pseudoOp==ASMCOP_ELSE ?
                                AsmClauseType::ELSE : AsmClauseType::ELSEIF)))
                        good = false;
                }
                break;
            case ASMCOP_IF:
            case ASMCOP_IFARCH:
            case ASMCOP_IF32:
            case ASMCOP_IF64:
            case ASMCOP_IFB:
            case ASMCOP_IFC:
            case ASMCOP_IFDEF:
            case ASMCOP_IFEQ:
            case ASMCOP_IFEQS:
            case ASMCOP_IFFMT:
            case ASMCOP_IFGE:
            case ASMCOP_IFGPU:
            case ASMCOP_IFGT:
            case ASMCOP_IFLE:
            case ASMCOP_IFLT:
            case ASMCOP_IFNARCH:
            case ASMCOP_IFNB:
            case ASMCOP_IFNC:
            case ASMCOP_IFNDEF:
            case ASMCOP_IFNE:
            case ASMCOP_IFNES:
            case ASMCOP_IFNFMT:
            case ASMCOP_IFNGPU:
            case ASMCOP_IFNOTDEF:
                if (!insideMacroOrRepeat)
                {
                    if (!pushClause(stmtPlace, AsmClauseType::IF))
                        good = false;
                }
                break;
            case ASMCOP_MACRO:
                if (!pushClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMCOP_IRP:
            case ASMCOP_IRPC:
            case ASMCOP_REPT:
            case ASMCOP_FOR:
            case ASMCOP_WHILE:
                if (!pushClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            default:
                break;
        }
    }
    return good;
}

bool Assembler::putMacroContent(RefPtr<AsmMacro> macro)
{
    const cxuint clauseLevel = clauses.size();
    bool good = true;
    while (clauses.size() >= clauseLevel)
    {
        if (!readLine())
        {
            // error, must be finished in by .endm or .endmacro
            good = false;
            break;
        }
        
        const char* linePtr = line;
        const char* end = line+lineSize;
        skipSpacesAndLabels(linePtr, end);
        const char* stmtPlace = linePtr;
        // if not pseudo-op
        if (linePtr == end || *linePtr != '.')
        {
            macro->addLine(currentInputFilter->getMacroSubst(),
                  currentInputFilter->getSource(),
                  currentInputFilter->getColTranslations(), lineSize, line);
            continue;
        }
        
        CString pseudoOpName = extractSymName(linePtr, end, false);
        toLowerString(pseudoOpName);
        
        const size_t pseudoOp = binaryFind(macroRepeatPseudoOpNamesTbl,
               macroRepeatPseudoOpNamesTbl + sizeof(macroRepeatPseudoOpNamesTbl) /
               sizeof(char*), pseudoOpName.c_str()+1, CStringLess()) -
               macroRepeatPseudoOpNamesTbl;
        // handle pseudo-op in macro content
        switch(pseudoOp)
        {
            case ASMMROP_ENDM:
            case ASMMROP_ENDMACRO:
                if (!popClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMMROP_ENDR:
            case ASMMROP_ENDREPT:
                if (!popClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            case ASMMROP_MACRO:
                if (!pushClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMMROP_IRP:
            case ASMMROP_IRPC:
            case ASMMROP_REPT:
            case ASMMROP_FOR:
            case ASMMROP_WHILE:
                if (!pushClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            default:
                break;
        }
        if (pseudoOp != ASMMROP_ENDM || clauses.size() >= clauseLevel)
            // add line if is still in macro
            macro->addLine(currentInputFilter->getMacroSubst(),
                  currentInputFilter->getSource(),
                  currentInputFilter->getColTranslations(), lineSize, line);
    }
    return good;
}

bool Assembler::putRepetitionContent(AsmRepeat& repeat)
{
    const cxuint clauseLevel = clauses.size();
    bool good = true;
    while (clauses.size() >= clauseLevel)
    {
        if (!readLine())
        {
            // error, must be finished in by .endm or .endmacro
            good = false;
            break;
        }
        
        const char* linePtr = line;
        const char* end = line+lineSize;
        skipSpacesAndLabels(linePtr, end);
        const char* stmtPlace = linePtr;
        // if not pseudo-op
        if (linePtr == end || *linePtr != '.')
        {
            repeat.addLine(currentInputFilter->getMacroSubst(),
               currentInputFilter->getSource(), currentInputFilter->getColTranslations(),
               lineSize, line);
            continue;
        }
        
        CString pseudoOpName = extractSymName(linePtr, end, false);
        toLowerString(pseudoOpName);
        const size_t pseudoOp = binaryFind(macroRepeatPseudoOpNamesTbl,
               macroRepeatPseudoOpNamesTbl + sizeof(macroRepeatPseudoOpNamesTbl) /
               sizeof(char*), pseudoOpName.c_str()+1, CStringLess()) -
               macroRepeatPseudoOpNamesTbl;
        // handle pseudo-op in macro content
        switch(pseudoOp)
        {
            case ASMMROP_ENDM:
                if (!popClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMMROP_ENDR:
                if (!popClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            case ASMMROP_MACRO:
                if (!pushClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMMROP_IRP:
            case ASMMROP_IRPC:
            case ASMMROP_REPT:
            case ASMMROP_FOR:
            case ASMMROP_WHILE:
                if (!pushClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            default:
                break;
        }
        if (pseudoOp != ASMMROP_ENDR || clauses.size() >= clauseLevel)
            // add line if is still in repetition
            repeat.addLine(currentInputFilter->getMacroSubst(),
                   currentInputFilter->getSource(),
                   currentInputFilter->getColTranslations(), lineSize, line);
    }
    return good;
}
