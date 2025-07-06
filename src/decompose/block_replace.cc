//
// Created by sabah on ১২/৫/২১.
//

#include "block_replace.h"
#include "vector"
#include "map"
#include <chrono>
#include <regex>
#include <iostream>
#include <dirent.h>
#include "../search/heuristics/domain_transition_graph.h"
#include "myutils.h"
#include "../search/tasks/root_task.h"

using namespace domain_transition_graph;

const successor_generator::SuccessorGenerator &get_successor_generator(TaskProxy proxy);

const successor_generator::SuccessorGenerator &get_successor_generator(TaskProxy proxy) {
    successor_generator::SuccessorGenerator &successor_generator =
            successor_generator::g_successor_generators[proxy];
    return successor_generator;
}

namespace block_replace {
    BlockReplace::BlockReplace(TaskProxy taskProxy, bool only_replace_block, bool compromise_flex, bool concurrency)
            :  state_registry(taskProxy),
               successor_generator(get_successor_generator(taskProxy)), subtask_initial_state(taskProxy.get_initial_state()){
        this->only_replace_block = only_replace_block;
        this->compromise_flex = compromise_flex;
        this->concurrency = concurrency;

        function<bool(int, int)> pruning_condition =
                [](int dtg_var, int cond_var) { return dtg_var <= cond_var; };
        DTGFactory factory(taskProxy, false, pruning_condition);
        transition_graphs = factory.build_dtgs();

    }

    void BlockReplace :: set_files_path(string domain_path, string problem_path){
        domain_file_path = domain_path;
        problem_file_path = problem_path;
    }

    void BlockReplace::initialize_block_replace() {
        store_predicate_names();
        if (concurrency) {
            clusterCausalGraph.compute_cluster();
            bdpop.clusterCausalGraph = &clusterCausalGraph;
        }
    }

    void BlockReplace :: set_concurrency (bool concurrency){
        this->concurrency = concurrency;
    }


    State BlockReplace :: get_successor(State state, int b_x)  {

        Block *block = bdpop[b_x];
        state.unpack();
        std::vector<int> new_values = state.get_unpacked_values();
        for (EffectProxy effectProxy : block->get_effects()) {
            new_values[effectProxy.get_fact().get_variable().get_id()] = effectProxy.get_fact().get_value();
        }

        State s = State(*tasks::g_root_task, std::move(new_values));

        return s;
    }


    /// This attempts to remove a non-concurrency constraint between block b_x and b_y
    /// \return
    bool BlockReplace :: remove_nonconcurrency(int b_x, int b_y){
        write_log_file( "check B:" + to_string(b_x) + "-> B:" + to_string(b_y) + " --- ");
        write_log_file("\tcheck " + to_string(b_x) +"\n");
        vector<int> conflicted_variables = bdpop.is_concurrent(b_x, b_y);

        if(conflicted_variables.empty())
            return true;

        set_initial_state_and_goals_for_subtask(b_x, false);
        int extended_bx = extend_block(b_x,b_y,conflicted_variables);
        write_log_file( "extended block:" + to_string(extended_bx) + "\n");

        if (!bdpop[extended_bx]->is_primary())
            return replace_block_by_block_for_nonconcurrency(extended_bx, b_y);
        else
            return replace_block_by_operators_for_nonconcurrency( b_x, b_y);

    }

    /////////////// Start of block Extention Functions ///////////////////

/// extends the block b_x to with it neighbours to allow substitution for concurrency
/// \return
    int BlockReplace :: extend_block(int b_x, int b_y, vector<int> conflicted_variables){


        set_initial_state_and_goals_for_subtask(b_x, false);
        Block* block_x = bdpop.get_block(b_x);
        vector<int> extensions;
        vector<int> predecessors = block_x->predecessors;
        vector<int> successors = block_x->successors;
        write_log_file("extending block " + to_string(b_x) + " with ");
        for(int p: predecessors){
            if (p == bdpop.init_block || bdpop.blocks_orderings.is_transitive(p, b_x)) continue;
            if(bdpop.blocks_orderings.has_any_ordering(p, b_y)) continue;
            if(!check_extension_candidency(p, b_x, conflicted_variables, true)) continue;
            extensions.push_back(p);
//            write_log_file( to_string(p) + " ");

        }
        for(int s: successors){
            if (s == bdpop.goal_block || bdpop.blocks_orderings.is_transitive( b_x, s)) continue;
            if(bdpop.blocks_orderings.has_any_ordering( b_y, s)) continue;
            if(!check_extension_candidency(b_x, s, conflicted_variables, false)) continue;
            extensions.push_back(s);
//            write_log_file( to_string(s) + " ");
        }
        if(extensions.empty())
            return b_x;
//        write_log_file("\n");
        extensions.push_back(b_x);
        int new_block_id = bdpop.create_compound_block(extensions);
        for(int b: extensions){
            bdpop.get_block(b)->parent = new_block_id;
        }
        bdpop.establish_extended_block_orderings(new_block_id, extensions);
        bdpop.find_transitive_ordering();
        set_initial_state_and_goals_for_subtask(new_block_id, false);
        return extend_block(new_block_id, b_y, conflicted_variables);
    }

    bool BlockReplace :: check_extension_candidency (int b_x, int b_y, vector<int> conflicted_variables, bool up){
        for (OrderingConstraint constraint: bdpop.blocks_orderings.get_constraints(b_x, b_y)){
            if (constraint.type == PC) {
                FactProxy start = constraint.factProxy;
                int var= start.get_variable().get_id();
                if(up){
                    for(FactProxy factProxy : subtask_goals){
                        if(factProxy.get_variable().get_id() == var){
                            if(!check_alternative_path(var, start.get_value(), factProxy.get_value(), conflicted_variables)){
                                return true;
                            }
                            break;
                        }
                    }
                } else{
                    for(FactProxy factProxy : subtask_initial_state){
                        if(factProxy.get_variable().get_id() == var){
                            if(!check_alternative_path(var, factProxy.get_value(), start.get_value(), conflicted_variables)){
                                return true;
                            }
                            break;
                        }
                    }
                }
            }
        }
        return false;
    }

    bool BlockReplace :: check_alternative_path(int var, int start_value, int end_value, vector<int> except_var){
//    write_log_file("check path for " + to_string(var) + " from " + to_string(start_value) + " to " + to_string(end_value) + "\n");
        domain_transition_graph::DomainTransitionGraph *dtg = transition_graphs[var].get();
        ValueNode *start_node = &dtg->nodes[start_value];
        int size = dtg->nodes.size();
        vector<int> path;
        vector<int> queue;
        vector<bool> visited(size);
        fill(visited.begin(), visited.end(), false);
        queue.push_back(start_value);
        while (!queue.empty()){
            int a = queue.front();
//        write_log_file("search " + to_string(a) + "\n");
            visited.at(a) = true;
            ValueNode* node_a = &dtg->nodes[a];
            queue.erase(queue.begin());
            for(int i =0; i< node_a->transitions.size();i++){
                ValueTransition *transition_node = &node_a->transitions.at(i);
                int new_child = transition_node->target->value;
                if(is_in_vector(new_child, &queue) || visited.at(new_child))
                    continue;
//            write_log_file("visit :"+ to_string(new_child) + "\n");
                transition_node->target->reached_from= node_a;
                if(new_child == end_value){
//                write_log_file("found\n");
                    if(check_path(start_node, transition_node->target, except_var))
                        return true;
                    else
                        continue;
                }
                queue.push_back(new_child);
            }
        }
//    write_log_file("no alternative path\n");
        return false;
    }


