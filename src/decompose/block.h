#ifndef FAST_DOWNWARD_BLOCK_H
#define FAST_DOWNWARD_BLOCK_H

using namespace std;

#include <vector>
#include "map"
#include "../search/operator_id.h"
#include "../search/task_proxy.h"


enum block_type  { REGULAR, TEMP_INTERNAL, TEMP_REMOVED};
struct Block {

    Block(int id, OperatorID op);

    Block(int id, vector<int> blocks, bool temp);

    Block(int id, OperatorID op, vector<int> internals, block_type type, bool temp, int parent);

    OperatorID op_index;
    int id;
    block_type type;
    bool is_temporary;
    vector<int> predecessors;
    vector<int> successors;


    int parent;
    vector<int> internal_blocks;
    vector<FactProxy> preconditions;
    vector<EffectProxy> effects;
//    map<int, Graph > effect_map;

    Block(const Block &block): op_index(block.op_index)  {
        id = block.id;
        type = block.type;
        is_temporary = block.is_temporary;
        predecessors = block.predecessors;
        successors = block.successors;
        parent = block.parent;
        internal_blocks = block.internal_blocks;
        preconditions = block.preconditions;
        effects = block.effects;
    }

    bool operator==(const Block &other) const {
        return id == other.id;
    }

    bool is_primary();

    string name();

    string getTypeName();

    string print();

    vector<FactProxy> get_preconditions();

    vector<EffectProxy> get_effects();

    string print_preconditions_and_effects();

    bool is_internal();

    bool is_outer();

    void set_outer();

    void set_type_normal();

    bool is_temp_removed();

    bool is_temp_internal();

    string print_predecessors_and_successors();

    bool has_precondition(FactProxy f);

    vector<EffectProxy> get_effects(FactProxy factProxy);

    bool has_precondition_of_different_value(FactProxy factProxy);

    bool is_consumer(FactProxy factProxy);

    bool is_producer(FactProxy factProxy);

    bool is_deleter(FactProxy factProxy);

    bool is_immediate_predecessor(int b_x);

    bool is_immediate_successor(int b_x);

    void delete_predecessor(int b_x);

    void delete_successor(int b_x);

    void add_predecessor(int b_x);

    void add_successor(int b_x);

    void add_internal(int b_x);

    void delete_internal(int b_x);
};


#endif FAST_DOWNWARD_BLOCK_H
