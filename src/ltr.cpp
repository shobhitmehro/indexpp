#include "ltr.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>
#include <numeric>

LTR::LTR(const Index& idx) : index{idx}, w(NFEATS, 1.0 / NFEATS), featStats(NFEATS) {}

// ─── per-doc scoring helpers (BM25 / QL / VSM) ─────────────────────────────

double LTR::bm25Doc(const std::string& query, int docId) const {
    const double b = 0.75, k1 = 1.2;
    double s = 0.0;
    double dl = index.docLenMap.count(docId) ? index.docLenMap.at(docId) : index.avgdl;
    std::istringstream iss(query);
    std::string term;
    while (std::getline(iss, term, ' ')) {
        auto it = index.invertedIndex.find(term);
        if (it == index.invertedIndex.end()) continue;
        auto pit = it->second.find(docId);
        double tf  = pit != it->second.end() ? pit->second : 0.0;
        double df  = static_cast<double>(it->second.size());
        double N   = static_cast<double>(index.docLenMap.size());
        double idf = std::log((N - df + 0.5) / (df + 0.5));
        double tfn = (k1 + 1.0) * tf / (k1 * (1.0 - b + b * dl / index.avgdl) + tf);
        s += tfn * idf;
    }
    return s;
}

double LTR::qlDoc(const std::string& query, int docId) const {
    const double lambda = 0.2;
    double s = 0.0;
    double dl = index.docLenMap.count(docId) ? index.docLenMap.at(docId) : 1.0;
    double N  = static_cast<double>(index.totalTerms);
    std::istringstream iss(query);
    std::string term;
    while (std::getline(iss, term, ' ')) {
        auto it = index.invertedIndex.find(term);
        if (it == index.invertedIndex.end()) continue;
        double cfreq = 0.0;
        for (const auto& [pid, cnt] : it->second) cfreq += cnt;
        double p_c = cfreq / N;
        auto pit = it->second.find(docId);
        double tf = pit != it->second.end() ? pit->second : 0.0;
        double p  = (1.0 - lambda) * tf / dl + lambda * p_c;
        if (p > 0.0) s += std::log(p);
    }
    return s;
}

double LTR::vsmDoc(const std::string& query, int docId) const {
    const double b = 0.75;
    double s = 0.0;
    double dl    = index.docLenMap.count(docId) ? index.docLenMap.at(docId) : index.avgdl;
    double avgdl = static_cast<double>(index.avgdl);
    double C     = static_cast<double>(index.numPassages);
    std::unordered_map<std::string, int> qTF;
    std::istringstream iss(query);
    std::string term;
    while (std::getline(iss, term, ' ')) qTF[term]++;
    for (const auto& [t, qcnt] : qTF) {
        auto it = index.invertedIndex.find(t);
        if (it == index.invertedIndex.end()) continue;
        double df   = static_cast<double>(it->second.size());
        double idf  = std::log((C + 1.0) / df);
        auto pit = it->second.find(docId);
        double tf   = pit != it->second.end() ? pit->second : 0.0;
        double logtf = std::log(1.0 + std::log(1.0 + tf));
        double norm  = 1.0 - b + b * dl / avgdl;
        s += static_cast<double>(qcnt) * logtf / norm * idf;
    }
    return s;
}

double LTR::queryCoverage(const std::string& query, int docId) const {
    std::istringstream iss(query);
    std::string term;
    int total = 0, found = 0;
    while (std::getline(iss, term, ' ')) {
        ++total;
        auto it = index.invertedIndex.find(term);
        if (it != index.invertedIndex.end() && it->second.count(docId)) ++found;
    }
    return total > 0 ? static_cast<double>(found) / total : 0.0;
}

double LTR::avgIDF(const std::string& query) const {
    std::istringstream iss(query);
    std::string term;
    double sum = 0.0;
    int cnt = 0;
    double N = static_cast<double>(index.docLenMap.size());
    while (std::getline(iss, term, ' ')) {
        ++cnt;
        auto it = index.invertedIndex.find(term);
        if (it != index.invertedIndex.end()) {
            double df = static_cast<double>(it->second.size());
            sum += std::log((N - df + 0.5) / (df + 0.5));
        }
    }
    return cnt > 0 ? sum / cnt : 0.0;
}

Features LTR::extractFeatures(const std::string& query, int docId) const {
    double dl = index.docLenMap.count(docId) ? index.docLenMap.at(docId) : index.avgdl;
    return {
        bm25Doc(query, docId),
        qlDoc(query, docId),
        vsmDoc(query, docId),
        dl / static_cast<double>(index.avgdl),
        queryCoverage(query, docId),
        avgIDF(query)
    };
}

// ─── feature normalization ───────────────────────────────────────────────────

void LTR::fitNormStats(std::vector<std::pair<int, std::vector<LTRSample>>>& data) {
    featStats.assign(NFEATS, {0.0, 1.0});
    int total = 0;
    for (auto& [qid, samples] : data)
        for (auto& s : samples) { total++; for (int f = 0; f < NFEATS; ++f) featStats[f].mean += s.feats[f]; }
    if (!total) return;
    for (int f = 0; f < NFEATS; ++f) featStats[f].mean /= total;
    double sq[NFEATS] = {};
    for (auto& [qid, samples] : data)
        for (auto& s : samples)
            for (int f = 0; f < NFEATS; ++f) { double d = s.feats[f] - featStats[f].mean; sq[f] += d*d; }
    for (int f = 0; f < NFEATS; ++f)
        featStats[f].std = std::sqrt(std::max(sq[f] / total, 1e-8));
    // Apply to all samples
    for (auto& [qid, samples] : data)
        for (auto& s : samples)
            for (int f = 0; f < NFEATS; ++f)
                s.feats[f] = (s.feats[f] - featStats[f].mean) / featStats[f].std;
}

