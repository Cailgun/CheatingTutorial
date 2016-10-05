// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include <d3dx9.h>



DWORD* findImportAddress(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName)
{
	// ��hook��ģ���ַ
	DWORD hookModuleBase = (DWORD)hookModule;
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hookModuleBase;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((DWORD)dosHeader + dosHeader->e_lfanew);
	// �����
	PIMAGE_IMPORT_DESCRIPTOR importTable = (PIMAGE_IMPORT_DESCRIPTOR)(hookModuleBase
		+ ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	// ���������ģ��
	for (; importTable->Characteristics != 0; importTable++)
	{
		// ���Ǻ�������ģ��
		if (_stricmp((LPCSTR)(hookModuleBase + importTable->Name), moduleName) != 0)
			continue;

		PIMAGE_THUNK_DATA info = (PIMAGE_THUNK_DATA)(hookModuleBase + importTable->OriginalFirstThunk);
		DWORD* iat = (DWORD*)(hookModuleBase + importTable->FirstThunk);

		// ��������ĺ���
		for (; info->u1.AddressOfData != 0; info++, iat++)
		{
			if ((info->u1.Ordinal & IMAGE_ORDINAL_FLAG) == 0) // ���ú����������
			{
				PIMAGE_IMPORT_BY_NAME name = (PIMAGE_IMPORT_BY_NAME)(hookModuleBase + info->u1.AddressOfData);
				if (strcmp(name->Name, functionName) == 0)
					return iat;
			}
		}

		return NULL; // û�ҵ�Ҫhook�ĺ���
	}

	return NULL; // û�ҵ�Ҫhook��ģ��
}

BOOL hookIAT(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName, void* hookFunction, void* oldAddress)
{
	DWORD* address = findImportAddress(hookModule, moduleName, functionName);
	if (address == NULL)
		return FALSE;

	// ����ԭ������ַ
	if (oldAddress != NULL)
		*(DWORD*)oldAddress = *address;

	// �޸�IAT�е�ַΪhookFunction
	DWORD oldProtect, oldProtect2;
	VirtualProtect(address, sizeof(DWORD), PAGE_READWRITE, &oldProtect);
	*address = (DWORD)hookFunction;
	VirtualProtect(address, sizeof(DWORD), oldProtect, &oldProtect2);

	return TRUE;
}

BOOL unhookIAT(HANDLE hookModule, LPCSTR moduleName, LPCSTR functionName)
{
	// ȡԭ������ַ
	void* oldAddress = GetProcAddress(GetModuleHandleA(moduleName), functionName);
	if (oldAddress == NULL)
		return FALSE;

	// �޸Ļ�ԭ������ַ
	return hookIAT(hookModule, moduleName, functionName, oldAddress, NULL);
}

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


typedef IDirect3D9* (WINAPI* Direct3DCreate9Type)(UINT SDKVersion);
typedef HRESULT(STDMETHODCALLTYPE* CreateDeviceType)(IDirect3D9* thiz, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);
typedef HRESULT(STDMETHODCALLTYPE* EndSceneType)(IDirect3DDevice9* thiz);
Direct3DCreate9Type RealDirect3DCreate9 = NULL;
CreateDeviceType RealCreateDevice = NULL;
EndSceneType RealEndScene = NULL;

IDirect3D9* g_d3d9 = NULL;
IDirect3DDevice9* g_device = NULL;
ID3DXFont* g_font = NULL;


HRESULT STDMETHODCALLTYPE MyEndScene(IDirect3DDevice9* thiz)
{
	static RECT rect = { 0, 0, 200, 200 };
	g_font->DrawText(NULL, _T("Hello World"), -1, &rect, DT_TOP | DT_LEFT, D3DCOLOR_XRGB(255, 0, 0));

	return RealEndScene(thiz);
}

HRESULT STDMETHODCALLTYPE MyCreateDevice(IDirect3D9* thiz, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
	unhookVTable(g_d3d9, 16, RealCreateDevice);
	HRESULT res = RealCreateDevice(thiz, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	g_device = *ppReturnedDeviceInterface;

	D3DXFONT_DESC d3dFont = {};
	d3dFont.Height = 25;
	d3dFont.Width = 12;
	d3dFont.Weight = 500;
	d3dFont.Italic = FALSE;
	d3dFont.CharSet = DEFAULT_CHARSET;
	wcscpy_s(d3dFont.FaceName, L"Times New Roman");
	D3DXCreateFontIndirect(g_device, &d3dFont, &g_font);

	// �����в�֪��Ϊʲô��һ�ε���DrawText��device���麯�����ָ���û�취ֻ����hookǰ����һ��
	static RECT rect = { 0, 0, 200, 200 };
	g_font->DrawText(NULL, _T("Hello World"), -1, &rect, DT_TOP | DT_LEFT, D3DCOLOR_XRGB(255, 0, 0));

	hookVTable(g_device, 42, MyEndScene, &RealEndScene); // EndScene��IDirect3DDevice9��43������

	return res;
}

IDirect3D9* WINAPI MyDirect3DCreate9(UINT SDKVersion)
{
	unhookIAT(GetModuleHandle(NULL), "d3d9.dll", "Direct3DCreate9");
	g_d3d9 = RealDirect3DCreate9(SDKVersion);
	hookVTable(g_d3d9, 16, MyCreateDevice, &RealCreateDevice); // CreateDevice��IDirect3D9��17������
	return g_d3d9;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hookIAT(GetModuleHandle(NULL), "d3d9.dll", "Direct3DCreate9", MyDirect3DCreate9, &RealDirect3DCreate9);
		break;

	case DLL_PROCESS_DETACH:
		if (g_device != NULL && RealEndScene != NULL)
			unhookVTable(g_device, 42, RealEndScene);
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

