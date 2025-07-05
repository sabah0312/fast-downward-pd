//
// Created by sabah on ১/৯/২০.
//

#include "block_deorder_plan.h"
#include "block.h"
#include <map>
#include <chrono>
#include "myutils.h"
#include "../search/tasks/root_task.h"
#include <sstream>

BlockDeorderPlan :: BlockDeorderPlan():task_proxy(*tasks::g_root_task){}

///initializing a plan by creating a primitive block for each operator
void BlockDeorderPlan :: initialize(Plan total_order_plan){
    blocks.clear();
    blocks_orderings.clear();
    int size = total_order_plan.size();
    OperatorsProxy operatorsProxy =task_proxy.get_operators();

    for (int i = 0; i < total_order_plan.size(); i++){
        Block block(i, total_order_plan[i]);
        OperatorProxy operatorProxy = operatorsProxy[block.op_index.get_index()];
        for(FactProxy factProxy:operatorProxy.get_preconditions())
            block.preconditions.push_back(factProxy);
        for(EffectProxy effectProxy:operatorProxy.get_effects())
            block.effects.push_back(effectProxy);
        insert(block);
    }
    next_block_id = blocks.size();
}

int BlockDeorderPlan:: get_plan_cost(const Plan &plan) {
    OperatorsProxy operators =task_proxy.get_operators();
    int plan_cost = 0;
    for (OperatorID op_id : plan) {
        plan_cost += operators[op_id].get_cost();
    }
    return plan_cost;
}

/// @brief return the pointer of a block
/// \param id of the block
Block* BlockDeorderPlan ::  get_block(int id)  {
    return &blocks.at(id);
}

///
/// \return the blocks of the BDPOP
map<int, Block>* BlockDeorderPlan ::  get_blocks(){
    return &blocks;
}

///
/// \return a id for the next new block
int BlockDeorderPlan :: get_new_block_id(){
    return next_block_id++;
}

///
/// \return the number of blocks
int BlockDeorderPlan :: size(){
    return blocks.size();
}

/// @brief insert a new block to the BDPOP
/// \param block
/// \return
Block* BlockDeorderPlan :: insert(Block block){
    blocks.insert(pair<int, Block>(block.id, block));
    return get_block(block.id);
}

///**
// * @brief checks every block. If a block is redundant, deletes it
// */
//bool BlockDeorderPlan :: remove_redundant_block(){
//    auto start = chrono:: high_resolution_clock::now();
//    bool found_redundant = false;
//    for (auto it = blocks.begin(); it != blocks.end();){
//        auto current  = it++;
//        int id = current->second.id;
//        if (id == goal_block ) continue;
//        if (is_redundant(id)){
//            if(print_details)
//                write_log_file( "**block "+ to_string(id) + " is redundant.\n");
//            delete_block(id);
//            found_redundant = true;
//        }
//    }
//
//    auto end = chrono:: high_resolution_clock::now();
//    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
//    cout << "<t> delete redundant block : (wall clock: "<< duration.count() << " ms)" << endl;
//    find_transitive_ordering();
//    return found_redundant;
//}

///**
// * @brief A block is redundant if it is not a producer in any causal link
// * @param b_x the block id
// * @return true, if it is redundant. flase, otherwise.
// */
//bool BlockDeorderPlan :: is_redundant(int b_x){
//    Block *block = get_block(b_x);
//    if(block->is_internal()) return false;
//    if(!block->is_primary() && block->internal_blocks.empty())
//        return true;
//    return get_associated_pc_constraints(b_x).empty();
//
//}



void BlockDeorderPlan :: add_constraint(int b_x, int b_y, OrderingConstraint constraint){
    if (blocks_orderings.have_constraint(b_x, b_y, constraint))
        return;
    blocks_orderings.add_constraint(b_x, b_y, constraint);
    get_block(b_x)->add_successor(b_y);
    get_block(b_y)->add_predecessor(b_x);
    if (print_details){
        write_log_file("\t\t\t (+) " + constraint.print() + "B:" + to_string(b_x) + " & B:" + to_string(b_y) + "\n");
    }

}

void BlockDeorderPlan :: remove_constraint(int b_x, int b_y, OrderingConstraint constraint){
    if (blocks_orderings.have_ordering(b_x, b_y))
        blocks_orderings.remove_constraint(b_x, b_y, constraint);
    if(!blocks_orderings.has_ordering_except(b_x, b_y, constraint.factProxy, constraint.type)) {
        get_block(b_x)->delete_successor(b_y);
        get_block(b_y)->delete_predecessor(b_x);
    }
    if (print_details){
        write_log_file("\t\t\t (-) " + constraint.print() + "B:" + to_string(b_x) + " & B:" + to_string(b_y) + "\n");
    }
}

void BlockDeorderPlan :: clear_constraints(int b_x, int b_y){
    if (!blocks_orderings.have_ordering(b_x, b_y)) return;

    blocks_orderings.clear_constraints(b_x,b_y);
    get_block(b_x)->delete_successor(b_y);
    get_block(b_y)->delete_predecessor(b_x);

}

/// check
int BlockDeorderPlan :: get_conditional_pc(int b_x, int b_y, OrderingConstraint* associated_constraint){
    vector<OrderingConstraint> constraints = blocks_orderings.get_constraints(b_x, b_y);
    for(int i = 0; i < constraints.size(); i++){
        OrderingConstraint constraint = constraints[i];
        if(constraint.type == PC && constraint.is_conditional_causal_link() && constraint.get_associated_constraint(associated_constraint)> 1)
            return i;
    }
    return -1;
}


///retrieve the causal links where block b_x is a producer
vector<OrderingConstraint> BlockDeorderPlan :: get_associated_pc_constraints(int b_x){
    vector<OrderingConstraint> pc_constraints;
    Block *block = get_block(b_x);
    if(block->is_internal()){
        for(OrderingConstraint constraint : get_associated_pc_constraints(block->parent)){
            if(is_producer_in_block(block, get_block(block->parent), constraint.factProxy)){
                pc_constraints.push_back(constraint);
            }
        }
    }

    for (int b_y : block->successors){
        vector<OrderingConstraint> constraints = blocks_orderings.get_constraints(b_x, b_y);
        for(OrderingConstraint constraint: constraints){
            if(constraint.type == PC && !constraint.is_incidental){
                pc_constraints.push_back(constraint);
            }
        }
    }
    return pc_constraints;
}

/**
 * @brief find a internal block that produces the fact and is responsible for producing the fact for parent block b_x
 * @param b_x the id of the parent block
 * @param fact the fact
 * @return the id of the internal responsible block. -1, otherwise
 */
int BlockDeorderPlan :: get_responsible_producer_block(int b_x, FactProxy fact){
    Block* block = get_block(b_x);

    if(block->is_primary()){
        if(is_producer(b_x, fact))
            return b_x;
        return -1;
    }
    vector<int> queue = get_initial_blocks_of_a_block(b_x);
    for (int i: block->internal_blocks){
        Block *block_i = get_block(i);
        if (is_producer(i, fact)){
            for (int j: block->internal_blocks){
                if (is_successor(j, i, false) &&  is_deleter(j, fact))
                    continue;
            }
            if(print_details)
                write_log_file( "child producer " + to_string(block_i->id) + "of the block " + to_string(b_x) + "\n");
            return block_i->id;
        }
    }
    if(print_details)
        write_log_file( "error!!!!!!!!!!!!!!! no producer found\n");
//    assert("no producer");
    return -1;
}

/**
 * @brief find the facts on which the block is a producer in some causal link.
 * @param b_x the block index
 * @param exclude_block the set of blocks which won't be considered
 * @return a vector of facts
 */
vector<FactProxy> BlockDeorderPlan :: get_causal_link_facts(int b_x, vector<int> exclude_block){
    vector<FactProxy> facts;
    Block* block = get_block(b_x);
    auto check =[&](){
        for(int b: get_block(b_x)->successors) {
            if (count(exclude_block.begin(), exclude_block.end(), b) > 0)
                continue;
            vector<OrderingConstraint> constraints = blocks_orderings.get_constraints(b_x, b);
            for (OrderingConstraint constraint: constraints) {
                if (constraint.type == PC && count(facts.begin(), facts.end(), constraint.factProxy) == 0) {
                    facts.push_back(constraint.factProxy);
                }
            }
        }
    };

    if (block->is_outer()){
        check();
    } else{
        Block* parent = get_block(block->parent);
        check();
        vector<FactProxy> parent_facts = get_causal_link_facts(block->parent, vector<int>(0));
        for (FactProxy factProxy: parent_facts){
            if (count(facts.begin(), facts.end(), factProxy) == 0 && is_producer(b_x, factProxy)){
                if (get_responsible_producer_block(parent->id, factProxy) == block->id)
                    facts.push_back(factProxy);
            }
        }
    }
    return  facts;
}


/**
 * @brief creates a temporary block with given internal blocks
 * @param internal_blocks the vector of internal blocks
 * @return the id of new temporary block
 */
int BlockDeorderPlan ::  create_compound_block(vector<int> internal_blocks){

    int block_id = get_new_block_id();
    Block new_block(block_id, internal_blocks, true);
    Block *block = insert(new_block);

    Block *internal_block = get_block(internal_blocks[0]);
    bool has_old_parent= false;
    Block* old_parent;
    if (internal_block->is_internal()) {
        old_parent = get_block(internal_block->parent);
        has_old_parent = true;
        old_parent->add_internal(block_id);
        block->parent = old_parent->id;
    }

    for (int internal : block->internal_blocks){
        Block* internal_block = get_block(internal);
        internal_block->type = TEMP_INTERNAL;
        get_block(internal)->parent = block_id;
        if (has_old_parent)
            old_parent->delete_internal(internal);
    }

    if(print_details){
        write_log_file("\t compound block has been made: " + block->print() + "\n");
    }
    block->preconditions = find_preconditions(block->internal_blocks); /// find the preconditions of the new temporary block
    block->effects = find_effects(block->internal_blocks);  /// find the effects of the new temporary block
//    write_log_file("extended block " + to_string(block_id) + "\n");
    return block_id;
}

int BlockDeorderPlan :: create_new_compound_block(BlockDeorderPlan *bdplan){
    int block_id = get_new_block_id();
    Block block(block_id, vector<int>(0), false);
    Block *new_parent_block = insert(block);

    map<int,int> internal_map;
    for(auto itr:bdplan->blocks){
        if( itr.first == bdplan->goal_block || itr.first == bdplan->init_block) continue;
        int id = get_new_block_id();
        Block internal(itr.second);
        internal.id = id;
        internal.type = REGULAR;
        internal.is_temporary = false;
        internal.predecessors.clear();
        internal.successors.clear();
        internal.parent = new_parent_block->id;
        insert(internal);
        new_parent_block->internal_blocks.push_back(id);
        internal_map.insert(pair<int, int>(itr.first, id));
//        if(print_details){
            write_log_file("\t compound block has been made: " + get_block(id)->print() + "\n");
        write_log_file(task_proxy.get_operators()[get_block(id)->op_index].get_name());
//        }
    }

    for(int i = 0; i < bdplan->blocks.size() ; i++){
        int old_i = bdplan->blocks.at(i).id;
        if( old_i == bdplan->init_block || old_i == bdplan->goal_block) continue;
        for(int j = 0; j <  bdplan->blocks.size() ; j++){
            int old_j = bdplan->blocks.at(j).id;
             if(old_i == old_j || old_j == bdplan->init_block || old_j == bdplan->goal_block) continue;
            int x = internal_map.at(old_j);
            int y = internal_map.at(old_i);
            write_log_file("establishing between " + to_string(j) + " and " + to_string(i)+ "\n");
            if (bdplan->blocks_orderings.have_ordering(old_j, old_i)) {
                Ordering *ordering = bdplan->blocks_orderings.get_ordering(old_j, old_i);
                for (OrderingConstraint constraint: ordering->orderingConstraints)
                {
                    add_constraint(x, y, constraint);
                    blocks_orderings.set_transitive(x, y, ordering->is_transitive);
                }
            }
        }
    }
    new_parent_block->effects = find_effects(new_parent_block->internal_blocks);
    new_parent_block->preconditions = find_preconditions(new_parent_block->internal_blocks);
    write_log_file(new_parent_block->print());
    write_log_file(new_parent_block->print_preconditions_and_effects());
    return new_parent_block->id;
}

