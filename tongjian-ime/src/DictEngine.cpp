#include "DictEngine.h"
#include "globals.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_set>
#include <utility>

namespace {

const double kFuzzyMargin = 150.0;
const double kTypoMargin = 300.0;
const double kTypoImproveThreshold = -800.0;

bool IsVowel(char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'v';
}

// 替换字符串中所有 from -> to
std::string ReplaceAll(const std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    std::string out;
    size_t pos = 0;
    for (;;) {
        size_t f = s.find(from, pos);
        if (f == std::string::npos) { out += s.substr(pos); break; }
        out += s.substr(pos, f - pos);
        out += to;
        pos = f + from.size();
    }
    return out;
}

} // namespace

double DictEngine::WordProbCN(int weight) const {
    return 100.0 * std::log((double)weight) - cn_total_;
}
double DictEngine::WordProbEN(int freq) const {
    return 100.0 * std::log((double)freq) - en_total_;
}

bool DictEngine::Load(const unsigned char* data, size_t size) {
    cn_.clear(); en_.clear(); max_key_len_ = 0;
    const unsigned char* p = data;
    const unsigned char* e = data + size;

    auto need = [&](size_t n) { return (size_t)(e - p) >= n; };
    auto rd_u8 = [&](uint8_t& v) { if (!need(1)) return false; v = *p++; return true; };
    auto rd_u16 = [&](uint16_t& v) { if (!need(2)) return false; memcpy(&v, p, 2); p += 2; return true; };
    auto rd_u32 = [&](uint32_t& v) { if (!need(4)) return false; memcpy(&v, p, 4); p += 4; return true; };
    auto rd_dbl = [&](double& v) { if (!need(8)) return false; memcpy(&v, p, 8); p += 8; return true; };

    if (!need(8)) return false;
    if (memcmp(p, "TJIE", 4) != 0) return false; p += 4;
    uint32_t version = 0;
    if (!rd_u32(version)) return false;

    uint32_t cnCount = 0;
    if (!rd_u32(cnCount)) return false;
    for (uint32_t i = 0; i < cnCount; ++i) {
        uint8_t klen = 0;
        if (!rd_u8(klen) || !need(klen)) return false;
        std::string key((const char*)p, klen); p += klen;
        uint16_t n = 0;
        if (!rd_u16(n)) return false;
        std::vector<std::pair<std::string, int>> v;
        v.reserve(n);
        for (uint16_t j = 0; j < n; ++j) {
            uint8_t hlen = 0;
            if (!rd_u8(hlen) || !need(hlen)) return false;
            std::string hanzi((const char*)p, hlen); p += hlen;
            uint32_t w = 0;
            if (!rd_u32(w)) return false;
            v.emplace_back(std::move(hanzi), (int)w);
        }
        if ((int)key.size() > max_key_len_) max_key_len_ = (int)key.size();
        cn_[std::move(key)] = std::move(v);
    }
    if (!rd_dbl(cn_total_)) return false;

    uint32_t enCount = 0;
    if (!rd_u32(enCount)) return false;
    for (uint32_t i = 0; i < enCount; ++i) {
        uint8_t wlen = 0;
        if (!rd_u8(wlen) || !need(wlen)) return false;
        std::string word((const char*)p, wlen); p += wlen;
        uint32_t f = 0;
        if (!rd_u32(f)) return false;
        en_[std::move(word)] = (int)f;
    }
    if (!rd_dbl(en_total_)) return false;

    return true;
}

