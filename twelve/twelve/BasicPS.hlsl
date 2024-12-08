#include "BRDF.hlsli"

#ifndef MIN_DIST
#define MIN_DIST (0.01)
#endif//MIN_DIST

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 WorldPos : WORLD_POS;
    float3x3 InvTangentBasis : INV_TANGENT_BASIS;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
};

cbuffer CbLight : register(b1)
{
    float3 LightPosition : packoffset(c0);
    float LightInvSqrRadius : packoffset(c0.w);
    float3 LightColor : packoffset(c1);
    float LightIntensity : packoffset(c1.w);
    float3 LightFoward : packoffset(c2);
    float LightAngleScale : packoffset(c2.w);
    float LightAngleOffset : packoffset(c3);
    int LightType : packoffset(c3.y);
};

cbuffer CbCamera : register(b2)
{
    float3 CameraPosition : packoffset(c0); // カメラ位置です.
}

Texture2D BaseColorMap : register(t0);
SamplerState BaseColorSmp : register(s0);

Texture2D MetallicMap : register(t1);
SamplerState MetallicSmp : register(s1);

Texture2D RoughnessMap : register(t2);
SamplerState RoughnessSmp : register(s2);

Texture2D NormalMap : register(t3);
SamplerState NormalSmp : register(s3);


float SmoothDistanceAttenuation
(
    float squaredDistance, // ライトへの距離の2乗
    float invSqrAttRadius // ライト半径の2乗の逆数
)
{
    float factor = squaredDistance * invSqrAttRadius;
    float smoothFactor = saturate(1.0f - factor * factor);
    return smoothFactor * smoothFactor;
}

float GetDistanceAttenuation
(
    float3 unnormalizedLightVector, // ライト位置とオブジェクト位置の差分ベクトル
    float invSqrAttRadius // ライト半径の2乗の逆数
)
{
    float sqrDist = dot(unnormalizedLightVector, unnormalizedLightVector);
    float attenuation = 1.0f / (max(sqrDist, MIN_DIST * MIN_DIST));

    attenuation *= SmoothDistanceAttenuation(sqrDist, invSqrAttRadius);

    return attenuation;
}

float3 EvaluatePointLight
(
    float3 N, // 法線ベクトル
    float3 worldPos, // ワールド空間のオブジェクト位置
    float3 lightPos, // ライトの位置
    float lightInvRadiusSq, // ライト半径の2乗の逆数
    float3 lightColor // ライトカラー
)
{
    float3 dif = lightPos - worldPos;
    float3 L = normalize(dif);
    float att = GetDistanceAttenuation(dif, lightInvRadiusSq);

    return saturate(dot(N, L)) * lightColor * att / 0.25f;
}

float GetAngleAttenuation
(
    float3 unnormalizedLightVector, // ライト位置とオブジェクト位置の差分ベクトル
    float3 lightDir, // 正規化済みのライトベクトル(ライトに向かう方向)
    float lightAngleScale, // スポットライトの角度減衰スケール
    float lightAngleOffset // スポットライトの角度オフセット
)
{
    float cd = dot(lightDir, unnormalizedLightVector);
    float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);
    
    // 滑らかに変化させる
    attenuation *= attenuation;
    
    return attenuation;
}

float3 EvaluateSpotLight
(
    float3 N, // 法線ベクトル
    float3 worldPos, // ワールド空間のオブジェクト位置
    float3 lightPos, // ライトの位置
    float lightInvRadiusSq, // ライト半径の2乗の逆数
    float3 lightForward, // ライトの前方ベクトル
    float3 lightColor, // ライトカラー
    float lightAngleScale, // スポットライトの角度減衰スケール
    float lightAngleOffset // スポットライトの角度オフセット
)
{
    float3 unnormalizedLightVector = lightPos - worldPos;
    float3 L = normalize(unnormalizedLightVector);
    float sqrDist = dot(unnormalizedLightVector, unnormalizedLightVector);
    float att = 1.0f / (max(sqrDist, MIN_DIST * MIN_DIST));
    att *= GetAngleAttenuation(-L, lightForward, lightAngleScale, lightAngleOffset);

    return saturate(dot(N, L)) * lightColor * att / F_PI;
}

