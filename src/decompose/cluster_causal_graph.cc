//
// Created by sabah on 7/2/22.
//

#include <map>
#include "cluster_causal_graph.h"
#include "variable_cluster.h"
#include "../search//heuristics/domain_transition_graph.h"
#include "myutils.h"
#include "../search/tasks/root_task.h"

using namespace domain_transition_graph;
using namespace std;
//ClusterCausalGraph ::ClusterCausalGraph(): task(modified_task::modified_root_task),
//                                           task_proxy(*task){
//
//}

ClusterCausalGraph ::ClusterCausalGraph(): task_proxy(*tasks::g_root_task){}
//
//}
//ClusterCausalGraph ::ClusterCausalGraph(){}

void ClusterCausalGraph:: compute_cluster(){
    causal_graph::CausalGraph cg =task_proxy.get_causal_graph();
    function<bool(int, int)> pruning_condition =
            [](int dtg_var, int cond_var) {return dtg_var <= cond_var;};
    DTGFactory factory(task_proxy, false, pruning_condition);
    std::vector<std::unique_ptr<domain_transition_graph::DomainTransitionGraph>> transition_graphs;
    transition_graphs = factory.build_dtgs();

    VariablesProxy variablesProxy =task_proxy.get_variables();
    map<int, vector<int>> initial_clusters;
    for(VariableProxy variableProxy: variablesProxy){
        int size = cg.get_pre_to_eff(variableProxy.get_id()).size();
        if(initial_clusters.count(size) == 0){
            pair<int, vector<int>> p (size, vector<int>(0));
            initial_clusters.insert(p);
        }
        initial_clusters.at(size).push_back(variableProxy.get_id());
    }


    ///filter cluster
    for(auto c:initial_clusters){
        auto cluster = c.second;
        map<int, vector<int>> new_clusters;
        while (!cluster.empty()){
            int v_id = cluster.front();
            cluster.erase(cluster.begin());
            int domain_size = variablesProxy[v_id].get_domain_size();
            if(new_clusters.count(domain_size) == 0){
                pair<int, vector<int>> p (domain_size, vector<int>(0));
                new_clusters.insert(p);
            }
            new_clusters.at(domain_size).push_back(v_id);

        }
        for(auto c1: new_clusters){
            candidate_clusters.push_back(c1.second);
        }
    }

    refine_candidate_cluster();

    for(int c = 0 ; c< candidate_clusters.size();c++){
        make_new_cluster(candidate_clusters[c]);
    }


    /// computing cluster_causal_graph
    for(auto itr: clusters){
        int v_id = itr.second.get_ground_variables()[0];
        pair<int, vector<int>> p(itr.first, vector<int>(0));
        causal_graph.insert(p);
        for(int v2: cg.get_pre_to_eff(v_id)){
            int c_id = variable_cluster_map.at(v2);
            if (std::count(causal_graph.at(itr.first).begin(), causal_graph.at(itr.first).end(), c_id) == 0 ) {
                causal_graph.at(itr.first).push_back(c_id);
            }
        }
    }

    /// assign type to clusters
    map<int, bool> in_ward;
    for(auto itr: clusters){
        in_ward.insert(pair<int, bool> (itr.first, true));
    }
    for(auto itr: causal_graph){
        for(int c_id: itr.second){
            in_ward.at(c_id) = false;
        }
    }
    for(auto itr: in_ward){
        if(itr.second)
            clusters.at(itr.first).set_type(agent);
    }
    print_clusters();
}

string ClusterCausalGraph :: print_variable_cluster(int id){
    VariableCluster* cluster= &clusters.at(id);
    string s= "";
    s += "<" + to_string(id) + ">: " + cluster->get_type_name() + "\n";
    for(int v_id: cluster->get_ground_variables()){
        s +=  "\t[" + to_string(v_id) + "] "  + task_proxy.get_variables()[v_id].get_fact(0).get_name() + "\n";
    }
    s += "\n";
    return s;
}

int ClusterCausalGraph:: make_new_cluster(vector<int> variables){
    VariableCluster variableCluster(next_id, regular);
    pair<int, VariableCluster> p(next_id, variableCluster);
    clusters.insert(p);
    int id = variableCluster.get_id();
    for(int v: variables){
        clusters.at(id).push_back1(v);
    }
//    cout << " new cluster \n";
//    variableCluster.print();
    next_id++;
    return id;
}

void ClusterCausalGraph :: compute_variable_cluster(){
    variable_cluster_map.clear();
    for(int c=0; c < candidate_clusters.size();c++){
        for(int v: candidate_clusters[c]){
            pair<int, int> p(v, c);
            variable_cluster_map.insert(p);
        }
    }
}

