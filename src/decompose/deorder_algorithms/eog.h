//
// Created by sabah on 6/1/25.
//

#ifndef FAST_DOWNWARD_PY_STEPWISE_DEORDER_H
#define FAST_DOWNWARD_PY_STEPWISE_DEORDER_H


//
// Created by sabah on 6/1/25.
//

#include "../deorder_algorithm.h"

#include <iostream>

using namespace std;

class EOG : public DeorderAlgorithm {
    void find_pcs();
    void find_cds_and_dps();
    void add_pc_constraint(int b_x, int b_y, OrderingConstraint constraint);
    EffectProxy get_producing_effect(int b_x, FactProxy f);
    int find_candidate_producer(int b_x, FactProxy factProxy, bool is_latest);
    bool is_candidate_producer(int b_x, FactProxy f, bool is_latest);

public:
    EOG(): DeorderAlgorithm(" ") {}
    void do_step_deordering();
    void write_eog_result();
    void run() override {
        initialize();
        write_problem_details();
        cout << "EOG is running"<< endl;
        do_step_deordering();
        write_eog_result();
    }

};

extern void add_eager_search_options_to_feature(plugins::Feature &feature, const string &description);

tuple<string,string>
get_eager_search_arguments_from_options(const plugins::Options &opts);


#endif //FAST_DOWNWARD_PY_STEPWISE_DEORDER_H
