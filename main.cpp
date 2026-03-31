#include "common.h"

// Code transformer
class Transformer {
private:
    map<string, string> varRenaming;
    int counter;
    mt19937 rng;

    string getNewName(const string& prefix) {
        counter++;
        return prefix + "_" + to_string(counter) + "_" + to_string(rng() % 1000);
    }

    bool isBuiltin(const string& name) {
        static set<string> builtins = {
            "function", "block", "set", "print", "if", "while",
            "array.create", "array.get", "array.set", "array.length",
            "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=",
            "and", "or", "not", "return"
        };
        return builtins.count(name) > 0;
    }

    bool isNumber(const string& s) {
        if (s.empty()) return false;
        size_t start = 0;
        if (s[0] == '-') start = 1;
        for (size_t i = start; i < s.size(); i++) {
            if (!isdigit(s[i])) return false;
        }
        return start < s.size();
    }

    shared_ptr<SExpr> transform(shared_ptr<SExpr> expr) {
        if (!expr) return nullptr;

        if (expr->isAtom) {
            string val = expr->value;
            if (!isBuiltin(val) && !isNumber(val)) {
                if (varRenaming.count(val)) {
                    return make_shared<SExpr>(varRenaming[val]);
                }
            }
            return expr->clone();
        }

        auto result = make_shared<SExpr>();
        result->isAtom = false;

        if (expr->children.empty()) {
            return result;
        }

        // Handle function definitions
        if (expr->children[0]->isAtom && expr->children[0]->value == "function") {
            if (expr->children.size() >= 2) {
                result->children.push_back(expr->children[0]->clone());

                auto funcDef = expr->children[1];

                if (funcDef->isAtom) {
                    string newName = getNewName("f");
                    varRenaming[funcDef->value] = newName;
                    result->children.push_back(make_shared<SExpr>(newName));
                } else if (!funcDef->children.empty()) {
                    auto paramList = make_shared<SExpr>();
                    paramList->isAtom = false;

                    // Save old renaming, but preserve function names globally
                    map<string, string> oldRenaming = varRenaming;
                    string funcName;
                    string funcNewName;

                    for (size_t i = 0; i < funcDef->children.size(); i++) {
                        if (funcDef->children[i]->isAtom) {
                            string paramName = funcDef->children[i]->value;
                            if (i == 0) {
                                string newName = getNewName("f");
                                funcName = paramName;
                                funcNewName = newName;
                                varRenaming[paramName] = newName;
                                paramList->children.push_back(make_shared<SExpr>(newName));
                            } else {
                                string newName = getNewName("p");
                                varRenaming[paramName] = newName;
                                paramList->children.push_back(make_shared<SExpr>(newName));
                            }
                        } else {
                            paramList->children.push_back(funcDef->children[i]->clone());
                        }
                    }
                    result->children.push_back(paramList);

                    for (size_t i = 2; i < expr->children.size(); i++) {
                        result->children.push_back(transform(expr->children[i]));
                    }

                    // Restore old renaming but keep the function name globally
                    varRenaming = oldRenaming;
                    if (!funcName.empty()) {
                        varRenaming[funcName] = funcNewName;
                    }
                    return result;
                }

                for (size_t i = 2; i < expr->children.size(); i++) {
                    result->children.push_back(transform(expr->children[i]));
                }
                return result;
            }
        }

        // Handle set statements
        if (expr->children[0]->isAtom && expr->children[0]->value == "set") {
            if (expr->children.size() >= 3) {
                result->children.push_back(expr->children[0]->clone());

                auto varName = expr->children[1];
                if (varName->isAtom && !isBuiltin(varName->value)) {
                    if (!varRenaming.count(varName->value)) {
                        varRenaming[varName->value] = getNewName("v");
                    }
                    result->children.push_back(make_shared<SExpr>(varRenaming[varName->value]));
                } else {
                    result->children.push_back(transform(varName));
                }

                for (size_t i = 2; i < expr->children.size(); i++) {
                    result->children.push_back(transform(expr->children[i]));
                }
                return result;
            }
        }

        for (const auto& child : expr->children) {
            result->children.push_back(transform(child));
        }

        return result;
    }

public:
    Transformer() : counter(0), rng(time(nullptr) ^ 0x5a5a5a5a) {}

    vector<shared_ptr<SExpr>> transformProgram(const vector<shared_ptr<SExpr>>& program) {
        vector<shared_ptr<SExpr>> result;
        varRenaming.clear();
        counter = 0;

        for (const auto& expr : program) {
            result.push_back(transform(expr));
        }

        return result;
    }
};

