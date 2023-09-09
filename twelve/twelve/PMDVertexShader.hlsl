#include "PMDHeader.hlsli"

PSIn VSMain(VSIn vsIn)
{
    PSIn psIn;
    float w = vsIn.weight / 100.0f;
    matrix bm = bones[vsIn.boneNo.x] * w + bones[vsIn.boneNo.y] * (1.0f - w);
    
    psIn.svPos = mul(bm, vsIn.pos);
    psIn.svPos = mul(world, psIn.svPos);
    
    if (vsIn.isntNo > 0)
    {
        psIn.svPos = mul(shadow, psIn.svPos);
    }
    
    psIn.worldPos = psIn.svPos;
    psIn.svPos = mul(mul(proj, view), psIn.svPos);
    vsIn.normal.w = 0;
    psIn.normal = mul(world, vsIn.normal);
    psIn.normalInView = mul(view, psIn.normal);
    psIn.uv = vsIn.uv;
    psIn.instNo = vsIn.isntNo;
    
    return psIn;
}

float4 VSShadow(VSIn vsIn)
{
    float w = vsIn.weight / 100.0f;
    matrix bm = bones[vsIn.boneNo.x] * w + bones[vsIn.boneNo.y] * (1.0f - w);
    
    vsIn.pos = mul(bm, vsIn.pos);
    vsIn.pos = mul(world, vsIn.pos);
    
    return mul(lightView, vsIn.pos);
}