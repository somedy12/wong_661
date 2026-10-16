#include "TextService.h"
#include "dict_data.h"
#include <utility>

// ---------------------------------------------------------------------------
// IUnknown
// ---------------------------------------------------------------------------
STDMETHODIMP CTextService::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_POINTER;
    *ppvObj = nullptr;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor)) {
        *ppvObj = static_cast<ITfTextInputProcessor*>(this);
    } else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx)) {
        *ppvObj = static_cast<ITfTextInputProcessorEx*>(this);
    } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObj = static_cast<ITfKeyEventSink*>(this);
    } else if (IsEqualIID(riid, IID_ITfEditSession)) {
        *ppvObj = static_cast<ITfEditSession*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CTextService::AddRef() { return (ULONG)InterlockedIncrement(&ref_); }
STDMETHODIMP_(ULONG) CTextService::Release() {
    LONG c = InterlockedDecrement(&ref_);
    if (c == 0) delete this;
    return (ULONG)c;
}

CTextService::CTextService() {}
CTextService::~CTextService() {
    candWin_.Destroy();
    if (commitContext_) commitContext_->Release();
    if (threadMgr_) threadMgr_->Release();
}

// ---------------------------------------------------------------------------
// ITfTextInputProcessor / Ex
// ---------------------------------------------------------------------------
STDMETHODIMP CTextService::Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) {
    return ActivateEx(pThreadMgr, tfClientId, 0);
}

STDMETHODIMP CTextService::ActivateEx(ITfThreadMgr* pThreadMgr, TfClientId tfClientId, DWORD /*dwFlags*/) {
    ImeLog(L"[IME] ActivateEx called");
    if (threadMgr_) threadMgr_->Release();
    threadMgr_ = pThreadMgr;
    threadMgr_->AddRef();
    clientId_ = tfClientId;
    activated_ = true;

    bool dictOk = LoadDictionary();
    bool winOk = candWin_.Create(g_hInst);
    wchar_t buf[256];
    swprintf(buf, 256, L"[IME] dictOk=%d winOk=%d", dictOk ? 1 : 0, winOk ? 1 : 0);
    ImeLog(buf);

    // 挂载按键接收器：通过 ITfKeystrokeMgr（这是 TSF 接收按键的正确方式）
    ITfKeystrokeMgr* keystrokeMgr = nullptr;
    if (SUCCEEDED(threadMgr_->QueryInterface(IID_ITfKeystrokeMgr, (void**)&keystrokeMgr))) {
        HRESULT hrAdv = keystrokeMgr->AdviseKeyEventSink(clientId_, static_cast<ITfKeyEventSink*>(this), TRUE);
        swprintf(buf, 256, L"[IME] AdviseKeyEventSink(KeystrokeMgr) hr=0x%08X", (unsigned)hrAdv);
        ImeLog(buf);
        keystrokeMgr->Release();
    } else {
        ImeLog(L"[IME] QI ITfKeystrokeMgr failed");
    }
    return S_OK;
}

STDMETHODIMP CTextService::Deactivate() {
    ImeLog(L"[IME] Deactivate");
    activated_ = false;
    HideCandidates();

    ITfKeystrokeMgr* keystrokeMgr = nullptr;
    if (threadMgr_ && SUCCEEDED(threadMgr_->QueryInterface(IID_ITfKeystrokeMgr, (void**)&keystrokeMgr))) {
        keystrokeMgr->UnadviseKeyEventSink(clientId_);
        keystrokeMgr->Release();
    }
    if (threadMgr_) { threadMgr_->Release(); threadMgr_ = nullptr; }
    return S_OK;
}

// ---------------------------------------------------------------------------
// ITfKeyEventSink
// ---------------------------------------------------------------------------
STDMETHODIMP CTextService::OnSetFocus(BOOL /*fForeground*/) {
    return S_OK;
}
STDMETHODIMP CTextService::OnTestKeyDown(ITfContext*, WPARAM wParam, LPARAM, BOOL* pfEaten) {
    wchar_t buf[128];
    swprintf(buf, 128, L"[IME] OnTestKeyDown vk=0x%lX", (unsigned long)wParam);
    ImeLog(buf);
    HandleKey(wParam, true, pfEaten);
    return S_OK;
}
STDMETHODIMP CTextService::OnKeyDown(ITfContext*, WPARAM wParam, LPARAM, BOOL* pfEaten) {
    wchar_t buf[128];
    swprintf(buf, 128, L"[IME] OnKeyDown vk=0x%lX", (unsigned long)wParam);
    ImeLog(buf);
    HandleKey(wParam, false, pfEaten);
    return S_OK;
}
STDMETHODIMP CTextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}
STDMETHODIMP CTextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}
STDMETHODIMP CTextService::OnPreservedKey(ITfContext*, const GUID&, BOOL* pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

// ---------------------------------------------------------------------------
// ITfEditSession
// ---------------------------------------------------------------------------
STDMETHODIMP CTextService::DoEditSession(TfEditCookie ec) {
    // 提交文本会话
    if (commitContext_ && !commitText_.empty()) {
        TF_SELECTION sel;
        ULONG fetched = 0;
        if (SUCCEEDED(commitContext_->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &fetched)) && fetched > 0) {
            sel.range->SetText(ec, 0, commitText_.c_str(), (LONG)commitText_.size());
            sel.range->Release();
        }
        commitContext_->Release();
        commitContext_ = nullptr;
        commitText_.clear();
    }
    return S_OK;
}

