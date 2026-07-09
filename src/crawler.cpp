#include "crawler.h"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <queue>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

Crawler::Crawler(int politenessMs) : politenessMs{politenessMs} {}

Crawler::ParsedUrl Crawler::parseUrl(const std::string& url) {
    ParsedUrl p;
    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
        p.scheme = "http"; p.host = url; p.port = 80; p.path = "/";
        return p;
    }
    p.scheme = url.substr(0, schemeEnd);
    std::string rest = url.substr(schemeEnd + 3);
    size_t slashPos = rest.find('/');
    std::string hostPort = (slashPos == std::string::npos) ? rest : rest.substr(0, slashPos);
    p.path = (slashPos == std::string::npos) ? "/" : rest.substr(slashPos);
    // strip query string from path for link dedup (keep it for requests)
    size_t colon = hostPort.find(':');
    if (colon != std::string::npos) {
        p.host = hostPort.substr(0, colon);
        try { p.port = std::stoi(hostPort.substr(colon + 1)); }
        catch (...) { p.port = 80; }
    } else {
        p.host = hostPort;
        p.port = (p.scheme == "https") ? 443 : 80;
    }
    return p;
}

std::string Crawler::resolveHref(const ParsedUrl& base, const std::string& href) {
    if (href.empty() || href[0] == '#' || href.substr(0,7) == "mailto:" ||
        href.substr(0,11) == "javascript:") return "";
    if (href.size() >= 4 && href.substr(0, 4) == "http") return href;
    if (href[0] == '/') return base.scheme + "://" + base.host + href;
    // relative
    std::string dir = base.path;
    size_t last = dir.rfind('/');
    dir = (last != std::string::npos) ? dir.substr(0, last + 1) : "/";
    return base.scheme + "://" + base.host + dir + href;
}

std::string Crawler::httpGet(const std::string& host, const std::string& path, int port) {
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0)
        return "";

    int sock = -1;
    for (struct addrinfo* rp = res; rp; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;
        struct timeval tv{5, 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sock); sock = -1;
    }
    freeaddrinfo(res);
    if (sock < 0) return "";

    std::string req = "GET " + path + " HTTP/1.0\r\nHost: " + host +
                      "\r\nUser-Agent: IndexPlusPlus/1.0\r\nAccept: text/html\r\nConnection: close\r\n\r\n";
    send(sock, req.c_str(), req.size(), 0);

    std::string response;
    response.reserve(65536);
    char buf[8192];
    ssize_t n;
    while ((n = recv(sock, buf, sizeof(buf), 0)) > 0)
        response.append(buf, static_cast<size_t>(n));
    close(sock);

    // Handle redirect (301/302) — one hop
    if (response.size() > 12) {
        std::string status = response.substr(9, 3);
        if (status == "301" || status == "302") {
            size_t loc = response.find("Location: ");
            if (loc == std::string::npos) loc = response.find("location: ");
            if (loc != std::string::npos) {
                size_t ls = loc + 10, le = response.find("\r\n", ls);
                std::string newUrl = response.substr(ls, le - ls);
                auto np = parseUrl(newUrl);
                if (np.scheme != "https") return httpGet(np.host, np.path, np.port);
            }
        }
    }

    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos) return "";
    return response.substr(headerEnd + 4);
}

std::string Crawler::extractTitle(const std::string& html) {
    for (const char* tag : {"<title>", "<TITLE>"}) {
        size_t s = html.find(tag);
        if (s == std::string::npos) continue;
        s += std::strlen(tag);
        size_t e = html.find("</title>", s);
        if (e == std::string::npos) e = html.find("</TITLE>", s);
        if (e != std::string::npos) return html.substr(s, e - s);
    }
    return "";
}

std::string Crawler::stripTags(const std::string& html) {
    std::string out;
    out.reserve(html.size() / 2);
    bool inTag = false, skipContent = false;
    for (size_t i = 0; i < html.size(); ++i) {
        if (html[i] == '<') {
            inTag = true;
            // Detect <script> and <style> to skip their content
            if (i + 7 <= html.size()) {
                char buf[8];
                for (size_t j = 0; j < 7; ++j) buf[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(html[i+j])));
                buf[7] = '\0';
                if (std::strncmp(buf, "<script", 7) == 0 || std::strncmp(buf, "<style", 6) == 0)
                    skipContent = true;
            }
            // Detect </script> or </style>
            if (i + 8 <= html.size()) {
                char buf[9];
                for (size_t j = 0; j < 8; ++j) buf[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(html[i+j])));
                buf[8] = '\0';
                if (std::strncmp(buf, "</script", 8) == 0 || std::strncmp(buf, "</style", 7) == 0)
                    skipContent = false;
            }
        } else if (html[i] == '>') {
            inTag = false;
            out += ' ';
        } else if (!inTag && !skipContent) {
            out += html[i];
        }
    }
    return out;
}

