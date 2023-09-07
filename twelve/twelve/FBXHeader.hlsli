//Texture2D<float4> tex : register(t0);

SamplerState smp : register(s0);

cbuffer Scene : register(b0)
{
    matrix view;
    matrix proj;
    matrix shadow;
    float3 eye;
};

cbuffer Material : register(b1)
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
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