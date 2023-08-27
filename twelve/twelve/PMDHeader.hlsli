Texture2D<float4> tex : register(t0);
Texture2D<float4> sph : register(t1);
Texture2D<float4> spa : register(t2);
Texture2D<float4> toon : register(t3);

SamplerState smp : register(s0);
SamplerState smpToon : register(s1);

cbuffer Scene : register(b0)
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

struct VSIn
{
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
    min16int2 boneNo : BONENO;
    min16int weight : WEIGHT;
    uint isntNo : SV_InstanceID;
};

struct PSIn
{
    float4 svPos : SV_POSITION;
    float4 worldPos : POSITION;
    float4 normal : NORMAL0;
    float4 normalInView : NORMAL1; // �J������Ԃ̖@��
    float2 uv : TEXCOORD;
    uint instNo : SV_InstanceID;
};

float4 CalcLambertDiffuse(float3 lightDirection, float4 lightColor, float4 normal);
float4 CalcPhongSpecular(float3 lightDirection, float4 lightColor, float4 worldPos, float4 normal);
float4 CalcLigFromDirectionLight(PSIn psIn);