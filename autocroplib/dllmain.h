// dllmain.h : Declaration of module class.

class CautocroplibModule : public CAtlDllModuleT< CautocroplibModule >
{
public :
	DECLARE_LIBID(LIBID_autocroplibLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_AUTOCROPLIB, "{2B927CF4-F981-4742-993C-7D137CA15688}")
};

extern class CautocroplibModule _AtlModule;
