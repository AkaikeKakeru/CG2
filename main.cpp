#define DIRECTINPUT_VERSION		0x0800 //DirectInputのバージョン指定

#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXTex.h>


#include <vector>
#include <string>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>

#include <d3dcompiler.h>//シェーダ用コンパイラ

#include <dinput.h>

#include "Struct.h"
#include "Object.h"


#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

#pragma comment(lib, "d3dcompiler.lib")//シェーダ用コンパイラ

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

using namespace DirectX;

//ウィンドウプロシージャ
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		//ウィンドウ破棄されたなら
	case WM_DESTROY:
		//OSに対してアプリ終了を通知
		PostQuitMessage(0);
		return 0;
	}

	//メッセージ処理
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	//------WindowsAPI初期化処理 ここから------
	//サイズ
	const int window_width = 1280;
	const int window_height = 720;

	const float PI = 3.1415926535f;



	//クラス設定
	WNDCLASSEX w{};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProc;
	w.lpszClassName = L"DirectXGame";
	w.hInstance = GetModuleHandle(nullptr);
	w.hCursor = LoadCursor(NULL, IDC_ARROW);

	//OSに登録
	RegisterClassEx(&w);
	//サイズ
	RECT wrc = { 0,0,window_width,window_height };
	//自動でサイズ修正
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindow(w.lpszClassName,
		L"DirectXGame",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		w.hInstance,
		nullptr);

	//表示状態にする
	ShowWindow(hwnd, SW_SHOW);

	MSG msg{};//メッセージ
	//------WindowsAPI初期化処理 ここまで------

	//------DirectX初期化処理 ここから------
#ifdef _DEBUG
			  //デバッグプレイヤーをオンに
	ID3D12Debug* debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
	}
