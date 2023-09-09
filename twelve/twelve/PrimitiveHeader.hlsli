Texture2D<float4> tex : register(t0);
Texture2D<float4> normalMap : register(t1);
Texture2D<float4> armMap : register(t2);
Texture2D<float4> shadowMap : register(t3);

SamplerState smp : register(s0);

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

cbuffer Light : register(b2)
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
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 worldPos : TEXCOORD1;
    float3 tangent : TANGENT;
    float3 biNormal : BINORMAL;
    float4 lightPos : LIGHTPOS;
};

float3 CalcLambertDiffuse(float3 lightDirection, float3 lightColor, float3 normal);
float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float4 worldPos, float3 normal);
float3 CalcLigFromDirectionLight(PSIn psIn, float3 normal);