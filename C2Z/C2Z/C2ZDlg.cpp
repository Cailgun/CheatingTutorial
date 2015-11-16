
// C2ZDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "C2Z.h"
#include "C2ZDlg.h"
#include "afxdialogex.h"

#pragma region ���ù�
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CC2ZDlg �Ի���



CC2ZDlg::CC2ZDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CC2ZDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CC2ZDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, m_enableButton);
	DDX_Control(pDX, IDC_BUTTON2, m_disableButton);
}

BEGIN_MESSAGE_MAP(CC2ZDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CC2ZDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CC2ZDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CC2ZDlg ��Ϣ�������

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CC2ZDlg::OnPaint()
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
HCURSOR CC2ZDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
#pragma endregion

//////////////////////////////////////////////////////////////////////////////

// ��ʼ��
BOOL CC2ZDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	OnBnClickedButton1();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// ����
void CC2ZDlg::OnBnClickedButton1()
{
	m_enableButton.EnableWindow(FALSE);
	m_disableButton.EnableWindow(TRUE);
	SetTimer(1, 200, timerProc); // ÿ0.2s���C���Ƿ��£���ģ��Z��
}

// �ر�
void CC2ZDlg::OnBnClickedButton2()
{
	m_enableButton.EnableWindow(TRUE);
	m_disableButton.EnableWindow(FALSE);
	KillTimer(1);
}

//��ʱģ�ⰴ��Z
void CALLBACK CC2ZDlg::timerProc(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime)
{
	if ((GetKeyState('C') & (1 << 15)) != 0) // C������
	{
		INPUT input;
		ZeroMemory(&input, sizeof(input));
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = 'Z';
		input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
		SendInput(1, &input, sizeof(INPUT)); // ����Z��
		Sleep(100); // ���ܶ������ڴ����߼�ʱ���һ��Z���Ƿ��²ŷ���Ļ�������ʱZ���պõ����û�з�Ӧ������Ҫ�ӳ�һ��
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT)); // ����Z��
	}
}
