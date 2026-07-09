#pragma once
#include "index.h"
#include "eval.h"
#include <vector>
#include <string>
#include <unordered_map>

using Features = std::vector<double>;

struct LTRSample {
    int docId;
    std::string passageId;
    Features feats;
    double relevance;
};

class LTR {
public:
    static constexpr int NFEATS = 6;
    // Features: [bm25, ql, vsm, doc_len_norm, query_coverage, avg_idf]

    explicit LTR(const Index& idx);

    // Train using qrel + a map of qid -> query text.
    // Supply as many (qid, query_string) pairs as you have; others are skipped.
    void train(const Qrel& qrel,
               const std::unordered_map<int, std::string>& queryMap,
               int ndcgK = 10, int epochs = 20);

    RankedList rerank(const std::string& query, const RankedList& candidates) const;

    Features extractFeatures(const std::string& query, int docId) const;
    const std::vector<double>& weights() const { return w; }

private:
    const Index& index;
    std::vector<double> w;

    // Per-feature normalization params (mean, std) fitted during training
    struct FeatStats { double mean{0}, std{1}; };
    std::vector<FeatStats> featStats;

    double score(const Features& f) const;
    Features normalizeFeats(const Features& f) const;

    double bm25Doc(const std::string& query, int docId) const;
    double qlDoc(const std::string& query, int docId) const;
    double vsmDoc(const std::string& query, int docId) const;
    double queryCoverage(const std::string& query, int docId) const;
    double avgIDF(const std::string& query) const;

    static double ndcgAtK(const std::vector<LTRSample>& sorted, int k);
    double evalMeanNDCG(const std::vector<std::pair<int, std::vector<LTRSample>>>& data, int k) const;
    void fitNormStats(std::vector<std::pair<int, std::vector<LTRSample>>>& data);
};
