#include "semantic.h"

#include <iostream>

// ============================================================
// Constructor
// ============================================================

SemanticAnalyzer::SemanticAnalyzer()
    : currentFunctionName(""),
      currentReturnType(""),
      insideFunction(false),
      errorCount(0)
{
    // Start with global scope
    scopes.push_back({});
}


// ============================================================
// Enter a new scope
// ============================================================

void SemanticAnalyzer::enterScope()
{
    scopes.push_back({});
}


// ============================================================
// Exit current scope
// ============================================================

void SemanticAnalyzer::exitScope()
{
    if (scopes.size() > 1)
    {
        scopes.pop_back();
    }
}


// ============================================================
// Declare variable in current scope
// ============================================================

bool SemanticAnalyzer::declareVariable(
    const std::string& name,
    const std::string& type
)
{
    if (scopes.empty())
    {
        return false;
    }

    auto& currentScope = scopes.back();

    // Variable already exists in THIS scope
    if (currentScope.find(name) != currentScope.end())
    {
        std::cerr
            << "Semantic error: variable '"
            << name
            << "' is already declared in this scope.\n";

        ++errorCount;

        return false;
    }

    currentScope[name] = type;

    return true;
}


// ============================================================
// Look up variable
//
// Search from innermost scope → outermost scope
// ============================================================

std::string SemanticAnalyzer::lookupVariable(
    const std::string& name
)
{
    // Start at innermost scope
    for (int i = static_cast<int>(scopes.size()) - 1;
         i >= 0;
         --i)
    {
        auto it = scopes[i].find(name);

        if (it != scopes[i].end())
        {
            return it->second;
        }
    }

    // Not found
    return "unknown";
}


// ============================================================
// Analyze entire program
// ============================================================

bool SemanticAnalyzer::analyze(
    const std::vector<ASTNode*>& statements
)
{
    errorCount = 0;

    functionTable.clear();

    currentFunctionName = "";
    currentReturnType = "";
    insideFunction = false;

    // Start completely fresh with global scope
    scopes.clear();
    scopes.push_back({});

    // --------------------------------------------------------
    // First collect all functions.
    //
    // This allows a function to call another function even if
    // that function appears later in the source code.
    // --------------------------------------------------------

    collectFunctions(statements);

    // --------------------------------------------------------
    // Analyze global statements
    // --------------------------------------------------------

    for (ASTNode* stmt : statements)
    {
        analyzeNode(stmt);
    }

    return errorCount == 0;
}


// ============================================================
// Collect function definitions
// ============================================================

void SemanticAnalyzer::collectFunctions(
    const std::vector<ASTNode*>& statements
)
{
    for (ASTNode* node : statements)
    {
        if (!node)
        {
            continue;
        }

        if (node->type != NodeType::FUNCTION)
        {
            continue;
        }

        std::string functionName = node->value;

        // ----------------------------------------------------
        // Check duplicate function
        // ----------------------------------------------------

        if (functionTable.find(functionName) != functionTable.end())
        {
            std::cerr
                << "Semantic error: function '"
                << functionName
                << "' is already declared.\n";

            ++errorCount;

            continue;
        }

        FunctionInfo info;

        // ----------------------------------------------------
        // Return type
        //
        // FUNCTION.fourth contains return type
        // ----------------------------------------------------

        if (node->fourth)
        {
            info.returnType = node->fourth->value;
        }
        else
        {
            info.returnType = "int";
        }

        // ----------------------------------------------------
        // Parameters
        //
        // FUNCTION.children contains parameter nodes
        // ----------------------------------------------------

        for (ASTNode* parameter : node->children)
        {
            if (!parameter)
            {
                continue;
            }

            std::string parameterName = parameter->value;

            std::string parameterType = "int";

            if (parameter->fourth)
            {
                parameterType = parameter->fourth->value;
            }

            // Check duplicate parameter names
            for (const std::string& existingName :
                 info.parameterNames)
            {
                if (existingName == parameterName)
                {
                    std::cerr
                        << "Semantic error: parameter '"
                        << parameterName
                        << "' appears more than once in function '"
                        << functionName
                        << "'.\n";

                    ++errorCount;
                }
            }

            info.parameterNames.push_back(parameterName);
            info.parameterTypes.push_back(parameterType);
        }

        functionTable[functionName] = info;
    }
}