bool BlockDeorderPlan :: unblock(int b_x) {
//    vector <tuple< int, int, OrderingConstraint>> remove_pc_orderings;

    Block *block = get_block(b_x);
    vector<int> parallel_blocks = get_parallel_blocks(b_x);
    bool create_ordering;

    if (print_details) write_log_file("establishing orders\n");
    cout << "predecessors\n";
    for (int predecessor: block->predecessors) {
        vector<OrderingConstraint> orderingConstraints = blocks_orderings.get_constraints(predecessor, b_x);

        for (OrderingConstraint constraint: orderingConstraints) {
            OrderingConstraint new_orderingConstraint(constraint);

            if (constraint.is_causal_link()) {
                for (int internal: block->internal_blocks) {
                    if (is_consumer(internal, constraint.factProxy)) {
                        bool found = false;
                        for(int p: get_block(internal)->predecessors){
                            if(blocks_orderings.have_constraint(p, internal, constraint.factProxy, PC)) {
                                found = true;
                                break;
                            }
                        }
                        if(!found)
                            add_constraint(predecessor, internal, new_orderingConstraint);
                    }
                }

            } else if (constraint.type == CD) {
                for (int internal: block->internal_blocks) {
                    if (is_deleter(internal, constraint.factProxy)) {
                        add_constraint(predecessor, internal, new_orderingConstraint);
                    }
                }
            } else if (constraint.type == DP) {
                for (int internal: block->internal_blocks) {
                    if (is_producer(internal, constraint.factProxy) &&
                    have_constraint(internal, constraint.factProxy, PC, false))
                        add_constraint(predecessor, internal, new_orderingConstraint);
                }
            }

        }
//        clear_constraints(predecessor, b_x);
    }
    cout << "successors\n";
    for (int successor: block->successors) {
        vector<OrderingConstraint> orderingConstraints = blocks_orderings.get_constraints( b_x, successor);

        for (OrderingConstraint constraint: orderingConstraints) {
            OrderingConstraint new_orderingConstraint(constraint);

            if (constraint.is_causal_link()) {
                int producer = get_responsible_producer_block(b_x, constraint.factProxy);
                if(producer <0) return false;
                add_constraint( producer, successor, new_orderingConstraint);

            } else if (constraint.type == CD) {
                for (int internal: block->internal_blocks) {
                    if (is_consumer(internal, constraint.factProxy)) {
                        add_constraint(internal, successor, new_orderingConstraint);
                    }
                }
            } else if (constraint.type == DP) {
                for (int internal: block->internal_blocks) {
                    if (is_deleter(internal, constraint.factProxy) )
                        add_constraint( internal, successor, new_orderingConstraint);
                }
            }
        }

//        clear_constraints(b_x,successor);
    }
    ///////////////////
    cout << "delete\n";
    for(int internal:block->internal_blocks){
        Block* internal_block = get_block(internal);
        internal_block->parent = block->parent;
        if(block->is_internal()){
            Block* new_parent_block = get_block(block->parent);
            new_parent_block->internal_blocks.push_back(internal);
        }
    }
    block->internal_blocks.clear();
    delete_block(b_x);

    ////////////////////

//    for(int internal: block->internal_blocks){
//        vector<int> i_parallel = parallel_blocks;
//        Block* internal_block = get_block(internal);
//        for(FactProxy factProxy: internal_block->preconditions){
//            for(int p: parallel_blocks){
//                if()
//            }
//        }
//    }
    return true;
}

bool BlockDeorderPlan :: has_op(OperatorID op, BlockDeorderPlan bdp){
        for(auto itr: bdp.blocks){
            if(itr.second.is_primary() && itr.second.op_index == op)
                    return true;

        }
    return false;
}

bool BlockDeorderPlan :: is_same(int b_x, BlockDeorderPlan bdp){
    map<int,int> block_map;
    Block* block_x = get_block(b_x);
    if(block_x->internal_blocks.size() != bdp.blocks.size()) return false;
    if(block_x->is_primary()){

    } else{
        for(int internal:block_x->internal_blocks){
//            Block*
//            if(!has_op(, bdp))
                return false;
        }
    }


}

/**
 * @brief establishes the orderings of a set of internal  blocks of a temporary block
 * @param b_temp the index of the temporary block
 * @param internal_blocks the set of internal  blocks
 * @return true if the ordering establishment is successful, false otherwise
 */
bool BlockDeorderPlan :: establish_extended_block_orderings(int b_x, vector<int> internal_blocks){
    vector <tuple< int, int, OrderingConstraint>> remove_pc_orderings;

    Block *temp_block = get_block(b_x);

    bool create_ordering;

    if (print_details) write_log_file("establishing orders\n");

    for (int j: internal_blocks) {
        int relatedBlock_i = j;
        Block* block_j = get_block(relatedBlock_i);

        ///establishing the connection with the predecessors
        vector<int> predecessors = block_j->predecessors;
        for (int predecessor: predecessors) {
//            int predecessor = i;
            if(is_in_vector(predecessor, &internal_blocks)) continue;
            Block* block_p = get_block(predecessor);
            if ( predecessor == b_x) continue;

            vector<OrderingConstraint> orderingConstraints = blocks_orderings.get_constraints(predecessor, relatedBlock_i);
            for (int k = orderingConstraints.size() - 1; k >= 0; k--) {
                OrderingConstraint new_orderingConstraint(orderingConstraints[k]);
                remove_constraint(predecessor, relatedBlock_i, orderingConstraints[k]);
                if (orderingConstraints[k].is_causal_link() && is_consumer(b_x, orderingConstraints[k].factProxy)) {
                    add_constraint(predecessor, b_x, new_orderingConstraint);
                }
                else if (orderingConstraints[k].type == CD && is_deleter(b_x, orderingConstraints[k].factProxy)){
                    add_constraint(predecessor, b_x, new_orderingConstraint);
                }
                else if (orderingConstraints[k].type == DP && is_producer(b_x, orderingConstraints[k].factProxy)){
                    add_constraint(predecessor, b_x, new_orderingConstraint);
                }
            }
        }

        ///establishing the connection with the successors
        vector<int> successors = block_j->successors;
        for (int successor : successors) {
            if(is_in_vector(successor, &internal_blocks)) continue;
            Block *block_p = get_block(successor);
            if ( successor == b_x) continue;

            vector<OrderingConstraint> orderingConstraints = blocks_orderings.get_constraints(relatedBlock_i, successor);
            for (int k = orderingConstraints.size() - 1; k >= 0; k--) {
                OrderingConstraint new_orderingConstraint(orderingConstraints[k]);
                remove_constraint(relatedBlock_i, successor, orderingConstraints[k]);
                if(orderingConstraints[k].type == PC && is_candidate_producer(b_x, orderingConstraints[k].factProxy)) {
                    add_constraint( b_x, successor, new_orderingConstraint);
                }
                else if (orderingConstraints[k].type == CD && is_consumer(b_x, orderingConstraints[k].factProxy)){
                    add_constraint( b_x, successor, new_orderingConstraint);
                }
                else if (orderingConstraints[k].type == DP && is_deleter(b_x, orderingConstraints[k].factProxy)){
                    add_constraint( b_x, successor, new_orderingConstraint);
                }
            }
        }
    }

    //cout << "number of remove ordering " << remove_orderings.size() << " ";

//    time();
    return true;
}

/**
 * @brief delete a block including its internal blocks
 * @param b_x the block index
 */
void BlockDeorderPlan :: delete_block(int b_x){
//    auto start = chrono:: high_resolution_clock::now();
    if(print_details)
        write_log_file( "deleting block " + to_string(b_x) + "\n" );
    Block* block = get_block(b_x);
    int parent = block->parent;
    delete_block(b_x, b_x);

    if (parent > 0) {
        Block *block_parent = get_block(parent);
        if(block_parent->internal_blocks.empty())
            delete_block(parent);
        else
            reconstruct_parent_after_deletion(parent);
    }


//    auto end = chrono:: high_resolution_clock::now();
//    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
//    cout << "<t> deleting block : (wall clock:"<< duration.count() << " ms)" << endl;
}

/**
 * @brief delete a block including its internal blocks
 * @param b_x the block index
 * @param r_x the root block index
 */
void BlockDeorderPlan :: delete_block(int b_x, int r_x){
//    cout << "deleting block " << b_x << endl;
    Block* block = &blocks.at(b_x);
    int parent = block->parent;
    if(!block->is_primary()){
        while (!block->internal_blocks.empty()){
            int internal = block->internal_blocks.back();
            block->internal_blocks.pop_back();
            delete_block(internal, r_x);
        }
    }
    delete_block_ordering(b_x);
    delete_block_predecessor_successor(b_x);
    if (block->is_internal()){
        Block* block_parent = get_block(parent);
        delete_element_from_vector(&block_parent->internal_blocks, b_x);
//        reconstruct_parent_after_deletion(parent);
    }
//    cout << "problem here!!!" << endl;
    blocks.erase(b_x);
}

void BlockDeorderPlan :: reconstruct_parent_after_deletion(int b_x){
    Block *block = get_block(b_x);
    if (block->internal_blocks.empty()) return;
    block->preconditions = find_preconditions(block->internal_blocks);
    block->effects = find_effects(block->internal_blocks);
//    write_log_file(block->print());
//    write_log_file(block->print_predecessors_and_successors());
//    write_log_file(block->print_preconditions_and_effects());
    reordering_block_after_deleting_element(b_x);
    if(block->is_internal()){
        reconstruct_parent_after_deletion(block->parent);
    }


}

void BlockDeorderPlan :: reordering_block_after_deleting_element (int b_x){
    Block* block = get_block(b_x);
    vector<int> del_successors, del_predecessors;
//    write_log_file("\t\treordering " + to_string(b_x) + "\n");
    for(int successor: block->successors){
//        write_log_file("\t\t\treordering with" + to_string(successor) + "\n");
        vector<OrderingConstraint> constraints = blocks_orderings.get_constraints(b_x, successor);
        for(OrderingConstraint constraint: constraints){
            if(constraint.type == PC && !is_producer(b_x, constraint.factProxy))
                blocks_orderings.remove_constraint( b_x, successor, constraint);
            else if(constraint.type == CD && !is_consumer(b_x, constraint.factProxy))
                blocks_orderings.remove_constraint( b_x, successor, constraint);
            else if (constraint.type == DP && !is_deleter(b_x, constraint.factProxy))
                blocks_orderings.remove_constraint( b_x, successor, constraint);
        }
//        write_log_file("done\n");
        if(blocks_orderings.get_constraints(b_x, successor).empty()) {
            del_successors.push_back(successor);
        }
//        write_log_file("*done\n");

    }
    for(int predecessor: block->predecessors){
//        write_log_file("\t\t\treordering with" + to_string(predecessor) + "\n");
        vector<OrderingConstraint> constraints = blocks_orderings.get_constraints(predecessor, b_x);
        for(OrderingConstraint constraint: constraints){
            if(constraint.type == PC && !is_consumer(b_x, constraint.factProxy))
                blocks_orderings.remove_constraint(predecessor, b_x, constraint);
            else if(constraint.type == CD && !is_deleter(b_x, constraint.factProxy))
                blocks_orderings.remove_constraint(predecessor, b_x, constraint);
            else if (constraint.type == DP && !is_producer(b_x, constraint.factProxy))
                blocks_orderings.remove_constraint(predecessor, b_x, constraint);
        }
//        write_log_file("done\n");
        if(blocks_orderings.get_constraints( predecessor, b_x).empty()) {
            del_predecessors.push_back(predecessor);
        }
//        write_log_file("*done\n");
    }
    for(int successor: del_successors) {
        block->delete_successor(successor);
        get_block(successor)->delete_predecessor(b_x);
    }

    for(int predecessor: del_predecessors) {
        block->delete_predecessor(b_x);
        get_block(predecessor)->delete_successor(b_x);
    }
}
/**
 * @brief delete all the orderings of a block
 * @param b_x the block index
 */
void BlockDeorderPlan :: delete_block_ordering(int b_x){
    map<int, Block>::iterator itr;
    for (itr = blocks.begin(); itr != blocks.end(); ++itr) {
       int b = itr->second.id;
       blocks_orderings.erase_ordering(b, b_x);
       blocks_orderings.erase_ordering(b_x, b);
    }
}

/**
 * @brief delete all the predecessors successors of a block
 * @param b_x the block index
 */
void BlockDeorderPlan :: delete_block_predecessor_successor(int b_x){
    Block* block_x = get_block(b_x);
    for(int predecessor: block_x->predecessors){
        get_block(predecessor)->delete_successor(b_x);
    }
    for(int successor: block_x->successors){
        get_block(successor)->delete_predecessor(b_x);
    }
}

/**
 * @brief find the preconditions of a set of  blocks
 * @param related_blocks a set of  blocks
 * @return a vector of FactProxy as the preconditions
 */
vector<FactProxy> BlockDeorderPlan :: find_preconditions (vector<int> related_blocks){
    vector<FactProxy> preconditions;

    for(int b_i : related_blocks) {
        Block* block_internal = get_block(b_i);
        if (block_internal->is_temp_removed()) continue;
        vector<FactProxy> i_preconditions =   block_internal->get_preconditions();
        vector<FactProxy> internal_pcs;

        for (int predecessor : block_internal->predecessors) {
            if (get_block(predecessor)->is_temp_removed() || !is_in_vector(predecessor, &related_blocks)) continue;
            for (OrderingConstraint constraint : blocks_orderings.get_constraints(predecessor, b_i)) {
                if (constraint.type == PC)
                    internal_pcs.push_back(constraint.factProxy);
            }
        }
        for (int i = 0; i < i_preconditions.size(); i++) {
            bool internal_pc_found = false;
            for (int j = 0; j < internal_pcs.size(); j++) {
                if (internal_pcs[j] == i_preconditions[i]) {
                    internal_pcs.erase((internal_pcs.begin() + j));
                    internal_pc_found = true;
                    break;
                }
            }
            if(!internal_pc_found && count(preconditions.begin(), preconditions.end(), i_preconditions[i]) == 0)
                preconditions.push_back(i_preconditions[i]);
        }
    }
    return preconditions;
}


/**
 * @brief find the effects of a set of  blocks
 * @param related_blocks a set of  blocks
 * @return a vector of EffectProxy as the effects
 */
