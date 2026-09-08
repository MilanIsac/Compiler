#include "codegen.h"

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <algorithm>

using namespace std;


// ============================================================
// Constructor
// ============================================================

CodeGenerator::CodeGenerator()
    : stackSize(0),
      currentFunction(""),
      currentFunctionExit(""),
      insideFunction(false),
      parameterIndex(0)
{
}


// ============================================================
// Check whether a string is an integer number
// ============================================================

bool CodeGenerator::isNumber(const string& value) const
{
    if (value.empty()) {
        return false;
    }

    size_t start = 0;

    if (value[0] == '-' || value[0] == '+') {
        if (value.size() == 1) {
            return false;
        }

        start = 1;
    }

    for (size_t i = start; i < value.size(); ++i) {
        if (!isdigit(static_cast<unsigned char>(value[i]))) {
            return false;
        }
    }

    return true;
}


// ============================================================
// Reset state for a new function
// ============================================================

void CodeGenerator::resetFunctionState()
{
    stackOffsets.clear();
    stackSize = 0;

    currentFunction.clear();
    currentFunctionExit.clear();

    parameterIndex = 0;
}


// ============================================================
// Calculate stack size
//
// Each variable/temp occupies 8 bytes.
//
// We align the final stack size to 16 bytes because the
// System V x86-64 ABI requires proper stack alignment.
// ============================================================

int CodeGenerator::calculateStackSize() const
{
    if (stackOffsets.empty()) {
        return 0;
    }

    int bytes =
        static_cast<int>(stackOffsets.size()) * 8;

    // Round up to 16-byte alignment.
    bytes = (bytes + 15) & ~15;

    return bytes;
}


// ============================================================
// Collect variables and temporaries used by one function
// ============================================================

void CodeGenerator::collectOperands(
    const vector<IRInstruction>& instructions
)
{
    stackOffsets.clear();

    int nextOffset = -4;

    auto addOperand = [&](const string& operand)
    {
        if (operand.empty()) {
            return;
        }

        if (isNumber(operand)) {
            return;
        }

        // Labels are not variables.
        if (!operand.empty() &&
            (operand[0] == '.' ||
             operand.find("label") == 0)) {
            return;
        }

        if (stackOffsets.find(operand) ==
            stackOffsets.end())
        {
            stackOffsets[operand] = nextOffset;

            // Move by 8 bytes.
            nextOffset -= 8;
        }
    };

    for (const auto& inst : instructions)
    {
        switch (inst.opcode)
        {
            case IROpcode::ASSIGN:
                addOperand(inst.result);
                addOperand(inst.operand1);
                break;

            case IROpcode::ADD:
            case IROpcode::SUB:
            case IROpcode::MUL:
            case IROpcode::DIV:
            case IROpcode::MOD:
            case IROpcode::BIT_AND:
            case IROpcode::BIT_OR:
            case IROpcode::BIT_XOR:
            case IROpcode::SHL:
            case IROpcode::SHR:
            case IROpcode::CMP_EQ:
            case IROpcode::CMP_NE:
            case IROpcode::CMP_LT:
            case IROpcode::CMP_LE:
            case IROpcode::CMP_GT:
            case IROpcode::CMP_GE:

                addOperand(inst.result);
                addOperand(inst.operand1);
                addOperand(inst.operand2);
                break;

            case IROpcode::NEG:

                addOperand(inst.result);
                addOperand(inst.operand1);
                break;

            case IROpcode::JUMP_IF_FALSE:
            case IROpcode::JUMP_IF_TRUE:

                addOperand(inst.operand1);
                break;

            case IROpcode::JUMP_IF_EQ:
            case IROpcode::JUMP_IF_NE:
            case IROpcode::JUMP_IF_LT:
            case IROpcode::JUMP_IF_LE:
            case IROpcode::JUMP_IF_GT:
            case IROpcode::JUMP_IF_GE:

                addOperand(inst.operand1);
                addOperand(inst.operand2);
                break;

            case IROpcode::RETURN:

                addOperand(inst.operand1);
                break;

            case IROpcode::PARAM:

                addOperand(inst.result);
                break;

            case IROpcode::ARG:
                addOperand(inst.operand1);
                break;

            case IROpcode::CALL:
                addOperand(inst.result);
                break;

            case IROpcode::LABEL:
            case IROpcode::JUMP:
            case IROpcode::FUNCTION_BEGIN:
            case IROpcode::FUNCTION_END:

                break;
        }
    }

    stackSize = calculateStackSize();
}


