#pragma once
#include <string>
#include <vector>
#include <unordered_set>

class Tokenizer {
public:
    Tokenizer();
    std::vector<std::string> tokenize(const std::string& text) const;
    // Normalize a single word: lowercase + strip punctuation
    static std::string normalize(const std::string& word);

private:
    std::unordered_set<std::string> stopwords;
    static std::string stem(const std::string& word);
};
