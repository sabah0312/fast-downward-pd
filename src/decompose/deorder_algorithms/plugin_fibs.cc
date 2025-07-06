//
// Created by sabah on 6/8/25.
//

#include "../../search/plugins/plugin.h"
#include "../deorder_algorithm.h"
#include "../block_replace.h"
#include "../plan_reduction.h"
#include "fibs.h"
#include "../../search/heuristics/domain_transition_graph.h"

using namespace std;

namespace plugin_FIBS {
    class SubstitutionDeorderFeature
            : public plugins::TypedFeature<DeorderAlgorithm, FIBS> {
    public:
        SubstitutionDeorderFeature() : TypedFeature("fibs") {
            document_title("Flexibility Improvement via Block-Substitution (FIBS)");
            document_synopsis(
                    "");
            add_option<plan_reduction_type>(
                    "plan_reduction",
                    "Plan reduction algorithm",
                    "NoReduction");
//            add_option<string>("planner", "Path for planner to generate plans for subtasks", "./fast-downward-pd.py --alias seq-sat-lama-2011");
            add_option<bool>("only_replace_block", "Substitute only compound block", "false");
            add_option<bool>("compromise_flex", "Compromise flex for plan quality", "false");
            add_option<bool>("concurrency", "Facilitate and improve concurrency", "false");
        }

        virtual shared_ptr<FIBS> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {
            plugins::Options options_copy(opts);

            return plugins::make_shared_from_arg_tuples<FIBS>(opts.get<bool>("only_replace_block"),
                    opts.get<bool>("compromise_flex"),  opts.get<bool>("concurrency"), opts.get<plan_reduction_type>("plan_reduction"));
        }
    };

    static plugins::FeaturePlugin<SubstitutionDeorderFeature> _plugin;
}

