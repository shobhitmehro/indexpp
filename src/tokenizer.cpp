#include "tokenizer.h"
#include <sstream>
#include <algorithm>
#include <cctype>

static const char* STOP_WORDS[] = {
    "a","about","above","after","again","against","all","am","an","and","any",
    "are","as","at","be","because","been","before","being","below","between",
    "both","but","by","can","did","do","does","doing","don","down","during",
    "each","few","for","from","further","get","got","had","has","have","having",
    "he","her","here","him","his","how","i","if","in","into","is","it","its",
    "itself","just","me","more","most","my","no","nor","not","now","of","off",
    "on","once","only","or","other","our","out","own","s","same","she","should",
    "so","some","such","t","than","that","the","their","them","then","there",
    "these","they","this","those","through","to","too","under","until","up",
    "us","very","was","we","were","what","when","where","which","while","who",
    "why","will","with","would","you","your"
};

Tokenizer::Tokenizer() {
    for (const char* w : STOP_WORDS)
        stopwords.insert(w);
}

std::string Tokenizer::normalize(const std::string& word) {
    std::string out;
    out.reserve(word.size());
    for (unsigned char c : word)
        if (std::isalpha(c))
            out += static_cast<char>(std::tolower(c));
    return out;
}

// Light suffix-stripping stemmer (handles common English inflections)
std::string Tokenizer::stem(const std::string& w) {
    if (w.size() <= 3) return w;

    static const std::pair<const char*, const char*> rules[] = {
        {"ational","ate"}, {"iveness","ive"}, {"fulness","ful"},
        {"ousness","ous"}, {"ization","ize"}, {"nesses","ness"},
        {"ations","ate"}, {"ingly","ing"}, {"edness","ed"},
        {"ation","ate"}, {"ating","ate"}, {"izing","ize"},
        {"ness",""}, {"ment",""}, {"able",""}, {"ible",""},
        {"ious",""}, {"ical",""}, {"ies","y"},
        {"ing",""}, {"ous",""}, {"ive",""}, {"ful",""},
        {"er",""}, {"ly",""}, {"ed",""}, {"es",""}, {"s",""}
    };

    for (const auto& [suf, rep] : rules) {
        size_t sl = std::strlen(suf);
        if (w.size() > sl + 2 && w.compare(w.size() - sl, sl, suf) == 0)
            return w.substr(0, w.size() - sl) + rep;
    }
    return w;
}

std::vector<std::string> Tokenizer::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        word = normalize(word);
        if (word.size() < 2) continue;
        if (stopwords.count(word)) continue;
        word = stem(word);
        if (word.empty() || word.size() < 2) continue;
        tokens.push_back(std::move(word));
    }
    return tokens;
}
