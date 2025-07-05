//
// Created by sabah on 3/14/25.
//

#ifndef FAST_DOWNWARD_PLAN_DECOMPOSE_H
#define FAST_DOWNWARD_PLAN_DECOMPOSE_Hi


#include "block_deorder_plan.h"
#include "myutils.h"

class PlanDecompose {


public:

    BlockDeorderPlan bdpop;
    bool is_agent(VariableProxy variableProxy){
//        write_log_file("variable " + variableProxy.get_fact(0).get_name()+"\n");
        causal_graph::CausalGraph cg =bdpop.task_proxy.get_causal_graph();
        vector<int> vars = cg.get_eff_to_pre(variableProxy.get_id());
        for(int i:vars){
//            write_log_file("\t" + bdpop.task_proxy.get_variables()[i].get_fact(0).get_name() + "\n");
            if( i != variableProxy.get_id())
                return false;
        }
        return true;
    }

vector<VariableProxy> agents;
    bool has_precondition(int b_x,int var){
        Block* block = bdpop.get_block(b_x);
        for(FactProxy factProxy: block->preconditions){
            if(factProxy.get_variable().get_id() == var)
                return true;
        }
        return false;
    }
    int select_var(Block* block){
        vector<int> vars;
        for(FactProxy factProxy: block->preconditions){
            int id = factProxy.get_variable().get_id();
            if(!is_in_vector(id, &vars))
                vars.push_back(id);
        }
        for(int var: vars){
            if(is_agent(bdpop.task_proxy.get_variables()[var]))
                return var;
        }
        return vars[0];
    }
    vector<int> add_block(int b_x, int var, map<int, bool> *visited){
        vector<int> queue;
        queue.push_back(b_x);
        vector<int> internals;
        internals.push_back(b_x);
        while (!queue.empty()) {
            int b = queue.back();
            visited->at(b)= true;
            queue.pop_back();
            Block *block = bdpop.get_block(b);
            for(int successor: block->successors) {
                if (successor == bdpop.goal_block) continue;
                if ( visited->at(successor) || bdpop.blocks_orderings.is_transitive(b, successor) ) continue;
                if(has_precondition(successor, var)){
                    internals.push_back(successor);
                    queue.push_back(successor);
                }
            }
        }
        return internals;
    }
    vector<vector<int>> decompose(){
        vector<vector<int>> decomposed_blocks;
        vector<VariableProxy> agents;
        for(VariableProxy var : bdpop.task_proxy.get_variables()){
            if (is_agent(var)){
                write_log_file("agent variable: " + to_string(var.get_id()) + var.get_fact(0).get_name() + "\n");
                agents.push_back(var);
            }
        }
        vector<int> queue;
        map<int, bool> visited = bdpop.get_boolean_map();
        queue.push_back(bdpop.init_block);
        visited.at(queue.back()) = true;
        int count = 1;
        while (!queue.empty()) {
            int b = queue.back();
            Block *block = bdpop.get_block(b);
            queue.pop_back();
            vector<int> successors = block->successors;
            for (int successor: successors) {
                if (successor == bdpop.goal_block || bdpop.blocks_orderings.is_transitive(b, successor) || visited.at(successor) ) continue;
                int var = select_var(bdpop.get_block(successor));
                vector<int> internals = add_block(successor, var, &visited);
                if (!internals.empty()) {
                    int new_b = bdpop.create_compound_block(internals);
                    bdpop.establish_extended_block_orderings(new_b, internals);
//                    bdpop.print_bdop_dot_file(false, "decompose-" + to_string(count++) + ".dot" );
                    visited.insert(pair<int, bool>(new_b, false));
                    bdpop.make_temporary_block_permanent(new_b);
                    write_log_file("<> decomposed on var " + bdpop.task_proxy.get_variables()[var].get_fact(0).get_name() + "\n");
                    decomposed_blocks.push_back(bdpop.get_block(new_b)->internal_blocks);
//                    bdpop.find_transitive_ordering();
//                    if(!bdpop.check_validity()) return;
                }
            }
        }
        bdpop.print_bdop_dot_file(false, "decomposed_plan.dot");
        return decomposed_blocks;

    }


};


#endif //FAST_DOWNWARD_PLAN_DECOMPOSE_H
