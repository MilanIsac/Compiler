#ifndef IR_H
#define IR_H

#include "ast.h"

#include <string>
#include <vector>

enum class IROpcode {
    ASSIGN,

    ADD,
    SUB,
    MUL,
    DIV,
    MOD,

    BIT_AND,
    BIT_OR,
    BIT_XOR,
    SHL,
    SHR,
    NEG,

    CMP_EQ,
    CMP_NE,
    CMP_LT,
    CMP_LE,
    CMP_GT,
    CMP_GE,

    LABEL,
    JUMP,

    JUMP_IF_FALSE,
    JUMP_IF_TRUE,

    // Direct conditional branches
    JUMP_IF_EQ,
    JUMP_IF_NE,
    JUMP_IF_LT,
    JUMP_IF_LE,
    JUMP_IF_GT,
    JUMP_IF_GE,

    // Functions
    FUNCTION_BEGIN,
    PARAM,
    FUNCTION_END,
    
    ARG,
    CALL,

    RETURN,
};

struct IRInstruction {
    IROpcode opcode;

    std::string result;
    std::string operand1;
    std::string operand2;

    std::string label;
};

// Generate IR from AST/program
std::vector<IRInstruction> generateIR(
    const std::vector<ASTNode*>& program
);

// Print IR
void printIR(
    const std::vector<IRInstruction>& ir
);

class IRGenerator {

private:
    int tempCounter;
    int labelCounter;

    std::vector<IRInstruction> instructions;

    std::string newTemp();
    std::string newLabel();

    std::string generateExpression(ASTNode* node);

    void generateCondition(
        ASTNode* node,
        const std::string& trueLabel,
        const std::string& falseLabel
    );

    void generate(ASTNode* node);

public:
    IRGenerator();

    std::vector<IRInstruction> generate(
        const std::vector<ASTNode*>& nodes
    );
};

#endif