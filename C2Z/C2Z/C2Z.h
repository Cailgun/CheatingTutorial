
// C2Z.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CC2ZApp: 
// �йش����ʵ�֣������ C2Z.cpp
//

class CC2ZApp : public CWinApp
{
public:
	CC2ZApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CC2ZApp theApp;