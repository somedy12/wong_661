const fs = require('fs');
const path = require('path');
const { pinyin } = require('./data/pinyin_pro_pkg/package/dist/index.js');

// self-test
console.log('self-test:', JSON.stringify(['我们', '女儿', '绿色', '会议', '银行', '重庆', '好吃'].map(w =>
  pinyin(w, { toneType: 'none', type: 'array', v: true }).join('')
)));

const ROOT = __dirname;
const essay = fs.readFileSync(path.join(ROOT, 'data', 'rime_essay.txt'), 'utf8').split('\n');
const CJK = /^[\u3400-\u9fff]+$/;
const MIN_FREQ = 30;
const MAX_LEN = 6;

const out = {};
let processed = 0, skipped = 0;
for (const line of essay) {
  const idx = line.indexOf('\t');
  if (idx < 0) continue;
  const word = line.slice(0, idx).trim();
  const freq = parseInt(line.slice(idx + 1).trim(), 10);
  if (!word || !isFinite(freq)) continue;
  if (freq < MIN_FREQ) continue;
  if (word.length < 2 || word.length > MAX_LEN) continue;
  if (!CJK.test(word)) continue;
  let py;
  try {
    py = pinyin(word, { toneType: 'none', type: 'array', v: true }).join('');
  } catch (e) { skipped++; continue; }
  if (!py || !/^[a-z]+$/.test(py)) { skipped++; continue; }
  (out[py] = out[py] || []).push([word, freq]);
  processed++;
}

fs.writeFileSync(path.join(ROOT, 'data', 'cn_extra.json'), JSON.stringify(out));
console.log('processed words:', processed, 'skipped:', skipped, 'unique pinyin keys:', Object.keys(out).length);