    bool BlockReplace ::  check_path(ValueNode* start_node, ValueNode* end_node, vector<int> except_var){
        ValueNode *curr = end_node;
        //cout << "found";
        bool alternative_path = true;
//    write_log_file("\tget_path " + to_string(start_node->value) + " " + to_string(end_node->value) + "\n");
        while(curr->value !=start_node->value){
            ValueNode* curr_next = curr->reached_from;
//        write_log_file("\t\tget_ops " + to_string(curr_next->value) + " " + to_string(curr->value) + "\n");
            vector<int> ops = get_operators(curr_next, curr);
            if(ops.empty())
                return false;
            bool if_any = false;
            for(int op: ops){
                OperatorProxy operatorProxy = bdpop.task_proxy.get_operators()[op];
                bool have_except_var = false;
                for(FactProxy factProxy: operatorProxy.get_preconditions()){
                    if(is_in_vector(factProxy.get_variable().get_id(), &except_var)){
                        have_except_var = true;
                        break;
                    }
                }
                if(!have_except_var)
                    if_any = true;
//            write_log_file("op :" + to_string(op) + ":" +task_proxy.get_operators()[op].get_name() + " ");
            }
            if(!if_any)
                return false;
//        write_log_file("\n");
            curr = curr_next;
        }
        return alternative_path;
    }

    vector<int> BlockReplace ::  get_operators(ValueNode* start_node, ValueNode* end_node){
        vector<int> ops;
        for(auto transition : start_node->transitions) {
            if(transition.target->value == end_node->value){
                for(auto label :transition.labels){
                    ops.push_back(label.op_id);
                }
            }
        }
        return ops;
    }
    /////////////// End of block Extention Functions ///////////////////

    /// This sets initial state and goals for subtask to substitute for concurrency
    void BlockReplace :: set_initial_state_and_goals_for_subtask(int b_x, bool allow_parallel){
        const State &initial_state = state_registry.get_initial_state();
        subtask_initial_state= get_initial_state_for_subtask(b_x, initial_state, vector<int>(0), allow_parallel);

        map<int, int> connected = bdpop.get_connection_map(b_x);
        subtask_goals = bdpop.get_causal_link_facts(b_x, vector<int>(0));


        get_goals(bdpop.get_block(b_x));

        vector<FactProxy> crossing_facts = get_crossing_facts(b_x, &connected);
        for(FactProxy factProxy: crossing_facts){
            add_to_goals(factProxy);
//        subtask_goals.push_back(factProxy);
        }
    }

/// Get the Facts that block x contribute to the outer blocks when x is an internal block
    void BlockReplace :: get_goals(Block* block_x){
        if (block_x->is_outer())
            return;
        Block* block_parent = bdpop.get_block(block_x->parent);
        vector<FactProxy> facts = bdpop.get_causal_link_facts(block_parent->id, vector<int>(0));
        for(FactProxy f: facts){
            if(count(subtask_goals.begin(), subtask_goals.end(), f) == 0 &&
               bdpop.is_producer_in_block(block_x, block_parent, f))
                add_to_goals(f);
        }
    }

    /// add a fact to the goals
    void BlockReplace :: add_to_goals(FactProxy factProxy){
        for(FactProxy f: subtask_goals){
            if(f.get_variable().get_id() == factProxy.get_variable().get_id()){
                continue;
                if(f.get_value()== factProxy.get_value()) return;
                else{
                    write_log_file( "inconsistent goals!!!\n");
                    exit(1);
                }
            }
        }
        subtask_goals.push_back(factProxy);
    }

//    bool BlockReplace::remove_ordering_constraints_by_replacing_block(int b_x, int b_y) {
//
//        map<string, vector<FactProxy>> constraints;
//        vector<FactProxy> facts;
//
//        constraints.insert(pair<string, vector<FactProxy>>("pc", facts));
//        constraints.insert(pair<string, vector<FactProxy>>("cd", facts));
//        constraints.insert(pair<string, vector<FactProxy>>("dp", facts));
//
//
//        for (OrderingConstraint constraint: blockDeorderPlan.blocks_orderings.get_constraints(b_x, b_y)) {
//            if (constraint.type == PC) constraints.at("pc").push_back(constraint.factProxy);
//            else if (constraint.type == CD) constraints.at("cd").push_back(constraint.factProxy);
//            else if (constraint.type == DP) constraints.at("dp").push_back(constraint.factProxy);
//        }
//        write_log_file(
//                "check B:" + to_string(b_x) + "-> B:" + to_string(b_y) + " --- " + "\tcheck " + to_string(b_y) + "\n");
//
//
//        if (remove_constraints_by_replacing_second_block(b_x, b_y, &constraints)) {
//            return true;
//        }
//
//        return false;
//    }

 ///This function attempts to remove ordering constraints between blocks b_x and b_y by substituting b_y
    bool BlockReplace::remove_ordering_constraints_by_replacing_block(int b_x, int b_y) {
        write_log_file("\ttry replacing by block " + to_string(b_x) + " " + to_string(b_y) + "\n");

        map<int, int> connected = bdpop.get_connection_map(b_y);

        vector<FactProxy> constraints = bdpop.get_causal_link_facts(b_y, vector<int>(0));

        const State &initial_state = state_registry.get_initial_state();
        initial_state.unpack();
        vector<int> exclude;
        exclude.push_back(b_x);

        State s = get_initial_state_for_subtask(b_y, initial_state, exclude, false);

        connected.at(b_x) = 0;

        if (!bdpop[b_y]->is_primary()) {
            connected.at(b_x) = 1;
            return replace_block_by_block_to_remove_ordering(b_y, b_x, b_y, s, constraints, &connected);
            return false;
        } else if (!only_replace_block) {
            return replace_block_by_operators_to_remove_ordering(b_y, b_x, b_y, s, constraints, &connected);
        }

        return false;
    }