#endif

	HRESULT result;
	ID3D12Device* device = nullptr;
	IDXGIFactory7* dxgiFactry = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	ID3D12CommandAllocator* commandAllocator = nullptr;
	ID3D12GraphicsCommandList* commandList = nullptr;
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12DescriptorHeap* rtvHeap = nullptr;

	//DXGIファクトリー生成
	result = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactry));
	assert(SUCCEEDED(result));

	//アダプターの列挙用
	std::vector<IDXGIAdapter4*>adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter4* tmpAdapter = nullptr;

	//パフォーマンスが高いものから順に、すべてのアダプターを列挙
	for (UINT i = 0;
		dxgiFactry->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND
		; i++)
	{
		//動的配列に追加
		adapters.push_back(tmpAdapter);
	}

	//妥当なアダプタを選別
	for (size_t i = 0; i < adapters.size(); i++) {
		DXGI_ADAPTER_DESC3 adapterDesc;
		//アダプターの情報を取得
		adapters[i]->GetDesc3(&adapterDesc);

		//ソフトウェアデバイスを回避
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//デバイスを採用してループを抜ける
			tmpAdapter = adapters[i];
			break;
		}
	}

	//対応レベルの配列
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D_FEATURE_LEVEL featureLevel;

	for (size_t i = 0; i < _countof(levels); i++) {
		//採用したアダプターでデバイスを生成
		result = D3D12CreateDevice(tmpAdapter, levels[i],
			IID_PPV_ARGS(&device));
		if (result == S_OK) {
			//デバイスを生成できた時点でループを抜ける
			featureLevel = levels[i];
			break;
		}
	}

	//コマンドアロケータを生成
	result = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(result));

	//コマンドリストを生成
	result = device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator, nullptr,
		IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(result));

	//コマンドキューを設定
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	//コマンドキューを生成
	result = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(result));

	//スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = 1280;
	swapChainDesc.Height = 720;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	
	//スワップチェーンの生成
	result = dxgiFactry->CreateSwapChainForHwnd(
		commandQueue, hwnd, &swapChainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)&swapChain);
	assert(SUCCEEDED(result));

	//デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount;
	//デスクリプタヒープの生成
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

	//バックバッファ
	std::vector<ID3D12Resource*> backBuffers;
	backBuffers.resize(swapChainDesc.BufferCount);


	//スワップチェーンのすべてのバッファについて処理する
	for (size_t i = 0; i < backBuffers.size(); i++) {
		//スワップチェーンからバッファを取得
		swapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
		//デスクリプタヒープのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		//裏か表化でアドレスがズレる
		rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		//レンダ―ターゲットビューの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		//シェーダーの計算結果をSRGBに変換して書き込む
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		//レンダ―ターゲットビューの生成
		device->CreateRenderTargetView(backBuffers[i], &rtvDesc, rtvHandle);
	}

	//フェンスの生成
	ID3D12Fence* fence = nullptr;
	UINT64 fenceVal = 0;

	result = device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	//DirectInputの初期化
	IDirectInput8* directInput = nullptr;
	result = DirectInput8Create(
		w.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	//キーボードデバイスの生成
	IDirectInputDevice8* keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	//入力データの形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);//標準形式
	assert(SUCCEEDED(result));

	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

	//------DirectX初期化処理 ここまで------

	//------描画初期化処理 ここから------
	
	//頂点データ構造体
	struct Vertex
	{
		XMFLOAT3 pos; //xyz座標
		XMFLOAT2 uv;  //uv座標
	};

	//Object* object = new Object();
	//Object* objectArr[10] = new Object();

	/*for (int i = 0; i < 10; i++)
	{
		objectArr[i]->transform_.Trans_.x = object.transform_.Trans_.x;
		objectArr[i]->transform_.Trans_.y = object.transform_.Trans_.y;
		objectArr[i]->transform_.Trans_.z = object.transform_.Trans_.z;
	}*/

	//頂点データ
	Vertex vertices[] =
	{
		//x		 y		z		u	  v
		{{-50.0f, -50.0f, 0.0f}, {0.0f, 1.0f}},//左下
		{{-50.0f,  50.0f, 0.0f}, {0.0f, 0.0f}},//左上
		{{ 50.0f, -50.0f, 0.0f}, {1.0f, 1.0f}},//右下
		{{ 50.0f,  50.0f, 0.0f}, {1.0f, 0.0f}},//右上
	};

	//Vertex vertices[] =
	//{
	//	//x		 y		z						u	  v
	//	{{  -object->transform_.direction_.x, 
	//		-object->transform_.direction_.y, 
	//		 object->transform_.direction_.z}, {0.0f, 1.0f}},//左下

	//	{{  -object->transform_.direction_.x, 
	//		+object->transform_.direction_.y, 
	//		 object->transform_.direction_.z}, {0.0f, 0.0f}},//左上

	//	{{  +object->transform_.direction_.x, 
	//		-object->transform_.direction_.y, 
	//		 object->transform_.direction_.z}, {1.0f, 1.0f}},//右下

	//	{{  +object->transform_.direction_.x, 
	//		+object->transform_.direction_.y, 
	//		 object->transform_.direction_.z}, {1.0f, 0.0f}},//右上
	//};


	//インデックスデータ
	unsigned short indices[] =
	{
		0,1,2,//一つ目
		1,2,3,//二つ目
		2,3,0,
		3,0,1,
	};

	//Transform transform_ =
	//{

	//	{
	//		0.0f,0.0f
	//	},

	//		{
	//			0.0f,
	//		},

	//		{
	//			1.0f,
	//		},

	//};

	//アフィン
	float affine[3][3] =
	{
		{1.0f,0.0f,0.0f},
		{0.0f,1.0f,0.0f},
		{0.0f,0.0f,1.0f},
	};


	//頂点データ全体のサイズ = 頂点データ一つ分のサイズ * 頂点データの要素数
	UINT sizeVB = static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices));

	//頂点バッファの設定
	//ヒープ設定
	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;//GPUの転送用
	//リソース設定
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB;//頂点データ全体のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//頂点バッファの生成
	ID3D12Resource* vertBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp,//ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));

	//GPU上のバッファに対応仮想メモリ(メインメモリ上)を取得
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));

