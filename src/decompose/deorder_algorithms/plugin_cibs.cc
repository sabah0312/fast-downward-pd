//
// Created by sabah on 7/5/25.
//

#include "../../search/plugins/plugin.h"
#include "../deorder_algorithm.h"
#include "../block_replace.h"
#include "cibs.h"

using namespace std;
using namespace block_replace;

namespace plugin_CIBS {
    class SubstitutionDeorderFeature
            : public plugins::TypedFeature<DeorderAlgorithm, CIBS> {
    public:
        SubstitutionDeorderFeature() : TypedFeature("cibs") {
            document_title("Concurrency Improvement via Block-Substitution (FIBS)");
            document_synopsis(
                    "");
            add_option<plan_reduction_type>(
                    "plan_reduction",
                    "Plan reduction algorithm",
                    "NoReduction");
            add_option<bool>("only_replace_block", "Substitute only compound block", "false");
            add_option<bool>("compromise_flex", "Compromise flex for plan quality", "false");
        }

        virtual shared_ptr<CIBS> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {
            plugins::Options options_copy(opts);

            return plugins::make_shared_from_arg_tuples<CIBS>(opts.get<bool>("only_replace_block"),
                                                              opts.get<bool>("compromise_flex"),
                                                                      opts.get<plan_reduction_type>("plan_reduction"));
        }
    };

    static plugins::FeaturePlugin<SubstitutionDeorderFeature> _plugin;
}


