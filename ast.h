#ifndef AST_H
#define AST_H

#include <iostream>
#include <string>
#include <vector>

enum class NodeType
{
    NUMBER,
    IDENTIFIER,
    STRING_LITERAL,

    BINARY_OP,
    COMPARISON,

    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,

    ASSIGN,
    VAR_DECL,

    IF,
    WHILE,
    FOR,
    RETURN,
    BLOCK,

    FUNCTION,
    CALL
};

struct ASTNode
{
    NodeType type;
    std::string value;

    ASTNode* left;
    ASTNode* right;
    ASTNode* third;
    ASTNode* fourth;

    std::vector<ASTNode*> children;

    ASTNode(NodeType t, const std::string& v = "")
        : type(t),
          value(v),
          left(nullptr),
          right(nullptr),
          third(nullptr),
          fourth(nullptr)
    {
    }
};

inline void freeAST(ASTNode* node)
{
    if (!node)
        return;

    freeAST(node->left);
    freeAST(node->right);
    freeAST(node->third);
    freeAST(node->fourth);

    for (ASTNode* child : node->children)
    {
        freeAST(child);
    }

    delete node;
}

// ============================================================
// AST printing
// ============================================================

inline void printAST(const ASTNode* node, int depth = 0)
{
    if (!node)
        return;

    for (int i = 0; i < depth; ++i)
        std::cout << "  ";

    std::cout << node->value << "\n";

    // FUNCTION
    if (node->type == NodeType::FUNCTION)
    {
        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "PARAMETERS\n";

        for (const ASTNode* param : node->children)
        {
            printAST(param, depth + 2);
        }

        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "BODY\n";

        printAST(node->left, depth + 2);

        return;
    }

    // IF
    if (node->type == NodeType::IF)
    {
        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "CONDITION\n";
        printAST(node->left, depth + 2);

        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "THEN\n";
        printAST(node->right, depth + 2);

        if (node->third)
        {
            for (int i = 0; i < depth + 1; ++i)
                std::cout << "  ";

            std::cout << "ELSE\n";
            printAST(node->third, depth + 2);
        }

        return;
    }

    // WHILE
    if (node->type == NodeType::WHILE)
    {
        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "CONDITION\n";
        printAST(node->left, depth + 2);

        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "BODY\n";
        printAST(node->right, depth + 2);

        return;
    }

    // FOR
    if (node->type == NodeType::FOR)
    {
        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "INIT\n";
        printAST(node->left, depth + 2);

        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "CONDITION\n";
        printAST(node->right, depth + 2);

        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "UPDATE\n";
        printAST(node->third, depth + 2);

        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "BODY\n";
        printAST(node->fourth, depth + 2);

        return;
    }

    // RETURN
    if (node->type == NodeType::RETURN)
    {
        printAST(node->left, depth + 1);
        return;
    }

    // BLOCK
    if (node->type == NodeType::BLOCK)
    {
        for (const ASTNode* child : node->children)
        {
            printAST(child, depth + 1);
        }

        return;
    }

    // CALL
    if (node->type == NodeType::CALL)
    {
        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";

        std::cout << "ARGUMENTS\n";

        for (const ASTNode* argument : node->children)
        {
            printAST(argument, depth + 2);
        }

        return;
    }

    // Normal node
    printAST(node->left, depth + 1);
    printAST(node->right, depth + 1);
}

#endif