float3 EvaluateSpotLightKaris
(
    float3 N,
    float3 worldPos,
    float3 lightPos,
    float lightInvRadiusSq,
    float3 lightForward,
    float3 lightColor,
    float lightAngleScale,
    float lightAngleOffset
)
{
    float3 unnormalizedLightVector = lightPos - worldPos;
    float3 L = normalize(unnormalizedLightVector);
    float sqrDist = dot(unnormalizedLightVector, unnormalizedLightVector);
    float att = 1.0f;
    att *= SmoothDistanceAttenuation(sqrDist, lightInvRadiusSq);
    att /= (sqrDist + 1.0f);
    att *= GetAngleAttenuation(-L, lightForward, lightAngleScale, lightAngleOffset);

    return saturate(dot(N, L)) * lightColor * att / F_PI;
}

float3 EvaluateSpotLightLagarde
(
    float3 N,
    float3 worldPos,
    float3 lightPos,
    float lightInvRadiusSq,
    float3 lightForward,
    float3 lightColor,
    float lightAngleScale,
    float lightAngleOffset
)
{
    float3 unnormalizedLightVector = lightPos - worldPos;
    float3 L = normalize(unnormalizedLightVector);
    float att = 1.0f;
    att *= GetDistanceAttenuation(unnormalizedLightVector, lightInvRadiusSq);
    att *= GetAngleAttenuation(-L, lightForward, lightAngleScale, lightAngleOffset);

    return saturate(dot(N, L)) * lightColor * att / F_PI;
}

//-----------------------------------------------------------------------------
//      ピクセルシェーダのメインエントリーポイントです.
//-----------------------------------------------------------------------------
PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;

    float3 L = normalize(LightPosition - input.WorldPos);
    float3 V = normalize(CameraPosition - input.WorldPos);
    float3 H = normalize(V + L);
    float3 N = NormalMap.Sample(NormalSmp, input.TexCoord).xyz * 2.0f - 1.0f;
    N = mul(input.InvTangentBasis, N);

    float NV = saturate(dot(N, V));
    float NH = saturate(dot(N, H));
    float NL = saturate(dot(N, L));

    float3 baseColor = BaseColorMap.Sample(BaseColorSmp, input.TexCoord).rgb;
    float metallic = MetallicMap.Sample(MetallicSmp, input.TexCoord).r;
    float roughness = RoughnessMap.Sample(RoughnessSmp, input.TexCoord).r;

    float3 Kd = baseColor * (1.0f - metallic);
    float3 diffuse = ComputeLambert(Kd);

    float3 Ks = baseColor * metallic;
    float3 specular = ComputeGGX(Ks, roughness, NH, NV, NL);

    float3 BRDF = (diffuse + specular);
    float3 lit = 0;
    
    if (LightType == 0)
    {
        lit = EvaluateSpotLight(
                N,
                input.WorldPos,
                LightPosition,
                LightInvSqrRadius,
                LightFoward,
                LightColor,
                LightAngleScale,
                LightAngleOffset) * LightIntensity;
    }
    else if (LightType == 1)
    {
        lit = EvaluateSpotLightKaris(
                N,
                input.WorldPos,
                LightPosition,
                LightInvSqrRadius,
                LightFoward,
                LightColor,
                LightAngleScale,
                LightAngleOffset) * LightIntensity;
    }
    else
    {
        lit = EvaluateSpotLightLagarde(
                N,
                input.WorldPos,
                LightPosition,
                LightInvSqrRadius,
                LightFoward,
                LightColor,
                LightAngleScale,
                LightAngleOffset) * LightIntensity;
    }

    output.Color.rgb = lit * BRDF;
    output.Color.a = 1.0f;

    return output;
}
