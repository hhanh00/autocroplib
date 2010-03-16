// autocropClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CoInitialize(NULL);

	{
	CComPtr<IAutoCrop> p;
	CoCreateInstance(CLSID_AutoCrop, NULL, CLSCTX_ALL, IID_IAutoCrop, (void **)&p);

	p->GetAutoCropValues(CComBSTR(L"D:\\TEMPRIP\\1.4.2\\precrop.avs"));
	}

	CoUninitialize();
	return 0;
}

