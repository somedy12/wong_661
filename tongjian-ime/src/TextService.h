#pragma once
#include "globals.h"
#include "DictEngine.h"
#include "CandidateWindow.h"

// TSF 文本服务：接收按键、生成候选、提交文字
class CTextService : public ITfTextInputProcessorEx,
                     public ITfKeyEventSink,
                     public ITfEditSession {
public:
    CTextService();
    virtual ~CTextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor
    STDMETHODIMP Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) override;
    STDMETHODIMP Deactivate() override;

    // ITfTextInputProcessorEx
    STDMETHODIMP ActivateEx(ITfThreadMgr* pThreadMgr, TfClientId tfClientId, DWORD dwFlags) override;

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground) override;
    STDMETHODIMP OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext* pic, const GUID& rguid, BOOL* pfEaten) override;

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec) override;

private:
    LONG ref_ = 1;
    ITfThreadMgr* threadMgr_ = nullptr;
    TfClientId clientId_ = TF_CLIENTID_NULL;
    bool activated_ = false;

    DictEngine engine_;
    CandidateWindow candWin_;

    std::string composition_;        // 当前拼音串（ascii）
    std::wstring correction_;        // 纠错提示
    std::vector<Candidate> currentCands_;

    // 提交相关
    std::wstring commitText_;
    ITfContext* commitContext_ = nullptr;

    bool LoadDictionary();
    void UpdateCandidates();
    void HideCandidates();
    void ClearComposition();
    HRESULT CommitText(const std::wstring& text);
    HRESULT CommitIndex(int idx);   // idx: 0..n-1 候选序号；-1 表示提交首选
    bool HandleKey(WPARAM wParam, bool testOnly, BOOL* pfEaten);
    bool GetCaretRect(RECT& rc);
};
