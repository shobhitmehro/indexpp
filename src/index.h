#pragma once
#include "unordered_map"
#include <string>
#include "fstream"
#include <sstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <utility>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>

// term -> posting list(docid, count)
using PostingList = std::unordered_map<int, int>;
using InvertedIndex = std::unordered_map<std::string, PostingList>;

class Index {
    private:
        std::string iname {};
        void buildIndex(const std::string& fpath);
        void saveToDisk();
        void loadIndex();
        void createNewIndex(const std::string& fpath);

    public:
        InvertedIndex invertedIndex {};
        std::unordered_map<int, int>    docLenMap {};
        std::unordered_map<int, std::string> docMap {};
        std::unordered_map<std::string, int> reverseDocMap {};  // passageId -> int docId
        int totalTerms {};
        int totalUniqueTerms {};
        int numPassages {};
        int avgdl {};

        void initialize(const std::string& fpath);
        Index(const std::string& name);
        ~Index();

        // Print top-N terms ranked by collection frequency (Zipf analysis)
        void printZipfStats(int topN = 30) const;
        // Returns sorted (term, collectionFreq) pairs for external analysis
        std::vector<std::pair<std::string, int>> termFreqRanking() const;
};