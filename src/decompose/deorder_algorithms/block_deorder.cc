//
// Created by sabah on ২৪/৪/২০.
//

#include <fstream>
#include "block_deorder.h"
#include "vector"
#include "../ordering_constraints.h"
#include "../block.h"
#include "map"
#include "../myutils.h"
#include <chrono>





///choose the constraint between two blocks to be removed
OrderingConstraint BlockDeorder :: choose_constraint (int b_x, int b_y){
    vector<OrderingConstraint> constraints = bdpop.blocks_orderings.get_constraints(b_x, b_y);
    for (int i = 0; i < constraints.size(); i++) {
        if (constraints[i].type == PC)
            return constraints[i];
    }
    return constraints[0];
}

//bool BlockDeorder :: make_temporary_compound_blocks_permanent(){
//
//}

/**
 * @brief checks whether there are any ordering constraints between two  blocks. if there are, it attempts
 *          to remove one constraint.
 * @param b_x the index of the first block
 * @param b_y the index of the second block
 * @return  true, all the orderings constraints between the two  blocks  have been removed successfuly,
 *          false otherwise
 */
bool BlockDeorder :: remove_ordering(int b_x, int b_y){

    if(!bdpop.blocks_orderings.has_any_ordering(b_x, b_y)){
        bdpop.make_temporary_block_permanent(b_x);
        bdpop.make_temporary_block_permanent(b_y);
        return true;
    }
    if(bdpop.print_details)
        write_log_file( "check B:" + to_string(b_x) + "-> B:" + to_string(b_y) + " --- \n");

    OrderingConstraint constraint  = choose_constraint(b_x, b_y);


    if (constraint.type == PC){
      int status = remove_pc(b_x, constraint);
      if (status < 0) return false;
      else b_x = status;
    }
    else if (constraint.type == CD) {
        bool direction_up_first;
        bool try_both = true;

        direction_up_first = initilize_up_first;
        int status;
        status = remove_cd(b_x, b_y, constraint, direction_up_first);
        if(status > 0){
            if (direction_up_first) b_x = status;
            else b_y = status;
        }
        //if (status == -1) return false;
        else if (status == -2 && try_both){
            status = remove_cd(b_x, b_y, constraint, !direction_up_first);
            if (status > 0) {
                if (direction_up_first) b_y = status;
                else b_x = status;
            }
        }
        if(status < 0)
            return false;
    }
    else{
        int status = remove_dp(b_y, constraint);
        if (status < 0) return false;
        else b_y = status;
    }


    vector<int> in_between_blocks = bdpop.find_in_between_blocks_for_temp_blocks(b_x, b_y);
    if (in_between_blocks.size() > 0){
        b_y = extend_block(b_y, in_between_blocks, true);
        if(b_y == -1) return false;
    }
    return remove_ordering(b_x, b_y);
}


///attempts to remove a PC constraint between two block
int BlockDeorder :: remove_pc (int b_x, OrderingConstraint constraint){

    FactProxy factProxy = constraint.factProxy;
    vector<int> intermediate_blocks;

    if (bdpop.print_details)
        write_log_file("\tremoving pc<" + bdpop.print_fact(factProxy) + "\n");

    int consumer = bdpop.search_upward(b_x, factProxy, 1);                     ///searching consumer upward
    if (consumer > -1){
        if(bdpop.print_details)
            write_log_file("\t\tfinding in-between  blocks between two temp  blocks " + to_string(consumer) + " and " +
                           to_string(b_x) + "\n");
        bdpop.find_intermediate_blocks(consumer, b_x, &intermediate_blocks); /// finding intermediate  blocks
        int status = extend_block(b_x, intermediate_blocks, true);
        return status;
    } else {
        return -2;
    }
}


