#include "common.h"

// Similarity checker for plagiarism detection
class SimilarityChecker {
private:
    // Compute structural similarity between two ASTs
    double computeTreeSimilarity(shared_ptr<SExpr> e1, shared_ptr<SExpr> e2) {
        if (!e1 || !e2) return 0.0;

        if (e1->isAtom && e2->isAtom) {
            // For atoms, check if they are the same
            return e1->value == e2->value ? 1.0 : 0.0;
        }

        if (e1->isAtom != e2->isAtom) return 0.0;

        // Both are lists
        if (e1->children.size() != e2->children.size()) {
            // Different sizes mean different structure
            return 0.3; // Partial similarity
        }

        double sum = 0.0;
        for (size_t i = 0; i < e1->children.size(); i++) {
            sum += computeTreeSimilarity(e1->children[i], e2->children[i]);
        }

        return sum / max(1.0, (double)e1->children.size());
    }

    // Extract structural features from AST
    void extractStructuralFeatures(shared_ptr<SExpr> expr, map<string, int>& features) {
        if (!expr) return;

        if (expr->isAtom) {
            // Don't count specific identifiers, only operators
            if (expr->value == "+" || expr->value == "-" || expr->value == "*" ||
                expr->value == "/" || expr->value == "%" || expr->value == "<" ||
                expr->value == ">" || expr->value == "<=" || expr->value == ">=" ||
                expr->value == "==" || expr->value == "!=" || expr->value == "and" ||
                expr->value == "or" || expr->value == "not") {
                features["op:" + expr->value]++;
            } else if (expr->value == "function" || expr->value == "block" ||
                       expr->value == "set" || expr->value == "print" ||
                       expr->value == "if" || expr->value == "while") {
                features["kw:" + expr->value]++;
            }
        } else {
            // Count list structure
            features["list_size:" + to_string(expr->children.size())]++;

            if (!expr->children.empty() && expr->children[0]->isAtom) {
                features["op:" + expr->children[0]->value]++;
            }

            for (const auto& child : expr->children) {
                extractStructuralFeatures(child, features);
            }
        }
    }

    // Compute cosine similarity between feature vectors
    double computeFeatureSimilarity(const map<string, int>& f1, const map<string, int>& f2) {
        if (f1.empty() || f2.empty()) return 0.5;

        set<string> allFeatures;
        for (const auto& p : f1) allFeatures.insert(p.first);
        for (const auto& p : f2) allFeatures.insert(p.first);

        double dotProduct = 0.0;
        double norm1 = 0.0, norm2 = 0.0;

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

    // Count depth of AST
    int computeDepth(shared_ptr<SExpr> expr) {
        if (!expr) return 0;
        if (expr->isAtom) return 1;

        int maxDepth = 0;
        for (const auto& child : expr->children) {
            maxDepth = max(maxDepth, computeDepth(child));
        }
        return maxDepth + 1;
    }

    // Normalize AST by removing variable names
    shared_ptr<SExpr> normalizeAST(shared_ptr<SExpr> expr) {
        if (!expr) return nullptr;

        if (expr->isAtom) {
            // Replace identifiers with generic names, keep operators and keywords
            string val = expr->value;
            if (val == "+" || val == "-" || val == "*" || val == "/" || val == "%" ||
                val == "<" || val == ">" || val == "<=" || val == ">=" || val == "==" ||
                val == "!=" || val == "and" || val == "or" || val == "not" ||
                val == "function" || val == "block" || val == "set" || val == "print" ||
                val == "if" || val == "while" || val == "array.create" ||
                val == "array.get" || val == "array.set" || val == "array.length") {
                return expr->clone();
            }

            // Check if it's a number
            bool isNum = true;
            size_t start = 0;
            if (!val.empty() && val[0] == '-') start = 1;
            for (size_t i = start; i < val.size(); i++) {
                if (!isdigit(val[i])) {
                    isNum = false;
                    break;
                }
            }

            if (isNum) {
                return expr->clone();
            }

            // Otherwise, it's an identifier - replace with generic name
            return make_shared<SExpr>("ID");
        }

        auto result = make_shared<SExpr>();
        result->isAtom = false;

        for (const auto& child : expr->children) {
            result->children.push_back(normalizeAST(child));
        }

        return result;
    }

    // Compute normalized structural similarity
    double computeNormalizedSimilarity(shared_ptr<SExpr> e1, shared_ptr<SExpr> e2) {
        auto n1 = normalizeAST(e1);
        auto n2 = normalizeAST(e2);
        return computeTreeSimilarity(n1, n2);
    }

public:
    double computeSimilarity(const vector<shared_ptr<SExpr>>& prog1,
                            const vector<shared_ptr<SExpr>>& prog2) {
        if (prog1.empty() || prog2.empty()) return 0.5;

        // 1. Normalized structural similarity (ignore variable names)
        double normalizedSim = 0.0;
        size_t minSize = min(prog1.size(), prog2.size());
        size_t maxSize = max(prog1.size(), prog2.size());

        for (size_t i = 0; i < minSize; i++) {
            normalizedSim += computeNormalizedSimilarity(prog1[i], prog2[i]);
        }
        normalizedSim /= maxSize;

        // 2. Feature-based similarity
        map<string, int> features1, features2;
        for (const auto& expr : prog1) {
            extractStructuralFeatures(expr, features1);
        }
        for (const auto& expr : prog2) {
            extractStructuralFeatures(expr, features2);
        }

        double featureSim = computeFeatureSimilarity(features1, features2);

        // 3. Size similarity
        double sizeSim = (double)minSize / maxSize;

        // 4. Depth similarity
        int depth1 = 0, depth2 = 0;
        for (const auto& expr : prog1) {
            depth1 = max(depth1, computeDepth(expr));
        }
        for (const auto& expr : prog2) {
            depth2 = max(depth2, computeDepth(expr));
        }
        double depthSim = 1.0 - abs(depth1 - depth2) / (double)max(depth1, depth2);

        // Weighted combination
        double similarity = 0.5 * normalizedSim + 0.3 * featureSim + 0.1 * sizeSim + 0.1 * depthSim;

        // Clamp to [0, 1]
        return max(0.0, min(1.0, similarity));
    }
};

int main() {
    // Read two programs from stdin
    string program1 = readProgram(cin);
    string program2 = readProgram(cin);

    // Read test inputs (not used in basic similarity check)
    string testInputs;
    string line;
    while (getline(cin, line)) {
        testInputs += line + "\n";
    }

    // Parse both programs
    Parser parser1(program1);
    auto exprs1 = parser1.parseAll();

    Parser parser2(program2);
    auto exprs2 = parser2.parseAll();

    // Compute similarity
    SimilarityChecker checker;
    double similarity = checker.computeSimilarity(exprs1, exprs2);

    // Output similarity score
    cout << fixed;
    cout.precision(6);
    cout << similarity << endl;

    return 0;
}
