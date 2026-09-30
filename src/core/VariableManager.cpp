#include "VariableManager.h"
#include <cmath>
#include <cctype>
#include <cstring>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace materializr {

VariableManager::VariableManager() = default;

bool VariableManager::set(const std::string& name, const std::string& expression) {
    if (name.empty() || expression.empty()) return false;

    // Validate name: must start with letter or underscore, then alnum or underscore
    if (!std::isalpha(static_cast<unsigned char>(name[0])) && name[0] != '_')
        return false;
    for (size_t i = 1; i < name.size(); ++i) {
        char ch = name[i];
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_')
            return false;
    }

    Variable& var = m_variables[name];
    var.name = name;
    var.expression = expression;
    return true;
}

double VariableManager::get(const std::string& name) const {
    auto it = m_variables.find(name);
    if (it != m_variables.end() && it->second.valid) {
        return it->second.value;
    }
    return 0.0;
}

bool VariableManager::exists(const std::string& name) const {
    return m_variables.find(name) != m_variables.end();
}

void VariableManager::remove(const std::string& name) {
    m_variables.erase(name);
}

double VariableManager::evaluate(const std::string& expression, bool* ok) const {
    if (ok) *ok = true;
    const char* p = expression.c_str();
    m_evalDepth = 0;
    double result = parseExpression(p, ok);
    // Check for trailing characters
    skipWhitespace(p);
    if (*p != '\0' && ok && *ok) {
        *ok = false;
    }
    return result;
}

const std::unordered_map<std::string, Variable>& VariableManager::getAll() const {
    return m_variables;
}

void VariableManager::recalculate() {
    for (auto& [name, var] : m_variables) {
        bool ok = true;
        m_evalDepth = 0;
        const char* p = var.expression.c_str();
        double val = parseExpression(p, &ok);
        skipWhitespace(p);
        if (*p != '\0') ok = false;

        var.valid = ok;
        if (ok) {
            var.value = val;
            var.error.clear();
        } else {
            var.value = 0.0;
            var.error = "Invalid expression";
        }
    }
}

void VariableManager::clear() {
    m_variables.clear();
}

// --- Recursive descent parser ---

void VariableManager::skipWhitespace(const char*& p) const {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
}

// parseExpression: handles + and -
double VariableManager::parseExpression(const char*& p, bool* ok) const {
    double left = parseTerm(p, ok);
    if (ok && !*ok) return left;

    skipWhitespace(p);
    while (*p == '+' || *p == '-') {
        char op = *p;
        ++p;
        double right = parseTerm(p, ok);
        if (ok && !*ok) return left;
        if (op == '+')
            left += right;
        else
            left -= right;
        skipWhitespace(p);
    }
    return left;
}

// parseTerm: handles * and /
double VariableManager::parseTerm(const char*& p, bool* ok) const {
    double left = parseFactor(p, ok);
    if (ok && !*ok) return left;

    skipWhitespace(p);
    while (*p == '*' || *p == '/') {
        char op = *p;
        ++p;
        double right = parseFactor(p, ok);
        if (ok && !*ok) return left;
        if (op == '*') {
            left *= right;
        } else {
            if (std::abs(right) < 1e-15) {
                if (ok) *ok = false;
                return 0.0;
            }
            left /= right;
        }
        skipWhitespace(p);
    }
    return left;
}

// parseFactor: handles unary minus
double VariableManager::parseFactor(const char*& p, bool* ok) const {
    skipWhitespace(p);
    if (*p == '-') {
        ++p;
        return -parseFactor(p, ok);
    }
    if (*p == '+') {
        ++p;
        return parseFactor(p, ok);
    }
    return parseAtom(p, ok);
}

