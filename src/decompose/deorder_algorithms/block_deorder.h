//
// Created by sabah on ২৪/৪/২০.
//

#ifndef FAST_DOWNWARD_BLOCK_DEORDER_H
#define FAST_DOWNWARD_BLOCK_DEORDER_H

#include "map"
#include "../deorder_algorithm.h"
#include "eog.h"
#include "../myutils.h"

using namespace std;

class BlockDeorder : public EOG {

public:
    BlockDeorder():EOG(){}
    BlockDeorderPlan bdpop;
    Orderings old_orderings;

    bool do_block_deordering() {
        auto check_pair = [&](int i, int j) -> bool{
            bdpop = blockDeorderPlan;
            old_orderings = bdpop.blocks_orderings;
            bool new_block_found = remove_ordering(i, j);

            if (new_block_found) {
                if(print_details)
                    cout << "\n**new blocks has been found**" << endl;
                bdpop.find_transitive_ordering();
                bdpop.calculate_flex();

                if (bdpop.flex[0] < blockDeorderPlan.flex[0]){
                    if(print_details)
                        cout << "\n\n** new flex is less!!!!"<< endl;
                    return false;
                }
                blockDeorderPlan = bdpop;
                if(print_details)
                    cout << "\n**new blocks has been found**" << endl;
                return true;
            }
            return false;
        };

        for (auto itr_i = blockDeorderPlan.blocks.begin(); itr_i != blockDeorderPlan.blocks.end(); itr_i++) {
            int i = itr_i->first;
            if (i == blockDeorderPlan.init_block || i == blockDeorderPlan.goal_block) continue;
            Block* block = blockDeorderPlan.get_block(i);
            for(int successor: block->successors){
//                if (is_time_maxed()) return false;
                if(successor == blockDeorderPlan.goal_block || blockDeorderPlan.blocks_orderings.is_transitive(i, successor))
                    continue;
                if(check_pair(i, successor)) return true;
            }
        }
        cout << "********* no more block extension possible.************\n";
        return false;

    }

    void block_deorder_start(){
        write_log_file("\n******  Block Deorder Started ********\n");
//        bdpop = blockDeorderPlan;
        bool any_block_found = false;
        bool new_block_found = true;
        while (new_block_found) {
            blockDeorderPlan.find_transitive_ordering();
            new_block_found = do_block_deordering();
            if(!any_block_found) any_block_found = new_block_found;
            blockDeorderPlan.calculate_flex();
//            if(new_block_found)
//                write_time_file();
        }
        write_log_file("\n******  Block Deorder Completed ********\n");

//        write_in_result_file(3);
    }

    void run() override {
        initialize();
        do_step_deordering();
        write_eog_result();
        block_deorder_start();
        write_bd_result();

    }


public:
    bool initilize_up_first = false;

    bool establish_block_outer_orderings(int b_x, vector<int> internal_blocks );

    bool re_establish_pc(int b_temp, int b_x, int b_y, OrderingConstraint *orderingConstraint, int type);

    bool remove_ordering(int b_x, int b_y);

    int remove_pc(int b_x, OrderingConstraint constraint );

    int extend_block(int b_x, vector<int> internal_blocks, bool upward );

    bool
    establish_extended_block(int b_x, vector<int> new_internals, bool upward );

    int remove_dp(int b_y, OrderingConstraint constraint );

    int remove_cd(int b_x, int b_y, OrderingConstraint constraint, bool direction_up );

    OrderingConstraint choose_constraint(int b_x, int b_y);

    void reestablish_new_internal_block_orderings(int b_x, vector<int> new_internals, bool upward);

    void write_bd_result();
};


#endif //FAST_DOWNWARD_BLOCK_DEORDER_H
