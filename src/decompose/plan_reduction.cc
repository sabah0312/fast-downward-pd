//
// Created by sabah on 6/12/25.
//

#include "plan_reduction.h"

bool PlanReduction :: is_redundant_by_backward_justification(int b_x){
    Block *block = bdpop.get_block(b_x);
    if(block->is_internal()) return false;
    if(!block->is_primary() && block->internal_blocks.empty())
        return true;
    return bdpop.get_associated_pc_constraints(b_x).empty();
}

void PlanReduction :: get_dependent_blocks(int b_x, vector<int> *dependent_blocks, vector<int> *visited){
    Block* block =  bdpop.get_block(b_x);
    if (count(visited->begin(), visited->end(), b_x) == 0){
        visited->push_back(b_x);
    }
    for (int successor: block->successors){
        if (!bdpop.blocks_orderings.have_constraint(b_x, successor, PC)) continue;
        if (successor == bdpop.goal_block) continue;
        if (count(visited->begin(), visited->end(), successor) == 0){
            get_dependent_blocks(successor, dependent_blocks, visited);
        }
        if (count(dependent_blocks->begin(), dependent_blocks->end(), successor) == 0){
            dependent_blocks->push_back(successor);
        }
    }
}

/**
* @brief checks every block. If a block is redundant, deletes it
*/
bool PlanReduction :: remove_redundant_block(){
    bool found_redundant = false;
    auto it = bdpop.blocks.begin();
    while (it != bdpop.blocks.end()){
        auto current  = it++;
        int id = current->second.id;
        if (id == bdpop.goal_block || id == bdpop.init_block ) continue;

        if(bdpop.print_details)
            write_log_file("checking redundancy for block " + to_string(id) + "\n");

        BlockDeorderPlan tempplan = bdpop;
        vector<int> dependent_blocks;
        vector<int> visited;
        get_dependent_blocks(id, &dependent_blocks, &visited);
        for(int i: dependent_blocks){
            tempplan.delete_block(i);

        }
        tempplan.delete_block(id);
        tempplan.find_transitive_ordering();
        if (tempplan.check_validity()) {
            write_log_file("** block " + to_string(id) +" is redundant.\n");
            write_log_file("\t\tdeleting blocks: ");
            for(int i: dependent_blocks){
                write_log_file(" "+ to_string(i));
            }
            write_log_file("\n");
            bdpop = tempplan;
            found_redundant = true;
            return true;
        }

    }
    return found_redundant;
}


bool PlanReduction :: remove_redundant_block_by_backward_justification(){
    write_log_file("\n****** Plan Reduction by Backward Justification Started ********\n");
    auto start = chrono:: high_resolution_clock::now();
    bool found_redundant = false;
    for (auto it = bdpop.blocks.begin(); it != bdpop.blocks.end();){
        auto current  = it++;
        int id = current->second.id;
        if (id == bdpop.goal_block ) continue;
        if (is_redundant_by_backward_justification(id)){
            write_log_file("** block " + to_string(id) +" is redundant.\n");
            bdpop.delete_block(id);
            found_redundant = true;
            return true;
        }
    }

    auto end = chrono:: high_resolution_clock::now();
    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);

    cout << "<t> delete redundant block : (wall clock: "<< duration.count() << " ms)" << endl;
    bdpop.find_transitive_ordering();
    write_log_file("\n****** Plan Reduction by Backward Justification Ended ********\n");
    return found_redundant;
}


void PlanReduction :: remove_redundant_by_forward_justification(){
    write_log_file("\n****** Plan Reduction by Forward Justification Started ********\n");
    auto start = chrono:: high_resolution_clock::now();

    bool redundant_removed = remove_redundant_block();
    while(redundant_removed){
        redundant_removed = remove_redundant_block();
    }
    auto end = chrono:: high_resolution_clock::now();
    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << "<t> attempt to delete redundant block : (wall clock: "<< duration.count() << " ms)" << endl;
    bdpop.find_transitive_ordering();
    write_log_file("\n****** Plan Reduction by Forward Justification Ended  (wall clock: " + to_string(duration.count()) + " ms)********\n");
}

BlockDeorderPlan PlanReduction::start(BlockDeorderPlan blockDeorderPlan) {
    if (type != NOREDUCTION) {
        bdpop = blockDeorderPlan;
        if (type == FJ)
            remove_redundant_by_forward_justification();
        else
            remove_redundant_block_by_backward_justification();
        return bdpop;
    }
}

void PlanReduction:: set_bdpo_plan (BlockDeorderPlan blockDeorderPlan){
    bdpop = blockDeorderPlan;
}

BlockDeorderPlan PlanReduction ::  get_bdpo_plan (){
    return bdpop;
}



static plugins::TypedEnumPlugin<plan_reduction_type> enum_plugin({
         {"bj", "plan reduction by backward justification"},
         {"fj", "plan reduction by forward justification"},
         {"noreduction", " no plan reduction"}
 });
