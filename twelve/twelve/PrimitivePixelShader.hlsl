#include "PrimitiveHeader.hlsli"

static const float PI = 3.1415926f;

float3 GetNormal(float3 normal, float3 tangent, float3 biNormal, float2 uv)
{
    float3 binSpaceNormal = normalMap.SampleLevel(smp, uv, 0.0f).xyz;
    binSpaceNormal = (binSpaceNormal * 2.0f) - 1.0f;

    float3 newNormal = tangent * binSpaceNormal.x + biNormal * binSpaceNormal.y + normal * binSpaceNormal.z;
    
    return newNormal;
}

// �x�b�N�}�����z���v�Z����
float Beckmann(float m, float t)
{
    float t2 = t * t;
    float t4 = t * t * t * t;
    float m2 = m * m;
    float D = 1.0f / (4.0f * m2 * t4);
    D *= exp((-1.0f / m2) * (1.0f - t2) / t2);
    return D;
}

// �t���l�����v�Z�BSchlick�ߎ����g�p
float SpcFresnel(float f0, float u)
{
    // from Schlick
    return f0 + (1 - f0) * pow(1 - u, 5);
}

/// <summary>
/// Cook-Torrance���f���̋��ʔ��˂��v�Z
/// </summary>
/// <param name="L">�����Ɍ������x�N�g��</param>
/// <param name="V">���_�Ɍ������x�N�g��</param>
/// <param name="N">�@���x�N�g��</param>
/// <param name="metallic">�����x</param>
float CookTorranceSpecular(float3 L, float3 V, float3 N, float metallic)
{
    float microfacet = 0.76f;

    // �����x�𐂒����˂̎��̃t���l�����˗��Ƃ��Ĉ���
    // �����x�������قǃt���l�����˂͑傫���Ȃ�
    float f0 = metallic;

    // ���C�g�Ɍ������x�N�g���Ǝ����Ɍ������x�N�g���̃n�[�t�x�N�g�������߂�
    float3 H = normalize(L + V);

    // �e��x�N�g�����ǂꂭ�炢���Ă��邩����ς𗘗p���ċ��߂�
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    // D�����x�b�N�}�����z��p���Čv�Z����
    float D = Beckmann(microfacet, NdotH);

    // F����Schlick�ߎ���p���Čv�Z����
    float F = SpcFresnel(f0, VdotH);

    // G�������߂�
    float G = min(1.0f, min(2 * NdotH * NdotV / VdotH, 2 * NdotH * NdotL / VdotH));

    // m�������߂�
    float m = PI * NdotV * NdotH;

    // �����܂ŋ��߂��A�l�𗘗p���āACook-Torrance���f���̋��ʔ��˂����߂�
    return max(F * D * G / m, 0.0);
}

/// <summary>
/// �t���l�����˂��l�������g�U���˂��v�Z
/// </summary>
/// <param name="N">�@��</param>
/// <param name="L">�����Ɍ������x�N�g���B���̕����Ƌt�����̃x�N�g���B</param>
/// <param name="V">�����Ɍ������x�N�g���B</param>
/// <param name="roughness">�e���B0�`1�͈̔́B</param>
float CalcDiffuseFromFresnel(float3 N, float3 L, float3 V, PSIn psIn)
{
    // �����Ɍ������x�N�g���Ǝ����Ɍ������x�N�g���̃n�[�t�x�N�g�������߂�
    float3 H = normalize(L + V);

    // �e����0.5�ŌŒ�B
    float roughness = armMap.Sample(smp, psIn.uv).g;

    float energyBias = lerp(0.0f, 0.5f, roughness);
    float energyFactor = lerp(1.0, 1.0 / 1.51, roughness);

    // �����Ɍ������x�N�g���ƃn�[�t�x�N�g�����ǂꂾ�����Ă��邩����ςŋ��߂�
    float dotLH = saturate(dot(L, H));

    // �����Ɍ������x�N�g���ƃn�[�t�x�N�g���A
    // �������s�ɓ��˂����Ƃ��̊g�U���˗ʂ����߂Ă���
    float Fd90 = energyBias + 2.0 * dotLH * dotLH * roughness;

    // �@���ƌ����Ɍ������x�N�g��w�𗘗p���Ċg�U���˗������߂�
    float dotNL = saturate(dot(N, L));
    float FL = (1 + (Fd90 - 1) * pow(1 - dotNL, 5));

    // �@���Ǝ��_�Ɍ������x�N�g���𗘗p���Ċg�U���˗������߂�
    float dotNV = saturate(dot(N, V));
    float FV = (1 + (Fd90 - 1) * pow(1 - dotNV, 5));

    // �@���ƌ����ւ̕����Ɉˑ�����g�U���˗��ƁA�@���Ǝ��_�x�N�g���Ɉˑ�����g�U���˗���
    // ��Z���čŏI�I�Ȋg�U���˗������߂Ă���BPI�ŏ��Z���Ă���̂͐��K�����s������
    return (FL * FV * energyFactor);
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

float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float4 worldPos, float3 normal)
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


