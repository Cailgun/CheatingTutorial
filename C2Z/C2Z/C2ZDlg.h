
// C2ZDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CC2ZDlg �Ի���
class CC2ZDlg : public CDialogEx
{
// ����
public:
	CC2ZDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_C2Z_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();

	static void CALLBACK timerProc(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime);


public:
	CButton m_enableButton;
	CButton m_disableButton;
};
