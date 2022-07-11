#include "Basic.hlsli"

Texture2D<float4> tex : register(t0); //0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); //0番スロットに設定されたサンプラー

float4 main(VSOutput input) : SV_TARGET
{
	//右下奥 向きのライト
	float3 light = normalize(float3(1,-1,1));

	//光源へのベクトルと法線ベクトルの内積
	float brightness = dot(-light, input.normal);



	//輝度をRGBに代入して出力
	return float4(brightness,brightness,brightness,1);


	//RGBをそれぞれの法線のXYZ、Aを1で出力
	//return float4(input.normal,1);
}