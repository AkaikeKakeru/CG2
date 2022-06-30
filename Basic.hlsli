//マテリアル
cbuffer ConstBufferDataMaterial : register(b0)
{
	//色(RGBA)
	float4 color;
}

//3D変換行列
cbuffer ConstBufferDataTransform : register(b1)
{
	matrix mat; //3D変換行列
}

//頂点シェーダ―の出力構造体
//(頂点シェーダ―からピクセルシェーダ―へのやり取りに使用する)
struct VSOutput
{
	//システム用頂点座標
	float4 svpos : SV_POSITION;

	//uv値
	float2 uv : TEXCOORD;
};