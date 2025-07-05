#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <memory>
#include <string>
#include "../decompose/deorder_algorithm.h"

class SearchAlgorithm;

extern std::shared_ptr<SearchAlgorithm> parse_cmd_line(
    int argc, const char **argv, bool is_unit_cost);

extern std::shared_ptr<DeorderAlgorithm> parse_cmd_line_for_deorder(
        int argc, const char **argv);

extern std::string usage(const std::string &progname);

#endif
