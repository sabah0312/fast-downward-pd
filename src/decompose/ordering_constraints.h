//
// Created by sabah on ১৫/৮/২০.
//

#ifndef FAST_DOWNWARD_ORDERING_CONSTRAINTS_H
#define FAST_DOWNWARD_ORDERING_CONSTRAINTS_H

#include "../search/task_proxy.h"
#include "map"

using namespace std;
enum constraint_type  {PC, CD, DP, OC, OCP, POC, NCON};

struct Ordering;

/// @brief This structure is for storing a single ordering constraint,
struct OrderingConstraint {
    constraint_type  type; /// indicates the type of the constraint : PC, CD or DP  */
    bool is_conditional;
    bool is_incidental;
    FactProxy factProxy; /// the fact of the constraint */
    vector<OrderingConstraint*> associated_constraints; /// in the case of conditional pc
    vector<EffectProxy> associated_effects; /// in the case of ipc and conditional pc
    Ordering* operatorOrdering;

    bool is_causal_link(){
        if(type == PC && !is_incidental) return true;
        return false;
    }

    OrderingConstraint(OrderingConstraint *constraint): factProxy(constraint->factProxy), type(constraint->type),
    is_conditional(constraint->is_conditional), associated_constraints(constraint->associated_constraints){
        is_incidental= constraint->is_incidental;
    }
    OrderingConstraint(FactProxy f, constraint_type t)
            : factProxy(f), type(t){
        is_conditional = false;
        is_incidental = false;
    }

    OrderingConstraint(FactProxy f, constraint_type t, bool conditional)
            : factProxy(f), type(t), is_conditional(conditional), is_incidental(false){
    }

    OrderingConstraint(FactProxy f, constraint_type t, bool conditional, bool incidental)
            : factProxy(f), type(t), is_conditional(conditional), is_incidental(incidental){
    }
    string getTypeName (){
        if (type == PC) {
            if (is_incidental)
                return "iPC";
            else if(is_conditional)
                return  "cPC";
            else
                return "PC";
        }
        else if (type == CD) return  "CD";
        else if (type == DP) return "DP";
        else if (type == OC) return "OC";
        else if (type == OCP) return "OCp";
        else if (type == POC) return "pOC";
        else if (type == NCON) return "NCON";
    }

    bool is_incidental_pc(){
        if(is_conditional && is_incidental)
            return true;
        return false;
    }

    int get_associated_constraint(OrderingConstraint* constraint){
        if(associated_constraints.empty())
            return -1;
        for(int i=0; i < associated_constraints.size();i++){
            if (associated_constraints[i] == constraint)
                return i;
        }
        return -1;
    }

    void remove_associated_constraint(int index){
        associated_constraints.erase(associated_constraints.begin() + index);
    }
    bool has_other_associated_constraint(OrderingConstraint* constraint){
        if(associated_constraints.empty())
            return false;
        for(OrderingConstraint* cons: associated_constraints){
            if (cons == constraint)
                continue;
            else
                return true;
        }
        return false;
    }

    bool is_conditional_causal_link(){
        if(is_conditional && !is_incidental)
            return true;
        return false;
    }

    bool operator==(const OrderingConstraint &other) const {
        return type == other.type && factProxy.get_pair() == other.factProxy.get_pair() &&
        is_conditional == other.is_conditional && is_incidental == other.is_incidental;
    }
    bool operator!=(const OrderingConstraint &other) const {
        return type != other.type || factProxy.get_pair() != other.factProxy.get_pair() || is_conditional != other.is_conditional
        || is_incidental == other.is_incidental;
    }
    string print(){
        string s;
        if(type == NCON){
            return  getTypeName() + "<" + to_string(factProxy.get_variable().get_id()) + ">";
        } else {
            s = getTypeName() + "<" + to_string(factProxy.get_variable().get_id()) + ":" + to_string(factProxy.get_value()) + ">";
            if (is_conditional) {
                 s+=" !cond";
                for (OrderingConstraint *constraint: associated_constraints) {
                    s+= "(";
                    constraint->print();
                    s+= ")";
                }
            }
            if (is_incidental) {
                s+= " ~i";
            }
        }
        return s;
    }

};


/// @brief This structure holds the all the ordering constraints between two operator of a bdpop
struct Ordering {
    int b_x;
    int b_y;
    bool is_transitive;
    vector<OrderingConstraint> orderingConstraints;
    Ordering(int x, int y): b_x(x), b_y(y), is_transitive(false){
    }
    bool has_any_ordering(){
        return !orderingConstraints.empty();
    }

};



#endif //FAST_DOWNWARD_ORDERING_CONSTRAINTS_H