vector<EffectProxy> BlockDeorderPlan :: find_effects(vector<int> related_blocks){

    map<int, vector< pair<int, EffectProxy>>> effects_map;
    vector<EffectProxy> effects;
    vector<bool> visited(related_blocks.size(), false);
    vector<int> queue;

    for(int j = 0 ; j < related_blocks.size() ; j++) {
        if (visited[j] || get_block(related_blocks[j])->is_temp_removed()) continue;
        queue.push_back(j);

        while (!queue.empty()) {
            int a = queue.back();
            int b = related_blocks[a];
            Block *block_x = get_block(b);
            visited[a] = true;
            queue.pop_back();
            vector<EffectProxy> i_effects =  block_x->get_effects();
            for (EffectProxy effectProxy: i_effects) {
                pair<int, EffectProxy> p(b, effectProxy);
                FactProxy factProxy = effectProxy.get_fact();
                if (effects_map.count(factProxy.get_variable().get_id())) {
                    bool pass = false;
                    auto itr = effects_map.find(factProxy.get_variable().get_id());
                    vector<pair<int, EffectProxy>> others = itr->second;
                    for (int i = others.size() -1; i >=0 ; i--) {
                        if (is_successor(others[i].first, b, true)) {
                            itr->second.erase(itr->second.begin() + i);
                        } else if (is_successor(b, others[i].first, true)){
                            pass = true;
                            break;
                        }
                    }
                    if (!pass)
                        itr->second.push_back(p);
                } else {
                    vector<pair<int, EffectProxy>> v(1, p);
                    effects_map.insert(pair<int, vector<pair<int, EffectProxy>>>(factProxy.get_variable().get_id(), v));
                }
            }
            for(int k = 0; k< related_blocks.size(); k++){
                if(visited[k] || blocks_orderings.is_transitive(b, related_blocks[k]) || !blocks_orderings.has_any_ordering(b, related_blocks[k]))
                    continue;
                queue.push_back(k);
            }
        }
    }
    map<int, vector< pair<int, EffectProxy>>> ::iterator itr;
    int count = 0;
    for (itr = effects_map.begin(); itr != effects_map.end(); itr++){
        for (int i =0 ; i < itr-> second.size() ;i++){
            bool add= true;
            EffectProxy effectProxy = itr->second[i].second;
            for(EffectProxy e: effects){
                if(is_equal(e, effectProxy)) add = false;
            }
            if(add) effects.push_back(effectProxy);
        }
    }
    return effects;
}

/// check whether two effect is same
//bool BlockDeorderPlan :: is_equal(EffectProxy e1, EffectProxy e2){
//    if(e1.get_fact() != e2.get_fact())
//        return false;
//    vector<FactProxy> facts;
//
//    for(int i=0; i <e1.get_conditions().size(); i++){
//        facts.push_back(e1.get_conditions()[i]);
//    }
//
//    for(int j=0; j < e2.get_conditions().size(); j++){
//        FactProxy f = e2.get_conditions()[j];
//        bool found = false;
//        for(int i=0; i<facts.size(); i++){
//            if(f.get_pair() == facts[i].get_pair()){
//                found = true;
//                facts.erase(facts.begin()+i);
//                break;
//            }
//        }
//        if(!found) return false;
//    }
//    if(facts.empty()) return true;
//    return false;
//}

///check whether a block b_x consumes a fact factProxy
bool BlockDeorderPlan :: is_consumer(int b_x, FactProxy factProxy){
    Block* block = get_block(b_x);
    if(block->has_precondition(factProxy))
        return true;
//    if(block->has_precondition(factProxy) || is_conditional_consumer(b_x, factProxy))
//        return true;
    return false;
}

///retrieves the fact that a block b_x consumes conditionally
vector<FactProxy> BlockDeorderPlan :: get_conditional_consuming_facts(int b_x){
    vector<FactProxy> facts;
    Block *block = get_block(b_x);
    if(block->is_primary()) {
        for (int predecessor: block->predecessors) {
            for (OrderingConstraint constraint: blocks_orderings.get_constraints(predecessor, b_x)) {
                if (constraint.type == PC && constraint.is_conditional && !constraint.is_incidental)
                    facts.push_back(constraint.factProxy);

            }
        }
    } else {
        for(int b_i : block->internal_blocks) {
            Block* block_internal = get_block(b_i);
            if (block_internal->is_temp_removed()) continue;
            vector<FactProxy> i_cond_consumers = get_conditional_consuming_facts(b_i);
            vector<FactProxy> internal_pcs;

            for (int predecessor : block_internal->predecessors) {
                if (get_block(predecessor)->is_temp_removed()) continue;
                if(is_in_vector(predecessor, &block->internal_blocks)){
                    for (OrderingConstraint constraint : blocks_orderings.get_constraints(predecessor, b_i)) {
                        if (constraint.type == PC && !constraint.is_incidental)
                            internal_pcs.push_back(constraint.factProxy);
                    }
                }

            }
            for (int i = 0; i < i_cond_consumers.size(); i++) {
                bool internal_pc_found = false;
                for (int j = 0; j < internal_pcs.size(); j++) {
                    if (internal_pcs[j] == i_cond_consumers[i]) {
                        internal_pcs.erase((internal_pcs.begin() + j));
                        internal_pc_found = true;
                        break;
                    }
                }
                if(!internal_pc_found && count(i_cond_consumers.begin(), i_cond_consumers.end(), i_cond_consumers[i]) == 0)
                    facts.push_back(i_cond_consumers[i]);
            }
        }
    }
    return facts;

}

///
//bool BlockDeorderPlan :: is_conditional_consumer_of_different_value(int b_x, FactProxy f){
//   for(FactProxy factProxy: get_conditional_consuming_facts(b_x)){
//       if(is_fact_of_same_var_diff_val(factProxy, f))
//           return true;
//   }
//   return false;
//}
//
//bool BlockDeorderPlan :: is_conditional_consumer (int b_x, FactProxy factProxy){
//    Block *block = get_block(b_x);
//    for(FactProxy f: get_conditional_consuming_facts(b_x)){
//        if(is_equal(f, factProxy))
//            return true;
//    }
//    return false;
//}

/*
 * returns true if the two facts have same variable, but different value
 */
//bool BlockDeorderPlan :: is_fact_of_same_var_diff_val(FactProxy f1, FactProxy f2){
//    if(f1.get_variable().get_id() == f2.get_variable().get_id() && f1.get_value() != f2.get_value())
//        return true;
//    return false;
//}

/*
 * returns true if the block is a producer of a fact that has same variable as the factproxy but different value
 */
bool BlockDeorderPlan :: is_producer_except_val(int b_x, FactProxy factProxy){
    Block *block = get_block(b_x);
    vector<EffectProxy> applicable_effect;
    for(EffectProxy effectProxy: block->get_effects()){
        if(is_fact_of_same_var_diff_val(effectProxy.get_fact(),factProxy)) {
            if (is_applicable(block, effectProxy) && !is_consumer(b_x, effectProxy.get_fact()))
                return true;
        }
    }
    return false;
}

bool BlockDeorderPlan :: is_active(int b_x, FactProxy factProxy){
    Block *block = get_block(b_x);
    vector<EffectProxy> applicable_effect;
    for(EffectProxy effectProxy: block->get_effects(factProxy)){
        if(is_applicable(block, effectProxy))
            return true;
    }
    return false;
}

//bool BlockDeorderPlan :: is_active_for_different_value(int b_x, FactProxy factProxy){
//    Block *block = get_block(b_x);
//    vector<EffectProxy> applicable_effect;
//    vector<EffectProxy> related_effects;
//    for(EffectProxy effectProxy: block->get_effects()){
//        if(is_fact_of_same_var_diff_val(effectProxy.get_fact(), factProxy)){
//            if(is_applicable(block, effectProxy))
//                return true;
//        }
//    }
//    return false;
//}

bool BlockDeorderPlan ::is_producer(int b_x, FactProxy factProxy) {
    if(get_block(b_x)->is_producer(factProxy))
        return true;
    return false;
}

bool BlockDeorderPlan ::is_deleter(int b_x, FactProxy factProxy) {
    Block* block = get_block(b_x);
    if(get_block(b_x)->is_deleter(factProxy))
        return true;
    return false;
}

bool BlockDeorderPlan :: is_deleter_op(int opId, FactProxy f){
    OperatorProxy operatorProxy =task_proxy.get_operators()[opId];
    bool is_deleter = false;
    for(EffectProxy effectProxy: operatorProxy.get_effects()){
        FactProxy factProxy = effectProxy.get_fact();
        if(is_fact_of_same_var_diff_val(factProxy, f)){
            is_deleter = true;
            for(FactProxy f1: operatorProxy.get_preconditions()){
                if(is_fact_of_same_var_diff_val(f1, f)){
                    is_deleter = false;
                    break;
                }
            }
            break;
        }
    }
    return is_deleter;
}
//
//int BlockDeorderPlan ::    is_producer_or_deleter(int b_x, FactProxy factProxy){
//    Block *block = get_block(b_x);
//    if(block->is_primary()){
//        if(is_active(b_x, factProxy))
//            return 1;
//        else if (is_active_for_different_value(b_x, factProxy))
//            return -1;
//        else
//            return 0;
//    }
//    vector<int> producers, others;
//    Graph graph = block->effect_map.at(factProxy.get_variable().get_id());
//    map<int, bool> visited;
//    for(int i : block->internal_blocks){
//        visited.insert(pair<int, bool>(i, false));
//    }
//    for(auto itr: graph.get()){
//        Block* internal_block= get_block(itr.first);
//        visited.at(internal_block->id)= true;
//        bool add = true;
//        if(is_producer(internal_block->id,factProxy)){
//            if(is_active(internal_block->id, factProxy)) {
//
//                for (int i = producers.size(); i > 0; i--) {
//                    if (graph.is_successor(i, internal_block->id))
//                        producers.erase(producers.begin() + i);
//                    else if (graph.is_successor(internal_block->id, i))
//                        add = false;
//                }
//                if (add) {
//                    for (int i = others.size(); i > 0; i--) {
//                        if (graph.is_successor(i, internal_block->id))
//                            others.erase(producers.begin() + i);
//                        else if (graph.is_successor(internal_block->id, i))
//                            add = true;
//                    }
//                }
//                if (add)
//                    producers.push_back(internal_block->id);
//            }
//        } else {
//            if(is_active_for_different_value(internal_block->id, factProxy)) {
//                for (int i = others.size(); i > 0; i--) {
//                    if (graph.is_successor(i, internal_block->id))
//                        others.erase(producers.begin() + i);
//                    else if (graph.is_successor(internal_block->id, i))
//                        add = false;
//                }
//                if (add) {
//                    for (int i = producers.size(); i > 0; i--) {
//                        if (graph.is_successor(i, internal_block->id)) {
//                            producers.erase(producers.begin() + i);
//                        } else if (graph.is_successor(internal_block->id, i)) {
//                            add = false;
//                        }
//                    }
//                }
//                if (add)
//                    others.push_back(internal_block->id);
//            }
//        }
//    }
//
//    if(producers.size() > 0 && others.size() == 0)
//        return 1;
//    else if(others.size() > 0)
//        return -1;
//    else
//        return 0;
//}



vector<EffectProxy> BlockDeorderPlan :: get_applicable_effects(int b_x){
    Block *block = get_block(b_x);
    vector<EffectProxy> applicable_effects;
    for(EffectProxy effectProxy: block->effects){
        if(is_applicable(block, effectProxy))
            applicable_effects.push_back(effectProxy);
    }
    return applicable_effects;
}

vector<EffectProxy> BlockDeorderPlan :: get_conditional_applicable_effects(Block* block_x){
    vector<EffectProxy> applicable_effects;
    for(EffectProxy effectProxy: block_x->effects){
        if(effectProxy.get_conditions().size() == 0) continue;
        if(is_applicable(block_x, effectProxy))
            applicable_effects.push_back(effectProxy);
    }
    return applicable_effects;
}

bool BlockDeorderPlan ::is_consumer_of_different_value(Block* block_x, FactProxy f) {
//    if (block_x->has_precondition_of_different_value(f) ||
//    is_conditional_consumer_of_different_value(block_x->id, f))
    if (block_x->has_precondition_of_different_value(f))
        return true;
    return false;
}

bool BlockDeorderPlan :: is_candidate_producer(int b_x, FactProxy factProxy){
    if(is_consumer(b_x, factProxy))
        return false;
    bool candidate = true;
    Block *block = get_block(b_x);
    if(get_producing_effect(b_x, factProxy) > -1)
        return true;
    return false;
}

int BlockDeorderPlan :: get_producing_effect(int b_x, FactProxy factProxy){
    int candidate = -1;
    Block *block = get_block(b_x);

    for(int i = 0; i< block->effects.size(); i++){
        EffectProxy effectProxy = block->effects[i];
        if(effectProxy.get_fact().get_pair() != factProxy.get_pair())
            continue;
        bool is_candidate = true;

//        if(effectProxy.get_conditions().size() > 0) {
//            for (FactProxy f: effectProxy.get_conditions()) {
//                if (!has_causal_link(b_x, f)) {
//                    int i = find_earliest_candidate_producer(b_x, f, vector<int>(0));
//                    if (i == -1) {
//                        is_candidate = false;
//                        break;
//                    }
//                }
//            }
//        }
        if(is_candidate)
            return i;
    }
    return -1;
}

int BlockDeorderPlan :: find_earliest_candidate_producer(int b_x, FactProxy f, vector<int> exclude = vector<int>(0)) {
    map<int, bool>  visited = get_boolean_map();
    vector<int> producers, deleters, candidate;
    find_earliest_candidate_producer(b_x, f, exclude, &visited, &producers, &deleters);
//    cout <<"0";

    if(producers.empty()){
        if(deleters.size() > 0) return -2;
        return -1;
    }
    for(int p:producers){
        bool is_cand = true;
        for(int d: deleters){
            if(is_successor(p,d) || !is_successor(d,p)){
                is_cand= false;
                break;
            }
        }
        if(is_cand) candidate.push_back(p);

    }
    producers.clear();
    producers = candidate;
    candidate.clear();
    for(int c= producers.size()-1; c >= 0; c--){
        bool is_cand = true;
        for(int i =0 ; i < c; i++){
            if(is_successor(producers[i], producers[c])){
                is_cand = false;
                break;
            }

        }
        if(is_cand) candidate.push_back(producers[c]);
    }

    return candidate[0];
}

/**
 * @brief searches a candidate producer of a fact f upwards from a block.
 * @param b_x the index of the block
 * @param f the factproxy
 * @param search_type 0, 1, 2 for poducer, consumer, deleter
 */
