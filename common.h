#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <memory>
#include <algorithm>
#include <cmath>
#include <random>

using namespace std;

// S-expression structure
struct SExpr {
    string value;
    vector<shared_ptr<SExpr>> children;
    bool isAtom;

    SExpr(const string& v) : value(v), isAtom(true) {}
    SExpr() : isAtom(false) {}

    shared_ptr<SExpr> clone() const {
        auto result = make_shared<SExpr>();
        result->value = value;
        result->isAtom = isAtom;
        for (const auto& child : children) {
            result->children.push_back(child->clone());
        }
        return result;
    }

    void print(ostream& os) const {
        if (isAtom) {
            os << value;
        } else {
            os << "(";
            for (size_t i = 0; i < children.size(); i++) {
                if (i > 0) os << " ";
                children[i]->print(os);
            }
            os << ")";
        }
    }
};

// Parser for S-expressions
class Parser {
private:
    string input;
    size_t pos;

    void skipWhitespace() {
        while (pos < input.size() && isspace(input[pos])) {
            pos++;
        }
    }

    shared_ptr<SExpr> parseAtom() {
        skipWhitespace();
        if (pos >= input.size()) return nullptr;

        size_t start = pos;
        if (input[pos] == '-' || isdigit(input[pos])) {
            if (input[pos] == '-') pos++;
            while (pos < input.size() && isdigit(input[pos])) pos++;
        } else {
            while (pos < input.size() && !isspace(input[pos]) &&
                   input[pos] != '(' && input[pos] != ')') {
                pos++;
            }
        }

        string value = input.substr(start, pos - start);
        return make_shared<SExpr>(value);
    }

public:
    Parser(const string& s) : input(s), pos(0) {}

    shared_ptr<SExpr> parse() {
        skipWhitespace();
        if (pos >= input.size()) return nullptr;

        if (input[pos] == '(') {
            pos++;
            auto expr = make_shared<SExpr>();
            expr->isAtom = false;

            while (true) {
                skipWhitespace();
                if (pos >= input.size()) break;
                if (input[pos] == ')') {
                    pos++;
                    break;
                }
                auto child = parse();
                if (child) {
                    expr->children.push_back(child);
                } else {
                    break;
                }
            }
            return expr;
        } else {
            return parseAtom();
        }
    }

    vector<shared_ptr<SExpr>> parseAll() {
        vector<shared_ptr<SExpr>> result;
        while (pos < input.size()) {
            skipWhitespace();
            if (pos >= input.size()) break;
            auto expr = parse();
            if (expr) {
                result.push_back(expr);
            } else {
                break;
            }
        }
        return result;
    }
};

// Read program until "endprogram"
string readProgram(istream& is) {
    string result;
    string line;
    while (getline(is, line)) {
        if (line == "endprogram") break;
        result += line + "\n";
    }
    return result;
}

#endif // COMMON_H
