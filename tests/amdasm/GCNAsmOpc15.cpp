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
#include "GCNAsmOpc.h"

const GCNAsmOpcodeCase encGCN15OpcodeCases[] =
{
    { "        s_add_u32       s21, s4, s61\n", 0x80153d04U, 0, false, true, "" },
    { "        s_add_u32       s21, s4, s100\n", 0x80156404U, 0, false, true, "" },
    { "        s_add_u32       s21, s4, s102\n", 0x80156604U, 0, false, true, "" },
    { "        s_add_u32       s21, s4, s103\n", 0x80156704U, 0, false, true, "" },
    { "        s_add_u32       s21, s4, s104\n", 0x80156804U, 0, false, true, "" },
    { "        s_add_u32       s21, s4, s105\n", 0x80156904U, 0, false, true, "" },
    { "xrv = %s105; s_add_u32 s21, s4, xrv\n", 0x80156904U, 0, false, true, "" },
    { "        s_add_u32       s21, s4, vcc_lo\n", 0x80156a04U, 0, false, true, "" },
    { "        s_add_u32       s21, s4, vcc_hi\n", 0x80156b04U, 0, false, true, "" },
    { "    s_add_u32  vcc[1:1], s4, s61", 0x806b3d04U, 0, false, true, "" },
    { "    s_add_u32  vcc[1], s4, s61", 0x806b3d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp0, s4, s61", 0x806c3d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp1, s4, s61", 0x806d3d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp[2:2], s4, s61", 0x806e3d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp12, s4, s61", 0x80783d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp15, s4, s61", 0x807b3d04U, 0, false, true, "" },
    { nullptr, 0, 0, false, false, 0 }
};

