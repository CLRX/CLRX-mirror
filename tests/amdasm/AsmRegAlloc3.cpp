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
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include "../TestUtils.h"
#include "AsmRegAlloc.h"

using namespace CLRX;

typedef AsmRegAllocator::OutLiveness OutLiveness;
typedef AsmRegAllocator::LinearDep LinearDep;
typedef AsmRegAllocator::VarIndexMap VarIndexMap;

struct LinearDep2
{
    cxbyte align;
    Array<size_t> prevVidxes;
    Array<size_t> nextVidxes;
};

struct AsmLivenessesCase
{
    const char* input;
    Array<OutLiveness> livenesses[MAX_REGTYPES_NUM];
    Array<std::pair<size_t, LinearDep2> > linearDepMaps[MAX_REGTYPES_NUM];
    bool good;
    const char* errorMessages;
};

static const AsmLivenessesCase createLivenessesCasesTbl[] =
{
    {   // 0 - simple case
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[4], s3
        v_xor_b32 va[4], va[2], v3
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // S3
                { { 0, 0 } }, // sa[2]'0
                { { 4, 5 } }, // sa[4]'0
                { { 8, 9 } }  // sa[4]'1
            },
            {   // for VGPRs
                { { 0, 9 } }, // V3
                { { 0, 9 } }, // va[2]'0
                { { SIZE_MAX-1, SIZE_MAX } } // va[4]'0 : out of range code block
            },
            { },
            { }
        },
        { },  // linearDepMaps
        true, ""
    },
    {   // 1 - simple case 2
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[4], s3
        v_xor_b32 va[4], va[2], v3
        s_endpgm
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // S3
                { { 0, 0 } }, // sa[2]'0
                { { 4, 5 } }, // sa[4]'0
                { { 8, 9 } }  // sa[4]'1
            },
            {   // for VGPRs
                { { 0, 9 } }, // V3
                { { 0, 9 } }, // va[2]'0
                { { 12, 13 } } // va[4]'0 : out of range code block
            },
            { },
            { }
        },
        { },  // linearDepMaps
        true, ""
    },
    {   // 2 - simple case (linear dep)
        R"ffDXD(.regvar sa:s:8, va:v:10
        .regvar sbuf:s:4, rbx4:v:6
        s_mov_b64 sa[4:5], sa[2:3]  # 0
        s_and_b64 sa[4:5], sa[4:5], s[4:5]
        v_add_f64 va[4:5], va[2:3], v[3:4]
        buffer_load_dwordx4 rbx4[1:5], va[6], sbuf[0:3], sa[7] idxen offset:603 tfe
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // S4
                { { 0, 5 } }, // S5
                { { 0, 0 } }, // sa[2]'0
                { { 0, 0 } }, // sa[3]'0
                { { 4, 5 } }, // sa[4]'0
                { { 8, 9 } }, // sa[4]'1
                { { 4, 5 } }, // sa[5]'0
                { { 8, 9 } }, // sa[5]'1
                { { 0, 17 } }, // sa[7]'0
                { { 0, 17 } }, // sbuf[0]'0
                { { 0, 17 } }, // sbuf[1]'0
                { { 0, 17 } }, // sbuf[2]'0
                { { 0, 17 } }, // sbuf[3]'0
            },
            {   // for VGPRs
                { { 0, 9 } }, // V3
                { { 0, 9 } }, // V4
                { { SIZE_MAX-1, SIZE_MAX } }, // rbx4[1]'0
                { { SIZE_MAX-1, SIZE_MAX } }, // rbx4[2]'0
                { { SIZE_MAX-1, SIZE_MAX } }, // rbx4[3]'0
                { { SIZE_MAX-1, SIZE_MAX } }, // rbx4[4]'0
                { { 0, 17 } }, // rbx4[5]'0: tfe - read before write
                { { 0, 9 } }, // va[2]'0
                { { 0, 9 } }, // va[3]'0
                { { 16, 17 } }, // va[4]'0 : out of range code block
                { { 16, 17 } }, // va[5]'0 : out of range code block
                { { 0, 17 } } // va[6]'0
            },
            { },
            { }
        },
        {   // linearDepMaps
            {   // for SGPRs
                { 0, { 0, { }, { 1 } } },  // S4
                { 1, { 0, { 0 }, { } } },  // S5
                { 2, { 2, { }, { 3 } } },  // sa[2]'0
                { 3, { 0, { 2 }, { } } },  // sa[3]'0
                { 4, { 2, { }, { 6 } } },  // sa[4]'0
                { 5, { 2, { }, { 7, 7 } } },  // sa[4]'1
                { 6, { 0, { 4 }, { } } },  // sa[5]'0
                { 7, { 0, { 5, 5 }, { } } },   // sa[5]'1
                { 9, { 4, { }, { 10 } } }, // sbuf[0]'0
                { 10, { 0, { 9 }, { 11 } } }, // sbuf[1]'0
                { 11, { 0, { 10 }, { 12 } } }, // sbuf[2]'0
                { 12, { 0, { 11 }, { } } }  // sbuf[3]'0
            },
            {   // for VGPRs
                { 0, { 0, { }, { 1 } } },  // V3
                { 1, { 0, { 0 }, { } } },  // V4
                { 2, { 1, { }, { 3 } } }, // rbx4[1]'0
                { 3, { 0, { 2 }, { 4 } } }, // rbx4[2]'0
                { 4, { 0, { 3 }, { 5 } } }, // rbx4[3]'0
                { 5, { 0, { 4 }, { 6 } } }, // rbx4[4]'0
                { 6, { 0, { 5 }, { } } }, // rbx4[5]'0
                { 7, { 1, { }, { 8 } } },  // va[2]'0
                { 8, { 0, { 7 }, { } } },  // va[3]'0
                { 9, { 1, { }, { 10 } } },  // va[4]'0
                { 10, { 0, { 9 }, { } } }  // va[5]'0
            },
            { },
            { }
        },
        true, ""
    },
    {   // 3 - next simple case
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[4], s3
        v_xor_b32 va[4], va[2], v3
        v_mov_b32 va[3], 1.45s
        v_mov_b32 va[6], -7.45s
        v_lshlrev_b32 va[5], sa[4], va[4]
        v_mac_f32 va[3], va[6], v0
        s_mul_i32 sa[7], sa[2], sa[3]
        v_mac_f32 va[3], va[7], v0
        s_endpgm
)ffDXD",
        {   // livenesses
            {   // for SGPRs
                { { 0, 5 } }, // 0: S3
                { { 0, 37 } }, // 1: sa[2]'0
                { { 0, 37 } }, // 2: sa[3]'0
                { { 4, 5 } }, // 3: sa[4]'0
                { { 8, 29 } }, // 4: sa[4]'1
                { { 40, 41 } }  // 5: sa[7]'0
            },
            {   // for VGPRs
                { { 0, 41 } }, // 0: v0
                { { 0, 9 } }, // 1: v3
                { { 0, 9 } }, // 2: va[2]'0
                { { 20, 41 } }, // 3: va[3]'0
                { { 12, 29 } }, // 4: va[4]'0
                { { 32, 33 } }, // 5: va[5]'0
                { { 28, 33 } }, // 6: va[6]'0
                { { 0, 41 } }  // 7: va[7]'0
            },
            { },
            { }
        },
        {
        },
        true, ""
    }
};