// ---------------------------------------------------------------------------
// 内部实现
// ---------------------------------------------------------------------------
bool CTextService::LoadDictionary() {
    if (engine_.IsLoaded()) return true;
    return engine_.Load(kDictData, kDictSize);
}

void CTextService::UpdateCandidates() {
    AnalyzeResult r = engine_.Analyze(composition_);
    currentCands_ = std::move(r.candidates);
    correction_ = r.correction;
    wchar_t buf[256];
    swprintf(buf, 256, L"[IME] UpdateCandidates comp='%S' cands=%d", composition_.c_str(), (int)currentCands_.size());
    ImeLog(buf);
    if (composition_.empty()) { HideCandidates(); return; }

    // 优先定位到输入框光标下方，失败则回退到鼠标位置
    RECT rc;
    POINT pt;
    if (GetCaretRect(rc)) {
        pt.x = rc.left;
        pt.y = rc.bottom;
    } else {
        GetCursorPos(&pt);
    }
    candWin_.Show(pt.x, pt.y + 2, Utf8ToWide(composition_), currentCands_, correction_);
}

void CTextService::HideCandidates() { candWin_.Hide(); }

bool CTextService::GetCaretRect(RECT& rc) {
    // 输入法 DLL 运行在目标进程内，按键在 UI 线程处理，直接用 GUI 线程信息拿插入符位置
    GUITHREADINFO gui = { sizeof(gui) };
    if (GetGUIThreadInfo(GetCurrentThreadId(), &gui) && !IsRectEmpty(&gui.rcCaret)) {
        rc = gui.rcCaret;
        return true;
    }

    // 回退：焦点上下文所在窗口线程的插入符位置
    if (threadMgr_) {
        ITfDocumentMgr* docMgr = nullptr;
        if (SUCCEEDED(threadMgr_->GetFocus(&docMgr)) && docMgr) {
            ITfContext* ctx = nullptr;
            if (SUCCEEDED(docMgr->GetTop(&ctx)) && ctx) {
                ITfContextView* view = nullptr;
                if (SUCCEEDED(ctx->GetActiveView(&view))) {
                    HWND hwnd = nullptr;
                    if (SUCCEEDED(view->GetWnd(&hwnd)) && hwnd) {
                        DWORD tid = GetWindowThreadProcessId(hwnd, nullptr);
                        GUITHREADINFO gui2 = { sizeof(gui2) };
                        if (GetGUIThreadInfo(tid, &gui2) && !IsRectEmpty(&gui2.rcCaret)) {
                            rc = gui2.rcCaret;
                            view->Release();
                            ctx->Release();
                            docMgr->Release();
                            return true;
                        }
                    }
                    view->Release();
                }
                ctx->Release();
            }
            docMgr->Release();
        }
    }
    return false;
}

void CTextService::ClearComposition() {
    composition_.clear();
    currentCands_.clear();
    correction_.clear();
    HideCandidates();
}

HRESULT CTextService::CommitText(const std::wstring& text) {
    if (!threadMgr_ || text.empty()) return E_FAIL;
    ITfDocumentMgr* docMgr = nullptr;
    if (FAILED(threadMgr_->GetFocus(&docMgr)) || !docMgr) return E_FAIL;
    ITfContext* ctx = nullptr;
    if (FAILED(docMgr->GetTop(&ctx)) || !ctx) { docMgr->Release(); return E_FAIL; }
    docMgr->Release();

    commitText_ = text;
    if (commitContext_) commitContext_->Release();
    commitContext_ = ctx;   // GetTop 已 AddRef

    HRESULT hr = E_FAIL;
    ctx->RequestEditSession(clientId_, this, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, &hr);
    return hr;
}

HRESULT CTextService::CommitIndex(int idx) {
    std::wstring text;
    if (idx >= 0 && idx < (int)currentCands_.size()) {
        text = currentCands_[idx].text;
    } else {
        text = Utf8ToWide(composition_);   // 无候选则提交原始拼音
    }
    HRESULT hr = CommitText(text);
    ClearComposition();
    return hr;
}

bool CTextService::HandleKey(WPARAM wParam, bool testOnly, BOOL* pfEaten) {
    *pfEaten = FALSE;

    bool isLetter = (wParam >= 'A' && wParam <= 'Z');
    if (isLetter) {
        *pfEaten = TRUE;
        if (!testOnly) {
            if ((int)composition_.size() < kMaxComposition) {
                composition_ += (char)(wParam + ('a' - 'A'));
                UpdateCandidates();
            }
        }
        return true;
    }

    if (composition_.empty()) return false;

    switch (wParam) {
    case VK_BACK:
        *pfEaten = TRUE;
        if (!testOnly) {
            if (!composition_.empty()) composition_.pop_back();
            if (composition_.empty()) ClearComposition(); else UpdateCandidates();
        }
        return true;
    case VK_SPACE:
        *pfEaten = TRUE;
        if (!testOnly) CommitIndex(0);
        return true;
    case VK_RETURN:
        *pfEaten = TRUE;
        if (!testOnly) CommitIndex(-1);   // 提交原始拼音（英文/原样）
        return true;
    case VK_ESCAPE:
        *pfEaten = TRUE;
        if (!testOnly) ClearComposition();
        return true;
    default:
        break;
    }

    if (wParam >= '1' && wParam <= '9') {
        int idx = (int)(wParam - '1');
        if (idx < (int)currentCands_.size()) {
            *pfEaten = TRUE;
            if (!testOnly) CommitIndex(idx);
            return true;
        }
    }
    return false;
}
