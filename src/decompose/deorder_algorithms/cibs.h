//
// Created by sabah on 7/5/25.
//

#ifndef CUSTOM_TRANSLATE_PY_CIBS_H
#define CUSTOM_TRANSLATE_PY_CIBS_H

#include <iostream>

#include "block_deorder.h"
#include "../plan_reduction.h"
#include "../block_replace.h"
#include "../../search/heuristics/domain_transition_graph.h"

using namespace block_replace;

class CIBS : public BlockDeorder {

    PlanReduction planReduction;
    BlockReplace blockReplace;

    bool print_details = false;
    bool timeout = false;

    void plan_reduction();

    void run_cibs();
    void block_substitution_concurrency();
    void write_cibs_result();

public:
    CIBS(bool only_replace_block, bool compromise_flex, plan_reduction_type type);

    void run() override {
        run_cibs();
    }

};


#endif //CUSTOM_TRANSLATE_PY_CIBS_H