// parseAtom: handles numbers, variable references (identifiers), parentheses, and functions
double VariableManager::parseAtom(const char*& p, bool* ok) const {
    skipWhitespace(p);

    // Parenthesized expression
    if (*p == '(') {
        ++p;
        double val = parseExpression(p, ok);
        skipWhitespace(p);
        if (*p == ')') {
            ++p;
        } else {
            if (ok) *ok = false;
        }
        return val;
    }

    // Number literal
    if (std::isdigit(static_cast<unsigned char>(*p)) || *p == '.') {
        const char* start = p;
        // Integer part
        while (std::isdigit(static_cast<unsigned char>(*p))) ++p;
        // Decimal part
        if (*p == '.') {
            ++p;
            while (std::isdigit(static_cast<unsigned char>(*p))) ++p;
        }
        // Exponent part
        if (*p == 'e' || *p == 'E') {
            ++p;
            if (*p == '+' || *p == '-') ++p;
            if (!std::isdigit(static_cast<unsigned char>(*p))) {
                if (ok) *ok = false;
                return 0.0;
            }
            while (std::isdigit(static_cast<unsigned char>(*p))) ++p;
        }
        char* end = nullptr;
        double val = std::strtod(start, &end);
        if (end == start) {
            if (ok) *ok = false;
            return 0.0;
        }
        return val;
    }

    // Identifier: variable name or function
    if (std::isalpha(static_cast<unsigned char>(*p)) || *p == '_') {
        const char* start = p;
        while (std::isalnum(static_cast<unsigned char>(*p)) || *p == '_') ++p;
        std::string ident(start, p);

        skipWhitespace(p);

        // Check if it's a function call
        if (*p == '(') {
            ++p;

            // Single-argument functions
            if (ident == "abs" || ident == "sqrt" ||
                ident == "sin" || ident == "cos" || ident == "tan") {
                double arg = parseExpression(p, ok);
                skipWhitespace(p);

                // Check for two-arg call on single-arg function
                if (*p == ',') {
                    if (ok) *ok = false;
                    return 0.0;
                }

                if (*p == ')') {
                    ++p;
                } else {
                    if (ok) *ok = false;
                    return 0.0;
                }

                if (ident == "abs") return std::abs(arg);
                if (ident == "sqrt") {
                    if (arg < 0.0) { if (ok) *ok = false; return 0.0; }
                    return std::sqrt(arg);
                }
                if (ident == "sin") return std::sin(arg * M_PI / 180.0);
                if (ident == "cos") return std::cos(arg * M_PI / 180.0);
                if (ident == "tan") return std::tan(arg * M_PI / 180.0);
            }
            // Two-argument functions
            else if (ident == "min" || ident == "max") {
                double arg1 = parseExpression(p, ok);
                skipWhitespace(p);
                if (*p != ',') {
                    if (ok) *ok = false;
                    return 0.0;
                }
                ++p; // skip comma
                double arg2 = parseExpression(p, ok);
                skipWhitespace(p);
                if (*p == ')') {
                    ++p;
                } else {
                    if (ok) *ok = false;
                    return 0.0;
                }

                if (ident == "min") return std::min(arg1, arg2);
                if (ident == "max") return std::max(arg1, arg2);
            } else {
                // Unknown function
                if (ok) *ok = false;
                return 0.0;
            }
        }

        // Variable reference
        ++m_evalDepth;
        if (m_evalDepth > kMaxEvalDepth) {
            // Circular dependency or too deep
            if (ok) *ok = false;
            --m_evalDepth;
            return 0.0;
        }

        auto it = m_variables.find(ident);
        if (it != m_variables.end()) {
            // Re-evaluate the variable's expression to handle dependencies
            const char* vp = it->second.expression.c_str();
            bool varOk = true;
            double val = parseExpression(vp, &varOk);
            skipWhitespace(vp);
            if (*vp != '\0') varOk = false;
            --m_evalDepth;
            if (!varOk) {
                if (ok) *ok = false;
                return 0.0;
            }
            return val;
        }

        --m_evalDepth;
        // Unknown identifier
        if (ok) *ok = false;
        return 0.0;
    }

    // Nothing matched
    if (ok) *ok = false;
    return 0.0;
}

} // namespace materializr
