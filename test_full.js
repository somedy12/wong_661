const fs = require('fs');
const path = require('path');
const html = fs.readFileSync(path.join(__dirname, 'index.html'), 'utf8');
const s = html.indexOf('<script>') + 8, e = html.indexOf('</script>');
const code = html.slice(s, e);

// minimal DOM shim so the injected glue layer can load
function makeEl() {
  return {
    innerHTML: '', textContent: '', value: '', selectionStart: 0,
    dataset: {}, addEventListener() {}, focus() {}, setRangeText() {}
  };
}
const els = {};
global.document = {
  querySelector(sel) { return els[sel] || (els[sel] = makeEl()); },
  querySelectorAll() { return []; }
};
global.localStorage = {
  _d: {},
  getItem(k) { return Object.prototype.hasOwnProperty.call(this._d, k) ? this._d[k] : null; },
  setItem(k, v) { this._d[k] = String(v); },
  removeItem(k) { delete this._d[k]; }
};

// execute the entire injected <script>
eval(code);

const T = globalThis.TongjianIME;
if (!T) { console.error('TongjianIME not defined after load'); process.exit(1); }

const samples = [
  'wo xiang join zhege meeting',
  '今天的meeting改到3点',
  'shi', 'she', 'can', 'no', 'china', 'share',
  'I think zhege idea hen good',
  'wo shi xuesheng',
  'zhongguo renmin yinhang',
  'nihao', 'nver', 'lvse', 'meeting',
  'chenggong', 'fazhan jingji', 'shehui fazhan', 'xuexi', 'shijie',
  'zhongwen', 'yingwen', 'dianshi', 'gongsi', 'jishu', 'wentai',
  'chongqing', 'lvyou', 'yinhangka', 'haochi',
  // correction / fuzzy cases
  'zhonguo', 'niha', 'xueshen', 'zhegn', 'zongguo', 'cat',
  'xing', 'shegn', 'zhegn hao'
];
console.log('== full injected script loads and analyzes ==');
for (const x of samples) {
  const r = T.analyze(x);
  console.log(JSON.stringify(x).padEnd(32), '->', r.results.slice(0, 4).map(a => a.text).join(' | '));
}