;



	/* verticesに記入 */

	//全頂点に対して
	for (int i = 0; i < _countof(vertices); i++)
	{
		vertMap[i] = vertices[i];//座標をコピー
	}

	//繋がりを解除
	vertBuff->Unmap(0, nullptr);

	//頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	//GPU仮想アドレス
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	//頂点バッファのサイズ
	vbView.SizeInBytes = sizeVB;
	//頂点１つ分のデータサイズ
	vbView.StrideInBytes = sizeof(vertices[0]);

	ID3DBlob* vsBlob = nullptr;//頂点シェーダオブジェクト
	ID3DBlob* psBlob = nullptr;//ピクセルシェーダオブジェクト
	ID3DBlob* errorBlob = nullptr;//エラーオブジェクト

	//頂点シェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"BasicVS.hlsl",//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルード可能にする
		"main", "vs_5_0",//エントリーポイント名、シェーダ―モデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用設定
		0,
		&vsBlob, &errorBlob);

	//エラーなら
	if (FAILED(result)) {
		//errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	//ピクセルシェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"BasicPS.hlsl",//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルード可能にする
		"main", "ps_5_0",//エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用設定
		0,
		&psBlob, &errorBlob);

	//エラーなら
	if (FAILED(result)) {
		//errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = 
	{
		{
			//xyz座標
			"POSITION",									//セマンティック名
			0,											//同じセマンティック名が複数あるときに使うインデックス(0でよい)
			DXGI_FORMAT_R32G32B32_FLOAT,				//要素数とビット数を表す　(XYZの3つでfloat型なのでR32G32B32_FLOAT
			0,											//入力スロットインデックス(0でよい)
			D3D12_APPEND_ALIGNED_ELEMENT,				//データのオフセット値　(D3D12_APPEND_ALIGNED_ELEMENTだと自動設定)
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	//入力データ種別　(標準はD3D12_INPUT_CLASSIFICATION_PER_VERTEX_DAT
			0											//一度に描画するインスタンス数(0でよい)
		},//(1行で書いた方が見やすいかも)

		//座標以外に 色、テクスチャUIなどを渡す場合はさらに続ける
		{
			//UV座標
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0,
		},
		//{/*...*/},
	};

	//グラフィックスパイプライン設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};

	//シェーダーの設定
	pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

	//サンプルマスクの設定
	pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//標準設定
	//pipelineDesc.SampleMask = UINT_MAX;

	//ラスタライザの設定
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//ポリゴン内塗りつぶし
	pipelineDesc.RasterizerState.DepthClipEnable = true;//深度クリッピングを有効に

	////ブレンドステート
	//pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask 
	//	= D3D12_COLOR_WRITE_ENABLE_ALL;//RGB全てのチャネルを描画

	//レンダ―ターゲットのブレンド設定
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//アルファ値共通設定
	blenddesc.BlendEnable = false; // ブレンド有効にする
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD; //ブレンドを有効にする
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE; //加算
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO; //デストの値を 0%使う　
 
	//加算合成
	//blenddesc.BlendOp = D3D12_BLEND_OP_ADD; //加算
	//blenddesc.SrcBlend = D3D12_BLEND_ONE; //ソースの値を100%使う
	//blenddesc.DestBlend = D3D12_BLEND_ONE; //デストの値を100%使う

	//減算合成
	//blenddesc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; //減算
	//blenddesc.SrcBlend = D3D12_BLEND_ONE; //ソースの値を100%使う
	//blenddesc.DestBlend = D3D12_BLEND_ONE; //デストの値を100%使う

	 //色反転
	 //blenddesc.BlendOp = D3D12_BLEND_OP_ADD; //加算
	 //blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR; //1.0f-デストから－の値
	 //blenddesc.DestBlend = D3D12_BLEND_ZERO; //使わない

	 //半透明合成
	 blenddesc.BlendOp = D3D12_BLEND_OP_ADD; //加算
	 blenddesc.SrcBlend = D3D12_BLEND_ONE; //ソースの値をアルファ値
	 blenddesc.DestBlend = D3D12_BLEND_ONE; //1.0f-ソースのアルファ値

	 //頂点レイアウトの設定
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);

	//図形の形状設定
	pipelineDesc.PrimitiveTopologyType
		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//その他の設定
	pipelineDesc.NumRenderTargets = 1;//描画対象は1つ
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//0～255指定のRGBA
	pipelineDesc.SampleDesc.Count = 1;//1ピクセルにつき1回サンプリング


	//ルートシグネチャ
	ID3D12RootSignature* rootSignature;

	//デスクリプタレンジの設定
	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.NumDescriptors = 1;//一度の描画に使うテクスチャが一枚なので1
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.BaseShaderRegister = 0;//テクスチャレジスタ番号0番
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//OK//

	//デスクリプタテーブルの設定
	D3D12_DESCRIPTOR_RANGE descRange{};
	descRange.NumDescriptors = 1;//定数は一つ
	descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; //種別は定数
	descRange.BaseShaderRegister = 0; //0番スロットから
	descRange.OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	////ルートパラメータの設定
	D3D12_ROOT_PARAMETER rootParams[3] = {};
	//定数バッファ0番
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//定数バッファビュー
	rootParams[0].Descriptor.ShaderRegister = 0;					//定数バッファ番号
	rootParams[0].Descriptor.RegisterSpace = 0;						//デフォルト値
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	//全てのシェーダから見える
	
	//テクスチャレジスタ0番
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	//定数バッファビュー
	rootParams[1].DescriptorTable.pDescriptorRanges = &descriptorRange;					//定数バッファ番号
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;						//デフォルト値
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	//全てのシェーダから見える

	//定数バッファ1番
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//種類
	rootParams[2].Descriptor.ShaderRegister = 1;					//定数バッファ番号
	rootParams[2].Descriptor.RegisterSpace = 0;						//デフォルト値
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	//全てのシェーダから見える

	//OK//

	//テクスチャサンプラーの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParams; //ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = _countof(rootParams); //ルートパラメータ数
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	//ルートシグネチャのシリアライズ
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(result));
	rootSigBlob->Release();
	//パイプラインにルートシグネイチャをセット
	pipelineDesc.pRootSignature = rootSignature;

	//パイプラインステートの生成
	ID3D12PipelineState* pipelineState = nullptr;
	result = device->CreateGraphicsPipelineState(&pipelineDesc, 
		IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(result));

	//定数バッファ用データ構造体(マテリアル)
	struct ConstBufferDataMaterial 
	{
		XMFLOAT4 color; //色(RGBA)
	};

	//資料05-02で追加
