#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "fstream"
#include <sstream>
#include <filesystem>

// QREL -> ground truth judgements

using RankedList = std::vector<std::pair<int, std::string>>;
using Qrel = std::unordered_map<int, std::unordered_map<std::string, int>>;

class Evaluator {
    public:
        enum Metric {NONE, PRECISION, RECALL, F1, RR};
        Evaluator(int qid, const std::string& query, const RankedList& rl);
        ~Evaluator();
        Evaluator(Evaluator&& evaluator);
        Evaluator& operator=(const Evaluator&& evaluator) = delete;
        Evaluator(const Evaluator& evaluator) = delete;
        Evaluator& operator=(const Evaluator& evaluator) = delete;
        double evaluate(Metric m);
        

    private:
        const RankedList& m_rl;
        int m_qid;
        const std::string& m_query;
        const std::optional<Qrel>& m_qrel;
        double calcPrecision();
        double calcRecall();
        double calcF1();
        double calcRR();
        bool validListAndQuery(const Qrel& qrel);
        std::optional<Qrel> loadQrel(const std::string& path);
};