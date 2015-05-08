kernel void imageTransform(read_only image2d_t inimg, write_only image2d_t outimg,
        global uint* inout)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    const size_t g0size = get_global_size(0);
    write_imageui(outimg, pos, read_imageui(inimg, pos)*2);
    inout[g0size*pos.y+pos.x] = inout[g0size*pos.y+pos.x]/67;
}

kernel void imageTransformS(read_only image2d_t inimg, write_only image2d_t outimg,
        global uint* inout, sampler_t samp)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    const size_t g0size = get_global_size(0);
    write_imageui(outimg, pos, read_imageui(inimg, samp, pos)*2);
    inout[g0size*pos.y+pos.x] = inout[g0size*pos.y+pos.x]/67;
}

kernel void imageTransformX(read_only image2d_t inimg, global uint* inout,
            write_only image2d_t outimg)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    const size_t g0size = get_global_size(0);
    write_imageui(outimg, pos, read_imageui(inimg, pos)*2);
    inout[g0size*pos.y+pos.x] = inout[g0size*pos.y+pos.x]/67;
}

constant sampler_t csamp1 = CLK_FILTER_LINEAR | CLK_ADDRESS_CLAMP;
constant sampler_t csamp2 = CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;

kernel void imageTransform5MS(
    read_only image2d_t inimg1, read_only image2d_t inimg2, read_only image2d_t inimg3,
    write_only image2d_t outimg1, write_only image2d_t outimg2, write_only image2d_t outimg3,
    sampler_t samp1, sampler_t samp2)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    const float4 c = read_imagef(inimg1, samp1, pos)*0.5f +
        read_imagef(inimg2, samp2, pos)*0.25f + read_imagef(inimg3, samp1, pos)*1.55f;
    write_imagef(outimg1, pos, c);
    const float4 c2 = read_imagef(inimg1, csamp1, pos)*0.1f -
        read_imagef(inimg2, csamp2, pos)*0.34f - read_imagef(inimg3, samp1, pos)*0.01f;
    write_imagef(outimg2, pos, c2);
    const float4 c3 = read_imagef(inimg1, samp1, pos)*0.1f *
        read_imagef(inimg2, csamp2, pos)*0.14f + read_imagef(inimg3, csamp1, pos)*0.51f;
    write_imagef(outimg3, pos, c3);
}


kernel void kernelPrintf(const global uint* input)
{
    printf("Hello world: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), input[get_global_id(0)]);
}

kernel void kernelPrintf2(const global uint* input, global uint* output, global uint* output2)
{
    size_t gid = get_global_id(0);
    printf("Hello world: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), input[gid]);
    output[gid] = input[gid]*4;
    output2[gid] = input[gid]*88;
}

kernel void kernelPrintf3(const global uint* input, global uint* output, global uint* output2,
            global float* mybuf)
{
    size_t gid = get_global_id(0);
    printf("Hello world: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), input[gid]);
    output[gid] = input[gid]*4;
    output2[gid] = input[gid]*88;
}

struct MyStructX {
    double16 a;
    double4 x;
    float16 b;
};

kernel void kernelPrintf4(const global uint* input, global uint* output, global uint* output2,
                struct MyStructX gf, int g)
{
    size_t gid = get_global_id(0);
    printf("Hello world: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), input[gid]);
    printf("Burtiurvgf: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), input[gid]);
    printf("xxxx: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), input[gid]);
    output[gid] = input[gid]*4;
    output2[gid] = input[gid]*88;
}

kernel void kernelPrintf5()
{
    printf("Hello world: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), get_global_size(0), get_global_size(1),
           get_global_size(2));
}

kernel void kernelPrintf6(const global uint* input, global uint* output, global uint* output2,
        global float* mybuf)
{
    size_t gid = get_global_id(0);
    printf("Hello world: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), input[gid]);
    output[gid] = input[gid]*4;
    output2[gid] = input[gid]*88;
}

kernel void kernelPrintf7(const global uint* input, global uint* output, global uint* output2,
        global float* mybuf, global float* mybuf2)
{
    size_t gid = get_global_id(0);
    printf("Hello world: %zu,%zu,%zu: %u\n", get_global_id(0), get_global_id(1),
           get_global_id(2), input[gid]);
    output[gid] = input[gid]*4;
    output2[gid] = input[gid]*88;
}