// ============================================================
// Analyze function
// ============================================================

void SemanticAnalyzer::analyzeFunction(ASTNode* node)
{
    if (!node)
    {
        return;
    }

    std::string previousFunctionName = currentFunctionName;
    std::string previousReturnType = currentReturnType;
    bool previousInsideFunction = insideFunction;

    // --------------------------------------------------------
    // Enter function
    // --------------------------------------------------------

    currentFunctionName = node->value;

    auto functionIt = functionTable.find(currentFunctionName);

    if (functionIt == functionTable.end())
    {
        return;
    }

    currentReturnType = functionIt->second.returnType;

    insideFunction = true;

    // --------------------------------------------------------
    // Function gets its own scope
    // --------------------------------------------------------

    enterScope();

    // --------------------------------------------------------
    // Add parameters to function scope
    // --------------------------------------------------------

    for (size_t i = 0;
         i < functionIt->second.parameterNames.size();
         ++i)
    {
        declareVariable(
            functionIt->second.parameterNames[i],
            functionIt->second.parameterTypes[i]
        );
    }

    // --------------------------------------------------------
    // Analyze function body
    // --------------------------------------------------------

    analyzeNode(node->left);

    // --------------------------------------------------------
    // Leave function scope
    // --------------------------------------------------------

    exitScope();

    // Restore previous state
    currentFunctionName = previousFunctionName;
    currentReturnType = previousReturnType;
    insideFunction = previousInsideFunction;
}


// ============================================================
// Analyze AST node
// ============================================================

