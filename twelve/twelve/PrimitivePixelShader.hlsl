#include "PrimitiveHeader.hlsli"

float4 PSMain(PSIn psIn) : SV_TARGET
{
    float3 normal = psIn.normal;
    
    // �@���}�b�v����^���W�F���g�x�[�X�̖@�����擾����
    float3 localNormal = normalMap.Sample(smp, psIn.uv).xyz;
    
    // �^���W�F���g�X�y�[�X�̖@����0�`1�͈̔͂���-1�`1�͈̔͂ɕ�������
    localNormal = (localNormal - 0.5f) * 2.0f;

    // �^���W�F���g�X�y�[�X�̖@�������[���h�X�y�[�X�ɕϊ�����
    normal = psIn.tangent * localNormal.x + psIn.biNormal * localNormal.y + normal * localNormal.z;
    
    // �f�B���N�V�������C�g�ɂ�郉�C�e�B���O���v�Z����
    float3 directionLig = CalcLigFromDirectionLight(psIn, normal);
    
    float4 finalColor = tex.Sample(smp, psIn.uv);
    float3 finalLig = directionLig + ambientLight;
    
    // ������Z����
    finalColor.xyz *= finalLig;
    
    return finalColor;
}

float3 CalcLambertDiffuse(float3 lightDirection, float3 lightColor, float3 normal)
{
    // �s�N�Z���̖@���ƃ��C�g�̕����̓��ς��v�Z����
    float t = dot(normal, lightDirection) * -1.0;
    
    // ���ς����̏ꍇ��0��Ԃ�
    t = max(t, 0.0);
    
    // �g�U���ˌ����v�Z����
    return lightColor * t;
}

float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float3 worldPos, float3 normal)
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
    t = pow(t, 5);

    // ���ʔ��ˌ������߂�
    return lightColor * t;
}

float3 CalcLigFromDirectionLight(PSIn psIn, float3 normal)
{
    float3 light = normalize(direction);
    
    // �f�B���N�V�������C�g�ɂ��Lambert�g�U���ˌ����v�Z����
    float3 diffDirection = CalcLambertDiffuse(light, color, normal);
    
    // �f�B���N�V�������C�g�ɂ��Phong���ʔ��ˌ����v�Z����
    float3 specDirection = CalcPhongSpecular(light, color, psIn.worldPos, normal);
    
    return diffDirection + specDirection;
}