
// TH14CheatDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "TH14Cheat.h"
#include "TH14CheatDlg.h"
#include "afxdialogex.h"

// ������ʹ���޸Ĵ���ʵ�֣�����ʹ��ÿ��д�ڴ�ʵ��
#define MODIFY_CODE

#pragma region ���ù�
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTH14CheatDlg �Ի���



CTH14CheatDlg::CTH14CheatDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTH14CheatDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_lockHp = m_lockBomb = m_lockPower = FALSE;
	m_process = NULL;
}

void CTH14CheatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTH14CheatDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CTH14CheatDlg ��Ϣ�������

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CTH14CheatDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CTH14CheatDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
#pragma endregion

//////////////////////////////////////////////////////////////////////////////

// ��ʼ��
BOOL CTH14CheatDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// ע����̹��Ӽ����ȼ�
	m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KbdProc, GetModuleHandle(NULL), NULL);
	// ע��1s��ʱ��
	SetTimer(0, 1000, NULL);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �ͷ�
void CTH14CheatDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	UnhookWindowsHookEx(m_hook);
	if (m_process != NULL)
	{
		// �ͷž��
		CloseHandle(m_process);
	}
}

// �����ȼ�
LRESULT CALLBACK CTH14CheatDlg::KbdProc(int code, WPARAM wParam, LPARAM lParam)
{
	CTH14CheatDlg* dlg = (CTH14CheatDlg*)AfxGetApp()->m_pMainWnd;
	if (code != HC_ACTION)
		return CallNextHookEx(dlg->m_hook, code, wParam, lParam);

	PKBDLLHOOKSTRUCT param = (PKBDLLHOOKSTRUCT)lParam;
	if ((param->flags & (1 << 7)) == 0) // ����
	{
		switch (param->vkCode)
		{
		case VK_F1: // ���޲л�
			dlg->m_lockHp = !dlg->m_lockHp;
#ifdef MODIFY_CODE
			dlg->modifyHpCode();
#endif
			return 1;
		case VK_F2: // ����ը��
			dlg->m_lockBomb = !dlg->m_lockBomb;
#ifdef MODIFY_CODE
			dlg->modifyBombCode();
#endif
			return 1;
		case VK_F3: // ������
			dlg->m_lockPower = !dlg->m_lockPower;
#ifdef MODIFY_CODE
			dlg->modifyPowerCode();
#endif
			return 1;
		}
	}

	return CallNextHookEx(dlg->m_hook, code, wParam, lParam);
}

// ����ʱ��
void CTH14CheatDlg::OnTimer(UINT_PTR nIDEvent)
{
	HWND hwnd = ::FindWindow(_T("BASE"), NULL);
	if (hwnd == NULL) // �����ѹر�
	{
		if (m_process != NULL)
		{
			// �ͷž��
			CloseHandle(m_process);
			m_process = NULL;
		}
	}
	else
	{
		// �򿪽���
		if (m_process == NULL)
		{
			DWORD pid;
			GetWindowThreadProcessId(hwnd, &pid);
			m_process = OpenProcess(/*PROCESS_ALL_ACCESS*/ PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
			if (m_process == NULL)
			{
				CString msg;
				msg.Format(_T("�򿪽���ʧ�ܣ�������룺%u"), GetLastError());
				MessageBox(msg, NULL, MB_ICONERROR);
				CDialogEx::OnTimer(nIDEvent);
				return;
			}

#ifdef MODIFY_CODE
			// �մ򿪽���ʱ�޸Ĵ���
			modifyHpCode();
			modifyBombCode();
			modifyPowerCode();
#endif
		}

#ifndef MODIFY_CODE
		// д�ڴ�
		DWORD buffer;
		if (m_lockHp)
		{
			WriteProcessMemory(m_process, (LPVOID)0x004F5864, &(buffer = 8), sizeof(DWORD), NULL);
		}
		if (m_lockBomb)
		{
			WriteProcessMemory(m_process, (LPVOID)0x004F5870, &(buffer = 8), sizeof(DWORD), NULL);
		}
		if (m_lockPower)
		{
			WriteProcessMemory(m_process, (LPVOID)0x004F5858, &(buffer = 400), sizeof(DWORD), NULL);
		}
#endif
	}

	CDialogEx::OnTimer(nIDEvent);
}

// �޸Ĺ��ڲл��Ĵ���
void CTH14CheatDlg::modifyHpCode()
{
	static const BYTE originalCode[] = { 0xA3, 0x64, 0x58, 0x4F, 0x00 };
	static const BYTE modifiedCode[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
	if (m_process != NULL)
	{
		WriteProcessMemory(m_process, (LPVOID)0x0044F618, m_lockHp ? modifiedCode : originalCode, sizeof(originalCode), NULL);
	}
}

// �޸Ĺ���ը���Ĵ���
void CTH14CheatDlg::modifyBombCode()
{
	static const BYTE originalCode1[] = { 0xA3, 0x70, 0x58, 0x4F, 0x00 };
	static const BYTE modifiedCode1[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
	static const BYTE originalCode2[] = { 0x7E, 0x0E };
	static const BYTE modifiedCode2[] = { 0x90, 0x90 };
	if (m_process != NULL)
	{
		WriteProcessMemory(m_process, (LPVOID)0x0041218A, m_lockBomb ? modifiedCode1 : originalCode1, sizeof(originalCode1), NULL);
		WriteProcessMemory(m_process, (LPVOID)0x0044DD68, m_lockBomb ? modifiedCode2 : originalCode2, sizeof(originalCode2), NULL);
	}
}

// �޸Ĺ��������Ĵ���
void CTH14CheatDlg::modifyPowerCode()
{
	static const BYTE originalCode1[] = { 0xA3, 0x58, 0x58, 0x4F, 0x00 };
	static const BYTE modifiedCode1[] = { 0xB8, 0x90, 0x01, 0x00, 0x00 };
	static const BYTE originalCode2[] = { 0x03, 0xC8, 0x3B, 0xCE, 0x0F, 0x4C, 0xCD };
	static const BYTE modifiedCode2[] = { 0xB9, 0x90, 0x01, 0x00, 0x00, 0x90, 0x90 };
	if (m_process != NULL)
	{
		WriteProcessMemory(m_process, (LPVOID)0x00435DAF, m_lockPower ? modifiedCode1 : originalCode1, sizeof(originalCode1), NULL);
		WriteProcessMemory(m_process, (LPVOID)0x0044DDB8, m_lockPower ? modifiedCode2 : originalCode2, sizeof(originalCode2), NULL);
		if (m_lockPower)
		{
			DWORD buffer;
			WriteProcessMemory(m_process, (LPVOID)0x004F5858, &(buffer = 400), sizeof(DWORD), NULL);
		}
	}
}