///attempts to remove a CD constraint between two block
int BlockDeorder :: remove_cd (int b_x, int b_y, OrderingConstraint constraint, bool direction_up){
    FactProxy factProxy = constraint.factProxy;
    vector<int> intermediate_blocks;

    if(bdpop.print_details)
        write_log_file("\tremoving cd<" + bdpop.print_fact(factProxy) + to_string(direction_up) + " \n");

    int producer;
    if (direction_up)
        producer = bdpop.search_upward(b_x, factProxy, 0);               ///searching producer downward
    else
        producer = bdpop.search_downward(b_y, factProxy, 0);              /// searching producer upward
    if (producer >= 0){
        if (direction_up){
            if(bdpop.print_details)
                write_log_file("\t\tfinding in-between  blocks between two temp  blocks " + to_string(producer) + " and " +
                                     to_string(b_x) + "\n");
            bdpop.find_intermediate_blocks(producer, b_x, &intermediate_blocks);
            int status = extend_block(b_x, intermediate_blocks, true);
            return status;
        } else {
            if(bdpop.print_details)
                write_log_file("\t\tfinding in-between  blocks between two temp  blocks " + to_string(b_y) + " and " +
                                     to_string(producer) + "\n");
            bdpop.find_intermediate_blocks(b_y, producer, &intermediate_blocks);
            int status = extend_block(b_y, intermediate_blocks, false);
            return status;
        }
    } else {
        return -2;
    }
}


///attempts to remove a DP constraint between two block
int BlockDeorder :: remove_dp (int b_y, OrderingConstraint constraint){
    FactProxy factProxy = constraint.factProxy;
    if(bdpop.print_details)
        write_log_file("\tremoving dp<" + bdpop.print_fact(factProxy)  + " \n");
    vector<int> consumers;
    vector<int> intermediate_blocks;
    vector<int> internal_blocks;
    map<int, bool> visited = bdpop.get_boolean_map();
    bdpop.search_consumers_downward(b_y, factProxy, &consumers, &visited); /// searching producer upward
    if (consumers.size() > 0){
        for (int i = 0; i < consumers.size(); i++){
            if(bdpop.print_details)
                write_log_file("\t\tfinding in-between  blocks between two temp  blocks " + to_string(b_y) + " and " +
                                     to_string(consumers[i]) + "\n");
            bdpop.find_intermediate_blocks(b_y, consumers[i], &intermediate_blocks);
            for (int internal: intermediate_blocks){
                if(!is_in_vector(internal, &internal_blocks))
                    internal_blocks.push_back(internal);
            }
            intermediate_blocks.clear();
        }
        int status = extend_block(b_y, internal_blocks, false);
        return status;
    } else {
        return -2;
    }
}

///
/// \param b_x the id of temporary block
/// \param new_internals the vector of new internal's id
/// \param upward  1 if block is extending upward, otherwise 0
void BlockDeorder :: reestablish_new_internal_block_orderings(int b_x, vector<int> new_internals, bool upward) {
    if(bdpop.print_details)
        write_log_file("\t\t re-establishing internal blocks orderings \n");
    Block *block = bdpop.get_block(b_x);
    if(upward){
        for (int new_internal: new_internals) {
            for (int internal:block->internal_blocks) {
                if(old_orderings.have_ordering(new_internal, internal)){
                    Ordering ordering(new_internal, internal);
                    Ordering* oldOrdering = old_orderings.get_ordering(new_internal, internal);
                    for(OrderingConstraint constraint: oldOrdering->orderingConstraints)
                        bdpop.add_constraint(new_internal, internal, constraint);
                    ordering.is_transitive = oldOrdering->is_transitive;
//                    ordering.orderingConstraints = oldOrdering->orderingConstraints;
//                    bdpop.blocks_orderings.set_ordering(new_internal, internal, ordering);
                }
            }
            bdpop.clear_constraints(new_internal, b_x);
        }

    } else {
        for (int new_internal: new_internals) {
            for (int internal:block->internal_blocks) {
                if(old_orderings.have_ordering(internal, new_internal)){
                    Ordering ordering(internal, new_internal);
                    Ordering* oldOrdering = old_orderings.get_ordering(internal, new_internal);
                    for(OrderingConstraint constraint: oldOrdering->orderingConstraints)
                        bdpop.add_constraint(internal, new_internal, constraint);
                    ordering.is_transitive = oldOrdering->is_transitive;
//                    ordering.orderingConstraints = oldOrdering->orderingConstraints;
//                    bdpop.blocks_orderings.set_ordering(internal, new_internal, ordering);
                }
            }
            bdpop.clear_constraints(b_x, new_internal);
        }
    }

}

