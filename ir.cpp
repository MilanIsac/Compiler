#include "ir.h"

#include <iostream>

// ============================================================
// Constructor
// ============================================================

IRGenerator::IRGenerator()
    : tempCounter(0),
      labelCounter(0)
{
}

// ============================================================
// Temporary
// ============================================================

std::string IRGenerator::newTemp()
{
    return "t" + std::to_string(++tempCounter);
}

// ============================================================
// Label
// ============================================================

std::string IRGenerator::newLabel()
{
    return "L" + std::to_string(++labelCounter);
}

// ============================================================
// Generate expression
// ============================================================

std::string IRGenerator::generateExpression(ASTNode* node)
{
    if (!node)
    {
        return "";
    }

    // Simple values
    if (node->type == NodeType::NUMBER ||
        node->type == NodeType::IDENTIFIER ||
        node->type == NodeType::STRING_LITERAL)
    {
        return node->value;
    }

    // --------------------------------------------------------
    // Binary operation
    // --------------------------------------------------------

    if (node->type == NodeType::BINARY_OP)
    {
        std::string left = generateExpression(node->left);
        std::string right = generateExpression(node->right);

        std::string temp = newTemp();

        IROpcode opcode;

        if (node->value == "+")
            opcode = IROpcode::ADD;
        else if (node->value == "-")
            opcode = IROpcode::SUB;
        else if (node->value == "*")
            opcode = IROpcode::MUL;
        else if (node->value == "/")
            opcode = IROpcode::DIV;
        else if (node->value == "%")
            opcode = IROpcode::MOD;
        else if (node->value == "&")
            opcode = IROpcode::BIT_AND;
        else if (node->value == "|")
            opcode = IROpcode::BIT_OR;
        else if (node->value == "^")
            opcode = IROpcode::BIT_XOR;
        else if (node->value == "<<")
            opcode = IROpcode::SHL;
        else if (node->value == ">>")
            opcode = IROpcode::SHR;
        else
            opcode = IROpcode::ADD;

        instructions.push_back({
            opcode,
            temp,
            left,
            right,
            ""
        });

        return temp;
    }

    // --------------------------------------------------------
    // Comparison / logical expression
    // --------------------------------------------------------

    if (node->type == NodeType::COMPARISON ||
        node->type == NodeType::LOGICAL_AND ||
        node->type == NodeType::LOGICAL_OR ||
        node->type == NodeType::LOGICAL_NOT)
    {
        std::string result = newTemp();

        std::string trueLabel = newLabel();
        std::string falseLabel = newLabel();
        std::string endLabel = newLabel();

        generateCondition(
            node,
            trueLabel,
            falseLabel
        );

        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            trueLabel
        });

        instructions.push_back({
            IROpcode::ASSIGN,
            result,
            "1",
            "",
            ""
        });

        instructions.push_back({
            IROpcode::JUMP,
            "",
            "",
            "",
            endLabel
        });

        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            falseLabel
        });

        instructions.push_back({
            IROpcode::ASSIGN,
            result,
            "0",
            "",
            ""
        });

        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            endLabel
        });

        return result;
    }

    // ============================================================
    // FUNCTION CALL
    // ============================================================

    if (node->type == NodeType::CALL)
    {
        for (ASTNode* argument : node->children)
        {
            std::string value = generateExpression(argument);

            instructions.push_back({
                IROpcode::ARG,
                "",
                value,
                "",
                ""
            });
        }

        std::string result = newTemp();

        instructions.push_back({
            IROpcode::CALL,
            result,
            std::to_string(node->children.size()),
            "",
            node->value
        });

        return result;
    }

    return "";
}

// ============================================================
// Generate condition
// ============================================================