    bool BlockReplace::replace_block_by_operators_to_remove_ordering(int b, int b_x, int b_y, State s,
                                                                     vector<FactProxy> constraints,
                                                                     map<int, int> *connected) {

//    cout << "\t try replacing by operators "<< endl;
        auto start = chrono::high_resolution_clock::now();

        Block *old_block = bdpop[b];
        OperatorProxy old_op = bdpop.task_proxy.get_operators()[old_block->op_index];
        int connected_block = b_x;
        if (b == b_x) connected_block = b_y;

        vector<FactProxy> goals_proxy = constraints;
        get_goals(old_block, &goals_proxy);

        vector<FactProxy> crossing_facts = get_crossing_facts(b, connected);
        vector<FactPair> goals;
        if (print_details) {
            string str;
            str = "\t Goals: ";
            for (FactProxy factProxy: goals_proxy) {
                str += bdpop.print_fact(factProxy);
            }
            str += "\n";

            str += "\t Crossing facts: ";
            for (FactProxy factProxy: crossing_facts) {
//            cout << "\t";
                str += "<" + to_string(factProxy.get_variable().get_id()) + ":" + to_string(factProxy.get_value()) +
                       ">, ";
            }
            str += "\n";
//            write_log_file(str);
        }

        vector<OperatorID> applicable_ops;
        successor_generator.generate_applicable_ops(s, applicable_ops);

        for (OperatorID operatorId: applicable_ops) {
            OperatorProxy operatorProxy_new = bdpop.task_proxy.get_operators()[operatorId];

            if (old_block->op_index == operatorId) continue;
            if (operatorProxy_new.get_name() == "init_action") continue;
            int max_cost = old_op.get_cost() * (100 + cost_compromise) / 100;
            if (operatorProxy_new.get_cost() > max_cost) continue;   /// checking cost
//        if(print_details)
            write_log_file("\ttry another operator " + operatorProxy_new.get_name() + "\n");

            bool has_goals = true;
            for (FactProxy f: goals_proxy) {
                has_goals = false;
                for (EffectProxy effectProxy: operatorProxy_new.get_effects()) {
                    if (is_equal(effectProxy.get_fact(), f)) {
                        has_goals = true;
                    }
                }
                if (!has_goals) break;
            }
            if (!has_goals) continue;
            bool is_deleter = false;
            for (FactProxy f: crossing_facts) {
                if (bdpop.is_deleter_op(operatorId.get_index(), f)) {
                    is_deleter = true;
                    break;
                }
            }

            if (is_deleter) continue;

            if (check_cd(operatorId, connected_block) ||
                check_dp(operatorId, connected_block, constraints))
                continue;
            write_log_file("\treplacing the block " + to_string(old_block->id) + " " +
                           bdpop.task_proxy.get_operators()[old_block->op_index].get_name() + "\ttry another operator " +
                           operatorProxy_new.get_name() + "\n");
            /// new bdpop construction
            BlockDeorderPlan newPlan = bdpop;

            Plan plan;
            plan.push_back(operatorId);

            Block *old_block = newPlan.get_block(b);
            old_block->type = TEMP_REMOVED;
            Block *new_block = make_block(plan, newPlan, goals_proxy);

            new_block->parent = old_block->parent;
            if (old_block->is_internal())
                newPlan.get_block(new_block->parent)->internal_blocks.push_back(new_block->id);

            if (print_details)
                new_block->print_preconditions_and_effects();

            int b_r = get_replacing_block(b, b_x, b_y);
            if (establish_new_block(newPlan, b, b_r, new_block->id, false)) {
                if (!newPlan.check_validity()) {
                    write_log_file("not valid\n");
//                newPlan.print_bdop_dot_file(false,   "invalid.dot");
                    newPlan.print_blocks_orderings();
                    exit(1);
                }
                newPlan.calculate_flex();
                if (bdpop.flex[0] < newPlan.flex[0] ||
                    (compromise_flex && newPlan.calculate_plan_cost() < bdpop.calculate_plan_cost())) {
                    newPlan.find_transitive_ordering();
                    bdpop = newPlan;
                    auto end = chrono::high_resolution_clock::now();
                    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    write_log_file("block " + to_string(b) +
                                   " has been replaced by action replacement\n\t<t> time for replacement attempt :"
                                   + to_string(duration.count()) + " ms\n");
                    return true;
                }
                write_log_file("less flexibility\n");
            }
        }
//     auto end = chrono:: high_resolution_clock::now();
//     auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
//     cout << "<t> time for operator replacement attempt : "<< duration.count() << " ms" << endl;
        return false;
    }


    vector<FactProxy> BlockReplace::get_crossing_facts(int b_x, const map<int, int> *connected) {
        vector<FactProxy> crossing_facts;
        for (auto itr = connected->begin(); itr != connected->end(); itr++) {
            if (itr->second == 1) {
                int b = itr->first;
                Block *block = bdpop.get_block(b);
                for (int successor: block->successors) {
                    if (connected->at(successor) != 2) continue;
                    for (auto constraint: bdpop.blocks_orderings.get_constraints(b, successor)) {
                        if (constraint.type == PC) crossing_facts.push_back(constraint.factProxy);
                    }
                }
            }

        }
        return crossing_facts;
    }


