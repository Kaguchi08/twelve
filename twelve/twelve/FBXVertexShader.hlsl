#include "FBXHeader.hlsli"

VSOut main(VSIn input)
{
    VSOut output;
    
    float4 ambientC = ambient;
    float4 diffuseC = diffuse;
    float4 specularC = specular;
    
    output.svpos = output.svpos = mul(mul(proj, view), input.pos);
    output.normal = input.normal;
    output.uv = input.uv;
    
    return output;
}