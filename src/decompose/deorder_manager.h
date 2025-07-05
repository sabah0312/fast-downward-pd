//
// Created by sabah on 6/6/25.
//

#ifndef CUSTOM_TRANSLATE_PY_DEORDER_MANAGER_H
#define CUSTOM_TRANSLATE_PY_DEORDER_MANAGER_H


#include <string>

class DeorderManager {
    std::string original_plan_filename;
    std::string domain_filepath;
    std::string problem_filepath;
    double start_time;
    bool is_input_sequential;
public:
    DeorderManager();
    void set_plan_file_path(std::string path);
    void set_domain_file_path(std::string path);
    void set_problem_file_path(std::string path);

    std::string get_plan_file_path();

    void remove_previous_files();

    std::string get_domain_file_path();

    std::string get_problem_file_path();

    void start_timer();

    double get_time();
};


#endif //CUSTOM_TRANSLATE_PY_DEORDER_MANAGER_H
