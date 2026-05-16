#pragma once
#include "unordered_map"
#include <string>
#include "fstream"
#include <sstream>
#include <filesystem>
#include <iostream>

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
        std::unordered_map<int, int> docLenMap {};
        std::unordered_map<int, std::string> docMap {};
        int totalTerms {};
        int totalUniqueTerms {};
        int numPassages {};
        int avgdl {};
        void initialize(const std::string& fpath);
        Index(const std::string& name);
        ~Index();
        

};