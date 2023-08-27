#include "PMDHeader.hlsli"

float4 PSMain(PSIn psIn) : SV_TARGET
{
    if (psIn.instNo > 0)
    {
        return float4(0, 0, 0, 1);
    }
   
    // �f�B���N�V�������C�g�ɂ�郉�C�e�B���O���v�Z����
    float4 directionLig = CalcLigFromDirectionLight(psIn);
    
    // �X�t�B�A�}�b�v�pUV
    float2 spUV = (psIn.normalInView.xy * float2(1, -1) + float2(1, 1)) * 0.5;
    float4 sphCol = sph.Sample(smp, spUV);
    float4 spaCol = spa.Sample(smp, spUV);
    
    float4 finalColor = tex.Sample(smp, psIn.uv);
    float4 finalLig = directionLig;
    
    // �X�t�B�A�}�b�v
    finalColor *= sphCol;
    finalColor += spaCol;
    
    // ������Z����
    finalColor *= finalLig;
    
    return finalColor;
}

float4 CalcLambertDiffuse(float3 lightDirection, float4 lightColor, float4 normal)
{
    // �s�N�Z���̖@���ƃ��C�g�̕����̓��ς��v�Z����
    float t = dot(normal, lightDirection) * -1.0;
    
    // ���ς����̏ꍇ��0��Ԃ�
    t = max(t, 0.0);
    
     // �g�D�[���V�F�[�f�B���O
    float4 toonDiff = toon.Sample(smpToon, float2(0, 1 - t));
    
    // �g�U���ˌ����v�Z����
    return lightColor * toonDiff;
}

float4 CalcPhongSpecular(float3 lightDirection, float4 lightColor, float4 worldPos, float4 normal)
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

float4 CalcLigFromDirectionLight(PSIn psIn)
{
    float3 light = normalize(float3(1, -1, 1));
    float4 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    // �f�B���N�V�������C�g�ɂ��Lambert�g�U���ˌ����v�Z����
    float4 diffDirection = CalcLambertDiffuse(light, color, psIn.normal);
    // �f�B�t���[�Y�F����Z����
    diffDirection *= diffuse;
    
    // �f�B���N�V�������C�g�ɂ��Phong���ʔ��ˌ����v�Z����
    float4 specDirection = CalcPhongSpecular(light, color, psIn.worldPos, psIn.normal);
    // �X�y�L�����F����Z����
    specDirection *= float4(specular, 1);
    
    return diffDirection + specDirection;
}