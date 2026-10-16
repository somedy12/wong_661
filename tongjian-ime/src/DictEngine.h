#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <utility>

struct Candidate {
    std::wstring text;   // 候选文本（汉字 或 英文）
    int score = 0;       // 分数，越高越好

    Candidate() = default;
    Candidate(std::wstring t, int s) : text(std::move(t)), score(s) {}
};

struct AnalyzeResult {
    std::vector<Candidate> candidates;  // 按分数降序
    std::wstring correction;            // 纠错/模糊音提示，如 "niha → nihao"，空则无
    bool corrected = false;
    bool ambiguous = false;             // 是否中英两可
};

// 中英混输词典引擎：加载 dict.bin，完成拼音切分、候选生成、模糊音、按键纠错、中英判别
class DictEngine {
public:
    bool Load(const unsigned char* data, size_t size);

    // 分析一段小写拼音/英文串，返回候选与纠错提示
    AnalyzeResult Analyze(const std::string& input) const;

    // 拼音 DP 切分（返回音节序列，供调试）
    std::vector<std::string> Segment(const std::string& s) const;

    bool IsLoaded() const { return !cn_.empty(); }

private:
    // 拼音键 -> 候选 (hanzi, weight)
    std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> cn_;
    // 英文词 -> 频次
    std::unordered_map<std::string, int> en_;
    double cn_total_ = 0.0;   // 100*ln(sum cn weights)
    double en_total_ = 0.0;   // 100*ln(sum en freqs)
    int max_key_len_ = 0;

    double WordProbCN(int weight) const;
    double WordProbEN(int freq) const;

    // 若可完整切分返回 true 并填 keys/score
    bool SegmentScore(const std::string& s, std::vector<std::string>& keys, double& score) const;

    // 对给定切分做 beam 搜索，产出 topN 个汉字候选
    std::vector<Candidate> Beam(const std::vector<std::string>& keys, int topN) const;

    // 模糊音候选串
    std::vector<std::string> FuzzyCandidates(const std::string& s) const;
    // 按键纠错候选串（编辑距离 1）
    std::vector<std::string> TypoCandidates(const std::string& s) const;
};
