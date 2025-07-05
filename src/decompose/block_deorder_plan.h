//
// Created by sabah on ১/৯/২০.
//

#ifndef FAST_DOWNWARD_BLOCK_DEORDER_PLAN_H
#define FAST_DOWNWARD_BLOCK_DEORDER_PLAN_H

#include "block.h"
#include "ordering_constraints.h"
#include "../search/plan_manager.h"
#include "map"
//#include "cluster_causal_graph.h"
#include "orderings.h"
#include "cluster_causal_graph.h"

using namespace std;


class BlockDeorderPlan {

public:
    BlockDeorderPlan();
    map<int, Block> blocks;
    Orderings blocks_orderings;
    Orderings concurrent_orderings;
    TaskProxy task_proxy;

    bool concurrency = false;
    int goal_block = -1;
    int init_block = -1;
    string file_prefix="";
//    string domain_file, problem_file;

    int next_block_id;
    bool print_details = false;
    float flex[2];
    ClusterCausalGraph *clusterCausalGraph;
    BlockDeorderPlan(const BlockDeorderPlan &blockDeorderPlan, TaskProxy taskProxy) : task_proxy(taskProxy) {
        blocks = blockDeorderPlan.blocks;
        blocks_orderings = blockDeorderPlan.blocks_orderings;
        goal_block= blockDeorderPlan.goal_block;
        init_block = blockDeorderPlan.init_block;
        next_block_id = blockDeorderPlan.next_block_id;
        print_details = blockDeorderPlan.print_details;
    }


    Block* operator[](int id)  {
        return &blocks.at(id);
    }


    string get_block_name(int b_x);

    vector<int> get_primary_blocks(int b_x);

    int cost(int b_x);

    void print_outer_blocks();

    void print_blocks_orderings();

    void print_cluster_orderings_dot_file(ostream &out, int b_x, bool with_transitive);

    void print_ordering_label_dot_file(ostream &out, vector<OrderingConstraint> orderingConstraints);

    void print_node_dot_file(ostream &out, int b_x);

    bool check_validity();

    bool check_validity(int b_x);

    int check_validity_of_causal_link(int b_x, FactProxy f);

    void find_intermediate_blocks(int b_x, int b_y, vector<int> *intermediate_blocks);

    vector<int> find_in_between_blocks_for_temp_blocks(int temp_x, int temp_y);

    int get_producer(int consumer, FactProxy f);

    void make_temporary_block_permanent(int b_x);

    vector<FactProxy> find_preconditions(vector<int> related_blocks);

    void find_transitive_ordering();

    vector<FactProxy> get_causal_link_facts(int b_x, vector<int> exclude_block);

    vector<int> get_primary_blocks();

    int check_operator_connected(int i, int j);

    void delete_block(int b_x);

    void delete_block_ordering(int b_x);

    void mark_as_connected(int b_x, map<int, int> *connected, int connection);

    map<int, bool> get_boolean_map();

    map<int, int> get_int_map();

    void search_consumers_downward(int b_x, FactProxy f, vector<int> *consumers, map<int, bool> *visited_blocks);

    Plan get_total_order_plan();

    vector<int> get_initial_blocks_of_a_block(int b_x);


    int find_block_id(OperatorID op, vector<int> related_blocks);

    void print_total_order_plan_dot_file(ostream &fout, Plan total_order_plan);

    bool is_producer_in_block(Block *block, Block *parent, FactProxy f);

    map<int, int> get_connection_map(int b_x);

    void print_state(State s);

    int get_responsible_producer_block(int b_x, FactProxy fact);

    int search_upward(int b_x, FactProxy f, int search_type);

    int search_downward(int b_x, FactProxy f, int search_type);

    int create_compound_block(vector<int> internal_blocks);

    bool find_intermediate_blocks(int b_x, int b_y, vector<int> *intermediate_blocks, map<int, bool> *visited);

    void delete_block_predecessor_successor(int b_x);

    vector<int> get_parallel_blocks(int b_x);

    string print_fact(FactProxy f);

    int get_plan_cost(const Plan &plan);

    bool is_successor(int b_x, int successor, bool allow_temp_internal = false);

    bool is_successor(int b_x, int successor, map<int, bool> *visited, bool allow_temp_internal);