#pragma region 3D変換行列
	//定数バッファ用データ構造体(3D変換行列)
	struct ConstBufferDataTransform {
		XMMATRIX mat; //3D変換行列
	};


	ID3D12Resource* constBuffTransform = nullptr;
	ConstBufferDataTransform* constMapTransform = nullptr;

	

#pragma endregion

//#pragma region constMapMaterial関連
//	
//	//ヒープ設定
//	D3D12_HEAP_PROPERTIES cbHeapProp{};
//	cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // GPUへの転送用
//	//リソース設定
//	D3D12_RESOURCE_DESC cbResourceDesc{};
//	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//	cbResourceDesc.Width = (sizeof(ConstBufferDataMaterial ) + 0xff) & ~0xff; //256バイトアラインメント
//	cbResourceDesc.Height = 1;
//	cbResourceDesc.DepthOrArraySize = 1;
//	cbResourceDesc.MipLevels = 1;
//	cbResourceDesc.SampleDesc.Count = 1;
//	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//	
//	ID3D12Resource* constBuffMaterial = nullptr;
//	//定数バッファの生成
//	result = device->CreateCommittedResource(
//		&cbHeapProp, //ヒープ設定
//		D3D12_HEAP_FLAG_NONE,
//		&cbResourceDesc, //リソース設定
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(&constBuffMaterial));
//	assert(SUCCEEDED(result));
//
//	//定数バッファのマッピング
//	ConstBufferDataMaterial* constMapMaterial = nullptr;
//	result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial); //マッピング
//	assert(SUCCEEDED(result));
//	
//#pragma endregion

#pragma region constMapTransfrom関連
	

		//ヒープ設定
		D3D12_HEAP_PROPERTIES cbHeapProp{};
		cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // GPUへの転送用
		 //リソース設定
		D3D12_RESOURCE_DESC cbResourceDesc{};
		cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		cbResourceDesc.Width = (sizeof(ConstBufferDataTransform) + 0xff) & ~0xff; //256バイトアラインメント
		cbResourceDesc.Height = 1;
		cbResourceDesc.DepthOrArraySize = 1;
		cbResourceDesc.MipLevels = 1;
		cbResourceDesc.SampleDesc.Count = 1;
		cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;



		//ID3D12Resource* constBuffTransform = nullptr;
		//定数バッファの生成
		result = device->CreateCommittedResource(
			&cbHeapProp, //ヒープ設定
			D3D12_HEAP_FLAG_NONE,
			&cbResourceDesc, //リソース設定
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constBuffTransform));
		assert(SUCCEEDED(result));

		//定数バッファのマッピング
		result = constBuffTransform->Map(0, nullptr, (void**)&constMapTransform); //マッピング
		assert(SUCCEEDED(result));

		//単位行列を代入
		constMapTransform->mat = XMMatrixIdentity();

