#include "FBXHeader.hlsli"

VSOut main(VSIn input)
{
    VSOut output;
    
    // todo ���[���h�s��
    
    output.svpos = output.svpos = mul(mul(proj, view), input.pos);
    output.normal = input.normal;
    output.uv = input.uv;
    
    return output;
}

float4 VSShadow(VSIn vsIn)
{
    // todo ���[���h�s��
    
    return mul(lightView, vsIn.pos);
}