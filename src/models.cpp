#include "models.h"

Models::Models(Model& name, const Index& ii) : mname{name}, target{ii} {};

std::vector<std::pair<int, std::string>> Models::execute(std::string& query, int k) {
    std::vector<std::pair<int, std::string>> result {};
    result.reserve(k);

    switch(this->mname) {
        case(Model::BM25): {
            this->bm25(k, query, result);
            break;
        }
        default: break;
    }
    return result;
}

//taat
void Models::bm25(int k, std::string& query, std::vector<std::pair<int, std::string>>& out, double b, double k1) {
    std::istringstream iss {query};
    std::string q_token;

    std::unordered_map<int, double> scoreMap {};
    while(std::getline(iss, q_token, ' ')) {
        auto it {target.invertedIndex.find(q_token)};
        if (it == target.invertedIndex.end()) continue;
        for(const auto& [pid, count] : it->second) {
            double num1 {(k1+1)*count};
            double den1 {k1 * (1-b+b*(static_cast<double>(target.docLenMap.at(pid))/target.avgdl)) + count};
            double num2 {std::size(target.docLenMap) - std::size(target.invertedIndex.at(q_token)) + 0.5};
            double den2 {std::size(target.invertedIndex.at(q_token)) + 0.5};
            scoreMap[pid] += (num1/den1) * std::log(num2/den2);
        }
    }
    std::priority_queue<
        std::pair<double, int>,
        std::vector<std::pair<double, int>>,
        std::greater<std::pair<double, int>>
    > pq;

    for(const auto& [pid, score] : scoreMap) {
        pq.push(std::pair {score, pid});
        if(pq.size() > static_cast<std::size_t>(k)) {
            pq.pop();
        }
    }

    while(!pq.empty()) {
        out.push_back(std::pair {pq.top().second, target.docMap.at(pq.top().second)} );
        pq.pop();
    }
    std::reverse(out.begin(), out.end());
}

std::ostream& operator<<(std::ostream& os, const std::vector<std::pair<int, std::string>>& results) {
    for (const auto& [docid, passage] : results) {
        os << docid << "\t" << passage << "\n";
    }
    return os;
}