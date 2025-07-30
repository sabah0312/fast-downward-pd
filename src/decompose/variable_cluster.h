//
// Created by sabah on 7/2/22.
//

#ifndef FAST_DOWNWARD_VARIABLE_CLUSTER_H
#define FAST_DOWNWARD_VARIABLE_CLUSTER_H

#include <vector>
#include <iostream>

using namespace std;

enum cluster_type  {agent, regular1};
class VariableCluster {
    int id;
    vector<int> ground_variables;
    cluster_type type;
//    shared_ptr<AbstractTask> task;
//    TaskProxy task_proxy;
public:
    VariableCluster(int id, cluster_type type): id(id), type(type){
    }
    void push_back(int variable){
        ground_variables.push_back(variable);
    }
    int get_id(){
        return id;
    }
    void push_back1(int v){
        ground_variables.push_back(v);
    }
    void set_type(cluster_type c_type){
        type = c_type;
    }
    cluster_type get_type(){
        return type;
    }
    vector<int> get_ground_variables(){
        return ground_variables;
    }

    string get_type_name(){
        if(type == agent)
            return "agent";
        else
            return "regular";
    }
};


#endif //FAST_DOWNWARD_VARIABLE_CLUSTER_H