/////
///// \param b_x the id of temporary block
///// \param new_internals the vector of new internal's id
///// \param upward  1 if block is extending upward, otherwise 0
//void BlockDeorder :: reestablish_new_internal_block_orderings(int b_x, vector<int> new_internals, bool upward) {
//    if(bdpop.print_details)
//        write_log_file("\t\t re-establishing internal blocks orderings \n");
//    Block *block = bdpop.get_block(b_x);
//    if(upward){
//        for (int new_internal: new_internals) {
//            vector<OrderingConstraint> constraints = bdpop.blocks_orderings.get_constraints(new_internal, b_x);
//            for (int k = constraints.size() - 1; k >= 0; k--) {
//                OrderingConstraint n_constraint(constraints[k]);
//                if (constraints[k].is_causal_link()) {
//                    for (int internal:block->internal_blocks) {
//                        if(bdpop.is_consumer(internal, n_constraint.factProxy) ){
//                            if(!bdpop.have_constraint(internal, n_constraint.factProxy, PC,
//                                                      false)) {
//                                if (bdpop.has_causal_link(internal, n_constraint.factProxy)) continue;
//                                bdpop.add_constraint(new_internal, internal, n_constraint);
//                            }
//                        }
//
//                    }
//                }
//                else if (constraints[k].type == CD){
//                    for (int internal:block->internal_blocks) {
//                        if(bdpop.is_deleter(internal, n_constraint.factProxy))
//                            bdpop.add_constraint(new_internal, internal, n_constraint);
//                    }
//                }
//                else if (constraints[k].type == DP){ ///check causal
//                    for (int internal:block->internal_blocks) {
//                        if(bdpop.is_producer(internal, n_constraint.factProxy)){
//                            if(bdpop.has_causal_link(internal, n_constraint.factProxy) ||
//                            (bdpop.has_causal_link(b_x, n_constraint.factProxy)
//                            && bdpop.is_producer_in_block(internal, b_x, n_constraint.factProxy)))
//                                bdpop.add_constraint(new_internal, internal, n_constraint);
//                        }
//
//                    }
//                }
//                bdpop.clear_constraints(new_internal, b_x);
//            }
//        }
//
//    } else {
//        for (int new_internal: new_internals) {
//            vector<OrderingConstraint> constraints = bdpop.blocks_orderings.get_constraints( b_x, new_internal);
//            for (int k = constraints.size() - 1; k >= 0; k--) {
//                OrderingConstraint n_constraint(constraints[k]);
//                if (constraints[k].is_causal_link()) {
//                    for (int internal:block->internal_blocks) {
//                        if(bdpop.is_producer_in_block(internal, b_x, n_constraint.factProxy))
//                            bdpop.add_constraint(internal, new_internal, n_constraint);
//
//                    }
//                }
//                else if (constraints[k].type == CD){
//                    for (int internal:block->internal_blocks) {
//                        if(bdpop.is_consumer(internal, n_constraint.factProxy))
//                            bdpop.add_constraint(internal, new_internal, n_constraint);
//                    }
//                }
//                else if (constraints[k].type == DP){ ///check causal
//                    for (int internal:block->internal_blocks) {
//                        if(bdpop.is_deleter(internal, n_constraint.factProxy)){
//                                bdpop.add_constraint(internal, new_internal, n_constraint);
//                        }
//
//                    }
//                }
//                bdpop.clear_constraints( b_x, new_internal);
//            }
//        }
//    }
//}


/**
 * @brief extend a block either by creating a new block or by extending a permanent/temporary block. If the block b_x is
            temporary block, this block will be extended. otherwise, a new block will be created
 * @param b_x the block from which the extension begins upward/downward
 * @param internal_blocks the vector of id of the internal blocks
 * @param upward true, if the extension is upward, false if downward
 * @return -1 if the extension is unsuccessful, otherwise returns the extended block index
 */
int BlockDeorder :: extend_block(int b_x, vector<int> internal_blocks, bool upward){
    Block *block = bdpop.get_block(b_x);
    if (block->is_temporary){ /// Extending a temporary block
        if(!establish_extended_block(b_x, internal_blocks, upward)){
            return -1;
        }
//        bdpop.print_blocks_orderings();
        return b_x;
    } else {                  /// creating a new block
        int index = b_x;
        if (block->is_primary())
            index =  block->id;
        if (!is_in_vector(index, &internal_blocks))
            internal_blocks.push_back(index);
        int new_block_id =  bdpop.create_compound_block(internal_blocks);
        Block *block = bdpop.get_block(new_block_id);
        block->is_temporary = true;
        if (!establish_block_outer_orderings(block->id, block->internal_blocks)) {
            return -1;
        }

        return new_block_id;
    }
}


