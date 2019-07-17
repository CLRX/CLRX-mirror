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
    { "s_add_u32 ttmp16, s4, s61", 0x807b3d04U, 0, false, false,
        "test.s:1:11: Error: TTMPRegister number out of range (0-15)\n" },
    { "    s_add_u32  tba_lo, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
    { "    s_add_u32  tba_hi, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
    { "    s_add_u32  tma_lo, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
    { "    s_add_u32  tma_hi, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
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
    /* register's symbols */
    { "zx=%s[20:23]; ss=%s4; b=%s[57:67];s_add_u32  zx[1], ss, b[4]",
            0x80153d04U, 0, false, true, "" },
    { "xv=2; zx=%s[xv+18:xv+19]; ss=%s[xv+2]; b=%s[xv+55:xv+65];"
        "s_add_u32  zx[1], ss, b[4]", 0x80153d04U, 0, false, true, "" },
    { "zx=%s[20:23]; ss=%execz; b=%s[57:67];s_add_u32  zx[1], ss, b[4]",
            0x80153dfcU, 0, false, true, "" },
    /* literals */
    { "    s_add_u32  s21, s4, 0x2a", 0x8015aa04U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -7", 0x8015c704U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, lit(-7)", 0x8015ff04U, 0xfffffff9, true, true, "" },
    { "    s_add_u32_e64  s21, s4, -7", 0x8015ff04U, 0xfffffff9, true, true, "" },
    { "    s_add_u32  s21, s4, lit(0)", 0x8015ff04U, 0x0, true, true, "" },
    { "    s_add_u32  s21, s4, lit(32)", 0x8015ff04U, 0x20, true, true, "" },
    { "    s_add_u32_e64 s21, s4, 32", 0x8015ff04U, 0x20, true, true, "" },
    { "    s_add_u32_e64 s21, 32, s4", 0x801504ffU, 0x20, true, true, "" },
    { "    s_add_u32  s21, s4, lit  (  32  )  ", 0x8015ff04U, 0x20, true, true, "" },
    { "    s_add_u32  s21, s4, LiT  (  32  )  ", 0x8015ff04U, 0x20, true, true, "" },
    { "    s_add_u32  s21, s4, lit(0.0)", 0x8015ff04U, 0x0, true, true, "" },
    { "    s_add_u32  s21, s4, lit(-.5)", 0x8015ff04U, 0xbf000000, true, true, "" },
    { "    s_add_u32_e64  s21, s4, -.5", 0x8015ff04U, 0xbf000000, true, true, "" },
    { "    s_add_u32  s21, s4, lit(2.0)", 0x8015ff04U, 0x40000000, true, true, "" },
    { "    s_add_u32  s21, s4, lit  ( 2.0 )", 0x8015ff04U, 0x40000000, true, true, "" },
    { "    s_add_u32_e64  s21, s4, 2.0", 0x8015ff04U, 0x40000000, true, true, "" },
    { "    s_add_u32  s21, s4, 1.", 0x8015f204U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -1.", 0x8015f304U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 2.", 0x8015f404U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -2.", 0x8015f504U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 4.", 0x8015f604U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -4.", 0x8015f704U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 1234", 0x8015ff04U, 1234, true, true, "" },
    { "    s_add_u32  s21, 1234, s4", 0x801504ffU, 1234, true, true, "" },
    { "    s_add_u32  s21, s4, -4.5", 0x8015ff04U, 0xc0900000U, true, true, "" },
    { "    s_add_u32  s21, s4, -4.5s", 0x8015ff04U, 0xc0900000U, true, true, "" },
    { "    s_add_u32  s21, s4, -4.5h", 0x8015ff04U, 0xc480U, true, true, "" },
    { "    s_add_u32  s21, s4, 3e-7", 0x8015ff04U, 0x34a10fb0U, true, true, "" },
    /* 64-bit registers and literals */
    { "    s_xor_b64 s[22:23], s[4:5], s[62:63]\n", 0x89963e04U, 0, false, true, "" },
    { "s1=22; s2=4;s3=62;s_xor_b64 s[s1:s1+1], s[s2:s2+1], s[s3:s3+1]\n",
        0x89963e04U, 0, false, true, "" },
    { "    s_xor_b64 vcc, s[4:5], s[62:63]\n", 0x89ea3e04U, 0, false, true, "" },
    { "    s_xor_b64 vcc[0:1], s[4:5], s[62:63]\n", 0x89ea3e04U, 0, false, true, "" },
    { "    s_xor_b64 s[22:23], 1.0, s[62:63]\n", 0x89963ef2U, 0, false, true, "" },
    { "    s_xor_b64 s[22:23], vccz, s[62:63]\n", 0x89963efbU, 0, false, true, "" },
    { "    s_xor_b64 s[22:23], execz, s[62:63]\n", 0x89963efcU, 0, false, true, "" },
    { "    s_xor_b64 s[22:23], scc, s[62:63]\n", 0x89963efdU, 0, false, true, "" },
    { nullptr, 0, 0, false, false, 0 }
};
