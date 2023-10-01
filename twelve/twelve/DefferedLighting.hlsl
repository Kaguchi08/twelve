SamplerState smp : register(s0);
SamplerComparisonState smpShadow : register(s1);

Texture2D<float4> albedoTex : register(t0);
Texture2D<float4> normalTex : register(t1);
Texture2D<float4> worldPosTex : register(t2);
Texture2D<float4> armTex : register(t3);
Texture2D<float4> shadowMap : register(t4);

cbuffer Scene : register(b0)
{
    matrix view;
    matrix proj;
    matrix lightView;
    matrix shadow;
    float3 eye;
};

cbuffer Light : register(b1)
{
    float3 direction;
    float3 color;
    float3 ambientLight;
};

struct VSIn
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct PSIn
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

static const float PI = 3.1415926f;

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
    float roughness = armTex.Sample(smp, psIn.uv).g;

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

PSIn VSMain(VSIn vsIn)
{
    PSIn psIn;
    psIn.svpos = vsIn.pos;
    psIn.uv = vsIn.uv;
    return psIn;
}

float4 PSMain(PSIn psIn) : SV_TARGET
{
    // G-Buffer�̓��e���g���ă��C�e�B���O
    // �A���x�h�J���[
    float4 albedoColor = albedoTex.Sample(smp, psIn.uv);
    
    // �@��
    float3 normal = normalTex.Sample(smp, psIn.uv).xyz;
    // �@����0�`1�͈̔͂���-1�`1�͈̔͂ɕϊ�
    normal = normal * 2.0f - 1.0f;
    
    // ���[���h���W
    float3 worldPos = worldPosTex.Sample(smp, psIn.uv).xyz;
    
    // �X�y�L�����J���[
    float3 specularColor = albedoColor;
    
    // �����x
    float metallic = armTex.Sample(smp, psIn.uv).b;
    
    // ���炩��
    float smoothness = 1 - armTex.Sample(smp, psIn.uv).g;
    
    // �����Ɍ������ĐL�т�x�N�g�����v�Z����
    float3 toEye = normalize(eye - worldPos);
    
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
    float3 spec = CookTorranceSpecular(-direction, toEye, normal, metallic) * color;

    // �����x��������΁A���ʔ��˂̓X�y�L�����J���[�A�Ⴏ��Δ�
    // �X�y�L�����J���[�̋��������ʔ��˗��Ƃ��Ĉ���
    spec *= lerp(float3(1.0f, 1.0f, 1.0f), specularColor, metallic);

    // ���炩�����g���āA�g�U���ˌ��Ƌ��ʔ��ˌ�����������
    // ���炩����������΁A�g�U���˂͎キ�Ȃ�
    lig += diffuse * (1.0f - smoothness) + spec;
    
    // ���������Z����
    lig += ambientLight * albedoColor;
    
    float4 finalColor = float4(lig, 1.0f);
    
    // ���C�g�̃r���[��Ԃł̍��W���v�Z����
    float4 posInLVP = mul(lightView, float4(worldPos, 1.0f));

    float2 shadowMapUV = posInLVP.xy / posInLVP.w;
    shadowMapUV *= float2(0.5f, -0.5f);
    shadowMapUV += 0.5f;
    
    float zInLVP = posInLVP.z / posInLVP.w;
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
    
    return finalColor;
}