int BlockDeorderPlan :: find_earliest_candidate_producer(int b_x, FactProxy f, vector<int> exclude ,
                                                         map<int, bool> *visited, vector<int> *producers, vector<int> *deleters){
    int earlier_candidate_producer = -1;
    visited->at(b_x) = true;
    for (int b_i : get_block(b_x)->predecessors) {

        Block *block_i = get_block(b_i);
//        write_log_file("init: " + to_string(b_i) + "\n ");
        if (block_i->type == TEMP_INTERNAL || block_i->type == TEMP_REMOVED ||  visited->at(b_i) || blocks_orderings.is_transitive(b_i, b_x))
            continue;
//        write_log_file("check: " + to_string(b_i) + "\n ");
        if (b_i != init_block && is_deleter(b_i, f) && !is_in_vector(b_i, &exclude)){
//            write_log_file("deleters: " + to_string(b_i) + "\n ");
            deleters->push_back(b_i);
            continue;
        }

        if (!is_in_vector(b_i, &exclude) && is_candidate_producer(b_i, f)) {
//            write_log_file("producers: " + to_string(b_i) + "\n ");
            producers->push_back(b_i);

        }
        find_earliest_candidate_producer(b_i, f, exclude, visited, producers,deleters);
    }

    return earlier_candidate_producer;
}


int BlockDeorderPlan :: find_earliest_incidental_candidate_producer(int b_x, FactProxy f, map<int, bool> *visited, int &has_deleter){  // not ok

    int earlier_candidate_producer = -1;
    visited->at(b_x) = true;
    for (int b_i : get_block(b_x)->predecessors) {
        Block *block_i = get_block(b_i);
        if (block_i->type == TEMP_INTERNAL || blocks_orderings.is_transitive(b_i, b_x) || visited->at(b_i))
            continue;
        if (b_i != init_block && is_deleter(b_i, f)){
            map<int, bool> temp_visited = get_boolean_map();
            mark_all_visited_predecessors(b_i, &temp_visited, visited);
            temp_visited.clear();
            has_deleter = 1;
            continue;
        }

        if (is_producer(b_i, f)) {
            earlier_candidate_producer = b_i;
        }
        int status = find_earliest_incidental_candidate_producer(b_i, f, visited, has_deleter);
        if (status > -1) {
            return status;
        } else if( earlier_candidate_producer > -1){
            return earlier_candidate_producer;
        }
    }
    return earlier_candidate_producer;
}

 int BlockDeorderPlan :: find_earliest_incidental_candidate_producer(int b_x, FactProxy f) {
    map<int, bool> visited = get_boolean_map();
    Block *block = get_block(b_x);
    int earlier_candidate_producer = -1;
    int has_deleter = 0;
    vector<int> predecesssors = block->predecessors;
    while(predecesssors.empty()){
        int p = predecesssors.back();
        predecesssors.pop_back();
        if (block->type == TEMP_INTERNAL || blocks_orderings.is_transitive(p, b_x) || visited.at(p))
            continue;

        if (p != init_block && is_deleter(p, f)){
            map<int, bool> temp_visited = get_boolean_map();
            mark_all_visited_predecessors(p, &temp_visited, &visited);
            temp_visited.clear();
            has_deleter = 1;
            continue;
        }

        if (is_producer(p, f)) {
            earlier_candidate_producer = p;
        }
        int producer = find_earliest_incidental_candidate_producer(b_x, f, &visited, has_deleter);
        if(producer > -1){
            return producer;
        }
        else if(earlier_candidate_producer > -1)
            return earlier_candidate_producer;

    }
    if(block->is_internal() && !has_deleter){
        return find_earliest_candidate_producer(block->parent, f);
    }
    return -1;

}

//EffectProxy BlockDeorderPlan :: get_effect(int b_x, FactProxy factProxy){
//    Block *block = get_block(b_x);
//    vector<EffectProxy> applicable_effect;
//    for(EffectProxy effectProxy: block->get_effects(factProxy)){
//        if(is_applicable(b_x, effectProxy))
//            return
//    }
//    if(applicable_effect.size() == 1) return true;
//    return false;
//}

bool BlockDeorderPlan :: is_applicable(Block* block_x, EffectProxy effectProxy){
    if(effectProxy.get_conditions().size() > 0) {
        for (FactProxy factProxy: effectProxy.get_conditions()) {
            if (!has_causal_link(block_x->id, factProxy)) return false;
        }
    }
    return true;

}

bool BlockDeorderPlan :: has_causal_link(int b_x, FactProxy factProxy){
    int producer = get_producer(b_x, factProxy);
//    cout << "producer "<< producer << endl;
    Block* block = get_block(b_x);
    if(producer == -1 && block->is_internal()){
        return has_causal_link(block->parent, factProxy);
    } else if(producer >= 0){
//        cout << "true";
        return true;
    }

//    cout <<"false";
    return false;
}

/**
 * @brief checks whether the block of index b_x is a deleter of a fact f
 * @param b_x index of the block
 * @param factProxy the fact
 * @return true if the block is a deleter, false otherwise
 */
//bool BlockDeorderPlan :: is_deleter(int b_x, FactProxy factProxy){
//    Block* block = get_block(b_x);
//    if(block->has_precondition_of_different_value(factProxy))
//        return false;
//    for(EffectProxy effectProxy: block->get_effects()){
//        if(is_fact_of_same_var_diff_val(effectProxy.get_fact(), factProxy)){
//            if(is_applicable(b_x, effectProxy)) {
//                return true;
//            }
//        }
//    }
//    return false;
//}

/**
 * @return a map of int to bool for blocks
 */
map<int, bool> BlockDeorderPlan :: get_boolean_map() {
    map<int, bool> visited;
    for (auto itr : blocks) {
        visited.insert(pair<int, bool>(itr.first, false));
    }
    return visited;
}

/**
 * @return a map of int to int for blocks
 */
map<int, int> BlockDeorderPlan :: get_int_map() {
    map<int, int> connected;
    map<int, Block>::iterator itr;
    for (itr = blocks.begin(); itr != blocks.end(); ++itr) {
        connected.insert(pair<int, int>(itr->first, 0));
    }
    return connected;
}

/**
 * @brief checks whether the block successor is the successor of the block b_x.
 * @param b_x index of the block
 * @param successor index of another block
 * @return true if the block successor is the successor of the block internal_x, false otherwise
 */
bool BlockDeorderPlan :: is_successor (int b_x, int successor, bool allow_temp_internal){
//    cout <<"\t\t is successor " << b_x << " "<< successor << endl;
    map<int, bool> visited = get_boolean_map();
    return is_successor(b_x, successor, &visited, allow_temp_internal);
}

/**
 * @brief an assistant function of is_immediate_successor(out, b_x, successor)
 */
bool BlockDeorderPlan :: is_successor (int b_x, int successor, map<int, bool> *visited ,  bool allow_temp_internal){
    if (blocks_orderings.has_any_ordering(b_x, successor)) return true;
    visited->at(b_x) = true;
    Block* block_x = get_block(b_x);
    for (int b_i : block_x->successors){
        Block* block_i = get_block(b_i);
        if(!allow_temp_internal && block_i->type == TEMP_INTERNAL) continue;

        if(visited->at(b_i) || blocks_orderings.is_transitive(b_x, b_i) ) continue;

        if (is_successor(b_i, successor, visited, allow_temp_internal))
            return true;
    }
    return false;
}

/**
 * @brief assign the value of the connection to the the block id of the connection map. also assign the same connection value
   for the internal blocks
 * @param b_x the block id
 * @param connected the connection map
 * @param connection the connection 0 0r 1 0r 2
 */
void BlockDeorderPlan :: mark_as_connected (int b_x, map<int, int> *connected, int connection){
    connected->at(b_x)= connection;
    if ( blocks.at(b_x).is_primary()){
        return;
    }
}

/**
 * @brief delete an element from an int vector
 * @param v the reference of the vector
 * @param elem the actual int element
 */


//void BlockDeorderPlan :: initialize_block_predecessor_successor(){
//    map<int, bool> visited_map = get_boolean_map();
//    vector<int> queue;
//    queue.push_back(init_block);
//    vector<int> candidates;
//
//    for (auto itr : blocks)
//        candidates.push_back(itr.first);
//
//    while(!queue.empty()){
//        int i = queue.front();
//       // delete_element_from_vector(&candidates, i);
//        queue.erase(queue.begin());
//        for (int j : candidates) {
//            if (blocks_orderings.has_any_ordering(i, j)){
//                if(!is_in_vector(j, &queue))
//                    queue.push_back(j);
//                get_block(i)->add_successor(j);
//                get_block(j)->add_predecessor(i);
//            }
//        }
//
//    }
//}

//void BlockDeorderPlan :: find_deleter_producer(int b_x, FactProxy f){
//    Block* block = get_block(b_x);
//    vector<int> candidates;
//    for (auto itr: blocks)
//        candidates.push_back(itr.first);
//
//    /// filter candidates
//    for (int i = candidates.size() - 1; i >= 0; i--) {
//        int id = candidates[i];
//        Block *block_i = get_block(id);
//        if (block->parent != block_i->parent || block_i->type == TEMP_INTERNAL|| has_any_ordering(b_x, id || id == init_block
//        || id == goal_block))
//            candidates.erase(candidates.begin() + i);
//    }
//    vector<int> queue;
//    map<int, bool>  visited = get_boolean_map();
//    queue.push_back(b_x);
//    while (!queue.empty()){
//        int curr_b = queue.front();
//        visited.at(curr_b) = true;
//        for(auto itr : blocks){
//            int b_i = itr.first;
//            if(is_transitive(b_i, curr_b) || !has_any_ordering(b_i, curr_b) || visited.at(b_i) )
//                continue;
//            if(itr.second.is_deleter(f))
//                return b_i;
//
//            queue.push_back(b_i);
//        }
//        queue.erase(queue.begin());
//    }
//    return -1;
//}



/**  @brief mark the all the predecessors or successors of the block b_x as 1 in the connected map. here, if the block is an outer block,
    only the outer blocks will be considered. if it is an internal block, only the internal blocks of
     its parents will be considered. Other blocks will be marked as irrelevant -1.
     @param b_x the block id
     @param connection if connection is 1 then mark all the predecessors. if 2 then mark successors
     @param connected the reference of connected block
     */
void BlockDeorderPlan :: mark_predecessors(int b_x, map<int, int> *connected, map<int, bool> *visited) {
    Block *block = get_block(b_x);
    visited->at(b_x) = true;

    for(int predecessor: block->predecessors){
        if(visited->at(predecessor) || blocks_orderings.is_transitive(predecessor, b_x) || get_block(predecessor)->is_temp_internal()) continue;
        connected->at(predecessor) = 1;
        mark_predecessors(predecessor, connected, visited);
    }
}

void BlockDeorderPlan :: mark_successors(int b_x, map<int, int> *connected, map<int, bool> *visited) {
    Block *block = get_block(b_x);
    visited->at(b_x) = true;

    for(int successor: block->successors){
        if(visited->at(successor) || blocks_orderings.is_transitive(b_x, successor) || get_block(successor)->is_temp_internal()) continue;
        connected->at(successor) = 2;
        mark_successors(successor, connected, visited);
    }
}

void BlockDeorderPlan :: mark_all_visited_predecessors(int b_x, map<int, bool> *visited, map<int, bool> *mark_visited) {
    Block *block = get_block(b_x);
    visited->at(b_x) = true;

    for(int predecessor: block->predecessors){
        if(visited->at(b_x)) continue;
        mark_visited->at(predecessor) = 1;
        mark_all_visited_predecessors(predecessor, visited, mark_visited);
    }
}
/**
 * @brief make a connection map where predecessors, successors and parallel blocks are marked 1, 2 and 0 respectively.
    if the block is an outer block, only the outer blocks will be considered. if it is an internal block, only the
    internal blocks of its parents will be considered. Other blocks will be marked as irrelevant -1.
 * @param b_x the block id
 * @return the connected map
 */
map<int, int> BlockDeorderPlan :: get_connection_map (int b_x){
    map<int, int> connected = get_int_map();
    map<int, bool> visited = get_boolean_map();

    connected[b_x] = -1;
    Block *block = get_block(b_x);

    vector<int> candidates;

    for (auto itr: blocks)
        candidates.push_back(itr.first);

    /// mark -1 all irrelevent blocks
    for (int i = candidates.size() - 1; i >= 0; i--){
        int id = candidates[i];
        Block* block_i = get_block(id);
        if (block->parent !=  block_i->parent){
            mark_as_connected(id, &connected, -1);
            candidates.erase(candidates.begin()+i);
            continue;
        }
        else if (block_i->type == TEMP_INTERNAL) {
            mark_as_connected(id, &connected, -1);
            candidates.erase(candidates.begin()+i);
            continue;
        }
    }
    mark_predecessors(b_x, &connected, &visited);
    mark_successors(b_x, &connected, &visited);
    return connected;
}

///// make the temporary block b_x permanent
void BlockDeorderPlan :: make_temporary_block_permanent(int b_x) {

        if (print_details) {
            write_log_file("\t* making temporary block permanent block" + to_string(b_x) + "\n");
            write_log_file(get_block(b_x)->print_preconditions_and_effects());
        }

    Block *block = get_block(b_x);
    for(int internal : block->internal_blocks){
        Block* block_internal = get_block(internal);
        block_internal->set_type_normal();
        block_internal->is_temporary = false;
    }
    block->is_temporary = false;
}

/**
 * @return a linear order bdpop
 */
