#include "src/index.h"
#include "src/models.h"
#include "src/eval.h"
#include "src/tokenizer.h"
#include "src/crawler.h"
#include "src/ltr.h"
#include "src/word2vec.h"
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <fstream>
#include <sstream>

static void printRankedList(const RankedList& rl, const std::string& label, int show = 5) {
    std::cout << "\n" << label << "\n";
    int n = std::min(show, static_cast<int>(rl.size()));
    for (int i = 0; i < n; ++i)
        std::cout << "  " << (i+1) << ". docId=" << rl[i].first
                  << "  passId=" << rl[i].second << '\n';
}

static void evalAll(int qid, const std::string& query, const RankedList& rl, int k = 5) {
    try {
        Evaluator ev(qid, query, rl);
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "  Precision@" << k << " = " << ev.evaluate(Metric::PRECISION, k) << '\n';
        std::cout << "  Recall@"    << k << "    = " << ev.evaluate(Metric::RECALL,    k) << '\n';
        std::cout << "  F1@"        << k << "       = " << ev.evaluate(Metric::F1,        k) << '\n';
        std::cout << "  RR@"        << k << "       = " << ev.evaluate(Metric::RR,        k) << '\n';
        std::cout << "  NDCG@"      << k << "     = " << ev.evaluate(Metric::NDCG,      k) << '\n';
    } catch (const std::exception& e) {
        std::cerr << "  [eval] " << e.what() << '\n';
    }
}

static Qrel loadQrel(const std::string& path) {
    Qrel qrel;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::vector<std::string> vals;
        std::string tok;
        while (iss >> tok) vals.push_back(tok);
        if (vals.size() < 4) continue;
        qrel[std::stoi(vals[0])][vals[2]] = std::stoi(vals[3]);
    }
    return qrel;
}

int main() {
    const std::string CORPUS = "data/corpus/antique-collection.tok.clean_kstem";
    const std::string QREL   = "data/antique-train-final.qrel";

    Index index{"search"};
    index.initialize(CORPUS);
    std::cout << "Passages: "       << index.numPassages      << "\n"
              << "Total tokens: "   << index.totalTerms       << "\n"
              << "Unique terms: "   << index.totalUniqueTerms << "\n"
              << "Avg doc length: " << index.avgdl            << "\n";

    index.printZipfStats(20);

    Tokenizer tok;
    std::string rawText = "Why do cats headbutt people? It's a sign of affection!";
    auto tokens = tok.tokenize(rawText);
    std::cout << "\nTokenizer input:  \"" << rawText << "\"\n";
    std::cout << "Tokens: ";
    for (const auto& t : tokens) std::cout << "[" << t << "] ";
    std::cout << '\n';

    std::string demoQuery = "why do cats headbutt";
    int K = 10;

    Model bm25_m = Model::BM25, ql_m = Model::QL, vsm_m = Model::VSM;
    Models bm25{bm25_m, index}, ql{ql_m, index}, vsm{vsm_m, index};

    RankedList rl_bm25_demo = bm25.execute(demoQuery, K);
    RankedList rl_ql_demo   = ql.execute(demoQuery, K);
    RankedList rl_vsm_demo  = vsm.execute(demoQuery, K);

    std::cout << "\nQuery: \"" << demoQuery << "\"\n";
    printRankedList(rl_bm25_demo, "BM25",  K);
    printRankedList(rl_ql_demo,   "QL/JM", K);
    printRankedList(rl_vsm_demo,  "VSM",   K);

    struct EvalQuery { int qid; std::string text; };
    std::vector<EvalQuery> evalQueries {
        {181119,  "how much does a golf caddy get paid tips"},
        {3910705, "why don t commercial planes have parachutes"},
        {2531329, "why do people do rituals before free throws"},
    };

    for (const auto& eq : evalQueries) {
        std::string qt = eq.text;
        std::cout << "\nQuery: \"" << eq.text << "\"  (qid=" << eq.qid << ")\n";
        RankedList rl_b = bm25.execute(qt, K);
        RankedList rl_q = ql.execute(qt, K);
        RankedList rl_v = vsm.execute(qt, K);
        std::cout << "BM25\n"; evalAll(eq.qid, eq.text, rl_b, 5);
        std::cout << "QL\n";   evalAll(eq.qid, eq.text, rl_q, 5);
        std::cout << "VSM\n";  evalAll(eq.qid, eq.text, rl_v, 5);
    }

    std::unordered_map<int, std::string> queryMap {
        {181119,  "how much does a golf caddy get paid tips"},
        {3910705, "why don t commercial planes have parachutes"},
        {2531329, "why do people do rituals before free throws"},
        {1680635, "how do airplanes fly"},
        {3470651, "what are the symptoms of appendicitis"},
        {1437381, "how do you get rid of hiccups"},
        {2471885, "how does the stock market work"},
        {4384685, "why is the sky blue"},
    };

    {
        std::ifstream qf("data/antique-train-queries.txt");
        if (qf.is_open()) {
            std::string line;
            while (std::getline(qf, line)) {
                if (line.empty()) continue;
                size_t tab = line.find('\t');
                if (tab == std::string::npos) continue;
                try {
                    int id = std::stoi(line.substr(0, tab));
                    queryMap[id] = line.substr(tab + 1);
                } catch (...) {}
            }
            std::cout << "[LTR] loaded " << queryMap.size() << " query texts from file\n";
        }
    }

    Qrel qrel = loadQrel(QREL);
    LTR ltr(index);
    ltr.train(qrel, queryMap, 10, 20);

    std::string ltrQuery = evalQueries[0].text;
    int ltrQid           = evalQueries[0].qid;
    RankedList rl_base   = bm25.execute(ltrQuery, K);
    RankedList rl_ltr    = ltr.rerank(ltrQuery, rl_base);

    std::cout << "\nRe-ranking \"" << ltrQuery << "\" (qid=" << ltrQid << ")\n";
    printRankedList(rl_base, "BM25 base", K);
    printRankedList(rl_ltr,  "LTR",       K);

    std::cout << "\nBM25 baseline\n";
    evalAll(ltrQid, ltrQuery, rl_base, 10);
    std::cout << "\nLTR re-ranked\n";
    evalAll(ltrQid, ltrQuery, rl_ltr, 10);

    std::cout << "\nLTR weights [bm25 ql vsm len_norm coverage avg_idf]\n  ";
    for (double w : ltr.weights()) std::cout << std::setprecision(3) << w << "  ";
    std::cout << '\n';

    std::cout << "\nWord2Vec training on first 5000 passages...\n";
    std::vector<std::vector<std::string>> w2v_corpus;
    {
        std::ifstream corp(CORPUS);
        std::string line;
        int count = 0;
        while (std::getline(corp, line) && count < 5000) {
            size_t tab = line.find('\t');
            if (tab == std::string::npos) continue;
            std::istringstream iss(line.substr(tab + 1));
            std::vector<std::string> words;
            std::string w;
            while (iss >> w) words.push_back(w);
            if (!words.empty()) { w2v_corpus.push_back(std::move(words)); ++count; }
        }
    }

    Word2Vec w2v(50, 3, 3, 0.025f, 5, Word2Vec::CBOW);
    w2v.train(w2v_corpus);

    std::cout << "sim(cat, dog)       = " << w2v.similarity("cat", "dog")       << '\n';
    std::cout << "sim(people, person) = " << w2v.similarity("people", "person") << '\n';
    std::cout << "sim(love, affection)= " << w2v.similarity("love", "affection")<< '\n';

    return 0;
}
