/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS 1

#include <cstdio>
#include <iostream>
#include <memory>
#include <zlib.h>
#include <png.h>
#include <CLRX/utils/Utilities.h>
#include "CLUtils.h"

using namespace CLRX;

static const char* imageMixSource = R"ffDXD(# ImageMix example
SMUL = 1
.ifarch GCN1.2
    SMUL = 4
.elseifarch GCN1.4
    SMUL = 4
.endif
.iffmt amd    # if AMD Catalyst
.kernel imageMix
    .config
        .dims xy        # uses two dimensions
        .uavid 11       # force first UAV=12
        .arg img1, image, read_only     # read_only image2d_t img1
        .arg img2, image, read_only     # read_only image2d_t img2
        .arg outimg, image, write_only    # write_only image2d_t outimg
        .userdata ptr_resource_table, 0, 2, 2   # s[2:3] - res. table for read only images
        .userdata imm_const_buffer, 0, 4, 4     # s[4:7] - kernel setup const buffer
        .userdata imm_const_buffer, 0, 8, 4     # s[8:11] - kernel args const buffer
        .userdata ptr_uav_table, 0, 12, 2       # s[12:13] - uav table for write only images
    .text
        s_buffer_load_dwordx2 s[0:1], s[4:7], 0x4*SMUL   # s[0:1] - local_size(0,1)
        s_buffer_load_dwordx2 s[4:5], s[4:7], 0x18*SMUL  # s[4:5] - global_offset(0,1)
        s_buffer_load_dwordx2 s[6:7], s[8:11], 0x0  # s[6:7] - img1 width and height
        s_waitcnt  lgkmcnt(0)           # wait for results
        s_mul_i32  s0, s14, s0          # s[0:1] - local_size(0,1)*group_id(0,1)
        s_mul_i32  s1, s15, s1
        s_add_u32  s0, s4, s0           # s[0:1] + global_offset(0,1)
        s_add_u32  s1, s5, s1
    .ifarch GCN1.4
        v_add_co_u32  v0, vcc, s0, v0      #  v[0:1] - global_id(0,1)
        v_add_co_u32  v1, vcc, s1, v1
    .else
        v_add_i32  v0, vcc, s0, v0      #  v[0:1] - global_id(0,1)
        v_add_i32  v1, vcc, s1, v1
    .endif
        v_cmp_gt_i32  s[0:1], s7, v1    # global_id(1) < img1.height
        v_cmp_gt_i32  vcc, s6, v0       # global_id(0) < img1.width
        s_and_b64  vcc, s[0:1], vcc
        s_and_saveexec_b64 s[0:1], vcc      # deactivate obsolete threads
        s_cbranch_execz end                 # skip to end
        s_load_dwordx8  s[4:11], s[2:3], 0x0       # load img1 descriptor
        s_load_dwordx8  s[16:23], s[2:3], 0x8*SMUL        # load img2 descriptor
        s_load_dwordx8  s[24:31], s[12:13], 0x0     # load outimg descriptor
        s_waitcnt       lgkmcnt(0)      # wait for descriptors
        image_load  v[5:8], v[0:1], s[4:11] dmask:15 unorm    # load pixel from img1
        image_load  v[9:12], v[0:1], s[16:23] dmask:15 unorm    # load pixel from img2
        s_waitcnt       vmcnt(0)        # wait for results
        v_add_f32       v2, v5, v9      # img1+img2
        v_add_f32       v3, v6, v10
        v_add_f32       v4, v7, v11
        v_add_f32       v5, v8, v12
        v_mul_f32       v2, 0.5, v2     # img1*0.5
        v_mul_f32       v3, 0.5, v3
        v_mul_f32       v4, 0.5, v4
        v_mul_f32       v5, 0.5, v5
        image_store     v[2:5], v[0:1], s[24:31] dmask:15 unorm glc # store pixel to outimg
end:
        s_endpgm
