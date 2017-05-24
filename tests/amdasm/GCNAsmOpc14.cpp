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
#include "GCNAsmOpc.h"

const GCNAsmOpcodeCase encGCN14OpcodeCases[] =
{
    { "s_add_u32 s21, shared_base, s61\n", 0x80153debU, 0, false, true, "" },
    { "s_add_u32 s21, src_shared_base, s61\n", 0x80153debU, 0, false, true, "" },
    { "s_add_u32 s21, shared_limit, s61\n", 0x80153decU, 0, false, true, "" },
    { "s_add_u32 s21, src_shared_limit, s61\n", 0x80153decU, 0, false, true, "" },
    { "s_add_u32 s21, private_base, s61\n", 0x80153dedU, 0, false, true, "" },
    { "s_add_u32 s21, src_private_base, s61\n", 0x80153dedU, 0, false, true, "" },
    { "s_add_u32 s21, private_limit, s61\n", 0x80153deeU, 0, false, true, "" },
    { "s_add_u32 s21, src_private_limit, s61\n", 0x80153deeU, 0, false, true, "" },
    { "s_add_u32 s21, pops_exiting_wave_id, s61\n", 0x80153defU, 0, false, true, "" },
    { "s_add_u32 s21, src_pops_exiting_wave_id, s61\n", 0x80153defU, 0, false, true, "" },
    { "s_add_u32 s21, execz, s61\n", 0x80153dfcU, 0, false, true, "" },
    { "s_add_u32 s21, src_execz, s61\n", 0x80153dfcU, 0, false, true, "" },
    { "s_add_u32 s21, vccz, s61\n", 0x80153dfbU, 0, false, true, "" },
    { "s_add_u32 s21, src_vccz, s61\n", 0x80153dfbU, 0, false, true, "" },
    { "s_add_u32 s21, scc, s61\n", 0x80153dfdU, 0, false, true, "" },
    { "s_add_u32 s21, src_scc, s61\n", 0x80153dfdU, 0, false, true, "" },
    { "s_add_u32 ttmp0, s4, s61", 0x806c3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp1, s4, s61", 0x806d3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp2, s4, s61", 0x806e3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp3, s4, s61", 0x806f3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp4, s4, s61", 0x80703d04U, 0, false, true, "" },
    { "s_add_u32 ttmp5, s4, s61", 0x80713d04U, 0, false, true, "" },
    { "s_add_u32 ttmp14, s4, s61", 0x807a3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp15, s4, s61", 0x807b3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp16, s4, s61", 0x807b3d04U, 0, false, false,
        "test.s:1:11: Error: TTMPRegister number out of range (0-15)\n" },
    { "s_add_u32 ttmp[5], s4, s61", 0x80713d04U, 0, false, true, "" },
    { "s_add_u32 ttmp[5:5], s4, s61", 0x80713d04U, 0, false, true, "" },
    { "    s_add_u32  tma_lo, s4, s61", 0x806e3d04U, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
    { "    s_add_u32  tba_lo, s4, s61", 0x806e3d04U, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
    { nullptr, 0, 0, false, false, 0 }
};