void IRGenerator::generateCondition(
    ASTNode* node,
    const std::string& trueLabel,
    const std::string& falseLabel
)
{
    if (!node)
    {
        instructions.push_back({
            IROpcode::JUMP,
            "",
            "",
            "",
            falseLabel
        });

        return;
    }

    // --------------------------------------------------------
    // Comparison
    // --------------------------------------------------------

    if (node->type == NodeType::COMPARISON)
    {
        std::string left =
            generateExpression(node->left);

        std::string right =
            generateExpression(node->right);

        IROpcode opcode;

        if (node->value == "==")
            opcode = IROpcode::JUMP_IF_EQ;
        else if (node->value == "!=")
            opcode = IROpcode::JUMP_IF_NE;
        else if (node->value == "<")
            opcode = IROpcode::JUMP_IF_LT;
        else if (node->value == "<=")
            opcode = IROpcode::JUMP_IF_LE;
        else if (node->value == ">")
            opcode = IROpcode::JUMP_IF_GT;
        else
            opcode = IROpcode::JUMP_IF_GE;

        instructions.push_back({
            opcode,
            "",
            left,
            right,
            trueLabel
        });

        instructions.push_back({
            IROpcode::JUMP,
            "",
            "",
            "",
            falseLabel
        });

        return;
    }

    // --------------------------------------------------------
    // Logical AND
    // --------------------------------------------------------

    if (node->type == NodeType::LOGICAL_AND)
    {
        std::string rightLabel = newLabel();

        generateCondition(
            node->left,
            rightLabel,
            falseLabel
        );

        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            rightLabel
        });

        generateCondition(
            node->right,
            trueLabel,
            falseLabel
        );

        return;
    }

    // --------------------------------------------------------
    // Logical OR
    // --------------------------------------------------------

    if (node->type == NodeType::LOGICAL_OR)
    {
        std::string rightLabel = newLabel();

        generateCondition(
            node->left,
            trueLabel,
            rightLabel
        );

        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            rightLabel
        });

        generateCondition(
            node->right,
            trueLabel,
            falseLabel
        );

        return;
    }

    // --------------------------------------------------------
    // Logical NOT
    // --------------------------------------------------------

    if (node->type == NodeType::LOGICAL_NOT)
    {
        generateCondition(
            node->left,
            falseLabel,
            trueLabel
        );

        return;
    }

    // --------------------------------------------------------
    // Normal expression as truth value
    // --------------------------------------------------------

    std::string value =
        generateExpression(node);

    instructions.push_back({
        IROpcode::JUMP_IF_FALSE,
        "",
        value,
        "",
        falseLabel
    });

    instructions.push_back({
        IROpcode::JUMP,
        "",
        "",
        "",
        trueLabel
    });
}

// ============================================================
// Generate one AST node
// ============================================================

