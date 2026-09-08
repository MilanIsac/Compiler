#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>

class CodeGenerator {
private:
    // Stack slot for every variable / temporary.
    std::unordered_map<std::string, int> stackOffsets;

    // Current stack size in bytes.
    int stackSize;

    // Name of the function currently being generated.
    std::string currentFunction;

    // Exit label of the current function.
    std::string currentFunctionExit;

    // Whether we are currently inside a function.
    bool insideFunction;

    // Number of parameters encountered in the current function.
    int parameterIndex;

    bool isNumber(const std::string& value) const;

    void collectOperands(
        const std::vector<IRInstruction>& instructions
    );

    int getOffset(const std::string& name);

    void emitOperand(
        std::ostream& out,
        const std::string& operand,
        const std::string& targetRegister
    );

    void emitComparisonJump(
        std::ostream& out,
        IROpcode opcode,
        const std::string& operand1,
        const std::string& operand2,
        const std::string& label
    );

    void resetFunctionState();

    int calculateStackSize() const;

    std::string makeSafeFunctionExitLabel(
        const std::string& functionName
    ) const;

    void emitFunctionPrologue(
        std::ostream& out,
        const std::string& functionName
    );

    void emitFunctionEpilogue(
        std::ostream& out
    );

    void emitParameterMove(
        std::ostream& out,
        const std::string& parameterName,
        int parameterIndex
    );

public:
    CodeGenerator();

    bool generate(
        const std::vector<IRInstruction>& instructions,
        const std::string& filename
    );
};

#endif