#include "FBXHeader.hlsli"

VSOut main(VSIn input)
{
    VSOut output;
    
    output.svpos = output.svpos = mul(mul(proj, view), input.pos);
    output.normal = input.normal;
    
    return output;
}