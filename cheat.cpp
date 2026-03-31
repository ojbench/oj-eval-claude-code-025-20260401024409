#include "common.h"

// Code transformer that modifies programs to evade plagiarism detection
class Transformer {
private:
    map<string, string> varRenaming;
    map<string, string> funcRenaming;
    int counter;
    mt19937 rng;

    string getNewName(const string& prefix) {
        counter++;
        // Use more randomized names
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
        return true;
    }

    shared_ptr<SExpr> transform(shared_ptr<SExpr> expr, bool inFunction = false) {
        if (!expr) return nullptr;

        if (expr->isAtom) {
            string val = expr->value;
            // Rename identifiers (but not numbers or builtins)
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
                    // Simple function name
                    string newName = getNewName("func");
                    funcRenaming[funcDef->value] = newName;
                    varRenaming[funcDef->value] = newName;
                    result->children.push_back(make_shared<SExpr>(newName));
                } else if (!funcDef->children.empty()) {
                    // Function with parameters (funcname param1 param2...)
                    auto paramList = make_shared<SExpr>();
                    paramList->isAtom = false;

                    map<string, string> oldRenaming = varRenaming;

                    for (size_t i = 0; i < funcDef->children.size(); i++) {
                        if (funcDef->children[i]->isAtom) {
                            string paramName = funcDef->children[i]->value;
                            if (i == 0) {
                                // Function name
                                string newName = getNewName("func");
                                funcRenaming[paramName] = newName;
                                varRenaming[paramName] = newName;
                                paramList->children.push_back(make_shared<SExpr>(newName));
                            } else {
                                // Parameter name
                                string newName = getNewName("param");
                                varRenaming[paramName] = newName;
                                paramList->children.push_back(make_shared<SExpr>(newName));
                            }
                        } else {
                            paramList->children.push_back(funcDef->children[i]->clone());
                        }
                    }
                    result->children.push_back(paramList);

                    // Process function body
                    for (size_t i = 2; i < expr->children.size(); i++) {
                        result->children.push_back(transform(expr->children[i], true));
                    }

                    varRenaming = oldRenaming;
                    return result;
                }

                // Process body
                for (size_t i = 2; i < expr->children.size(); i++) {
                    result->children.push_back(transform(expr->children[i], true));
                }
                return result;
            }
        }

        // Handle set statements (variable assignments)
        if (expr->children[0]->isAtom && expr->children[0]->value == "set") {
            if (expr->children.size() >= 3) {
                result->children.push_back(expr->children[0]->clone());

                auto varName = expr->children[1];
                if (varName->isAtom && !isBuiltin(varName->value)) {
                    if (!varRenaming.count(varName->value)) {
                        varRenaming[varName->value] = getNewName("var");
                    }
                    result->children.push_back(make_shared<SExpr>(varRenaming[varName->value]));
                } else {
                    result->children.push_back(transform(varName, inFunction));
                }

                for (size_t i = 2; i < expr->children.size(); i++) {
                    result->children.push_back(transform(expr->children[i], inFunction));
                }
                return result;
            }
        }

        // Default: transform all children
        for (const auto& child : expr->children) {
            result->children.push_back(transform(child, inFunction));
        }

        return result;
    }

public:
    Transformer() : counter(0), rng(time(nullptr)) {}

    vector<shared_ptr<SExpr>> transformProgram(const vector<shared_ptr<SExpr>>& program) {
        vector<shared_ptr<SExpr>> result;
        varRenaming.clear();
        funcRenaming.clear();
        counter = 0;

        for (const auto& expr : program) {
            result.push_back(transform(expr));
        }

        return result;
    }
};

int main() {
    // Read program from stdin
    string program = readProgram(cin);
    Parser parser(program);
    auto exprs = parser.parseAll();

    // Transform the program
    Transformer transformer;
    auto transformed = transformer.transformProgram(exprs);

    // Output transformed program
    for (const auto& expr : transformed) {
        expr->print(cout);
        cout << "\n";
    }

    return 0;
}
