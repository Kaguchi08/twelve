#include "FBXHeader.hlsli"

float4 main(PSIn input) : SV_TARGET
{
    float4 color = tex.Sample(smp, input.uv);

    //color *= diffuse;
    
    return color;
}