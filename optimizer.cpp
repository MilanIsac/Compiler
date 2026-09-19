#include "optimizer.h"

#include <cctype>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// ============================================================
// Is integer helper
// ============================================================

static bool isNumber(const std::string& value)
{
    if (value.empty())
        return false;

    size_t start = 0;

    if (value[0] == '-' || value[0] == '+')
        start = 1;

    if (start >= value.size())
        return false;

    for (size_t i = start; i < value.size(); ++i)
    {
        if (!isdigit(static_cast<unsigned char>(value[i])))
            return false;
    }

    return true;
}


// ============================================================
// Check whether a name is a compiler temporary
//
// Examples:
// t1
// t2
// t25
// ============================================================

[[maybe_unused]] static bool isTemp(const std::string& name)
{
    if (name.size() < 2 || name[0] != 't')
        return false;

    for (size_t i = 1; i < name.size(); ++i)
    {
        if (!isdigit(static_cast<unsigned char>(name[i])))
            return false;
    }

    return true;
}


// ============================================================
// Pass 1
// Constant folding + algebraic simplification
// ============================================================

static bool constantFold(std::vector<IRInstruction>& ir)
{
    bool changed = false;

    for (auto& inst : ir)
    {
        bool isBinOp =
            (inst.opcode == IROpcode::ADD ||
             inst.opcode == IROpcode::SUB ||
             inst.opcode == IROpcode::MUL ||
             inst.opcode == IROpcode::DIV ||
             inst.opcode == IROpcode::MOD ||
             inst.opcode == IROpcode::BIT_AND ||
             inst.opcode == IROpcode::BIT_OR ||
             inst.opcode == IROpcode::BIT_XOR ||
             inst.opcode == IROpcode::SHL ||
             inst.opcode == IROpcode::SHR);

        if (!isBinOp)
            continue;

        bool num1 = isNumber(inst.operand1);
        bool num2 = isNumber(inst.operand2);

        // --------------------------------------------------------
        // Constant folding
        // Example:
        //
        // t1 = 10 + 5
        //
        // becomes:
        //
        // t1 = 15
        // --------------------------------------------------------

        if (num1 && num2)
        {
            long long a = std::stoll(inst.operand1);
            long long b = std::stoll(inst.operand2);

            long long result = 0;
            bool valid = true;

            switch (inst.opcode)
            {
                case IROpcode::ADD:
                    result = a + b;
                    break;

                case IROpcode::SUB:
                    result = a - b;
                    break;

                case IROpcode::MUL:
                    result = a * b;
                    break;

                case IROpcode::DIV:
                    if (b == 0)
                        valid = false;
                    else
                        result = a / b;
                    break;

                case IROpcode::MOD:
                    if (b == 0)
                        valid = false;
                    else
                        result = a % b;
                    break;

                case IROpcode::BIT_AND:
                    result = a & b;
                    break;

                case IROpcode::BIT_OR:
                    result = a | b;
                    break;

                case IROpcode::BIT_XOR:
                    result = a ^ b;
                    break;

                case IROpcode::SHL:
                    result = a << b;
                    break;

                case IROpcode::SHR:
                    result = a >> b;
                    break;

                default:
                    valid = false;
                    break;
            }

            if (valid)
            {
                inst.opcode = IROpcode::ASSIGN;
                inst.operand1 = std::to_string(result);
                inst.operand2 = "";

                changed = true;

                continue;
            }
        }


        // --------------------------------------------------------
        // Algebraic identities
        // --------------------------------------------------------

        // x + 0 -> x
        if (inst.opcode == IROpcode::ADD &&
            inst.operand2 == "0")
        {
            inst.opcode = IROpcode::ASSIGN;
            inst.operand2 = "";

            changed = true;
            continue;
        }

        // 0 + x -> x
        if (inst.opcode == IROpcode::ADD &&
            inst.operand1 == "0")
        {
            inst.opcode = IROpcode::ASSIGN;
            inst.operand1 = inst.operand2;
            inst.operand2 = "";

            changed = true;
            continue;
        }

        // x - 0 -> x
        if (inst.opcode == IROpcode::SUB &&
            inst.operand2 == "0")
        {
            inst.opcode = IROpcode::ASSIGN;
            inst.operand2 = "";

            changed = true;
            continue;
        }

        // x - x -> 0
        if (inst.opcode == IROpcode::SUB &&
            inst.operand1 == inst.operand2 &&
            !inst.operand1.empty())
        {
            inst.opcode = IROpcode::ASSIGN;
            inst.operand1 = "0";
            inst.operand2 = "";

            changed = true;
            continue;
        }

        // x * 1 -> x
        if (inst.opcode == IROpcode::MUL &&
            inst.operand2 == "1")
        {
            inst.opcode = IROpcode::ASSIGN;
            inst.operand2 = "";

            changed = true;
            continue;
        }

        // 1 * x -> x
        if (inst.opcode == IROpcode::MUL &&
            inst.operand1 == "1")
        {
            inst.opcode = IROpcode::ASSIGN;
            inst.operand1 = inst.operand2;
            inst.operand2 = "";

            changed = true;
            continue;
        }

        // x * 0 -> 0
        // 0 * x -> 0
        if (inst.opcode == IROpcode::MUL &&
            (inst.operand1 == "0" ||
             inst.operand2 == "0"))
        {
            inst.opcode = IROpcode::ASSIGN;
            inst.operand1 = "0";
            inst.operand2 = "";

            changed = true;
            continue;
        }

        // x / 1 -> x
        if (inst.opcode == IROpcode::DIV &&
            inst.operand2 == "1")
        {
            inst.opcode = IROpcode::ASSIGN;
            inst.operand2 = "";

            changed = true;
            continue;
        }
    }

    return changed;
}


