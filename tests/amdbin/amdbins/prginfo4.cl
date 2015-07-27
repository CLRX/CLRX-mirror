/* prginfo4 - images with sampler in program scope */

constant sampler_t samp1 = CLK_FILTER_LINEAR | CLK_ADDRESS_CLAMP;

kernel void imageTransform(read_only image2d_t inimg, write_only image2d_t outimg)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    write_imageui(outimg, pos, read_imageui(inimg, samp1, pos)*2);
}

kernel void imageTransform2(read_only image2d_t inimg1, read_only image2d_t inimg2,
            write_only image2d_t outimg1, write_only image2d_t outimg2)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    const float4 c = read_imagef(inimg1, samp1, pos)*0.5f +
            read_imagef(inimg2, samp1, pos)*0.25f;
    write_imagef(outimg1, pos, c);
    const float4 c2 = read_imagef(inimg1, samp1, pos)*0.1f -
            read_imagef(inimg2, samp1, pos)*0.34f;
    write_imagef(outimg2, pos, c2);
}


kernel void imageTransform3(read_only image2d_t inimg1,
            write_only image2d_t outimg1, write_only image2d_t outimg2)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    const float4 c = read_imagef(inimg1, samp1, pos)*0.5f;
    write_imagef(outimg1, pos, c);
    write_imagef(outimg2, pos, c*0.4f);
}

kernel void imageTransform4(read_only image2d_t inimg1, read_only image2d_t inimg2,
                            write_only image2d_t outimg)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    write_imagef(outimg, pos,
        read_imagef(inimg1, samp1, pos)*0.5f + read_imagef(inimg2, samp1, pos)*0.25f);
}

kernel void imageTransform5(
    read_only image2d_t inimg1, read_only image2d_t inimg2, read_only image2d_t inimg3,
    write_only image2d_t outimg1, write_only image2d_t outimg2, write_only image2d_t outimg3)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    const float4 c = read_imagef(inimg1, samp1, pos)*0.5f +
        read_imagef(inimg2, samp1, pos)*0.25f + read_imagef(inimg3, samp1, pos)*1.55f;
    write_imagef(outimg1, pos, c);
    const float4 c2 = read_imagef(inimg1, samp1, pos)*0.1f -
        read_imagef(inimg2, samp1, pos)*0.34f - read_imagef(inimg3, samp1, pos)*0.01f;
    write_imagef(outimg2, pos, c2);
    const float4 c3 = read_imagef(inimg1, samp1, pos)*0.1f *
        read_imagef(inimg2, samp1, pos)*0.14f + read_imagef(inimg3, samp1, pos)*0.51f;
    write_imagef(outimg3, pos, c3);
}

kernel void imageTransform6(read_only image2d_t inimg1,
            write_only image2d_t outimg1, write_only image2d_t outimg2,
            write_only image2d_t outimg3)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    const float4 c = read_imagef(inimg1, samp1, pos)*0.5f;
    write_imagef(outimg1, pos, c);
    write_imagef(outimg2, pos, c*0.4f);
    write_imagef(outimg3, pos, c-1.3f);
}

kernel void imageTransform7(read_only image2d_t inimg1, read_only image2d_t inimg2,
        read_only image2d_t inimg3, write_only image2d_t outimg)
{
    const int2 pos = (int2)(get_global_id(0), get_global_id(1));
    write_imagef(outimg, pos,
        read_imagef(inimg1, samp1, pos)*0.5f + read_imagef(inimg2, samp1, pos)*0.25f -
            read_imagef(inimg3, samp1, pos)*1.3f);
}