    vector<struct OrderingConstraint> get_associated_pc_constraints(int b_x);

    void delete_block(int b_x, int original);

    bool is_consumer(int b_x, FactProxy factProxy);

    bool is_producer(int b_x, FactProxy factProxy);

    bool is_deleter(int b_x, FactProxy factProxy);

    bool has_causal_link(int b_x, FactProxy factProxy);

    bool is_candidate_producer(int b_x, FactProxy factProxy);

    int get_producing_effect(int b_x, FactProxy factProxy);

    void topological_sort(int b_x, vector<int> *queue, map<int, bool> *visited);

    void write_total_order_plan(string filename);

    void mark_predecessors(int b_x, map<int, int> *connected, map<int, bool> *visited);

    void mark_successors(int b_x, map<int, int> *connected, map<int, bool> *visited);

    void initialize(Plan total_order_plan);

    vector<EffectProxy> get_applicable_effects(int b_x);

    vector<FactProxy> get_conditional_consuming_facts(int b_x);

    void mark_all_visited_predecessors(int b_x, map<int, bool> *visited, map<int, bool> *mark_visited);

    bool is_producer_except_val(int b_x, FactProxy factProxy);

    int search_upward_producer_except_value(int b_x, FactProxy f);


    int find_earliest_incidental_candidate_producer(int b_x, FactProxy f);

    int find_earliest_incidental_candidate_producer(int b_x, FactProxy f, map<int, bool> *visited, int &has_deleter);

    bool have_constraint(int b_x, FactProxy f, constraint_type t, bool is_incidental);

    bool is_active(int b_x, FactProxy factProxy);

    bool is_applicable(Block *block_x, EffectProxy effectProxy);

    vector<EffectProxy> get_conditional_applicable_effects(Block *block_x);

    bool is_consumer_of_different_value(Block *block_x, FactProxy f);

    bool is_deleter_op(int opId, FactProxy f);

    int find_earliest_candidate_producer(int b_x, FactProxy f, vector<int> exclude);

    int find_earliest_candidate_producer(int b_x, FactProxy f, vector<int> exclude, map<int, bool> *visited,
                                         vector<int> *producers, vector<int> *deleters);

    int calculate_height();

    int calculate_height(int b_x, map<int, bool> *visited, map<int, int> *height);

    void find_concurrent_actions();

    vector<int> is_concurrent(int b_x, int b_y);

    void print_cluster_concurrent_orderings_dot_file(ostream &fout);

    void write_block_deorder_plan(string filename);

    void write_block(ofstream &out, string prefix, Block *block);

    void calculate_flex();

    int calculate_plan_cost();

    void print_bdop_dot_file(bool with_transitive, string filename);

    vector<EffectProxy> find_effects(vector<int> related_blocks);

    vector<int> get_compound_blocks();

    vector<int> is_concurrent(Block *block_x, Block *block_y);

    bool establish_extended_block_orderings(int b_x, vector<int> internal_blocks);

    bool validate_total_order_plan();

    void reordering_block_after_deleting_element(int b_x);

    void reconstruct_parent_after_deletion(int bx);

    bool is_producer_in_block(int b_x, int parent, FactProxy f);

    void print_effects_and_preconditions();

    void print_predecessord_successors();

    void add_constraint(int b_x, int b_y, OrderingConstraint constraint);

    void remove_constraint(int b_x, int b_y, OrderingConstraint constraint);

    void clear_constraints(int b_x, int b_y);

    int get_conditional_pc(int b_x, int b_y, OrderingConstraint *associated_constraint);

    Block *get_block(int id);

    map<int, Block> *get_blocks();

    int get_new_block_id();

    int size();

    Block *insert(Block block);
    Plan read_sequential_plan (string planfile);

    Plan get_total_order_plan(int b_x);

    void write_total_order_plan(Plan plan, string filename);

    int create_new_compound_block(BlockDeorderPlan *bdplan);

    bool unblock(int b_x);

    bool is_same(int b_x, BlockDeorderPlan bdp);

    bool has_op(OperatorID op, BlockDeorderPlan bdp, int b_x);

    bool has_op(OperatorID op, BlockDeorderPlan bdp);
};


#endif //FAST_DOWNWARD_BLOCK_DEORDER_PLAN_H
