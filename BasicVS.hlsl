#include "Basic.hlsli"

VSOutput main( float4 pos : POSITION, float2 uv : TEXCOORD )
{
	VSOutput output; //ピクセルシェーダ―に渡す値
	output.svpos = pos;
	output.uv = uv;

	return output;
}