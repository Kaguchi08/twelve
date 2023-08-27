#include "PMDHeader.hlsli"

float4 PSMain(PSIn psIn) : SV_TARGET
{
    if (psIn.instNo > 0)
    {
        return float4(0, 0, 0, 1);
    }
   
    // ディレクションライトによるライティングを計算する
    float4 directionLig = CalcLigFromDirectionLight(psIn);
    
    // スフィアマップ用UV
    float2 spUV = (psIn.normalInView.xy * float2(1, -1) + float2(1, 1)) * 0.5;
    float4 sphCol = sph.Sample(smp, spUV);
    float4 spaCol = spa.Sample(smp, spUV);
    
    float4 finalColor = tex.Sample(smp, psIn.uv);
    float4 finalLig = directionLig;
    
    // スフィアマップ
    finalColor *= sphCol;
    finalColor += spaCol;
    
    // 光を乗算する
    finalColor *= finalLig;
    
    return finalColor;
}

float4 CalcLambertDiffuse(float3 lightDirection, float4 lightColor, float4 normal)
{
    // ピクセルの法線とライトの方向の内積を計算する
    float t = dot(normal, lightDirection) * -1.0;
    
    // 内積が負の場合は0を返す
    t = max(t, 0.0);
    
     // トゥーンシェーディング
    float4 toonDiff = toon.Sample(smpToon, float2(0, 1 - t));
    
    // 拡散反射光を計算する
    return lightColor * toonDiff;
}

float4 CalcPhongSpecular(float3 lightDirection, float4 lightColor, float4 worldPos, float4 normal)
{
    // 反射ベクトルを求める
    float3 refVec = reflect(lightDirection, normal);

    // 光が当たったサーフェイスから視点に伸びるベクトルを求める
    float3 toEye = eye - worldPos;
    toEye = normalize(toEye);

    // 鏡面反射の強さを求める
    float t = dot(refVec, toEye);

    // 鏡面反射の強さを0以上の数値にする
    t = max(0.0f, t);

    // 鏡面反射の強さを絞る
    if (specularity > 0)
    {
        t = pow(t, specularity);
    }

    // 鏡面反射光を求める
    return lightColor * t;
}

float4 CalcLigFromDirectionLight(PSIn psIn)
{
    float3 light = normalize(float3(1, -1, 1));
    float4 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    // ディレクションライトによるLambert拡散反射光を計算する
    float4 diffDirection = CalcLambertDiffuse(light, color, psIn.normal);
    // ディフューズ色を乗算する
    diffDirection *= diffuse;
    
    // ディレクションライトによるPhong鏡面反射光を計算する
    float4 specDirection = CalcPhongSpecular(light, color, psIn.worldPos, psIn.normal);
    // スペキュラ色を乗算する
    specDirection *= float4(specular, 1);
    
    return diffDirection + specDirection;
}