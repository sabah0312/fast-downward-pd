//
// Created by sabah on 6/6/25.
//

#include <fstream>
#include "deorder_manager.h"
#include "../search/task_proxy.h"
#include "myutils.h"

using namespace std;

DeorderManager:: DeorderManager():is_input_sequential(true){

}

void DeorderManager :: start_timer(){
    start_time = std::clock();
}

double DeorderManager :: get_time (){
    return  (std::clock() - start_time) / (double)CLOCKS_PER_SEC;
}

void DeorderManager:: set_plan_file_path(string path){
    original_plan_filename = path;
}

string DeorderManager:: get_plan_file_path(){
    return original_plan_filename;
}

string DeorderManager:: get_domain_file_path(){
    return domain_filepath;
}

string DeorderManager:: get_problem_file_path(){
    return problem_filepath;
}

void DeorderManager:: set_domain_file_path(string path){
    domain_filepath = path;
}

void DeorderManager:: set_problem_file_path(string path){
    problem_filepath = path;
}


void DeorderManager :: remove_previous_files(){
    remove("result.csv");
    remove("pop.dot");
    remove("pop.txt");
    remove("fibs.txt");
    remove("fibs.dot");
    remove("bd.txt");
    remove("bd.dot");
    remove("cibs.txt");
    remove("cibs.dot");
    remove("step.txt");
    remove("time.txt");
    remove("total_order_plan.txt");
    remove("log.txt");
    remove("problem_details.csv");
    remove("bd_out.csv");
    remove("eog_out.csv");
    remove("fibs_out.csv");
    remove("cibs_out.csv");

}





