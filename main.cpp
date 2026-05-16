#include "src/index.h"
#include "src/models.h"
#include <iostream>

int main() {
    auto* index = new Index {"search"};
    index->initialize("data/corpus/antique-collection.tok.clean_kstem");
    Model bm25_model {Model::BM25};
    Model ql_model {Model::QL};
    auto* model_bm25 {new Models(bm25_model, *index)};
    auto* model_ql {new Models(ql_model, *index)};
    std::string query {"why do cat headbutt"};
    std::cout << model_bm25->execute(query);
    std::cout << model_ql->execute(query);
    return 0;
}