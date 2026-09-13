#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct FunctionInfo
{
    std::string returnType;

    std::vector<std::string> parameterNames;
    std::vector<std::string> parameterTypes;
};

class SemanticAnalyzer
{
private:
    // Variables currently visible.
    std::unordered_map<std::string, std::string> symbolTable;

    // All functions in the program.
    std::unordered_map<std::string, FunctionInfo> functionTable;

    // Information about the function currently being analyzed.
    std::string currentFunctionName;
    std::string currentReturnType;
    bool insideFunction;

    int errorCount;

    void collectFunctions(
        const std::vector<ASTNode*>& statements
    );

    void analyzeFunction(ASTNode* node);

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