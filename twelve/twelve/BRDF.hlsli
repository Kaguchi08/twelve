//-----------------------------------------------------------------------------
// File : BRDF.hlsli
// Desc : Bidirectional Reflectance Distribution Function.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#ifndef BRDF_HLSLI
#define BRDF_HLSLI

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
#ifndef F_PI
#define F_PI        3.14159265358979323f   // 円周率.
#endif//F_PI


//-----------------------------------------------------------------------------
//      Schlickによるフレネル項の近似式.
//-----------------------------------------------------------------------------
float3 SchlickFresnel(float3 specular, float VH)
{
    return specular + (1.0f - specular) * pow((1.0f - VH), 5.0f);
}

//-----------------------------------------------------------------------------
//      GGXによる法線分布関数.
//-----------------------------------------------------------------------------
float D_GGX(float a, float NH)
{
    float a2 = a * a;
    float f = (NH * NH) * (a2 - 1) + 1;
    return a2 / (F_PI * f * f);
}

//-----------------------------------------------------------------------------
//      Height Correlated Smithによる幾何減衰項.
//-----------------------------------------------------------------------------
float G2_Smith(float NL, float NV, float a)
{
    float a2 = a * a;
    
    float epsilon = max(1e-6f, a2 * 1e-6f);
    float NL2 = max(NL * NL, epsilon);
    float NV2 = max(NV * NV, epsilon);

    float Lambda_V = (-1.0f + sqrt(a2 * (1.0f - NL2) / NL2 + 1.0f)) * 0.5f;
    float Lambda_L = (-1.0f + sqrt(a2 * (1.0f - NV2) / NV2 + 1.0f)) * 0.5f;
    return 1.0f / (1.0f + Lambda_V + Lambda_V);
}

//-----------------------------------------------------------------------------
//      Lambert BRDFを計算します.
//-----------------------------------------------------------------------------
float3 ComputeLambert(float3 Kd)
{
    return Kd / F_PI;
}

//-----------------------------------------------------------------------------
//      Phong BRDFを計算します.
//-----------------------------------------------------------------------------
float3 ComputePhong
(
    float3 Ks,
    float shininess,
    float LdotR
)
{
    return Ks * ((shininess + 2.0f) / (2.0f * F_PI)) * pow(LdotR, shininess);
}

//-----------------------------------------------------------------------------
//      GGX BRDFを計算します.
//-----------------------------------------------------------------------------
float3 ComputeGGX
(
    float3 Ks,
    float roughness,
    float NdotH,
    float NdotV,
    float NdotL
)
{
    // 分母がゼロの場合を回避するためのチェック
    float epsilon = 1e-6f;
    if (NdotV < epsilon || NdotL < epsilon)
    {
        return float3(0.0f, 0.0f, 0.0f); // BRDFはゼロ
    }
    
    float a = roughness * roughness;
    float D = D_GGX(a, NdotH);
    float G = G2_Smith(NdotL, NdotV, a);
    float3 F = SchlickFresnel(Ks, NdotL);

    return (D * G * F) / (4.0f * NdotV * NdotL);
}

#endif//BRDF_HLSLI
