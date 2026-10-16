#pragma once
// 全局常量与 GUID —— 同键 (Tongjian) 中英混输输入法

#include <windows.h>
#include <objbase.h>
#include <msctf.h>
#include <string>
#include <vector>
#include <cstdio>

// 调试日志（写入用户目录，便于排查输入法运行时问题）
inline void ImeLog(const wchar_t* msg) {
    FILE* f = _wfopen(L"C:\\Users\\25745\\AppData\\Local\\TongjianUpdate\\ime_runtime.log", L"a, ccs=UTF-8");
    if (f) { fwprintf(f, L"%s\n", msg); fclose(f); }
}

// 本输入法使用的语言 ID：简体中文 0x0804
const LANGID kLangId = 0x0804;

// 候选窗口最多显示的候选数
const int kMaxCandidates = 5;

// 输入串最长长度（拼音）
const int kMaxComposition = 64;

// COM 类 / 文本服务 CLSID
// {8B3C9B1E-6A4D-4F3E-9C1A-2B7D5E8F0A1B}
const CLSID c_clsidTextService =
{ 0x8b3c9b1e, 0x6a4d, 0x4f3e, { 0x9c, 0x1a, 0x2b, 0x7d, 0x5e, 0x8f, 0x0a, 0x1b } };

// 语言配置文件 GUID（用于 ITfInputProcessorProfiles::Register）
// {A1B2C3D4-E5F6-4748-8990-A1B2C3D4E5F6}
const GUID c_guidProfile =
{ 0xa1b2c3d4, 0xe5f6, 0x4748, { 0x89, 0x90, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6 } };

// 模块句柄与引用计数（在 dllmain.cpp 中定义）
extern HINSTANCE g_hInst;
extern LONG g_cRefDll;

void DllAddRef();
void DllRelease();

// UTF-8 <-> UTF-16 转换工具（内联，避免额外编译单元）
inline std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    if (n <= 0) return L"";
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}
inline std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return "";
    std::string u(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), &u[0], n, nullptr, nullptr);
    return u;
}
