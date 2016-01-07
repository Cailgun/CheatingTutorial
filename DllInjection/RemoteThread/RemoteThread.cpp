// RemoteThread.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"


// ����������Ȩ������ĳЩ������ʧ��
BOOL EnablePrivilege(BOOL enable)
{
	// �õ����ƾ��
	HANDLE hToken = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken))
		return FALSE;

	// �õ���Ȩֵ
	LUID luid;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		return FALSE;

	// �������ƾ��Ȩ��
	TOKEN_PRIVILEGES tp = {};
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL))
		return FALSE;

	// �ر����ƾ��
	CloseHandle(hToken);
	
	return TRUE;
}

// ע��DLL������ģ������64λ����ֻ�ܷ��ص�32λ��
HMODULE InjectDll(HANDLE process, LPCTSTR dllPath)
{
	DWORD dllPathSize = ((DWORD)_tcslen(dllPath) + 1) * sizeof(TCHAR);

	// �����ڴ��������DLL·��
	void* remoteMemory = VirtualAllocEx(process, NULL, dllPathSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (remoteMemory == NULL)
	{
		printf("�����ڴ�ʧ�ܣ�������룺%u\n", GetLastError());
		return 0;
	}

	// д��DLL·��
	if (!WriteProcessMemory(process, remoteMemory, dllPath, dllPathSize, NULL))
	{
		printf("д���ڴ�ʧ�ܣ�������룺%u\n", GetLastError());
		return 0;
	}

	// ����Զ�̵߳���LoadLibrary
	HANDLE remoteThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, remoteMemory, 0, NULL);
	if (remoteThread == NULL)
	{
		printf("����Զ�߳�ʧ�ܣ�������룺%u\n", GetLastError());
		return NULL;
	}
	// �ȴ�Զ�߳̽���
	WaitForSingleObject(remoteThread, INFINITE);
	// ȡDLL��Ŀ����̵ľ��
	DWORD remoteModule;
	GetExitCodeThread(remoteThread, &remoteModule);

	// �ͷ�
	CloseHandle(remoteThread);
	VirtualFreeEx(process, remoteMemory, dllPathSize, MEM_DECOMMIT);

	return (HMODULE)remoteModule;
}

// ж��DLL
BOOL FreeRemoteDll(HANDLE process, HMODULE remoteModule)
{
	// ����Զ�̵߳���FreeLibrary
	HANDLE remoteThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)FreeLibrary, (LPVOID)remoteModule, 0, NULL);
	if (remoteThread == NULL)
	{
		printf("����Զ�߳�ʧ�ܣ�������룺%u\n", GetLastError());
		return FALSE;
	}
	// �ȴ�Զ�߳̽���
	WaitForSingleObject(remoteThread, INFINITE);
	// ȡ����ֵ
	DWORD result;
	GetExitCodeThread(remoteThread, &result);

	// �ͷ�
	CloseHandle(remoteThread);
	return result != 0;
}

#ifdef _WIN64
#include <tlhelp32.h>
HMODULE GetRemoteModuleHandle(DWORD pid, LPCTSTR moduleName)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	MODULEENTRY32 moduleentry;
	moduleentry.dwSize = sizeof(moduleentry);

	BOOL flag = Module32First(snapshot, &moduleentry);
	HMODULE handle = NULL;
	do
	{
		if (_tcsicmp(moduleentry.szModule, moduleName) == 0)
		{
			handle = moduleentry.hModule;
			break;
		}
		flag = Module32Next(snapshot, &moduleentry);
	} while (flag);

	CloseHandle(snapshot);
	return handle;
}
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	// ����Ȩ��
	EnablePrivilege(TRUE);

	// �򿪽���
	HWND hwnd = FindWindow(NULL, _T("���������"));
	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (process == NULL)
	{
		printf("�򿪽���ʧ�ܣ�������룺%u\n", GetLastError());
		return 1;
	}
	
	// Ҫ��RemoteThreadDll.dll���ڱ�����ǰĿ¼��
	TCHAR dllPath[MAX_PATH]; // Ҫ�þ���·��
	GetCurrentDirectory(_countof(dllPath), dllPath);
	_tcscat_s(dllPath, _T("\\RemoteThreadDll.dll"));


	// ע��DLL
	HMODULE remoteModule = InjectDll(process, dllPath);
	if (remoteModule == NULL)
	{
		CloseHandle(process);
		return 2;
	}
#ifdef _WIN64
	remoteModule = GetRemoteModuleHandle(pid, _T("RemoteThreadDll.dll"));
	printf("ģ������0x%X%X\n", *((DWORD*)&remoteModule + 1), (DWORD)remoteModule);
#else
	printf("ģ������0x%X\n", (DWORD)remoteModule);
#endif

	// ��ͣ
	printf("���س�ж��DLL\n");
	getchar();

	// ж��DLL
	if (!FreeRemoteDll(process, remoteModule))
	{
		CloseHandle(process);
		return 3;
	}


	// �رս���
	CloseHandle(process);

	return 0;
}