void SemanticAnalyzer::analyzeNode(ASTNode* node)
{
    if (!node)
    {
        return;
    }

    switch (node->type)
    {
        // ====================================================
        // FUNCTION
        // ====================================================

        case NodeType::FUNCTION:
        {
            analyzeFunction(node);
            break;
        }


        // ====================================================
        // VARIABLE DECLARATION
        // ====================================================

        case NodeType::VAR_DECL:
        {
            if (!node->left)
            {
                break;
            }

            std::string variableName = node->left->value;

            std::string declaredType =
                node->value.empty()
                    ? "int"
                    : node->value;

            // Declare in CURRENT scope
            declareVariable(
                variableName,
                declaredType
            );

            // Check initializer
            if (node->right)
            {
                std::string rhsType =
                    evaluateType(node->right);

                if (rhsType != declaredType &&
                    rhsType != "unknown")
                {
                    std::cerr
                        << "Type error: cannot initialize "
                        << declaredType
                        << " variable '"
                        << variableName
                        << "' with "
                        << rhsType
                        << "\n";

                    ++errorCount;
                }
            }

            break;
        }


        // ====================================================
        // ASSIGNMENT
        // ====================================================

        case NodeType::ASSIGN:
        {
            if (!node->left)
            {
                break;
            }

            std::string variableName =
                node->left->value;

            std::string rhsType =
                evaluateType(node->right);

            // ------------------------------------------------
            // Search ALL visible scopes
            // ------------------------------------------------

            std::string variableType =
                lookupVariable(variableName);

            // ------------------------------------------------
            // Preserve the old behavior:
            //
            // If an assignment is made to an unknown variable,
            // treat it as an int variable in the current scope.
            // ------------------------------------------------

            if (variableType == "unknown")
            {
                std::cout
                    << "Declaring variable '"
                    << variableName
                    << "' as int in current scope\n";

                declareVariable(
                    variableName,
                    "int"
                );

                variableType = "int";
            }

            // ------------------------------------------------
            // Type checking
            // ------------------------------------------------

            if (rhsType != variableType &&
                rhsType != "unknown")
            {
                std::cerr
                    << "Type error: cannot assign "
                    << rhsType
                    << " to variable '"
                    << variableName
                    << "'\n";

                ++errorCount;
            }

            break;
        }


        // ====================================================
        // IF
        // ====================================================

        case NodeType::IF:
        {
            std::string conditionType =
                evaluateType(node->left);

            if (conditionType != "int" &&
                conditionType != "unknown")
            {
                std::cerr
                    << "Type error: if condition must be int/bool\n";

                ++errorCount;
            }

            // ------------------------------------------------
            // The block itself creates the scope.
            // ------------------------------------------------

            analyzeNode(node->right);

            if (node->third)
            {
                analyzeNode(node->third);
            }

            break;
        }


        // ====================================================
        // WHILE
        // ====================================================

        case NodeType::WHILE:
        {
            std::string conditionType =
                evaluateType(node->left);

            if (conditionType != "int" &&
                conditionType != "unknown")
            {
                std::cerr
                    << "Type error: while condition must be int/bool\n";

                ++errorCount;
            }

            analyzeNode(node->right);

            break;
        }


        // ====================================================
        // FOR
        // ====================================================

        case NodeType::FOR:
        {
            // ------------------------------------------------
            // A for-loop gets its own scope.
            //
            // This is important for:
            //
            // for (int i = 0; ... )
            //
            // i should disappear after the loop.
            // ------------------------------------------------

            enterScope();

            analyzeNode(node->left);

            if (node->right)
            {
                std::string conditionType =
                    evaluateType(node->right);

                if (conditionType != "int" &&
                    conditionType != "unknown")
                {
                    std::cerr
                        << "Type error: for condition must be int/bool\n";

                    ++errorCount;
                }
            }

            analyzeNode(node->third);
            analyzeNode(node->fourth);

            exitScope();

            break;
        }


        // ====================================================
        // RETURN
        // ====================================================

        case NodeType::RETURN:
        {
            if (!insideFunction)
            {
                // Top-level return is allowed by our current
                // compiler design.
                if (node->left)
                {
                    evaluateType(node->left);
                }

                break;
            }

            std::string actualType = "void";

            if (node->left)
            {
                actualType =
                    evaluateType(node->left);
            }

            // ------------------------------------------------
            // Check return type
            // ------------------------------------------------

            if (actualType != currentReturnType &&
                actualType != "unknown")
            {
                std::cerr
                    << "Semantic error: function '"
                    << currentFunctionName
                    << "' should return "
                    << currentReturnType
                    << " but returns "
                    << actualType
                    << ".\n";

                ++errorCount;
            }

            break;
        }


        // ====================================================
        // BLOCK
        // ====================================================

        case NodeType::BLOCK:
        {
            // ------------------------------------------------
            // Every block creates a new lexical scope.
            //
            // Example:
            //
            // {
            //     int x = 10;
            // }
            //
            // x disappears when we leave this block.
            // ------------------------------------------------

            enterScope();

            for (ASTNode* child : node->children)
            {
                analyzeNode(child);
            }

            exitScope();

            break;
        }


        // ====================================================
        // Default
        // ====================================================

        default:
        {
            evaluateType(node);
            break;
        }
    }
}


// ============================================================
// Evaluate expression type
// ============================================================