// ============================================================
// Get stack offset
// ============================================================

int CodeGenerator::getOffset(const string& name)
{
    auto it = stackOffsets.find(name);

    if (it != stackOffsets.end()) {
        return it->second;
    }

    // Allocate a new slot if necessary.
    int offset = -4 -
        static_cast<int>(stackOffsets.size()) * 8;

    stackOffsets[name] = offset;

    stackSize = calculateStackSize();

    return offset;
}


// ============================================================
// Emit an operand into a register
// ============================================================

void CodeGenerator::emitOperand(
    ostream& out,
    const string& operand,
    const string& targetRegister
)
{
    if (operand.empty()) {
        out << "    xor " << targetRegister
            << ", " << targetRegister << "\n";

        return;
    }

    // Immediate integer.
    if (isNumber(operand))
    {
        out << "    mov "
            << targetRegister
            << ", "
            << operand
            << "\n";

        return;
    }

    // Variable / temporary.
    int offset = getOffset(operand);

    out << "    mov "
        << targetRegister
        << ", DWORD PTR [rbp"
        << offset
        << "]\n";
}


// ============================================================
// Emit comparison branch
// ============================================================

void CodeGenerator::emitComparisonJump(
    ostream& out,
    IROpcode opcode,
    const string& operand1,
    const string& operand2,
    const string& label
)
{
    emitOperand(out, operand1, "eax");
    emitOperand(out, operand2, "ecx");

    out << "    cmp eax, ecx\n";

    switch (opcode)
    {
        case IROpcode::JUMP_IF_EQ:
            out << "    je " << label << "\n";
            break;

        case IROpcode::JUMP_IF_NE:
            out << "    jne " << label << "\n";
            break;

        case IROpcode::JUMP_IF_LT:
            out << "    jl " << label << "\n";
            break;

        case IROpcode::JUMP_IF_LE:
            out << "    jle " << label << "\n";
            break;

        case IROpcode::JUMP_IF_GT:
            out << "    jg " << label << "\n";
            break;

        case IROpcode::JUMP_IF_GE:
            out << "    jge " << label << "\n";
            break;

        default:
            break;
    }
}

void CodeGenerator::emitCallArgument(
    std::ostream& out,
    const std::string& operand,
    int index
)
{
    static const char* argumentRegisters[] =
    {
        "edi",
        "esi",
        "edx",
        "ecx",
        "r8d",
        "r9d"
    };

    if (index < 0 || index >= 6)
    {
        std::cerr
            << "Code generation error: "
            << "more than 6 integer function arguments are not supported yet.\n";

        return;
    }

    const char* reg = argumentRegisters[index];

    if (isNumber(operand))
    {
        out << "    mov "
            << reg
            << ", "
            << operand
            << "\n";
    }
    else
    {
        int offset = getOffset(operand);
        out << "    mov "
            << reg
            << ", DWORD PTR [rbp"
            << (offset < 0 ? "" : "-")
            << offset
            << "]\n";
    }
}

// ============================================================
// Create unique function exit label
// ============================================================

string CodeGenerator::makeSafeFunctionExitLabel(
    const string& functionName
) const
{
    string result = functionName;

    for (char& c : result)
    {
        if (!isalnum(static_cast<unsigned char>(c)) &&
            c != '_')
        {
            c = '_';
        }
    }

    return ".L_" + result + "_exit";
}