.elseiffmt amdcl2  # AMD OpenCL 2.0 code
.kernel imageMix
    .config
        .dims xy
        .useargs
        .usesetup
        .setupargs
        .arg img1, image, read_only     # read_only image2d_t img1
        .arg img2, image, read_only     # read_only image2d_t img2
        .arg outimg, image, write_only    # write_only image2d_t outimg
    .text
    .ifarch GCN1.4
        GIDX = %s10
        GIDY = %s11
    .else
        GIDX = %s8
        GIDY = %s9
    .endif
    .if32
        s_load_dwordx2 s[0:1], s[6:7], 6*SMUL   # load img1
    .else
        s_load_dwordx2 s[0:1], s[6:7], 12*SMUL   # load img1
    .endif
    .if32
        s_load_dwordx2 s[2:3], s[6:7], 0          # global_offset(0,1)
    .else
        s_load_dword s2, s[6:7], 0          # global_offset(0)
        s_load_dword s3, s[6:7], 2*SMUL          # global_offset(1)
    .endif
        s_load_dword s4, s[4:5], 1*SMUL          # localSizes dword
        s_waitcnt lgkmcnt(0)
        s_lshr_b32 s5, s4, 16               # localsize(1)
        s_and_b32 s4, s4, 0xffff            # localsize(0)
        s_mul_i32 s4, GIDX, s4              # localsize(0)*groupid(0)
        s_add_u32 s4, s4, s2                # +global_offset(0)
        s_mul_i32 s5, GIDY, s5              # localsize(1)*groupid(1)
        s_add_u32 s5, s5, s3                # +global_offset(1)
    .ifarch GCN1.4
        v_add_co_u32 v0, vcc, s4, v0           # globalid(0)
        v_add_co_u32 v1, vcc, s5, v1           # globalid(1)
    .else
        v_add_i32 v0, vcc, s4, v0           # globalid(0)
        v_add_i32 v1, vcc, s5, v1           # globalid(1)
    .endif
        s_load_dword s8, s[0:1], 2*SMUL     # load third image desc dword (width,height)
        s_waitcnt lgkmcnt(0)
        s_and_b32 s4, s8, (1<<14)-1         # img width-1
        s_bfe_u32 s5, s8, (14<<16)+14       # img height-1
        s_add_u32 s4, 1, s4         # img width
        s_add_u32 s5, 1, s5         # img height
        v_cmp_gt_i32  s[2:3], s5, v1    # global_id(1) < img1.height
        v_cmp_gt_i32  vcc, s4, v0       # global_id(0) < img1.width
        s_and_b64  vcc, s[2:3], vcc
        s_and_saveexec_b64 s[2:3], vcc      # deactivate obsolete threads
        s_cbranch_execz end                 # skip to end
        
    .if32
        s_load_dwordx4 s[4:7], s[6:7], 8*SMUL   # load img2,outimg pointers
    .else
        s_load_dwordx4 s[4:7], s[6:7], 14*SMUL   # load img2,outimg pointers
    .endif
        s_waitcnt lgkmcnt(0)                # wait
        s_load_dwordx8 s[8:15], s[0:1], 0   # load first img desc
        s_load_dwordx8 s[16:23], s[4:5], 0  # load second img desc
        s_load_dwordx8 s[24:31], s[6:7], 0  # load outimg desc
        s_waitcnt lgkmcnt(0)                # wait for descriptors
        
        image_load v[5:8], v[0:1], s[8:15] dmask:15 unorm   # load pixel from img1
        image_load v[9:12], v[0:1], s[16:23] dmask:15 unorm   # load pixel from img2
        s_waitcnt vmcnt(0)
        v_add_f32       v2, v5, v9      # img1+img2
        v_add_f32       v3, v6, v10
        v_add_f32       v4, v7, v11
        v_add_f32       v5, v8, v12
        v_mul_f32       v2, 0.5, v2     # img1*0.5
        v_mul_f32       v3, 0.5, v3
        v_mul_f32       v4, 0.5, v4
        v_mul_f32       v5, 0.5, v5
        image_store v[2:5], v[0:1], s[24:31] dmask:15 unorm glc # store pixel to outimg
end:
        s_endpgm
.else
    .error "Unsupported GalliumCompute"
.endif
)ffDXD";

void png_warn_error_fn(png_structp pngstr, const char *str)
{
    fputs(str,stderr);
}

class ImageMix: public CLFacade
{
private:
    cl_uint width, height;
    cl_mem image1, image2, outImage;
    const char* outFileName;
    
    void loadImageInt(const char* inFilename);
public:
    ImageMix(cl_uint deviceIndex, cxuint useCL, const char* imgFileName1,
             const char* imgFileName2, const char* outFileName);
    ~ImageMix() = default;
    
    void run();
    void saveImage();
};