    bool BlockReplace::replace_block_by_block_to_remove_ordering(int b, int b_x, int b_y, State s, vector<FactProxy> constraints,
                                                                 map<int, int> *connected) {

        write_log_file("\t try replacing by block \n");
        auto start = chrono::high_resolution_clock::now();

        Block *old_block = bdpop.get_block(b);
        vector<FactProxy> goals_proxy = constraints;
        get_goals(old_block, &goals_proxy);

        vector<FactProxy> crossing_facts = get_crossing_facts(b, connected);
        for (FactProxy factProxy: crossing_facts) {
            add_to_goals(factProxy, &goals_proxy);
        }

        vector<FactPair> goals;
        for (FactProxy factProxy: goals_proxy) {
            goals.push_back(factProxy.get_pair());
        }

        int block_cost = bdpop.cost(b);
        int max_cost = block_cost * (100 + cost_compromise) / 100;

        if (print_details) {
            string str;
            str = "\t Initial State: ";
            for (int i = 0; i < s.get_unpacked_values().size(); i++) {
                str = str + "\t<" + to_string(i) + "," + to_string(s.get_unpacked_values()[i]) + ">" +
                      bdpop.task_proxy.get_variables()[i].get_fact(s.get_unpacked_values()[i]).get_name() + "\n";
            }
            str += "\n\t Goals: ";
            for (FactProxy factProxy: goals_proxy) {
                str += "<" + to_string(factProxy.get_variable().get_id()) + ":" + to_string(factProxy.get_value()) +
                       ">, ";
            }
            str += "\n";
            write_log_file(str);
        }

        write_problem(s, goals_proxy, vector<int>(0));
        ostringstream com;
        string problem_path = "problem.txt";
        com<< "(ulimit -t 60; python3 " << planner_path << " " << domain_file_path << " " << problem_path << ")";
        system(com.str().c_str());

        vector<string> planfiles;

        DIR *dr;
        struct dirent *en;
        dr = opendir("."); //open all directory
        if (dr) {
            while ((en = readdir(dr)) != NULL) {
                string name = en->d_name;
                if (name.find("sas_plan") == 0) {
                    planfiles.push_back(name);
                }
            }
            closedir(dr); //close all directory
        }


        for (string s: planfiles) {
            write_log_file("try another " + s + "\n");
            Plan plan = bdpop.read_sequential_plan(s);
            if (plan.size() <= 2) continue;
            int plan_cost = bdpop.get_plan_cost(plan);

            if (plan_cost > max_cost) continue;
            for (OperatorID op: plan) {
                write_log_file("\n\t\t" + bdpop.task_proxy.get_operators()[op].get_name() + "\n");
            }

            ///constructing bdpop
            BlockDeorderPlan newPlan = bdpop;
            old_block = newPlan.get_block(b);
            old_block->type = TEMP_REMOVED;
            Block *new_block = make_block(plan, newPlan, goals_proxy);
            if (print_details)
                new_block->print_preconditions_and_effects();
            new_block->parent = old_block->parent;
            if (new_block->is_internal()) {
                newPlan.get_block(new_block->parent)->internal_blocks.push_back(new_block->id);
            }
            int b_r = get_replacing_block(b, b_x, b_y);

//        newPlan.print_blocks_orderings();
            if (establish_new_block(newPlan, b, b_r, new_block->id, true)) {
                if (!newPlan.check_validity()) {
                    write_log_file("not valid\n");
                    newPlan.print_bdop_dot_file(false, "invalid.dot");
//                newPlan.print_blocks_orderings();
                    continue;
                }
                newPlan.calculate_flex();
//            if(newPlan.calculate_plan_cost() < bdpop.calculate_plan_cost() || bdpop.flex[0] < newPlan.flex[0]) {
                if (bdpop.flex[0] < newPlan.flex[0] ||
                    (compromise_flex && newPlan.calculate_plan_cost() < bdpop.calculate_plan_cost())) {
                    newPlan.find_transitive_ordering();
                    bdpop = newPlan;
                    auto end = chrono::high_resolution_clock::now();
                    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
                    write_log_file("block " + to_string(b) +
                                   " has been replaced by block replacement\n\t<t> time for replacement attempt :"
                                   + to_string(duration.count()) + " ms\n");
                    return true;
                }
                write_log_file("rejected for less flexibility \n");
            }
        }

        for (string s: planfiles) {
            remove(s.c_str());
        }
        return false;
    }

bool BlockReplace :: replace_block_by_block_for_nonconcurrency(int b_x, int b_y){

    vector<int> conflicted_vars = bdpop.is_concurrent(b_x, b_y);
    map<int, int> connected = bdpop.get_connection_map(b_x);
    write_log_file("\t try replacing by block \n");
    auto start = chrono:: high_resolution_clock::now();

    Block* block_x = bdpop.get_block(b_x);

    vector<FactPair> goals;
    for(FactProxy factProxy: subtask_goals){
        goals.push_back(factProxy.get_pair());
    }

    int block_cost = bdpop.cost(b_x);
    int max_cost = block_cost * (100+cost_compromise)/100;

    if(print_details) {
        string str;
        str = "\t Initial State: ";
        for (int i = 0; i< subtask_initial_state.get_unpacked_values().size() ; i++){
            str = str + "\t<" + to_string( i) + "," + to_string(subtask_initial_state.get_unpacked_values()[i]) + ">" +
                  bdpop.task_proxy.get_variables()[i].get_fact(subtask_initial_state.get_unpacked_values()[i]).get_name() + "\n";
        }
        str += "\n\t Goals: ";
        for (FactProxy factProxy: subtask_goals) {
            str += "<" + to_string(factProxy.get_variable().get_id()) + ":" + to_string(factProxy.get_value()) + ">, ";
        }
        str += "\n";
        write_log_file(str);
    }

    write_problem(subtask_initial_state, subtask_goals, conflicted_vars);
    ostringstream com;
    string problem_path = "problem.txt";
    com << "(ulimit -t 60; python3 " << planner_path << " " << domain_file_path << " " << problem_path << ")";
    system(com.str().c_str());

    vector<string> planfiles;

    DIR *dr;
    struct dirent *en;
    dr = opendir("."); //open all directory
    if (dr) {
        while ((en = readdir(dr)) != NULL) {
            string  name = en->d_name;
            if ( name.find("sas_plan") == 0 )
            {
                planfiles.push_back(name);
            }
        }
        closedir(dr); //close all directory
    }


    for(string s: planfiles){
        write_log_file( "try another " + s + "\n");
        Plan  plan = bdpop.read_sequential_plan(s);
        if(plan.size() < 1 ) continue;
        int plan_cost = bdpop.get_plan_cost(plan);

        if(plan_cost > max_cost) continue;
        for (OperatorID op: plan){
            write_log_file("\n\t\t" + bdpop.task_proxy.get_operators()[op].get_name() + "\n");
        }

        ///constructing bdpop
        BlockDeorderPlan newPlan = bdpop;
        block_x->type = TEMP_REMOVED;
        Block* new_block = make_block( plan, newPlan, subtask_goals);
        vector<int> conflicted_variables = newPlan.is_concurrent(b_y, new_block->id);
        if(conflicted_variables.size() > 0){
            continue;
        }

        if (print_details)
            new_block->print_preconditions_and_effects();
        new_block->parent = block_x->parent;
        if(new_block->is_internal()){
            newPlan.get_block(new_block->parent)->internal_blocks.push_back(new_block->id);
        }

        if(establish_new_block(newPlan,b_x,b_y,new_block->id, true)){

            newPlan.find_transitive_ordering();
            newPlan.find_concurrent_actions();

            if(!newPlan.check_validity()){
                newPlan.print_bdop_dot_file(false,  "invalid.dot");
                continue;
            }
            newPlan.calculate_flex();
            if(bdpop.flex[1] < newPlan.flex[1] || (compromise_flex && newPlan.calculate_plan_cost() < bdpop.calculate_plan_cost())) {
                bdpop = newPlan;
                auto end = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
                write_log_file("block " + to_string(b_x) + " has been replaced by block replacement\n\t<t> time for replacement attempt :"
                               + to_string(duration.count()) + " ms\n");
                return true;
            }
            write_log_file( "rejected for less flexibility \n");
        }
    }

    for(string s: planfiles){
        remove(s.c_str());
    }
    return false;
}