// ============================================================
// Pass 2
// Constant conditional jump folding
// ============================================================

static bool foldBranchConditions(
    std::vector<IRInstruction>& ir)
{
    bool changed = false;

    std::vector<IRInstruction> newIR;

    for (const auto& inst : ir)
    {
        bool isCmpJump =
            (inst.opcode == IROpcode::JUMP_IF_EQ ||
             inst.opcode == IROpcode::JUMP_IF_NE ||
             inst.opcode == IROpcode::JUMP_IF_LT ||
             inst.opcode == IROpcode::JUMP_IF_LE ||
             inst.opcode == IROpcode::JUMP_IF_GT ||
             inst.opcode == IROpcode::JUMP_IF_GE);

        // --------------------------------------------------------
        // Example:
        //
        // JUMP_IF_EQ 10, 10, L1
        //
        // becomes:
        //
        // JUMP L1
        // --------------------------------------------------------

        if (isCmpJump &&
            isNumber(inst.operand1) &&
            isNumber(inst.operand2))
        {
            long long a =
                std::stoll(inst.operand1);

            long long b =
                std::stoll(inst.operand2);

            bool taken = false;

            switch (inst.opcode)
            {
                case IROpcode::JUMP_IF_EQ:
                    taken = (a == b);
                    break;

                case IROpcode::JUMP_IF_NE:
                    taken = (a != b);
                    break;

                case IROpcode::JUMP_IF_LT:
                    taken = (a < b);
                    break;

                case IROpcode::JUMP_IF_LE:
                    taken = (a <= b);
                    break;

                case IROpcode::JUMP_IF_GT:
                    taken = (a > b);
                    break;

                case IROpcode::JUMP_IF_GE:
                    taken = (a >= b);
                    break;

                default:
                    break;
            }

            if (taken)
            {
                newIR.push_back({
                    IROpcode::JUMP,
                    "",
                    "",
                    "",
                    inst.label
                });
            }

            changed = true;
            continue;
        }


        // --------------------------------------------------------
        // JUMP_IF_FALSE constant
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::JUMP_IF_FALSE &&
            isNumber(inst.operand1))
        {
            long long value =
                std::stoll(inst.operand1);

            if (value == 0)
            {
                newIR.push_back({
                    IROpcode::JUMP,
                    "",
                    "",
                    "",
                    inst.label
                });
            }

            changed = true;
            continue;
        }


        // --------------------------------------------------------
        // JUMP_IF_TRUE constant
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::JUMP_IF_TRUE &&
            isNumber(inst.operand1))
        {
            long long value =
                std::stoll(inst.operand1);

            if (value != 0)
            {
                newIR.push_back({
                    IROpcode::JUMP,
                    "",
                    "",
                    "",
                    inst.label
                });
            }

            changed = true;
            continue;
        }


        newIR.push_back(inst);
    }

    if (changed)
        ir = std::move(newIR);

    return changed;
}


// ============================================================
// Pass 3
// Local copy & constant propagation
// ============================================================

