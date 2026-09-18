#include "semantic.h"

#include <iostream>
#include <algorithm>

// ============================================================
// Constructor
// ============================================================

SemanticAnalyzer::SemanticAnalyzer()
    : currentFunctionName(""),
      currentReturnType(Type::INT),
      currentReturnTypeStr("int"),
      insideFunction(false),
      errorCount(0)
{
    scopes.reserve(16);
    scopes.push_back({});
}

// ============================================================
// String to Type conversion
// ============================================================

Type SemanticAnalyzer::stringToType(const std::string& str)
{
    if (str == "int")    return Type::INT;
    if (str == "bool")   return Type::BOOL;
    if (str == "string") return Type::STRING;
    if (str == "void")   return Type::VOID;
    if (str == "char")   return Type::CHAR;
    if (str == "float")  return Type::FLOAT;
    if (str == "double") return Type::DOUBLE;
    return Type::UNKNOWN;
}

// ============================================================
// Type to String conversion
// ============================================================

std::string SemanticAnalyzer::typeToString(Type t)
{
    switch (t)
    {
        case Type::INT:        return "int";
        case Type::BOOL:       return "bool";
        case Type::STRING:     return "string";
        case Type::VOID:       return "void";
        case Type::CHAR:       return "char";
        case Type::FLOAT:      return "float";
        case Type::DOUBLE:     return "double";
        default:               return "unknown";
    }
}

// ============================================================
// Numeric type check
// ============================================================

bool SemanticAnalyzer::isNumericType(Type type)
{
    return type == Type::INT ||
           type == Type::CHAR ||
           type == Type::FLOAT ||
           type == Type::DOUBLE;
}

// ============================================================
// Boolean compatibility check
// ============================================================

bool SemanticAnalyzer::isBooleanCompatible(Type type)
{
    return type == Type::BOOL ||
           type == Type::INT;
}

// ============================================================
// Type compatibility check
// ============================================================

bool SemanticAnalyzer::areTypesCompatible(Type expected, Type actual)
{
    if (expected == Type::UNKNOWN || actual == Type::UNKNOWN ||
        expected == Type::TYPE_ERROR || actual == Type::TYPE_ERROR)
    {
        return true;
    }

    if (expected == actual)
    {
        return true;
    }

    // char -> int promotion
    if (expected == Type::INT && actual == Type::CHAR)
    {
        return true;
    }

    return false;
}

// ============================================================
// Scope management
// ============================================================

void SemanticAnalyzer::enterScope()
{
    scopes.push_back({});
}

void SemanticAnalyzer::exitScope()
{
    if (scopes.size() > 1)
    {
        scopes.pop_back();
    }
}

bool SemanticAnalyzer::declareVariable(const std::string& name, Type type)
{
    if (scopes.empty())
    {
        return false;
    }

    auto& currentScope = scopes.back();

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

Type SemanticAnalyzer::lookupVariable(const std::string& name)
{
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i)
    {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end())
        {
            return it->second;
        }
    }

    return Type::UNKNOWN;
}

// ============================================================
// Analyze program
// ============================================================

bool SemanticAnalyzer::analyze(const std::vector<ASTNode*>& statements)
{
    errorCount = 0;
    functionTable.clear();

    currentFunctionName = "";
    currentReturnType = Type::INT;
    currentReturnTypeStr = "int";
    insideFunction = false;

    scopes.clear();
    scopes.push_back({});

    // 1. Collect all function signatures first (allows mutual recursion)
    collectFunctions(statements);

    // 2. Analyze all top-level statements / function definitions
    for (ASTNode* statement : statements)
    {
        analyzeNode(statement);
    }

    return errorCount == 0;
}

// ============================================================
// Collect functions
// ============================================================