/**
 * @brief establish an extended block by finding its new preconditions, effects and orderings
 * @param upward true if the extension is upward, false if downward
 * @return true if establishment successful, false otherwise
 */
bool BlockDeorder :: establish_extended_block (int b_x, vector<int> new_internals, bool upward){

    Block *block = bdpop.get_block(b_x);
    Block *parent_block;
    if(block->parent > 0) parent_block = bdpop.get_block(block->parent);
    if(bdpop.print_details)
        write_log_file("\t>> extending the temporary block " + to_string(b_x) + " " + bdpop.get_block_name(b_x) + " ");

    vector<int> previous_internal_blocks = block->internal_blocks;
    if(is_in_vector(b_x, &new_internals)){
        delete_element_from_vector(&new_internals, b_x);
    }

//    bdpop.print_blocks_orderings();
    reestablish_new_internal_block_orderings(b_x, new_internals, upward); /// reestablish block's internal ordering with new internal blocks


    for (int internal: new_internals) {                       ///adding new internal  blocks
        block->internal_blocks.push_back(internal);
        Block* internal_block = bdpop.get_block(internal);
        internal_block->type = TEMP_INTERNAL;
        internal_block->parent = b_x;
        if (block->parent > 0)
            parent_block->delete_internal(internal);
    }
    if(bdpop.print_details)
        write_log_file( "to " + bdpop.get_block_name(b_x ) +"\n");
    if(bdpop.print_details){
        write_log_file("\t\t " + block->print() +"\n" );
    }
    sort(block->internal_blocks.begin(), block->internal_blocks.end());

    block->preconditions.clear();
    block->preconditions = bdpop.find_preconditions(block->internal_blocks);  /// finding new preconditions
    block->effects.clear();
    block->effects = bdpop.find_effects( block->internal_blocks);  /// finding new effects
//    write_log_file(block->print_preconditions_and_effects());

    if (upward && !establish_block_outer_orderings(b_x, new_internals))  /// extending links
            return false;

    vector <tuple< int, int, OrderingConstraint>> remove_pc_orderings;
    if(bdpop.print_details)
        write_log_file( "\testablishing orders\n\t\tchecking existing orderings\n");

    if(bdpop.print_details) write_log_file("\t\t\tchecking connections with successors\n");
    vector<int> successors = block->successors;
    for ( int successor : successors) {
        if (bdpop[successor]->type == TEMP_INTERNAL) continue;

        vector<OrderingConstraint> constraints = bdpop.blocks_orderings.get_constraints(b_x, successor);
        for (OrderingConstraint constraint: constraints) {
            if (constraint.type == PC && !bdpop.is_producer(b_x, constraint.factProxy)){
                tuple<int, int, OrderingConstraint> t(b_x, successor, constraint);
                remove_pc_orderings.push_back(t);
            }
            else if (constraint.type == CD && !bdpop.is_consumer(b_x, constraint.factProxy))
                bdpop.remove_constraint(b_x, successor, constraint);
            else if (constraint.type == DP && !bdpop.is_deleter(b_x, constraint.factProxy))
                bdpop.remove_constraint(b_x, successor, constraint);

        }
    }

    ///checking connections with predecessors
    if(bdpop.print_details) write_log_file("\t\t\tchecking connections with predecessors.\n");
    vector<int> predecessors = block->predecessors;
    for ( int predecessor : predecessors) {
        vector<OrderingConstraint> constraints = bdpop.blocks_orderings.get_constraints(predecessor, b_x);
        for (OrderingConstraint constraint: constraints){
            if (constraint.type == PC && !bdpop.is_consumer(b_x, constraint.factProxy))
                bdpop.remove_constraint(predecessor, b_x, constraint);
            else if (constraint.type == CD && !bdpop.is_deleter(b_x, constraint.factProxy))
                bdpop.remove_constraint(predecessor, b_x, constraint);
            else if (constraint.type == DP && !bdpop.is_producer(b_x, constraint.factProxy))
                bdpop.remove_constraint(predecessor, b_x, constraint);
        }

    }
    if (!upward && !establish_block_outer_orderings(b_x, new_internals))
            return false;


    for (int i = 0; i < remove_pc_orderings.size(); i++){
        if(!re_establish_pc(b_x, get<0>(remove_pc_orderings[i]), get<1>(remove_pc_orderings[i]),
                            &get<2>(remove_pc_orderings[i]), 1))
            return false;
        bdpop.remove_constraint(get<0>(remove_pc_orderings[i]), get<1>(remove_pc_orderings[i]),
                                &get<2>(remove_pc_orderings[i]));
        }
    return true;
}



