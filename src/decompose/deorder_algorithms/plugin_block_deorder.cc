//
// Created by sabah on 6/7/25.
//
#include "../../search/plugins/plugin.h"
#include "../deorder_algorithm.h"
#include "block_deorder.h"

using namespace std;

namespace plugin_block_deorder {
    class BlockDeorderFeature
            : public plugins::TypedFeature<DeorderAlgorithm, BlockDeorder> {
    public:
        BlockDeorderFeature() : TypedFeature("block_deorder") {
            document_title("Block Deorder (BD)");
            document_synopsis(
                    "");


        }

        virtual shared_ptr<BlockDeorder> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {
            plugins::Options options_copy(opts);

            return plugins::make_shared_from_arg_tuples<BlockDeorder>();
        }
    };

    static plugins::FeaturePlugin<BlockDeorderFeature> _plugin;
}
