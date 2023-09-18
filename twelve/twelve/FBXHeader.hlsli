SamplerState smp : register(s0);
SamplerComparisonState smpShadow : register(s1);

Texture2D<float4> shadowMap : register(t0);
Texture2D<float4> tex : register(t1);

cbuffer Scene : register(b0)
{
    matrix view;
    matrix proj;
    matrix lightView;
    matrix shadow;
    float3 eye;
};

cbuffer Material : register(b1)
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
};

cbuffer Shadow : register(b2)
{
    matrix world;
};

struct VSIn
{
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct PSIn
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};