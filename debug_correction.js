const fs = require('fs');
const path = require('path');
const ime = require('./ime_core.js');
const dict = JSON.parse(fs.readFileSync(path.join(__dirname, 'data', 'dict.json'), 'utf8'));
ime.configure(dict);

for (const s of ['zhonguo', 'niha', 'xueshen', 'zhegn', 'shegn', 'zongguo', 'xuesheng', 'zhongguo']) {
  const seg = ime.segmentPinyin(s);
  console.log(`\n=== ${s} ===`);
  console.log('exact seg :', seg ? seg.seg.join('+') : null, 'score', seg ? seg.score.toFixed(1) : null);
  // what does analyze give?
  const r = ime.analyze(s);
  console.log('results   :', r.results.slice(0, 5).map(x => `${x.text}<${x.type}>(${x.score.toFixed(0)})`).join(' | '));
  console.log('signals   :', r.signals.map(x => x.join(':')).join(' | '));
}