std::string SemanticAnalyzer::evaluateType(ASTNode* node)
{
    if (!node)
    {
        return "unknown";
    }

    switch (node->type)
    {
        // ====================================================
        // NUMBER
        // ====================================================

        case NodeType::NUMBER:
            return "int";


        // ====================================================
        // STRING
        // ====================================================

        case NodeType::STRING_LITERAL:
            return "string";


        // ====================================================
        // IDENTIFIER
        // ====================================================

        case NodeType::IDENTIFIER:
        {
            std::string type =
                lookupVariable(node->value);

            if (type == "unknown")
            {
                std::cerr
                    << "Semantic error: variable '"
                    << node->value
                    << "' used before declaration!\n";

                ++errorCount;

                return "unknown";
            }

            return type;
        }


        // ====================================================
        // BINARY OPERATION
        // ====================================================

        case NodeType::BINARY_OP:
        {
            std::string leftType =
                evaluateType(node->left);

            std::string rightType =
                evaluateType(node->right);

            if (leftType != "int" ||
                rightType != "int")
            {
                std::cerr
                    << "Type error in binary operation '"
                    << node->value
                    << "': "
                    << leftType
                    << " vs "
                    << rightType
                    << "\n";

                ++errorCount;

                return "unknown";
            }

            return "int";
        }


        // ====================================================
        // COMPARISON
        // ====================================================

        case NodeType::COMPARISON:
        {
            std::string leftType =
                evaluateType(node->left);

            std::string rightType =
                evaluateType(node->right);

            if (leftType != "int" ||
                rightType != "int")
            {
                std::cerr
                    << "Type error in comparison '"
                    << node->value
                    << "'\n";

                ++errorCount;

                return "unknown";
            }

            return "int";
        }


        // ====================================================
        // LOGICAL AND / OR
        // ====================================================

        case NodeType::LOGICAL_AND:
        case NodeType::LOGICAL_OR:
        {
            std::string leftType =
                evaluateType(node->left);

            std::string rightType =
                evaluateType(node->right);

            if (leftType != "int" ||
                rightType != "int")
            {
                std::cerr
                    << "Type error: operands of "
                    << node->value
                    << " must be int/bool\n";

                ++errorCount;

                return "unknown";
            }

            return "int";
        }


        // ====================================================
        // LOGICAL NOT
        // ====================================================

        case NodeType::LOGICAL_NOT:
        {
            std::string operandType =
                evaluateType(node->left);

            if (operandType != "int")
            {
                std::cerr
                    << "Type error: operand of ! must be int/bool\n";

                ++errorCount;

                return "unknown";
            }

            return "int";
        }


        // ====================================================
        // FUNCTION CALL
        // ====================================================

        case NodeType::CALL:
        {
            auto functionIt =
                functionTable.find(node->value);

            // ------------------------------------------------
            // Function doesn't exist
            // ------------------------------------------------

            if (functionIt == functionTable.end())
            {
                std::cerr
                    << "Semantic error: function '"
                    << node->value
                    << "' is not declared.\n";

                ++errorCount;

                // Still analyze arguments
                for (ASTNode* argument : node->children)
                {
                    evaluateType(argument);
                }

                return "unknown";
            }

            const FunctionInfo& function =
                functionIt->second;

            // ------------------------------------------------
            // Check argument count
            // ------------------------------------------------

            if (node->children.size() !=
                function.parameterTypes.size())
            {
                std::cerr
                    << "Semantic error: function '"
                    << node->value
                    << "' expects "
                    << function.parameterTypes.size()
                    << " argument(s), but "
                    << node->children.size()
                    << " were provided.\n";

                ++errorCount;
            }

            // ------------------------------------------------
            // Check individual argument types
            // ------------------------------------------------

            size_t count =
                node->children.size();

            if (count > function.parameterTypes.size())
            {
                count = function.parameterTypes.size();
            }

            for (size_t i = 0; i < count; ++i)
            {
                std::string argumentType =
                    evaluateType(node->children[i]);

                std::string expectedType =
                    function.parameterTypes[i];

                if (argumentType != expectedType &&
                    argumentType != "unknown")
                {
                    std::cerr
                        << "Semantic error: argument "
                        << (i + 1)
                        << " of function '"
                        << node->value
                        << "' should be "
                        << expectedType
                        << " but got "
                        << argumentType
                        << ".\n";

                    ++errorCount;
                }
            }

            // Analyze extra arguments too
            for (size_t i = count;
                 i < node->children.size();
                 ++i)
            {
                evaluateType(node->children[i]);
            }

            return function.returnType;
        }


        // ====================================================
        // Default
        // ====================================================

        default:
            return "unknown";
    }
}