//
// Created by sabah on 7/7/25.
//


#include "../../search/plugins/plugin.h"
#include "../deorder_algorithm.h"
#include "../plan_decompose.h"

using namespace std;

namespace plugin_decompose {
    class DecomposeFeature
            : public plugins::TypedFeature<DeorderAlgorithm, PlanDecompose> {
    public:
        DecomposeFeature() : TypedFeature("decompose") {
            document_title("");
            document_synopsis(
                    "");
        }

        virtual shared_ptr<PlanDecompose> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {
            plugins::Options options_copy(opts);

            return plugins::make_shared_from_arg_tuples<PlanDecompose>();
        }
    };

    static plugins::FeaturePlugin<DecomposeFeature> _plugin;
}

