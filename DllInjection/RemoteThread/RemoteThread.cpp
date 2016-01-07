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
DWORD InjectDll(HANDLE process, LPCTSTR dllPath)
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
		return 0;
	}
	// �ȴ�Զ�߳̽���
	WaitForSingleObject(remoteThread, INFINITE);
	// ȡDLL��Ŀ����̵ľ��
	DWORD remoteModule;
	GetExitCodeThread(remoteThread, &remoteModule);

	// �ͷ�
	CloseHandle(remoteThread);
	VirtualFreeEx(process, remoteMemory, dllPathSize, MEM_DECOMMIT);

	return remoteModule;
}

// ж��DLL��ֻ������32λ������ΪCreateRemoteThread���ܴ�64λ��ַ������64λ����ֻ��ע�������Ȼ��Զ�߳�ִ���˰�...
BOOL FreeRemoteDll(HANDLE process, DWORD remoteModule)
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
	BOOL result;
	GetExitCodeThread(remoteThread, (LPDWORD)&result);

	// �ͷ�
	CloseHandle(remoteThread);
	return result;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// ����Ȩ��
	EnablePrivilege(TRUE);

	// �򿪽���
	HWND hwnd = FindWindow(NULL, _T("ARPTest"));
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
	DWORD remoteModule = InjectDll(process, dllPath);
	if (remoteModule == 0)
	{
		CloseHandle(process);
		return 2;
	}
	printf("ģ������0x%X\n", remoteModule);

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