ImageMix::ImageMix(cl_uint deviceIndex, cxuint useCL, const char* imgFileName1,
             const char* imgFileName2, const char* _outFileName)
             : CLFacade(deviceIndex, imageMixSource, "imageMix", useCL),
               width(0), height(0), outFileName(_outFileName)
{
    loadImageInt(imgFileName1);
    loadImageInt(imgFileName2);
    image1 = memObjects[0];
    image2 = memObjects[1];
    const cl_image_format fmt = { CL_RGBA, CL_UNORM_INT8 };
    cl_int error;
    outImage = clCreateImage2D(context, CL_MEM_WRITE_ONLY, &fmt, width, height,
                0, nullptr, &error);
    if (error != CL_SUCCESS)
        throw CLError(error, "clCreateImage2D");
    memObjects.push_back(outImage);
}

void ImageMix::loadImageInt(const char* inFilename)
{
    FILE* file = nullptr;
    png_structp pngStruct = nullptr;
    png_infop pngInfo = nullptr, pngEndInfo = nullptr;
    cxuint imageWidth, imageHeight;
    try
    {
    file = fopen(inFilename, "rb");
    if (file==nullptr)
        throw Exception(std::string("Can't open file '")+inFilename+'\'');
    {
        /* check PNG signature */
        cxbyte header[8];
        if (fread(header, 1, 8, file)==0)
            throw Exception(std::string("Can't read file'")+inFilename+'\'');
        if (png_sig_cmp(header, 0, 8))
            throw Exception("This is not PNG file");
    }
    /* initialize libpng structures */
    pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                   png_warn_error_fn, png_warn_error_fn);
    if (pngStruct==nullptr)
        throw Exception("Can't initialize PNG reading");
    pngInfo = png_create_info_struct(pngStruct);
    if (pngInfo==nullptr)
        throw Exception("Can't initialize PNG reading");
    png_init_io(pngStruct, file);
    png_set_sig_bytes(pngStruct, 8);
    /* read PNG file */
    // final format is: 24-bit RGB or 32-bit RGBA
    png_read_png(pngStruct, pngInfo, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING |
                PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_STRIP_16, nullptr);
    imageWidth = png_get_image_width(pngStruct, pngInfo);
    imageHeight = png_get_image_height(pngStruct, pngInfo);
    if (this->width!=0)
    {
        if (imageWidth!=this->width || imageHeight!=this->height)
            throw Exception("Image size doesn't match!");
    }
    else
    {
        /* if not initialized (first image) */
        this->width = imageWidth;
        this->height = imageHeight;
    }
        
    cxbyte** rows = png_get_rows(pngStruct, pngInfo);
    /* creating CL image */
    cl_int error;
    const cl_image_format fmt = { CL_RGBA, CL_UNORM_INT8 };
    cl_mem image = clCreateImage2D(context, CL_MEM_READ_ONLY, &fmt, imageWidth, imageHeight,
                0, nullptr, &error);
    if (error != CL_SUCCESS)
        throw CLError(error, "clCreateImage2D");
    memObjects.push_back(image);
    
    const bool hasAlpha = (png_get_channels(pngStruct, pngInfo)&1)==0;
    std::unique_ptr<cxbyte[]> outRow;
    if (!hasAlpha)
        outRow.reset(new cxbyte[4*imageWidth]);
    
    for (size_t y = 0; y < height; y++)
    {
        /* transfer image to CL image object */
        size_t origin[3] = { 0, y, 0 };
        size_t region[3] = { imageWidth, 1, 1 };
        if (!hasAlpha)
        {
            // convert to row for image with alpha
            for (size_t x = 0; x < imageWidth; x++)
            {
                outRow[x*4] = rows[y][x*3];
                outRow[x*4+1] = rows[y][x*3+1];
                outRow[x*4+2] = rows[y][x*3+2];
                outRow[x*4+3] = 255;
            }
            error = clEnqueueWriteImage(queue, image, CL_TRUE, origin, region, 0, 0,
                        outRow.get(), 0, nullptr, nullptr);
        }
        else
            error = clEnqueueWriteImage(queue, image, CL_TRUE, origin, region, 0, 0,
                        rows[y], 0, nullptr, nullptr);
        if (error != CL_SUCCESS)
            throw CLError(error, "clEnqueueWriteImage");
    }
    }
    catch(...)
    {
        if (pngStruct!=nullptr)
            png_destroy_read_struct(&pngStruct, &pngInfo, &pngEndInfo);
        if (file!=nullptr)
            fclose(file);
        throw;
    }
    png_destroy_read_struct(&pngStruct, &pngInfo, &pngEndInfo);
    fclose(file);
}