Plan BlockDeorderPlan :: get_total_order_plan(){
    Plan plan;
    map<int, bool> visited = get_boolean_map();

    vector<int> queue;
    for(auto itr: blocks){
        int b = itr.first;
        if(visited.at(itr.first) || itr.second.is_internal() || is_in_vector(b, &queue)) continue;

        topological_sort(b, &queue, &visited);
    }

    while(!queue.empty()){
        int b_x = queue.back();
        queue.pop_back();
        plan.push_back(get_block(b_x)->op_index);
    }
    return plan;
}

Plan BlockDeorderPlan :: get_total_order_plan(int b_x){
    Plan plan;
    map<int, bool> visited = get_boolean_map();
    Block* block = get_block(b_x);

    vector<int> queue;
    for(int b: block->internal_blocks){
        Block* internal_block = get_block(b);
        if(visited.at(b)  || is_in_vector(b, &queue)) continue;
        topological_sort(b, &queue, &visited);
    }

    while(!queue.empty()){
        int b_x = queue.back();
        queue.pop_back();
        plan.push_back(get_block(b_x)->op_index);
    }
    return plan;
}

void BlockDeorderPlan :: topological_sort(int b_x, vector<int> *queue, map<int, bool> *visited){
    Block *block = get_block(b_x);
    visited->at(b_x) = true;
    for(int successor: block->successors){
        if(visited->at(successor) || is_in_vector(successor, queue)) continue;
        topological_sort(successor, queue, visited);
    }
    if(block->is_primary() && block->id != init_block && block->id != goal_block)
        queue->push_back(b_x);
    else{
        for(int internal: block->internal_blocks){
            if(visited->at(internal)) continue;
            topological_sort(internal, queue, visited);
        }
    }
}

vector<int> BlockDeorderPlan :: get_initial_blocks_of_a_block(int b_x){
    Block *block = get_block(b_x);
    vector<int> initial_blocks;
    for (int i: block->internal_blocks){
        if(get_block(i)->predecessors.size()== 0)
            initial_blocks.push_back(i);
    }
    return initial_blocks;
}

/**
 * @brief finds the producer of the fact f of a consumer in a causal link
 * @param consumer the index of the block consumer
 * @param f the fact
 * @return the index of the producer if there is any causal link of the consumer of the fact f, -1 otherwise
 */
int BlockDeorderPlan :: get_producer (int consumer, FactProxy f){
    Block* block_consumer = get_block(consumer);
    for (int b_x: block_consumer->predecessors){
//        cout << b_x << endl;
        Block* block_x = get_block(b_x);
        if ( block_x->type == TEMP_INTERNAL)
            continue;
        for(OrderingConstraint constraint : blocks_orderings.get_constraints(b_x, consumer)){
            if (constraint.type == PC && is_equal(constraint.factProxy, f)) return b_x;
        }
    }
    return -1;
}

bool BlockDeorderPlan :: is_producer_in_block(int b_x, int parent, FactProxy f){
    return is_producer_in_block(get_block(b_x), get_block(parent), f);
}
/**
 * check whether an internal block is a producer of a fact f in a composite block
 * @param block the id of the internal block
 * @param parent the id of the parent block
 * @param f the fact
 * @return true if block is the producer, false otherwise
 */
bool BlockDeorderPlan :: is_producer_in_block(Block* block, Block* parent, FactProxy f){
    if (!is_producer(block->id, f)) return false;
    //TODO make connected map for internal block
    for(int internal : parent->internal_blocks){
        Block* block_internal = get_block(internal);
        if(is_deleter(internal, f) && !is_successor(internal, block->id, false))
            return false;
    }
    return true;
}


/**
 * @brief searches a producer of a fact that has same variable as the factproxy f but with different value upwards from a block.
 * @param b_x the index of the block
 * @param f the factproxy
 * @param search_type 0, 1, 2 for producer, consumer, deleter
 */
int BlockDeorderPlan :: search_upward_producer_except_value(int b_x, FactProxy f){
    vector<int> queue;
    map<int, bool>  visited = get_boolean_map();
    queue.push_back(b_x);
    while (!queue.empty()){
        int curr_b = queue.front();
        visited.at(curr_b) = true;
        vector<int> immediate_predecessors = get_block(curr_b)->predecessors;
        for(int b_i : immediate_predecessors){
            Block* block_i = get_block(b_i);
            if( block_i->type == TEMP_INTERNAL || blocks_orderings.is_transitive(b_i, curr_b) || visited.at(b_i) || b_i == init_block)
                continue;
            if(is_producer_except_val(b_i, f))
                return b_i;
            if(!is_in_vector(b_i, &queue))
                queue.push_back(b_i);
        }
        queue.erase(queue.begin());
    }
    return -1;
}


/**
 * @brief searches a consumer of a fact f upwards from a block.
 * @param b_x the index of the block
 * @param f the factproxy
 * @param search_type 0, 1, 2 for producer, consumer, deleter
 */
int BlockDeorderPlan :: search_upward(int b_x, FactProxy f, int search_type){
    vector<int> queue;
    map<int, bool>  visited = get_boolean_map();
    queue.push_back(b_x);
    while (!queue.empty()){
        int curr_b = queue.front();
        visited.at(curr_b) = true;
        vector<int> immediate_predecessors = get_block(curr_b)->predecessors;
        for(int b_i : immediate_predecessors){
            Block* block_i = get_block(b_i);
            if( block_i->type == TEMP_INTERNAL || blocks_orderings.is_transitive(b_i, curr_b) || visited.at(b_i) || b_i == init_block)
                continue;
            if((search_type == 0 && is_producer(b_i, f)) ||  (search_type == 1 && is_consumer(b_i, f))
            || (search_type == 2 && is_deleter(b_i, f)))
                return b_i;
            if(!is_in_vector(b_i, &queue))
                queue.push_back(b_i);
        }
        queue.erase(queue.begin());
    }
    return -1;
}

/**
 * @brief searches a producer/consumer/deleter of a fact f downwards from a block.
 * @param b_x the index of the block
 * @param f the factproxy
 * @param search_type 0, 1, 2 for poducer, consumer, deleter
 */
int BlockDeorderPlan :: search_downward(int b_x, FactProxy f, int search_type){
    vector<int> queue;
    map< int, bool>  visited = get_boolean_map();
    queue.push_back(b_x);
    while (!queue.empty()){
        int curr_b = queue.front();
        visited.at(curr_b) = true;
        vector<int> immediate_successors = get_block(curr_b)->successors;
        for(int b_i: immediate_successors){
            Block* block_i = get_block(b_i);
            if( block_i->type == TEMP_INTERNAL || blocks_orderings.is_transitive(curr_b, b_i) || visited.at(b_i))
                continue;
            if((search_type == 0 && is_producer(b_i, f)) ||  (search_type == 1 && is_consumer(b_i, f))
               || (search_type == 2 && is_deleter(b_i, f)))
                return b_i;
            if(!is_in_vector(b_i, &queue))
                queue.push_back(b_i);
        }
        queue.erase(queue.begin());
    }
    return -1;
}

/**
 * @brief searches all the consumers of a fact f downwards from a block.
 * @param b_x the index of the block
 * @param f the factproxy
 * @param consumers the vector of indices of found consumers
 */
void BlockDeorderPlan :: search_consumers_downward(int b_x, FactProxy f, vector<int> *consumers, map<int, bool> *visited_blocks){
//    auto start = chrono:: high_resolution_clock::now();
    vector<int> immediate_successors = get_block(b_x)->successors;
    for(int b_i : immediate_successors){
        Block* block_i = get_block(b_i);
        if( block_i->type == TEMP_INTERNAL || b_i == goal_block) continue;
        if(blocks_orderings.have_constraint(b_x, b_i, f, PC)) consumers->push_back(b_i);
    }
//    auto end = chrono:: high_resolution_clock::now();
//    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
//    //cout << "<t> searching consumer downward : "<< duration.count() << " ms" << endl;
}



/**
 * @brief find out the in-between/intermediate  blocks between two temporary  blocks
 * @param temp_x the index of first temporary block
 * @param temp_y the index of second temporary block
 * @return the vector of in-between  blocks
 */
vector<int> BlockDeorderPlan :: find_in_between_blocks_for_temp_blocks (int temp_x, int temp_y){
  //  auto start = chrono:: high_resolution_clock::now();
    if(print_details)
        write_log_file("\t\tfinding in-between  blocks between two temp  blocks " + to_string(temp_x) + " and " +
                               to_string(temp_y) + "\n");
    vector<int> in_between_blocks;
    map<int, bool> visited = get_boolean_map();
    Block* block_x = get_block(temp_x);
    for(int i : block_x->successors){
        Block* block_i = get_block(i);
        if ( block_i->type == TEMP_INTERNAL || i == goal_block || blocks_orderings.is_transitive(temp_x, i))
            continue;
        find_intermediate_blocks(i, temp_y, &in_between_blocks, &visited);
    }
    sort(in_between_blocks.begin(), in_between_blocks.end());
   // auto end = chrono:: high_resolution_clock::now();
  //  auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
  //  cout << "<t> find in-between blocks : "<< duration.count() << " ms" << endl;
    return in_between_blocks;
}

/**
 * @brief finds all the intermediate  blocks between block b_x and block b_y
 * @param b_x the index of the first block
 * @param b_y the index of the second block
 * @param intermediate_blocks the vector of the collected indices of intermediate bocks
 */
void BlockDeorderPlan ::find_intermediate_blocks(int b_x, int b_y, vector<int> *intermediate_blocks) {
    map<int, bool> visited = get_boolean_map();
    find_intermediate_blocks(b_x, b_y,  intermediate_blocks, &visited);
    if(count(intermediate_blocks->begin(), intermediate_blocks->end(), b_x) == 0)
        intermediate_blocks->push_back(b_x);
    if(count(intermediate_blocks->begin(), intermediate_blocks->end(), b_y) == 0)
        intermediate_blocks->push_back(b_y);
    sort(intermediate_blocks->begin(), intermediate_blocks->end());
    if(print_details){
        string s = "\t\t\tintermediate blocks are :";
        for(int i : *intermediate_blocks)
            s+= to_string( i) + " ";
        s+= "\n";
        write_log_file(s);
    }

   // auto end = chrono:: high_resolution_clock::now();
   // auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
   // cout << "<t> find intermediate blocks : "<< duration.count() << " ms" << endl;

}

/**
 * @brief an assistant function to find intermediate blocks between two  blocks
 * @param visited_blocks a vector of bool to track the visited  blocks
 * @return
 */
bool BlockDeorderPlan :: find_intermediate_blocks(int b_x, int b_y, vector<int> *intermediate_blocks, map<int, bool> *visited){
    visited->at(b_x) = true;
    bool found = false;
    Block* block_x = get_block(b_x);
//    if(print_details)
//        block_x->print_predecessors_and_successors();
    vector<int> successors = block_x->successors;
    for(int b_i : successors){
        Block* block_i = get_block(b_i);
        if ( block_i->type == TEMP_INTERNAL || b_i == goal_block || blocks_orderings.is_transitive(b_x, b_i))
            continue;
        if (is_in_vector(b_i, intermediate_blocks) || b_i == b_y){
            found = true;
            if(!is_in_vector(b_x, intermediate_blocks))
                intermediate_blocks->insert(intermediate_blocks->begin(), b_x);
            continue;
        }
        if (visited->at(b_i)) continue;
        if (find_intermediate_blocks(b_i, b_y, intermediate_blocks, visited)){
            if(!is_in_vector(b_x, intermediate_blocks))
                intermediate_blocks->insert(intermediate_blocks->begin(), b_x);
            found = true;
        }
    }
    return found;
}


/** @brief this function checks the validity of causal link where block b_x is the consumer and f is the fact.
* @param b_x the block index of the consumer
* @param f the fact of the causal link
* @return -1 if the consumer has no causal link on f with any producer or there exists any deleter between producer
     and consumer. Otherwise, return the index of the producer. */
int BlockDeorderPlan :: check_validity_of_causal_link(int b_x, FactProxy f) {
    Block *block = get_block(b_x);
    int producer = -1;
    /// find the producer
    producer = get_producer(b_x, f);
    /** if producer is not found. check if the block is an internal block and whether its parent is the consumer of that
     *  fact, then find the producer of the fact for its parent block
     */
    if (producer == -1){
//        get_block(block->parent)->print_preconditions_and_effects();
        if (block->is_internal() &&  is_consumer(block->parent, f))
            return check_validity_of_causal_link(block->parent, f);
        else
            return -1;
    }
    ///check whether there is any deleter between the producer and consumer
    map<int, bool> visited = get_boolean_map();
    vector<int> intermediate_blocks;
    find_intermediate_blocks(producer, b_x,  &intermediate_blocks, &visited);
    delete_element_from_vector(&intermediate_blocks, b_x);
    for (int i_x: intermediate_blocks){
        if (is_deleter(i_x,f)){
            cout << "block " << i_x << " is deleter" << endl;
            return -1;
        }

    }
    return producer;
}

/** @brief this function checks whether a block  has a certain type (t: PC, CD, DP) of ordering constraint on a fact (f)
* @param b_x the index of the block to be checked
* @param f the fact to be checked
* @param t the type of ordering constraint to be checked
* @return true if the block has a ordering constraint of type t on a fact f, false otherwise */
bool BlockDeorderPlan :: have_constraint(int b_x, FactProxy f, constraint_type t, bool is_incidental = false){
    Block *block = get_block(b_x);
    for(int b_s: block->successors){
        for (OrderingConstraint constraint :blocks_orderings.get_constraints(b_x, b_s)){
            if (constraint.type == t && is_equal(constraint.factProxy , f) && constraint.is_incidental == is_incidental )
                return true;
        }
    }
    return false;
}



/**
 * @brief checks validity of all  blocks
 */
