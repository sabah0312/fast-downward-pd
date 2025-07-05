//
// Created by oem on ১৬/১১/২১.
//

#ifndef FAST_DOWNWARD_ORDERINGS_H
#define FAST_DOWNWARD_ORDERINGS_H
#include "map";
#include "ordering_constraints.h"


using namespace std;

class Orderings {


public:
    map<pair<int, int>, Ordering> orderings;

    Ordering* get_ordering(int b_x, int b_y){
        return &orderings.at(pair<int, int>(b_x, b_y));
    }

    int have_ordering (int b_x, int b_y){
        return orderings.count(pair<int, int>(b_x, b_y)) > 0;
    }

    bool have_constraint(int b_x, int b_y, OrderingConstraint constraint) {
        if(have_ordering(b_x, b_y)) {
            vector<OrderingConstraint> constraints = get_constraints(b_x, b_y);
            return count(constraints.begin(), constraints.end(), constraint) > 0;
        }
        return false;
    }

    bool has_any_ordering(int b_x, int b_y){
        if(have_ordering(b_x, b_y))
            return get_ordering(b_x, b_y)->has_any_ordering();
        return false;
    }

    bool have_constraint(int b_x, int b_y, FactProxy f, constraint_type t){
        for (OrderingConstraint constraint: get_constraints(b_x, b_y)){
            if (constraint.type == t && constraint.factProxy.get_pair()  == f.get_pair()) return true;
        }
        return false;
    }

    bool have_constraint(int b_x, int b_y, constraint_type t){
        for (OrderingConstraint constraint: get_constraints(b_x, b_y)){
            if (constraint.type == t) return true;
        }
        return false;
    }



    bool has_ordering_except(int b_x, int b_y, FactProxy f, constraint_type type){
        vector<OrderingConstraint> constraints = get_constraints(b_x, b_y);
        for(OrderingConstraint constraint: constraints){
            if(constraint.factProxy.get_pair() == f.get_pair() && constraint.type == type) continue;
            else {
                return true;
            }
        }
        return false;
    }

    void set_transitive(int b_x, int b_y, bool is_transitive){
        if(have_ordering(b_x, b_y))
            orderings.at(pair<int, int>(b_x, b_y)).is_transitive = is_transitive;
    }

    void set_ordering(int b_x, int b_y, Ordering ordering){
        orderings.at(pair<int, int>(b_x, b_y)) = ordering;
    }

    void clear(){
        orderings.clear();
    }

    void clear_constraints(int b_x, int b_y){
        if(have_ordering(b_x, b_y)){
            orderings.at(pair<int, int>(b_x, b_y)).orderingConstraints.clear();
            // set_transitive(b_x, b_y, false);
        }
    }

    void erase_ordering (int b_x, int b_y){
        if (have_ordering(b_x, b_y)){
            orderings.erase(pair<int, int>(b_x, b_y));
        }
    }

    void remove_constraint(int b_x, int b_y, int position){
        auto begin = orderings.at(pair<int, int>(b_x, b_y)).orderingConstraints.begin();
        orderings.at(pair<int, int>(b_x, b_y)).orderingConstraints.erase(begin + position);
    }

    void remove_constraint(int b_x, int b_y, OrderingConstraint constraint){
        if (have_ordering(b_x, b_y)){
            vector<OrderingConstraint> constraints = get_ordering(b_x, b_y)->orderingConstraints;
            for(int i =0; i < constraints.size(); i++){
                if (constraints [i] == constraint){
                    remove_constraint(b_x, b_y, i);
                    break;
                }
            }
        }
    }

    bool is_transitive(int b_x, int b_y) {
        if(have_ordering(b_x, b_y))
            return get_ordering(b_x, b_y)->is_transitive;
        return false;
    }

    OrderingConstraint get_constraint(int b_x, int b_y, int index){
        return get_ordering(b_x, b_y)->orderingConstraints[index];
    }

    vector<OrderingConstraint>  get_constraints(int b_x, int b_y) {
        vector<OrderingConstraint> constraints;
        if (have_ordering(b_x, b_y))
            return get_ordering(b_x, b_y)->orderingConstraints;
        constraints.clear();
        return constraints;
    }

    OrderingConstraint* add_constraint(int b_x, int b_y, OrderingConstraint constraint){
        if (!have_ordering(b_x, b_y)){
            pair<int, int> p(b_x, b_y);
            Ordering op(b_x, b_y);
            orderings.insert(pair<pair<int, int>, Ordering>(p, op));
        }
        if (have_constraint(b_x, b_y, constraint)) return NULL;
        constraint.operatorOrdering = get_ordering(b_x, b_y);
        orderings.at(pair<int, int>(b_x, b_y)).orderingConstraints.push_back(constraint);
        return &get_ordering(b_x, b_y)->orderingConstraints.back();
    }

    void add_ordering(int b_x, int b_y){
        if (!have_ordering(b_x, b_y)){
            pair<int, int> p(b_x, b_y);
            Ordering op(b_x, b_y);
            orderings.insert(pair<pair<int, int>, Ordering>(p, op));
        }
    }

};


#endif //FAST_DOWNWARD_ORDERINGS_H
