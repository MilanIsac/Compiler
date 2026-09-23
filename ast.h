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
    BOOL_LITERAL,

    BINARY_OP,
    COMPARISON,

    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,

    ASSIGN,
    VAR_DECL,

    ARRAY_DECL,
    ARRAY_ACCESS,

    ADDRESS_OF,
    DEREFERENCE,

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
    std::string inferredType;

    ASTNode* left;
    ASTNode* right;
    ASTNode* third;
    ASTNode* fourth;

    std::vector<ASTNode*> children;

    ASTNode(NodeType t, const std::string& v = "")
        : type(t),
          value(v),
          inferredType(""),
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

inline ASTNode* cloneAST(const ASTNode* node)
{
    if (!node)
        return nullptr;

    ASTNode* copy = new ASTNode(node->type, node->value);
    copy->inferredType = node->inferredType;
    copy->left = cloneAST(node->left);
    copy->right = cloneAST(node->right);
    copy->third = cloneAST(node->third);
    copy->fourth = cloneAST(node->fourth);

    for (const ASTNode* child : node->children)
    {
        copy->children.push_back(cloneAST(child));
    }

    return copy;
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

    // ARRAY ACCESS
    if (node->type == NodeType::ARRAY_ACCESS)
    {
        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";
        std::cout << "INDEX\n";
        printAST(node->left, depth + 2);
        return;
    }

    // ARRAY DECLARATION
    if (node->type == NodeType::ARRAY_DECL)
    {
        for (int i = 0; i < depth + 1; ++i)
            std::cout << "  ";
        std::cout << "SIZE\n";
        printAST(node->right, depth + 2);
        return;
    }

    // ADDRESS OF (&x)
    if (node->type == NodeType::ADDRESS_OF)
    {
        printAST(node->left, depth + 1);
        return;
    }

    // DEREFERENCE (*p)
    if (node->type == NodeType::DEREFERENCE)
    {
        printAST(node->left, depth + 1);
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