static TestSingleVReg getTestSingleVReg(const AsmSingleVReg& vr,
        const std::unordered_map<const AsmRegVar*, CString>& rvMap)
{
    if (vr.regVar == nullptr)
        return { "", vr.index };
    
    auto it = rvMap.find(vr.regVar);
    if (it == rvMap.end())
        throw Exception("getTestSingleVReg: RegVar not found!!");
    return { it->second, vr.index };
}

static void testCreateLivenessesCase(cxuint i, const AsmLivenessesCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input,
                    (ASM_ALL&~ASM_ALTMACRO) | ASM_TESTRUN | ASM_TESTRESOLVE,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " testAsmLivenesses#" << i;
        throw Exception(oss.str());
    }
    const AsmSection& section = assembler.getSections()[0];
    
    AsmRegAllocator regAlloc(assembler);
    
    regAlloc.createCodeStructure(section.codeFlow, section.getSize(),
                            section.content.data());
    regAlloc.createSSAData(*section.usageHandler);
    regAlloc.applySSAReplaces();
    regAlloc.createLivenesses(*section.usageHandler, section.getSize(),
                            section.content.data());
    
    std::ostringstream oss;
    oss << " testAsmLivenesses case#" << i;
    const std::string testCaseName = oss.str();
    
    assertValue<bool>("testAsmLivenesses", testCaseName+".good",
                      testCase.good, good);
    assertString("testAsmLivenesses", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
    
    std::unordered_map<const AsmRegVar*, CString> regVarNamesMap;
    for (const auto& rvEntry: assembler.getRegVarMap())
        regVarNamesMap.insert(std::make_pair(&rvEntry.second, rvEntry.first));
    
    // generate livenesses indices conversion table
    const VarIndexMap* vregIndexMaps = regAlloc.getVregIndexMaps();
    
    std::vector<size_t> lvIndexCvtTables[MAX_REGTYPES_NUM];
    std::vector<size_t> revLvIndexCvtTables[MAX_REGTYPES_NUM];
    for (size_t r = 0; r < MAX_REGTYPES_NUM; r++)
    {
        const VarIndexMap& vregIndexMap = vregIndexMaps[r];
        Array<std::pair<TestSingleVReg, const std::vector<size_t>*> > outVregIdxMap(
                    vregIndexMap.size());
        
        size_t j = 0;
        for (const auto& entry: vregIndexMap)
        {
            TestSingleVReg vreg = getTestSingleVReg(entry.first, regVarNamesMap);
            outVregIdxMap[j++] = std::make_pair(vreg, &entry.second);
        }
        mapSort(outVregIdxMap.begin(), outVregIdxMap.end());
        
        std::vector<size_t>& lvIndexCvtTable = lvIndexCvtTables[r];
        std::vector<size_t>& revLvIndexCvtTable = revLvIndexCvtTables[r];
        // generate livenessCvt table
        for (const auto& entry: outVregIdxMap)
            for (size_t v: *entry.second)
                if (v != SIZE_MAX)
                {
                    /*std::cout << "lvidx: " << v << ": " << entry.first.name << ":" <<
                            entry.first.index << std::endl;*/
                    size_t j = lvIndexCvtTable.size();
                    lvIndexCvtTable.push_back(v);
                    if (v+1 > revLvIndexCvtTable.size())
                        revLvIndexCvtTable.resize(v+1);
                    revLvIndexCvtTable[v] = j;
                }
    }
    
    const Array<OutLiveness>* resLivenesses = regAlloc.getOutLivenesses();
    for (size_t r = 0; r < MAX_REGTYPES_NUM; r++)
    {
        std::ostringstream rOss;
        rOss << "live.regtype#" << r;
        rOss.flush();
        std::string rtname(rOss.str());
        
        assertValue("testAsmLivenesses", testCaseName + rtname + ".size",
                    testCase.livenesses[r].size(), resLivenesses[r].size());
        
        for (size_t li = 0; li < resLivenesses[r].size(); li++)
        {
            std::ostringstream lOss;
            lOss << ".liveness#" << li;
            lOss.flush();
            std::string lvname(rtname + lOss.str());
            const OutLiveness& expLv = testCase.livenesses[r][li];
            const OutLiveness& resLv = resLivenesses[r][lvIndexCvtTables[r][li]];
            
            // checking liveness
            assertValue("testAsmLivenesses", testCaseName + lvname + ".size",
                    expLv.size(), resLv.size());
            for (size_t k = 0; k < resLv.size(); k++)
            {
                std::ostringstream regOss;
                regOss << ".reg#" << k;
                regOss.flush();
                std::string regname(lvname + regOss.str());
                assertValue("testAsmLivenesses", testCaseName + regname + ".first",
                    expLv[k].first, resLv[k].first);
                assertValue("testAsmLivenesses", testCaseName + regname + ".second",
                    expLv[k].second, resLv[k].second);
            }
        }
    }
    
    const std::unordered_map<size_t, LinearDep>* resLinearDepMaps =
                regAlloc.getLinearDepMaps();
    for (size_t r = 0; r < MAX_REGTYPES_NUM; r++)
    {
        std::ostringstream rOss;
        rOss << "lndep.regtype#" << r;
        rOss.flush();
        std::string rtname(rOss.str());
        
        assertValue("testAsmLivenesses", testCaseName + rtname + ".size",
                    testCase.linearDepMaps[r].size(), resLinearDepMaps[r].size());
        
        for (size_t di = 0; di < testCase.linearDepMaps[r].size(); di++)
        {
            std::ostringstream lOss;
            lOss << ".lndep#" << di;
            lOss.flush();
            std::string ldname(rtname + lOss.str());
            const auto& expLinearDepEntry = testCase.linearDepMaps[r][di];
            auto rlit = resLinearDepMaps[r].find(
                            lvIndexCvtTables[r][expLinearDepEntry.first]);
            
            std::ostringstream vOss;
            vOss << expLinearDepEntry.first;
            vOss.flush();
            assertTrue("testAsmLivenesses", testCaseName + ldname + ".key=" + vOss.str(),
                        rlit != resLinearDepMaps[r].end());
            const LinearDep2& expLinearDep = expLinearDepEntry.second;
            const LinearDep& resLinearDep = rlit->second;
            
            assertValue("testAsmLivenesses", testCaseName + ldname + ".align",
                        cxuint(expLinearDep.align), cxuint(resLinearDep.align));
            
            Array<size_t> resPrevVidxes(resLinearDep.prevVidxes.size());
            // convert to res ssaIdIndices
            for (size_t k = 0; k < resLinearDep.prevVidxes.size(); k++)
                resPrevVidxes[k] = revLvIndexCvtTables[r][resLinearDep.prevVidxes[k]];
            
            assertArray("testAsmLivenesses", testCaseName + ldname + ".prevVidxes",
                        expLinearDep.prevVidxes, resPrevVidxes);
            
            Array<size_t> resNextVidxes(resLinearDep.nextVidxes.size());
            // convert to res ssaIdIndices
            for (size_t k = 0; k < resLinearDep.nextVidxes.size(); k++)
                resNextVidxes[k] = revLvIndexCvtTables[r][resLinearDep.nextVidxes[k]];
            
            assertArray("testAsmLivenesses", testCaseName + ldname + ".nextVidxes",
                        expLinearDep.nextVidxes, resNextVidxes);
        }
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(createLivenessesCasesTbl)/sizeof(AsmLivenessesCase); i++)
        try
        { testCreateLivenessesCase(i, createLivenessesCasesTbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