// Similarity checker
class SimilarityChecker {
private:
    shared_ptr<SExpr> normalizeAST(shared_ptr<SExpr> expr) {
        if (!expr) return nullptr;

        if (expr->isAtom) {
            string val = expr->value;
            static set<string> keywords = {
                "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=",
                "and", "or", "not", "function", "block", "set", "print",
                "if", "while", "array.create", "array.get", "array.set", "array.length"
            };

            if (keywords.count(val)) {
                return expr->clone();
            }

            // Check if number
            bool isNum = true;
            size_t start = 0;
            if (!val.empty() && val[0] == '-') start = 1;
            for (size_t i = start; i < val.size(); i++) {
                if (!isdigit(val[i])) {
                    isNum = false;
                    break;
                }
            }

            if (isNum && start < val.size()) {
                return expr->clone();
            }

            return make_shared<SExpr>("ID");
        }

        auto result = make_shared<SExpr>();
        result->isAtom = false;

        for (const auto& child : expr->children) {
            result->children.push_back(normalizeAST(child));
        }

        return result;
    }

    double computeTreeSimilarity(shared_ptr<SExpr> e1, shared_ptr<SExpr> e2) {
        if (!e1 || !e2) return 0.0;

        if (e1->isAtom && e2->isAtom) {
            return e1->value == e2->value ? 1.0 : 0.0;
        }

        if (e1->isAtom != e2->isAtom) return 0.0;

        if (e1->children.size() != e2->children.size()) {
            return 0.2;
        }

        double sum = 0.0;
        for (size_t i = 0; i < e1->children.size(); i++) {
            sum += computeTreeSimilarity(e1->children[i], e2->children[i]);
        }

        return sum / max(1.0, (double)e1->children.size());
    }

    void extractFeatures(shared_ptr<SExpr> expr, map<string, int>& features) {
        if (!expr) return;

        if (expr->isAtom) {
            static set<string> ops = {
                "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=",
                "and", "or", "not", "function", "block", "set", "print",
                "if", "while", "array.create", "array.get", "array.set", "array.length"
            };
            if (ops.count(expr->value)) {
                features["op:" + expr->value]++;
            }
        } else {
            features["list:" + to_string(expr->children.size())]++;
            for (const auto& child : expr->children) {
                extractFeatures(child, features);
            }
        }
    }

    double computeFeatureSimilarity(const map<string, int>& f1, const map<string, int>& f2) {
        if (f1.empty() || f2.empty()) return 0.5;

        set<string> allFeatures;
        for (const auto& p : f1) allFeatures.insert(p.first);
        for (const auto& p : f2) allFeatures.insert(p.first);

        double dotProduct = 0.0, norm1 = 0.0, norm2 = 0.0;

        for (const auto& feat : allFeatures) {
            int v1 = f1.count(feat) ? f1.at(feat) : 0;
            int v2 = f2.count(feat) ? f2.at(feat) : 0;
            dotProduct += v1 * v2;
            norm1 += v1 * v1;
            norm2 += v2 * v2;
        }

        if (norm1 == 0 || norm2 == 0) return 0.5;
        return dotProduct / (sqrt(norm1) * sqrt(norm2));
    }

public:
    double computeSimilarity(const vector<shared_ptr<SExpr>>& prog1,
                            const vector<shared_ptr<SExpr>>& prog2) {
        if (prog1.empty() || prog2.empty()) return 0.5;

        double normalizedSim = 0.0;
        size_t minSize = min(prog1.size(), prog2.size());
        size_t maxSize = max(prog1.size(), prog2.size());

        for (size_t i = 0; i < minSize; i++) {
            auto n1 = normalizeAST(prog1[i]);
            auto n2 = normalizeAST(prog2[i]);
            normalizedSim += computeTreeSimilarity(n1, n2);
        }
        normalizedSim /= maxSize;

        map<string, int> features1, features2;
        for (const auto& expr : prog1) {
            extractFeatures(expr, features1);
        }
        for (const auto& expr : prog2) {
            extractFeatures(expr, features2);
        }

        double featureSim = computeFeatureSimilarity(features1, features2);
        double sizeSim = (double)minSize / maxSize;

        double similarity = 0.5 * normalizedSim + 0.3 * featureSim + 0.2 * sizeSim;
        return max(0.0, min(1.0, similarity));
    }
};

int main(int argc, char* argv[]) {
    // Read first program
    string program1 = readProgram(cin);

    // Try to read second program (for anticheat mode)
    string program2 = readProgram(cin);

    if (program2.empty() || program2.find_first_not_of(" \t\n\r") == string::npos) {
        // Only one program provided - CHEAT mode
        Parser parser(program1);
        auto exprs = parser.parseAll();

        Transformer transformer;
        auto transformed = transformer.transformProgram(exprs);

        for (const auto& expr : transformed) {
            expr->print(cout);
            cout << "\n";
        }
    } else {
        // Two programs provided - ANTICHEAT mode
        // Read any remaining test inputs (not used in basic check)
        string line;
        while (getline(cin, line)) {
        }

        Parser parser1(program1);
        auto exprs1 = parser1.parseAll();

        Parser parser2(program2);
        auto exprs2 = parser2.parseAll();

        SimilarityChecker checker;
        double similarity = checker.computeSimilarity(exprs1, exprs2);

        cout << fixed;
        cout.precision(6);
        cout << similarity << endl;
    }

    return 0;
}