    bool BlockReplace::establish_new_block(BlockDeorderPlan &newPlan, int b, int b_r, int new_b, bool check) {

        newPlan.get_block(b)->print_predecessors_and_successors();
        Block *new_block = newPlan.get_block(new_b);
        if (establish_connection_for_replacing_block(newPlan, b, b_r, new_block->id)) {

            newPlan.find_transitive_ordering();
            if (new_block->is_internal()) {
                int parent = new_block->parent;
                Block *old_parent = newPlan.get_block(parent);
                Block new_parent = newPlan.blocks.at(new_block->parent);
                map<int, int> connected = newPlan.get_connection_map(parent);
                vector<FactProxy> crossing_facts = get_crossing_facts(b, &connected);
                new_parent.preconditions = newPlan.find_preconditions(new_parent.internal_blocks);
                new_parent.effects = newPlan.find_effects(new_parent.internal_blocks);

                int new_id = newPlan.get_new_block_id();
                new_parent.id = new_id;
                Block *new_p = newPlan.insert(new_parent);
                old_parent->type = TEMP_REMOVED;
                for (int i: new_parent.internal_blocks) {
                    newPlan.get_block(i)->parent = new_id;
                }
                if (new_parent.is_internal()) {
                    newPlan.get_block(new_parent.parent)->internal_blocks.push_back(new_parent.id);
                    delete_element_from_vector(&newPlan.get_block(old_parent->parent)->internal_blocks, parent);
                }
                for (FactProxy f: crossing_facts) {
                    if (newPlan.is_deleter(new_id, f))
                        return false;
                }
                old_parent->internal_blocks.clear();
                if (!establish_new_block(newPlan, parent, -1, new_id, false)) return false;
            }
            return true;
        }

        return false;
    }


    void BlockReplace::get_goals(Block *block_x, vector<FactProxy> *goals) {
        if (block_x->is_outer())
            return;
        Block *block_parent = bdpop.get_block(block_x->parent);
        vector<FactProxy> facts = bdpop.get_causal_link_facts(block_parent->id, vector<int>(0));
        for (FactProxy f: facts) {
            if (count(goals->begin(), goals->end(), f) == 0 &&
                bdpop.is_producer_in_block(block_x, block_parent, f))
                add_to_goals(f, goals);
        }
    }

    void BlockReplace::add_to_goals(FactProxy factProxy, vector<FactProxy> *goals) {
        for (auto itr = goals->begin(); itr != goals->end(); itr++) {
            if (itr->get_pair().var == factProxy.get_pair().var) {
                if (itr->get_value() == factProxy.get_value()) return;
                else {
                    write_log_file("inconsistent goals!!!\n");
                    exit(1);
                }
            }
        }
        goals->push_back(factProxy);
    }

    Block *BlockReplace::make_block(Plan plan, BlockDeorderPlan &newPlan, vector<FactProxy> goals) {
//    cout << "here";
//    if(1)
//        cout<<"here";
        write_log_file("\tmaking a new block from a bdpop");
        int block_id = newPlan.get_new_block_id();
        if (plan.size() == 1) {              /// if primitive block
            write_log_file("\tprimitive plan");
            OperatorProxy operatorProxy = bdpop.task_proxy.get_operators()[plan[0]];
            Block internal_block(block_id, plan[0], vector<int>(0), REGULAR, false, block_id);
            Block *internal = newPlan.insert(internal_block);
            for (FactProxy factProxy: operatorProxy.get_preconditions())
                internal->preconditions.push_back(factProxy);
            for (EffectProxy effectProxy: operatorProxy.get_effects())
                internal->effects.push_back(effectProxy);
            internal->print_preconditions_and_effects();
            return internal;

        } else {                        /// if compound block
            cout << "here";
//        write_log_file( "\tcompound plan");

            block_deorder.reset_plan(plan);
            BlockDeorderPlan *newDplan = &block_deorder.blockDeorderPlan;
            int goal_id = newDplan->get_new_block_id();
//            write_log_file(to_string(goal_id));
            Block goal_block(goal_id, OperatorID(-1));
            for (FactProxy f: goals) {
                goal_block.preconditions.push_back(f);
            }
            newDplan->insert(goal_block);
            newDplan->goal_block = goal_id;


            block_deorder.do_step_deordering();
//            write_log_file("\tcompound plan");

            Block block(block_id, vector<int>(0), false);
            Block *parent = newPlan.insert(block);

//        newDplan->print_blocks_orderings();
            for (auto itr: newDplan->blocks) {
                if (itr.first == newDplan->goal_block) continue;
                int id = newPlan.get_new_block_id();
                Block internal(itr.second);
                internal.id = id;
                internal.type = REGULAR;
                internal.is_temporary = false;
                internal.predecessors.clear();
                internal.successors.clear();
                internal.parent = parent->id;
                newPlan.insert(internal);
                parent->internal_blocks.push_back(id);

//            internal.print();
//            internal.print_preconditions_and_effects();

            }

            for (int i = 0; i < parent->internal_blocks.size(); i++) {
                for (int j = 0; j < i; j++) {
                    int x = parent->internal_blocks[j];
                    int y = parent->internal_blocks[i];
                    if (newDplan->blocks_orderings.have_ordering(j, i)) {
                        Ordering *ordering = newDplan->blocks_orderings.get_ordering(j, i);
                        for (OrderingConstraint constraint: ordering->orderingConstraints) {
                            newPlan.add_constraint(x, y, constraint);
                            newPlan.blocks_orderings.set_transitive(x, y, ordering->is_transitive);
                        }

                    }

                }
            }
            parent->effects = newPlan.find_effects(parent->internal_blocks);
            parent->preconditions = newPlan.find_preconditions(parent->internal_blocks);

            if (print_details) {
                parent->print();
                parent->print_preconditions_and_effects();
            }
            return parent;
        }
    }


