# -*- coding: utf-8 -*-
"""Inject the built dictionary + core logic + DOM glue into index.html."""
import os, re

ROOT = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(ROOT, 'data')

with open(os.path.join(DATA, 'dict.json'), encoding='utf-8') as f:
    dict_text = f.read()
with open(os.path.join(ROOT, 'ime_core.js'), encoding='utf-8') as f:
    core_text = f.read()
with open(os.path.join(ROOT, 'glue.js'), encoding='utf-8') as f:
    glue_text = f.read()
with open(os.path.join(ROOT, 'index.html'), encoding='utf-8') as f:
    html = f.read()

new_script = (
    '<script>\n'
    'const DICT = ' + dict_text + ';\n'
    + core_text + '\n'
    + glue_text + '\n'
    '</script>'
)

# replace the original <script>...</script> block (plain splice, no regex escapes)
start = html.index('<script>')
end = html.index('</script>', start) + len('</script>')
html2 = html[:start] + new_script + html[end:]

# update copy that described the demo-only dictionaries (idempotent)
html2 = html2.replace('中文候选为演示词库', '词库：拼音词典 13 万条 · 英文词 2 万 · 字符二元模型')
html2 = html2.replace('<h3>两种切分并行打分</h3>',
                      '<h3>词频语言模型打分</h3>')
html2 = html2.replace('检查字母串能否切成合法拼音音节，同时匹配英文词。辅音比例只作弱信号。',
                      '用 13 万条拼音词典动态切分并排序，同时匹配 2 万英文词；字符二元模型结合前文纠偏。')
html2 = html2.replace('中文语境提高中文候选优先级；连续英文会把英文候选排前面。',
                      '中文语境提高中文候选；连续英文把英文候选排前。真实词频 + 前后文 + 你的选择共同决定。')
# refresh the word-count figure regardless of its previous value
html2 = re.sub(r'词库：拼音词典 [0-9.]+ 万条', '词库：拼音词典 13 万条', html2)
html2 = re.sub(r'用 [0-9.]+ 万条拼音词典', '用 13 万条拼音词典', html2)

out_path = os.path.join(ROOT, 'index.html')
with open(out_path, 'w', encoding='utf-8') as f:
    f.write(html2)

print(f'final index.html written: {os.path.getsize(out_path)} bytes ({os.path.getsize(out_path)/1024:.1f} KB)')
