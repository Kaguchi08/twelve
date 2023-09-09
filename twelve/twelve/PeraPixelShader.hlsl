#include "Type.hlsli"

cbuffer Weight : register(b0)
{
    float4 bk_weights[2];
};

Texture2D<float4> tex : register(t0);
Texture2D<float4> effect_tex : register(t1);
Texture2D<float> depth_tex : register(t2);

SamplerState smp : register(s0);

float4 PeraPS(PeraType input) : SV_TARGET
{
    //float depth = depth_tex.Sample(smp, input.uv).r;
    
    //depth = pow(depth, 20);
    
    //return float4(depth, depth, depth, 1);
    
    float4 col = tex.Sample(smp, input.uv);
    
    return col;
  
    //float Y = dot(col.rgb, float3(0.299, 0.587, 0.114));
    
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    
    float dx = 1.0f / w;
    float dy = 1.0f / h;
    float4 ret = float4(0, 0, 0, 0);
    
    
    //今のピクセルを中心に縦横5つずつになるよう加算する
	//最上段
    ret += tex.Sample(smp, input.uv + float2(-2 * dx, 2 * dy)) * 1 / 256;
    ret += tex.Sample(smp, input.uv + float2(-1 * dx, 2 * dy)) * 4 / 256;
    ret += tex.Sample(smp, input.uv + float2(0 * dx, 2 * dy)) * 6 / 256;
    ret += tex.Sample(smp, input.uv + float2(1 * dx, 2 * dy)) * 4 / 256;
    ret += tex.Sample(smp, input.uv + float2(2 * dx, 2 * dy)) * 1 / 256;
	//ひとつ上段
    ret += tex.Sample(smp, input.uv + float2(-2 * dx, 1 * dy)) * 4 / 256;
    ret += tex.Sample(smp, input.uv + float2(-1 * dx, 1 * dy)) * 16 / 256;
    ret += tex.Sample(smp, input.uv + float2(0 * dx, 1 * dy)) * 24 / 256;
    ret += tex.Sample(smp, input.uv + float2(1 * dx, 1 * dy)) * 16 / 256;
    ret += tex.Sample(smp, input.uv + float2(2 * dx, 1 * dy)) * 4 / 256;
	//中心列
    ret += tex.Sample(smp, input.uv + float2(-2 * dx, 0 * dy)) * 6 / 256;
    ret += tex.Sample(smp, input.uv + float2(-1 * dx, 0 * dy)) * 24 / 256;
    ret += tex.Sample(smp, input.uv + float2(0 * dx, 0 * dy)) * 36 / 256;
    ret += tex.Sample(smp, input.uv + float2(1 * dx, 0 * dy)) * 24 / 256;
    ret += tex.Sample(smp, input.uv + float2(2 * dx, 0 * dy)) * 6 / 256;
	//一つ下段
    ret += tex.Sample(smp, input.uv + float2(-2 * dx, -1 * dy)) * 4 / 256;
    ret += tex.Sample(smp, input.uv + float2(-1 * dx, -1 * dy)) * 16 / 256;
    ret += tex.Sample(smp, input.uv + float2(0 * dx, -1 * dy)) * 24 / 256;
    ret += tex.Sample(smp, input.uv + float2(1 * dx, -1 * dy)) * 16 / 256;
    ret += tex.Sample(smp, input.uv + float2(2 * dx, -1 * dy)) * 4 / 256;
	//最下段
    ret += tex.Sample(smp, input.uv + float2(-2 * dx, -2 * dy)) * 1 / 256;
    ret += tex.Sample(smp, input.uv + float2(-1 * dx, -2 * dy)) * 4 / 256;
    ret += tex.Sample(smp, input.uv + float2(0 * dx, -2 * dy)) * 6 / 256;
    ret += tex.Sample(smp, input.uv + float2(1 * dx, -2 * dy)) * 4 / 256;
    ret += tex.Sample(smp, input.uv + float2(2 * dx, -2 * dy)) * 1 / 256;
    
    ret = float4(0, 0, 0, 0);
    ret += bk_weights[0] * col;
    for (int i = 1; i < 8; ++i)
    {
        ret += bk_weights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(i * dx, 0));
        ret += bk_weights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(-i * dx, 0));
    }
    return float4(ret.rgb, col.a);

    //return ret;
}

float4 VerticalBokehPS(PeraType input) : SV_TARGET
{
    return tex.Sample(smp, input.uv);
    
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1.0f / w;
    float dy = 1.0f / h;
    float4 ret = float4(0, 0, 0, 0);
    
    float2 nmTex = effect_tex.Sample(smp, input.uv).xy;
    nmTex = nmTex * 2.0f - 1.0f;

    return tex.Sample(smp, input.uv + nmTex * 0.1f);

    float4 col = tex.Sample(smp, input.uv);
    ret += bk_weights[0] * col;
    for (int i = 1; i < 8; ++i)
    {
        ret += bk_weights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, dy * i));
        ret += bk_weights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, -dy * i));
    }
    return float4(ret.rgb, col.a);
}