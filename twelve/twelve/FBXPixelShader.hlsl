#include "FBXHeader.hlsli"

float4 main(VSOut input) : SV_TARGET
{
    float4 color = float4(1.0, 1.0, 1.0, 1.0);

    //color *= diffuse;
    
    return color;
}