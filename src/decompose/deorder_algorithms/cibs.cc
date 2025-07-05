//
// Created by sabah on 7/5/25.
//

#include "cibs.h"
#include "vector"
#include "map"
#include <iostream>


CIBS::CIBS(bool only_replace_block, bool compromise_flex, plan_reduction_type type)
        : BlockDeorder(), blockReplace(task_proxy, only_replace_block, compromise_flex, true),
          planReduction(type){
}

void CIBS::run_cibs() {
    initialize();
    restart_timer();
    write_problem_details();
    blockReplace.set_files_path(deorderManager.get_domain_file_path(), deorderManager.get_problem_file_path());

    set_plan(original_plan);
    blockDeorderPlan.concurrency = true;
    blockReplace.initialize_block_replace();
    do_step_deordering();
    write_eog_result();

    block_deorder_start();
    write_bd_result();
    plan_reduction();
    block_substitution_concurrency();
    plan_reduction();
    write_cibs_result();
}

void CIBS::plan_reduction() {
    blockDeorderPlan = planReduction.start(blockDeorderPlan);
}

void CIBS :: block_substitution_concurrency(){
    write_log_file("\n****** Block Substitution concurrency ********\n");

    auto remove =[&]() {
        for(auto relation : blockDeorderPlan.concurrent_orderings.orderings){
            int b_x = relation.first.first;
            int b_y = relation.first.second;
            bdpop = blockDeorderPlan;
            if(blockReplace.remove_nonconcurrency(b_x, b_y)){
                cout << "\n**new block has been replaced for nonconcurrency**" << endl;
                cout<< "!# action:" << b_x << " " << b_y <<endl;
                bdpop = blockDeorderPlan;
                return true;
            }
            bdpop = blockDeorderPlan;
            if(blockReplace.remove_nonconcurrency(b_y,b_x)){
                cout << "\n**new block has been replaced for nonconcurrency**" << endl;
                cout<< "!# action:" << b_y << " " << b_x <<endl;
                bdpop = blockDeorderPlan;
                return true;
            }

        }
        if(print_details){
            blockDeorderPlan.print_blocks_orderings();
            blockDeorderPlan.print_outer_blocks();
            cout << "********* no more block replacement possible.************\n";
            return false;
        }
        return false;
    };
    bool new_replacement_found = true;
    while (new_replacement_found && !timeout) {
        new_replacement_found = remove();
        blockDeorderPlan.calculate_flex();
    }

}


void CIBS::write_cibs_result() {
    blockDeorderPlan.calculate_flex(); /// calculating flex of the BDPO plan
    double time = deorderManager.get_time();
    Plan final_plan = blockDeorderPlan.get_total_order_plan();
    double cost = calculate_plan_cost(final_plan, blockDeorderPlan.task_proxy);
    double flex = blockDeorderPlan.flex[0];
    double cflex = blockDeorderPlan.flex[1];

    fstream fout;
    fout.open("cibs_out.csv", ios::out);    /// open result file

    fout << "cibs_flex, cibs_cflex, cibs_cost, cibs_time \n";
    fout << flex << "," << cflex << "," << cost << "," << time << endl;
    fout.close();
    write_in_file("step.txt", "\t<CIBS>" + to_string(flex) +
                              "\t" + to_string(cflex) + "\t" + to_string(cost) + "\t" + to_string(time) + "\n");


    blockDeorderPlan.write_block_deorder_plan("cibs.txt");
    blockDeorderPlan.print_bdop_dot_file(false, "cibs.dot");
}