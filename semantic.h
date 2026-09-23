#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

// ============================================================
// Internal Type representation for fast O(1) comparisons
// ============================================================

enum class Type : uint8_t
{
    INT,
    BOOL,
    STRING,
    VOID,
    CHAR,
    FLOAT,
    DOUBLE,
    POINTER,
    UNKNOWN,
    TYPE_ERROR
};

// ============================================================
// Function information
// ============================================================

struct SymbolInfo
{
    Type type = Type::UNKNOWN;
    std::string typeStr;
    bool isArray = false;
    int arraySize = 0;
    bool isPointer = false;
    Type pointsTo = Type::UNKNOWN;
};

struct FunctionInfo
{
    Type returnType = Type::INT;
    std::string returnTypeStr = "int";

    std::vector<std::string> parameterNames;
    std::vector<Type> parameterTypes;
    std::vector<std::string> parameterTypeStrings;
};

// ============================================================
// Semantic Analyzer
// ============================================================

class SemanticAnalyzer
{
private:
    // --------------------------------------------------------
    // Stack of lexical scopes: variable name -> Type enum
    // --------------------------------------------------------
    std::vector<
        std::unordered_map<std::string, SymbolInfo>
    > scopes;

    // --------------------------------------------------------
    // Function table
    // --------------------------------------------------------
    std::unordered_map<std::string, FunctionInfo> functionTable;

    // --------------------------------------------------------
    // Current function state
    // --------------------------------------------------------
    std::string currentFunctionName;
    Type currentReturnType;
    std::string currentReturnTypeStr;
    bool insideFunction;

    // --------------------------------------------------------
    // Errors
    // --------------------------------------------------------
    int errorCount;

    // --------------------------------------------------------
    // Function handling
    // --------------------------------------------------------
    void collectFunctions(const std::vector<ASTNode*>& statements);
    void analyzeFunction(ASTNode* node);

    // --------------------------------------------------------
    // Scope handling
    // --------------------------------------------------------
    void enterScope();
    void exitScope();

    bool declareVariable(
        const std::string& name,
        Type type,
        bool isArray = false,
        int arraySize = 0,
        const std::string& typeStr = "",
        Type pointsTo = Type::UNKNOWN
    );

    SymbolInfo lookupSymbol(const std::string& name);
    Type lookupVariable(const std::string& name);

public:
    // --------------------------------------------------------
    // Fast Type helpers
    // --------------------------------------------------------
    static std::string typeToString(Type t);
    static Type stringToType(const std::string& str);

    static bool isNumericType(Type type);
    static bool isBooleanCompatible(Type type);
    static bool areTypesCompatible(Type expected, Type actual);

    // String compatibility overloads
    static bool isNumericType(const std::string& type)
    {
        return isNumericType(stringToType(type));
    }

    static bool isBooleanCompatible(const std::string& type)
    {
        return isBooleanCompatible(stringToType(type));
    }

    static bool areTypesCompatible(const std::string& expected, const std::string& actual)
    {
        return areTypesCompatible(stringToType(expected), stringToType(actual));
    }

    SemanticAnalyzer();

    bool analyze(const std::vector<ASTNode*>& statements);
    void analyzeNode(ASTNode* node);

    // Fast enum-based type evaluation
    Type evaluateNodeType(ASTNode* node);

    // Public API compatibility string method
    std::string evaluateType(ASTNode* node);

    int getErrorCount() const
    {
        return errorCount;
    }
};

#endif