float4 PSMain(PSIn psIn) : SV_TARGET
{
    // �@�����v�Z
    float3 normal = GetNormal(psIn.normal, psIn.tangent, psIn.biNormal, psIn.uv);
    
    // �A���x�h�J���[
    float4 albedoColor = tex.Sample(smp, psIn.uv);
    
    // �X�y�L�����J���[
    float3 specColor = albedoColor;
    
    // �����x
    float metallic = armMap.Sample(smp, psIn.uv).b;
    
    // ���炩��
    float smooth = 1 - armMap.Sample(smp, psIn.uv).g;
    
    // �����Ɍ������ĐL�т�x�N�g�����v�Z����
    float3 toEye = normalize(eye - psIn.worldPos);
    
    float3 lig = 0;

    // �V���v���ȃf�B�Y�j�[�x�[�X�̊g�U���˂�����
    // �t���l�����˂��l�������g�U���˂��v�Z
    float diffuseFromFresnel = CalcDiffuseFromFresnel(normal, -direction, toEye, psIn);

    // ���K��Lambert�g�U���˂����߂�
    float NdotL = saturate(dot(normal, -direction));
    float3 lambertDiffuse = color * NdotL / PI;

    // �ŏI�I�Ȋg�U���ˌ����v�Z����
    float3 diffuse = albedoColor * diffuseFromFresnel * lambertDiffuse;

    // Cook-Torrance���f���𗘗p�������ʔ��˗����v�Z����
    // Cook-Torrance���f���̋��ʔ��˗����v�Z����
    float3 spec = CookTorranceSpecular(-direction, toEye, normal, smooth) * color;

    // �����x��������΁A���ʔ��˂̓X�y�L�����J���[�A�Ⴏ��Δ�
    // �X�y�L�����J���[�̋��������ʔ��˗��Ƃ��Ĉ���
    spec *= lerp(float3(1.0f, 1.0f, 1.0f), specColor, metallic);

    // ���炩�����g���āA�g�U���ˌ��Ƌ��ʔ��ˌ�����������
    // ���炩����������΁A�g�U���˂͎キ�Ȃ�
    lig += diffuse * (1.0f - smooth) + spec;
    
    // ���������Z����
    lig += ambientLight * albedoColor;
    
    float4 finalColor = float4(lig, 1.0f);
    
    float2 shadowMapUV = psIn.lightPos.xy / psIn.lightPos.w;
    shadowMapUV *= float2(0.5f, -0.5f);
    shadowMapUV += 0.5f;
    
    float zInLVP = psIn.lightPos.z / psIn.lightPos.w;
    //float zInShadowMap = shadowMap.Sample(smp, shadowMapUV);
    
    if (shadowMapUV.x > 0.0f && shadowMapUV.x < 1.0f
        && shadowMapUV.y > 0.0f && shadowMapUV.y < 1.0f)
    {
        float shadow = shadowMap.SampleCmpLevelZero(smpShadow, shadowMapUV, zInLVP - 0.001f);
    
        // �e�������Ă���Ƃ��̐F���v�Z����
        float3 shadowColor = finalColor.xyz * 0.5f;
    
        // �Օ������g���Đ��`���
        finalColor.xyz = lerp(shadowColor, finalColor.xyz, shadow);
    }

    
    // �ȉ��͖@���}�b�v�𗘗p�������C�e�B���O�̌v�Z
    
    //float3 normal = psIn.normal;
    
    // �@���}�b�v����^���W�F���g�x�[�X�̖@�����擾����
    //float3 localNormal = normalMap.Sample(smp, psIn.uv).xyz;
    
    // �^���W�F���g�X�y�[�X�̖@����0�`1�͈̔͂���-1�`1�͈̔͂ɕ�������
    //localNormal = (localNormal - 0.5f) * 2.0f;

    // �^���W�F���g�X�y�[�X�̖@�������[���h�X�y�[�X�ɕϊ�����
    //normal = psIn.tangent * localNormal.x + psIn.biNormal * localNormal.y + normal * localNormal.z;
    
    // �f�B���N�V�������C�g�ɂ�郉�C�e�B���O���v�Z����
    //float3 directionLig = CalcLigFromDirectionLight(psIn, normal);
    
    //float4 finalColor = tex.Sample(smp, psIn.uv);
    //float3 finalLig = directionLig + ambientLight;
    
    // ������Z����
    //finalColor.xyz *= finalLig;
    
    return finalColor;
}

PSOut PSGBuffer(PSIn psIn)
{
    PSOut psOut;
    
    // �A���x�h�J���[���o��
    psOut.albedo = tex.Sample(smp, psIn.uv);
    
    // �@�����o��
    psOut.normal = float4(GetNormal(psIn.normal, psIn.tangent, psIn.biNormal, psIn.uv), 1.0f);
    // �@���͈̔͂�-1�`1����0�`1�ɕϊ�
    psOut.normal.xyz = psOut.normal.xyz * 0.5f + 0.5f;
    
    // ARM�}�b�v���o��
    psOut.arm = armMap.Sample(smp, psIn.uv);
    
    // ���[���h���W���o��
    psOut.worldPos = psIn.worldPos;
    
    // �e���o��
    psOut.shadow = 1.0f;
    
    return psOut;
}

