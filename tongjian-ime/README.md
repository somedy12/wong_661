# 同键 · 中英免切换输入法（Windows 系统输入法）

把「同键」中英混输输入法做成了真正的 **Windows TSF 文本服务**：编译安装后，它会出现在系统的
「设置 → 时间和语言 → 语言 → 添加键盘」列表里，可以用 `Win+空格` / `Ctrl+Shift` 切换，
在任意程序中输入中文（拼音）或英文。

## 功能

- 中英混输：输入拼音自动转汉字，输入英文词自动保留英文（如 `meeting`、`join`、`idea`）。
- 拼音模糊音（冗余容错）：平翘舌 `zh↔z / ch↔c / sh↔s`、声母 `n↔l / f↔h / r↔l`、前后鼻音 `in↔ing / en↔eng / an↔ang`。
- 按键纠错：漏打/多打/错键/相邻键交换（如 `zhonguo→zhongguo`、`niha→nihao`）。
- 纠错提醒：候选窗口里用 `⚠` 显示「纠正/模糊音：原串 → 纠正串」。
- 内嵌词库：11.4 万条拼音词典 + 2 万英文高频词（约 2.5 MB，编译进 DLL）。

## 环境要求

**安装使用**：只需 **Windows 10 / 11（64 位）**，无需其它依赖。

**从源码编译**（开发者才需要）：

- **Visual Studio 2022**（免费 Community 版即可），安装时勾选 **“使用 C++ 的桌面开发”** 工作负载
  （会自带 MSVC 编译器 + Windows SDK + CMake）。

> 也可以只装轻量的 **“Visual Studio 2022 生成工具 (Build Tools)”**，同样勾选“使用 C++ 的桌面开发”。

**最快安装 Build Tools**（管理员 PowerShell 里运行）：

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

或手动下载：https://visualstudio.microsoft.com/visual-cpp-build-tools/ （勾选“使用 C++ 的桌面开发”）。

## 一键安装（推荐，无需 Visual Studio）

双击 **`dist\TongjianIME-Setup.exe`**（右键 →「以管理员身份运行」）即可：它内置了**已编译好的 DLL**，
会自动复制到 `C:\Program Files\TongjianIME\` 并注册，完成后按下面「第 3 步」加入键盘列表即可。

> 这个 exe 是**自解压二进制安装程序**，DLL 已静态链接运行库，**不依赖任何 VC++ 运行库**，
> 可在任意 Windows 10/11 64 位电脑上直接安装，无需装 Visual Studio。

`dist\` 目录里还附带了原始 `TongjianIME.dll` 和 `uninstall.bat`，可手动安装/卸载。

## 从源码编译（开发者，需要 Visual Studio）

1. 双击 **`build.bat`** —— 用 CMake 生成 x64 工程并编译，产出 `build\Release\TongjianIME.dll`。
2. 右键 **`install.bat`** → **“以管理员身份运行”** —— 注册 DLL 为系统输入法。
3. 打开 **设置 → 时间和语言 → 语言和区域** → 中文(简体，中国) → `...` → 语言选项 → 键盘 →
   **添加键盘** → 勾选 **“同键中英混输”**。

切换：`Win+空格` 或 `Ctrl+Shift`。

> 若第 3 步列表里没出现，注销后重新登录（或重启）一次即可。

## 使用方法

| 按键 | 作用 |
|---|---|
| 字母 `a-z` | 输入拼音（候选窗口实时显示） |
| `空格` | 上屏第一个候选（首选） |
| 数字 `1`-`5` | 上屏对应候选 |
| `回车` | 上屏原始拼音（英文/原样） |
| `退格` | 删除一个拼音字母 |
| `Esc` | 取消输入 |

示例：输入 `nihao` → 候选显示「1.你好 …」→ 按空格上屏「你好」。
输入 `meeting` → 候选显示「1.meeting …」→ 按空格上屏「meeting」。
输入 `zongguo`（平翘舌错误）→ 自动按模糊音纠正为「中国」，并提示 `模糊音：zongguo → zhongguo`。

## 卸载

右键 **`uninstall.bat`** → “以管理员身份运行”，再到设置里移除该键盘。

## 目录结构

```
tongjian-ime/
├── setup.bat            一键编译+安装（管理员）
├── build.bat / install.bat / uninstall.bat
├── make_installer.ps1   重新生成安装程序 exe（PowerShell）
├── dist/TongjianIME-Setup.exe   自解压安装程序（一键）
├── CMakeLists.txt       构建配置
├── make_dict_binary.py  把 ../data/dict.json 转成 src/dict_data.h + dict.bin
└── src/
    ├── dllmain.cpp      COM 服务器 + 注册/反注册
    ├── TextService.*    TSF 文本服务（按键、候选、上屏）
    ├── CandidateWindow.* 候选弹出窗口
    ├── DictEngine.*     词典引擎（拼音切分/候选/模糊音/纠错/中英判别）
    ├── globals.h        常量与 GUID
    ├── dict_data.h      内嵌词库（由 make_dict_binary.py 生成的字节数组头文件）
    └── dict.bin         同样内容的二进制（便于人工检查）
```

## 重新生成词库

词库来自仓库根目录的 `data/dict.json`（11.4 万拼音词条 + 2 万英文词）。改动后执行：

```bat
python make_dict_binary.py      # 重新生成 src/dict_data.h 与 dict.bin
build.bat                       # 重新编译
install.bat                     # 重新注册（管理员）
powershell -File make_installer.ps1   # 重新打包 dist\TongjianIME-Setup.exe
```

## 技术说明与已知限制

- 这是标准 TSF 文本服务（实现 `ITfTextInputProcessorEx` / `ITfKeyEventSink`），注册后成为系统输入法。
- 当前版本为**独立候选窗**模式：拼音串显示在候选窗口里（不内联到正文），上屏采用 `ITfRange::SetText`。
- 排序使用一元词频语言模型；网页原型里的「字符二元模型」暂未移植（可选后续加入）。
- 候选窗口定位在鼠标光标附近；后续可改用 `ITfContextView::GetTextExt` 精确跟随光标。
- 请务必用 **x64** 编译，以便在 64 位程序中正常工作。
