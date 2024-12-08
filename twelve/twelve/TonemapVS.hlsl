//-----------------------------------------------------------------------------
// File : TonemapVS.hlsl
// Desc : Vertex Shader For Tonemap
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float2 Position : POSITION; // �ʒu���W�ł�.
    float2 TexCoord : TEXCOORD; // �e�N�X�`�����W�ł�.
};

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

//-----------------------------------------------------------------------------
//      ���C���G���g���[�|�C���g�ł�.
//-----------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.Position = float4(input.Position, 0.0f, 1.0f);
    output.TexCoord = input.TexCoord;

    return output;
}