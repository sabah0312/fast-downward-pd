

#include "../../search/plugins/plugin.h"
#include "../deorder_algorithm.h"
#include "eog.h"

using namespace std;

namespace plugin_eog {
class EOGDeorderFeature
    : public plugins::TypedFeature<DeorderAlgorithm, EOG> {
public:
    EOGDeorderFeature() : TypedFeature("eog") {
        document_title("Stepwise Deorder (EOG)");
        document_synopsis(
            "");
    }

    virtual shared_ptr<EOG> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        plugins::Options options_copy(opts);

        return plugins::make_shared_from_arg_tuples<EOG>();
    }
};

static plugins::FeaturePlugin<EOGDeorderFeature> _plugin;
}