static bool copyAndConstantPropagation(
    std::vector<IRInstruction>& ir)
{
    bool changed = false;

    std::unordered_map<
        std::string,
        std::string
    > valueMap;


    for (auto& inst : ir)
    {
        // --------------------------------------------------------
        // Function parameters are definitions.
        //
        // Do NOT propagate values across PARAM.
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::PARAM)
        {
            if (!inst.result.empty())
            {
                valueMap.erase(inst.result);

                // Remove mappings that point to this parameter.
                for (auto it = valueMap.begin();
                     it != valueMap.end();)
                {
                    if (it->second == inst.result)
                        it = valueMap.erase(it);
                    else
                        ++it;
                }
            }

            continue;
        }


        // --------------------------------------------------------
        // Propagate operand1
        // --------------------------------------------------------

        if (!inst.operand1.empty())
        {
            auto it = valueMap.find(inst.operand1);
            if (it != valueMap.end())
            {
                inst.operand1 = it->second;
                changed = true;
            }
        }

        // --------------------------------------------------------
        // Propagate operand2
        // --------------------------------------------------------

        if (!inst.operand2.empty())
        {
            auto it = valueMap.find(inst.operand2);
            if (it != valueMap.end())
            {
                inst.operand2 = it->second;
                changed = true;
            }
        }

        // --------------------------------------------------------
        // Control-flow boundaries and calls reset local knowledge.
        // (Handled after propagating operand values for returns/branches!)
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::LABEL ||
            inst.opcode == IROpcode::JUMP ||
            inst.opcode == IROpcode::JUMP_IF_FALSE ||
            inst.opcode == IROpcode::JUMP_IF_TRUE ||
            inst.opcode == IROpcode::JUMP_IF_EQ ||
            inst.opcode == IROpcode::JUMP_IF_NE ||
            inst.opcode == IROpcode::JUMP_IF_LT ||
            inst.opcode == IROpcode::JUMP_IF_LE ||
            inst.opcode == IROpcode::JUMP_IF_GT ||
            inst.opcode == IROpcode::JUMP_IF_GE ||
            inst.opcode == IROpcode::CALL ||
            inst.opcode == IROpcode::RETURN)
        {
            valueMap.clear();
            continue;
        }

        if (inst.opcode == IROpcode::ARG)
        {
            continue;
        }

        // --------------------------------------------------------
        // Record assignment
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::ASSIGN &&
            !inst.result.empty())
        {
            // Remove mappings that point to result.
            for (auto it = valueMap.begin();
                 it != valueMap.end();)
            {
                if (it->second == inst.result)
                    it = valueMap.erase(it);
                else
                    ++it;
            }

            if (!inst.operand1.empty())
            {
                valueMap[inst.result] =
                    inst.operand1;
            }
            else
            {
                valueMap.erase(inst.result);
            }
        }
        else if (!inst.result.empty())
        {
            // Arithmetic result invalidates previous mapping.
            valueMap.erase(inst.result);

            for (auto it = valueMap.begin();
                 it != valueMap.end();)
            {
                if (it->second == inst.result)
                    it = valueMap.erase(it);
                else
                    ++it;
            }
        }
    }

    return changed;
}


// ============================================================
// Pass 4
// Control-flow & jump simplification
// ============================================================

static bool simplifyControlFlow(
    std::vector<IRInstruction>& ir)
{
    bool changed = false;

    std::vector<IRInstruction> newIR;

    for (size_t i = 0; i < ir.size(); ++i)
    {
        // --------------------------------------------------------
        // Never optimize through function boundaries.
        // --------------------------------------------------------

        if (ir[i].opcode == IROpcode::FUNCTION_BEGIN ||
            ir[i].opcode == IROpcode::FUNCTION_END)
        {
            newIR.push_back(ir[i]);
            continue;
        }


        // --------------------------------------------------------
        // JUMP immediately followed by same LABEL:
        //
        // JUMP L1
        // L1:
        //
        // becomes:
        //
        // L1:
        // --------------------------------------------------------

        if (ir[i].opcode == IROpcode::JUMP)
        {
            if (i + 1 < ir.size() &&
                ir[i + 1].opcode == IROpcode::LABEL &&
                ir[i + 1].label == ir[i].label)
            {
                changed = true;
                continue;
            }
        }


        newIR.push_back(ir[i]);


        // --------------------------------------------------------
        // Remove unreachable instructions after JUMP / RETURN.
        //
        // IMPORTANT:
        //
        // Stop at:
        // - LABEL
        // - FUNCTION_BEGIN
        // - FUNCTION_END
        //
        // so that we never delete another function.
        // --------------------------------------------------------

        if (ir[i].opcode == IROpcode::JUMP ||
            ir[i].opcode == IROpcode::RETURN)
        {
            size_t j = i + 1;

            while (j < ir.size() &&
                   ir[j].opcode != IROpcode::LABEL &&
                   ir[j].opcode != IROpcode::FUNCTION_BEGIN &&
                   ir[j].opcode != IROpcode::FUNCTION_END)
            {
                ++j;
                changed = true;
            }

            if (j > 0)
                i = j - 1;
        }
    }


    if (changed)
        ir = std::move(newIR);

    return changed;
}


