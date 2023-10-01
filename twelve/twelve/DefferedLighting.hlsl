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

// ベックマン分布を計算する
float Beckmann(float m, float t)
{
    float t2 = t * t;
    float t4 = t * t * t * t;
    float m2 = m * m;
    float D = 1.0f / (4.0f * m2 * t4);
    D *= exp((-1.0f / m2) * (1.0f - t2) / t2);
    return D;
}

// フレネルを計算。Schlick近似を使用
float SpcFresnel(float f0, float u)
{
    // from Schlick
    return f0 + (1 - f0) * pow(1 - u, 5);
}

/// <summary>
/// Cook-Torranceモデルの鏡面反射を計算
/// </summary>
/// <param name="L">光源に向かうベクトル</param>
/// <param name="V">視点に向かうベクトル</param>
/// <param name="N">法線ベクトル</param>
/// <param name="metallic">金属度</param>
float CookTorranceSpecular(float3 L, float3 V, float3 N, float metallic)
{
    float microfacet = 0.76f;

    // 金属度を垂直入射の時のフレネル反射率として扱う
    // 金属度が高いほどフレネル反射は大きくなる
    float f0 = metallic;

    // ライトに向かうベクトルと視線に向かうベクトルのハーフベクトルを求める
    float3 H = normalize(L + V);

    // 各種ベクトルがどれくらい似ているかを内積を利用して求める
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    // D項をベックマン分布を用いて計算する
    float D = Beckmann(microfacet, NdotH);

    // F項をSchlick近似を用いて計算する
    float F = SpcFresnel(f0, VdotH);

    // G項を求める
    float G = min(1.0f, min(2 * NdotH * NdotV / VdotH, 2 * NdotH * NdotL / VdotH));

    // m項を求める
    float m = PI * NdotV * NdotH;

    // ここまで求めた、値を利用して、Cook-Torranceモデルの鏡面反射を求める
    return max(F * D * G / m, 0.0);
}

/// <summary>
/// フレネル反射を考慮した拡散反射を計算
/// </summary>
/// <param name="N">法線</param>
/// <param name="L">光源に向かうベクトル。光の方向と逆向きのベクトル。</param>
/// <param name="V">視線に向かうベクトル。</param>
/// <param name="roughness">粗さ。0～1の範囲。</param>
float CalcDiffuseFromFresnel(float3 N, float3 L, float3 V, PSIn psIn)
{
    // 光源に向かうベクトルと視線に向かうベクトルのハーフベクトルを求める
    float3 H = normalize(L + V);

    // 粗さは0.5で固定。
    float roughness = armTex.Sample(smp, psIn.uv).g;

    float energyBias = lerp(0.0f, 0.5f, roughness);
    float energyFactor = lerp(1.0, 1.0 / 1.51, roughness);

    // 光源に向かうベクトルとハーフベクトルがどれだけ似ているかを内積で求める
    float dotLH = saturate(dot(L, H));

    // 光源に向かうベクトルとハーフベクトル、
    // 光が平行に入射したときの拡散反射量を求めている
    float Fd90 = energyBias + 2.0 * dotLH * dotLH * roughness;

    // 法線と光源に向かうベクトルwを利用して拡散反射率を求める
    float dotNL = saturate(dot(N, L));
    float FL = (1 + (Fd90 - 1) * pow(1 - dotNL, 5));

    // 法線と視点に向かうベクトルを利用して拡散反射率を求める
    float dotNV = saturate(dot(N, V));
    float FV = (1 + (Fd90 - 1) * pow(1 - dotNV, 5));

    // 法線と光源への方向に依存する拡散反射率と、法線と視点ベクトルに依存する拡散反射率を
    // 乗算して最終的な拡散反射率を求めている。PIで除算しているのは正規化を行うため
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
    // G-Bufferの内容を使ってライティング
    // アルベドカラー
    float4 albedoColor = albedoTex.Sample(smp, psIn.uv);
    
    // 法線
    float3 normal = normalTex.Sample(smp, psIn.uv).xyz;
    // 法線を0～1の範囲から-1～1の範囲に変換
    normal = normal * 2.0f - 1.0f;
    
    // ワールド座標
    float3 worldPos = worldPosTex.Sample(smp, psIn.uv).xyz;
    
    // スペキュラカラー
    float3 specularColor = albedoColor;
    
    // 金属度
    float metallic = armTex.Sample(smp, psIn.uv).b;
    
    // 滑らかさ
    float smoothness = 1 - armTex.Sample(smp, psIn.uv).g;
    
    // 視線に向かって伸びるベクトルを計算する
    float3 toEye = normalize(eye - worldPos);
    
    float3 lig = 0;
    
    // シンプルなディズニーベースの拡散反射を実装
    // フレネル反射を考慮した拡散反射を計算
    float diffuseFromFresnel = CalcDiffuseFromFresnel(normal, -direction, toEye, psIn);

    // 正規化Lambert拡散反射を求める
    float NdotL = saturate(dot(normal, -direction));
    float3 lambertDiffuse = color * NdotL / PI;

    // 最終的な拡散反射光を計算する
    float3 diffuse = albedoColor * diffuseFromFresnel * lambertDiffuse;

    // Cook-Torranceモデルを利用した鏡面反射率を計算する
    // Cook-Torranceモデルの鏡面反射率を計算する
    float3 spec = CookTorranceSpecular(-direction, toEye, normal, metallic) * color;

    // 金属度が高ければ、鏡面反射はスペキュラカラー、低ければ白
    // スペキュラカラーの強さを鏡面反射率として扱う
    spec *= lerp(float3(1.0f, 1.0f, 1.0f), specularColor, metallic);

    // 滑らかさを使って、拡散反射光と鏡面反射光を合成する
    // 滑らかさが高ければ、拡散反射は弱くなる
    lig += diffuse * (1.0f - smoothness) + spec;
    
    // 環境光を加算する
    lig += ambientLight * albedoColor;
    
    float4 finalColor = float4(lig, 1.0f);
    
    // ライトのビュー空間での座標を計算する
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
    
        // 影が落ちているときの色を計算する
        float3 shadowColor = finalColor.xyz * 0.5f;
    
        // 遮蔽率を使って線形補間
        finalColor.xyz = lerp(shadowColor, finalColor.xyz, shadow);
    }
    
    return finalColor;
}