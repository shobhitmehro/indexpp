#pragma once
#include <string>
#include <vector>
#include <unordered_set>

struct CrawledDoc {
    std::string url;
    std::string title;
    std::string text;
    int depth;
};

class Crawler {
public:
    explicit Crawler(int politenessMs = 500);
    std::vector<CrawledDoc> crawl(const std::string& seedUrl, int maxPages = 50, int maxDepth = 2);
    // Write crawled docs in corpus format: <id>\t<text>
    void saveCorpus(const std::vector<CrawledDoc>& docs, const std::string& outPath) const;

private:
    int politenessMs;

    struct ParsedUrl {
        std::string scheme;
        std::string host;
        int port;
        std::string path;
    };

    static ParsedUrl parseUrl(const std::string& url);
    static std::string resolveHref(const ParsedUrl& base, const std::string& href);
    std::string httpGet(const std::string& host, const std::string& path, int port);
    static std::string extractTitle(const std::string& html);
    static std::string extractText(const std::string& html);
    static std::vector<std::string> extractLinks(const std::string& html, const ParsedUrl& base);
    static std::string stripTags(const std::string& html);
    static std::string decodeEntities(const std::string& s);
    static std::string condenseWhitespace(const std::string& s);
};
