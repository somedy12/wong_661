#include "globals.h"
#include "TextService.h"
#include <new>

HINSTANCE g_hInst = nullptr;
LONG g_cRefDll = 0;

void DllAddRef() { InterlockedIncrement(&g_cRefDll); }
void DllRelease() { InterlockedDecrement(&g_cRefDll); }

// ---------------------------------------------------------------------------
// 类工厂
// ---------------------------------------------------------------------------
class CClassFactory : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
            *ppv = static_cast<IClassFactory*>(this); AddRef(); return S_OK;
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { DllAddRef(); return 2; }
    STDMETHODIMP_(ULONG) Release() override { DllRelease(); return 1; }

    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override {
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;
        CTextService* p = new (std::nothrow) CTextService();
        if (!p) return E_OUTOFMEMORY;
        HRESULT hr = p->QueryInterface(riid, ppv);
        p->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL fLock) override {
        if (fLock) DllAddRef(); else DllRelease();
        return S_OK;
    }
};

// ---------------------------------------------------------------------------
// 注册表写入工具
// ---------------------------------------------------------------------------
static std::wstring GuidToString(const GUID& guid) {
    wchar_t buf[64];
    StringFromGUID2(guid, buf, 64);
    return buf;
}

static bool WriteReg(HKEY root, const std::wstring& key, const std::wstring& value, const std::wstring& data) {
    HKEY hk = nullptr;
    if (RegCreateKeyExW(root, key.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hk, nullptr) != ERROR_SUCCESS)
        return false;
    LONG r = RegSetValueExW(hk, value.c_str(), 0, REG_SZ, (const BYTE*)data.c_str(),
                            (DWORD)((data.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hk);
    return r == ERROR_SUCCESS;
}

static bool WriteRegDword(HKEY root, const std::wstring& key, const std::wstring& value, DWORD dword) {
    HKEY hk = nullptr;
    if (RegCreateKeyExW(root, key.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hk, nullptr) != ERROR_SUCCESS)
        return false;
    LONG r = RegSetValueExW(hk, value.c_str(), 0, REG_DWORD, (const BYTE*)&dword, sizeof(dword));
    RegCloseKey(hk);
    return r == ERROR_SUCCESS;
}

static void DeleteKeyTree(HKEY root, const std::wstring& key) {
    RegDeleteTreeW(root, key.c_str());
}

// ---------------------------------------------------------------------------
// 注册 / 反注册
// ---------------------------------------------------------------------------
static HRESULT RegisterServer() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(g_hInst, path, MAX_PATH);
    std::wstring clsid = GuidToString(c_clsidTextService);

    std::wstring base = L"Software\\Classes\\CLSID\\" + clsid;
    WriteReg(HKEY_LOCAL_MACHINE, base, L"", L"Tongjian IME");
    WriteReg(HKEY_LOCAL_MACHINE, base + L"\\InprocServer32", L"", path);
    WriteReg(HKEY_LOCAL_MACHINE, base + L"\\InprocServer32", L"ThreadingModel", L"Apartment");

    // TSF 文本服务标记（HKLM\Software\Microsoft\CTF\TIP\{CLSID}）
    std::wstring tip = L"Software\\Microsoft\\CTF\\TIP\\" + clsid;
    WriteRegDword(HKEY_LOCAL_MACHINE, tip, L"EnableOnDesktop", 1);
    return S_OK;
}

static HRESULT RegisterProfiles() {
    ITfInputProcessorProfiles* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfiles, (void**)&profiles);
    if (FAILED(hr)) return hr;

    hr = profiles->Register(c_clsidTextService);
    if (SUCCEEDED(hr)) {
        hr = profiles->AddLanguageProfile(c_clsidTextService, kLangId, c_guidProfile,
                                          L"同键中英混输", (ULONG)wcslen(L"同键中英混输"),
                                          nullptr, 0, 0);
    }
    profiles->Release();
    return hr;
}

static HRESULT RegisterCategories() {
    ITfCategoryMgr* catMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfCategoryMgr, (void**)&catMgr);
    if (FAILED(hr)) return hr;
    hr = catMgr->RegisterCategory(c_clsidTextService, GUID_TFCAT_TIP_KEYBOARD, c_clsidTextService);
    catMgr->Release();
    return hr;
}

static HRESULT UnregisterProfiles() {
    ITfInputProcessorProfiles* profiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfiles, (void**)&profiles);
    if (FAILED(hr)) return hr;
    profiles->RemoveLanguageProfile(c_clsidTextService, kLangId, c_guidProfile);
    profiles->Unregister(c_clsidTextService);
    profiles->Release();
    return S_OK;
}

static HRESULT UnregisterCategories() {
    ITfCategoryMgr* catMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfCategoryMgr, (void**)&catMgr);
    if (FAILED(hr)) return hr;
    catMgr->UnregisterCategory(c_clsidTextService, GUID_TFCAT_TIP_KEYBOARD, c_clsidTextService);
    catMgr->Release();
    return S_OK;
}

static HRESULT UnregisterServer() {
    std::wstring clsid = GuidToString(c_clsidTextService);
    DeleteKeyTree(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\" + clsid);
    DeleteKeyTree(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\CTF\\TIP\\" + clsid);
    return S_OK;
}

// ---------------------------------------------------------------------------
// 导出（通过 exports.def 声明，函数签名与 SDK 声明保持一致）
// ---------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (!IsEqualCLSID(rclsid, c_clsidTextService)) return CLASS_E_CLASSNOTAVAILABLE;
    CClassFactory* f = new (std::nothrow) CClassFactory();
    if (!f) return E_OUTOFMEMORY;
    HRESULT hr = f->QueryInterface(riid, ppv);
    f->Release();
    return hr;
}

STDAPI DllCanUnloadNow() { return g_cRefDll == 0 ? S_OK : S_FALSE; }

STDAPI DllRegisterServer() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // RPC_E_CHANGED_MODE：线程已被其它线程模型初始化，继续执行即可
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return hr;
    bool bUninit = (hr == S_OK);

    RegisterServer();
    RegisterProfiles();
    RegisterCategories();

    if (bUninit) CoUninitialize();
    return S_OK;
}

STDAPI DllUnregisterServer() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return hr;
    bool bUninit = (hr == S_OK);

    UnregisterProfiles();
    UnregisterCategories();
    UnregisterServer();

    if (bUninit) CoUninitialize();
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hInst = hInst;
        DisableThreadLibraryCalls(hInst);
    }
    return TRUE;
}