// ============================================================
// Pass 5
// Dead code & unused temporary elimination
// ============================================================

static bool eliminateDeadCode(
    std::vector<IRInstruction>& ir)
{
    bool changed = false;


    // --------------------------------------------------------
    // Collect labels and variables used by this function.
    // --------------------------------------------------------

    std::unordered_set<std::string> usedLabels;
    std::unordered_set<std::string> usedVars;


    for (const auto& inst : ir)
    {
        if (!inst.label.empty() &&
            inst.opcode != IROpcode::LABEL)
        {
            usedLabels.insert(inst.label);
        }

        if (!inst.operand1.empty())
        {
            usedVars.insert(inst.operand1);
        }

        if (!inst.operand2.empty())
        {
            usedVars.insert(inst.operand2);
        }
    }


    std::vector<IRInstruction> newIR;


    for (const auto& inst : ir)
    {
        // --------------------------------------------------------
        // NEVER remove function markers.
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::FUNCTION_BEGIN ||
            inst.opcode == IROpcode::FUNCTION_END)
        {
            newIR.push_back(inst);
            continue;
        }


        // --------------------------------------------------------
        // PARAM must never be removed.
        //
        // It has a side effect:
        //
        // ABI register -> local parameter
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::PARAM)
        {
            newIR.push_back(inst);
            continue;
        }


        // --------------------------------------------------------
        // Protect ARG and CALL from DCE.
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::ARG ||
            inst.opcode == IROpcode::CALL)
        {
            newIR.push_back(inst);
            continue;
        }


        // --------------------------------------------------------
        // Remove unused labels.
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::LABEL)
        {
            if (usedLabels.find(inst.label) ==
                usedLabels.end())
            {
                changed = true;
                continue;
            }
        }


        // --------------------------------------------------------
        // Remove unused compiler temporary.
        //
        // Example:
        //
        // t1 = a + b
        //
        // if t1 is never used, remove it.
        // --------------------------------------------------------

        if (!inst.result.empty() &&
            usedVars.find(inst.result) == usedVars.end())
        {
            // Do not remove PARAM/CALL/FUNCTION markers.
            if (inst.opcode != IROpcode::PARAM &&
                inst.opcode != IROpcode::CALL &&
                inst.opcode != IROpcode::FUNCTION_BEGIN &&
                inst.opcode != IROpcode::FUNCTION_END)
            {
                changed = true;
                continue;
            }
        }


        // --------------------------------------------------------
        // x = x
        // --------------------------------------------------------

        if (inst.opcode == IROpcode::ASSIGN &&
            inst.result == inst.operand1 &&
            !inst.result.empty())
        {
            changed = true;
            continue;
        }


        newIR.push_back(inst);
    }


    if (changed)
        ir = std::move(newIR);

    return changed;
}


// ============================================================
// Pass 6
// Strength reduction
//
// Converts expensive operations into cheaper equivalents.
// Example:
// x * 2  ->  x << 1
// x * 8  ->  x << 3
// ============================================================

