//
// Created by sabah on 6/12/25.
//

#ifndef CUSTOM_TRANSLATE_PY_PLAN_REDUCTION_H
#define CUSTOM_TRANSLATE_PY_PLAN_REDUCTION_H


#include <chrono>
#include "block_deorder_plan.h"
#include "myutils.h"
#include "../search/plugins/plugin.h"


enum plan_reduction_type  { BJ, FJ, NOREDUCTION};

class PlanReduction {
    BlockDeorderPlan bdpop;

    plan_reduction_type type;

    bool is_redundant_by_backward_justification(int b_x);

    void get_dependent_blocks(int b_x, vector<int> *dependent_blocks, vector<int> *visited);

    bool remove_redundant_block();

    bool remove_redundant_block_by_backward_justification();

    void remove_redundant_by_forward_justification();


public:

    PlanReduction(plan_reduction_type t): type(t){};

    void set_bdpo_plan(BlockDeorderPlan blockDeorderPlan);

    void start();

    BlockDeorderPlan get_bdpo_plan();

    BlockDeorderPlan start(BlockDeorderPlan blockDeorderPlan);
};




#endif //CUSTOM_TRANSLATE_PY_PLAN_REDUCTION_H
