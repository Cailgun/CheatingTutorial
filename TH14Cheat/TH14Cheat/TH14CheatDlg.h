
// TH14CheatDlg.h : ͷ�ļ�
//

#pragma once


// CTH14CheatDlg �Ի���
class CTH14CheatDlg : public CDialogEx
{
// ����
public:
	CTH14CheatDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_TH14CHEAT_DIALOG };

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
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();

	static LRESULT CALLBACK KbdProc(int code, WPARAM wParam, LPARAM lParam);

	void modifyHpCode();
	void modifyBombCode();
	void modifyPowerCode();


public:
	HHOOK m_hook;

	BOOL m_lockHp;
	BOOL m_lockBomb;
	BOOL m_lockPower;

	HANDLE m_process;
};