static bool strengthReduction(std::vector<IRInstruction>& ir)
{
    bool changed = false;

    for (auto& inst : ir)
    {
        if (inst.opcode == IROpcode::MUL)
        {
            // Case 1: x * (2^k) -> x << k
            if (isNumber(inst.operand2))
            {
                long long b = std::stoll(inst.operand2);
                if (b > 1 && (b & (b - 1)) == 0)
                {
                    int shift = 0;
                    while ((1LL << shift) < b)
                        ++shift;

                    inst.opcode = IROpcode::SHL;
                    inst.operand2 = std::to_string(shift);
                    changed = true;
                    continue;
                }
            }

            // Case 2: (2^k) * x -> x << k
            if (isNumber(inst.operand1))
            {
                long long a = std::stoll(inst.operand1);
                if (a > 1 && (a & (a - 1)) == 0)
                {
                    int shift = 0;
                    while ((1LL << shift) < a)
                        ++shift;

                    inst.opcode = IROpcode::SHL;
                    inst.operand1 = inst.operand2;
                    inst.operand2 = std::to_string(shift);
                    changed = true;
                    continue;
                }
            }
        }
    }

    return changed;
}


// ============================================================
// Pass 7
// Common Subexpression Elimination (CSE)
//
// Reuses previously computed results for identical expressions.
// Example:
// t1 = x + y
// t2 = x + y
// becomes:
// t1 = x + y
// t2 = t1
// ============================================================

static bool eliminateCommonSubexpressions(std::vector<IRInstruction>& ir)
{
    bool changed = false;

    std::unordered_map<std::string, std::string> exprMap;
    std::unordered_map<std::string, std::unordered_set<std::string>> varToKeys;

    auto invalidateVar = [&](const std::string& var) {
        if (var.empty())
            return;
        auto it = varToKeys.find(var);
        if (it != varToKeys.end())
        {
            for (const auto& key : it->second)
            {
                exprMap.erase(key);
            }
            varToKeys.erase(it);
        }
        for (auto eit = exprMap.begin(); eit != exprMap.end(); )
        {
            if (eit->second == var)
            {
                eit = exprMap.erase(eit);
            }
            else
            {
                ++eit;
            }
        }
    };

    auto clearState = [&]() {
        exprMap.clear();
        varToKeys.clear();
    };

    for (auto& inst : ir)
    {
        if (inst.opcode == IROpcode::LABEL ||
            inst.opcode == IROpcode::JUMP ||
            inst.opcode == IROpcode::JUMP_IF_FALSE ||
            inst.opcode == IROpcode::JUMP_IF_TRUE ||
            inst.opcode == IROpcode::JUMP_IF_EQ ||
            inst.opcode == IROpcode::JUMP_IF_NE ||
            inst.opcode == IROpcode::JUMP_IF_LT ||
            inst.opcode == IROpcode::JUMP_IF_LE ||
            inst.opcode == IROpcode::JUMP_IF_GT ||
            inst.opcode == IROpcode::JUMP_IF_GE ||
            inst.opcode == IROpcode::CALL ||
            inst.opcode == IROpcode::RETURN)
        {
            clearState();
            continue;
        }

        if (inst.opcode == IROpcode::PARAM)
        {
            invalidateVar(inst.result);
            continue;
        }

        bool isBinOp =
            (inst.opcode == IROpcode::ADD ||
             inst.opcode == IROpcode::SUB ||
             inst.opcode == IROpcode::MUL ||
             inst.opcode == IROpcode::DIV ||
             inst.opcode == IROpcode::MOD ||
             inst.opcode == IROpcode::BIT_AND ||
             inst.opcode == IROpcode::BIT_OR ||
             inst.opcode == IROpcode::BIT_XOR ||
             inst.opcode == IROpcode::SHL ||
             inst.opcode == IROpcode::SHR ||
             inst.opcode == IROpcode::CMP_EQ ||
             inst.opcode == IROpcode::CMP_NE ||
             inst.opcode == IROpcode::CMP_LT ||
             inst.opcode == IROpcode::CMP_LE ||
             inst.opcode == IROpcode::CMP_GT ||
             inst.opcode == IROpcode::CMP_GE);

        bool isUnaryOp = (inst.opcode == IROpcode::NEG);

        if (isBinOp)
        {
            std::string op1 = inst.operand1;
            std::string op2 = inst.operand2;

            bool isCommutative =
                (inst.opcode == IROpcode::ADD ||
                 inst.opcode == IROpcode::MUL ||
                 inst.opcode == IROpcode::BIT_AND ||
                 inst.opcode == IROpcode::BIT_OR ||
                 inst.opcode == IROpcode::BIT_XOR ||
                 inst.opcode == IROpcode::CMP_EQ ||
                 inst.opcode == IROpcode::CMP_NE);

            if (isCommutative && op1 > op2)
            {
                std::swap(op1, op2);
            }

            std::string key = std::to_string(static_cast<int>(inst.opcode)) + ":" + op1 + ":" + op2;

            auto it = exprMap.find(key);
            if (it != exprMap.end() && it->second != inst.result)
            {
                inst.opcode = IROpcode::ASSIGN;
                inst.operand1 = it->second;
                inst.operand2 = "";
                changed = true;
                invalidateVar(inst.result);
            }
            else
            {
                invalidateVar(inst.result);
                exprMap[key] = inst.result;
                if (!isNumber(inst.operand1))
                    varToKeys[inst.operand1].insert(key);
                if (!isNumber(inst.operand2))
                    varToKeys[inst.operand2].insert(key);
                varToKeys[inst.result].insert(key);
            }
        }
        else if (isUnaryOp)
        {
            std::string key = "NEG:" + inst.operand1;
            auto it = exprMap.find(key);
            if (it != exprMap.end() && it->second != inst.result)
            {
                inst.opcode = IROpcode::ASSIGN;
                inst.operand1 = it->second;
                inst.operand2 = "";
                changed = true;
                invalidateVar(inst.result);
            }
            else
            {
                invalidateVar(inst.result);
                exprMap[key] = inst.result;
                if (!isNumber(inst.operand1))
                    varToKeys[inst.operand1].insert(key);
                varToKeys[inst.result].insert(key);
            }
        }
        else
        {
            if (!inst.result.empty())
            {
                invalidateVar(inst.result);
            }
        }
    }

    return changed;
}