void ImageMix::run()
{
    cl_int error;
    size_t origin[3] = { 0, 0, 0 };
    size_t region[3] = { width, height, 1 };
    {
        std::unique_ptr<cxbyte[]> filledData(new cxbyte[4*width*height]);
        std::fill(filledData.get(), filledData.get()+4*width*height, cxbyte(0));
        
        error = clEnqueueWriteImage(queue, memObjects[2], CL_TRUE, origin, region, 0, 0,
                    filledData.get(), 0, nullptr, nullptr);
        if (error != CL_SUCCESS)
            throw CLError(error, "clEnqueueWriteImage");
    }
    // set kernel
    clSetKernelArg(kernels[0], 0, sizeof(cl_mem), &image1);
    clSetKernelArg(kernels[0], 1, sizeof(cl_mem), &image2);
    clSetKernelArg(kernels[0], 2, sizeof(cl_mem), &outImage);
    
    size_t workGroupSize, workGroupSizeMul;
    getKernelInfo(kernels[0], workGroupSize, workGroupSizeMul);
    
    // calculating best localsize dimension (nearest to square root of work group size)
    size_t bestDim = 1;
    for (size_t xDim = 1; xDim*xDim <= workGroupSize; xDim++)
        if (xDim*(workGroupSize/xDim) == workGroupSize)
            bestDim = xDim;
    
    // calculating kernel local size and work size
    size_t localSize[2] = { workGroupSize/bestDim, bestDim };
    size_t workSize[2] = { (width+localSize[0]-1)/localSize[0]*localSize[0],
        (height+localSize[1]-1)/localSize[1]*localSize[1] };
    
    std::cout << "WorkSize: " << workSize[0] << "x" << workSize[1] <<
            ", LocalSize: " << localSize[0] << "x" << localSize[1] << std::endl;
    callNDRangeKernel(kernels[0], 2, nullptr, workSize, localSize);
    
    /* write image to output */
    std::unique_ptr<cl_uchar4[]> outPixels(new cl_uchar4[width*height]);
    error = clEnqueueReadImage(queue, outImage, CL_TRUE, origin, region, 0, 0,
                       outPixels.get(), 0, nullptr, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clEnqueueReadImage");
    FILE* outFile = nullptr;
    png_structp pngStruct = nullptr;
    png_infop pngInfo = nullptr;
    try
    {
        // writing ouput image
        outFile = fopen(outFileName, "wb");
        if (outFile==nullptr)
            throw Exception(std::string("Can't open file '")+outFileName+'\'');
        pngStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,
            png_warn_error_fn, png_warn_error_fn);
        if (pngStruct == nullptr)
            throw Exception("Can't initialize PNG structures!");
        pngInfo=png_create_info_struct(pngStruct);
        if(pngInfo == nullptr)
            throw Exception("Can't initialize PNG structures!");
        
        png_init_io(pngStruct, outFile);
        png_set_compression_level(pngStruct, Z_BEST_COMPRESSION);
        // final output is 32-bit RGBA with interlace
        png_set_IHDR(pngStruct, pngInfo, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(pngStruct,pngInfo);
        
        for (size_t y = 0; y < height; y++)
            png_write_row(pngStruct, (png_bytep)&outPixels[y*width]);
        png_write_end(pngStruct,pngInfo);
        png_destroy_write_struct(&pngStruct, &pngInfo);
        fclose(outFile);
    }
    catch(...)
    {
        if (pngStruct!=nullptr)
            png_destroy_write_struct(&pngStruct, &pngInfo);
        if (outFile!=nullptr)
            fclose(outFile);
        throw;
    }
}

int main(int argc, const char** argv)
try
{
    cl_uint deviceIndex = 0;
    cxuint useCL = 0;
    if (CLFacade::parseArgs("ImageMix", "[IMAGE1] [IMAGE2] [OUTIMAGE]",
                argc, argv, deviceIndex, useCL))
        return 0;
    const char* inFileName1 = "image1.png";
    const char* inFileName2 = "image2.png";
    const char* outFileName = "outimage.png";
    
    if (argc >= 3)
        inFileName1 = argv[2];
    if (argc >= 4)
        inFileName2 = argv[3];
    if (argc >= 5)
        outFileName = argv[4];
    
    ImageMix imageMix(deviceIndex, useCL, inFileName1, inFileName2, outFileName);
    imageMix.run();
    return 0;
}
catch(const std::exception& ex)
{
    std::cerr << ex.what() << std::endl;
    return 1;
}
catch(...)
{
    std::cerr << "unknown exception!" << std::endl;
    return 1;
}
