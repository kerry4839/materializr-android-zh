#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace materializr {

struct Variable {
    std::string name;
    std::string expression; // e.g. "50", "width * 0.6", "height + 10"
    double value = 0.0;
    bool valid = true;
    std::string error;
};

class VariableManager {
public:
    VariableManager();

    // Define or update a variable
    bool set(const std::string& name, const std::string& expression);

    // Get a variable's resolved value
    double get(const std::string& name) const;
    bool exists(const std::string& name) const;

    // Remove a variable
    void remove(const std::string& name);

    // Evaluate an expression string (can reference other variables)
    double evaluate(const std::string& expression, bool* ok = nullptr) const;

    // Get all variables
    const std::unordered_map<std::string, Variable>& getAll() const;

    // Recalculate all variables (call after any change)
    void recalculate();

    void clear();

private:
    std::unordered_map<std::string, Variable> m_variables;

    // Simple expression parser
    double parseExpression(const char*& p, bool* ok) const;
    double parseTerm(const char*& p, bool* ok) const;
    double parseFactor(const char*& p, bool* ok) const;
    double parseAtom(const char*& p, bool* ok) const;
    void skipWhitespace(const char*& p) const;

    // Recursion depth tracking for circular dependency detection
    mutable int m_evalDepth = 0;
    static constexpr int kMaxEvalDepth = 50;
};

} // namespace materializr
