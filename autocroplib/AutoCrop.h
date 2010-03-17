// AutoCrop.h : Declaration of the CAutoCrop

#pragma once
#include "resource.h"       // main symbols

#include "autocroplib_i.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

const int nSamples = 9;

class IScriptEnvironment;
// CAutoCrop

class ATL_NO_VTABLE CAutoCrop :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAutoCrop, &CLSID_AutoCrop>,
	public IDispatchImpl<IAutoCrop, &IID_IAutoCrop, &LIBID_autocroplibLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CAutoCrop();

DECLARE_REGISTRY_RESOURCEID(IDR_AUTOCROP)

DECLARE_NOT_AGGREGATABLE(CAutoCrop)

BEGIN_COM_MAP(CAutoCrop)
	COM_INTERFACE_ENTRY(IAutoCrop)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();
	void FinalRelease()
	{
	}

public:
	STDMETHOD(GetAutoCropValues)(BSTR fileName);
	STDMETHOD(get_Left)(USHORT* pVal);
	STDMETHOD(get_Top)(USHORT* pVal);
	STDMETHOD(get_Right)(USHORT* pVal);
	STDMETHOD(get_Bottom)(USHORT* pVal);
	STDMETHOD(get_FrameCount)(int* pVal);

private:
	HRESULT AutoCropFrame(int frameNo, int *pLeft, int *pTop, int *pRight, int *pBottom);
	HRESULT Median(int *pValues, int *pMedian);

	WCHAR m_path[MAX_PATH];
	IScriptEnvironment *m_pEnv;
	PClip m_clp;
	VideoInfo m_inf;
	int m_left[nSamples], m_top[nSamples], m_right[nSamples], m_bottom[nSamples];
	int m_mLeft, m_mTop, m_mRight, m_mBottom;

	int m_frameCount;
};

OBJECT_ENTRY_AUTO(__uuidof(AutoCrop), CAutoCrop)