// ============================================================
// Optimize ONE region
//
// A region is either:
// - top-level/main code
// - one complete function
//
// This is important because optimization information must not
// leak from one function into another.
// ============================================================

static bool optimizeRegion(
    std::vector<IRInstruction>& region)
{
    const int MAX_PASSES = 20;

    bool overallChanged = false;

    for (int pass = 0;
         pass < MAX_PASSES;
         ++pass)
    {
        bool changed = false;

        changed |= constantFold(region);

        changed |= strengthReduction(region);

        changed |= eliminateCommonSubexpressions(region);

        changed |= foldBranchConditions(region);

        changed |= copyAndConstantPropagation(region);

        changed |= simplifyControlFlow(region);

        changed |= eliminateDeadCode(region);


        if (!changed)
            break;

        overallChanged = true;
    }

    return overallChanged;
}


// ============================================================
// Main optimizer
//
// IMPORTANT:
// This version is FUNCTION-AWARE.
//
// Before:
//     optimize(all IR)
//
// Now:
//
//     main region
//          ↓
//     optimize(main)
//
//     function add
//          ↓
//     optimize(add)
//
//     function foo
//          ↓
//     optimize(foo)
//
// Function boundaries are preserved.
// ============================================================

void Optimizer::optimize(
    std::vector<IRInstruction>& ir)
{
    if (ir.empty())
        return;


    std::vector<IRInstruction> result;

    std::vector<IRInstruction> region;

    // bool insideFunction = false;


    for (const auto& inst : ir)
    {
        // ========================================================
        // Function begins
        // ========================================================

        if (inst.opcode == IROpcode::FUNCTION_BEGIN)
        {
            // Optimize any top-level region before the function.
            if (!region.empty())
            {
                optimizeRegion(region);

                result.insert(
                    result.end(),
                    region.begin(),
                    region.end()
                );

                region.clear();
            }


            // Preserve FUNCTION_BEGIN.
            result.push_back(inst);

            // insideFunction = true;

            continue;
        }


        // ========================================================
        // Function ends
        // ========================================================

        if (inst.opcode == IROpcode::FUNCTION_END)
        {
            // Optimize the function body only.
            if (!region.empty())
            {
                optimizeRegion(region);

                result.insert(
                    result.end(),
                    region.begin(),
                    region.end()
                );

                region.clear();
            }


            // Preserve FUNCTION_END.
            result.push_back(inst);

            // insideFunction = false;

            continue;
        }


        // ========================================================
        // Normal instruction
        // ========================================================

        region.push_back(inst);
    }


    // ========================================================
    // Remaining top-level/main code
    // ========================================================

    if (!region.empty())
    {
        optimizeRegion(region);

        result.insert(
            result.end(),
            region.begin(),
            region.end()
        );
    }


    // ========================================================
    // Replace original IR
    // ========================================================

    ir = std::move(result);
}