//
// Created by sabah on 7/2/22.
//

#ifndef FAST_DOWNWARD_CLUSTER_CAUSAL_GRAPH_H
#define FAST_DOWNWARD_CLUSTER_CAUSAL_GRAPH_H


#include "variable_cluster.h"
#include "../search/abstract_task.h"
#include "../search/task_proxy.h"
#include "../search/task_utils/causal_graph.h"

class ClusterCausalGraph {
//    shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    map<int, VariableCluster> clusters;
    map<int, int> variable_cluster_map;
    map<int, vector<int>> causal_graph;
    int next_id = 0;
    vector<vector<int>> candidate_clusters;


    bool is_same(vector<int> v1, vector<int> v2);


public:
    VariableCluster *get_cluster(int id);

    int get_cluster_for_variable(int v_id){
        return variable_cluster_map.at(v_id);
    }
    void compute_cluster();

    ClusterCausalGraph();
    void print_clusters();

    void add_to_cluster(int v_id, int cluster_id);

    bool is_in_same_cluster(int v_id1, int v_id2);

    string print_cluster_causal_graph();

    int make_new_cluster(vector<int> variables);

    void compute_variable_cluster();

    void refine_candidate_cluster();

    tuple<int, int> count_agent_cluster_variable();

    string print_variable_cluster(int id);
};


#endif //FAST_DOWNWARD_CLUSTER_CAUSAL_GRAPH_H
