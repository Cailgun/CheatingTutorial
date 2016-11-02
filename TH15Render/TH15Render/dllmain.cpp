// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include <d3d9.h>
#include "Decoder.h"
#include <mutex>


BOOL hookVTable(void* pInterface, int index, void* hookFunction, void* oldAddress)
{
	DWORD* address = &(*(DWORD**)pInterface)[index];
	if (address == NULL)
		return FALSE;

	// ����ԭ������ַ
	if (oldAddress != NULL)
		*(DWORD*)oldAddress = *address;

	// �޸��麯�����е�ַΪhookFunction
	DWORD oldProtect, oldProtect2;
	VirtualProtect(address, sizeof(DWORD), PAGE_READWRITE, &oldProtect);
	*address = (DWORD)hookFunction;
	VirtualProtect(address, sizeof(DWORD), oldProtect, &oldProtect2);

	return TRUE;
}

BOOL unhookVTable(void* pInterface, int index, void* oldAddress)
{
	// �޸Ļ�ԭ������ַ
	return hookVTable(pInterface, index, oldAddress, NULL);
}


typedef HRESULT(STDMETHODCALLTYPE* DrawPrimitiveUPType)(IDirect3DDevice9* thiz, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
DrawPrimitiveUPType RealDrawPrimitiveUP = NULL;

// ͨ�����Եõ���TH15��deviceָ���ַ
IDirect3DDevice9** g_ppDevice = (IDirect3DDevice9**)0x4E77D8;
IDirect3DDevice9* g_pDevice = *g_ppDevice;

// �����õĶ���ṹ
struct THVertex
{
	FLOAT    x, y, z;
	D3DCOLOR specular, diffuse;
	FLOAT    tu, tv;

	void Set(FLOAT x_, FLOAT y_, FLOAT tu_, FLOAT tv_)
	{
		x = x_;
		y = y_;
		tu = tu_;
		tv = tv_;
	}
};

// ������
HWND g_mainWnd = NULL;
WNDPROC g_realWndProc = NULL;
const UINT WM_DLL_INIT = WM_APP;
const UINT WM_UPDATE_TEXTURE = WM_APP + 1;

// ��Ƶ������
CDecoder* g_decoder = NULL;
SIZE g_videoSize;
SIZE g_scaledSize;
// ��Ƶ����
IDirect3DTexture9* g_texture = NULL;
BYTE* g_frameBuffer = NULL;
std::mutex g_frameBufferLock;


HRESULT STDMETHODCALLTYPE MyDrawPrimitiveUP(DWORD esp, IDirect3DDevice9* thiz, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	// �ж��ǲ�������Ⱦ�Ի�
	if (*(DWORD*)(esp + 0x9C) == 14			// ����
		|| *(DWORD*)(esp + 0x88) == 14		// ������ɫ��סshift
		)									// ��������ҾͲ�֪����ô�ж���...
	{
		// �Լ�����Ⱦ

		// ������������
		static THVertex vertex[6];
		memcpy(vertex, pVertexStreamZeroData, sizeof(vertex));
		float x = (vertex[0].x + vertex[5].x) / 2, y = (vertex[0].y + vertex[5].y) / 2; // �е�
		vertex[0].Set(x - g_scaledSize.cx / 2, y - g_scaledSize.cy / 2, 0, 0); // ����
		vertex[1].Set(x + g_scaledSize.cx / 2, y - g_scaledSize.cy / 2, 1, 0); // ����
		vertex[2].Set(x - g_scaledSize.cx / 2, y + g_scaledSize.cy / 2, 0, 1); // ����
		vertex[3].Set(x + g_scaledSize.cx / 2, y - g_scaledSize.cy / 2, 1, 0); // ����
		vertex[4].Set(x - g_scaledSize.cx / 2, y + g_scaledSize.cy / 2, 0, 1); // ����
		vertex[5].Set(x + g_scaledSize.cx / 2, y + g_scaledSize.cy / 2, 1, 1); // ����

		// ��������
		IDirect3DBaseTexture9* oldTexture = NULL;
		thiz->GetTexture(0, &oldTexture);
		thiz->SetTexture(0, g_texture);

		// ��Ⱦ
		HRESULT hr = RealDrawPrimitiveUP(thiz, PrimitiveType, PrimitiveCount, vertex, VertexStreamZeroStride);

		// �ָ�
		thiz->SetTexture(0, oldTexture);
		return hr;
	}
	return RealDrawPrimitiveUP(thiz, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

__declspec(naked) // ���ñ������Զ��Ӵ����ƻ�ջ
HRESULT STDMETHODCALLTYPE MyDrawPrimitiveUPWrapper(IDirect3DDevice9* thiz, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	__asm
	{
		pop ecx  // ���ص�ַ��ջ
		push esp // ��ʱ[esp] = thiz
		push ecx // �ָ����ص�ַ
		jmp MyDrawPrimitiveUP
	}
}


// �ѽ��������RGB���ݿ�����g_frameBuffer
void OnPresent(BYTE* data)
{
	g_frameBufferLock.lock();
	memcpy(g_frameBuffer, data, g_videoSize.cx * g_videoSize.cy * 4);
	g_frameBufferLock.unlock();
	PostMessage(g_mainWnd, WM_UPDATE_TEXTURE, 0, 0);
}

LRESULT CALLBACK MyWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_DLL_INIT) // �����̵߳ĳ�ʼ��
	{
		// ��ʼ��FFmpeg������
		av_register_all();

		// ����������
		g_decoder = new CDecoder("E:\\Bad Apple.avi");
		g_decoder->SetOnPresent(std::function<void(BYTE*)>(OnPresent));

		// ��������
		g_decoder->GetVideoSize(g_videoSize);
		g_frameBuffer = new BYTE[g_videoSize.cx * g_videoSize.cy * 4];
		D3DDISPLAYMODE dm;
		g_pDevice->GetDisplayMode(NULL, &dm);
		g_pDevice->CreateTexture(g_videoSize.cx, g_videoSize.cy, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &g_texture, NULL);

		float scale1 = 100.0f / g_videoSize.cx;
		float scale2 = 100.0f / g_videoSize.cy;
		float scale = scale1 < scale2 ? scale1 : scale2;
		g_scaledSize.cx = LONG(g_videoSize.cx * scale1);
		g_scaledSize.cy = LONG(g_videoSize.cy * scale1);

		// ��ʼ����
		g_decoder->Run();

		// hook D3D��Ⱦ
		hookVTable(g_pDevice, 83, MyDrawPrimitiveUPWrapper, &RealDrawPrimitiveUP);
		return 0;
	}
	else if (Msg == WM_UPDATE_TEXTURE) // ��������
	{
		// D3D9�����̰߳�ȫ�ģ����������˴�����Ϣʱ������Ⱦ
		D3DLOCKED_RECT rect;
		g_texture->LockRect(0, &rect, NULL, 0);
		g_frameBufferLock.lock();
		for (int y = 0; y < g_videoSize.cy; y++)
			memcpy((BYTE*)rect.pBits + y * rect.Pitch, g_frameBuffer + y * g_videoSize.cx * 4, g_videoSize.cx * 4);
		g_frameBufferLock.unlock();
		g_texture->UnlockRect(0);
		return 0;
	}

	return CallWindowProc(g_realWndProc, hWnd, Msg, wParam, lParam);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// ���໯���ڣ����ش�����Ϣ
		g_mainWnd = FindWindow(_T("BASE"), _T("��������`�@�` Legacy of Lunatic Kingdom. ver 1.00b")); // ���ı����������...
		g_realWndProc = (WNDPROC)SetWindowLongPtr(g_mainWnd, GWLP_WNDPROC, (ULONG_PTR)MyWndProc);

		// �������ĳ�ʼ�������߳����
		PostMessage(g_mainWnd, WM_DLL_INIT, 0, 0);
		break;

	case DLL_PROCESS_DETACH:
		// �ָ�D3D hook
		if (*g_ppDevice != NULL && RealDrawPrimitiveUP != NULL)
			unhookVTable(g_pDevice, 83, RealDrawPrimitiveUP);

		// �ָ����ڹ���
		if (IsWindow(g_mainWnd))
			SetWindowLongPtr(g_mainWnd, GWLP_WNDPROC, (ULONG_PTR)g_realWndProc);

		// �ͷ�
		if (g_decoder != NULL)
			delete g_decoder;
		if (g_texture != NULL)
			g_texture->Release();
		if (g_frameBuffer != NULL)
			delete g_frameBuffer;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

