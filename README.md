# index++

A C++17 search engine built on the [ANTIQUE](https://ciir.cs.umass.edu/downloads/Antique/) community QA dataset (403K passages).

## Features

- **Inverted Index** term-at-a-time traversal, cereal binary persistence, automatic cache invalidation
- **Retrieval Models** BM25, Query Likelihood (JM smoothing), Vector Space Model (pivoted length norm)
- **Learning to Rank** Coordinate Ascent over 6 features `[bm25, ql, vsm, doc_len_norm, query_coverage, avg_idf]`, optimizes NDCG@k
- **Evaluation** Precision@k, Recall@k, F1@k, Reciprocal Rank, NDCG@k (graded)
- **Zipf's Law Analysis** rank-frequency stats over the full collection vocabulary
- **Tokenizer** lowercase, punctuation stripping, stopword removal, suffix-stripping stemmer
- **Web Crawler** BFS crawler via raw POSIX sockets (HTTP), HTML text/link extraction, configurable depth and page limits
- **Word2Vec (CBOW)** negative sampling, cosine similarity, trains on any tokenized corpus

## Dependencies

- C++17 compiler (`g++` or `clang++`)
- [cereal](https://uscilab.github.io/cereal/) header-only serialization library

Install cereal on macOS
```bash
brew install cereal
```

## Build

```bash
make          # release
make debug    # with -g -O0
make asan     # with AddressSanitizer
make clean
```

Binary is written to `build/index++`.

## Data

Place the ANTIQUE corpus and qrel under `data/`

```
data/
  corpus/
    antique-collection.tok.clean_kstem   # pre-tokenized/Krovetz-stemmed passages
  antique-train-final.qrel               # relevance judgements
  antique-train-queries.txt              # (optional) qid\tquery_text, enables full LTR training
```

## Usage

```bash
./build/index++
```

On first run the index is built from the corpus and saved to `search.index`. Subsequent runs load from cache.

### Crawling new content

```cpp
Crawler crawler(500);  // 500ms politeness delay between requests
auto docs = crawler.crawl("http://example.com", 50 /*pages*/, 2 /*depth*/);
crawler.saveCorpus(docs, "data/corpus/crawled.tsv");

Index webIndex{"web"};
webIndex.initialize("data/corpus/crawled.tsv");
```

### LTR training

Supply a `qid` to query text map. If `data/antique-train-queries.txt` exists it is loaded automatically.

```cpp
LTR ltr(index);
ltr.train(qrel, queryMap, /*ndcgK=*/10, /*epochs=*/20);
RankedList reranked = ltr.rerank(query, bm25Results);
```

## Project Structure

```
src/
  index.h/cpp      inverted index, Zipf analysis
  models.h/cpp     BM25, QL, VSM
  eval.h/cpp       Precision, Recall, F1, RR, NDCG
  tokenizer.h/cpp  text preprocessing pipeline
  crawler.h/cpp    HTTP web crawler
  ltr.h/cpp        Learning to Rank (Coordinate Ascent)
  word2vec.h/cpp   CBOW embeddings
main.cpp           end-to-end demo
Makefile
```
