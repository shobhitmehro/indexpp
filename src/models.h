#include "index.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <ostream>

enum Model {
    NONE,
    BM25,
    QL,
    VSM
};

class Models {
    private:
        Model& mname;
        const Index& target;
        void bm25(int k, std::string& query, std::vector<std::pair<int, std::string>>& res, double b=0.75, double k1=1.2);
        void ql(int k, std::string& query, std::vector<std::pair<int, std::string>>& res, double lambda=0.2);
    public:
        Models(Model& name, const Index& ii);
        std::vector<std::pair<int, std::string>> execute(std::string& query, int k=5);

};

std::ostream& operator<<(std::ostream& os, const std::vector<std::pair<int, std::string>>& results);