bool BlockDeorderPlan :: check_validity (){
    bool valid = true;
    auto start = chrono:: high_resolution_clock::now();
    if(print_details)
        write_log_file("checking validity of all  blocks\n");
    for (auto itr : blocks){
        if (itr.first == init_block) continue;
        if (!check_validity(itr.first)) {
            if(print_details)
                write_log_file("!!! bdpo invalid !!!\n");
            return false;
        }
    }
    auto end = chrono:: high_resolution_clock::now();
    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << "<t> checking validity : "<< duration.count() << " ms" << endl;
    return true;
}

vector<int> BlockDeorderPlan :: get_parallel_blocks(int b_x){
    map<int, int> connection_map = get_connection_map(b_x);

    vector<int> parallel_blocks;

    for(auto itr: connection_map){
        if(itr.second == 0)
            parallel_blocks.push_back(itr.first);
    }
    return parallel_blocks;
}

/**
 * @brief checks validity of a block that is checking whether each precondition of the  blocks is supported by a causal link and
 *      this block is not a threat to any causal link
 * @param b_x the index of the block
 */
bool BlockDeorderPlan :: check_validity (int b_x){

    if(print_details)
        write_log_file( "\tchecking validity of block " + to_string(b_x) + "\n" );
    Block *block = get_block(b_x);

    vector<int> parallel_blocks = get_parallel_blocks(b_x);

    /// check if each precondition is supported by a causal link
    if(print_details) {
        cout << "checking validity of causal links\n";
        block->print_predecessors_and_successors();
    }
    for (FactProxy factProxy :  block->get_preconditions()){
        if (check_validity_of_causal_link(b_x, factProxy) == -1){
            if(print_details)
                write_log_file("!!!!!!!!!!!!!!!!!!!!\n bdpop invalid\nno producer of " + to_string(b_x) + "]" +
                get_block_name(b_x) + "for fact " + print_fact(factProxy) + "\n");
//            cout << endl;
            return false;
        }
        for (int parallel: parallel_blocks) {
            Block* parallel_block = get_block(parallel);
            if(parallel_block->type == TEMP_REMOVED) continue;
            if (is_deleter(parallel, factProxy)){
                if(print_details)
                    write_log_file("!!!!!!!!!!!!!!!!\n bdpop invalid\n[" + to_string(parallel) + "]" +
                    get_block_name(parallel) + " is a threat to [" + to_string( b_x) + "]" +  get_block_name(b_x)
                    + "for fact "+ print_fact(factProxy) + "\n");
                return false;
            }
        }
    }
    /// check if this block is a threat for any causal link of its parallel and successor  blocks
    if(print_details)
        cout<< "checking validity of parallel blocks\n";
    for (int parallel: parallel_blocks){
        Block* parallel_block = get_block(parallel);
        if(parallel_block->type == TEMP_REMOVED) continue;
        for (FactProxy factProxy :  parallel_block->get_preconditions()){
            if (is_deleter(b_x, factProxy)){
                if(print_details)
                    write_log_file("!!!!!!!!!!!!!!!!!!\n bdpop invalid\n[" + to_string(b_x) + "]" + get_block_name(b_x) +
                    " is a threat to [" + to_string(parallel) + "]" + get_block_name(parallel) +  "for fact " +
                    print_fact(factProxy) + "\n");
                return false;
            }
        }
    }
    if(print_details)
        cout<< "checking validity of successor blocks\n";
    for (int successor: block->successors){
        Block* successor_block = get_block(successor);
        if(successor_block->type == TEMP_REMOVED || successor_block->is_temp_internal()) continue;
        for (FactProxy factProxy :  successor_block->get_preconditions()){
            if (is_deleter(b_x, factProxy)){
                int producer = get_producer(successor, factProxy);
                if (producer == -1){
                    if(print_details)
                        write_log_file("!!!!!!!!!!!!!!!!!!\n bdpop invalid\n[" + to_string(b_x) + "]" + get_block_name(b_x) +
                        " is a threat to [" + to_string(successor) + "]" + get_block_name(successor) + "for fact "+
                                   print_fact(factProxy) + "\n");
                    return false;
                }
              //  successor_block->print_predecessors_and_successors();
                if (is_successor(producer, b_x, false)) {
                    if(print_details)
                        write_log_file("!!!!!!!!!!!!!!!!!!\n bdpop invalid\n[" + to_string(b_x) + "]" + get_block_name(b_x)
                        + " is a threat to [" + to_string(successor) + "]" + get_block_name(successor) + "for fact "+
                                   print_fact(factProxy) + "\n");
                    return false;
                }
            }
        }
    }

    //cout << "valid\n.";
    return true;
}

/**
 * find the index of the elementary block that contains the operator op among a set of blocks
 * @param op the operator id
 * @param related_blocks A vector of block ids
 * @return the index of the block that contains the operator op, if found. return -1 otherwise
 */
int BlockDeorderPlan :: find_block_id(OperatorID op, vector<int> related_blocks){
    for (int i: related_blocks){
        Block *block = get_block(i);
        if(!block->is_primary())
            continue;
        if(block->op_index == op)
            return block->id;
    }
    return -1;
}

/**
 * @brief finds the transitive orderings and makes them is_transitive true
 */
void BlockDeorderPlan :: find_transitive_ordering (){
//    auto start = chrono:: high_resolution_clock::now();
    if(print_details)
        cout << "finding transitive orderings.\n";

    for (auto itr : blocks){
        for(auto itr2 : blocks){
            blocks_orderings.set_transitive(itr.first, itr2.first, false);
        }
    }
    for (auto itr : blocks){
         int i = itr.first;
//        write_log_file("block "+ to_string(i) + "\n ");
        Block* block_i = get_block(i);
//        block_i->print_predecessors_and_successors();
        for (int j: block_i->successors){
//            write_log_file("\t\t check block "+ to_string(j) + "\n ");
            if(blocks_orderings.is_transitive(i, j)) continue;
            Block* block_j = get_block(j);
            bool transitive_found = false;
            for (int k : block_j->predecessors) {
//                write_log_file("\t\t\t check block "+ to_string(k) + "\n ");
                if( i == k || blocks_orderings.has_any_ordering(k, i)) continue;
                if (is_successor(i, k, false)) {          // finding i & j is transitive or not
                    blocks_orderings.set_transitive(i, j, true);
                    transitive_found = true;
                    break;
                }
            }
//            write_log_file("\t\t\tdone\n");
            if (transitive_found) {                // if transitive, the successors of j also transitive of i
                for (int successor_j: block_j->successors) {
                    if (blocks_orderings.has_any_ordering(i, successor_j)) {
                        blocks_orderings.set_transitive(i, successor_j, true);
                    }
                }
            }
//            write_log_file("\t\t\t*done\n");
        }
//        write_log_file("\t\tdone\n");
    }
//    auto end = chrono:: high_resolution_clock::now();
//    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
//    cout << "<t> find transitive order : "<< duration.count() << " ms" << endl;
}


///**
// * @brief finds transitive ordering among the internal  blocks of a block
// * @param b_x the index of the block
// */
//void BlockDeorderPlan :: find_transitive_ordering (int b_x ){
//    cout << "finding transitive orderings of internal  blocks of compound block " << b_x <<".\n";
//
//    Block *block = &blocks.at(b_x);
//    for (int internal_x : block->internal_blocks){
//
//        for (int internal_y : block->internal_blocks){
//            if ( internal_x == internal_y) continue;
//            bool transitive_found = false;
//            if(blocks_orderings.has_any_ordering(internal_x, internal_y)) {
//                blocks_orderings.set_transitive(internal_x, internal_y, false);
//                for (int internal_k : block->internal_blocks) {
//                    if (internal_x == internal_k ) continue;
//                    if (blocks_orderings.has_any_ordering(internal_k, internal_y) && is_successor(internal_x, internal_k, false)) {
//                        blocks_orderings.set_transitive(internal_x, internal_y , true);
//                        transitive_found = true;
//                        break;
//                    }
//                }
//                if (transitive_found){
//                    for (int internal_k : block->internal_blocks) {
//                        if (internal_x == internal_k ) continue;
//                        if (blocks_orderings.has_any_ordering(internal_y, internal_k) && blocks_orderings.has_any_ordering(internal_x, internal_k)) {
//                            blocks_orderings.set_transitive(internal_x, internal_k, true);
//                        }
//                    }
//                }
//            }
//        }
//    }
//}

///**
// * @brief checks whether two blocks are connected
// * @param i the index of the first block
// * @param j the index of the second block
// * @return true if connected, false otherwise
// */
//bool BlockDeorderPlan:: check_operator_connected(int i, int j){
//    Block *block_i = get_block(i);
//    Block *block_j = get_block(j);
//
//    if (block_i->parent == block_j->parent )
//        return blocks_orderings.has_any_ordering(i, j);
//    else if (block_i->parent > -1 && block_j->parent == -1)
//        return check_operator_connected(block_i->parent, j);
//    else if(block_i->parent == -1 && block_j->parent > -1)
//        return check_operator_connected(i, block_j->parent);
//
//
//    bool same_parent = false;
//    Block *block = get_block(block_i->parent);
//    while (block->parent > -1){
//        if( block->parent == block_j->parent){
//            same_parent = true;
//            break;
//        }
//        block = get_block(block->parent);
//    }
//
//    if (same_parent) block_i = block;
//    else{
//        block = get_block(block_j->parent);
//        while (block->parent > -1){
//            if( block->parent == block_i->parent){
//                same_parent = true;
//                break;
//            }
//            block = get_block(block->parent);
//        }
//        if (same_parent) block_j = block;
//    }
//
//    return blocks_orderings.has_any_ordering(block_i->id, block_j->id);
//}


/**
 * @brief checks whether two blocks are connected
 * @param i the index of the first block
 * @param j the index of the second block
 * @return true if connected, false otherwise
 */
int BlockDeorderPlan:: check_operator_connected(int i, int j){
    Block *block_i = get_block(i);
    Block *block_j = get_block(j);
    vector<int> parents;

    bool same_parent = false;
    Block *block = block_i;
    while (block->parent > -1){
        block = get_block(block->parent);
        parents.push_back(block->id);
    }

    block = block_j;
    int common_parent = -1;
    while (block->parent > -1){
        block = get_block(block->parent);
        if(is_in_vector(block->id, &parents)){
            common_parent = block->id;
            break;
        }
    }
    Block *parent_i = block_i;
    Block *parent_j = block_j;
    while (parent_i->parent != common_parent){
//        cout << "i " << parent_i->parent << endl;
        parent_i = get_block(parent_i->parent);
    }
    while (parent_j->parent != common_parent){
        parent_j = get_block(parent_j->parent);
    }
//    cout << "here"<< endl;
    int x = parent_i->id;
    int y = parent_j->id;
    if (blocks_orderings.has_any_ordering(x, y) || is_successor(x,y, false)) return 1;
    else if (concurrent_orderings.has_any_ordering(x, y))
        return -1;
    else
        return 0;

}

void BlockDeorderPlan :: find_concurrent_actions(){
    concurrent_orderings.clear();
    for (auto block : blocks) {;
        if(block.first == init_block || block.first == goal_block)
            continue;
        vector<int> unordered_blocks = get_parallel_blocks(block.first);
        for(int u:unordered_blocks){
            if(concurrent_orderings.have_constraint(block.first, u, NCON) || concurrent_orderings.have_constraint(u, block.first, NCON)) continue;
            vector<int> conflicted_variables = is_concurrent(block.first, u);
            if(conflicted_variables.size() > 0){
                for(int c: conflicted_variables) {
                    OrderingConstraint new_nn = OrderingConstraint(task_proxy.get_variables()[c].get_fact(0), NCON);
                    concurrent_orderings.add_constraint(block.first, u, new_nn);
                }
            }
        }
    }
}


vector<int> BlockDeorderPlan :: is_concurrent(int b_x, int b_y){
    write_log_file("hello\n");
    vector<int> xblocks, yblocks;
    xblocks = get_primary_blocks(b_x);
    yblocks = get_primary_blocks(b_y);
    vector<int> conflicted_variables;

    for(int i: xblocks){
        for(int j: yblocks){
            Block* block_x = get_block(i);
            Block* block_y = get_block(j);
            vector<int> c_v = is_concurrent(block_x, block_y);
            if(!c_v.empty()){
                for(int c : c_v){
                    if(!is_in_vector(c, &conflicted_variables))
                        conflicted_variables.push_back(c);
                }
            }
        }
    }
    return conflicted_variables;
}

vector<int> BlockDeorderPlan :: is_concurrent(Block* block_x, Block* block_y){
    vector<int> conflicted_variables;

    map<int, int> pre1,pre2;
    map<int, vector<int>> eff2, eff1;
    for(FactProxy factProxy: block_x->get_preconditions()){
        pre1.insert(pair<int, int>(factProxy.get_variable().get_id(), factProxy.get_value()));
    }

    for(FactProxy factProxy: block_y->get_preconditions()){
        pre2.insert(pair<int, int>(factProxy.get_variable().get_id(), factProxy.get_value()));
    }

    for(EffectProxy effectProxy: block_x->get_effects()){
        FactProxy factProxy = effectProxy.get_fact();
        int v = factProxy.get_variable().get_id();
        if(eff1.count(v) == 0)
            eff1.insert(pair<int, vector<int>>(v, vector<int>(0)));
        eff1.at(v).push_back(factProxy.get_value());
    }

    for(EffectProxy effectProxy: block_y->get_effects()){
        FactProxy factProxy = effectProxy.get_fact();
        int v = factProxy.get_variable().get_id();
        if(eff2.count(v) == 0)
            eff2.insert(pair<int, vector<int>>(v, vector<int>(0)));
        eff2.at(v).push_back(factProxy.get_value());
    }

    for(auto p: pre1){
        int v = p.first;
        if(pre2.count(v) > 0 && pre2.at(v) != pre1.at(v)){
//            concurrent = false;
            conflicted_variables.push_back(v);
        }

        if(eff1.count(v) > 0 ){
            if(pre2.count(v) >0 || eff2.count(v) > 0) {
//                concurrent = false;
                conflicted_variables.push_back(v);
            }
        }
    }
    for(auto p: pre2){
        int v = p.first;
        if(eff2.count(v) > 0 ){
            if(pre1.count(v) >0 || eff1.count(v) > 0) {
//                concurrent = false;
                conflicted_variables.push_back(v);
            }
        }
    }
    return conflicted_variables;
}