bool DictEngine::SegmentScore(const std::string& s, std::vector<std::string>& keys, double& score) const {
    int n = (int)s.size();
    if (n == 0) return false;
    std::vector<double> best(n + 1, -1e18);
    std::vector<int> back(n + 1, -1);
    best[0] = 0.0;
    for (int i = 0; i < n; ++i) {
        if (best[i] <= -1e17) continue;
        int maxJ = n;
        if (maxJ > i + max_key_len_) maxJ = i + max_key_len_;
        for (int j = i + 1; j <= maxJ; ++j) {
            std::string key = s.substr(i, j - i);
            auto it = cn_.find(key);
            if (it == cn_.end() || it->second.empty()) continue;
            double sc = WordProbCN(it->second[0].second);
            double cand = best[i] + sc;
            if (cand > best[j]) { best[j] = cand; back[j] = i; }
        }
    }
    if (best[n] <= -1e17) return false;
    keys.clear();
    int i = n;
    while (i > 0) { int p = back[i]; keys.push_back(s.substr(p, i - p)); i = p; }
    std::reverse(keys.begin(), keys.end());
    score = best[n];
    return true;
}

std::vector<Candidate> DictEngine::Beam(const std::vector<std::string>& keys, int topN) const {
    struct B { std::wstring text; double score; };
    std::vector<B> beams;
    beams.push_back({ L"", 0.0 });
    for (const auto& key : keys) {
        auto it = cn_.find(key);
        if (it == cn_.end()) return {};
        const auto& opts = it->second;
        std::vector<B> nb;
        for (const auto& b : beams) {
            int m = (int)opts.size();
            if (m > 6) m = 6;
            for (int k = 0; k < m; ++k) {
                double wp = WordProbCN(opts[k].second);
                nb.push_back({ b.text + Utf8ToWide(opts[k].first), b.score + wp });
            }
        }
        std::sort(nb.begin(), nb.end(), [](const B& a, const B& b) { return a.score > b.score; });
        if ((int)nb.size() > topN) nb.resize(topN);
        beams = std::move(nb);
    }
    std::vector<Candidate> out;
    out.reserve(beams.size());
    for (auto& b : beams) out.push_back({ b.text, (int)std::llround(b.score) });
    return out;
}

std::vector<std::string> DictEngine::FuzzyCandidates(const std::string& s) const {
    std::unordered_set<std::string> seen;
    std::vector<std::string> out;
    auto add = [&](const std::string& x) {
        if (!x.empty() && x != s && seen.insert(x).second) out.push_back(x);
    };

    const std::pair<const char*, const char*> digraphs[] = {
        { "zh", "z" }, { "ch", "c" }, { "sh", "s" }
    };
    for (auto& pr : digraphs) {
        add(ReplaceAll(s, pr.first, pr.second));
        add(ReplaceAll(s, pr.second, pr.first));
    }
    const std::pair<char, char> initials[] = {
        { 'n', 'l' }, { 'f', 'h' }, { 'r', 'l' }
    };
    for (auto& pr : initials) {
        if (!s.empty() && s[0] == pr.first) add(std::string(1, pr.second) + s.substr(1));
        if (!s.empty() && s[0] == pr.second) add(std::string(1, pr.first) + s.substr(1));
    }
    // 前后鼻音：ng -> n
    size_t i = s.find("ng");
    while (i != std::string::npos) {
        add(s.substr(0, i) + "n" + s.substr(i + 2));
        i = s.find("ng", i + 1);
    }
    // 前后鼻音：n -> ng（仅音节末尾）
    for (size_t j = 0; j < s.size(); ++j) {
        if (s[j] == 'n') {
            char nxt = (j + 1 < s.size()) ? s[j + 1] : '\0';
            if (nxt == '\0' || !IsVowel(nxt)) add(s.substr(0, j) + "ng" + s.substr(j + 1));
        }
    }
    return out;
}