#pragma region 単位行列で埋めた後
#pragma region 平行投影行列の計算

		//DirectXMathで用意されている関数に置き換え
		 constMapTransform->mat = XMMatrixOrthographicOffCenterLH(
			 0.0f, window_width,//左端、右端
			  window_height, 0.0f,//下端、上端
			 0.0f, 1.0f);//前端、奥端
#pragma endregion
#pragma region 投資投影変換行列の計算

		 XMMATRIX matProjection =
			 XMMatrixPerspectiveFovLH(
			 XMConvertToRadians(45.0f),//上下画角45度
			 (float)window_width / window_height,//アスペクト比(画面横幅/画面縦幅)
			 0.1f, 1000.0f
		 );//前端、奥端

#pragma region ビュー行列の作成
		 XMMATRIX matView;
		 XMFLOAT3 eye(100, -80, -100);	//視点座標
		 XMFLOAT3 target(0, 0, 0);	//注視点座標
		 XMFLOAT3 up(0, 1, 0);		//上方向ベクトル
		 matView = XMMatrixLookAtLH(XMLoadFloat3(&eye),
			 XMLoadFloat3(&target), XMLoadFloat3(&up));

#pragma endregion


		 constMapTransform->mat = matView * matProjection;
#pragma endregion

#pragma endregion

	// 値を書き込むと自動的に転送される
	//constMapMaterial->color = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.5f); //RGBAで半透明の赤

	
