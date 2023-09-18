#include "PrimitiveHeader.hlsli"

PSIn VSMain(VSIn vsIn)
{
    PSIn psIn;
    
    psIn.svpos = mul(world, vsIn.pos);
    
    psIn.worldPos = psIn.svpos;
    psIn.svpos = mul(view, psIn.svpos);
    psIn.svpos = mul(proj, psIn.svpos);
    psIn.normal = normalize(mul(world, vsIn.normal));
    
    psIn.uv = vsIn.uv;
    
    psIn.tangent = normalize(mul(world, vsIn.tangent));
    psIn.biNormal = normalize(mul(world, vsIn.biNormal));
    
    psIn.lightPos = mul(lightView, psIn.worldPos);
    
    return psIn;
}

float4 VSShadow(VSIn vsIn) : SV_POSITION
{
    vsIn.pos = mul(world, vsIn.pos);
    
    return mul(lightView, vsIn.pos);
}