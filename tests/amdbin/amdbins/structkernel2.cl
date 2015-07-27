struct mystructsub
{
    ushort dx;
    ushort dy;
};

struct mystructsub2
{
    ushort fx;
    ushort fy;
    ushort fz;
};

struct mystruct
{
    float4 b;
    float4 a;
    float4 c;
    struct mystructsub sx;
    struct mystructsub2 sy;
    uint x;
    uint y;
};

union myunion
{
    uint4 x;
    float4 y;
    double4 z;
};

typedef enum _EnumX {
    DFFF1 = 0,
    DFFF2 = 1
} EnumX;

__kernel void myKernel1(struct mystruct b, uint n, global float* inout, union myunion ddv,
            struct mystructsub2 xdf, EnumX enumx)
{ }
