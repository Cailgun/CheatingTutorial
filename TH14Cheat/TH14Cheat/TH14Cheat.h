
// TH14Cheat.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CTH14CheatApp: 
// �йش����ʵ�֣������ TH14Cheat.cpp
//

class CTH14CheatApp : public CWinApp
{
public:
	CTH14CheatApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CTH14CheatApp theApp;