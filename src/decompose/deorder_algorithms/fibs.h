//
// Created by oem on ১২/৫/২১.
//

#ifndef FAST_DOWNWARD_FIBS_H
#define FAST_DOWNWARD_FIBS_H

#include <iostream>

#include "block_deorder.h"
#include "../plan_reduction.h"
#include "../block_replace.h"
#include "../../search/heuristics/domain_transition_graph.h"

using namespace block_replace;

class FIBS: public BlockDeorder {

    PlanReduction planReduction;
    BlockReplace blockReplace;

    bool print_details = false;
    bool concurrency = false;
    bool timeout = false;

    void plan_reduction();

    void run_fibs();
    void block_substitution_deorder();
    void write_fibs_result();

public:
    FIBS( bool only_replace_block, bool compromise_flex, bool concurrency,
         plan_reduction_type type);

    void run() override {
        run_fibs();
    }

};
#endif //FAST_DOWNWARD_FIBS_H
