#include "PrimitiveHeader.hlsli"

PSIn VSMain(VSIn vsIn)
{
    PSIn psIn;
    
    // todo ���[���h�s��
    
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
    // todo ���[���h�s��
    
    return mul(lightView, vsIn.pos);
}