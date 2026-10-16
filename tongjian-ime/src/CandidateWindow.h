#pragma once
#include "globals.h"
#include "DictEngine.h"

// 候选弹出窗口：显示拼音串、纠错提示与候选列表
class CandidateWindow {
public:
    bool Create(HINSTANCE hInst);
    void Destroy();
    void Show(int x, int y, const std::wstring& pinyin,
              const std::vector<Candidate>& cands, const std::wstring& correction);
    void Hide();
    bool IsVisible() const { return hwnd_ != nullptr && IsWindow(hwnd_); }

private:
    HWND hwnd_ = nullptr;
    std::wstring text_;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void Paint();
};
