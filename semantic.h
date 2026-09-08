#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class SemanticAnalyzer
{
private:
    std::unordered_map<std::string, std::string> symbolTable;
    int errorCount;

public:
    SemanticAnalyzer();

    bool analyze(const std::vector<ASTNode*>& statements);
    void analyzeNode(ASTNode* node);
    std::string evaluateType(ASTNode* node);

    int getErrorCount() const { return errorCount; }
};

#endif