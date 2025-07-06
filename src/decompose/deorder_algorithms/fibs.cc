//
// Created by sabah on ১২/৫/২১.
//

#include "../block_replace.h"
#include "vector"
#include "map"
#include <iostream>
#include "fibs.h"

FIBS::FIBS(bool only_replace_block, bool compromise_flex, bool concurrency, plan_reduction_type type)
        : BlockDeorder(), blockReplace(task_proxy, only_replace_block, compromise_flex, concurrency),
        planReduction(type){
    this->concurrency = concurrency;
}

void FIBS::run_fibs() {
    initialize();
    restart_timer();
    write_problem_details();
    blockReplace.set_files_path(deorderManager.get_domain_file_path(), deorderManager.get_problem_file_path());

    set_plan(original_plan);

    if(concurrency){
        blockDeorderPlan.concurrency = true;
        blockReplace.set_concurrency(true);
    }

    blockReplace.initialize_block_replace();
    do_step_deordering();
    write_eog_result();

    block_deorder_start();
    write_bd_result();
    plan_reduction();
    block_substitution_deorder();
    plan_reduction();
    write_fibs_result();
}

void FIBS::plan_reduction() {
    if (planReduction.type != NOREDUCTION)
        blockDeorderPlan = planReduction.start(blockDeorderPlan);
}


/// *******************************  BSD : Block Substitution Deordering **************************** ///
void FIBS :: block_substitution_deorder() {
    auto remove =[&]() {
        for (auto itr_i = blockDeorderPlan.blocks.begin(); itr_i != blockDeorderPlan.blocks.end(); itr_i++) {
            int i = itr_i->first;
            if (i == blockDeorderPlan.init_block || i == blockDeorderPlan.goal_block) continue;
            for (auto itr_j = blockDeorderPlan.blocks.begin(); itr_j != blockDeorderPlan.blocks.end(); itr_j++) {
                if (deorderManager.get_time() > max_time) {
                    timeout = true;
                    return false;
                }
                int j = itr_j->first;
                if (j == blockDeorderPlan.init_block || j == blockDeorderPlan.goal_block) continue;
                if (i == j || blockDeorderPlan.blocks_orderings.is_transitive(i, j) ||
                    !blockDeorderPlan.blocks_orderings.has_any_ordering(i, j))
                    continue;
                blockReplace.bdpop = blockDeorderPlan;
                if (blockReplace.remove_ordering_constraints_by_replacing_block(i, j)) {
                    cout << "\n**new block has been replaced**" << endl;
                    cout << "|| action:" << i << " " << j << endl;
                    blockDeorderPlan = blockReplace.bdpop;
                    return true;
                }
            }
        }
        return false;
    };
    bool new_replacement_found = true;
    write_log_file("\n******  Block Substitution Deorder Started ********\n");
    while (new_replacement_found && !timeout) {
        new_replacement_found = remove();
        blockDeorderPlan.calculate_flex();
    }
    write_log_file("\n******  Block Substitution Deorder completed ********\n");

}

void FIBS::write_fibs_result() {
    blockDeorderPlan.calculate_flex(); /// calculating flex of the BDPO plan
    double time = deorderManager.get_time();
    Plan final_plan = blockDeorderPlan.get_total_order_plan();
    double cost = calculate_plan_cost(final_plan, blockDeorderPlan.task_proxy);
    double flex = blockDeorderPlan.flex[0];
    double cflex = blockDeorderPlan.flex[1];

    fstream fout;
    fout.open("fibs_out.csv", ios::out);    /// open result file

    fout << "fibs_flex, fibs_cflex, fibs_cost, fibs_time \n";
    fout << flex << "," << cflex << "," << cost << "," << time << endl;
    fout.close();
    write_in_file("step.txt", "\t<FIBS>" + to_string(flex) +
                              "\t" + to_string(cflex) + "\t" + to_string(cost) + "\t" + to_string(time) + "\n");


    blockDeorderPlan.write_block_deorder_plan("fibs.txt");
    blockDeorderPlan.print_bdop_dot_file(false, "fibs.dot");
}