    /**
     * @brief try establish the causal links , cd, dp connection for the replacing block/operator, also check the existing
     *      connection
     */
    bool
    BlockReplace::establish_connection_for_replacing_block(BlockDeorderPlan &newPlan, int b_x, int b_r, int new_b) {

        Block *old_block = newPlan[b_x];
        Block *new_block = newPlan[new_b];
        write_log_file(
                "\testablishing connection for replacing block " + to_string(new_b) + " for block " + to_string(b_x) +
                "\n");
        auto start = chrono::high_resolution_clock::now();

        if (print_details)
            write_log_file("\t-establishing links to new block's precondition\n");
        vector<int> exclude;
        if (b_r >= 0)
            exclude.push_back(b_r);

        ///*****************     establishing causal links for preconditions of the replacing block ****************************************** ///
        for (FactProxy factProxy1: new_block->get_preconditions()) {
            if (print_details)
                write_log_file("\t check precondition " + to_string(factProxy1.get_variable().get_id()) + ":" +
                               to_string(factProxy1.get_value()) + "\n");

            int producer = newPlan.find_earliest_candidate_producer(b_x, factProxy1, exclude);
            if (producer >= 0)
                add_replacement_orderings(newPlan, producer, new_b, factProxy1, PC);

            if (producer == -1 && new_block->is_internal()) {
                Block *parent = newPlan.get_block(new_block->parent);
                while (1) {
                    producer = newPlan.find_earliest_candidate_producer(parent->id, factProxy1, exclude);
                    if (producer >= 0) break;
                    if (producer == -1 || parent->is_internal()) {
                        parent = newPlan.get_block(parent->parent);
                    } else if (producer == -2) break;
                }
            }

            if (producer == -2) {
                assert("no producer found!!!!");
                return false;
            }
        }

        /// **************************************** establishing causal links with the blocks to whom the substituted block provides atoms **************** ///
        if (print_details)
            write_log_file("\t-establishing causal link\n");
        for (int i: old_block->successors) {
            if (i == b_r) continue;
            for (OrderingConstraint constraint: newPlan.blocks_orderings.get_constraints(b_x, i)) {
                if (constraint.type == PC) {
                    int producer = new_b;
                    if (!newPlan.is_producer(new_b, constraint.factProxy)) {
                        if (newPlan.is_consumer(new_b, constraint.factProxy)) {
                            producer = newPlan.get_producer(new_b, constraint.factProxy);
                        } else {
                            producer = newPlan.find_earliest_candidate_producer(i, constraint.factProxy, exclude);
                        }
                        if (producer < 0) {
                            write_log_file(
                                    "no producer found" + bdpop.print_fact(constraint.factProxy) + "\n");
                            return false;
                        }

                    }
                    add_replacement_orderings(newPlan, producer, i, constraint.factProxy, PC);
                }
            }
        }

        ///*****************************       establishing cd dp connection with predecessors and successors ************************ ///
        cout << "\t-establishing cd dp connection with predecessors\n";

        map<int, bool> visited = newPlan.get_boolean_map();
        resolve_threat_with_predecessors(newPlan, new_b, new_b, &visited);
        write_log_file("\t-establishing cd dp connection with successors\n");
        resolve_threat_with_successors(newPlan, new_b, new_b, &visited);
        visited.at(new_b) = true;

        newPlan.delete_block(b_x);
        ///*****************************       establishing cd dp connection with unordered blocks ************************************ ///
        map<int, int> new_connected = newPlan.get_connection_map(new_b);
        write_log_file("\t-establishing cd dp connection with parallels\n");
        for (auto itr: new_connected) {
            if (itr.second == 0) {
                int b = itr.first;
                if (visited.at(b)) continue;

                if (check_conflict(newPlan, new_b, b)) {
                    if (!replace_conflicting_block(newPlan, b, new_b, &visited))
                        return false;
                } else {
                    if (establish_cd_dp_connection(newPlan, new_b, b)) {
                        resolve_threat_with_successors(newPlan, new_b, b, &visited);
                    } else {
                        if (establish_cd_dp_connection(newPlan, b, new_b)) {
                            resolve_threat_with_predecessors(newPlan, new_b, b, &visited);

                        }
                    }
                }
                visited.at(b) = true;
            }
        }

        ///*********************** Checking crossing facts ****************************** ///
        vector<int> deleted_conflicted_blocks;
//    vector<OrderingConstraint> crossing_conflicted_facts;
        map<int, int> connected = newPlan.get_connection_map(new_b);
        write_log_file("checking crossing factors\n ");
        for (auto itr: connected) {
            if (itr.second == 1) {
                int b = itr.first;
//            cout << "checking "<< b << endl;
                Block *block = newPlan.get_block(b);
                for (int successor: block->successors) {
                    if (connected.at(successor) != 2) continue;
                    for (auto constraint: newPlan.blocks_orderings.get_constraints(b, successor)) {
                        if (constraint.type == PC && new_block->is_deleter(constraint.factProxy)) {
                            if (!is_in_vector(successor, &deleted_conflicted_blocks)) {
                                deleted_conflicted_blocks.push_back(successor);
                            }

                        }
                    }
                }
            } else if (new_block->is_internal() && itr.second == 2) {
                int b = itr.first;
                Block *block = newPlan.get_block(b);
                for (FactProxy f: block->preconditions) {
                    for (int p: block->predecessors) {
                        if (!newPlan.blocks_orderings.have_constraint(p, b, f, PC) &&
                            newPlan.get_block(new_block->parent)->is_consumer(f) && new_block->is_deleter(f)) {
                            deleted_conflicted_blocks.push_back(b);
                        }
                    }
                }
            }

        }
        for (int i: deleted_conflicted_blocks) {
            if (!replace_conflicting_block(newPlan, i, new_b, &visited))
                return false;
        }

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
        write_log_file("<t> establishing connection: " + to_string(duration.count()) + " ms\n");
        return true;
    }

    bool BlockReplace::check_conflict(BlockDeorderPlan &newPlan, int b_x, int new_b) {
        Block *block_x = newPlan.get_block(b_x);
        Block *block_n = newPlan.get_block(new_b);
        vector<FactProxy> conflicting_facts;
        bool is_conflict = false;
        for (FactProxy f: block_n->preconditions) {
            if (block_n->is_deleter(f) && block_x->is_consumer(f) && block_x->is_deleter(f)) {
                is_conflict = true;
            }
        }
        return is_conflict;
    }

    bool
    BlockReplace::replace_conflicting_block(BlockDeorderPlan &newPlan, int b_x, int new_b, map<int, bool> *visited) {
        write_log_file("replacing conflicting block " + to_string(b_x) + "\n");
        Block *block_x = newPlan.get_block(b_x);
        Block *new_block = newPlan.get_block(new_b);
        vector<FactProxy> causal_facts;
        get_goals(block_x, &causal_facts);

        for (FactProxy f: causal_facts) {
            if (!newPlan.is_producer(new_b, f)) return false;
        }
        for (int s: block_x->successors) {
            for (OrderingConstraint constraint: newPlan.blocks_orderings.get_constraints(b_x, s)) {
                if (constraint.type == PC) {
                    if (newPlan.is_producer(new_b, constraint.factProxy)) {
                        add_replacement_orderings(newPlan, new_b, s, constraint.factProxy, PC);
                        establish_cd_dp_connection(newPlan, new_b, s);
                        resolve_threat_with_successors(newPlan, new_b, s, visited);
                        visited->at(s) = true;
                    } else
                        return false;
                }
            }
        }
        newPlan.delete_block(b_x);
        return true;
    }


/// establish CD DP orderings with predecessors
    void BlockReplace::resolve_threat_with_predecessors(BlockDeorderPlan &newPlan, int new_b, int b,
                                                        map<int, bool> *visited) {
        if (visited->at(b)) return;
        Block *block = newPlan.get_block(b);
        for (int i = 0; i < block->predecessors.size(); i++) {
            int p = block->predecessors[i];
            if (visited->at(p) || newPlan.blocks_orderings.is_transitive(p, b)) continue;
            establish_cd_dp_connection(newPlan, p, new_b);
            resolve_threat_with_predecessors(newPlan, new_b, p, visited);
            visited->at(p) = true;
        }
    }

/// establish CD DP orderings with successors
    void
    BlockReplace::resolve_threat_with_successors(BlockDeorderPlan &newPlan, int new_b, int b, map<int, bool> *visited) {
        if (visited->at(b)) return;
        Block *block = newPlan.get_block(b);
        for (int i = 0; i < block->successors.size(); i++) {
            int p = block->successors[i];
            if (visited->at(p) || newPlan.blocks_orderings.is_transitive(b, p)) continue;
            establish_cd_dp_connection(newPlan, new_b, p);
            resolve_threat_with_successors(newPlan, new_b, p, visited);
            visited->at(p) = true;
        }
    }

