#include <Windows.h>

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

int WINAPI winMain(HINSTANCE,HINSTANCE,LPSTR,int){
	
	return 0;
}