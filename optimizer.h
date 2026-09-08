#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ir.h"

#include <vector>

class Optimizer
{
public:
    void optimize(std::vector<IRInstruction>& ir);
};

#endif