/**
 * @brief establishes the orderings of a set of internal  blocks of a temporary block
 * @param b_x the index of the temporary block
 * @param internal_blocks the set of internal  blocks
 * @return true if the ordering establishment is successful, false otherwise
 */
bool BlockDeorder :: establish_block_outer_orderings(int b_x, vector<int> internal_blocks){
    vector <tuple< int, int, OrderingConstraint>> remove_pc_orderings;

    if(bdpop.print_details)
        write_log_file( "\t\testablishing  new outer orderings\n");

    for (int internal: internal_blocks) {
        Block* block_internal = bdpop.get_block(internal);

        ///establishing the connection with the successors
        vector<int> successors = block_internal->successors;
        for (int successor : successors) {
            if(bdpop.get_block(successor)->is_temp_internal()) continue;

            vector<OrderingConstraint> orderingConstraints = bdpop.blocks_orderings.get_constraints(internal, successor);
            for (OrderingConstraint constraint: orderingConstraints) {
                OrderingConstraint new_orderingConstraint(constraint);
                if(constraint.type == PC) {
                    if ( bdpop.is_candidate_producer(b_x, constraint.factProxy)) {
                        bdpop.add_constraint(b_x, successor, new_orderingConstraint);
                        bdpop.remove_constraint(internal, successor, constraint);
                    } else {
                        tuple<int, int, OrderingConstraint> t(internal, successor, constraint);
                        remove_pc_orderings.push_back(t);
                    }
                }
                else if (constraint.type == CD){
                    bdpop.remove_constraint(internal, successor, constraint);
                    if (bdpop.is_consumer(b_x, constraint.factProxy))
                        bdpop.add_constraint(b_x, successor, new_orderingConstraint);
                }
                else if (constraint.type == DP){
                    bdpop.remove_constraint(internal, successor, constraint);
                    if (bdpop.is_deleter(b_x, constraint.factProxy))
                        bdpop.add_constraint(b_x, successor, new_orderingConstraint);
                }
            }
        }

        ///establishing the connection with the predecessors
        vector<int> predecessors = block_internal->predecessors;
        for (int predecessor: predecessors) {
//            if (bdpop.is_in_vector(predecessor, &internal_blocks)) continue;
            if(bdpop.get_block(predecessor)->is_temp_internal()) continue;
            vector<OrderingConstraint> orderingConstraints = bdpop.blocks_orderings.get_constraints(predecessor, internal);
            for (OrderingConstraint constraint: orderingConstraints) {
                OrderingConstraint new_orderingConstraint(constraint);
                if (constraint.is_causal_link()) {
                    if ( bdpop.is_consumer(b_x, constraint.factProxy)) {
                        bdpop.add_constraint(predecessor, b_x, new_orderingConstraint);
                        bdpop.remove_constraint(predecessor, internal, constraint);
                    } else {
                        if(!re_establish_pc(b_x, predecessor, internal, &constraint, 2))
                            return false;
                        bdpop.remove_constraint(b_x, predecessor, constraint);
                    }
                }
                else if (constraint.type == CD){
                    bdpop.remove_constraint(predecessor, internal, constraint);
                    if (bdpop.is_deleter(b_x, constraint.factProxy))
                        bdpop.add_constraint(predecessor, b_x, new_orderingConstraint);
                }
                else if (constraint.type == DP){ ///check causal
                    bdpop.remove_constraint(predecessor, internal, constraint);
                    if (bdpop.is_producer(b_x, constraint.factProxy))
                        bdpop.add_constraint(predecessor, b_x, new_orderingConstraint);
                }
            }
        }


    }

    for (int i = 0; i < remove_pc_orderings.size(); i++){
        if(!re_establish_pc(b_x, get<0>(remove_pc_orderings[i]), get<1>(remove_pc_orderings[i]),
                            &get<2>(remove_pc_orderings[i]), 1))
            return false;
        bdpop.remove_constraint(get<0>(remove_pc_orderings[i]), get<1>(remove_pc_orderings[i]),
                                &get<2>(remove_pc_orderings[i]));
    }
    return true;
}

