# 同键（Tongjian）中英混输输入法

同键尝试让中文拼音和英文在同一输入流程中自然混输。仓库包含浏览器交互原型、Windows TSF 输入法、词库生成脚本及本地词频数据。

## 项目组成

- `index.html`：浏览器原型，展示中英候选、拼音切分、停顿信号和本地候选偏好学习。直接用浏览器打开即可。
- `ime_core.js`、`glue.js`：原型的输入分析和界面逻辑；`inject.py` 将词库和脚本嵌入单文件页面。
- `tongjian-ime/`：Windows 64 位 TSF 文本服务。安装说明、按键说明和编译方式见 [`tongjian-ime/README.md`](tongjian-ime/README.md)。
- `data/`：拼音、中文词频和英文词频数据；`build_dict.py` 生成 `data/dict.json`，Windows 输入法的 `make_dict_binary.py` 再生成内嵌词库。

## Windows 输入法

快速试用可运行 `tongjian-ime/dist/TongjianIME-Setup.exe`，详细步骤和卸载方法见项目说明。该预编译安装程序面向 Windows 10/11 x64。

从源码编译需要 Visual Studio 2022 C++ 桌面开发组件和 CMake。词库有更新时，在仓库根目录运行：

```powershell
python build_dict.py
python tongjian-ime/make_dict_binary.py
```

然后按 [`tongjian-ime/README.md`](tongjian-ime/README.md) 中的步骤构建 DLL 和安装程序。浏览器原型更新后，可运行 `python inject.py` 将核心脚本和 `data/dict.json` 重新嵌入 `index.html`。

## 数据和第三方许可

训练词频数据和软件依赖来自多个上游项目，适用不同的许可和使用限制。请先阅读 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)，再复制、再分发或用于商业用途。尤其是英文词频语料带有研究和个人使用条件；本仓库默认按私有项目保存。

## 免责声明

此项目仍在开发中。浏览器原型使用本地存储记忆候选选择；Windows TSF 版本和浏览器原型的功能并不完全相同。