Features LTR::normalizeFeats(const Features& f) const {
    Features out(NFEATS);
    for (int i = 0; i < NFEATS; ++i)
        out[i] = (f[i] - featStats[i].mean) / featStats[i].std;
    return out;
}

// ─── scoring & NDCG ─────────────────────────────────────────────────────────

double LTR::score(const Features& f) const {
    double s = 0.0;
    for (int i = 0; i < NFEATS; ++i) s += w[i] * f[i];
    return s;
}

// Graded NDCG: gain = 2^rel - 1
double LTR::ndcgAtK(const std::vector<LTRSample>& sorted, int k) {
    int n = std::min(k, static_cast<int>(sorted.size()));
    double dcg = 0.0;
    for (int i = 0; i < n; ++i)
        dcg += (std::pow(2.0, sorted[i].relevance) - 1.0) / std::log2(i + 2.0);
    // Ideal DCG
    std::vector<double> rels;
    rels.reserve(sorted.size());
    for (const auto& s : sorted) rels.push_back(s.relevance);
    std::sort(rels.rbegin(), rels.rend());
    double idcg = 0.0;
    for (int i = 0; i < std::min(k, static_cast<int>(rels.size())); ++i)
        idcg += (std::pow(2.0, rels[i]) - 1.0) / std::log2(i + 2.0);
    return idcg > 0.0 ? dcg / idcg : 0.0;
}

double LTR::evalMeanNDCG(const std::vector<std::pair<int, std::vector<LTRSample>>>& data, int k) const {
    double total = 0.0;
    for (const auto& [qid, samples] : data) {
        std::vector<LTRSample> ranked = samples;
        std::sort(ranked.begin(), ranked.end(),
            [&](const LTRSample& a, const LTRSample& b){ return score(a.feats) > score(b.feats); });
        total += ndcgAtK(ranked, k);
    }
    return data.empty() ? 0.0 : total / static_cast<double>(data.size());
}

// ─── Coordinate Ascent training ──────────────────────────────────────────────

void LTR::train(const Qrel& qrel,
                const std::unordered_map<int, std::string>& queryMap,
                int ndcgK, int epochs) {
    // Build training samples for each query that has text
    std::vector<std::pair<int, std::vector<LTRSample>>> trainData;

    for (const auto& [qid, passRel] : qrel) {
        auto qit = queryMap.find(qid);
        if (qit == queryMap.end()) continue;
        const std::string& query = qit->second;

        std::vector<LTRSample> samples;
        for (const auto& [passId, rel] : passRel) {
            auto rit = index.reverseDocMap.find(passId);
            if (rit == index.reverseDocMap.end()) continue;
            int docId = rit->second;
            LTRSample s;
            s.docId     = docId;
            s.passageId = passId;
            s.relevance = static_cast<double>(rel);
            s.feats     = extractFeatures(query, docId);
            samples.push_back(std::move(s));
        }
        if (samples.size() >= 2)
            trainData.push_back({qid, std::move(samples)});
    }

    if (trainData.empty()) {
        std::cerr << "[LTR] no training data — supply queryMap entries matching qrel qids\n";
        return;
    }
    std::cout << "[LTR] training on " << trainData.size() << " queries, "
              << ndcgK << " NDCG cutoff\n";

    // Normalize features
    fitNormStats(trainData);

    // Coordinate Ascent — line search over 21 candidate values per dimension
    w.assign(NFEATS, 0.0); w[0] = 1.0; // start with BM25-only
    double bestNDCG = evalMeanNDCG(trainData, ndcgK);
    std::cout << "[LTR] init NDCG@" << ndcgK << " = " << bestNDCG << '\n';

    // Candidate weight values to try per dimension
    static const double candidates[] = {
        -3.0,-2.0,-1.5,-1.0,-0.5,-0.2,-0.1,0.0,
         0.1, 0.2, 0.5, 1.0, 1.5, 2.0, 3.0
    };
    constexpr int NCAND = static_cast<int>(sizeof(candidates)/sizeof(candidates[0]));

    for (int epoch = 0; epoch < epochs; ++epoch) {
        bool anyImproved = false;
        for (int dim = 0; dim < NFEATS; ++dim) {
            double savedW = w[dim];
            double bestW  = savedW;
            for (int c = 0; c < NCAND; ++c) {
                w[dim] = candidates[c];
                double ndcg = evalMeanNDCG(trainData, ndcgK);
                if (ndcg > bestNDCG + 1e-6) {
                    bestNDCG = ndcg;
                    bestW    = candidates[c];
                    anyImproved = true;
                }
            }
            w[dim] = bestW;
        }
        std::cout << "[LTR] epoch " << epoch + 1 << " NDCG@" << ndcgK << " = " << bestNDCG << '\n';
        if (!anyImproved) { std::cout << "[LTR] converged\n"; break; }
    }

    std::cout << "[LTR] final weights [bm25 ql vsm len_norm coverage avg_idf]: ";
    for (double wi : w) std::cout << wi << ' ';
    std::cout << '\n';
}

// ─── inference ───────────────────────────────────────────────────────────────

RankedList LTR::rerank(const std::string& query, const RankedList& candidates) const {
    std::vector<std::pair<double, std::pair<int, std::string>>> scored;
    scored.reserve(candidates.size());
    for (const auto& [docId, passId] : candidates) {
        Features f = normalizeFeats(extractFeatures(query, docId));
        scored.push_back({score(f), {docId, passId}});
    }
    std::sort(scored.begin(), scored.end(), std::greater<>());
    RankedList out;
    out.reserve(candidates.size());
    for (const auto& [s, p] : scored) out.push_back(p);
    return out;
}
