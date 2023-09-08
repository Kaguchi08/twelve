#include "PrimitiveHeader.hlsli"

float4 PSMain(PSIn psIn) : SV_TARGET
{
    float3 normal = psIn.normal;
    
    // 法線マップからタンジェントベースの法線を取得する
    float3 localNormal = normalMap.Sample(smp, psIn.uv).xyz;
    
    // タンジェントスペースの法線を0～1の範囲から-1～1の範囲に復元する
    localNormal = (localNormal - 0.5f) * 2.0f;

    // タンジェントスペースの法線をワールドスペースに変換する
    normal = psIn.tangent * localNormal.x + psIn.biNormal * localNormal.y + normal * localNormal.z;
    
    // ディレクションライトによるライティングを計算する
    float3 directionLig = CalcLigFromDirectionLight(psIn, normal);
    
    float4 finalColor = tex.Sample(smp, psIn.uv);
    float3 finalLig = directionLig + ambientLight;
    
    // 光を乗算する
    finalColor.xyz *= finalLig;
    
    return finalColor;
}

float3 CalcLambertDiffuse(float3 lightDirection, float3 lightColor, float3 normal)
{
    // ピクセルの法線とライトの方向の内積を計算する
    float t = dot(normal, lightDirection) * -1.0;
    
    // 内積が負の場合は0を返す
    t = max(t, 0.0);
    
    // 拡散反射光を計算する
    return lightColor * t;
}

float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float3 worldPos, float3 normal)
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
    t = pow(t, 5);

    // 鏡面反射光を求める
    return lightColor * t;
}

float3 CalcLigFromDirectionLight(PSIn psIn, float3 normal)
{
    float3 light = normalize(direction);
    
    // ディレクションライトによるLambert拡散反射光を計算する
    float3 diffDirection = CalcLambertDiffuse(light, color, normal);
    
    // ディレクションライトによるPhong鏡面反射光を計算する
    float3 specDirection = CalcPhongSpecular(light, color, psIn.worldPos, normal);
    
    return diffDirection + specDirection;
}