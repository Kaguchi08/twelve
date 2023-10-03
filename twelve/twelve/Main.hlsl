SamplerState smp : register(s0);

Texture2D<float4> tex : register(t0);

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


PSIn VSMain(VSIn vsIn)
{
    PSIn psIn;
    psIn.svpos = vsIn.pos;
    psIn.uv = vsIn.uv;
    return psIn;
}

float4 PSMain(PSIn psIn) : SV_TARGET
{
    float4 col = tex.Sample(smp, psIn.uv);
    
    return col;
}