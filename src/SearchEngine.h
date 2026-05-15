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

class SearchEngine {
    private:
        std::string sname {};
        InvertedIndex invertedIndex {};
        void buildIndex(const std::string& fpath);
        void saveToDisk();
        void loadIndex();
        void createNewIndex(const std::string& fpath);

    public:
        void initialize(const std::string& fpath);
        SearchEngine(const std::string& name);
        



};