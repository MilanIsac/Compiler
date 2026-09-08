#include "semantic.h"

#include <iostream>

// ============================================================
// Constructor
// ============================================================

SemanticAnalyzer::SemanticAnalyzer()
    : errorCount(0)
{
}

// ============================================================
// Analyze program
// ============================================================

bool SemanticAnalyzer::analyze(const std::vector<ASTNode*>& statements)
{
    errorCount = 0;
    for (ASTNode* stmt : statements)
    {
        analyzeNode(stmt);
    }
    return errorCount == 0;
}

// ============================================================
// Analyze node
// ============================================================

void SemanticAnalyzer::analyzeNode(ASTNode* node)
{
    if (!node)
        return;

    switch (node->type)
    {
        // ----------------------------------------------------
        // Assignment
        // ----------------------------------------------------
        case NodeType::ASSIGN:
        {
            std::string varName = node->left->value;
            std::string rhsType = evaluateType(node->right);

            // First assignment declares variable if undeclared
            if (symbolTable.find(varName) == symbolTable.end())
            {
                std::cout << "Declaring variable '" << varName << "' as int\n";
                symbolTable[varName] = "int";
            }

            if (rhsType != "int")
            {
                std::cerr << "Type error: cannot assign " << rhsType
                          << " to variable '" << varName << "'\n";
                ++errorCount;
            }
            break;
        }

        // ----------------------------------------------------
        // Variable Declaration
        // ----------------------------------------------------
        case NodeType::VAR_DECL:
        {
            std::string varName = node->left->value;
            std::string declaredType = node->value.empty() ? "int" : node->value;
            symbolTable[varName] = declaredType;

            if (node->right)
            {
                std::string rhsType = evaluateType(node->right);
                if (rhsType != declaredType && rhsType != "unknown")
                {
                    std::cerr << "Type error: cannot initialize " << declaredType
                              << " variable '" << varName << "' with " << rhsType << "\n";
                    ++errorCount;
                }
            }
            break;
        }

        // ----------------------------------------------------
        // IF
        // ----------------------------------------------------
        case NodeType::IF:
        {
            std::string conditionType = evaluateType(node->left);
            if (conditionType != "int")
            {
                std::cerr << "Type error: if condition must be int/bool\n";
                ++errorCount;
            }

            analyzeNode(node->right);
            analyzeNode(node->third);
            break;
        }

        // ----------------------------------------------------
        // WHILE
        // ----------------------------------------------------
        case NodeType::WHILE:
        {
            std::string conditionType = evaluateType(node->left);
            if (conditionType != "int")
            {
                std::cerr << "Type error: while condition must be int/bool\n";
                ++errorCount;
            }

            analyzeNode(node->right);
            break;
        }

        // ----------------------------------------------------
        // FOR
        // ----------------------------------------------------
        case NodeType::FOR:
        {
            analyzeNode(node->left);

            if (node->right)
            {
                std::string conditionType = evaluateType(node->right);
                if (conditionType != "int")
                {
                    std::cerr << "Type error: for condition must be int/bool\n";
                    ++errorCount;
                }
            }

            analyzeNode(node->third);
            analyzeNode(node->fourth);
            break;
        }

        // ----------------------------------------------------
        // RETURN
        // ----------------------------------------------------
        case NodeType::RETURN:
        {
            if (node->left)
            {
                evaluateType(node->left);
            }
            break;
        }

        // ----------------------------------------------------
        // BLOCK
        // ----------------------------------------------------
        case NodeType::BLOCK:
        {
            for (ASTNode* child : node->children)
            {
                analyzeNode(child);
            }
            break;
        }

        default:
        {
            evaluateType(node);
            break;
        }
    }
}

// ============================================================
// Evaluate type
// ============================================================

std::string SemanticAnalyzer::evaluateType(ASTNode* node)
{
    if (!node)
        return "unknown";

    switch (node->type)
    {
        case NodeType::NUMBER:
            return "int";

        case NodeType::STRING_LITERAL:
            return "string";

        case NodeType::IDENTIFIER:
        {
            auto it = symbolTable.find(node->value);
            if (it == symbolTable.end())
            {
                std::cerr << "Semantic error: variable '" << node->value
                          << "' used before declaration!\n";
                ++errorCount;
                return "unknown";
            }
            return it->second;
        }

        case NodeType::BINARY_OP:
        {
            std::string leftType = evaluateType(node->left);
            std::string rightType = evaluateType(node->right);

            if (leftType != "int" || rightType != "int")
            {
                std::cerr << "Type error in binary operation '" << node->value
                          << "': " << leftType << " vs " << rightType << "\n";
                ++errorCount;
                return "unknown";
            }
            return "int";
        }

        case NodeType::COMPARISON:
        {
            std::string leftType = evaluateType(node->left);
            std::string rightType = evaluateType(node->right);

            if (leftType != "int" || rightType != "int")
            {
                std::cerr << "Type error in comparison '" << node->value << "'\n";
                ++errorCount;
                return "unknown";
            }
            return "int";
        }

        case NodeType::LOGICAL_AND:
        case NodeType::LOGICAL_OR:
        {
            std::string leftType = evaluateType(node->left);
            std::string rightType = evaluateType(node->right);

            if (leftType != "int" || rightType != "int")
            {
                std::cerr << "Type error: operands of " << node->value << " must be int/bool\n";
                ++errorCount;
                return "unknown";
            }
            return "int";
        }

        case NodeType::LOGICAL_NOT:
        {
            std::string operandType = evaluateType(node->left);
            if (operandType != "int")
            {
                std::cerr << "Type error: operand of ! must be int/bool\n";
                ++errorCount;
                return "unknown";
            }
            return "int";
        }

        default:
            return "unknown";
    }
}