
// TH14CheatDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "TH14Cheat.h"
#include "TH14CheatDlg.h"
#include "afxdialogex.h"

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
			return 1;
		case VK_F2: // ����ը��
			dlg->m_lockBomb = !dlg->m_lockBomb;
			return 1;
		case VK_F3: // ������
			dlg->m_lockPower = !dlg->m_lockPower;
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
			}
		}

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
	}

	CDialogEx::OnTimer(nIDEvent);
}
