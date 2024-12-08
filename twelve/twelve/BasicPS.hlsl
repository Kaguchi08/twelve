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

cbuffer DirectionalLight : register(b0)
{
    float3 DirectionalLightColor : packoffset(c0);
    float DirectionalLightIntensity : packoffset(c0.w);
    float3 DirectionalLightForward : packoffset(c1);
};

cbuffer CbLight : register(b1)
{
    float3 LightPosition : packoffset(c0);
    float LightInvSqrRadius : packoffset(c0.w);
    float3 LightColor : packoffset(c1);
    float LightIntensity : packoffset(c1.w);
    float3 LightForward : packoffset(c2);
    float LightAngleScale : packoffset(c2.w);
    float LightAngleOffset : packoffset(c3);
    int LightType : packoffset(c3.y);
};

cbuffer CbCamera : register(b2)
{
    float3 CameraPosition : packoffset(c0); // �J�����ʒu�ł�.
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
    float squaredDistance, // ���C�g�ւ̋�����2��
    float invSqrAttRadius // ���C�g���a��2��̋t��
)
{
    float factor = squaredDistance * invSqrAttRadius;
    float smoothFactor = saturate(1.0f - factor * factor);
    return smoothFactor * smoothFactor;
}

float GetDistanceAttenuation
(
    float3 unnormalizedLightVector, // ���C�g�ʒu�ƃI�u�W�F�N�g�ʒu�̍����x�N�g��
    float invSqrAttRadius // ���C�g���a��2��̋t��
)
{
    float sqrDist = dot(unnormalizedLightVector, unnormalizedLightVector);
    float attenuation = 1.0f / (max(sqrDist, MIN_DIST * MIN_DIST));

    attenuation *= SmoothDistanceAttenuation(sqrDist, invSqrAttRadius);

    return attenuation;
}

float3 EvaluatePointLight
(
    float3 N, // �@���x�N�g��
    float3 worldPos, // ���[���h��Ԃ̃I�u�W�F�N�g�ʒu
    float3 lightPos, // ���C�g�̈ʒu
    float lightInvRadiusSq, // ���C�g���a��2��̋t��
    float3 lightColor // ���C�g�J���[
)
{
    float3 dif = lightPos - worldPos;
    float3 L = normalize(dif);
    float att = GetDistanceAttenuation(dif, lightInvRadiusSq);

    return saturate(dot(N, L)) * lightColor * att / 0.25f;
}

float GetAngleAttenuation
(
    float3 unnormalizedLightVector, // ���C�g�ʒu�ƃI�u�W�F�N�g�ʒu�̍����x�N�g��
    float3 lightDir, // ���K���ς݂̃��C�g�x�N�g��(���C�g�Ɍ���������)
    float lightAngleScale, // �X�|�b�g���C�g�̊p�x�����X�P�[��
    float lightAngleOffset // �X�|�b�g���C�g�̊p�x�I�t�Z�b�g
)
{
    float cd = dot(lightDir, unnormalizedLightVector);
    float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);
    
    // ���炩�ɕω�������
    attenuation *= attenuation;
    
    return attenuation;
}

float3 EvaluateSpotLight
(
    float3 N, // �@���x�N�g��
    float3 worldPos, // ���[���h��Ԃ̃I�u�W�F�N�g�ʒu
    float3 lightPos, // ���C�g�̈ʒu
    float lightInvRadiusSq, // ���C�g���a��2��̋t��
    float3 lightForward, // ���C�g�̑O���x�N�g��
    float3 lightColor, // ���C�g�J���[
    float lightAngleScale, // �X�|�b�g���C�g�̊p�x�����X�P�[��
    float lightAngleOffset // �X�|�b�g���C�g�̊p�x�I�t�Z�b�g
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
//      �s�N�Z���V�F�[�_�̃��C���G���g���[�|�C���g�ł�.
//-----------------------------------------------------------------------------
PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;
    
    float3 N = NormalMap.Sample(NormalSmp, input.TexCoord).xyz * 2.0f - 1.0f;
    float3 V = normalize(CameraPosition - input.WorldPos);
    N = mul(input.InvTangentBasis, N);

    float3 baseColor = BaseColorMap.Sample(BaseColorSmp, input.TexCoord).rgb;
    float metallic = MetallicMap.Sample(MetallicSmp, input.TexCoord).r;
    float roughness = RoughnessMap.Sample(RoughnessSmp, input.TexCoord).r;
    
    float3 Kd = baseColor * (1.0f - metallic);
    float3 diffuse = ComputeLambert(Kd);

    float3 Ks = baseColor * metallic;
    
    float NV = saturate(dot(N, V));
    
    float3 color = 0;
    
    {
        float3 L = normalize(LightPosition - input.WorldPos);
        float3 H = normalize(V + L);

        float NH = saturate(dot(N, H));
        float NL = saturate(dot(N, L));

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
                LightForward,
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
                LightForward,
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
                LightForward,
                LightColor,
                LightAngleScale,
                LightAngleOffset) * LightIntensity;
        }
        
        color += lit * BRDF;
    }
    
    
    // Directional Light
    {
        float3 L = normalize(DirectionalLightForward);
        float3 H = normalize(V + L);
        
        float NH = saturate(dot(N, H));
        float NL = saturate(dot(N, L));
        
        float3 specular = ComputeGGX(Ks, roughness, NH, NV, NL);

        float3 BRDF = (diffuse + specular);
        float3 lit = NL * DirectionalLightColor.rgb * DirectionalLightIntensity;
        
        color += lit * BRDF;
    }

    output.Color.rgb = color;
    output.Color.a = 1.0f;

    return output;
}