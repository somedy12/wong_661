# 第三方数据与软件声明

同键词库及构建流程使用了下列上游数据或软件。生成词库、嵌入式词库、安装包和包含词库的网页均可能包含相应上游数据的衍生成分；不要把这些衍生文件当作没有限制的自有数据重新分发。

## 中文拼音与词频数据

- **Rime 袖珍简化字拼音词典**：`data/pinyin_simp.dict.yaml`。文件注明其源自 Android 开源拼音输入法项目；Rime 项目页面列出 LGPL-3.0。上游：[rime/rime-pinyin-simp](https://github.com/rime/rime-pinyin-simp)。
- **Rime 八股文词频**：`data/rime_essay.txt`。上游项目采用 LGPL-3.0：[rime/rime-essay](https://github.com/rime/rime-essay)。
- **OpenCC 字符映射数据**：构建脚本读取 OpenCC JS 包中的繁简字符映射。`opencc-js` 声明 MIT 与 Apache-2.0 双许可，数据许可信息见包内 `data/opencc_pkg/package/THIRD_PARTY_LICENSES.md`；上游：[nk2028/opencc-js](https://github.com/nk2028/opencc-js)。

## 英文词频数据

- **FrequencyWords**：`data/en_50k.txt`。数据来自 OpenSubtitles 语料生成的词频表，内容标注 CC BY-SA 4.0；代码仓库为 MIT。来源和许可说明：[hermitdave/FrequencyWords](https://github.com/hermitdave/FrequencyWords)。
- **Google Web Trillion Word Corpus 派生词表**：`data/google-20k.txt`。上游说明数据派生自 Google Web Trillion Word Corpus，并受 Linguistic Data Consortium 许可约束；上游说明仅允许教育、个人或研究用途。来源及其许可说明：[first20hours/google-10000-english](https://github.com/first20hours/google-10000-english)。

## 构建依赖

- **pinyin-pro**：`data/pinyin_pro_pkg/package/`，MIT 许可；版权和许可文本见该目录的 `LICENSE`。上游：[zh-lx/pinyin-pro](https://github.com/zh-lx/pinyin-pro)。

## 重要提醒

请在公开发布或商业使用词库和预编译文件之前，分别核对上游当前许可、署名要求及语料使用条款。上面列出的许可名称和链接用于来源追踪，不替代各上游附带的完整许可文本。
