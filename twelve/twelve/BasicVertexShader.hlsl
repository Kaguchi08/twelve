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
    matrix shadow;
    float3 eye;
};

cbuffer Transform : register(b1)
{
    matrix world;
    matrix bones[512];
};

cbuffer Material : register(b2)
{
    float4 diffuse;
    float3 specular;
    float specularity;
    float3 ambient;
};

BasicType BasicVS(float4 pos : POSITION,
                  float4 normal : NORMAL,
                  float2 uv : TEXCOORD,
                  min16int2 bone_no : BONENO,
                  min16int weight : WEIGHT,
                  uint inst_no : SV_InstanceID)
{
    BasicType output;
    float w = weight / 100.0f;
    matrix bm = bones[bone_no.x] * w + bones[bone_no.y] * (1.0f - w);
    
    pos = mul(bm, pos);
    pos = mul(world, pos);
    if (inst_no > 0)
    {
        pos = mul(shadow, pos);
    }
    output.svpos = mul(mul(proj, view), pos);
    output.pos = pos;
    normal.w = 0;
    output.normal = mul(world, normal);
    output.uv = uv;
    output.inst_no = inst_no;
    
    return output;
}