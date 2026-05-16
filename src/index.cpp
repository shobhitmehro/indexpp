#include "index.h"

Index::Index(const std::string& name) : iname{name} {}

void Index::initialize(const std::string& fpath) {
    this->buildIndex(fpath);
}

void Index::buildIndex(const std::string& fpath) {
    std::string fname = this->iname + ".index";
    if (std::filesystem::exists(fname)) {
        std::cout << "loading index from cache" << '\n';
        loadIndex();
    } else {
        std::cout << "building new index from: " << fpath << '\n';
        createNewIndex(fpath);
    }
}

void Index::createNewIndex(const std::string& fpath) {
    std::ifstream file{fpath};
    if (!file.is_open()) {
        std::cerr << "Error: Could not open input file: " << fpath << std::endl;
        return;
    }

    std::string line;
    int doc_count {0};
    int total_tokens {0};
    while (std::getline(file, line)) {
        
        if (line.empty()) continue; 
        
        std::istringstream ss {line};
        std::string doc_id, passage;

        if (!std::getline(ss, doc_id, '\t') || !std::getline(ss, passage)) {
            continue; 
        }

        docMap[doc_count] = doc_id;

        std::stringstream tokens {passage};
        std::string token;
        while (tokens >> token) {
            invertedIndex[token][doc_count]++;
            ++total_tokens;
        }
        ++doc_count;
    }
    

    this->numPassages = doc_count;
    this->totalTerms = total_tokens;
    this->totalUniqueTerms = std::size(this->invertedIndex);

    std::cout << this->numPassages << this->totalTerms << this->totalUniqueTerms << '\n';
    
    for(const auto& [term, posting_list] : this->invertedIndex) {
        for(const auto& [pid, count] : posting_list) {
            this->docLenMap[pid] += count;
        }
    }

    int sum {};
    for(const auto& [pid, len] : this->docLenMap) {
        sum += len;
    }
    avgdl = sum / std::size(docLenMap);

    file.close();
    saveToDisk();
}

void Index::loadIndex() {
    std::string fname {this->iname + ".index"};
    std::ifstream is {fname, std::ios::binary};

    if(!is.is_open()) return;

    try {
        {
            cereal::BinaryInputArchive archive {is};
            archive(this->invertedIndex, this->docMap, this->numPassages, this->totalTerms, this->totalUniqueTerms, this->docLenMap, this->avgdl);
        }
        is.close();
    } 
    catch(const std::exception& e) {
        std::cerr << "Index corrupted, deleting and restarting: " << e.what() << '\n';
        std::filesystem::remove(fname); 
    }
}

void Index::saveToDisk() {
    std::string fname = this->iname + ".index";
    std::ofstream os{fname, std::ios::binary};
    
    if (os.is_open()) {
        try {
            {
                cereal::BinaryOutputArchive archive{os};
                archive(this->invertedIndex, this->docMap, this->numPassages, this->totalTerms, this->totalUniqueTerms, this->docLenMap, this->avgdl);
            } 
            os.close();
            std::cout << "Index saved successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Cereal save error: " << e.what() << '\n';
        }
    } else {
        std::cerr << "Failed to open " << fname << " for writing." << std::endl;
    }
}

Index::~Index() {
    try {
        invertedIndex.clear();
        docMap.clear();
        docLenMap.clear();
    } catch (const std::exception& e) {
        std::cerr << "Error during memory deallocation: " << e.what() << std::endl;
    }
}