/* 同键 (Tongjian) 中英混输核心逻辑 — 纯函数，可运行于浏览器与 Node */
(function (global) {
  'use strict';

  let D = null;
  let KEY_SCORE = null;   // pinyin key -> top word log-prob (x100)
  let MAX_KEY_LEN = 0;

  const LN = new Map();
  function ln(x) {
    let v = LN.get(x);
    if (v === undefined) {
      v = Math.log(x);
      if (LN.size > 300000) LN.clear();
      LN.set(x, v);
    }
    return v;
  }

  const BIGRAM_W = 0.4;   // weight of the character-bigram context model
  const BACKOFF = -1050;  // default log-prob for an unseen char transition

  function configure(dict) {
    D = dict;
    KEY_SCORE = new Map();
    MAX_KEY_LEN = 0;
    for (const k in D.cn) {
      const arr = D.cn[k];
      if (arr && arr.length) {
        KEY_SCORE.set(k, wordProbCN(arr[0][1]));
        if (k.length > MAX_KEY_LEN) MAX_KEY_LEN = k.length;
      }
    }
  }

  function wordProbCN(weight) { return 100 * ln(weight) - D.cn_total; }
  function wordProbEN(freq)   { return 100 * ln(freq) - D.en_total; }

  function isCJK(ch) { return ch >= '\u3400' && ch <= '\u9fff'; }

  function bigramScore(prev, next) {
    const rows = D.bigram[prev];
    if (rows) {
      for (let i = 0; i < rows.length; i++) {
        if (rows[i][0] === next) return rows[i][1];
      }
    }
    const u = D.uni[next];
    return u !== undefined ? u : BACKOFF;
  }

  // ---- pinyin segmentation: DP over dictionary keys, maximizing unigram LM ----
  function segmentPinyin(s) {
    const n = s.length;
    if (!n) return null;
    const best = new Float64Array(n + 1).fill(-Infinity);
    const back = new Int32Array(n + 1).fill(-1);
    best[0] = 0;
    for (let i = 0; i < n; i++) {
      if (best[i] === -Infinity) continue;
      const maxJ = n < i + MAX_KEY_LEN ? n : i + MAX_KEY_LEN;
      for (let j = i + 1; j <= maxJ; j++) {
        const sc = KEY_SCORE.get(s.slice(i, j));
        if (sc === undefined) continue;
        const cand = best[i] + sc;
        if (cand > best[j]) { best[j] = cand; back[j] = i; }
      }
    }
    if (best[n] === -Infinity) return null;
    const seg = [];
    let i = n;
    while (i > 0) { const p = back[i]; seg.push(s.slice(p, i)); i = p; }
    seg.reverse();
    return { seg, score: best[n] };
  }

  // ================= 拼音容错 + 按键纠错 =================
  function isVowelCh(ch) { return ch === 'a' || ch === 'e' || ch === 'i' || ch === 'o' || ch === 'u' || ch === 'v'; }
  function replaceAllStr(s, from, to) { return s.split(from).join(to); }

  const FUZZY_DIGRAPH = [['zh', 'z'], ['ch', 'c'], ['sh', 's']];   // 平翘舌（可全局替换，zh/ch/sh 只作声母）
  const FUZZY_INITIAL = [['n', 'l'], ['f', 'h'], ['r', 'l']];      // 声母模糊（只在词首替换，避免误伤韵尾 n）
  const FUZZY_MARGIN = 150;   // 模糊音需比原切分高出这么多才覆盖
  const TYPO_MARGIN = 300;    // 按键纠错需比原切分高出这么多才覆盖
  const TYPO_IMPROVE_THRESHOLD = -800;  // 原切分得分低于此值才尝试按键纠错

  // 由 s 经常见模糊音变换得到的候选串（是否合法由后续切分验证）
  function fuzzyCandidates(s) {
    const out = [];
    const add = x => { if (x && x !== s) out.push(x); };
    for (const [a, b] of FUZZY_DIGRAPH) {
      add(replaceAllStr(s, a, b));
      add(replaceAllStr(s, b, a));
    }
    for (const [a, b] of FUZZY_INITIAL) {
      if (s.startsWith(a)) add(b + s.slice(1));
      if (s.startsWith(b)) add(a + s.slice(1));
    }
    // 前后鼻音：ng -> n（任意位置，ng 只作韵尾）
    let i = s.indexOf('ng');
    while (i >= 0) { add(s.slice(0, i) + 'n' + s.slice(i + 2)); i = s.indexOf('ng', i + 1); }
    // 前后鼻音：n -> ng（仅当 n 处于音节末尾）
    for (let j = 0; j < s.length; j++) {
      if (s[j] === 'n') {
        const nxt = s[j + 1];
        if (nxt === undefined || !isVowelCh(nxt)) add(s.slice(0, j) + 'ng' + s.slice(j + 1));
      }
    }
    return out;
  }

  // 由 s 经一次按键错误（编辑距离 1：替换/插入/删除/交换）得到的候选串
  function typoCandidates(s) {
    const out = [];
    const seen = new Set();
    const add = x => { if (x && x !== s && !seen.has(x)) { seen.add(x); out.push(x); } };
    const alpha = 'abcdefghijklmnopqrstuvwxyz';
    for (let i = 0; i < s.length; i++) {           // substitute
      const pre = s.slice(0, i), suf = s.slice(i + 1);
      for (let k = 0; k < 26; k++) if (alpha[k] !== s[i]) add(pre + alpha[k] + suf);
    }
    for (let i = 0; i <= s.length; i++) {          // insert
      const pre = s.slice(0, i), suf = s.slice(i);
      for (let k = 0; k < 26; k++) add(pre + alpha[k] + suf);
    }
    for (let i = 0; i < s.length; i++) add(s.slice(0, i) + s.slice(i + 1)); // delete
    for (let i = 0; i < s.length - 1; i++) add(s.slice(0, i) + s[i + 1] + s[i] + s.slice(i + 2)); // transpose
    return out;
  }

  // beam search over a segmentation, producing top-N Chinese strings
  function beamCandidates(seg, prevChar, topN) {
    let beams = [{ text: '', score: 0 }];
    for (const key of seg) {
      const opts = D.cn[key];
      if (!opts) return [];
      const nb = [];
      for (const b of beams) {
        const prev = b.text.length ? b.text[b.text.length - 1] : (prevChar || '');
        const m = opts.length < 6 ? opts.length : 6;
        for (let k = 0; k < m; k++) {
          const ch = opts[k][0];
          const wp = wordProbCN(opts[k][1]);
          nb.push({ text: b.text + ch, score: b.score + wp + BIGRAM_W * bigramScore(prev, ch) });
        }
      }
      nb.sort((a, b) => b.score - a.score);
      beams = nb.slice(0, topN);
    }
    return beams;
  }

  function tokenize(raw) {
    const re = /[A-Za-z]+|[\u3400-\u9fff]+|\s+|[^A-Za-z\u3400-\u9fff\s]+/g;
    const m = raw.match(re) || [];
    return m.map(function (t) {
      if (/^[A-Za-z]+$/.test(t)) return { type: 'latin', text: t, low: t.toLowerCase() };
      if (/^[\u3400-\u9fff]+$/.test(t)) return { type: 'cn', text: t };
      if (/^\s+$/.test(t)) return { type: 'space', text: t };
      return { type: 'other', text: t };
    });
  }

  function resolveLatin(t) {
    const low = t.low;
    const enFreq = D.en[low];
    const enScore = enFreq !== undefined ? wordProbEN(enFreq) : -Infinity;

    let seg = segmentPinyin(low);
    let correction = null;   // {from, to, kind: 'typo'|'fuzzy'}

    // 仅当该串不是已识别的英文词、且原切分缺失或偏弱时，才做拼音容错/纠错
    if (enFreq === undefined && (seg === null || seg.score < TYPO_IMPROVE_THRESHOLD)) {
      let bestFuzzy = null, bestTypo = null;
      for (const c of fuzzyCandidates(low)) {
        const sg = segmentPinyin(c);
        if (sg && (!bestFuzzy || sg.score > bestFuzzy.score)) bestFuzzy = { text: c, seg: sg, score: sg.score };
      }
      if (low.length <= 14) {
        for (const c of typoCandidates(low)) {
          const sg = segmentPinyin(c);
          if (sg && (!bestTypo || sg.score > bestTypo.score)) bestTypo = { text: c, seg: sg, score: sg.score };
        }
      }
      if (seg === null) {
        // 无法切分：优先按键纠错，其次模糊音
        if (bestTypo) {
          seg = bestTypo.seg;
          correction = { from: low, to: bestTypo.text, kind: 'typo' };
        } else if (bestFuzzy) {
          seg = bestFuzzy.seg;
          correction = { from: low, to: bestFuzzy.text, kind: 'fuzzy' };
        }
      } else {
        // 可切分但偏弱：优先按模糊音解释，其次按键纠错
        if (bestFuzzy && bestFuzzy.score > seg.score + FUZZY_MARGIN) {
          seg = bestFuzzy.seg;
          correction = { from: low, to: bestFuzzy.text, kind: 'fuzzy' };
        } else if (bestTypo && bestTypo.score > seg.score + TYPO_MARGIN) {
          seg = bestTypo.seg;
          correction = { from: low, to: bestTypo.text, kind: 'typo' };
        }
      }
    }

    const cn = seg ? beamCandidates(seg.seg, '', 5) : [];
    const cnScore = seg ? seg.score : -Infinity;
    return {
      seg, cn, cnScore, enScore, correction,
      cnText: cn.length ? cn[0].text : null,
      enText: enFreq !== undefined ? t.text : null,   // keep original casing
      ambiguous: !!cn.length && enFreq !== undefined,
      raw: t.text
    };
  }

  // compose output text: replace latin tokens via getText, keep the user's own
  // spacing, but drop a space that sits between two resolved Chinese chunks
  // ("wo xiang" -> "我想"); keep spaces around English words and punctuation.
  function compose(tokens, getText) {
    const out = [];
    let li = 0;
    for (const t of tokens) {
      if (t.type === 'latin') {
        const txt = getText(t, li); li++;
        const isCn = !!txt && /^[\u3400-\u9fff]+$/.test(txt);
        out.push({ text: txt, kind: isCn ? 'cn' : 'latin' });
      } else if (t.type === 'cn') {
        out.push({ text: t.text, kind: 'cn' });
      } else if (t.type === 'space') {
        out.push({ text: ' ', kind: 'space' });
      } else {
        out.push({ text: t.text, kind: 'other' });
      }
    }
    let s = '';
    for (let i = 0; i < out.length; i++) {
      const it = out[i];
      if (it.kind === 'space') {
        const prev = i > 0 ? out[i - 1] : null;
        const next = i < out.length - 1 ? out[i + 1] : null;
        if (prev && next && prev.kind === 'cn' && next.kind === 'cn') continue; // drop
      }
      s += it.text;
    }
    return s.trim();
  }

  function analyze(raw, opts) {
    opts = opts || {};
    const cnBoost = opts.cnBoost || 0;
    const enBoost = opts.enBoost || 0;

    const tokens = tokenize(raw);
    const latinTokens = tokens.filter(t => t.type === 'latin');
    const latinInfo = latinTokens.map(resolveLatin);
    const hasCJK = tokens.some(t => t.type === 'cn');

    const CN_CTX = 120, EN_CTX = 120;
    let ctx = 0;
    const latinLang = latinInfo.map(function (info) {
      let lang;
      if (info.cnText && info.enText) {
        const cn = info.cnScore + (ctx > 0 ? CN_CTX : 0) + cnBoost;
        const en = info.enScore + (ctx < 0 ? EN_CTX : 0) + enBoost;
        lang = en >= cn ? 'en' : 'cn';
      } else if (info.enText) lang = 'en';
      else if (info.cnText) lang = 'cn';
      else lang = 'raw';
      ctx = lang === 'cn' ? 1 : lang === 'en' ? -1 : ctx;
      return lang;
    });

    const mixedText = compose(tokens, function (t, li) {
      const lang = latinLang[li], info = latinInfo[li];
      if (lang === 'en') return t.text;
      if (lang === 'cn') return info.cnText;
      return t.text;
    });

    // signals
    const signals = [];
    if (/\s/.test(raw.trim())) signals.push(['空格分词', 'active']);
    if (latinInfo.some(i => i.cnText)) signals.push(['拼音 / 中文可识别', 'active']);
    if (latinInfo.some(i => i.enText)) signals.push(['英文词匹配', 'active']);
    if (latinInfo.some(i => i.ambiguous)) signals.push(['存在双语歧义', 'amb']);
    if (hasCJK) signals.push(['中文片段保留', 'active']);
    // correction reminders (拼音容错 / 按键纠错提示)
    for (const info of latinInfo) {
      if (info.correction) {
        const label = info.correction.kind === 'fuzzy' ? '模糊音' : '拼写纠正';
        signals.push([`${label}：${info.correction.from} → ${info.correction.to}`, 'amb']);
      }
    }

    let results = [], context;

    if (latinTokens.length === 1 && !hasCJK) {
      // single token -> surface every Chinese candidate + the English word
      const info = latinInfo[0], t = latinTokens[0];
      const cnType = info.correction ? '拼音转写 · 已纠正' : '拼音转写 · 中文';
      for (const c of info.cn) {
        results.push({ text: c.text, type: cnType, score: c.score + cnBoost, lang: 'cn' });
      }
      if (info.enText) {
        results.push({ text: t.text, type: '英文词', score: info.enScore + enBoost, lang: 'en' });
      }
      if (!results.length) {
        results.push({ text: t.text, type: '原文', score: -1e9, lang: 'raw' });
      }
      results.sort((a, b) => b.score - a.score);
      if (info.cnText && info.enText) {
        context = Math.abs(info.cnScore - info.enScore) < 220
          ? '中英歧义，请选择'
          : (info.enScore > info.cnScore ? '英文语境优先' : '中文语境优先');
      } else {
        context = info.cnText ? '中文语境优先' : (info.enText ? '英文语境优先' : '无法识别');
      }
    } else {
      results.push({ text: mixedText, type: '中英混输', score: 0, lang: 'mixed' });
      // only offer a flip-alternative when the two readings are genuinely close
      const AMB_THRESHOLD = 220;
      const ambIdx = [];
      latinInfo.forEach((info, i) => {
        if (info.ambiguous && Math.abs(info.cnScore - info.enScore) < AMB_THRESHOLD) ambIdx.push(i);
      });
      for (const ai of ambIdx.slice(0, 2)) {
        const variant = compose(tokens, function (t, li) {
          if (li === ai) {
            const cur = latinLang[li], info = latinInfo[li];
            return cur === 'en' ? info.cnText : (info.enText || t.text);
          }
          const lang = latinLang[li], info = latinInfo[li];
          if (lang === 'en') return t.text;
          if (lang === 'cn') return info.cnText;
          return t.text;
        });
        if (variant !== mixedText) results.push({ text: variant, type: '备选 · 切换', score: -1, lang: 'alt' });
      }
      context = ctx > 0 ? '中文语境优先' : ctx < 0 ? '英文语境优先' : '结合拼写判断';
    }

    return { results, signals, context };
  }

  const api = { configure, analyze, tokenize, segmentPinyin, beamCandidates, fuzzyCandidates, typoCandidates };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  global.TongjianIME = api;
})(typeof window !== 'undefined' ? window : globalThis);
