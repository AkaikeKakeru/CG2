#include "Basic.hlsli"

VSOutput main( float4 pos : POSITION, float2 uv : TEXCOORD )
{
	VSOutput output; //ピクセルシェーダ―に渡す値

	////元々のコード
	//output.svpos = pos;

	output.svpos = mul(mat, pos); //座標に行列を乗算
	output.uv = uv;
	return output;
}