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
}

// ============================================================
// Collect all function definitions
//
// We do this BEFORE analyzing the program.
//
// This allows:
//
//     add(10, 20)
//
// to work even if add() is defined later in the source.
// ============================================================

void SemanticAnalyzer::collectFunctions(
    const std::vector<ASTNode*>& statements
)
{
    for (ASTNode* node : statements)
    {
        if (!node)
            continue;

        if (node->type != NodeType::FUNCTION)
            continue;

        std::string functionName = node->value;

        // Check for duplicate function definitions.
        if (functionTable.find(functionName) != functionTable.end())
        {
            std::cerr
                << "Semantic error: function '"
                << functionName
                << "' is already defined.\n";

            ++errorCount;
            continue;
        }

        FunctionInfo info;

        // ----------------------------------------------------
        // Return type
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
        // ----------------------------------------------------

        for (ASTNode* parameter : node->children)
        {
            if (!parameter)
                continue;

            std::string parameterName = parameter->value;

            std::string parameterType = "int";

            if (parameter->fourth)
            {
                parameterType = parameter->fourth->value;
            }

            info.parameterNames.push_back(parameterName);
            info.parameterTypes.push_back(parameterType);
        }

        functionTable[functionName] = info;
    }
}

// ============================================================
// Analyze entire program
// ============================================================

bool SemanticAnalyzer::analyze(
    const std::vector<ASTNode*>& statements
)
{
    errorCount = 0;

    symbolTable.clear();
    functionTable.clear();

    currentFunctionName.clear();
    currentReturnType.clear();
    insideFunction = false;

    // --------------------------------------------------------
    // First collect ALL functions.
    //
    // This must happen before checking function calls.
    // --------------------------------------------------------

    collectFunctions(statements);

    // --------------------------------------------------------
    // Now analyze everything.
    // --------------------------------------------------------

    for (ASTNode* stmt : statements)
    {
        analyzeNode(stmt);
    }

    return errorCount == 0;
}

// ============================================================
// Analyze a function
// ============================================================

