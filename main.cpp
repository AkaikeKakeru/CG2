#include <Windows.h>

#include <vector>
#include <string>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

//ウィンドウプロシージャ
LRESULT WindowProc(HWND hwnd,UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
	//ウィンドウ破棄されたなら
	case WM_DESTROY:
		//OSに対してアプリ終了を通知
		PostQuitMessage(0);
		return 0;
	}
	
	//メッセージ処理
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int){
	
	//サイズ
	const int window_width = 1280;
	const int window_height = 720;

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
	
#ifdef _DEBUG
			  //デバッグプレイヤーをオンに
	ID3D12Debug* debugController;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))){
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
		dxgiFactry->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND
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

	for (size_t i = 0; i < _countof(levels); i++){
		//採用したアダプターでデバイスを生成
		result = D3D12CreateDevice(tmpAdapter,levels[i],
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

		//4.描画コマンド

		//4.ここまで、描画コマンド

		//5.リソースバリアを戻す
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList->ResourceBarrier(1, &barrierDesc);

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