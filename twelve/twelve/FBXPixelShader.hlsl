#include "FBXHeader.hlsli"

float4 main(VSOut input) : SV_TARGET
{
	
    float4 color = float4(0.3, 0.3, 0.3, 1.0);

    return float4(input.normal.xyz, 1);
}