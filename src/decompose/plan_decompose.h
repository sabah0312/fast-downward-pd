//
// Created by sabah on 3/14/25.
//

#ifndef FAST_DOWNWARD_PLAN_DECOMPOSE_H
#define FAST_DOWNWARD_PLAN_DECOMPOSE_H


#include "block_deorder_plan.h"
#include "myutils.h"
#include "deorder_algorithm.h"
#include "deorder_algorithms/eog.h"

class PlanDecompose : public EOG {

public:
    PlanDecompose():EOG(){}

    BlockDeorderPlan bdpop;

    vector<VariableProxy> agents;

    bool is_agent(VariableProxy variableProxy);

    bool has_precondition(int b_x, int var);

    int select_var(Block *block);

    vector<int> add_block(int b_x, int var, map<int, bool> *visited);

    vector<vector<int>> decompose();

    void run();
};


#endif //FAST_DOWNWARD_PLAN_DECOMPOSE_H
