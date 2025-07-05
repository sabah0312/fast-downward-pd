//
// Created by sabah on 6/1/25.
//

#ifndef FAST_DOWNWARD_PY_DEORDER_H
#define FAST_DOWNWARD_PY_DEORDER_H


#include "../search/plugins/options.h"
#include "deorder_manager.h"
#include "block_deorder_plan.h"
#include "../search/task_utils/successor_generator.h"
#include "../search/state_registry.h"

class DeorderAlgorithm {

    std::string  description;
protected:
    DeorderManager deorderManager;
    Plan original_plan;
    bool print_details = true;
    TaskProxy task_proxy;
    double max_time = 5400;

public:
    BlockDeorderPlan blockDeorderPlan;
    DeorderAlgorithm(const string &description);
    virtual void run() = 0;
    DeorderManager &get_deorder_manager() {return deorderManager;}
    virtual ~DeorderAlgorithm(){}

    void read_plan(string planfile);

    void initialize();

    int get_plan_cost(const Plan &plan);

    void reset_plan(Plan plan);

    void  set_plan(Plan plan);

    void reset_plan(BlockDeorderPlan bdpop);

    void write_problem_details();

    void restart_timer();
};

extern void add_deorder_algorithm_options_to_feature(
        plugins::Feature &feature, const std::string &description);

extern std::tuple<std::string, std::string>
get_deorder_algorithm_arguments_from_options(
        const plugins::Options &opts);

#endif //FAST_DOWNWARD_PY_DEORDER_H
