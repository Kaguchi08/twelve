#include "PMDHeader.hlsli"

float4 PSMain(PSIn psIn) : SV_TARGET
{
    // �f�B���N�V�������C�g�ɂ�郉�C�e�B���O���v�Z����
    float3 directionLig = CalcLigFromDirectionLight(psIn);
    
    // �X�t�B�A�}�b�v�pUV
    float2 spUV = (psIn.normalInView.xy * float2(1, -1) + float2(1, 1)) * 0.5;
    float4 sphCol = sph.Sample(smp, spUV);
    float4 spaCol = spa.Sample(smp, spUV);
    
    float4 finalColor = tex.Sample(smp, psIn.uv);
    float3 finalLig = directionLig + ambientLight;
    
    // �X�t�B�A�}�b�v
    finalColor *= sphCol;
    finalColor += spaCol;
    
    // ������Z����
    finalColor.xyz *= finalLig;
    
    float2 shadowMapUV = psIn.lightPos.xy / psIn.lightPos.w;
    shadowMapUV *= float2(0.5f, -0.5f);
    shadowMapUV += 0.5f;
    
    float zInLVP = psIn.lightPos.z / psIn.lightPos.w;
    float zInShadowMap = shadowMap.Sample(smp, shadowMapUV);
    
    if ((zInLVP - 0.001f) > zInShadowMap)
    {
        finalColor.xyz *= 0.5f;
    }
    
    return finalColor;
}

float3 CalcLambertDiffuse(float3 lightDirection, float3 lightColor, float4 normal)
{
    // �s�N�Z���̖@���ƃ��C�g�̕����̓��ς��v�Z����
    float t = dot(normal, lightDirection) * -1.0;
    
    // ���ς����̏ꍇ��0��Ԃ�
    t = max(t, 0.0);
    
     // �g�D�[���V�F�[�f�B���O
    float4 toonDiff = toon.Sample(smpToon, float2(0, 1 - t));
    
    // �g�U���ˌ����v�Z����
    return lightColor * toonDiff.xyz;
}

float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float4 worldPos, float4 normal)
{
    // ���˃x�N�g�������߂�
    float3 refVec = reflect(lightDirection, normal);

    // �������������T�[�t�F�C�X���王�_�ɐL�т�x�N�g�������߂�
    float3 toEye = eye - worldPos;
    toEye = normalize(toEye);

    // ���ʔ��˂̋��������߂�
    float t = dot(refVec, toEye);

    // ���ʔ��˂̋�����0�ȏ�̐��l�ɂ���
    t = max(0.0f, t);

    // ���ʔ��˂̋������i��
    if (specularity > 0)
    {
        t = pow(t, specularity);
    }

    // ���ʔ��ˌ������߂�
    return lightColor * t;
}

float3 CalcLigFromDirectionLight(PSIn psIn)
{
    float3 light = normalize(direction);
    
    // �f�B���N�V�������C�g�ɂ��Lambert�g�U���ˌ����v�Z����
    float3 diffDirection = CalcLambertDiffuse(light, color, psIn.normal);
    // �f�B�t���[�Y�F����Z����
    diffDirection *= diffuse.xyz;
    
    // �f�B���N�V�������C�g�ɂ��Phong���ʔ��ˌ����v�Z����
    float3 specDirection = CalcPhongSpecular(light, color, psIn.worldPos, psIn.normal);
    // �X�y�L�����F����Z����
    specDirection *= specular;
    
    return diffDirection + specDirection;
}