void IRGenerator::generate(ASTNode* node)
{
    if (!node)
    {
        return;
    }

    // ========================================================
    // FUNCTION
    // ========================================================

    if (node->type == NodeType::FUNCTION)
    {
        /*
            AST representation:

            FUNCTION
              value       = function name
              fourth      = return type
              children    = parameters
              left        = body BLOCK

            Parameter representation:

              parameter->value       = parameter name
              parameter->fourth      = parameter type
        */

        std::string functionName = node->value;

        std::string returnType = "void";

        if (node->fourth)
        {
            returnType = node->fourth->value;
        }

        // ----------------------------------------------------
        // Function begin
        // ----------------------------------------------------

        instructions.push_back({
            IROpcode::FUNCTION_BEGIN,
            functionName,
            returnType,
            "",
            ""
        });

        // ----------------------------------------------------
        // Parameters
        // ----------------------------------------------------

        for (ASTNode* parameter : node->children)
        {
            if (!parameter)
            {
                continue;
            }

            std::string parameterName =
                parameter->value;

            std::string parameterType = "int";

            if (parameter->fourth)
            {
                parameterType =
                    parameter->fourth->value;
            }

            instructions.push_back({
                IROpcode::PARAM,
                parameterName,
                parameterType,
                "",
                ""
            });
        }

        // ----------------------------------------------------
        // Function body
        // ----------------------------------------------------

        if (node->left)
        {
            generate(node->left);
        }

        // ----------------------------------------------------
        // Function end
        // ----------------------------------------------------

        instructions.push_back({
            IROpcode::FUNCTION_END,
            functionName,
            "",
            "",
            ""
        });

        return;
    }

    // ========================================================
    // Assignment / Variable declaration
    // ========================================================

    if (node->type == NodeType::ASSIGN ||
        node->type == NodeType::VAR_DECL)
    {
        if (!node->left)
        {
            return;
        }

        std::string value =
            generateExpression(node->right);

        instructions.push_back({
            IROpcode::ASSIGN,
            node->left->value,
            value,
            "",
            ""
        });

        return;
    }

    // ========================================================
    // Return
    // ========================================================

    if (node->type == NodeType::RETURN)
    {
        std::string value =
            node->left
                ? generateExpression(node->left)
                : "0";

        instructions.push_back({
            IROpcode::RETURN,
            "",
            value,
            "",
            ""
        });

        return;
    }

    // ========================================================
    // Block
    // ========================================================

    if (node->type == NodeType::BLOCK)
    {
        for (ASTNode* child : node->children)
        {
            generate(child);
        }

        return;
    }

    // ========================================================
    // IF
    // ========================================================

    if (node->type == NodeType::IF)
    {
        std::string thenLabel = newLabel();
        std::string endLabel = newLabel();

        if (node->third)
        {
            std::string elseLabel = newLabel();

            generateCondition(
                node->left,
                thenLabel,
                elseLabel
            );

            instructions.push_back({
                IROpcode::LABEL,
                "",
                "",
                "",
                thenLabel
            });

            generate(node->right);

            instructions.push_back({
                IROpcode::JUMP,
                "",
                "",
                "",
                endLabel
            });

            instructions.push_back({
                IROpcode::LABEL,
                "",
                "",
                "",
                elseLabel
            });

            generate(node->third);

            instructions.push_back({
                IROpcode::LABEL,
                "",
                "",
                "",
                endLabel
            });
        }
        else
        {
            generateCondition(
                node->left,
                thenLabel,
                endLabel
            );

            instructions.push_back({
                IROpcode::LABEL,
                "",
                "",
                "",
                thenLabel
            });

            generate(node->right);

            instructions.push_back({
                IROpcode::LABEL,
                "",
                "",
                "",
                endLabel
            });
        }

        return;
    }

    // ========================================================
    // WHILE
    // ========================================================

    if (node->type == NodeType::WHILE)
    {
        std::string startLabel = newLabel();
        std::string bodyLabel = newLabel();
        std::string endLabel = newLabel();

        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            startLabel
        });

        generateCondition(
            node->left,
            bodyLabel,
            endLabel
        );

        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            bodyLabel
        });

        generate(node->right);

        instructions.push_back({
            IROpcode::JUMP,
            "",
            "",
            "",
            startLabel
        });

        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            endLabel
        });

        return;
    }

    // ========================================================
    // FOR
    // ========================================================

    if (node->type == NodeType::FOR)
    {
        std::string startLabel = newLabel();
        std::string bodyLabel = newLabel();
        std::string endLabel = newLabel();

        // Initialization
        if (node->left)
        {
            generate(node->left);
        }

        // Start
        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            startLabel
        });

        // Condition
        if (node->right)
        {
            generateCondition(
                node->right,
                bodyLabel,
                endLabel
            );
        }

        // Body
        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            bodyLabel
        });

        if (node->fourth)
        {
            generate(node->fourth);
        }

        // Update
        if (node->third)
        {
            generate(node->third);
        }

        // Back to start
        instructions.push_back({
            IROpcode::JUMP,
            "",
            "",
            "",
            startLabel
        });

        // End
        instructions.push_back({
            IROpcode::LABEL,
            "",
            "",
            "",
            endLabel
        });

        return;
    }

    // ========================================================
    // Expression statement
    // ========================================================

    generateExpression(node);
}

// ============================================================
// Public generate
// ============================================================

std::vector<IRInstruction> IRGenerator::generate(
    const std::vector<ASTNode*>& nodes
)
{
    instructions.clear();

    tempCounter = 0;
    labelCounter = 0;

    for (ASTNode* node : nodes)
    {
        generate(node);
    }

    return instructions;
}

// ============================================================
// Compatibility function
// ============================================================

std::vector<IRInstruction> generateIR(
    const std::vector<ASTNode*>& program
)
{
    IRGenerator generator;

    return generator.generate(program);
}

// ============================================================
// Print IR
// ============================================================

