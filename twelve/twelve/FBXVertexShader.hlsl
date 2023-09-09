#include "FBXHeader.hlsli"

VSOut main(VSIn input)
{
    VSOut output;
    
    // todo ワールド行列
    
    output.svpos = output.svpos = mul(mul(proj, view), input.pos);
    output.normal = input.normal;
    output.uv = input.uv;
    
    return output;
}

float4 VSShadow(VSIn vsIn)
{
    // todo ワールド行列
    
    return mul(lightView, vsIn.pos);
}