///**
// * @brief tries to remove an invalid ordering constraint between block b_x and block b_y due to the construction of
// *          temporary block b_temp. New ordering might be needed to create in order to remove of a causal link.
// * @param b_temp the index of the temporary constraint
// * @param b_x the index of the first operator
// * @param b_y the index of the second operator
// * @param orderingConstraint the ordering constraint
// * @param index the index of the ordering constraint between block b_x and block b_y
// * @param direct true, no need to check pc connection
// * @return false if a new necessary ordering can not be created in order to remove a causal link, true otherwise
// */
//bool BlockDeorder :: remove_ordering(int b_temp, int b_x, int b_y, OrderingConstraint* orderingConstraint, int type) {
//    /// type = -1, when we don't need to re-establish the pc connection and we can directly remove the constraint
//    if (type > -1 && orderingConstraint->type == PC) {   /// write again
//        if(!re_establish_pc(b_temp, b_x, b_y, orderingConstraint, type))
//            return false;
//    }
////    tempBlocksOrderings.store(b_temp, b_x, b_y, orderingConstraint, false);
//    bdpop.remove_constraint(b_x, b_y, orderingConstraint);
//    return true;
//}


bool BlockDeorder :: re_establish_pc(int b_temp, int b_x, int b_y, OrderingConstraint* orderingConstraint, int type) {
    Block *block_temp = bdpop[b_temp];
    int producer = -1;
    if(bdpop.print_details) write_log_file( "type: " + to_string(type) + " " +bdpop.print_fact(orderingConstraint->factProxy));
    /// type = 1, when b_x is an internal block of b_temp
    if(type == 1){
        if (bdpop.is_consumer(b_temp, orderingConstraint->factProxy)) {
            producer = bdpop.get_producer(b_temp, orderingConstraint->factProxy);
        }
        else {
            producer = bdpop.find_earliest_candidate_producer(b_y, orderingConstraint->factProxy, vector<int>(0));
            if(producer < 0)
                producer = bdpop.find_earliest_candidate_producer(b_temp, orderingConstraint->factProxy, vector<int>(0));
        }
    }
    /// type = 2, when b_y is an internal block of b_temp, it may occcur when the previous PC connection is conditional.
    else if (type == 2){
        producer = bdpop.find_earliest_candidate_producer(b_temp, orderingConstraint->factProxy, vector<int>(0));
        if(producer > -1){
            OrderingConstraint new_orderingConstraint(orderingConstraint);
            bdpop.add_constraint(producer,b_temp, new_orderingConstraint);
            return true;
        }
    }
    /// type =0, when none of the b_x and b_y is internal block of b_temp
    else if(type == 0){
        producer = bdpop.find_earliest_candidate_producer(b_y, orderingConstraint->factProxy, vector<int>(0));
    }
    if (producer  < 0) {
        if(bdpop.print_details) {
            write_log_file( "no producer " + bdpop.print_fact(orderingConstraint->factProxy) +" "
            + to_string(b_temp) + block_temp->getTypeName() + " " + to_string(block_temp->parent) + "\n");
        }
        if (block_temp->is_internal())
            cout << " " << bdpop.is_consumer(block_temp->parent, orderingConstraint->factProxy) << endl;
        return false;
    } else{
        OrderingConstraint new_orderingConstraint(orderingConstraint);
        bdpop.add_constraint(producer, b_y, new_orderingConstraint);
        return true;
    }
}

void BlockDeorder :: write_bd_result(){
    blockDeorderPlan.calculate_flex(); /// calculating flex of the BDPO plan
    double time = deorderManager.get_time();
    Plan final_plan = blockDeorderPlan.get_total_order_plan();
    double cost = calculate_plan_cost(final_plan, blockDeorderPlan.task_proxy);
    double flex =  blockDeorderPlan.flex[0];
    double cflex = blockDeorderPlan.flex[1];

    fstream fout;
    fout.open("bd_out.csv", ios::out);    /// open result file

    fout<< "bd_flex, bd_cflex, bd_cost, bd_time \n";
    fout << flex << "," << cflex << "," << cost << "," << time << endl;
    fout.close();
//
    write_in_file("step.txt", "\t<block_deorder>"+ to_string(flex) +
                              "\t" + to_string(cflex) + "\t"+ to_string(cost) + "\t" + to_string(time) + "\n");


    bdpop.write_block_deorder_plan("bd.txt");
    bdpop.print_bdop_dot_file( false, "bd.dot");
//    if(print_details)
//        bdpop.print_bdop_dot_file(true, "tpop.dot");
}