/**
 * @return the vector of ids of all primary blocks
 */
vector<int> BlockDeorderPlan :: get_primary_blocks(){
    vector<int> primary_blocks;
    for(auto itr: blocks){
        if (itr.first == goal_block || itr.first == init_block) continue;
        if(itr.second.is_primary())
            primary_blocks.push_back(itr.first);
    }
    return primary_blocks;
}

vector<int> BlockDeorderPlan :: get_compound_blocks(){
    vector<int> compound_blocks;
    for(auto itr: blocks){
        if (itr.first == goal_block || itr.first == init_block) continue;
        if(!itr.second.is_primary())
            compound_blocks.push_back(itr.first);
    }
    return compound_blocks;
}



/**
 * @brief  returns the set of the primary blocks of a compound block
 * @param b_x the index of the block
 */
vector <int> BlockDeorderPlan :: get_primary_blocks(int b_x){
    vector<int> internals;
    Block *block = get_block(b_x);
    if(block->is_primary()) internals.push_back(b_x);
    for (int i : block->internal_blocks)
    {
        Block* internal = get_block(i);
        if( internal->is_primary()) internals.push_back(i);
        else {
            for (int internal_x :  get_primary_blocks(i))
                internals.push_back(internal_x);
        }
    }
    return internals;
}





int BlockDeorderPlan :: cost(int b_x){
    Block *block = get_block(b_x);
    if (b_x == init_block || b_x == goal_block) return 0;
    if (block->is_primary()){
        return task_proxy.get_operators()[block->op_index].get_cost();
    }
    int block_cost = 0;
    for(int i: block->internal_blocks){
        block_cost += cost(i);
    }
    return block_cost;
}

//
///**
// * @brief calculates the flex of the block de-ordered  total_order_plan
// * @return the flex value
// */
//float BlockDeorderPlan :: calculate_flex() {
//    vector<int> primary_blocks = get_primary_blocks();
//    int plan_size = primary_blocks.size();
//    vector<vector<bool>> ordering(plan_size , vector<bool>(plan_size, false));
//    for (int i = 0 ; i < plan_size ; i++) {
//        for (int j = 0 ; j < plan_size ; j++) {
//            if (i == j) continue;
//           // cout << "checking operator " << i << " & " << j << endl;
//            if (check_operator_connected( primary_blocks[i], primary_blocks[j]))
//                ordering[i][j] = true;
//           if(ordering[i][j]){
//                for (int k = 0; k < plan_size ; k++) {
//                    if ( k == j || k == i) continue;
//                    if (ordering[k][i])
//                        ordering[k][j] = true;
//                }
//            }
//        }
//    }
//    int count_total = 0;
//    int count_connected = 0;
//
//    for (int i = 0; i < plan_size  -1; i++) {
//        for (int j = i+1; j < plan_size; j++) {
//            count_total++;
//            if (ordering[i][j] || ordering[j][i]) {
//                count_connected++;
//            }
//        }
//    }
//    int unpaired = (count_total- count_connected);
// //   cout << "total: " << count_total << "connected: "<< count_connected << " unpaired: " << unpaired << endl;
//    float flex = (float)unpaired / (float)count_total;
//    return flex;
//}

int BlockDeorderPlan :: calculate_plan_cost() {
    OperatorsProxy operators =task_proxy.get_operators();
    int plan_cost = 0;
    for (auto block: blocks) {
        int id = block.first;
        if(id == init_block || id == goal_block || !block.second.is_primary()) continue;
        plan_cost += operators[block.second.op_index].get_cost();
    }
    return plan_cost;
}
/**
 * @brief calculates the flex of the block de-ordered  total_order_plan
 * @return the flex value
 */
void  BlockDeorderPlan :: calculate_flex() {
    if(concurrency)
        find_concurrent_actions();
    vector<int> primary_blocks = get_primary_blocks();
    int plan_size = primary_blocks.size();
    vector<vector<int>> ordering(plan_size , vector<int>(plan_size, 0));
    for (int i = 0 ; i < plan_size ; i++) {
        for (int j = 0 ; j < plan_size ; j++) {
            if (i == j) continue;
            ordering[i][j] = check_operator_connected( primary_blocks[i], primary_blocks[j]);
        }
    }
    int count_total = 0;
    int count_connected = 0;
    int nonconcurrent = 0;

    for (int i = 0; i < plan_size  -1; i++) {
        for (int j = i+1; j < plan_size; j++) {
            count_total++;
            if (ordering[i][j] > 0 || ordering[j][i] > 0) {
                count_connected++;
            } else if(ordering[i][j] < 0 || ordering[j][i] < 0)
                nonconcurrent++;
        }
    }
    int unpaired = (count_total- count_connected);
    int concurrent = (unpaired - nonconcurrent);
    flex[0] = (float)unpaired / (float)count_total;
    flex[1] = (float)concurrent / (float)count_total;
    if(print_details) {
        cout << "total: " << count_total << "\tconnected: " << count_connected << "\tunpaired: " << unpaired << endl;
        cout << "nonconcurrent: " << nonconcurrent << "\tconcurrent: " << concurrent << endl;
        cout<< flex[0] << "\t" << flex[1];
    }


}

int BlockDeorderPlan ::calculate_height() {
    map<int, bool> visited = get_boolean_map();
    map<int, int> height = get_int_map();
    calculate_height(init_block, &visited, &height);
    return height.at(init_block);
}

int BlockDeorderPlan :: calculate_height(int b_x, map<int, bool> * visited, map<int, int> *height) {
    Block* block = get_block(b_x);
    visited->at(b_x) = true;
    vector<int> successors = block->successors;
    if(successors.size() == 0) {
        height->at(b_x) = 1;
        return 1;
    }
    int max = 0;
    int self_height = 1;
    if(!block->is_primary()){
        vector<int> internal = get_initial_blocks_of_a_block(b_x);
        for(int i : internal){
            if(!visited->at(i)) calculate_height(i, visited, height);
            int ht = height->at(i);
            if(ht > self_height) self_height = ht;
        }
    }
    for(int s: successors){

        if(blocks_orderings.is_transitive(b_x, s) || s == goal_block) continue;
        if(!visited->at(s)) calculate_height(s, visited, height);
        int ht = height->at(s);
        if(ht > max) max= ht;
    }
    height->at(b_x) = max +self_height;
    return height->at(b_x);
}

void BlockDeorderPlan :: write_total_order_plan(string filename){
    Plan plan = get_total_order_plan();
    write_total_order_plan(plan, filename);
}

void BlockDeorderPlan :: write_total_order_plan(Plan plan, string filename){
    ofstream fout;
    fout.open(filename);
    for(OperatorID id: plan){
        fout << "(" <<task_proxy.get_operators()[id].get_name() << ")" << endl;
    }
}

void BlockDeorderPlan :: write_block_deorder_plan(string filename){
    ofstream fout;
    fout.open(filename);
    for(auto block: blocks){
        if(block.second.is_internal())
            continue;
        write_block(fout, "", &block.second);
    }
    fout << ":orderings(" << endl;
    for(auto block : blocks) {
        if(block.second.is_internal())
//            if(block.first == init_block || block.first == goal_block || block.second.is_internal())
            continue;
        int id = block.first;
        for (int successor: block.second.successors) {
            Ordering* ordering = blocks_orderings.get_ordering(id, successor);
            if(ordering->is_transitive) continue;
            fout << "\t(< b" << id << " b" << successor;
//            for (OrderingConstraint orderingConstraint: blocks_orderings.get_ordering(id,
//                                                                                      successor)->orderingConstraints) {
//                fout << " " << orderingConstraint.getTypeName() << "<"
//                    << orderingConstraint.factProxy.get_variable().get_id() << ":" <<
//                    orderingConstraint.factProxy.get_value() << ">";
//            }
            fout << ")" << endl;
        }
        /// Nonconcurrent Ordering
        for (auto block2 : blocks) {
            if(block.first == init_block || block.first == goal_block || block.second.is_internal())
                continue;
            vector<OrderingConstraint> constraints = concurrent_orderings.get_constraints(block.first, block2.first);
            for(OrderingConstraint constraint: constraints){
                fout << " " << "\t" << "(# b" << block.first << " b" << block2.first;
//                fout << " " <<constraint.getTypeName();
//                print_fact(constraint.factProxy);
                fout <<")"<< endl;
            }
        }
    }

    fout << ")";
}

void BlockDeorderPlan :: write_block(ofstream &out, string prefix, Block* block){
    out << prefix << "(";
    if(block->is_primary()){
        out << "b" << block->id << "(" <<task_proxy.get_operators()[block->op_index].get_name() << ")" ;
    } else{
        out << "b" << block->id << endl;
        out << prefix << "\t:preconditions(" << endl;
        out << prefix +"\t\t";
        for(FactProxy factProxy:block->preconditions){
            out << "(" << factProxy.get_name() << ") ";
        }
        out <<endl;
        out << prefix +"\t)"<< endl;

        out << prefix << "\t:effects(" << endl;
        out << prefix +"\t\t";
        for(EffectProxy effectProxy:block->effects){
            out << "(" << effectProxy.get_fact().get_name() << ") ";
        }
        out <<endl;
        out << prefix +"\t)"<< endl;

        out << prefix << "\t:subtasks(" << endl;
        for(int internal : block->internal_blocks){
            Block* block_internal = get_block(internal);
            write_block(out, prefix + "\t\t", block_internal);
        }
        out << prefix +"\t)"<< endl;
        out << prefix << "\t" << ":orderings(" << endl;
        for(int internal : block->internal_blocks){
            Block* block_internal = get_block(internal);
            for(int successor: block_internal->successors){
                Ordering* ordering = blocks_orderings.get_ordering(internal, successor);
                if(ordering->is_transitive) continue;
                out << prefix + "\t\t" << "(< b" << internal << " b" << successor;
//               for(OrderingConstraint orderingConstraint: ordering->orderingConstraints){
//                   out << " " <<orderingConstraint.getTypeName() << "<" << orderingConstraint.factProxy.get_variable().get_id() << ":" <<
//                   orderingConstraint.factProxy.get_value()<< ">";
//               }
               out <<")"<< endl;
            }
            /// Nonconcurrent Ordering
            for (int internal2 : block->internal_blocks) {
                vector<OrderingConstraint> constraints = concurrent_orderings.get_constraints(internal, internal2);
                for(OrderingConstraint constraint: constraints){
                    out << prefix + "\t\t" << "(# b" << internal << " b" << internal2;
//                        out << " " <<constraint.getTypeName();
//                        print_fact(constraint.factProxy);
                    out <<")"<< endl;
                }
            }
        }
        out<< prefix + "\t\t" <<  ")";
        out<< endl << prefix ;

    }
    out << ")" << endl;
}


/// @brief This function is to read the total_order_plan file. It reads and finds operators of the total_order_plan file.
Plan BlockDeorderPlan :: read_sequential_plan (string planfile){
    ifstream in;
    in.open (planfile);
    if (!in.is_open()) exit(1);
    auto read_operator =[&]() {
        string w;
        string opname;
        in >> w;
        opname = w;
        while(1){
            in >> w;
            if(w.back() == ')'){
                w.resize(w.size() - 1);
                opname.append(" ");
                opname.append(w);
                break;
            }
            opname.append(" ");
            opname.append(w);
        }
        transform(opname.begin(), opname.end(), opname.begin(), ::tolower);
        return opname;
    };
    Plan p;
    char c;
    string fullopname;
    string word;
    OperatorsProxy operatorsProxy =task_proxy.get_operators();
    in >> c;
    while(c == '('){
        c=NULL;
        fullopname = read_operator();
        for(OperatorProxy  op:operatorsProxy){
            string name = op.get_name();

            if (fullopname.compare(name) == 0){
                OperatorID operatorId(op.get_id());
                p.push_back(operatorId);
                break;
            }
        }
        in >> c;
        if(c == NULL)
            break;
    }
    in.close();
    return p;
}

///// @brief This function is called from read_plan() function. It reads a single operator name from total_order_plan file and return it to read_sequential_plan() function.
//string BlockDeorderPlan :: read_operator(ifstream &in){
//    string word;
//    string opname;
//    in>> word;
//    opname = word;
//    while(1){
//        in>>word;
//        if(word.back() == ')'){
//            word.resize(word.size()-1);
//            opname.append(" ");
//            opname.append(word);
//            break;
//        }
//        opname.append(" ");
//        opname.append(word);
//    }
//    transform(opname.begin(), opname.end(), opname.begin(), ::tolower);
//    return opname;
//}











//////////////////////////////// All Print Functions //////////////////////

/**
 * @brief if a block is primary, name of the block is its operator index. Otherwise, the name consisits of the ids of
    its internal block
 */
string BlockDeorderPlan :: get_block_name(int b_x){
    ostringstream s;
    s << "b";
    Block *block = get_block(b_x);
    if (block->is_primary()) s << "<" << block->op_index << ">";
    else {
        vector<int> elementary_blocks = get_primary_blocks(b_x);
        sort(elementary_blocks.begin(), elementary_blocks.end());
        s <<"<";
        for (int i = 0; i < elementary_blocks.size() ; i++){
            s << elementary_blocks[i];
            if(i != elementary_blocks.size() - 1) s << ":";
        }
        s << ">";
    }
    return s.str();
}