void ClusterCausalGraph:: refine_candidate_cluster(){

    causal_graph::CausalGraph cg =task_proxy.get_causal_graph();
    compute_variable_cluster();

    vector<map<int, map<int, vector<int>>>> cluster_variable_cg_mapping(candidate_clusters.size());
    //cluster
    for(int c=0; c <candidate_clusters.size(); c++){
        //cluster_variable
        for(int v: candidate_clusters[c]){
            vector<int> ids = cg.get_pre_to_eff(v);
            map<int, vector<int>> m;
            pair<int, map<int, vector<int>>> p(v, m);
            cluster_variable_cg_mapping[c].insert(p);
            for(auto id:ids){
                int c_id = variable_cluster_map.at(id);
                if(cluster_variable_cg_mapping[c].at(v).count(c_id) == 0){
                    pair<int, vector<int>> p1(c_id, vector<int>(0));
                    cluster_variable_cg_mapping[c].at(v).insert(p1);
                }
                cluster_variable_cg_mapping[c].at(v).at(c_id).push_back(id);
            }
        }
    }


    bool is_same;
    for(int c = 0; c < candidate_clusters.size(); c++){
        is_same = true;
        vector<int> variables = candidate_clusters[c];
        map<int, vector<int>> new_clusters;
        vector<int> seeds;
        seeds.push_back(variables.front());
        new_clusters.insert(pair<int, vector<int>>(variables.front(), vector<int>(0)));
        new_clusters.at(variables.front()).push_back(variables.front());
//

        for(int v = 1; v < variables.size(); v++){
            int v_id = variables[v];
            map<int, vector<int>> map_v = cluster_variable_cg_mapping[c].at(v_id);
            bool found = false;
            for(int s:seeds){
                bool match = true;
                map<int, vector<int>> individual = cluster_variable_cg_mapping[c].at(s);
                for(auto m: map_v){
                    int c_id = m.first;
                    if(individual.count(c_id) > 0 && m.second.size() == individual.at(c_id).size()){
//
//                    }
                    } else {
                        match = false;
                        continue;
                    }
                }
                if(match){
                    new_clusters.at(s).push_back(v_id);
                    found = true;
                    break;
                }
            }
            if(!found){
                seeds.push_back(v_id);
                new_clusters.insert(pair<int, vector<int>>(v_id, vector<int>(0)));
                new_clusters.at(v_id).push_back(v_id);
            }

        }
        if(new_clusters.size() == 1)
            continue;
        else{
            for(auto c:new_clusters){
                candidate_clusters.push_back(c.second);
            }
            candidate_clusters.erase(candidate_clusters.begin() + c);
            refine_candidate_cluster();
            return;
        }
    }
}

void ClusterCausalGraph :: add_to_cluster(int v_id, int cluster_id){
    clusters.at(cluster_id).push_back(v_id);
    pair<int, int> p(v_id, cluster_id);
    variable_cluster_map.insert(p);
}

VariableCluster* ClusterCausalGraph :: get_cluster(int id){
    return &clusters.at(id);
}

bool ClusterCausalGraph :: is_same(vector<int> v1, vector<int> v2){
    for(int i: v1){
        for(int j= v2.size() -1; j>=0; j--){
            if(i==v2.at(j) or is_in_same_cluster(i, v2.at(j))){
                v2.erase(v2.begin()+j);
                break;
            }
        }
    }
    if(v2.empty()) return true;
    return false;
}

bool ClusterCausalGraph :: is_in_same_cluster(int v_id1, int v_id2)
{
    if(variable_cluster_map.count(v_id1) == 0 || variable_cluster_map.count(v_id2) == 0)
        return false;
    else if( variable_cluster_map.at(v_id1) == variable_cluster_map.at(v_id2))
        return true;
    else
        return false;
}

void ClusterCausalGraph :: print_clusters(){
    for (auto cluster: clusters) {
        write_in_file("log.txt", print_variable_cluster(cluster.first));
    }

}

string ClusterCausalGraph :: print_cluster_causal_graph(){
    string s = "";
    for(auto itr: causal_graph){
        s +=  "<" + to_string(itr.first) + "> ";
        for(int c_id: itr.second){
            s += to_string(c_id) + ",";
        }
        s += "\n";
    }
    write_in_file("log.txt", s);
    return s;
}

tuple<int, int> ClusterCausalGraph :: count_agent_cluster_variable(){
    int c_n =0, n= 0;
    cout << " ";
    for (auto cluster: clusters){
        if(cluster.second.get_type() == agent){
            c_n ++;
//            cluster.second.print();
            n = n+ cluster.second.get_ground_variables().size();
        }
    }
    return tuple<int, int>(c_n, n);
}