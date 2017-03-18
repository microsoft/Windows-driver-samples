// dllmain.h : Declaration of module class.

class CSampleMft0Module : public ATL::CAtlDllModuleT< CSampleMft0Module >
{
public :
    DECLARE_LIBID(LIBID_SampleMft0Lib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_SAMPLEMFT0, "{F9ED475B-DC47-458A-A138-55D7E8A57AF0}")
};

extern class CSampleMft0Module _AtlModule;
