const fs = require('fs');
const path = require('path');
const ime = require('./ime_core.js');

const dict = JSON.parse(fs.readFileSync(path.join(__dirname, 'data', 'dict.json'), 'utf8'));
ime.configure(dict);

const samples = [
  'wo xiang join zhege meeting',
  '今天的meeting改到3点',
  'shi',
  'she',
  'can',
  'no',
  'china',
  'share',
  'I think zhege idea hen good',
  'wo shi xuesheng',
  'nihao',
  'zhongguo renmin yinhang',
  'nver',
  'lvse',
];

for (const s of samples) {
  const r = ime.analyze(s);
  console.log('INPUT  :', JSON.stringify(s));
  console.log('CTX    :', r.context);
  console.log('RESULTS:', r.results.map(x => `${x.text}  <${x.type}/${x.lang}> (${x.score.toFixed(1)})`).join('  |  '));
  console.log('---');
}
