#include "word2vec.h"
#include <stdexcept>
#include <algorithm>
#include <numeric>

Word2Vec::Word2Vec(int dim, int window, int epochs, float lr, int negSamples, Type t)
    : dim{dim}, window{window}, epochs{epochs}, negSamples{negSamples},
      lr{lr}, type{t}, rng{std::random_device{}()} {}


void Word2Vec::train(const std::vector<std::vector<std::string>>& corpus) {
    buildVocab(corpus);
    for (int e = 0; e < epochs; ++e)
        for (const auto& sentence : corpus)
            trainCBOW(sentence);
}

std::vector<float> Word2Vec::embedding(const std::string& word) const {
    auto it = vocab.find(word);
    if (it == vocab.end()) return {};
    return W_in[it->second];
}

float Word2Vec::similarity(const std::string& a, const std::string& b) const {
    auto va = embedding(a), vb = embedding(b);
    if (va.empty() || vb.empty()) return 0.0f;
    float na = norm(va), nb = norm(vb);
    if (na == 0.0f || nb == 0.0f) return 0.0f;
    return dot(va, vb) / (na * nb);
}

void Word2Vec::buildVocab(const std::vector<std::vector<std::string>>& corpus) {
    std::unordered_map<std::string, int> freq;
    for (const auto& sentence : corpus)
        for (const auto& w : sentence)
            freq[w]++;

    for (const auto& [w, _] : freq) {
        vocab[w] = static_cast<int>(idx2word.size());
        idx2word.push_back(w);
    }

    int V = static_cast<int>(idx2word.size());
    std::uniform_real_distribution<float> init(-0.5f / dim, 0.5f / dim);

    W_in.assign(V, std::vector<float>(dim));
    W_out.assign(V, std::vector<float>(dim, 0.0f));
    for (auto& row : W_in)
        for (auto& v : row)
            v = init(rng);

    buildNoiseTable(freq);
}

void Word2Vec::buildNoiseTable(const std::unordered_map<std::string, int>& freq, int tableSize) {
    double total = 0.0;
    std::vector<double> powered(idx2word.size());
    for (int i = 0; i < static_cast<int>(idx2word.size()); ++i) {
        powered[i] = std::pow(static_cast<double>(freq.at(idx2word[i])), 0.75);
        total += powered[i];
    }

    noiseTable.resize(tableSize);
    int wi = 0;
    double cumulative = powered[0] / total;
    for (int i = 0; i < tableSize; ++i) {
        noiseTable[i] = wi;
        if (static_cast<double>(i + 1) / tableSize > cumulative && wi + 1 < static_cast<int>(idx2word.size())) {
            cumulative += powered[++wi] / total;
        }
    }
}


void Word2Vec::trainCBOW(const std::vector<std::string>& sentence) {
    int n = static_cast<int>(sentence.size());
    std::uniform_int_distribution<int> noiseDist(0, static_cast<int>(noiseTable.size()) - 1);

    for (int i = 0; i < n; ++i) {
        auto cit = vocab.find(sentence[i]);
        if (cit == vocab.end()) continue;
        int target = cit->second;

        std::vector<int> ctxIds;
        for (int j = i - window; j <= i + window; ++j) {
            if (j < 0 || j >= n || j == i) continue;
            auto it = vocab.find(sentence[j]);
            if (it != vocab.end()) ctxIds.push_back(it->second);
        }
        if (ctxIds.empty()) continue;

        std::vector<float> h(dim, 0.0f);
        for (int id : ctxIds)
            for (int d = 0; d < dim; ++d)
                h[d] += W_in[id][d];
        float scale = 1.0f / static_cast<float>(ctxIds.size());
        for (float& v : h) v *= scale;

        std::vector<float> dh(dim, 0.0f);

        auto update = [&](int wordIdx, float label) {
            float score = sigmoid(dot(h, W_out[wordIdx]));
            float err   = (label - score) * lr;

            for (int d = 0; d < dim; ++d) {
                dh[d]             += err * W_out[wordIdx][d];
                W_out[wordIdx][d] += err * h[d];
            }
        };

        update(target, 1.0f);
        for (int s = 0; s < negSamples; ++s) {
            int neg = noiseTable[noiseDist(rng)];
            if (neg == target) continue;
            update(neg, 0.0f);
        }

        for (int id : ctxIds)
            for (int d = 0; d < dim; ++d)
                W_in[id][d] += dh[d] * scale;
    }
}

// ---------- math helpers ----------

float Word2Vec::dot(const std::vector<float>& a, const std::vector<float>& b) const {
    float s = 0.0f;
    for (int d = 0; d < dim; ++d) s += a[d] * b[d];
    return s;
}

float Word2Vec::sigmoid(float x) const {
    return 1.0f / (1.0f + std::exp(-x));
}

float Word2Vec::norm(const std::vector<float>& v) const {
    float s = 0.0f;
    for (float x : v) s += x * x;
    return std::sqrt(s);
}