std::string Crawler::decodeEntities(const std::string& s) {
    static const std::pair<const char*, char> entities[] = {
        {"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"nbsp", ' '},
        {"apos", '\''}, {"copy", ' '}, {"reg", ' '}
    };
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '&') {
            size_t semi = s.find(';', i + 1);
            if (semi != std::string::npos && semi - i <= 8) {
                std::string ent = s.substr(i + 1, semi - i - 1);
                bool found = false;
                for (const auto& [name, ch] : entities) {
                    if (ent == name) { out += ch; found = true; break; }
                }
                if (found) { i = semi; continue; }
            }
        }
        out += s[i];
    }
    return out;
}

std::string Crawler::condenseWhitespace(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool prevSpace = true;
    for (unsigned char c : s) {
        if (std::isspace(c)) {
            if (!prevSpace) out += ' ';
            prevSpace = true;
        } else {
            out += static_cast<char>(c);
            prevSpace = false;
        }
    }
    return out;
}

std::string Crawler::extractText(const std::string& html) {
    return condenseWhitespace(decodeEntities(stripTags(html)));
}

std::vector<std::string> Crawler::extractLinks(const std::string& html, const ParsedUrl& base) {
    std::vector<std::string> links;
    size_t pos = 0;
    std::string lower = html;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    while (true) {
        size_t apos = lower.find("<a ", pos);
        if (apos == std::string::npos) break;
        size_t tagEnd = lower.find('>', apos);
        if (tagEnd == std::string::npos) break;

        size_t hpos = lower.find("href=", apos);
        if (hpos == std::string::npos || hpos > tagEnd) { pos = tagEnd; continue; }
        hpos += 5;

        char delim = html[hpos];
        std::string href;
        if (delim == '"' || delim == '\'') {
            size_t end = html.find(delim, hpos + 1);
            if (end != std::string::npos) href = html.substr(hpos + 1, end - hpos - 1);
            pos = (end != std::string::npos) ? end : tagEnd;
        } else {
            // unquoted href
            size_t end = hpos;
            while (end < html.size() && !std::isspace(static_cast<unsigned char>(html[end])) && html[end] != '>') ++end;
            href = html.substr(hpos, end - hpos);
            pos = end;
        }

        if (!href.empty()) {
            std::string resolved = resolveHref(base, href);
            if (!resolved.empty()) links.push_back(std::move(resolved));
        }
    }
    return links;
}

std::vector<CrawledDoc> Crawler::crawl(const std::string& seedUrl, int maxPages, int maxDepth) {
    std::vector<CrawledDoc> docs;
    std::unordered_set<std::string> visited;

    struct Entry { std::string url; int depth; };
    std::queue<Entry> frontier;
    frontier.push({seedUrl, 0});
    visited.insert(seedUrl);

    while (!frontier.empty() && static_cast<int>(docs.size()) < maxPages) {
        auto [url, depth] = frontier.front();
        frontier.pop();

        auto parsed = parseUrl(url);
        if (parsed.scheme == "https") {
            std::cerr << "[crawler] skip HTTPS (no TLS): " << url << '\n';
            continue;
        }

        std::cout << "[crawler] fetching (" << docs.size() + 1 << "/" << maxPages
                  << ") depth=" << depth << " " << url << '\n';

        std::string html = httpGet(parsed.host, parsed.path, parsed.port);
        if (html.empty()) continue;

        CrawledDoc doc;
        doc.url   = url;
        doc.depth = depth;
        doc.title = extractTitle(html);
        doc.text  = extractText(html);
        if (doc.text.empty()) continue;
        docs.push_back(std::move(doc));

        if (depth < maxDepth) {
            for (const auto& link : extractLinks(html, parsed)) {
                if (!visited.count(link)) {
                    visited.insert(link);
                    frontier.push({link, depth + 1});
                }
            }
        }

        if (politenessMs > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(politenessMs));
    }

    std::cout << "[crawler] done. collected " << docs.size() << " pages\n";
    return docs;
}

void Crawler::saveCorpus(const std::vector<CrawledDoc>& docs, const std::string& outPath) const {
    std::ofstream out(outPath);
    for (size_t i = 0; i < docs.size(); ++i)
        out << "crawled_" << i << '\t' << docs[i].text << '\n';
    std::cout << "[crawler] saved " << docs.size() << " docs to " << outPath << '\n';
}