// ============================================================
// Emit function prologue
// ============================================================

void CodeGenerator::emitFunctionPrologue(
    ostream& out,
    const string& functionName
)
{
    out << ".globl " << functionName << "\n";
    out << functionName << ":\n";

    out << "    push rbp\n";
    out << "    mov rbp, rsp\n";

    if (stackSize > 0)
    {
        out << "    sub rsp, "
            << stackSize
            << "\n";
    }

    // Default return value = 0.
    out << "    xor eax, eax\n";

    out << "\n";
}


// ============================================================
// Emit function epilogue
// ============================================================

void CodeGenerator::emitFunctionEpilogue(
    ostream& out
)
{
    out << currentFunctionExit << ":\n";

    out << "    mov rsp, rbp\n";
    out << "    pop rbp\n";
    out << "    ret\n";

    out << "\n";
}


// ============================================================
// Move function parameters from ABI registers into stack slots
//
// System V AMD64 integer argument registers:
//
// arg1 -> edi
// arg2 -> esi
// arg3 -> edx
// arg4 -> ecx
// arg5 -> r8d
// arg6 -> r9d
//
// ============================================================

void CodeGenerator::emitParameterMove(
    ostream& out,
    const string& parameterName,
    int index
)
{
    static const vector<string> parameterRegisters =
    {
        "edi",
        "esi",
        "edx",
        "ecx",
        "r8d",
        "r9d"
    };

    if (index < 0 ||
        index >= static_cast<int>(
            parameterRegisters.size()))
    {
        cerr
            << "Error: more than 6 integer parameters "
            << "are currently unsupported.\n";

        return;
    }

    int offset = getOffset(parameterName);

    out << "    mov DWORD PTR [rbp"
        << offset
        << "], "
        << parameterRegisters[index]
        << "\n";
}


// ============================================================
// Main code generation function
// ============================================================

