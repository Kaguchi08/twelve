
#include "Type.hlsli"

Texture2D<float4> tex : register(t0);
Texture2D<float4> sph : register(t1);
Texture2D<float4> spa : register(t2);
Texture2D<float4> toon : register(t3);

SamplerState smp : register(s0);
SamplerState smpToon : register(s1);

cbuffer SceneData : register(b0)
{
    matrix view;
    matrix proj;
    matrix light_view;
    matrix shadow;
    float3 eye;
};

cbuffer Material : register(b2)
{
    float4 diffuse;
    float3 specular;
    float specularity;
    float3 ambient;
};

float4 BasicPS(BasicType input) : SV_TARGET
{
    if (input.inst_no > 0)
    {
        return float4(0, 0, 0, 1);
    }
    
    float3 eye_ray = normalize(input.pos - eye);
    float3 light = normalize(float3(1, -1, 1));
    float3 r_light = reflect(light, input.normal);
    
    // スペキュラ
    float p = saturate(dot(r_light, -eye_ray));
    float spec_b = 0;
    if (p > 0 && specularity > 0)
    {
        spec_b = pow(p, specularity);
    }
    
    // ディフューズ
    float diff_b = dot(-light, input.normal);
    float4 toon_col = toon.Sample(smpToon, float2(0, 1 - diff_b));
    
    // スフィアマップ用UV
    float2 sp_uv = (input.normal.xy * float2(1, -1) + float2(1, 1)) * 0.5;
    float4 sph_col = sph.Sample(smp, sp_uv);
    float4 spa_col = spa.Sample(smp, sp_uv);
    
    float4 tex_col = tex.Sample(smp, input.uv);
    
    float4 ret = float4((spa_col + sph_col * tex_col * toon_col * diffuse).rgb, diffuse.a) 
                + float4(specular * spec_b, 1);
    
    return ret;
}