void SemanticAnalyzer::analyzeFunction(ASTNode* node)
{
    if (!node)
        return;

    std::string functionName = node->value;

    auto functionIt = functionTable.find(functionName);

    if (functionIt == functionTable.end())
        return;

    const FunctionInfo& function = functionIt->second;

    // --------------------------------------------------------
    // Save the old semantic state.
    //
    // A function should have its own variables.
    // --------------------------------------------------------

    std::unordered_map<std::string, std::string>
        oldSymbolTable = symbolTable;

    std::string oldFunctionName = currentFunctionName;
    std::string oldReturnType = currentReturnType;
    bool oldInsideFunction = insideFunction;

    // --------------------------------------------------------
    // Enter function.
    // --------------------------------------------------------

    symbolTable.clear();

    currentFunctionName = functionName;
    currentReturnType = function.returnType;
    insideFunction = true;

    // --------------------------------------------------------
    // Add parameters to the function's symbol table.
    // --------------------------------------------------------

    for (size_t i = 0;
         i < function.parameterNames.size();
         ++i)
    {
        const std::string& name =
            function.parameterNames[i];

        const std::string& type =
            function.parameterTypes[i];

        // Check duplicate parameter names.

        if (symbolTable.find(name) != symbolTable.end())
        {
            std::cerr
                << "Semantic error: duplicate parameter '"
                << name
                << "' in function '"
                << functionName
                << "'.\n";

            ++errorCount;
        }
        else
        {
            symbolTable[name] = type;
        }
    }

    // --------------------------------------------------------
    // Analyze function body.
    // --------------------------------------------------------

    if (node->left)
    {
        analyzeNode(node->left);
    }

    // --------------------------------------------------------
    // Restore previous state.
    // --------------------------------------------------------

    symbolTable = oldSymbolTable;

    currentFunctionName = oldFunctionName;
    currentReturnType = oldReturnType;
    insideFunction = oldInsideFunction;
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
        // ====================================================
        // FUNCTION
        // ====================================================

        case NodeType::FUNCTION:
        {
            analyzeFunction(node);
            break;
        }

        // ====================================================
        // ASSIGNMENT
        // ====================================================

        case NodeType::ASSIGN:
        {
            if (!node->left)
                break;

            std::string varName =
                node->left->value;

            std::string rhsType =
                evaluateType(node->right);

            // Existing compiler behavior:
            // first assignment creates an int variable.

            if (symbolTable.find(varName)
                == symbolTable.end())
            {
                symbolTable[varName] = "int";
            }

            std::string variableType =
                symbolTable[varName];

            if (rhsType != "unknown" &&
                rhsType != variableType)
            {
                std::cerr
                    << "Type error: cannot assign "
                    << rhsType
                    << " to variable '"
                    << varName
                    << "' of type "
                    << variableType
                    << ".\n";

                ++errorCount;
            }

            break;
        }

        // ====================================================
        // VARIABLE DECLARATION
        // ====================================================

        case NodeType::VAR_DECL:
        {
            if (!node->left)
                break;

            std::string varName =
                node->left->value;

            std::string declaredType =
                node->value.empty()
                    ? "int"
                    : node->value;

            // Duplicate variable declaration.

            if (symbolTable.find(varName)
                != symbolTable.end())
            {
                std::cerr
                    << "Semantic error: variable '"
                    << varName
                    << "' is already declared.\n";

                ++errorCount;
            }
            else
            {
                symbolTable[varName] = declaredType;
            }

            // Check initializer.

            if (node->right)
            {
                std::string rhsType =
                    evaluateType(node->right);

                if (rhsType != "unknown" &&
                    rhsType != declaredType)
                {
                    std::cerr
                        << "Type error: cannot initialize "
                        << declaredType
                        << " variable '"
                        << varName
                        << "' with "
                        << rhsType
                        << ".\n";

                    ++errorCount;
                }
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

            if (conditionType != "unknown" &&
                conditionType != "int")
            {
                std::cerr
                    << "Type error: if condition "
                    << "must be int/bool.\n";

                ++errorCount;
            }

            analyzeNode(node->right);
            analyzeNode(node->third);

            break;
        }

        // ====================================================
        // WHILE
        // ====================================================

        case NodeType::WHILE:
        {
            std::string conditionType =
                evaluateType(node->left);

            if (conditionType != "unknown" &&
                conditionType != "int")
            {
                std::cerr
                    << "Type error: while condition "
                    << "must be int/bool.\n";

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
            analyzeNode(node->left);

            if (node->right)
            {
                std::string conditionType =
                    evaluateType(node->right);

                if (conditionType != "unknown" &&
                    conditionType != "int")
                {
                    std::cerr
                        << "Type error: for condition "
                        << "must be int/bool.\n";

                    ++errorCount;
                }
            }

            analyzeNode(node->third);
            analyzeNode(node->fourth);

            break;
        }

        // ====================================================
        // RETURN
        // ====================================================

        case NodeType::RETURN:
        {
            std::string returnType = "void";

            if (node->left)
            {
                returnType =
                    evaluateType(node->left);
            }

            // Only perform function return checking
            // when actually inside a function.

            if (insideFunction)
            {
                if (returnType != "unknown" &&
                    returnType != currentReturnType)
                {
                    std::cerr
                        << "Type error: function '"
                        << currentFunctionName
                        << "' should return "
                        << currentReturnType
                        << " but returns "
                        << returnType
                        << ".\n";

                    ++errorCount;
                }
            }

            break;
        }

        // ====================================================
        // BLOCK
        // ====================================================

        case NodeType::BLOCK:
        {
            for (ASTNode* child : node->children)
            {
                analyzeNode(child);
            }

            break;
        }

        // ====================================================
        // DEFAULT
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

std::string SemanticAnalyzer::evaluateType(
    ASTNode* node
)
{
    if (!node)
        return "unknown";

    switch (node->type)
    {
        // ====================================================
        // NUMBER
        // ====================================================

        case NodeType::NUMBER:
        {
            return "int";
        }

        // ====================================================
        // STRING
        // ====================================================

        case NodeType::STRING_LITERAL:
        {
            return "string";
        }

        // ====================================================
        // IDENTIFIER
        // ====================================================

        case NodeType::IDENTIFIER:
        {
            auto it =
                symbolTable.find(node->value);

            if (it == symbolTable.end())
            {
                std::cerr
                    << "Semantic error: variable '"
                    << node->value
                    << "' used before declaration.\n";

                ++errorCount;

                return "unknown";
            }

            return it->second;
        }

        // ====================================================
        // FUNCTION CALL
        // ====================================================

        case NodeType::CALL:
        {
            std::string functionName =
                node->value;

            // ------------------------------------------------
            // Does the function exist?
            // ------------------------------------------------

            auto functionIt =
                functionTable.find(functionName);

            if (functionIt == functionTable.end())
            {
                std::cerr
                    << "Semantic error: function '"
                    << functionName
                    << "' is not declared.\n";

                ++errorCount;

                // Still analyze arguments so we can find
                // additional errors.

                for (ASTNode* argument : node->children)
                {
                    evaluateType(argument);
                }

                return "unknown";
            }

            const FunctionInfo& function =
                functionIt->second;

            // ------------------------------------------------
            // Check argument count.
            // ------------------------------------------------

            if (node->children.size()
                != function.parameterTypes.size())
            {
                std::cerr
                    << "Semantic error: function '"
                    << functionName
                    << "' expects "
                    << function.parameterTypes.size()
                    << " argument(s), but "
                    << node->children.size()
                    << " were provided.\n";

                ++errorCount;
            }

            // ------------------------------------------------
            // Check each argument.
            // ------------------------------------------------

            size_t count =
                node->children.size();

            if (count >
                function.parameterTypes.size())
            {
                count =
                    function.parameterTypes.size();
            }

            for (size_t i = 0; i < count; ++i)
            {
                std::string argumentType =
                    evaluateType(node->children[i]);

                const std::string& parameterType =
                    function.parameterTypes[i];

                if (argumentType != "unknown" &&
                    argumentType != parameterType)
                {
                    std::cerr
                        << "Type error: argument "
                        << (i + 1)
                        << " of function '"
                        << functionName
                        << "' expects "
                        << parameterType
                        << " but got "
                        << argumentType
                        << ".\n";

                    ++errorCount;
                }
            }

            // Analyze extra arguments too.

            for (size_t i = count;
                 i < node->children.size();
                 ++i)
            {
                evaluateType(node->children[i]);
            }

            // ------------------------------------------------
            // The type of a function call is its return type.
            // ------------------------------------------------

            return function.returnType;
        }

        // ====================================================
        // BINARY OPERATIONS
        // ====================================================

        case NodeType::BINARY_OP:
        {
            std::string leftType =
                evaluateType(node->left);

            std::string rightType =
                evaluateType(node->right);

            if (leftType == "unknown" ||
                rightType == "unknown")
            {
                return "unknown";
            }

            if (leftType != "int" ||
                rightType != "int")
            {
                std::cerr
                    << "Type error in binary operation '"
                    << node->value
                    << "': operands must be int.\n";

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

            if (leftType == "unknown" ||
                rightType == "unknown")
            {
                return "unknown";
            }

            if (leftType != "int" ||
                rightType != "int")
            {
                std::cerr
                    << "Type error: operands of comparison '"
                    << node->value
                    << "' must be int.\n";

                ++errorCount;

                return "unknown";
            }

            // Comparisons produce int/bool-like values.

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

            if (leftType == "unknown" ||
                rightType == "unknown")
            {
                return "unknown";
            }

            if (leftType != "int" ||
                rightType != "int")
            {
                std::cerr
                    << "Type error: operands of '"
                    << node->value
                    << "' must be int/bool.\n";

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

            if (operandType == "unknown")
                return "unknown";

            if (operandType != "int")
            {
                std::cerr
                    << "Type error: operand of ! "
                    << "must be int/bool.\n";

                ++errorCount;

                return "unknown";
            }

            return "int";
        }

        // ====================================================
        // Other nodes
        // ====================================================

        default:
        {
            return "unknown";
        }
    }
}