#include "Basic.hlsli"

Texture2D<float4> tex : register(t0); //0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); //0番スロットに設定されたサンプラー

float4 main(VSOutput input) : SV_TARGET
{

	float scroll = 0.001f;
	float2 defo = input.uv;
	
	input.uv.x += scroll;
	
	if (input.uv.x > 8.0f)
	{
		input.uv.x = defo.x;
	}

	return float4(tex.Sample(smp,input.uv));

}