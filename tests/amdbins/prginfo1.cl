/* prginfo1 - global pointers */

kernel void transformer1(global uint* inout)
{
    const size_t gid = get_global_id(0);
    inout[gid] = inout[gid]*4345;
}

kernel void transformer(global uint8* inout)
{
    const size_t gid = get_global_id(0);
    inout[gid] = inout[gid]*4345;
}

kernel void transformern(uint x, global uint8* inout)
{
    const size_t gid = get_global_id(0);
    inout[gid] = inout[gid]*x;
}

kernel void transformer2d(global uint8* inout)
{
    const size_t gid0 = get_global_id(0);
    const size_t gid1 = get_global_id(1);
    const size_t gsize = get_global_id(0);
    inout[gid0+gid1*gsize] = inout[gid0+gid1*gsize]*4345;
}

kernel void transformer3d(global uint8* inout)
{
    const size_t gid0 = get_global_id(0);
    const size_t gid1 = get_global_id(1);
    const size_t gid2 = get_global_id(2);
    const size_t gsize0 = get_global_id(0);
    const size_t gsize1 = get_global_id(1);
    inout[gid0+(gid1+gid2*gsize1)*gsize0] = inout[gid0+(gid1+gid2*gsize1)*gsize0]*4345;
}

kernel void copier(const global uint8* in, global uint8* out)
{
    const size_t gid = get_global_id(0);
    out[gid] = in[gid]*4345;
}

kernel void changer1(const global uint8* in1, const global uint8* in2, global uint8* out)
{
    const size_t gid = get_global_id(0);
    out[gid] = in1[gid]*in2[gid];
}

kernel void changer2(const global uint8* in1, const global uint8* in2,
                global uint8* out1, global uint8* out2)
{
    const size_t gid = get_global_id(0);
    out1[gid] = in1[gid]*in2[gid];
    out2[gid] = in1[gid]+in2[gid];
}

kernel void constchanger2(const constant uint8* in1, const constant uint8* in2,
                global uint8* out1, global uint8* out2)
{
    const size_t gid = get_global_id(0);
    out1[gid] = in1[gid]*in2[gid];
    out2[gid] = in1[gid]+in2[gid];
}

kernel void transformerVoid(global void* inout)
{
    const size_t gid = get_global_id(0);
    ((global uint*)inout)[gid] = ((global uint*)inout)[gid]*4345;
}
