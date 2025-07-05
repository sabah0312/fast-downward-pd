//
// Created by sabah on 6/1/25.
//
#include "../search/plugins/plugin.h"
#include <iostream>

#include "deorder_algorithm.h"
#include "../search/tasks/root_task.h"
#include "myutils.h"

using namespace std;

DeorderAlgorithm :: DeorderAlgorithm(const string &description)
        : description(description), task_proxy(*tasks::g_root_task) {
}
void DeorderAlgorithm :: set_plan(Plan plan){
    original_plan = plan;
    blockDeorderPlan.initialize(original_plan);
    blockDeorderPlan.init_block = 0;
    blockDeorderPlan.goal_block = blockDeorderPlan.blocks.size() - 1;

}

void DeorderAlgorithm :: initialize(){
    deorderManager.remove_previous_files();
    read_plan(deorderManager.get_plan_file_path());
    set_plan(original_plan);
    deorderManager.start_timer();
}

void DeorderAlgorithm :: restart_timer(){
    deorderManager.start_timer();
}

void DeorderAlgorithm :: reset_plan(Plan plan){
    original_plan = plan;
    blockDeorderPlan.initialize(original_plan);
}

void DeorderAlgorithm :: reset_plan(BlockDeorderPlan bdpop){
    blockDeorderPlan = bdpop;
}

int DeorderAlgorithm:: get_plan_cost(const Plan &plan) {
    OperatorsProxy operators =task_proxy.get_operators();
    int plan_cost = 0;
    for (OperatorID op_id : plan) {
        plan_cost += operators[op_id].get_cost();
    }
    return plan_cost;
}

void DeorderAlgorithm :: read_plan (string planfile){
    original_plan = blockDeorderPlan.read_sequential_plan(planfile);
    OperatorsProxy operatorsProxy = blockDeorderPlan.task_proxy.get_operators();
    int goal_op_ind =  operatorsProxy[operatorsProxy.size()-1].get_id();
    int start_op_ind = operatorsProxy[operatorsProxy.size()-2].get_id();
    original_plan.insert(original_plan.begin(), OperatorID(start_op_ind));
    original_plan.push_back(OperatorID(goal_op_ind));
}

void DeorderAlgorithm :: write_problem_details() {
    ofstream out;
    string result_file = "problem_details.csv";
    out.open(result_file, ios::out | ios::app);
    out<< "initial_size, initial_cost, variables, operators \n";
    out<< original_plan.size() << "," <<  calculate_plan_cost(original_plan, blockDeorderPlan.task_proxy) <<
                   "," << blockDeorderPlan.task_proxy.get_variables().size() << "," << blockDeorderPlan.task_proxy.get_operators().size() << endl;
    out.close();
}


void add_deorder_algorithm_options_to_feature(
        plugins::Feature &feature, const string &description) {
    feature.add_option<string>(
            "plan",
            "",
            "sas_plan");
    feature.add_option<string>(
            "description",
            "description used to identify search algorithm in logs",
            "\"" + description + "\"");
//    utils::add_log_options_to_feature(feature);
}

tuple<string,string>
get_deorder_algorithm_arguments_from_options(
        const plugins::Options &opts) {
    return make_tuple(
                    opts.get<string>("plan"),
                    opts.get<string>("description")
            );
}

static class DeorderAlgorithmCategoryPlugin : public plugins::TypedCategoryPlugin<DeorderAlgorithm> {
public:
    DeorderAlgorithmCategoryPlugin() : TypedCategoryPlugin("DeorderAlgorithm") {
        // TODO: Replace add synopsis for the wiki page.
        // document_synopsis("...");
    }
}
_category_plugin;