#pragma endregion



	// インデックスデータ全体のサイズ
	UINT sizeIB = static_cast<UINT>(sizeof(uint16_t) * _countof(indices));

	// リソース設定
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeIB; // インデックス情報が入る分のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//インデックスバッファの生成
	ID3D12Resource* indexBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff));

	//　インデックスバッファをマッピング
	uint16_t* indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);
	// 全インデックスに対して
	for (int i = 0; i < _countof(indices); i++)
	{
		indexMap[i] = indices[i]; //インデックスをコピー
	}
	//マッピング解除
	indexBuff->Unmap(0, nullptr);

	//インデックスバッファビューの作成
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;


	TexMetadata metadata{};
	ScratchImage scratchImg{};

	//WICテクスチャのロード
	result = LoadFromWICFile(
		L"Resources/texture.png",
		WIC_FLAGS_NONE,
		&metadata, scratchImg
	);

	ScratchImage mipChine{};
	//ミップマップ生成
	result = GenerateMipMaps(
		scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChine);
	if (SUCCEEDED(result)) {
		scratchImg = std::move(mipChine);
		metadata = scratchImg.GetMetadata();
	}

	//読み込んだでデイヒューズテクスチャをSRGBとして扱う
	metadata.format = MakeSRGB(metadata.format);


	//ヒープ設定
	D3D12_HEAP_PROPERTIES textureHeapProp{};
	textureHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	textureHeapProp.CPUPageProperty =
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	textureHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	//リソース設定
	D3D12_RESOURCE_DESC textureResourceDesc{};
	textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureResourceDesc.Format = metadata.format;
	textureResourceDesc.Width = metadata.width; //幅
	textureResourceDesc.Height = (UINT)metadata.height; //高さ
	textureResourceDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
	textureResourceDesc.MipLevels = (UINT16)metadata.mipLevels;
	textureResourceDesc.SampleDesc.Count = 1;


	//テクスチャバッファの生成
	ID3D12Resource* texBuff = nullptr;
	result = device->CreateCommittedResource(
		&textureHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	//全ミップマップについて
	for (size_t i = 0; i < metadata.mipLevels; i++)
	{
		//ミップマップレベルを指定してイメージを取得
		const Image* img = scratchImg.GetImage(i, 0, 0);
		//テクスチャバッファにデータ転送
		result = texBuff->WriteToSubresource(
			(UINT)i,
			nullptr,//全領域へコピー
			img->pixels,//元データアドレス
			(UINT)img->rowPitch,//1ラインサイズ
			(UINT)img->slicePitch//一枚サイズ
		);
		assert(SUCCEEDED(result));
	}

	//元データ開放
	//delete[] imageData;


	//SRVの最大個数
	const size_t kMaxSRVCount = 2056;
	
	//デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
	srvHeapDesc.NumDescriptors = kMaxSRVCount;

	//設定を基にSRV用デスクリプタヒープを生成
	ID3D12DescriptorHeap* srvHeap = nullptr;
	result = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
	assert(SUCCEEDED(result));


	//SRVヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();


	//シェーダリソースビュー設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};//設定構造体
	srvDesc.Format = resDesc.Format;//RGBA float
	srvDesc.Shader4ComponentMapping =
	D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

	//ハンドルの指す位置にシェーダーリソースビュー作成
	device->CreateShaderResourceView(texBuff, &srvDesc, srvHandle);
	
	//OK//


	////CBV,SRV,UAVの1個分のサイズを取得
	//UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	////SRVヒープの先頭ハンドルを取得
	//D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
	////ハンドルを一つ進める(SRVの位置)
	//srvHandle.ptr += descriptorSize * 1;

	////CBV(コンスタントバッファビュー)の設定
	//D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	////cbvDescの値設定(省略)
	//device->CreateConstantBufferView(&cbvDesc, srvHandle);

	//------描画初期化処理 ここまで------

	//ゲームループ
	while (true) {

		//バックバッファの番号を取得(0番か1番)
		UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

		//1.リソースバリアで書き込みに変更
		D3D12_RESOURCE_BARRIER barrierDesc{};
		barrierDesc.Transition.pResource = backBuffers[bbIndex];
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrierDesc);

		//2.描画先の変更
		//レンダ―ターゲットビューのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIndex * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		//3.画面クリア          R      G      B     A
		FLOAT clearColor[] = { 0.1f, 0.25f, 0.5f, 0.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);


		//キーボード情報の取得開始
		keyboard->Acquire();
		//全キーの入力状態を取得する
		BYTE key[256] = {};
		keyboard->GetDeviceState(sizeof(key), key);

#pragma region transform関連

		//transform_.Trans_.x = 0.0f;
		//transform_.Trans_.y = 0.0f;

		//transform_.Rota = 0.0f;

		//transform_.Scale = 1.0f;


		//if(key[DIK_D])
		//{
		//	transform_.Trans_.x += 0.05f;
		//}
		//if(key[DIK_A])
		//{
		//	transform_.Trans_.x -= 0.05f;
		//}

		//if(key[DIK_W])
		//{
		//	transform_.Trans_.y += 0.05f;
		//}
		//if(key[DIK_S])
		//{
		//	transform_.Trans_.y -= 0.05f;
		//}

		//if(key[DIK_Q])
		//{
		//	transform_.Rota -= PI / 32;
		//}
		//if(key[DIK_E])
		//{
		//	transform_.Rota += PI / 32;
		//}

		//if(key[DIK_Z])
		//{
		//	transform_.Scale -= 0.1f;
		//}
		//if(key[DIK_C])
		//{
		//	transform_.Scale += 0.1f;
		//}

		//	affine[0][0] = transform_.Scale * cos(transform_.Rota);
		//	affine[0][1] = transform_.Scale * ( - sin(transform_.Rota));
		//	affine[0][2] = transform_.Trans_.x;

		//	affine[1][0] = transform_.Scale * sin(transform_.Rota);
		//	affine[1][1] = transform_.Scale * cos(transform_.Rota);
		//	affine[1][2] = transform_.Trans_.y;

		//	affine[2][0] = 0.0f;
		//	affine[2][1] = 0.0f;
		//	affine[2][2] = 1.0f;

		//for (int i = 0; i < _countof(vertices); i++)
		//{
		//	vertices[i].x = vertices[i].x * affine[0][0] +
		//					vertices[i].y * affine[0][1] +
		//							 1.0f * affine[0][2];

		//	vertices[i].y = vertices[i].x * affine[1][0] +
		//					vertices[i].y * affine[1][1] +
		//							 1.0f * affine[1][2];

		//	vertices[i].z = vertices[i].x * affine[2][0] +
		//					vertices[i].y * affine[2][1] +
		//							 1.0f * affine[2][2];
		//}
#pragma endregion

#pragma ターゲットの周りを回るカメラ

		static float angle = 0.0f; //カメラの回転角

		if(key[DIK_D] || key[DIK_A])
		{
			if(key[DIK_D]) { angle += XMConvertToRadians(1.0f);}
			else if(key[DIK_A]) { angle -= XMConvertToRadians(1.0f);}
		
			//angleラジアンだけY軸周りに回転、半径は-100
			eye.x = -100 * sinf(angle);
			eye.z = -100 * cosf(angle);
			matView = XMMatrixLookAtLH(XMLoadFloat3(&eye),
				XMLoadFloat3(&target), XMLoadFloat3(&up));

			constMapTransform->mat = matView * matProjection;
		}
#pragma endregion

		//全頂点に対して
		for (int i = 0; i < _countof(vertices); i++)
		{
			vertMap[i] = vertices[i];//座標をコピー
		}

		//4.描画コマンドここから

		//ビューポート設定コマンド
		D3D12_VIEWPORT viewport{};
		viewport.Width = window_width;				//横幅
		viewport.Height = window_height;			//縦幅
		viewport.TopLeftX = 0;		//左上x
		viewport.TopLeftY = 0;						//左上y
		viewport.MinDepth = 0.0f;					//最小深度(0でよい)
		viewport.MaxDepth = 1.0f;					//最大深度(1でよい)
		//ビューポート設定コマンドを、コマンドリストに積む
		commandList->RSSetViewports(1, &viewport);

		//シザー矩形
		D3D12_RECT scissorRect{};							//切り抜き座標
		scissorRect.left = 0;								//左
		scissorRect.right = scissorRect.left + window_width;//右
		scissorRect.top = 0;								//上
		scissorRect.bottom = scissorRect.top + window_height;//下

		//シザー矩形設定コマンドを、コマンドリストに積む
		commandList->RSSetScissorRects(1, &scissorRect);

		//パイプラインステートとルートシグネチャの設定コマンド
		commandList->SetPipelineState(pipelineState);
		commandList->SetGraphicsRootSignature(rootSignature);

		//プリミティブ形状の設定コマンド
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);//三角形リスト

		//頂点バッファビューの設定コマンド
		commandList->IASetVertexBuffers(0, 1, &vbView);

		//定数バッファビュー(CBV)の設定コマンド
		//commandList->SetGraphicsRootConstantBufferView(0, constBuffMaterial->GetGPUVirtualAddress());

		//SRVヒープの設定コマンド
		commandList->SetDescriptorHeaps(1, &srvHeap);

		//SRVヒープの先頭ハンドルを取得(SRVを指しているはず)
		D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
		//SRVヒープの先頭にあるSRVをルートパラメータ1番に設定
		commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);
		//定数バッファビュー(CBV)の設定コマンド
		commandList->SetGraphicsRootConstantBufferView(2, constBuffTransform->GetGPUVirtualAddress());


		//インデックスバッファビューの設定コマンド
		commandList->IASetIndexBuffer(&ibView);

		//描画コマンド
		//commandList->DrawInstanced(_countof(vertices), 1, 0, 0);//全ての頂点を使って描画
		//commandList->DrawInstanced(6, 1, 0, 0);//全ての頂点を使って描画
		commandList->DrawIndexedInstanced(_countof(indices),1,0,0,0);//全ての頂点を使って描画

		//4.ここまで、描画コマンド

		//5.リソースバリアを戻す
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList->ResourceBarrier(1, &barrierDesc);


		//命令のクローズ
		result = commandList->Close();
		assert(SUCCEEDED(result));

		//コマンドリストの実行
		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(1, commandLists);

		//画面に表示するバッファをフリップ(裏表の入れ替え)
		result = swapChain->Present(1, 0);
		result = device->GetDeviceRemovedReason();
		assert(SUCCEEDED(result));

		//コマンドの実行完了を待つ
		commandQueue->Signal(fence, ++fenceVal);
		if (fence->GetCompletedValue() != fenceVal) {
			HANDLE event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}

		//キューをクリア
		result = commandAllocator->Reset();
		assert(SUCCEEDED(result));
		//	再びコマンドリストを貯める準備
		result = commandList->Reset(commandAllocator, nullptr);
		assert(SUCCEEDED(result));


		//メッセージはあるか？
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//×で終了メッセージが来たらループを抜ける
		if (msg.message == WM_QUIT) {
			break;
		}

	}

	//クラス登録を解除
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}