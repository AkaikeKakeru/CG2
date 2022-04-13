#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

//�E�B���h�E�v���V�[�W��
LRESULT WindowProc(HWND hwnd,UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
	//�E�B���h�E�j�����ꂽ�Ȃ�
	case WM_DESTROY:
		//OS�ɑ΂��ăA�v���I����ʒm
		PostQuitMessage(0);
		return 0;
	}
	
	//���b�Z�[�W����
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int){
	
	//�T�C�Y
	const int window_width = 1280;
	const int window_height = 720;

	//�N���X�ݒ�
	WNDCLASSEX w{};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProc;
	w.lpszClassName = L"DirectXGame";
	w.hInstance = GetModuleHandle(nullptr);
	w.hCursor = LoadCursor(NULL, IDC_ARROW);

	//OS�ɓo�^
	RegisterClassEx(&w);
	//�T�C�Y
	RECT wrc = { 0,0,window_width,window_height };
	//�����ŃT�C�Y�C��
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

	//�\����Ԃɂ���
	ShowWindow(hwnd, SW_SHOW);

	MSG msg{};//���b�Z�[�W

	HRESULT result;
	ID3D12Device* device = nullptr;
	IDXGIFactory7* dxgiFactry = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	ID3D12CommandAllocator* cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* commandList = nullptr;
	ID3D12CommandQueue* commandQueue = nullptr;
	ID3D12DescriptorHeap* rtvHeap = nullptr;

	//�Q�[�����[�v
	while (true) {
		//���b�Z�[�W�͂��邩�H
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//�~�ŏI�����b�Z�[�W�������烋�[�v�𔲂���
		if (msg.message == WM_QUIT) {
			break;
		}

	}

	//�N���X�o�^������
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}