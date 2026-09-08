#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

#include <string>
#include <vector>

class Parser
{
private:
    std::vector<Token> tokens;
    size_t pos;

    // ========================================================
    // Token helpers
    // ========================================================

    Token peek() const;
    Token peekNext() const;
    Token advance();

    bool check(const std::string& value) const;
    bool checkType(TokenType type) const;

    bool match(const std::string& value);
    bool matchType(TokenType type, Token& outToken);

    // ========================================================
    // Expression parsing
    // ========================================================

    ASTNode* primary();
    ASTNode* unary();

    ASTNode* multiplicative();
    ASTNode* additive();

    ASTNode* shift();

    ASTNode* comparison();
    ASTNode* equality();

    ASTNode* bitwiseAnd();
    ASTNode* bitwiseXor();
    ASTNode* bitwiseOr();

    ASTNode* logicalAnd();
    ASTNode* logicalOr();

    ASTNode* expression();

    // ========================================================
    // Statements
    // ========================================================

    ASTNode* varDeclaration();
    ASTNode* assignmentOrExpr();

    ASTNode* parseReturn();
    ASTNode* parseIf();
    ASTNode* parseWhile();
    ASTNode* parseFor();
    ASTNode* parseBlock();

    // ========================================================
    // Functions
    // ========================================================

    bool isTypeKeyword(const Token& token) const;
    ASTNode* parseFunction();

    // ========================================================
    // General statement parser
    // ========================================================

    ASTNode* statement();

public:

    // ========================================================
    // Constructor
    // ========================================================

    explicit Parser(std::vector<Token> tokens);

    // ========================================================
    // Parse entire program
    // ========================================================

    std::vector<ASTNode*> parseProgram();

    // ========================================================
    // Compatibility aliases
    // ========================================================

    ASTNode* factor()
    {
        return primary();
    }

    ASTNode* term()
    {
        return multiplicative();
    }

    ASTNode* logicalNot()
    {
        return unary();
    }
};

#endif