string BlockDeorderPlan :: print_fact(FactProxy f){
    return  "<" + to_string(f.get_variable().get_id()) + ":" + to_string(f.get_value()) + "> " + f.get_name();
}

void BlockDeorderPlan :: print_effects_and_preconditions(){
    string s="";
    for (auto itr : blocks){
        if(itr.second.is_primary())
            s+= "[" + to_string(itr.first) + "] <" + to_string(itr.second.op_index.get_index())
                + "> " +task_proxy.get_operators()[itr.second.op_index].get_name();
        s+= itr.second.print_preconditions_and_effects();
    }
    write_log_file(s);
}

void BlockDeorderPlan :: print_predecessord_successors(){
    for (auto itr : blocks){
        itr.second.print_predecessors_and_successors();
    }
}


/**
 * @brief prints the block orderings
 */
void BlockDeorderPlan :: print_blocks_orderings (){
    string s;
    s= "-------------------- start blocks Orderings -------------------\n";
    for (auto itr: blocks) {
        int b_x = itr.first;
        Block* block = get_block(b_x);
        s+=  " [" + to_string(b_x) + "] " + get_block_name(b_x) + "type : " + block->getTypeName();
        if ( block->parent!= -1) cout << " " << block->parent;
        s+= "\n";
//        block->print_predecessors_and_successors();
        for(int j: block->successors){
            s+= " \t[" + to_string(j) + "] "  +  get_block_name(j) + " ";
            for (OrderingConstraint constraint : blocks_orderings.get_constraints(b_x,j)){
               s+= constraint.print();
            }
            s+= to_string( blocks_orderings.is_transitive(b_x, j)) ;
            s+= "\n";
        }
        s+= "\n";
    }
    s+=  "-------------------- end Orderings -------------------\n";
    write_log_file(s);
}

/**
 * @brief print the dot file of the block de-ordered  total_order_plan
 * @param with_transitive transitive orderings will be printed if true, not printed otherwise
 * @param filename the name of the dot file
 */
void BlockDeorderPlan ::   print_bdop_dot_file(bool with_transitive, string filename){
    Plan total_order_plan = get_total_order_plan();
//    auto start = chrono:: high_resolution_clock::now();
//    OperatorsProxy operatorsProxy = mtask_proxy.get_operators();
    ofstream fout;
    fout.open (filename);
    fout << "digraph bdop {\n" << "\tnode [shape=box];\n"  <<"Graph [compound=true];\n";
    fout << "\tsubgraph popgrah {\n"<< "\t\trankdir=\"TB\";" << endl;

    for (auto itr : blocks){
        if ( itr.second.is_primary() )
            fout << "\t\t" << itr.first << "[shape = box]"  << endl;
    }

    for (auto itr: blocks){   /// compound node declaration
        if( itr.second.is_internal())
            continue;
        print_node_dot_file(fout, itr.first);
    }

    for (auto itr_i: blocks) {
        int i = itr_i.first;
        Block* block_x = get_block(i);
        if ( block_x->is_internal()) continue;
//        if(i == 51)
//            cout << " ";
        fout<<"\t\t//printing ordering of outer block " << i << endl;
        if ( block_x->is_primary()){
            for (int j: block_x->successors){
                Block* block_y = get_block(j);
                if (!block_y->is_primary() ||  block_y->is_internal()) continue;
                vector<OrderingConstraint> orderingConstraints = blocks_orderings.get_constraints(i, j);
                if (!with_transitive && blocks_orderings.is_transitive(i, j)) continue;
                fout <<"\t\t//with primary outer block " << j << endl;
                fout <<"\t\t" << i << " -> "  <<j;
                print_ordering_label_dot_file(fout, orderingConstraints);
                fout << ";\n";
            }
        } else print_cluster_orderings_dot_file(fout, i, with_transitive);      /// cluster orderings
    }
    fout <<"\t\t"<< init_block << "[shape=diamond];" <<endl;
    fout <<"\t\t"<< goal_block << "[shape=diamond];" << endl;
    fout << "\t};";

//    if(with_cncurrent)
    print_cluster_concurrent_orderings_dot_file(fout);
    print_total_order_plan_dot_file(fout, total_order_plan);

    fout.close();
//    auto end = chrono:: high_resolution_clock::now();
//    auto duration = chrono::duration_cast<std::chrono::milliseconds>(end - start);
//    cout << "<t> printing dot file : "<< duration.count() << " ms" << endl;
}


void BlockDeorderPlan :: print_total_order_plan_dot_file(ostream &fout, Plan total_order_plan ){
    fout << "\n\tsubgraph lineargrah {"<< endl;                                /// linear  total_order_plan
    fout <<"\tnode [margin=0 fontcolor=blue shape=none];"<<endl;
    vector<int> primary_blocks = get_primary_blocks();
    for (int i=0; i < total_order_plan.size(); i++) {
        OperatorProxy operatorProxy =task_proxy.get_operators()[total_order_plan[i]];
        int b;
        if (operatorProxy.get_name() == "init_action") b = init_block;
        else if (operatorProxy.get_name() == "goal_action") b = goal_block;
        else b = find_block_id(total_order_plan[i], primary_blocks);
        fout << "\t\tnode_"<< i;
        fout << "[label=\"[" << b << "] " << operatorProxy.get_name() << "\"];" << endl;
    }
    fout << "\t\t";
    for (int i=0; i < total_order_plan.size(); i++) {
        fout << "node_"<< i;
        if (i < total_order_plan.size() - 1) fout << "->";
    }

    fout << "\n\t};";
    fout << "\n}";
}


/**
 * @brief prints the orderings of a compound block in a dot file
 * @param b_x the index of the block
 * @param with_transitive transitive orderings will be printed if true, not printed otherwise
 */
void BlockDeorderPlan :: print_cluster_orderings_dot_file (ostream &fout, int b_x, bool with_transitive){

    Block *block = get_block(b_x);
    if ( block->is_primary())
        return;
//    cout <<" cluster ordering "<< b_x << endl;
    vector<int> bx_operators = get_primary_blocks(b_x);
    sort (bx_operators.begin(), bx_operators.end());
    int last_internal = bx_operators[bx_operators.size() - 1];
    int first_internal = bx_operators[0];
    /// outgoing orderings
    for (int b_y : block->successors){
        Block* block_y = get_block(b_y);
        vector<int> by_operators = get_primary_blocks(b_y);
        sort (by_operators.begin(), by_operators.end());

        if (!with_transitive && blocks_orderings.is_transitive(b_x, b_y)) continue;
        fout << "\t\t" << last_internal<< " -> " << by_operators[0];
        print_ordering_label_dot_file(fout, blocks_orderings.get_constraints(b_x, b_y));
        fout << "[ltail = cluster_" << b_x;
        if (! block_y->is_primary())
            fout << ", lhead = cluster_" << b_y;
        fout << "];\n";

    }
    for (int b_y : block->predecessors){
//        cout<< "\t\t" << b_y<< endl;
        Block* block_y = get_block(b_y);
        vector<int> by_operators = get_primary_blocks(b_y);
        sort (by_operators.begin(), by_operators.end());

        if ((!with_transitive && blocks_orderings.is_transitive(b_y, b_x)) || ! block_y->is_primary()) continue;
        fout << "\t\t" << by_operators[by_operators.size() - 1]<< " -> " << first_internal;
        print_ordering_label_dot_file(fout, blocks_orderings.get_constraints(b_y, b_x));
        fout << "[";
        if (!block_y->is_primary())
            fout << "ltail = cluster_" << b_y << ",";
        fout << "lhead = cluster_" << b_x << "];\n";
    }


    fout << "\t\t//intermediate orderings \n";
    for (int internal_x : block->internal_blocks){       /// intermediate orderings
        for (int internal_y:  block->internal_blocks){;
            vector<int> int_x_operators = get_primary_blocks(internal_x);
            vector<int> int_y_operators = get_primary_blocks(internal_y);
            sort (int_x_operators.begin(), int_x_operators.end());
            sort (int_y_operators.begin(), int_y_operators.end());
            int last_internal = int_x_operators[int_x_operators.size() - 1];
            int first_internal = int_y_operators[0];
            Block* block_x = get_block(internal_x);
            Block* block_y = get_block(internal_y);
            int x = internal_x;
            int y = internal_y;
            if (blocks_orderings.has_any_ordering(internal_x, internal_y)) {
                if (!with_transitive && blocks_orderings.is_transitive(internal_x, internal_y)) continue;
                fout << "\t\t" << last_internal << " -> " << first_internal;
                print_ordering_label_dot_file(fout, blocks_orderings.get_constraints(internal_x, internal_y));
                fout << endl;
            }
        }
    }


    for (int internal: block->internal_blocks){
        print_cluster_orderings_dot_file(fout, internal, with_transitive);
    }
    fout<< endl;
}

/**
 * @brief prints the orderings of a compound block in a dot file
 * @param b_x the index of the block
 * @param with_transitive transitive orderings will be printed if true, not printed otherwise
 */
void BlockDeorderPlan :: print_cluster_concurrent_orderings_dot_file (ostream &fout){

    if(concurrent_orderings.orderings.empty()) return;
    for(auto c: concurrent_orderings.orderings){
        Block* block_x = get_block(c.first.first);
        Block* block_y = get_block(c.first.second);
        int first = block_x->id;
        int second = block_y->id;
        if(!block_x->is_primary())
            first = get_primary_blocks(first)[0];
        if(!block_y->is_primary()){
            vector<int> bx_operators = get_primary_blocks(second);
            sort (bx_operators.begin(), bx_operators.end());
            second= bx_operators[bx_operators.size()-1];
        }
        fout << "\t\t" << first<< " -> " << second << " [";
        if(!block_x->is_primary())
            fout << "ltail=cluster_" << block_x->id << ",";
        if(!block_y->is_primary())
            fout << "lhead=cluster_"<< block_y->id << ",";
        fout << "arrowhead=none]";
        print_ordering_label_dot_file(fout, c.second.orderingConstraints);
    }

}


/**
 * @brief prints the label of an edge for orderings between two  blocks in the dot file
 * @param orderingConstraints the vector of ordering constraints
 */
void BlockDeorderPlan :: print_ordering_label_dot_file (ostream &fout, vector<OrderingConstraint> orderingConstraints){
//    if (orderingConstraints.empty()) return;
    fout << " [label=\"";
    for (int k =0; k < orderingConstraints.size() ; k++ ){
        fout << orderingConstraints[k].getTypeName();
        if(orderingConstraints[k].type != NCON) {
            fout << orderingConstraints[k].factProxy.get_variable().get_id() << ":";
            fout << orderingConstraints[k].factProxy.get_value();
        } else
            fout << orderingConstraints[k].factProxy.get_variable().get_id() ;
        if (k != orderingConstraints.size() - 1)
            fout << "\\n";
    }
    fout << "\"";
    if(!orderingConstraints.empty() && orderingConstraints.front().type == NCON){
        fout << ",color=red, penwidth=5" ;
    }
    fout << "]";
}


/**
 * @brief writes the subgraph cluster for compound block
 * @param b_x the index of the block
 */
void BlockDeorderPlan :: print_node_dot_file(ostream &fout, int b_x){
    Block* block = get_block(b_x);
    if( block->is_primary())
        return;
    fout << "\t\tsubgraph cluster_"<<b_x <<"{"<<endl;
    if ( block->is_internal())
        fout << "\t\t\tstyle = dotted;"<< endl;
    else
        fout << "\t\t\tcolor=blue;"<< endl;
    fout << "\t\t\tlabel=\"" << b_x << "\"" << endl;
    fout <<"\t\t\t";

    for (int internal :  get_primary_blocks(b_x)){
        fout << internal << " ";
    }
    fout << ";" << endl;

    for(int internal: block->internal_blocks){
        if(! get_block(internal)->is_primary())
            print_node_dot_file(fout, internal);
    }

    fout << "\n\t\t};"<<endl;
}

///prints the outer block
void BlockDeorderPlan:: print_outer_blocks(){
    cout << "The outer blocks: "<<endl;
    for (auto itr : blocks){
        if ( itr.second.is_internal()||  itr.second.is_primary())
            continue;
        cout << itr.first << ". " ;
        cout <<  get_block_name(itr.first);
        cout<< endl;
    }
}

/**
 * @brief print the values of all variables in a state
 * @param s the state
 */
void BlockDeorderPlan :: print_state(State s){
    vector<int> v = s.get_unpacked_values();
    cout << "state: ";
    for(int i = 0; i <v.size(); i++){
        cout << "[" << i << "," << v[i] << "]" <<task_proxy.get_variables()[i].get_fact(v[i]).get_name() << " ";
    }
    cout << endl;
}


bool file_check_is_valid(string filename){
    ifstream fin;
    fin.open(filename);
    for( std::string line; getline( fin, line ); ) {
        if (line.find("check_val") != string::npos) {
            fin.close();
            return false;
        }

        if (line.find("Plan valid") != string::npos) {
            fin.close();
            return true;
        }
    }
    return false;
}


//bool BlockDeorderPlan :: validate_total_order_plan(){
//    cout << "start validating partial order bdpop" << endl;
//
//    string name = "total_order_plan.txt";
//    write_total_order_plan(name);
//
//    string log_file_name =  "log.txt";
//    freopen(log_file_name.c_str(), "a", stdout);
//    ostringstream  com;
//    com << "./validate ";
//    com << domain_file << " " << problem_file << " " << name;
//    cout << com.str();
//    system(com.str().c_str());
//    ::fclose(stdout);
//
//    bool is_valid = file_check_is_valid(log_file_name);
//    if(is_valid)
//        return true;
//    return false;
//}