    bool BlockReplace::establish_cd_dp_connection(BlockDeorderPlan &newPlan, int b_x, int b_y) {
        bool established_connection = false;
        Block *block_x = newPlan[b_x];
        Block *block_y = newPlan[b_y];
        for (FactProxy factProxy: block_x->get_preconditions()) {
            if (newPlan.is_deleter(b_y, factProxy)) {
                add_replacement_orderings(newPlan, b_x, b_y, factProxy, CD);
                established_connection = true;
            }

        }
        if (b_x == newPlan.init_block) return established_connection;
        for (int s: block_y->successors) {
            for (OrderingConstraint constraint: newPlan.blocks_orderings.get_constraints(b_y, s)) {
                if (constraint.type == PC && newPlan.is_deleter(b_x, constraint.factProxy)) {
                    add_replacement_orderings(newPlan, b_x, b_y, constraint.factProxy, DP);
                    established_connection = true;
                }
            }
        }
        return established_connection;
    }



bool BlockReplace :: replace_block_by_operators_for_nonconcurrency(int b_x, int b_y){

    vector<int> conflicted_vars = bdpop.is_concurrent(b_x, b_y);

    map<int, int> connected = bdpop.get_connection_map(b_x);

    write_log_file("\t try replacing by block \n");
    auto start = chrono:: high_resolution_clock::now();

    Block* block_x = bdpop.get_block(b_x);
    OperatorProxy old_op = bdpop.task_proxy.get_operators()[block_x->op_index];

    vector<FactProxy> goals_proxy = bdpop.get_causal_link_facts(b_x, vector<int>(0));


    get_goals(bdpop.get_block(b_x), &goals_proxy);

    vector<FactPair> goals;
    for(FactProxy factProxy: goals_proxy){
        goals.push_back(factProxy.get_pair());
    }
    vector<FactProxy> crossing_facts = get_crossing_facts(b_x, &connected);
    if(print_details) {
        string str;
        str = "\t Goals: ";
        for (FactProxy factProxy: goals_proxy) {
            str += bdpop.print_fact(factProxy);
        }
        str += "\n";

        str += "\t Crossing facts: ";
        for (FactProxy factProxy: crossing_facts) {
            str += "<" + to_string( factProxy.get_variable().get_id()) +  ":" + to_string(factProxy.get_value()) + ">, ";
        }
        str += "\n";
        write_log_file(str);
    }

    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(subtask_initial_state,applicable_ops);

    for(OperatorID operatorId: applicable_ops) {
        OperatorProxy operatorProxy_new = bdpop.task_proxy.get_operators()[operatorId];

        if(block_x->op_index == operatorId) continue;
        if (operatorProxy_new.get_name() == "init_action") continue;
        int max_cost = old_op.get_cost() * (100+cost_compromise)/100;
        if(operatorProxy_new.get_cost() > max_cost) continue;   /// checking cost
        if(print_details)
            write_log_file("\ttry another operator " + operatorProxy_new.get_name() + "\n");

        bool has_goals = true;
        for(FactProxy f: goals_proxy){
            has_goals = false;
            for(EffectProxy effectProxy: operatorProxy_new.get_effects()){
                if(is_equal(effectProxy.get_fact(), f)) {
                    has_goals = true;
                }
            }
            if(!has_goals) break;
        }
        if(!has_goals) continue;
        bool is_deleter = false;
        for(FactProxy f: crossing_facts){
            if (bdpop.is_deleter_op(operatorId.get_index(), f)){
                is_deleter = true;
                break;
            }
        }

        if(is_deleter) continue;

        if(check_cd(operatorId, b_y) ||
           check_dp(operatorId, b_y, subtask_goals)
           || !bdpop.is_concurrent(b_x, b_y).empty())
            continue;
        write_log_file("\treplacing the block " + to_string(block_x->id) + " " +
                       bdpop.task_proxy.get_operators()[block_x->op_index].get_name() + "\ttry another operator " + operatorProxy_new.get_name() + "\n" );
        /// new bdpop construction
        BlockDeorderPlan newPlan = bdpop;

        Plan plan;
        plan.push_back(operatorId);

        block_x->type = TEMP_REMOVED;
        Block* new_block = make_block(plan, newPlan,goals_proxy);
        new_block->parent = block_x->parent;
        if (block_x->is_internal())
            newPlan.get_block(new_block->parent)->internal_blocks.push_back(new_block->id);

        if (print_details)
            new_block->print_preconditions_and_effects();


        if(establish_new_block(newPlan,b_x,b_y,new_block->id, false)){
            newPlan.find_transitive_ordering();
            newPlan.find_concurrent_actions();
            if(!newPlan.check_validity()){
                write_log_file( "not valid\n");
//                newPlan.print_bdop_dot_file(false,   "invalid.dot");
                newPlan.print_blocks_orderings();
                exit(1);
            }
            newPlan.calculate_flex();
//            if(newPlan.calculate_plan_cost() < bdpop.calculate_plan_cost() || bdpop.flex[0] < newPlan.flex[0]) {
            if(bdpop.flex[1] < newPlan.flex[1] ||
               (compromise_flex && newPlan.calculate_plan_cost() < bdpop.calculate_plan_cost())) {
//                newPlan.find_transitive_ordering();
                bdpop = newPlan;
                auto end = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
                write_log_file("block " + to_string(b_x) + " has been replaced by action replacement\n\t<t> time for replacement attempt :"
                               + to_string(duration.count()) + " ms\n");
                return true;
            }
            write_log_file("less flexibility\n");
        }
    }
    return false;
}

    int BlockReplace::get_replacing_block(int b, int b_x, int b_y) {
        if (b == b_x) return b_y;
        else return b_x;
    }


