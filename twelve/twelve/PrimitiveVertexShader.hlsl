#include "PrimitiveHeader.hlsli"

PSIn VSMain(VSIn vsIn)
{
    PSIn psIn;
    
    // todo ワールド行列
    
    psIn.worldPos = vsIn.pos;
    psIn.svpos = mul(view, vsIn.pos);
    psIn.svpos = mul(proj, psIn.svpos);
    psIn.normal = normalize(vsIn.normal);
    
    psIn.uv = vsIn.uv;
    
    psIn.tangent = normalize(vsIn.tangent);
    psIn.biNormal = normalize(vsIn.biNormal);
    
    return psIn;
}

float4 VSShadow(VSIn vsIn)
{
    // todo ワールド行列
    
    return mul(lightView, vsIn.pos);
}