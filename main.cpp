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

	HRESULT result;
	ID3D12Device* device = nullptr;
	IDXGIFactory7* dxgiFactry = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	ID3D12CommandAllocator* cmdAllocator = nullptr;
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
	
	//ゲームループ
	while (true) {
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