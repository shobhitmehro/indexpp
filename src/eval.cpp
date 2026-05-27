#include "eval.h"

Evaluator::Evaluator(int qid, const std::string& query, const RankedList& rl) : 
    m_qid{qid}, 
    m_query{query},
    m_rl{rl}, 
    m_qrel{this->loadQrel("ok")} 
    {};

Evaluator::Evaluator(Evaluator&& evaluator) : 
    m_qid{evaluator.m_qid}, 
    m_query{std::move(evaluator.m_query)}, 
    m_rl{std::move(evaluator.m_rl)}, 
    m_qrel{std::move(evaluator.m_qrel)} 
    {};

double Evaluator::evaluate(Metric m) {
    switch(m) {
        case(Metric::PRECISION): return calcPrecision();      
        case(Metric::RECALL): return calcRecall();
        case(Metric::F1): return calcF1();
        case(Metric::RR): return calcRR();
        default: break;
    }
}

double Evaluator::calcPrecision() {

}

std::optional<Qrel> Evaluator::loadQrel(const std::string& path) {
    Qrel qrel {};
    if(!std::filesystem::exists(path)) throw std::runtime_error("invalid pathname");

    std::ifstream file {path};
    if (!file.is_open()) {
        std::cerr << "Error: Could not open input file: " << path << std::endl;
        return std::nullopt;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue; 
        
        std::istringstream iss {line};
        std::string s;
        std::vector<std::string> values;

        while (getline(iss, s, ' ' )) {
            values.push_back(s);
        }

        qrel[std::stoi(values[0])][values[2]] = std::stoi(values[3]);
    }

    if(!validListAndQuery(qrel)) return std::nullopt;
    
    return qrel;
}

bool Evaluator::validListAndQuery(const Qrel& qrel) {
    if(qrel.find(m_qid) == qrel.end()) return false;

    std::unordered_map<std::string, int> passage_rel {qrel.at(m_qid)};

    for(const auto [i, pid] : m_rl) {
        if(passage_rel.find(pid) == passage_rel.end()) return false;
    }
    return true;
}