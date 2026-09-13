#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
// Information about a function
// ============================================================

struct FunctionInfo
{
    std::string returnType;

    std::vector<std::string> parameterNames;
    std::vector<std::string> parameterTypes;
};

// ============================================================
// Semantic Analyzer
// ============================================================

class SemanticAnalyzer
{
private:

    // --------------------------------------------------------
    // Scope stack
    //
    // scopes[0] = global scope
    // scopes[1] = function/block scope
    // scopes[2] = nested block
    // etc.
    // --------------------------------------------------------

    std::vector<
        std::unordered_map<std::string, std::string>
    > scopes;

    // --------------------------------------------------------
    // Function table
    // --------------------------------------------------------

    std::unordered_map<std::string, FunctionInfo> functionTable;

    // --------------------------------------------------------
    // Current function information
    // --------------------------------------------------------

    std::string currentFunctionName;
    std::string currentReturnType;
    bool insideFunction;

    // --------------------------------------------------------
    // Error counter
    // --------------------------------------------------------

    int errorCount;

    // --------------------------------------------------------
    // Function collection
    // --------------------------------------------------------

    void collectFunctions(
        const std::vector<ASTNode*>& statements
    );

    void analyzeFunction(ASTNode* node);

    // --------------------------------------------------------
    // Scope management
    // --------------------------------------------------------

    void enterScope();

    void exitScope();

    bool declareVariable(
        const std::string& name,
        const std::string& type
    );

    std::string lookupVariable(
        const std::string& name
    );

public:

    SemanticAnalyzer();

    bool analyze(
        const std::vector<ASTNode*>& statements
    );

    void analyzeNode(ASTNode* node);

    std::string evaluateType(ASTNode* node);

    int getErrorCount() const
    {
        return errorCount;
    }
};

#endif