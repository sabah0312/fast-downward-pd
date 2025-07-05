//
// Created by oem on ১২/৫/২১.
//

#ifndef FAST_DOWNWARD_BLOCK_REPLACE_H
#define FAST_DOWNWARD_BLOCK_REPLACE_H

#include <iostream>

#include "deorder_algorithms/block_deorder.h"
#include "plan_reduction.h"
#include "cluster_causal_graph.h"

namespace domain_transition_graph {
    class DomainTransitionGraph;
    struct ValueNode;
}

namespace block_replace {

    class BlockReplace {

        vector<string> predicates;
        int cost_compromise = 0;
        BlockDeorder block_deorder;
        ClusterCausalGraph clusterCausalGraph;
        State subtask_initial_state;
        vector<FactProxy> subtask_goals;

        const successor_generator::SuccessorGenerator &successor_generator;
        StateRegistry state_registry;
        std::vector<std::unique_ptr<domain_transition_graph::DomainTransitionGraph>> transition_graphs;


        string domain_file_path, problem_file_path;
        bool print_details = false;
        bool only_replace_block = false;
        bool compromise_flex = false;
        bool concurrency = false;

        State get_successor(State state, int b_x);
        bool check_cd(OperatorID op, int b_x);

        bool replace_block_by_block_to_remove_ordering(int b, int b_x, int b_y, State s, vector<FactProxy> constraints,
                                                       map<int, int> *connected);

        void get_goals(Block *block_x, vector<FactProxy> *goals);

        State get_initial_state_for_subtask(int b_x, State initial_state, vector<int> exclude, bool allow_parallel);


        bool replace_block_by_operators_to_remove_ordering(int b, int b_x, int b_y, State s, vector<FactProxy> constraints,
                                                           map<int, int> *connected);


        vector<FactProxy> get_crossing_facts(int b_x, const map<int, int> *connected);


        void add_to_goals(FactProxy f, vector<FactProxy> *goals);

        bool check_dp(OperatorID op, int b_x, vector<FactProxy> causal_links);

        int get_replacing_block(int b, int b_x, int b_y);

        bool block_substitution_concurrency();

    public:
        BlockDeorderPlan bdpop;
        BlockReplace(TaskProxy taskProxy, bool only_replace_block, bool compromise_flex, bool concurrency);

        void set_files_path(string domain_path, string problem_path);

        void set_concurrency(bool concurrency);

        bool
        remove_constraints_by_replacing_second_block(int b_x, int b_y, map<string, vector<FactProxy>> *constraints);

        bool establish_connection_for_replacing_block(BlockDeorderPlan &newPlan, int b_x, int b_r, int new_b);

        void resolve_threat_with_predecessors(BlockDeorderPlan &newPlan, int new_b, int b, map<int, bool> *visited);

        void resolve_threat_with_successors(BlockDeorderPlan &newPlan, int new_b, int b, map<int, bool> *visited);

        bool establish_cd_dp_connection(BlockDeorderPlan &newPlan, int b_x, int b_y);

        void
        add_replacement_orderings(BlockDeorderPlan &newPlan, int b_x, int b_y, FactProxy factProxy,
                                  constraint_type type);


        bool remove_ordering_constraints_by_replacing_block(int b_x, int b_y);

        void store_predicate_names();

        bool check_conflict(BlockDeorderPlan &newPlan, int b_r, int new_b);

        Block *make_block(Plan plan, BlockDeorderPlan &newPlan, vector<FactProxy> goals);

        bool establish_new_block(BlockDeorderPlan &newPlan, int b, int b_r, int new_b, bool check);

        bool replace_conflicting_block(BlockDeorderPlan &newPlan, int b_x, int new_b, map<int, bool> *visited);

        bool replace_block_by_operators_for_nonconcurrency(int b_x, int b_y);

        bool remove_nonconcurrency(int b_x, int b_y);

        void write_problem(State initial_state, vector<FactProxy> goals, vector<int> exclude_var);


        void initialize_block_replace();

        bool check_extension_candidency(int b_x, int b_y, vector<int> conflicted_variables, bool up);

        int extend_block(int b_x, int b_y, vector<int> conflicted_variables);

        bool check_alternative_path(int var, int start_value, int end_value, vector<int> except_var);


        void set_initial_state_and_goals_for_subtask(int b_x);

        bool replace_block_by_block_for_nonconcurrency(int b_x, int b_y);

        void get_goals(Block *block_x);

        void add_to_goals(FactProxy factProxy);

        void set_initial_state_and_goals_for_subtask(int b_x, bool allow_parallel);

        bool check_path(domain_transition_graph::ValueNode *start_node, domain_transition_graph::ValueNode *end_node,
                        vector<int> except_var);

        vector<int>
        get_operators(domain_transition_graph::ValueNode *start_node, domain_transition_graph::ValueNode *end_node);
    };

}
#endif //FAST_DOWNWARD_BLOCK_REPLACE_H
