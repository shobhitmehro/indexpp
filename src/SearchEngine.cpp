#include "SearchEngine.h"

SearchEngine::SearchEngine(const std::string& name) : sname {name} {};

void SearchEngine::initialize(const std::string& fpath) { this->buildIndex(fpath); };

void SearchEngine::buildIndex(const std::string& fpath) {
    std::string fname {this->sname + ".index"};
    if(std::filesystem::exists(fname)) {
        std::cout << "loading index from cache" << '\n';
        loadIndex();
    }
    else {
        std::cout << "building new index" << '\n';
        createNewIndex(fpath);
    }
}

void SearchEngine::createNewIndex(const std::string& fpath) {
    std::ifstream file {fpath};
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file!" << std::endl;
        return;
    }
    std::string line; 
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string doc_id, passage;

        std::getline(ss, doc_id, '\t');
        std::getline(ss, passage);

        std::cout << "ID: " << doc_id << "\n";

        std::stringstream tokens {passage};
        std::string token;
        
        int id = std::stoi(doc_id);
        while(tokens >> token) {
            invertedIndex[token][id]++;
        }

        break;
    }
}

void SearchEngine::loadIndex() {
    std::string fname {this->sname + ".index"};
    std::ifstream is {fname, std::ios::binary};

    if(!is.is_open()) throw std::runtime_error("failed to load inverted index file " + fname);

    try {
        cereal::BinaryInputArchive archive {is};
        archive(this->invertedIndex);
    } 
    catch(const cereal::Exception& e) {
        std::cerr << "cereal deserialization error: " << e.what() << '\n';
    }
}

void SearchEngine::saveToDisk() {
    std::string fname {this->sname + ".index"};
    std::ofstream os {fname, std::ios::binary};

    if(os.is_open()) {
        cereal::BinaryOutputArchive archive {os};
        archive(this->invertedIndex);
    }
}