void printIR(
    const std::vector<IRInstruction>& ir
)
{
    std::cout << "\n=== IR ===\n";

    for (const auto& inst : ir)
    {
        switch (inst.opcode)
        {
            // ------------------------------------------------
            // Assignment
            // ------------------------------------------------

            case IROpcode::ASSIGN:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1;
                break;

            // ------------------------------------------------
            // Arithmetic
            // ------------------------------------------------

            case IROpcode::ADD:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " + "
                    << inst.operand2;
                break;

            case IROpcode::SUB:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " - "
                    << inst.operand2;
                break;

            case IROpcode::MUL:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " * "
                    << inst.operand2;
                break;

            case IROpcode::DIV:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " / "
                    << inst.operand2;
                break;

            case IROpcode::MOD:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " % "
                    << inst.operand2;
                break;

            // ------------------------------------------------
            // Bitwise
            // ------------------------------------------------

            case IROpcode::BIT_AND:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " & "
                    << inst.operand2;
                break;

            case IROpcode::BIT_OR:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " | "
                    << inst.operand2;
                break;

            case IROpcode::BIT_XOR:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " ^ "
                    << inst.operand2;
                break;

            case IROpcode::SHL:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " << "
                    << inst.operand2;
                break;

            case IROpcode::SHR:
                std::cout
                    << inst.result
                    << " = "
                    << inst.operand1
                    << " >> "
                    << inst.operand2;
                break;

            case IROpcode::NEG:
                std::cout
                    << inst.result
                    << " = -"
                    << inst.operand1;
                break;

            // ------------------------------------------------
            // Labels / jumps
            // ------------------------------------------------

            case IROpcode::LABEL:
                std::cout
                    << inst.label
                    << ":";
                break;

            case IROpcode::JUMP:
                std::cout
                    << "GOTO "
                    << inst.label;
                break;

            case IROpcode::JUMP_IF_FALSE:
                std::cout
                    << "IF_FALSE "
                    << inst.operand1
                    << " GOTO "
                    << inst.label;
                break;

            case IROpcode::JUMP_IF_TRUE:
                std::cout
                    << "IF_TRUE "
                    << inst.operand1
                    << " GOTO "
                    << inst.label;
                break;

            // ------------------------------------------------
            // Direct conditional branches
            // ------------------------------------------------

            case IROpcode::JUMP_IF_EQ:
                std::cout
                    << "IF "
                    << inst.operand1
                    << " == "
                    << inst.operand2
                    << " GOTO "
                    << inst.label;
                break;

            case IROpcode::JUMP_IF_NE:
                std::cout
                    << "IF "
                    << inst.operand1
                    << " != "
                    << inst.operand2
                    << " GOTO "
                    << inst.label;
                break;

            case IROpcode::JUMP_IF_LT:
                std::cout
                    << "IF "
                    << inst.operand1
                    << " < "
                    << inst.operand2
                    << " GOTO "
                    << inst.label;
                break;

            case IROpcode::JUMP_IF_LE:
                std::cout
                    << "IF "
                    << inst.operand1
                    << " <= "
                    << inst.operand2
                    << " GOTO "
                    << inst.label;
                break;

            case IROpcode::JUMP_IF_GT:
                std::cout
                    << "IF "
                    << inst.operand1
                    << " > "
                    << inst.operand2
                    << " GOTO "
                    << inst.label;
                break;

            case IROpcode::JUMP_IF_GE:
                std::cout
                    << "IF "
                    << inst.operand1
                    << " >= "
                    << inst.operand2
                    << " GOTO "
                    << inst.label;
                break;

            // ------------------------------------------------
            // Functions
            // ------------------------------------------------

            case IROpcode::FUNCTION_BEGIN:
                std::cout
                    << "FUNCTION_BEGIN "
                    << inst.result
                    << " : "
                    << inst.operand1;
                break;

            case IROpcode::PARAM:
                std::cout
                    << "PARAM "
                    << inst.result
                    << " : "
                    << inst.operand1;
                break;

            case IROpcode::FUNCTION_END:
                std::cout
                    << "FUNCTION_END "
                    << inst.result;
                break;

            // ------------------------------------------------
            // Function calls
            // ------------------------------------------------

            case IROpcode::ARG:
                std::cout << "ARG " << inst.operand1;
                break;

            case IROpcode::CALL:
                std::cout
                    << inst.result
                    << " = CALL "
                    << inst.label
                    << ", "
                    << inst.operand1;
                break;

            // ------------------------------------------------
            // Return
            // ------------------------------------------------

            case IROpcode::RETURN:
                std::cout
                    << "RETURN "
                    << inst.operand1;
                break;

            // ------------------------------------------------
            // Unknown
            // ------------------------------------------------

            default:
                std::cout
                    << "<unknown IR instruction>";
                break;
        }

        std::cout << '\n';
    }
}