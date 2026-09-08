#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "ast.h"
#include "ir.h"
#include "optimizer.h"
#include "codegen.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[])
{
    std::string code;
    std::string outputFile = "output.s";

    // Read from file if specified, else stdin
    if (argc > 1)
    {
        std::string arg1 = argv[1];
        if (arg1 == "-h" || arg1 == "--help")
        {
            std::cout << "Usage: " << argv[0] << " [source_file] [-o output.s]\n";
            return 0;
        }

        std::ifstream file(arg1);
        if (!file.is_open())
        {
            std::cerr << "Error: cannot open source file '" << arg1 << "'\n";
            return 1;
        }
        code = std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

        for (int i = 2; i < argc; ++i)
        {
            if (std::string(argv[i]) == "-o" && i + 1 < argc)
            {
                outputFile = argv[++i];
            }
        }
    }
    else
    {
        std::cout << "Enter code. Finish with Ctrl+D:\n";
        code = std::string((std::istreambuf_iterator<char>(std::cin)),
                           std::istreambuf_iterator<char>());
    }

    if (code.empty())
    {
        std::cerr << "No input provided!\n";
        return 1;
    }

    // --------------------------------------------------------
    // 1. Lexical Analysis
    // --------------------------------------------------------
    Lexer lexer(code);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "\n=== TOKENS ===\n";
    for (const Token& token : tokens)
    {
        std::cout << tokenTypeToStr(token.type) << " : " << token.value << "\n";
    }

    // --------------------------------------------------------
    // 2. Syntax Analysis (Parsing)
    // --------------------------------------------------------
    Parser parser(tokens);
    std::vector<ASTNode*> program = parser.parseProgram();

    std::cout << "\n=== AST ===\n";
    for (size_t i = 0; i < program.size(); ++i)
    {
        std::cout << "Statement " << (i + 1) << ":\n";
        printAST(program[i], 1);
        std::cout << "\n";
    }

    // --------------------------------------------------------
    // 3. Semantic Analysis
    // --------------------------------------------------------
    std::cout << "=== SEMANTIC ANALYSIS ===\n";
    SemanticAnalyzer analyzer;
    bool semanticOk = analyzer.analyze(program);

    if (semanticOk)
    {
        std::cout << "Semantic analysis completed successfully.\n";
    }
    else
    {
        std::cout << "Semantic analysis finished with " << analyzer.getErrorCount() << " error(s).\n";
    }

    // --------------------------------------------------------
    // 4. IR Generation
    // --------------------------------------------------------
    IRGenerator irGenerator;
    std::vector<IRInstruction> ir = irGenerator.generate(program);

    std::cout << "\n=== ORIGINAL IR ===\n";
    printIR(ir);

    // --------------------------------------------------------
    // 5. Optimization
    // --------------------------------------------------------
    Optimizer optimizer;
    optimizer.optimize(ir);

    std::cout << "\n=== OPTIMIZED IR ===\n";
    printIR(ir);

    // --------------------------------------------------------
    // 6. Code Generation
    // --------------------------------------------------------
    CodeGenerator codeGenerator;
    if (!codeGenerator.generate(ir, outputFile))
    {
        std::cerr << "Code generation failed.\n";
        for (ASTNode* node : program)
        {
            freeAST(node);
        }
        return 1;
    }

    std::cout << "\nAssembly generated: " << outputFile << "\n";

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------
    for (ASTNode* node : program)
    {
        freeAST(node);
    }

    return 0;
}