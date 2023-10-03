SamplerState smp : register(s0);
SamplerComparisonState smpShadow : register(s1);

Texture2D<float4> shadowMap : register(t0);
Texture2D<float4> tex : register(t1);
Texture2D<float4> normalMap : register(t2);
Texture2D<float4> armMap : register(t3);

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

cbuffer Light : register(b3)
{
    float3 direction;
    float3 color;
    float3 ambientLight;
};

struct VSIn
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    float3 biNormal : BINORMAL;
};

struct PSIn
{
    float4 svpos : SV_POSITION;
    float4 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    float3 biNormal : BINORMAL;
    float4 lightPos : LIGHTPOS;
};

struct PSOut
{
    float4 albedo : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 worldPos : SV_TARGET2;
    float4 arm : SV_TARGET3;
    float4 shadow : SV_TARGET4;
};