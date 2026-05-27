#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <random>
#include <cmath>

class Word2Vec {
public:
    enum Type { NONE, CBOW, SKIPGRAM };

    Word2Vec(int dim = 100, int window = 5, int epochs = 5, float lr = 0.025f,
             int negSamples = 5, Type t = CBOW);

    void train(const std::vector<std::vector<std::string>>& corpus);
    std::vector<float> embedding(const std::string& word) const;
    float similarity(const std::string& a, const std::string& b) const;

private:
    int dim, window, epochs, negSamples;
    float lr;
    Type type;

    std::unordered_map<std::string, int> vocab;
    std::vector<std::string> idx2word;
    std::vector<std::vector<float>> W_in, W_out;  

    std::vector<int> noiseTable;
    std::mt19937 rng;

    void buildVocab(const std::vector<std::vector<std::string>>& corpus);
    void buildNoiseTable(const std::unordered_map<std::string, int>& freq, int tableSize = 1'000'000);
    void trainCBOW(const std::vector<std::string>& sentence);

    float dot(const std::vector<float>& a, const std::vector<float>& b) const;
    float sigmoid(float x) const;
    float norm(const std::vector<float>& v) const;
};
