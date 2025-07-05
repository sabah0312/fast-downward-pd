
#include <fstream>
#include "../../search/plugins/plugin.h"
#include "../deorder_algorithm.h"
#include "eog.h"
#include "../myutils.h"

using namespace std;

void EOG :: do_step_deordering(){
    cout<< "--------------------------------Start step-wise deordering ----------------------------------------------"<< endl;
    find_pcs();
    find_cds_and_dps();
    cout<< "--------------------------------Finished step-wise deordering ----------------------------------------------"<< endl << endl;
    blockDeorderPlan.find_transitive_ordering();
    write_eog_result();

    cout<<"finding transitive ordering done" << endl;

}

void EOG :: find_pcs(){
    for (int consumer = 1; consumer < blockDeorderPlan.size(); consumer++){
        Block* block_i = blockDeorderPlan.get_block(consumer);
        for (FactProxy factProxy : block_i->preconditions){
            int candidateProducer = find_candidate_producer(consumer, factProxy, false);

            if(candidateProducer >=0){
                OrderingConstraint new_pc = OrderingConstraint(factProxy, PC);
                add_pc_constraint(candidateProducer, consumer, new_pc);
            } else {
                cout<< "no producer";
                continue;
            }
        }
    }
}

void EOG :: find_cds_and_dps(){
    int deleter_op;
    for (int i=0; i < blockDeorderPlan.size(); i++) {
        Block *block_i = blockDeorderPlan.get_block(i);
        for(FactProxy factProxy: block_i->preconditions){
            for (int j=i+1; j < blockDeorderPlan.size(); ++j){
                if (blockDeorderPlan.is_deleter(j, factProxy)) {
                    OrderingConstraint new_cd = OrderingConstraint(factProxy, CD);
                    blockDeorderPlan.add_constraint(i,j, new_cd);
                }
            }
        }

        /// start finding dps
        for (EffectProxy effectProxy : block_i->get_effects()){
            FactProxy factProxy = effectProxy.get_fact();
            if (!blockDeorderPlan.have_constraint(i, factProxy, PC, false))
                continue;
            for (int j=i-1; j>=1; j--){
                if (blockDeorderPlan.is_deleter(j, factProxy)) {
                    OrderingConstraint new_dp = OrderingConstraint(factProxy, DP);
                    blockDeorderPlan.add_constraint(j, i, new_dp);
                }
            }
        }
    }
}

void EOG :: add_pc_constraint(int b_x, int b_y, OrderingConstraint constraint){
    blockDeorderPlan.add_constraint(b_x, b_y, constraint);
    EffectProxy effectProxy = get_producing_effect(b_x, constraint.factProxy);
    for(FactProxy factProxy: effectProxy.get_conditions()){
        cout<< factProxy.get_name() << endl;
        int i = find_candidate_producer(b_x, factProxy, false);
        if(i< -1){
            cout << "error";
        }
        OrderingConstraint conditional_pc = OrderingConstraint(factProxy, PC, true);
        conditional_pc.associated_constraints.push_back(&constraint);
        add_pc_constraint(i, b_x, conditional_pc);
    }
}

EffectProxy EOG :: get_producing_effect(int b_x, FactProxy f){
    Block *block = blockDeorderPlan.get_block(b_x);

    vector<EffectProxy> effectsProxy = block->get_effects();

    for (EffectProxy effectProxy: effectsProxy){
        if (effectProxy.get_fact().get_pair() == f.get_pair()){
            bool is_candidtate = true;
            for (int i = 0; i < effectProxy.get_conditions().size(); i++){
                FactProxy factProxy = effectProxy.get_conditions()[i];
                int op_p = find_candidate_producer(b_x, factProxy, true);
                if(op_p == -1) {
                    is_candidtate = false;
                    break;
                }
            }
            if(is_candidtate){
                return effectProxy;
            }
        }
    }
}

///// @brief This function extracts the producer-consumer(PC) order constraints (causal-links) in the total-order total_order_plan
int EOG :: find_candidate_producer(int b_x, FactProxy factProxy, bool is_latest){
    int earlier_op = -1;
    for (int j = b_x - 1; j >= 0; j--){
        if (is_candidate_producer(j, factProxy, is_latest)) {
            earlier_op = j;
            if(is_latest)
                break;
        }
        else if(j != blockDeorderPlan.init_block && blockDeorderPlan.is_deleter(j, factProxy)){
            break;
        }
    }
    if (earlier_op >= 0) {
        return earlier_op;
    }
    else return -1;
}

bool EOG:: is_candidate_producer(int b_x, FactProxy f, bool is_latest){
    if (blockDeorderPlan.is_consumer(b_x, f)) return false;
    Block *block = blockDeorderPlan.get_block(b_x);
    vector<EffectProxy> effectsProxy = block->get_effects();
    for (EffectProxy effectProxy: effectsProxy){
        if (effectProxy.get_fact().get_pair() == f.get_pair()){
            bool is_candidtate = true;
            for (int i = 0; i < effectProxy.get_conditions().size(); i++){
                FactProxy factProxy = effectProxy.get_conditions()[i];
                int op_p = find_candidate_producer(b_x, factProxy, is_latest);
                if(op_p == -1) {
                    is_candidtate = false;
                    break;
                }
            }
            if(is_candidtate) {
                return true;
            }
        }
    }
    return false;
}

void EOG :: write_eog_result(){
    blockDeorderPlan.calculate_flex(); /// calculating flex of the BDPO plan

    double time = deorderManager.get_time();
    Plan final_plan = blockDeorderPlan.get_total_order_plan();
    double cost = calculate_plan_cost(final_plan, blockDeorderPlan.task_proxy);
    double flex =  blockDeorderPlan.flex[0];
    double cflex = blockDeorderPlan.flex[1];

    fstream fout;
    fout.open("eog_out.csv", ios::out);    /// open result file

    fout<< "eog_flex, eog_cflex, eog_cost, eog_time \n";
    fout << flex << "," << cflex << "," << cost << "," << time << endl;
    fout.close();
//
    write_in_file("step.txt", "\t<eog>"+ to_string(flex) +
                              "\t" + to_string(cflex) + "\t"+ to_string(cost) + "\t" + to_string(time) + "\n");

    blockDeorderPlan.write_block_deorder_plan("pop.txt");
    blockDeorderPlan.print_bdop_dot_file( false, "pop.dot");
}

void add_eager_search_options_to_feature(
        plugins::Feature &feature, const string &description) {
}

tuple<string, string>
get_eager_search_arguments_from_options(const plugins::Options &opts) {
    return get_deorder_algorithm_arguments_from_options(opts);
}
