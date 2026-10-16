/* ---- 同键 DOM 交互层（依赖已注入的 DICT 与 TongjianIME） ---- */
const ime = TongjianIME;
ime.configure(DICT);

const input = document.querySelector('#input'),
      signals = document.querySelector('#signals'),
      candidates = document.querySelector('#candidates'),
      count = document.querySelector('#cand-count'),
      contextLabel = document.querySelector('#context-label'),
      learnCount = document.querySelector('#learn-count');

let lastKeyAt = 0, pauses = [], selectedIndex = 0, lastResults = [];
let learned = {};
try { learned = JSON.parse(localStorage.getItem('tongjian-learned') || '{}'); } catch (e) { learned = {}; }

function learnedBoost(key) {
  const rec = learned[key];
  if (!rec) return { cn: 0, en: 0 };
  return { cn: (rec.cn || 0) * 80, en: (rec.en || 0) * 80 };
}

function analyze(raw) {
  const key = raw.toLowerCase().replace(/[^a-z]/g, '');
  const boost = learnedBoost(key);
  return ime.analyze(raw, { cnBoost: boost.cn, enBoost: boost.en });
}

function escapeHtml(s) {
  return s.replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

function render() {
  const raw = input.value;
  const result = analyze(raw);
  lastResults = result.results;

  let sig = result.signals.map(([s, c]) => `<span class="signal ${c}">${s}</span>`).join('');
  if (pauses.some(x => x >= 520)) sig += '<span class="signal active">检测到输入停顿</span>';
  signals.innerHTML = sig;

  contextLabel.textContent = result.context;
  count.textContent = result.results.length ? `· ${result.results.length} 个` : '';

  if (!result.results.length) {
    candidates.innerHTML = '<div class="empty">输入后，这里会同时分析中文和英文候选</div>';
    return;
  }
  selectedIndex = Math.min(selectedIndex, result.results.length - 1);
  candidates.innerHTML = result.results.map((r, i) =>
    `<button class="candidate ${i === selectedIndex ? 'selected' : ''}" data-index="${i}">` +
    `<span class="num">${i + 1}</span><div class="text">${escapeHtml(r.text)}</div>` +
    `<div class="meta">${escapeHtml(r.type)} · ${i === 0 ? '优先候选' : '备选'}</div></button>`
  ).join('');

  candidates.querySelectorAll('button').forEach(btn => btn.addEventListener('click', () => {
    selectedIndex = Number(btn.dataset.index);
    render();
    commit(false);
  }));
}

function commit(addSpace) {
  if (!lastResults.length) return;
  const chosen = lastResults[selectedIndex];
  const key = input.value.toLowerCase().replace(/[^a-z]/g, '');
  const lang = chosen.lang === 'en' ? 'en' : (chosen.lang === 'cn' ? 'cn' : 'mixed');
  learned[key] = learned[key] || {};
  learned[key][lang] = (learned[key][lang] || 0) + 1;
  localStorage.setItem('tongjian-learned', JSON.stringify(learned));
  learnCount.textContent = `已记录 ${Object.keys(learned).length} 组输入偏好`;

  input.setRangeText(chosen.text + (addSpace ? ' ' : ''), 0, input.value.length, 'end');
  pauses = [];
  render();
  input.focus();
}

input.addEventListener('input', () => {
  const now = Date.now();
  if (lastKeyAt && now - lastKeyAt > 520) pauses.push(now - lastKeyAt);
  lastKeyAt = now;
  selectedIndex = 0;
  render();
});

input.addEventListener('keydown', e => {
  if (e.key === 'Enter') {
    e.preventDefault();
    commit(true);
  } else if (e.key === 'ArrowDown' || e.key === 'ArrowUp') {
    e.preventDefault();
    if (!lastResults.length) return;
    selectedIndex = (selectedIndex + (e.key === 'ArrowDown' ? 1 : -1) + lastResults.length) % lastResults.length;
    render();
  }
});

document.querySelectorAll('.sample').forEach(b => b.addEventListener('click', () => {
  input.value = b.dataset.value;
  input.focus();
  render();
}));

if (Object.keys(learned).length) learnCount.textContent = `已记录 ${Object.keys(learned).length} 组输入偏好`;