std::vector<std::string> DictEngine::TypoCandidates(const std::string& s) const {
    std::unordered_set<std::string> seen;
    std::vector<std::string> out;
    auto add = [&](const std::string& x) {
        if (!x.empty() && x != s && seen.insert(x).second) out.push_back(x);
    };
    const std::string alpha = "abcdefghijklmnopqrstuvwxyz";
    for (size_t i = 0; i < s.size(); ++i) {              // substitute
        std::string pre = s.substr(0, i), suf = s.substr(i + 1);
        for (char c : alpha) if (c != s[i]) add(pre + c + suf);
    }
    for (size_t i = 0; i <= s.size(); ++i) {             // insert
        std::string pre = s.substr(0, i), suf = s.substr(i);
        for (char c : alpha) add(pre + c + suf);
    }
    for (size_t i = 0; i < s.size(); ++i) add(s.substr(0, i) + s.substr(i + 1)); // delete
    for (size_t i = 0; i + 1 < s.size(); ++i) {          // transpose
        std::string t = s;
        std::swap(t[i], t[i + 1]);
        add(t);
    }
    return out;
}

std::vector<std::string> DictEngine::Segment(const std::string& s) const {
    std::vector<std::string> keys;
    double score = 0.0;
    if (SegmentScore(s, keys, score)) return keys;
    return {};
}

AnalyzeResult DictEngine::Analyze(const std::string& input) const {
    AnalyzeResult r;

    // 小写、仅保留 a-z
    std::string s;
    s.reserve(input.size());
    for (char c : input) {
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c >= 'a' && c <= 'z') s += c;
    }
    if (s.empty()) return r;

    std::vector<std::string> keys;
    double score = 0.0;
    bool ok = SegmentScore(s, keys, score);

    auto enIt = en_.find(s);
    bool hasEn = (enIt != en_.end());
    double enScore = hasEn ? WordProbEN(enIt->second) : -1e18;

    std::string used = s;   // 纠正后实际使用的拼音串

    // 纠错/模糊音：仅当该串不是已识别的英文词、且原切分缺失或偏弱
    if (!hasEn && (!ok || score < kTypoImproveThreshold)) {
        std::vector<std::string> fuzzy = FuzzyCandidates(s);
        std::vector<std::string> typo = (s.size() <= 14) ? TypoCandidates(s) : std::vector<std::string>();

        std::string bf, bt;
        double bfs = -1e18, bts = -1e18;
        std::vector<std::string> bfkeys, btkeys;
        for (const auto& c : fuzzy) {
            std::vector<std::string> k2; double sc = 0.0;
            if (SegmentScore(c, k2, sc) && sc > bfs) { bfs = sc; bf = c; bfkeys = std::move(k2); }
        }
        for (const auto& c : typo) {
            std::vector<std::string> k2; double sc = 0.0;
            if (SegmentScore(c, k2, sc) && sc > bts) { bts = sc; bt = c; btkeys = std::move(k2); }
        }

        std::wstring kind;
        if (!ok) {
            if (!bt.empty()) { keys = std::move(btkeys); score = bts; used = bt; ok = true; kind = L"纠正"; }
            else if (!bf.empty()) { keys = std::move(bfkeys); score = bfs; used = bf; ok = true; kind = L"模糊音"; }
        } else {
            if (!bf.empty() && bfs > score + kFuzzyMargin) {
                keys = std::move(bfkeys); score = bfs; used = bf; kind = L"模糊音";
            } else if (!bt.empty() && bts > score + kTypoMargin) {
                keys = std::move(btkeys); score = bts; used = bt; kind = L"纠正";
            }
        }
        if (!kind.empty()) {
            r.corrected = true;
            r.correction = kind + L"：" + Utf8ToWide(s) + L" → " + Utf8ToWide(used);
        }
    }

    if (ok) {
        auto cnCands = Beam(keys, kMaxCandidates);
        for (auto& c : cnCands) r.candidates.push_back(std::move(c));
    }
    if (hasEn) {
        Candidate e;
        e.text = Utf8ToWide(s);
        e.score = (int)std::llround(enScore);
        r.candidates.push_back(std::move(e));
    }
    std::sort(r.candidates.begin(), r.candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.score > b.score; });

    r.ambiguous = hasEn && ok;
    return r;
}
