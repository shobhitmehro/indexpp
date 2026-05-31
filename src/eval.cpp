#include "eval.h"

Evaluator::Evaluator(int qid, const std::string& query, const RankedList& rl) : 
    m_qid{qid}, 
    m_query{query},
    m_rl{rl}, 
    m_qrel{this->loadQrel("/Users/shobhitmehrotra/Desktop/index++/data/antique-train-final.qrel")} 
    {};

Evaluator::~Evaluator() {}

Evaluator::Evaluator(Evaluator&& evaluator) : 
    m_qid{evaluator.m_qid}, 
    m_query{std::move(evaluator.m_query)}, 
    m_rl{std::move(evaluator.m_rl)}, 
    m_qrel{std::move(evaluator.m_qrel)} 
    {};

double Evaluator::evaluate(Metric m, int k) {
    switch(m) {
        case(Metric::PRECISION): return calcPrecision(k);      
        case(Metric::RECALL): return calcRecall(k);
        case(Metric::F1): return calcF1(k);
        case(Metric::RR): return calcRR(k);
        default: break;
    }
}

double Evaluator::calcPrecision(int k) {
    if(!m_qrel) throw std::runtime_error("invalid query id and/or passage result(s) id");

    const auto& passage_rel = m_qrel->at(m_qid);
    int retrieved = 0;
    int relevant = 0;

    for(const auto& [i, pid] : m_rl) {
        if(k > 0 && retrieved >= k) break;
        ++retrieved;

        auto it = passage_rel.find(pid);
        if(it != passage_rel.end() && it->second > 0) {
            ++relevant;
        }
    }
    return retrieved > 0 ? static_cast<double>(relevant) / retrieved : 0.0;
}

double Evaluator::calcRecall(int k) {
    if(!m_qrel) throw std::runtime_error("invalid query id and/or passage result(s) id");

    const auto& passage_rel = m_qrel->at(m_qid);

    int total_relevant = 0;
    for(const auto& [pid, score] : passage_rel) {
        if(score > 0) ++total_relevant;
    }
    if(total_relevant == 0) return 0.0;

    int retrieved = 0;
    int relevant_retrieved = 0;
    for(const auto& [i, pid] : m_rl) {
        if(k > 0 && retrieved >= k) break;
        ++retrieved;

        auto it = passage_rel.find(pid);
        if(it != passage_rel.end() && it->second > 0) {
            ++relevant_retrieved;
        }
    }
    return static_cast<double>(relevant_retrieved) / total_relevant;
}

double Evaluator::calcF1(int k) {
    double p = calcPrecision(k);
    double r = calcRecall(k);
    if(p + r == 0.0) return 0.0;
    return 2.0 * p * r / (p + r);
}

double Evaluator::calcRR(int k) {
    if(!m_qrel) throw std::runtime_error("invalid query id and/or passage result(s) id");

    const auto& passage_rel = m_qrel->at(m_qid);
    int retrieved = 0;
    for(const auto& [i, pid] : m_rl) {
        if(k > 0 && retrieved >= k) break;
        ++retrieved;

        auto it = passage_rel.find(pid);
        if(it != passage_rel.end() && it->second > 0) {
            return 1.0 / retrieved;
        }
    }
    return 0.0;
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