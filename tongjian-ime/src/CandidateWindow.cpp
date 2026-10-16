#include "CandidateWindow.h"

namespace {
const wchar_t* kClassName = L"TongjianCandidateWindow";
const int kFontPx = 22;          // 候选窗字号
const int kPadX = 16;            // 左右内边距
const int kPadY = 10;            // 上下内边距

HFONT CreateUiFont() {
    return CreateFontW(-kFontPx, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
}
} // namespace

bool CandidateWindow::Create(HINSTANCE hInst) {
    if (hwnd_) return true;
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                            kClassName, L"同键候选",
                            WS_POPUP | WS_BORDER,
                            0, 0, 10, 10,
                            nullptr, nullptr, hInst, this);
    wchar_t buf[128];
    swprintf(buf, 128, L"[IME] CandidateWindow Create hwnd=%p", (void*)hwnd_);
    ImeLog(buf);
    return hwnd_ != nullptr;
}

void CandidateWindow::Destroy() {
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
}

void CandidateWindow::Show(int x, int y, const std::wstring& pinyin,
                           const std::vector<Candidate>& cands,
                           const std::wstring& correction) {
    if (!hwnd_) return;

    // 拼音一行，候选每个一行，更大更清晰
    std::wstring t;
    if (!correction.empty()) t += L"⚠ " + correction + L"\r\n";
    t += pinyin + L"\r\n";
    for (size_t i = 0; i < cands.size() && i < (size_t)kMaxCandidates; ++i) {
        t += std::to_wstring(i + 1) + L". " + cands[i].text + L"\r\n";
    }
    while (!t.empty() && (t.back() == L'\n' || t.back() == L'\r')) t.pop_back();
    text_ = t;

    HDC dc = GetDC(hwnd_);
    HFONT font = CreateUiFont();
    HFONT old = (HFONT)SelectObject(dc, font);
    RECT mrc = { 0, 0, 0, 0 };
    DrawTextW(dc, text_.c_str(), -1, &mrc, DT_CALCRECT | DT_LEFT | DT_TOP);
    SelectObject(dc, old);
    DeleteObject(font);
    ReleaseDC(hwnd_, dc);

    int w = mrc.right - mrc.left + kPadX * 2;
    int h = mrc.bottom - mrc.top + kPadY * 2;

    // 保证窗口不出屏幕
    int sx = GetSystemMetrics(SM_CXSCREEN), sy = GetSystemMetrics(SM_CYSCREEN);
    if (x + w > sx) x = sx - w;
    if (y + h > sy) y = sy - h - 8;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, w, h, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    InvalidateRect(hwnd_, nullptr, TRUE);
    wchar_t buf[128];
    swprintf(buf, 128, L"[IME] CandidateWindow Show x=%d y=%d w=%d h=%d", x, y, w, h);
    ImeLog(buf);
}

void CandidateWindow::Hide() {
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
}

void CandidateWindow::Paint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd_, &ps);
    RECT rc;
    GetClientRect(hwnd_, &rc);
    HBRUSH bg = CreateSolidBrush(RGB(0xFB, 0xFC, 0xFA));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0x20, 0x29, 0x23));
    HFONT font = CreateUiFont();
    HFONT old = (HFONT)SelectObject(dc, font);

    RECT rt = rc;
    rt.left += kPadX; rt.top += kPadY; rt.right -= kPadX; rt.bottom -= kPadY;
    DrawTextW(dc, text_.c_str(), -1, &rt, DT_LEFT | DT_TOP | DT_NOPREFIX);

    SelectObject(dc, old);
    DeleteObject(font);
    EndPaint(hwnd_, &ps);
}

LRESULT CALLBACK CandidateWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    CandidateWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        self = (CandidateWindow*)((CREATESTRUCT*)lp)->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    } else {
        self = (CandidateWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }
    switch (msg) {
    case WM_PAINT:
        if (self) self->Paint();
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
