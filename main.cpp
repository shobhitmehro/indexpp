#include "src/index.h"
#include "src/models.h"
#include "src/eval.h"
#include <iostream>

int main() {
    Index index {"search"};
    index.initialize("data/corpus/antique-collection.tok.clean_kstem");
    Model bm25_model {Model::BM25};
    Model ql_model {Model::QL};
    Model vsm_movel {Model::VSM};
    Models model_bm25 {bm25_model, index};
    Models model_ql {ql_model, index};
    Models model_vsm {vsm_movel, index};
    std::string query {"why do cats headbutt"};
    RankedList rl {model_bm25.execute(query)};
    auto evaluator {std::make_unique<Evaluator>(3698636, query, rl)};
    std::cout << model_bm25.execute(query) << '\n';
    
    std::cout << evaluator->evaluate(Metric::PRECISION, 5) << '\n';
    return 0;
}