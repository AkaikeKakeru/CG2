#include "Basic.hlsli"

Texture2D<float4> tex : register(t0); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp : register(s0); //0�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[

float4 main(VSOutput input) : SV_TARGET
{
	//�E���� �����̃��C�g
	float3 light = normalize(float3(1,-1,1));

	//diffuse��[0,1]�͈̔͂�Clamp����
	//�����ւ̃x�N�g���Ɩ@���x�N�g���̓���
	float diffuse = saturate(dot(-light, input.normal));

	//�A���r�G���g����0.3�Ƃ��Čv�Z
	float brightness = diffuse + 0.3f;


	//�P�x��RGB�ɑ�����ďo��
	return float4(brightness,brightness,brightness,1);


	//RGB�����ꂼ��̖@����XYZ�AA��1�ŏo��
	//return float4(input.normal,1);
}