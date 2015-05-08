/* prginfo7.cl - ratop */

kernel void transformer25gp(
    global uint8* inout1, global uint8* inout2, global uint8* inout3, global uint8* inout4,
    global uint8* inout5, global uint8* inout6, global uint8* inout7, global uint8* inout8,
    global uint8* inout9, global uint8* inout10, global uint8* inout11, global uint8* inout12,
    global uint8* inout13, global uint8* inout14, global uint8* inout15, global uint8* inout16,
    global uint8* inout17, global uint8* inout18, global uint8* inout19, global uint8* inout20,
    global uint8* inout21, global uint8* inout22, global uint8* inout23, global uint8* inout24,
    global uint8* inout25, global uint8* inout26, global uint8* inout27, global uint8* inout28,
    global uint8* inout29, global uint8* inout30, global uint8* inout31, global uint8* inout32,
    global uint8* inout33, global uint8* inout34, global uint8* inout35, global uint8* inout36,
    global uint8* inout37, global uint8* inout38, global uint8* inout39, global uint8* inout40,
    global uint8* inout41, global uint8* inout42, global uint8* inout43, global uint8* inout44,
    global uint8* inout45, global uint8* inout46, global uint8* inout47, global uint8* inout48,
    global uint8* inout49, global uint8* inout50, global uint8* inout51, global uint8* inout52,
    global uint8* inout53, global uint8* inout54, global uint8* inout55, global uint8* inout56,
    global uint8* inout57, global uint8* inout58, global uint8* inout59, global uint8* inout60,
    global uint8* inout61, global uint8* inout62, global uint8* inout63, global uint8* inout64,
    global uint8* inout65, global uint8* inout66, global uint8* inout67, global uint8* inout68,
    global uint8* inout69, global uint8* inout70, global uint8* inout71, global uint8* inout72,
    global uint8* inout73, global uint8* inout74, global uint8* inout75, global uint8* inout76,
    global uint8* inout77, global uint8* inout78, global uint8* inout79, global uint8* inout80,
    global uint8* inout81, global uint8* inout82, global uint8* inout83, global uint8* inout84,
    global uint8* inout85, global uint8* inout86, global uint8* inout87, global uint8* inout88,
    global uint8* inout89, global uint8* inout90, global uint8* inout91, global uint8* inout92,
    global uint8* inout93, global uint8* inout94, global uint8* inout95, global uint8* inout96,
    global uint8* inout97, global uint8* inout98, global uint8* inout99, global uint8* inout100,
    global uint8* inout101, global uint8* inout102, global uint8* inout103, global uint8* inout104,
    global uint8* inout105, global uint8* inout106, global uint8* inout107, global uint8* inout108,
    global uint8* inout109, global uint8* inout110, global uint8* inout111, global uint8* inout112,
    global uint8* inout113, global uint8* inout114, global uint8* inout115, global uint8* inout116,
    global uint8* inout117, global uint8* inout118, global uint8* inout119, global uint8* inout120,
    global uint8* inout121, global uint8* inout122, global uint8* inout123, global uint8* inout124,
    global uint8* inout125, global uint8* inout126, global uint8* inout127, global uint8* inout128,
    global uint8* inout129, global uint8* inout130, global uint8* inout131, global uint8* inout132,
    global uint8* inout133, global uint8* inout134, global uint8* inout135, global uint8* inout136,
    global uint8* inout137, global uint8* inout138, global uint8* inout139, global uint8* inout140)
{
    const size_t gid = get_global_id(0);
    inout1[gid] = inout1[gid]*5000;
    inout2[gid] = inout2[gid]*2;
    inout3[gid] = inout3[gid]*3;
    inout4[gid] = inout4[gid]*4;
    inout5[gid] = inout5[gid]*5;
    inout6[gid] = inout6[gid]*6;
    inout7[gid] = inout7[gid]*7;
    inout8[gid] = inout8[gid]*8;
    inout9[gid] = inout9[gid]*9;
    inout10[gid] = inout10[gid]*10;
    inout11[gid] = inout11[gid]*11;
    inout12[gid] = inout12[gid]*12;
    inout13[gid] = inout13[gid]*13;
    inout14[gid] = inout14[gid]*14;
    inout15[gid] = inout15[gid]*15;
    inout16[gid] = inout16[gid]*16;
    inout17[gid] = inout17[gid]*17;
    inout18[gid] = inout18[gid]*18;
    inout19[gid] = inout19[gid]*19;
    inout20[gid] = inout20[gid]*20;
    inout21[gid] = inout21[gid]*21;
    inout22[gid] = inout22[gid]*22;
    inout23[gid] = inout23[gid]*23;
    inout24[gid] = inout24[gid]*24;
    inout25[gid] = inout25[gid]*25;
    inout26[gid] = inout26[gid]*26;
    inout27[gid] = inout27[gid]*27;
    inout28[gid] = inout28[gid]*28;
    inout29[gid] = inout29[gid]*29;
    inout30[gid] = inout30[gid]*30;
    inout31[gid] = inout31[gid]*31;
    inout32[gid] = inout32[gid]*32;
    inout33[gid] = inout33[gid]*33;
    inout34[gid] = inout34[gid]*34;
    inout35[gid] = inout35[gid]*35;
    inout36[gid] = inout36[gid]*36;
    inout37[gid] = inout37[gid]*37;
    inout38[gid] = inout38[gid]*38;
    inout39[gid] = inout39[gid]*39;
    inout40[gid] = inout40[gid]*40;
    inout41[gid] = inout41[gid]*41;
    inout42[gid] = inout42[gid]*42;
    inout43[gid] = inout43[gid]*43;
    inout44[gid] = inout44[gid]*44;
    inout45[gid] = inout45[gid]*45;
    inout46[gid] = inout46[gid]*46;
    inout47[gid] = inout47[gid]*47;
    inout48[gid] = inout48[gid]*48;
    inout49[gid] = inout49[gid]*49;
    inout50[gid] = inout50[gid]*50;
    inout51[gid] = inout51[gid]*51;
    inout52[gid] = inout52[gid]*52;
    inout53[gid] = inout53[gid]*53;
    inout54[gid] = inout54[gid]*54;
    inout55[gid] = inout55[gid]*55;
    inout56[gid] = inout56[gid]*56;
    inout57[gid] = inout57[gid]*57;
    inout58[gid] = inout58[gid]*58;
    inout59[gid] = inout59[gid]*59;
    inout60[gid] = inout60[gid]*60;
    inout61[gid] = inout61[gid]*61;
    inout62[gid] = inout62[gid]*62;
    inout63[gid] = inout63[gid]*63;
    inout64[gid] = inout64[gid]*64;
    inout65[gid] = inout65[gid]*65;
    inout66[gid] = inout66[gid]*66;
    inout67[gid] = inout67[gid]*67;
    inout68[gid] = inout68[gid]*68;
    inout69[gid] = inout69[gid]*69;
    inout70[gid] = inout70[gid]*70;
    inout71[gid] = inout71[gid]*71;
    inout72[gid] = inout72[gid]*72;
    inout73[gid] = inout73[gid]*73;
    inout74[gid] = inout74[gid]*74;
    inout75[gid] = inout75[gid]*75;
    inout76[gid] = inout76[gid]*76;
    inout77[gid] = inout77[gid]*77;
    inout78[gid] = inout78[gid]*78;
    inout79[gid] = inout79[gid]*79;
    inout80[gid] = inout80[gid]*80;
    inout81[gid] = inout81[gid]*81;
    inout82[gid] = inout82[gid]*82;
    inout83[gid] = inout83[gid]*83;
    inout84[gid] = inout84[gid]*84;
    inout85[gid] = inout85[gid]*85;
    inout86[gid] = inout86[gid]*86;
    inout87[gid] = inout87[gid]*87;
    inout88[gid] = inout88[gid]*88;
    inout89[gid] = inout89[gid]*89;
    inout90[gid] = inout90[gid]*90;
    inout91[gid] = inout91[gid]*91;
    inout92[gid] = inout92[gid]*92;
    inout93[gid] = inout93[gid]*93;
    inout94[gid] = inout94[gid]*94;
    inout95[gid] = inout95[gid]*95;
    inout96[gid] = inout96[gid]*96;
    inout97[gid] = inout97[gid]*97;
    inout98[gid] = inout98[gid]*98;
    inout99[gid] = inout99[gid]*99;
    inout100[gid] = inout100[gid]*100;
    inout101[gid] = inout101[gid]*101;
    inout102[gid] = inout102[gid]*102;
    inout103[gid] = inout103[gid]*103;
    inout104[gid] = inout104[gid]*104;
    inout105[gid] = inout105[gid]*105;
    inout106[gid] = inout106[gid]*106;
    inout107[gid] = inout107[gid]*107;
    inout108[gid] = inout108[gid]*108;
    inout109[gid] = inout109[gid]*109;
    inout110[gid] = inout110[gid]*110;
    inout111[gid] = inout111[gid]*111;
    inout112[gid] = inout112[gid]*112;
    inout113[gid] = inout113[gid]*113;
    inout114[gid] = inout114[gid]*114;
    inout115[gid] = inout115[gid]*115;
    inout116[gid] = inout116[gid]*116;
    inout117[gid] = inout117[gid]*117;
    inout118[gid] = inout118[gid]*118;
    inout119[gid] = inout119[gid]*119;
    inout120[gid] = inout120[gid]*120;
    inout121[gid] = inout121[gid]*121;
    inout122[gid] = inout122[gid]*122;
    inout123[gid] = inout123[gid]*123;
    inout124[gid] = inout124[gid]*124;
    inout125[gid] = inout125[gid]*125;
    inout126[gid] = inout126[gid]*126;
    inout127[gid] = inout127[gid]*127;
    inout128[gid] = inout128[gid]*128;
    inout129[gid] = inout129[gid]*129;
    inout130[gid] = inout130[gid]*130;
    inout131[gid] = inout131[gid]*131;
    inout132[gid] = inout132[gid]*132;
    inout133[gid] = inout133[gid]*133;
    inout134[gid] = inout134[gid]*134;
    inout135[gid] = inout135[gid]*135;
    inout136[gid] = inout136[gid]*136;
    inout137[gid] = inout137[gid]*137;
    inout138[gid] = inout138[gid]*138;
    inout139[gid] = inout139[gid]*139;
    inout140[gid] = inout140[gid]*140;
}