    State BlockReplace::get_initial_state_for_subtask(int b_x, State initial_state, vector<int> exclude,
                                                      bool allow_parallel) {

        Block *block = bdpop.get_block(b_x);
        State s = initial_state;

        /// if the block is an inner block, get state up to its parent block
        if (block->is_internal()) {
            s = get_initial_state_for_subtask(block->parent, s, vector<int>(0), allow_parallel);
        }
        vector<int> queue;

        /// if the block is an outer block, calculate initial state from the init block
        if (block->is_outer())
            queue.push_back(bdpop.init_block);
        else
            queue = bdpop.get_initial_blocks_of_a_block(block->parent);

        map<int, int> connected_map = bdpop.get_connection_map(b_x);
        map<int, bool> visited_map = bdpop.get_boolean_map();

        write_log_file("\ttry replacing by block \n");
        ///compute initial State
        while (!queue.empty()) {
            int itr = 0;

            for (; itr < queue.size(); itr++) {
                bool next = false;
                for (int q: queue) {
                    if (queue[itr] == q) continue;
                    if (bdpop.is_successor(q, queue[itr])) {
                        next = true;
                        break;
                    }
                }
                if (!next) break;
            }
            int i = queue[itr];
            queue.erase(queue.begin() + itr);
            visited_map.at(i) = true;

            if (is_in_vector(i, &exclude)) continue;
            if ((!allow_parallel || connected_map[i] != 0) && connected_map[i] != 1) continue;

//            write_log_file(to_string(i) + " ");
            s = get_successor(s, i);

            for (int b: bdpop.get_block(i)->successors) {
                if ((!allow_parallel || connected_map[i] != 0) && connected_map[i] != 1) continue;
                if (visited_map[b] || is_in_vector(b, &queue)) continue;
                queue.push_back(b);
            }
        }

        return s;
    }

/**
 * @brief check
 * @return
 */
    bool BlockReplace::check_cd(OperatorID op, int b_x) {
        OperatorProxy operatorProxy = bdpop.task_proxy.get_operators()[op];
        Block *block = bdpop[b_x];
        PreconditionsProxy preconditionsProxy = operatorProxy.get_preconditions();
        for (FactProxy factProxy: preconditionsProxy) {
            if (bdpop.is_deleter(b_x, factProxy))
                return true;
        }
        for (FactProxy factProxy: block->get_preconditions()) {
            if (bdpop.is_deleter_op(operatorProxy.get_id(), factProxy))
                return true;
        }
        return false;
    }

    bool BlockReplace::check_dp(OperatorID op, int b_x, vector<FactProxy> causal_links) {
        OperatorProxy operatorProxy = bdpop.task_proxy.get_operators()[op];
        Block *block = bdpop[b_x];
        EffectsProxy effectsProxy = operatorProxy.get_effects();
        for (EffectProxy effectProxy: effectsProxy) {
            if (count(causal_links.begin(), causal_links.end(), effectProxy.get_fact()) > 0 &&
                bdpop.is_deleter(b_x, effectProxy.get_fact()))
                return true;
        }
        for (EffectProxy effectProxy: block->get_effects()) {
            if (bdpop.have_constraint(b_x, effectProxy.get_fact(), PC, false) &&
                bdpop.is_deleter_op(operatorProxy.get_id(), effectProxy.get_fact()))
                return true;
        }
        return false;
    }

    void BlockReplace::add_replacement_orderings(BlockDeorderPlan &newPlan, int b_x, int b_y, FactProxy factProxy,
                                                 constraint_type type) {

        OrderingConstraint constraint(factProxy, type);

        vector<OrderingConstraint> constraints = newPlan.blocks_orderings.get_constraints(b_x, b_y);

        if (constraints.empty() || count(constraints.begin(), constraints.end(), constraint) == 0) {
            newPlan.add_constraint(b_x, b_y, constraint);
        }
        if (print_details) {
            write_log_file("\t\t(+)" + constraint.getTypeName() + bdpop.print_fact(factProxy) +
                           to_string(b_x) + " & " + to_string(b_y) + "\n");

        }

    }

    string convert_fact(string s) {
        s = std::regex_replace(s, std::regex("Atom "), "");
        s = std::regex_replace(s, std::regex("\\("), " ");
        s = std::regex_replace(s, std::regex("\\)"), "");
        s = std::regex_replace(s, std::regex(","), "");
        if (s.find("Negated") != string::npos) {
            s = std::regex_replace(s, std::regex("Negated"), "");
            s = "not (" + s + ")";
        }
        return s;
    }

    string read_problem_file(ifstream fin, ofstream out);

    void BlockReplace::write_problem(State initial_state, vector<FactProxy> goals, vector<int> exclude_var) {
        ofstream out;
        out.open("problem.txt");

        ifstream fin;
        string line;
        fin.open(problem_file_path);
//    fin.open( "problem.pddl");
        string word, predicate;
        char x, c;

        auto read_word = [&] {
            x = fin.get();
            word = "";
            while (x != ' ' && x != ')' && x != '(' && x != '\n') {
                word = word + x;
                x = fin.get();
            }
            transform(word.begin(), word.end(), word.begin(), ::tolower);
        };
        auto read_predicate = [&] {
            word = "";
            x = c;
            while (x != ')') {
                word = word + x;
                x = fin.get();
                if (x == '(') {
                    while (x != ')') {
                        word = word + x;
                        x = fin.get();
                    }
                    word = word + x;
                    x = fin.get();
                }
            }
            transform(word.begin(), word.end(), word.begin(), ::tolower);
        };
        c = fin.get();
        while (c != '(') {
            out << c;
            c = fin.get();
        }
        /// writing up to init
        out << c;
        read_word();

        while (word != ":init") {
            out << word;
            out << x;
            read_word();
        }
        out << word;

        /// writing initial state
        for (int i = 0; i < initial_state.get_unpacked_values().size(); i++) {
            if (is_in_vector(i, &exclude_var)) continue;
            string name = convert_fact(
                    bdpop.task_proxy.get_variables()[i].get_fact(initial_state.get_unpacked_values()[i]).get_name());
            out << "(" << name << ") " << endl;
        }


        /// writing extra predicates
        c = fin.get();
        while (c != ')') {
            if (c == '(') {
                read_predicate();
                string pred = trim(word);
                if (pred.front() == '(')
                    pred.erase(pred.begin());
                string s = get_tokens(pred, ' ').front();
                bool found = false;
                for (string p: predicates) {
//                transform(initial_state.begin(), initial_state.end(), initial_state.begin(), ::tolower);
                    if (trim(p).compare(trim(s)) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    out << word << ")" << endl;
            }
            c = fin.get();
        }

        out << c << endl;

        ///writing goal

        out << "(:goal" << endl;
        out << "(and" << endl;
        for (FactProxy f: goals) {
            if (is_in_vector(f.get_variable().get_id(), &exclude_var)) continue;
            string name = convert_fact(f.get_name());
            out << "(" << name << ")" << endl;
        }

        ///skipping goal
        read_word();
        while (word != ":goal") {
            read_word();
        }
        while (word != "and") {
            read_word();
        }

        c = fin.get();
        while (c != ')') {
            if (c == '(') {
                read_predicate();
            }
            c = fin.get();
        }
        while (!fin.eof()) {
            out << c;
            c = fin.get();
        }

        fin.close();
        out.close();
    }

    void BlockReplace::store_predicate_names() {
        for (VariableProxy var: bdpop.task_proxy.get_variables()) {
            int size = var.get_domain_size();
            string name1 = "";
            for (int i = 0; i < size; i++) {
                string name = var.get_fact(i).get_name();
                name = replace_string(name, "Atom ", "");
                name = replace_string(name, "Negated", "");
                name = get_tokens(name, '(').front();
                if (find(predicates.begin(), predicates.end(), name) == predicates.end()) {
                    transform(name.begin(), name.end(), name.begin(), ::tolower);
                    predicates.push_back(name);
                }
            }

        }
        for (string p: predicates)
            cout << p << endl;
    }

}