bool CodeGenerator::generate(
    const vector<IRInstruction>& instructions,
    const string& filename
)
{
    ofstream out(filename);

    if (!out)
    {
        cerr
            << "Error: cannot open output file: "
            << filename
            << "\n";

        return false;
    }


    // ========================================================
    // Assembly header
    // ========================================================

    out << ".intel_syntax noprefix\n";
    out << ".text\n\n";


    // ========================================================
    // First pass:
    //
    // Split IR into top-level instructions and functions.
    //
    // This lets us calculate stack slots separately for every
    // function.
    // ========================================================

    vector<vector<IRInstruction>> functions;

    vector<IRInstruction> currentFunctionIR;

    bool collectingFunction = false;

    string functionName;


    // We keep top-level instructions here.
    vector<IRInstruction> mainIR;


    for (const auto& inst : instructions)
    {
        if (inst.opcode == IROpcode::FUNCTION_BEGIN)
        {
            // Start a new function.
            collectingFunction = true;

            currentFunctionIR.clear();

            functionName = inst.result;

            // Keep FUNCTION_BEGIN in the vector.
            currentFunctionIR.push_back(inst);

            continue;
        }


        if (inst.opcode == IROpcode::FUNCTION_END)
        {
            if (collectingFunction)
            {
                currentFunctionIR.push_back(inst);

                functions.push_back(
                    currentFunctionIR
                );

                currentFunctionIR.clear();

                collectingFunction = false;
                functionName.clear();
            }

            continue;
        }


        if (collectingFunction)
        {
            currentFunctionIR.push_back(inst);
        }
        else
        {
            mainIR.push_back(inst);
        }
    }


    // ========================================================
    // Generate main
    // ========================================================

    if (!mainIR.empty())
    {
        resetFunctionState();

        currentFunction = "main";
        currentFunctionExit = ".L_main_exit";
        insideFunction = true;

        collectOperands(mainIR);

        out << ".globl main\n";
        out << "main:\n";

        out << "    push rbp\n";
        out << "    mov rbp, rsp\n";

        if (stackSize > 0)
        {
            out << "    sub rsp, "
                << stackSize
                << "\n";
        }

        // Default return value.
        out << "    xor eax, eax\n";

        out << "\n";


        // ====================================================
        // Generate instructions inside main
        // ====================================================

        int argumentIndex = 0;

        for (const auto& inst : mainIR)
        {
            switch (inst.opcode)
            {
                // ------------------------------------------------
                // Assignment
                // ------------------------------------------------

                case IROpcode::ASSIGN:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // ADD
                // ------------------------------------------------

                case IROpcode::ADD:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    add eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // SUB
                // ------------------------------------------------

                case IROpcode::SUB:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    sub eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // MUL
                // ------------------------------------------------

                case IROpcode::MUL:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    imul eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // DIV
                // ------------------------------------------------

                case IROpcode::DIV:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    cdq\n";
                    out << "    idiv ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // MOD
                // ------------------------------------------------

                case IROpcode::MOD:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    cdq\n";
                    out << "    idiv ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], edx\n";

                    break;
                }


                // ------------------------------------------------
                // BIT AND
                // ------------------------------------------------

                case IROpcode::BIT_AND:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    and eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // BIT OR
                // ------------------------------------------------

                case IROpcode::BIT_OR:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    or eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // BIT XOR
                // ------------------------------------------------

                case IROpcode::BIT_XOR:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    xor eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // SHIFT LEFT
                // ------------------------------------------------

                case IROpcode::SHL:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    mov ecx, ecx\n";
                    out << "    shl eax, cl\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // SHIFT RIGHT
                // ------------------------------------------------

                case IROpcode::SHR:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    sar eax, cl\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // NEG
                // ------------------------------------------------

                case IROpcode::NEG:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    out << "    neg eax\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // Comparison producing a value
                // ------------------------------------------------

                case IROpcode::CMP_EQ:
                case IROpcode::CMP_NE:
                case IROpcode::CMP_LT:
                case IROpcode::CMP_LE:
                case IROpcode::CMP_GT:
                case IROpcode::CMP_GE:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    cmp eax, ecx\n";

                    string trueLabel =
                        ".L_cmp_true";

                    string falseLabel =
                        ".L_cmp_false";

                    string endLabel =
                        ".L_cmp_end";

                    // Use unique-ish labels based on result.
                    trueLabel += "_" + inst.result;
                    falseLabel += "_" + inst.result;
                    endLabel += "_" + inst.result;

                    for (char& c : trueLabel)
                    {
                        if (!isalnum(
                                static_cast<unsigned char>(c)) &&
                            c != '_')
                        {
                            c = '_';
                        }
                    }

                    for (char& c : falseLabel)
                    {
                        if (!isalnum(
                                static_cast<unsigned char>(c)) &&
                            c != '_')
                        {
                            c = '_';
                        }
                    }

                    for (char& c : endLabel)
                    {
                        if (!isalnum(
                                static_cast<unsigned char>(c)) &&
                            c != '_')
                        {
                            c = '_';
                        }
                    }

                    switch (inst.opcode)
                    {
                        case IROpcode::CMP_EQ:
                            out << "    je "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_NE:
                            out << "    jne "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_LT:
                            out << "    jl "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_LE:
                            out << "    jle "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_GT:
                            out << "    jg "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_GE:
                            out << "    jge "
                                << trueLabel
                                << "\n";
                            break;

                        default:
                            break;
                    }

                    out << "    mov eax, 0\n";
                    out << "    jmp "
                        << endLabel
                        << "\n";

                    out << trueLabel << ":\n";
                    out << "    mov eax, 1\n";

                    out << endLabel << ":\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // ------------------------------------------------
                // LABEL
                // ------------------------------------------------

                case IROpcode::LABEL:
                {
                    out << inst.label << ":\n";
                    break;
                }


                // ------------------------------------------------
                // JUMP
                // ------------------------------------------------

                case IROpcode::JUMP:
                {
                    out
                        << "    jmp "
                        << inst.label
                        << "\n";

                    break;
                }


                // ------------------------------------------------
                // JUMP_IF_FALSE
                // ------------------------------------------------

                case IROpcode::JUMP_IF_FALSE:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    out << "    cmp eax, 0\n";

                    out
                        << "    je "
                        << inst.label
                        << "\n";

                    break;
                }


                // ------------------------------------------------
                // JUMP_IF_TRUE
                // ------------------------------------------------

                case IROpcode::JUMP_IF_TRUE:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    out << "    cmp eax, 0\n";

                    out
                        << "    jne "
                        << inst.label
                        << "\n";

                    break;
                }


                // ------------------------------------------------
                // Direct conditional branches
                // ------------------------------------------------

                case IROpcode::JUMP_IF_EQ:
                case IROpcode::JUMP_IF_NE:
                case IROpcode::JUMP_IF_LT:
                case IROpcode::JUMP_IF_LE:
                case IROpcode::JUMP_IF_GT:
                case IROpcode::JUMP_IF_GE:
                {
                    emitComparisonJump(
                        out,
                        inst.opcode,
                        inst.operand1,
                        inst.operand2,
                        inst.label
                    );

                    break;
                }


                // ------------------------------------------------
                // RETURN
                // ------------------------------------------------

                case IROpcode::RETURN:
                {
                    if (!inst.operand1.empty())
                    {
                        emitOperand(
                            out,
                            inst.operand1,
                            "eax"
                        );
                    }
                    else
                    {
                        out << "    xor eax, eax\n";
                    }

                    out
                        << "    jmp "
                        << currentFunctionExit
                        << "\n";

                    break;
                }


                // ------------------------------------------------
                // Function opcodes should not appear in main.
                // ------------------------------------------------

                case IROpcode::FUNCTION_BEGIN:
                case IROpcode::PARAM:
                case IROpcode::FUNCTION_END:
                    break;

                // ====================================================
                // FUNCTION ARGUMENT
                // ====================================================

                case IROpcode::ARG:
                {
                    emitCallArgument(
                        out,
                        inst.operand1,
                        argumentIndex
                    );

                    ++argumentIndex;
                    break;
                }

                // ====================================================
                // FUNCTION CALL
                // ====================================================

                case IROpcode::CALL:
                {
                    if (argumentIndex > 6)
                    {
                        cerr
                            << "Code generation error: "
                            << "function calls with more than 6 arguments "
                            << "are not supported yet.\n";

                        out.close();
                        return false;
                    }

                    out << "    call "
                        << inst.label
                        << "\n";

                    // Integer return value is in EAX.
                    int offset = getOffset(inst.result);
                    out << "    mov DWORD PTR [rbp"
                        << (offset < 0 ? "" : "-")
                        << offset
                        << "], eax\n";

                    // Ready for the next call.
                    argumentIndex = 0;

                    break;
                }
            }
        }


        // ========================================================
        // main epilogue
        // ========================================================

        out << "\n";
        out << currentFunctionExit << ":\n";

        out << "    mov rsp, rbp\n";
        out << "    pop rbp\n";
        out << "    ret\n";

        out << "\n";

        insideFunction = false;
    }


    // ========================================================
    // Generate user-defined functions
    // ========================================================

    for (const auto& functionIR : functions)
    {
        resetFunctionState();

        if (functionIR.empty()) {
            continue;
        }


        // --------------------------------------------------------
        // Find FUNCTION_BEGIN
        // --------------------------------------------------------

        string name;
        string returnType;

        for (const auto& inst : functionIR)
        {
            if (inst.opcode ==
                IROpcode::FUNCTION_BEGIN)
            {
                name = inst.result;
                returnType = inst.operand1;
                break;
            }
        }

        if (name.empty())
        {
            continue;
        }


        currentFunction = name;
        currentFunctionExit =
            makeSafeFunctionExitLabel(name);

        insideFunction = true;


        // --------------------------------------------------------
        // Collect all variables/temporaries.
        // --------------------------------------------------------

        collectOperands(functionIR);


        // --------------------------------------------------------
        // Function header
        // --------------------------------------------------------

        out << ".globl "
            << name
            << "\n";

        out << name
            << ":\n";

        out << "    push rbp\n";
        out << "    mov rbp, rsp\n";

        if (stackSize > 0)
        {
            out << "    sub rsp, "
                << stackSize
                << "\n";
        }

        // Default return value.
        out << "    xor eax, eax\n";

        out << "\n";


        // --------------------------------------------------------
        // Function body
        // --------------------------------------------------------

        parameterIndex = 0;
        int argumentIndex = 0;

        for (const auto& inst : functionIR)
        {
            switch (inst.opcode)
            {
                // =================================================
                // FUNCTION_BEGIN
                // =================================================

                case IROpcode::FUNCTION_BEGIN:
                {
                    break;
                }


                // =================================================
                // PARAM
                //
                // Move ABI argument register into the function's
                // local stack slot.
                // =================================================

                case IROpcode::PARAM:
                {
                    emitParameterMove(
                        out,
                        inst.result,
                        parameterIndex
                    );

                    parameterIndex++;

                    break;
                }


                // =================================================
                // ASSIGN
                // =================================================

                case IROpcode::ASSIGN:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // ADD
                // =================================================

                case IROpcode::ADD:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    add eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // SUB
                // =================================================

                case IROpcode::SUB:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    sub eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // MUL
                // =================================================

                case IROpcode::MUL:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    imul eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // DIV
                // =================================================

                case IROpcode::DIV:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    cdq\n";
                    out << "    idiv ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // MOD
                // =================================================

                case IROpcode::MOD:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    cdq\n";
                    out << "    idiv ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], edx\n";

                    break;
                }


                // =================================================
                // BIT AND
                // =================================================

                case IROpcode::BIT_AND:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    and eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // BIT OR
                // =================================================

                case IROpcode::BIT_OR:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    or eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // BIT XOR
                // =================================================

                case IROpcode::BIT_XOR:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    xor eax, ecx\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // SHL
                // =================================================

                case IROpcode::SHL:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    shl eax, cl\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // SHR
                // =================================================

                case IROpcode::SHR:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    sar eax, cl\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // NEG
                // =================================================

                case IROpcode::NEG:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    out << "    neg eax\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // Comparisons
                // =================================================

                case IROpcode::CMP_EQ:
                case IROpcode::CMP_NE:
                case IROpcode::CMP_LT:
                case IROpcode::CMP_LE:
                case IROpcode::CMP_GT:
                case IROpcode::CMP_GE:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    emitOperand(
                        out,
                        inst.operand2,
                        "ecx"
                    );

                    out << "    cmp eax, ecx\n";

                    string base =
                        ".L_cmp_" + name +
                        "_" + inst.result;

                    for (char& c : base)
                    {
                        if (!isalnum(
                                static_cast<unsigned char>(c)) &&
                            c != '_')
                        {
                            c = '_';
                        }
                    }

                    string trueLabel =
                        base + "_true";

                    string endLabel =
                        base + "_end";

                    switch (inst.opcode)
                    {
                        case IROpcode::CMP_EQ:
                            out << "    je "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_NE:
                            out << "    jne "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_LT:
                            out << "    jl "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_LE:
                            out << "    jle "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_GT:
                            out << "    jg "
                                << trueLabel
                                << "\n";
                            break;

                        case IROpcode::CMP_GE:
                            out << "    jge "
                                << trueLabel
                                << "\n";
                            break;

                        default:
                            break;
                    }

                    out << "    mov eax, 0\n";

                    out
                        << "    jmp "
                        << endLabel
                        << "\n";

                    out
                        << trueLabel
                        << ":\n";

                    out << "    mov eax, 1\n";

                    out
                        << endLabel
                        << ":\n";

                    int offset =
                        getOffset(inst.result);

                    out
                        << "    mov DWORD PTR [rbp"
                        << offset
                        << "], eax\n";

                    break;
                }


                // =================================================
                // LABEL
                // =================================================

                case IROpcode::LABEL:
                {
                    out << inst.label << ":\n";
                    break;
                }


                // =================================================
                // JUMP
                // =================================================

                case IROpcode::JUMP:
                {
                    out
                        << "    jmp "
                        << inst.label
                        << "\n";

                    break;
                }


                // =================================================
                // JUMP_IF_FALSE
                // =================================================

                case IROpcode::JUMP_IF_FALSE:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    out << "    cmp eax, 0\n";

                    out
                        << "    je "
                        << inst.label
                        << "\n";

                    break;
                }


                // =================================================
                // JUMP_IF_TRUE
                // =================================================

                case IROpcode::JUMP_IF_TRUE:
                {
                    emitOperand(
                        out,
                        inst.operand1,
                        "eax"
                    );

                    out << "    cmp eax, 0\n";

                    out
                        << "    jne "
                        << inst.label
                        << "\n";

                    break;
                }


                // =================================================
                // Direct conditional branches
                // =================================================

                case IROpcode::JUMP_IF_EQ:
                case IROpcode::JUMP_IF_NE:
                case IROpcode::JUMP_IF_LT:
                case IROpcode::JUMP_IF_LE:
                case IROpcode::JUMP_IF_GT:
                case IROpcode::JUMP_IF_GE:
                {
                    emitComparisonJump(
                        out,
                        inst.opcode,
                        inst.operand1,
                        inst.operand2,
                        inst.label
                    );

                    break;
                }


                // =================================================
                // RETURN
                // =================================================

                case IROpcode::RETURN:
                {
                    if (!inst.operand1.empty())
                    {
                        emitOperand(
                            out,
                            inst.operand1,
                            "eax"
                        );
                    }
                    else
                    {
                        out << "    xor eax, eax\n";
                    }

                    out
                        << "    jmp "
                        << currentFunctionExit
                        << "\n";

                    break;
                }


                // =================================================
                // FUNCTION_END
                // =================================================

                case IROpcode::FUNCTION_END:
                {
                    break;
                }

                // ====================================================
                // FUNCTION ARGUMENT
                // ====================================================

                case IROpcode::ARG:
                {
                    emitCallArgument(
                        out,
                        inst.operand1,
                        argumentIndex
                    );

                    ++argumentIndex;
                    break;
                }

                // ====================================================
                // FUNCTION CALL
                // ====================================================

                case IROpcode::CALL:
                {
                    if (argumentIndex > 6)
                    {
                        cerr
                            << "Code generation error: "
                            << "function calls with more than 6 arguments "
                            << "are not supported yet.\n";

                        out.close();
                        return false;
                    }

                    out << "    call "
                        << inst.label
                        << "\n";

                    // Integer return value is in EAX.
                    int offset = getOffset(inst.result);
                    out << "    mov DWORD PTR [rbp"
                        << (offset < 0 ? "" : "-")
                        << offset
                        << "], eax\n";

                    // Ready for the next call.
                    argumentIndex = 0;

                    break;
                }
            }
        }


        // --------------------------------------------------------
        // Function epilogue
        // --------------------------------------------------------

        emitFunctionEpilogue(out);

        insideFunction = false;
    }


    // ========================================================
    // GNU-stack note
    // ========================================================

    out
        << ".section .note.GNU-stack,\"\",@progbits\n";


    out.close();

    return true;
}