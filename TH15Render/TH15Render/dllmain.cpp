// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include <d3d9.h>
#include "Decoder.h"
#include <mutex>


#pragma pack(push)
#pragma pack(1)
struct JmpCode
{
private:
	const BYTE jmp;
	DWORD address;

public:
	JmpCode(DWORD srcAddr, DWORD dstAddr)
		: jmp(0xE9)
	{
		setAddress(srcAddr, dstAddr);
	}

	void setAddress(DWORD srcAddr, DWORD dstAddr)
	{
		address = dstAddr - srcAddr - sizeof(JmpCode);
	}
};
#pragma pack(pop)

void hook(void* originalFunction, void* hookFunction, BYTE* oldCode)
{
	JmpCode code((uintptr_t)originalFunction, (uintptr_t)hookFunction);
	DWORD oldProtect, oldProtect2;
	VirtualProtect(originalFunction, sizeof(code), PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(oldCode, originalFunction, sizeof(code));
	memcpy(originalFunction, &code, sizeof(code));
	VirtualProtect(originalFunction, sizeof(code), oldProtect, &oldProtect2);
}

void unhook(void* originalFunction, BYTE* oldCode)
{
	DWORD oldProtect, oldProtect2;
	VirtualProtect(originalFunction, sizeof(JmpCode), PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(originalFunction, oldCode, sizeof(JmpCode));
	VirtualProtect(originalFunction, sizeof(JmpCode), oldProtect, &oldProtect2);
}

BOOL hookVTable(void* pInterface, int index, void* hookFunction, void** oldAddress)
{
	void** address = &(*(void***)pInterface)[index];
	if (address == NULL)
		return FALSE;

	// ����ԭ������ַ
	if (oldAddress != NULL)
		*oldAddress = *address;

	// �޸��麯�����е�ַΪhookFunction
	DWORD oldProtect, oldProtect2;
	VirtualProtect(address, sizeof(DWORD), PAGE_READWRITE, &oldProtect);
	*address = hookFunction;
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
IDirect3DDevice9*& g_device = *(IDirect3DDevice9**)0x4E77D8;

// ��ʵ����Լ����__thiscall��������__fastcallҲ����ʹ��һ��������ecx�Ĵ���
int(__fastcall* const RenderPlayer)(void*) = (int(__fastcall*)(void*))0x4872F0;
BYTE renderPlayerOldCode[sizeof(JmpCode)];
// �Դӵ���RenderPlayer�����������Ⱦ�˼���
int g_renderCount = 999;

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

// ��Ƶ������
CDecoder* g_decoder = NULL;
SIZE g_videoSize;
SIZE g_scaledSize;
// ��Ƶ����
IDirect3DTexture9* g_texture = NULL;
BYTE* g_frameBuffer = NULL;
bool g_textureNeedUpdate = FALSE;
std::mutex g_frameBufferLock;


int __fastcall MyRenderPlayer(void* thiz)
{
	g_renderCount = 0;
	unhook(RenderPlayer, renderPlayerOldCode);
	int result = ((int(__fastcall*)(void*))0x4872F0)(thiz);
	hook(RenderPlayer, MyRenderPlayer, renderPlayerOldCode);
	return result;
}

HRESULT STDMETHODCALLTYPE MyDrawPrimitiveUP(IDirect3DDevice9* thiz, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	if (g_texture == NULL)
	{
		// ��ʼ��
		g_device->CreateTexture(g_videoSize.cx, g_videoSize.cy, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &g_texture, NULL);
	}

	// �ж��ǲ�������Ⱦ�Ի�
	if (++g_renderCount == 1)
	{
		// �Լ�����Ⱦ

		// ��������
		if (g_textureNeedUpdate)
		{
			D3DLOCKED_RECT rect;
			g_texture->LockRect(0, &rect, NULL, 0);
			g_frameBufferLock.lock();
			for (int y = 0; y < g_videoSize.cy; y++)
				memcpy((BYTE*)rect.pBits + y * rect.Pitch, g_frameBuffer + y * g_videoSize.cx * 4, g_videoSize.cx * 4);
			g_textureNeedUpdate = false;
			g_frameBufferLock.unlock();
			g_texture->UnlockRect(0);
		}

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

// �ѽ��������RGB���ݿ�����g_frameBuffer
void OnPresent(BYTE* data)
{
	g_frameBufferLock.lock();
	memcpy(g_frameBuffer, data, g_videoSize.cx * g_videoSize.cy * 4);
	g_textureNeedUpdate = true;
	g_frameBufferLock.unlock();
}

DWORD WINAPI initThread(LPVOID)
{
	// ��ʼ��FFmpeg������
	av_register_all();

	// ����������
	g_decoder = new CDecoder("E:\\Bad Apple.avi");
	g_decoder->SetOnPresent(std::function<void(BYTE*)>(OnPresent));

	// �����������ݻ���
	g_decoder->GetVideoSize(g_videoSize);
	g_frameBuffer = new BYTE[g_videoSize.cx * g_videoSize.cy * 4];

	float scale1 = 100.0f / g_videoSize.cx;
	float scale2 = 100.0f / g_videoSize.cy;
	float scale = scale1 < scale2 ? scale1 : scale2;
	g_scaledSize.cx = LONG(g_videoSize.cx * scale1);
	g_scaledSize.cy = LONG(g_videoSize.cy * scale1);

	// ��ʼ����
	g_decoder->Run();

	// hook
	hookVTable(g_device, 83, MyDrawPrimitiveUP, (void**)&RealDrawPrimitiveUP);
	hook(RenderPlayer, MyRenderPlayer, renderPlayerOldCode);

	return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// ����һ���̳߳�ʼ������������
		CloseHandle(CreateThread(NULL, 0, initThread, NULL, 0, NULL));
		break;

	case DLL_PROCESS_DETACH:
		// �ָ�hook
		unhook(RenderPlayer, renderPlayerOldCode);
		if (g_device != NULL && RealDrawPrimitiveUP != NULL)
			unhookVTable(g_device, 83, RealDrawPrimitiveUP);

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

