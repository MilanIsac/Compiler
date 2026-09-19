#ifndef CFG_H
#define CFG_H

#include "ir.h"

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct BasicBlock;

// ============================================================
// SSA Phi Node representation (Phase 7)
//
// Represents: result = PHI( (val1, predBlock1), (val2, predBlock2), ... )
// ============================================================

struct PhiNode
{
    std::string result;       // SSA variable defined, e.g. "x_2"
    std::string originalVar;  // Original variable name, e.g. "x"

    // List of (value, predecessor block) incoming pairs
    std::vector<std::pair<std::string, BasicBlock*>> incoming;
};

// ============================================================
// Basic Block (Phase 6B)
//
// A sequence of instructions with:
// - Exactly one entry point at the beginning
// - Exactly one exit path at the end (no internal jumps/labels)
// ============================================================

struct BasicBlock
{
    int id;                                // Unique identifier (0, 1, 2, ...)
    std::string label;                     // Block label (e.g., "entry", "L1", "L_then")
    std::vector<IRInstruction> instructions; // Instructions inside this block
    std::vector<PhiNode> phis;             // SSA phi nodes at the head of the block

    std::vector<BasicBlock*> predecessors;
    std::vector<BasicBlock*> successors;

    // Dominance information (Phase 6C)
    std::set<BasicBlock*> dominators;        // Set of blocks dominating this block
    BasicBlock* idom = nullptr;              // Immediate dominator
    std::vector<BasicBlock*> domTreeChildren;// Children in dominator tree
    std::set<BasicBlock*> domFrontier;       // Dominance frontier DF(b)

    BasicBlock(int id, const std::string& label = "");

    void addSuccessor(BasicBlock* succ);
    void addPredecessor(BasicBlock* pred);
    void removeSuccessor(BasicBlock* succ);
    void removePredecessor(BasicBlock* pred);

    bool hasPhi() const;
    bool isTerminated() const;
};

// ============================================================
// Control Flow Graph (Phase 6C)
//
// Directed graph of basic blocks representing the flow of control
// through a function or program.
// ============================================================

class ControlFlowGraph
{
public:
    std::string name;                      // Function name or "main"
    std::string returnType;                // Function return type
    std::vector<std::pair<std::string, std::string>> parameters; // (paramName, paramType)

    std::vector<std::unique_ptr<BasicBlock>> blocks;
    BasicBlock* entry = nullptr;

    ControlFlowGraph();
    explicit ControlFlowGraph(const std::string& name);

    // Build CFG from a sequence of IR instructions for a function
    static ControlFlowGraph build(
        const std::string& functionName,
        const std::string& returnType,
        const std::vector<std::pair<std::string, std::string>>& params,
        const std::vector<IRInstruction>& instructions
    );

    // Build CFGs for all functions in the program IR
    static std::vector<ControlFlowGraph> buildAll(
        const std::vector<IRInstruction>& programIR
    );

    // Compute dominators, immediate dominators, and dominance frontiers
    void computeDominance();

    // Eliminate unreachable blocks
    bool eliminateDeadBlocks();

    // Convert CFG back to standard linear IR (for codegen)
    std::vector<IRInstruction> toLinearIR() const;

    // Convert CFG to linear IR with PHI instructions (for SSA viewing)
    std::vector<IRInstruction> toSSALinearIR() const;

    // Printing and visualization
    void print(std::ostream& out = std::cout) const;
    void printDominance(std::ostream& out = std::cout) const;
    void toDot(std::ostream& out) const;
};

#endif
