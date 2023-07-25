//Texture2D<float4> tex : register(t0);

SamplerState smp : register(s0);

cbuffer SceneData : register(b0)
{
    matrix view;
    matrix proj;
    matrix light_view;
    matrix shadow;
    float3 eye;
};

struct VSIn
{
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VSOut
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};