void SemanticAnalyzer::collectFunctions(const std::vector<ASTNode*>& statements)
{
    for (ASTNode* node : statements)
    {
        if (!node || node->type != NodeType::FUNCTION)
        {
            continue;
        }

        std::string functionName = node->value;

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

        if (node->fourth)
        {
            info.returnTypeStr = node->fourth->value;
            info.returnType = stringToType(info.returnTypeStr);
        }
        else
        {
            info.returnTypeStr = "int";
            info.returnType = Type::INT;
        }

        for (ASTNode* parameter : node->children)
        {
            if (!parameter)
            {
                continue;
            }

            std::string parameterName = parameter->value;
            std::string parameterTypeStr = "int";

            if (parameter->fourth)
            {
                parameterTypeStr = parameter->fourth->value;
            }

            Type paramType = stringToType(parameterTypeStr);

            if (paramType == Type::VOID)
            {
                std::cerr
                    << "Type error: parameter '"
                    << parameterName
                    << "' of function '"
                    << functionName
                    << "' cannot be of type 'void'.\n";

                ++errorCount;
            }

            for (const std::string& existingName : info.parameterNames)
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
            info.parameterTypes.push_back(paramType);
            info.parameterTypeStrings.push_back(parameterTypeStr);
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
    Type previousReturnType = currentReturnType;
    std::string previousReturnTypeStr = currentReturnTypeStr;
    bool previousInsideFunction = insideFunction;

    currentFunctionName = node->value;

    auto functionIt = functionTable.find(currentFunctionName);
    if (functionIt == functionTable.end())
    {
        return;
    }

    currentReturnType = functionIt->second.returnType;
    currentReturnTypeStr = functionIt->second.returnTypeStr;
    insideFunction = true;

    enterScope();

    for (size_t i = 0; i < functionIt->second.parameterNames.size(); ++i)
    {
        declareVariable(
            functionIt->second.parameterNames[i],
            functionIt->second.parameterTypes[i]
        );
    }

    analyzeNode(node->left);

    exitScope();

    currentFunctionName = previousFunctionName;
    currentReturnType = previousReturnType;
    currentReturnTypeStr = previousReturnTypeStr;
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
        case NodeType::FUNCTION:
        {
            analyzeFunction(node);
            break;
        }

        case NodeType::VAR_DECL:
        {
            if (!node->left)
            {
                break;
            }

            std::string variableName = node->left->value;
            std::string declaredTypeStr = node->value.empty() ? "int" : node->value;
            Type declaredType = stringToType(declaredTypeStr);

            if (declaredType == Type::VOID)
            {
                std::cerr
                    << "Type error: variable '"
                    << variableName
                    << "' cannot be declared as void.\n";

                ++errorCount;
                break;
            }

            declareVariable(variableName, declaredType);

            if (node->right)
            {
                Type actualType = evaluateNodeType(node->right);

                if (!areTypesCompatible(declaredType, actualType))
                {
                    std::cerr
                        << "Type error: cannot assign "
                        << typeToString(actualType)
                        << " to "
                        << declaredTypeStr
                        << " variable '"
                        << variableName
                        << "'.\n";

                    ++errorCount;
                }
            }

            break;
        }

        case NodeType::ASSIGN:
        {
            if (!node->left)
            {
                break;
            }

            std::string variableName = node->left->value;

            Type variableType = lookupVariable(variableName);

            if (variableType == Type::UNKNOWN)
            {
                std::cerr
                    << "Semantic error: variable '"
                    << variableName
                    << "' used before declaration!\n";

                ++errorCount;

                if (node->right)
                {
                    evaluateNodeType(node->right);
                }

                break;
            }

            Type actualType = evaluateNodeType(node->right);

            if (!areTypesCompatible(variableType, actualType))
            {
                std::cerr
                    << "Type error: cannot assign "
                    << typeToString(actualType)
                    << " to variable '"
                    << variableName
                    << "' of type "
                    << typeToString(variableType)
                    << ".\n";

                ++errorCount;
            }

            break;
        }

        case NodeType::IF:
        {
            Type conditionType = evaluateNodeType(node->left);

            if (!isBooleanCompatible(conditionType) &&
                conditionType != Type::UNKNOWN &&
                conditionType != Type::TYPE_ERROR)
            {
                std::cerr
                    << "Type error: condition of 'if' must be bool or int, but got "
                    << typeToString(conditionType)
                    << ".\n";

                ++errorCount;
            }

            analyzeNode(node->right);

            if (node->third)
            {
                analyzeNode(node->third);
            }

            break;
        }

        case NodeType::WHILE:
        {
            Type conditionType = evaluateNodeType(node->left);

            if (!isBooleanCompatible(conditionType) &&
                conditionType != Type::UNKNOWN &&
                conditionType != Type::TYPE_ERROR)
            {
                std::cerr
                    << "Type error: condition of 'while' must be bool or int, but got "
                    << typeToString(conditionType)
                    << ".\n";

                ++errorCount;
            }

            analyzeNode(node->right);

            break;
        }

        case NodeType::FOR:
        {
            enterScope();

            analyzeNode(node->left);

            if (node->right)
            {
                Type conditionType = evaluateNodeType(node->right);

                if (!isBooleanCompatible(conditionType) &&
                    conditionType != Type::UNKNOWN &&
                    conditionType != Type::TYPE_ERROR)
                {
                    std::cerr
                        << "Type error: condition of 'for' must be bool or int, but got "
                        << typeToString(conditionType)
                        << ".\n";

                    ++errorCount;
                }
            }

            analyzeNode(node->third);
            analyzeNode(node->fourth);

            exitScope();

            break;
        }

        case NodeType::RETURN:
        {
            if (!insideFunction)
            {
                if (node->left)
                {
                    evaluateNodeType(node->left);
                }
                break;
            }

            Type actualType = Type::VOID;

            if (node->left)
            {
                actualType = evaluateNodeType(node->left);
            }

            if (currentReturnType == Type::VOID)
            {
                if (actualType != Type::VOID)
                {
                    std::cerr
                        << "Type error: function '"
                        << currentFunctionName
                        << "' has void return type and cannot return a value.\n";

                    ++errorCount;
                }
            }
            else
            {
                if (actualType == Type::VOID)
                {
                    std::cerr
                        << "Type error: function '"
                        << currentFunctionName
                        << "' must return "
                        << currentReturnTypeStr
                        << ", but returned void.\n";

                    ++errorCount;
                }
                else if (!areTypesCompatible(currentReturnType, actualType))
                {
                    std::cerr
                        << "Type error: function '"
                        << currentFunctionName
                        << "' must return "
                        << currentReturnTypeStr
                        << ", but returned "
                        << typeToString(actualType)
                        << ".\n";

                    ++errorCount;
                }
            }

            break;
        }

        case NodeType::BLOCK:
        {
            enterScope();

            for (ASTNode* child : node->children)
            {
                analyzeNode(child);
            }

            exitScope();

            break;
        }

        default:
        {
            evaluateNodeType(node);
            break;
        }
    }
}

// ============================================================
// Evaluate expression type
// ============================================================

Type SemanticAnalyzer::evaluateNodeType(ASTNode* node)
{
    if (!node)
    {
        return Type::UNKNOWN;
    }

    switch (node->type)
    {
        case NodeType::NUMBER:
        {
            node->inferredType = "int";
            return Type::INT;
        }

        case NodeType::STRING_LITERAL:
        {
            node->inferredType = "string";
            return Type::STRING;
        }

        case NodeType::BOOL_LITERAL:
        {
            node->inferredType = "bool";
            return Type::BOOL;
        }

        case NodeType::IDENTIFIER:
        {
            Type type = lookupVariable(node->value);

            if (type == Type::UNKNOWN)
            {
                std::cerr
                    << "Semantic error: variable '"
                    << node->value
                    << "' used before declaration!\n";

                ++errorCount;

                node->inferredType = "unknown";
                return Type::UNKNOWN;
            }

            node->inferredType = typeToString(type);
            return type;
        }

        case NodeType::BINARY_OP:
        {
            Type leftType = evaluateNodeType(node->left);
            Type rightType = evaluateNodeType(node->right);

            if (leftType == Type::UNKNOWN || rightType == Type::UNKNOWN ||
                leftType == Type::TYPE_ERROR || rightType == Type::TYPE_ERROR)
            {
                return Type::TYPE_ERROR;
            }

            if (node->value == "+" || node->value == "-" ||
                node->value == "*" || node->value == "/" ||
                node->value == "%" || node->value == "<<" ||
                node->value == ">>" || node->value == "&" ||
                node->value == "|" || node->value == "^")
            {
                if (!isNumericType(leftType) || !isNumericType(rightType))
                {
                    std::cerr
                        << "Type error: operator '"
                        << node->value
                        << "' cannot be used with "
                        << typeToString(leftType)
                        << " and "
                        << typeToString(rightType)
                        << ".\n";

                    ++errorCount;
                    return Type::TYPE_ERROR;
                }

                node->inferredType = "int";
                return Type::INT;
            }

            std::cerr
                << "Type error: unknown binary operator '"
                << node->value
                << "'.\n";

            ++errorCount;
            return Type::TYPE_ERROR;
        }

        case NodeType::COMPARISON:
        {
            Type leftType = evaluateNodeType(node->left);
            Type rightType = evaluateNodeType(node->right);

            if (leftType == Type::UNKNOWN || rightType == Type::UNKNOWN ||
                leftType == Type::TYPE_ERROR || rightType == Type::TYPE_ERROR)
            {
                return Type::TYPE_ERROR;
            }

            if (node->value == "<" || node->value == "<=" ||
                node->value == ">" || node->value == ">=")
            {
                if (!isNumericType(leftType) || !isNumericType(rightType))
                {
                    std::cerr
                        << "Type error: comparison '"
                        << node->value
                        << "' requires numeric operands, but got "
                        << typeToString(leftType)
                        << " and "
                        << typeToString(rightType)
                        << ".\n";

                    ++errorCount;
                    return Type::TYPE_ERROR;
                }
            }
            else if (node->value == "==" || node->value == "!=")
            {
                bool valid = false;
                if (leftType == rightType)
                {
                    valid = true;
                }
                else if (isNumericType(leftType) && isNumericType(rightType))
                {
                    valid = true;
                }
                else if (isBooleanCompatible(leftType) && isBooleanCompatible(rightType))
                {
                    valid = true;
                }

                if (!valid)
                {
                    std::cerr
                        << "Type error: comparison '"
                        << node->value
                        << "' cannot compare "
                        << typeToString(leftType)
                        << " and "
                        << typeToString(rightType)
                        << ".\n";

                    ++errorCount;
                    return Type::TYPE_ERROR;
                }
            }

            node->inferredType = "bool";
            return Type::BOOL;
        }

        case NodeType::LOGICAL_AND:
        case NodeType::LOGICAL_OR:
        {
            Type leftType = evaluateNodeType(node->left);
            Type rightType = evaluateNodeType(node->right);

            if (leftType == Type::UNKNOWN || rightType == Type::UNKNOWN ||
                leftType == Type::TYPE_ERROR || rightType == Type::TYPE_ERROR)
            {
                return Type::TYPE_ERROR;
            }

            if (!isBooleanCompatible(leftType) || !isBooleanCompatible(rightType))
            {
                std::cerr
                    << "Type error: operands of '"
                    << node->value
                    << "' must be bool or int, but got "
                    << typeToString(leftType)
                    << " and "
                    << typeToString(rightType)
                    << ".\n";

                ++errorCount;
                return Type::TYPE_ERROR;
            }

            node->inferredType = "bool";
            return Type::BOOL;
        }

        case NodeType::LOGICAL_NOT:
        {
            Type operandType = evaluateNodeType(node->left);

            if (operandType == Type::UNKNOWN || operandType == Type::TYPE_ERROR)
            {
                return Type::TYPE_ERROR;
            }

            if (!isBooleanCompatible(operandType))
            {
                std::cerr
                    << "Type error: operand of '!' must be bool or int, but got "
                    << typeToString(operandType)
                    << ".\n";

                ++errorCount;
                return Type::TYPE_ERROR;
            }

            node->inferredType = "bool";
            return Type::BOOL;
        }

        case NodeType::CALL:
        {
            auto functionIt = functionTable.find(node->value);

            if (functionIt == functionTable.end())
            {
                std::cerr
                    << "Semantic error: function '"
                    << node->value
                    << "' is not declared.\n";

                ++errorCount;

                for (ASTNode* argument : node->children)
                {
                    evaluateNodeType(argument);
                }

                return Type::UNKNOWN;
            }

            const FunctionInfo& function = functionIt->second;

            if (node->children.size() != function.parameterTypes.size())
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

            size_t count = std::min(node->children.size(), function.parameterTypes.size());

            for (size_t i = 0; i < count; ++i)
            {
                Type actualType = evaluateNodeType(node->children[i]);
                Type expectedType = function.parameterTypes[i];

                if (!areTypesCompatible(expectedType, actualType))
                {
                    std::cerr
                        << "Type error: function '"
                        << node->value
                        << "' expects "
                        << function.parameterTypeStrings[i]
                        << " for argument "
                        << (i + 1)
                        << ", but got "
                        << typeToString(actualType)
                        << ".\n";

                    ++errorCount;
                }
            }

            for (size_t i = count; i < node->children.size(); ++i)
            {
                evaluateNodeType(node->children[i]);
            }

            node->inferredType = function.returnTypeStr;
            return function.returnType;
        }

        default:
            return Type::UNKNOWN;
    }
}

// ============================================================
// Public evaluateType returning std::string
// ============================================================

std::string SemanticAnalyzer::evaluateType(ASTNode* node)
{
    return typeToString(evaluateNodeType(node));
}