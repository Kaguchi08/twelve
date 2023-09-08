#include "PrimitiveHeader.hlsli"

PSIn VSMain(VSIn vsIn)
{
    PSIn psIn;
    
    psIn.worldPos = vsIn.pos;
    
    psIn.svpos = mul(view, vsIn.pos);
    psIn.svpos = mul(proj, psIn.svpos);
    psIn.normal = normalize(vsIn.normal);
    
    psIn.uv = vsIn.uv;
    
    psIn.tangent = normalize(vsIn.tangent);
    psIn.biNormal = normalize(vsIn.biNormal);
    
    return psIn;
}