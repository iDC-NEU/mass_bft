//
// Created by user on 23-8-3.
//

#pragma once

#include "common/parallel_merkle_tree.h"
#include "proto/block.h"

namespace util {
    class ProofGenerator {
    public:
        explicit ProofGenerator(const proto::Block::Body& body, int wpSize = 0) {
            if (wpSize >= 2) {
                wp = std::make_unique<util::thread_pool_light>(
                        std::min((int)std::thread::hardware_concurrency(), wpSize), "pr_gn_wk");
            }
            if (!body.serializeForProofGen(posList, serializedBody)) {
                CHECK(false) << "Serialize proof failed!";
            }
        }

        explicit ProofGenerator(const proto::Block::ExecuteResult& executeResult, int wpSize = 0) {
            if (wpSize >= 2) {
                wp = std::make_unique<util::thread_pool_light>(
                        std::min((int)std::thread::hardware_concurrency(), wpSize), "pr_gn_wk");
            }
            if (!executeResult.serializeForProofGen(posList, serializedBody)) {
                CHECK(false) << "Serialize proof failed!";
            }
        }

        [[nodiscard]] std::unique_ptr<pmt::MerkleTree> generateMerkleTree(pmt::ModeType nodeType = pmt::ModeType::ModeProofGenAndTreeBuild) const {
            if (posList.empty()) {
                return nullptr;
            }
            pmt::Config pmtConfig;
            pmtConfig.Mode = nodeType;
            if (posList[0] > 1024) {
                pmtConfig.LeafGenParallel = true;
            }
            if (posList.size() > 1024) {
                pmtConfig.RunInParallel = true;
            }
            std::string_view bodySV = serializedBody;
            std::vector<std::unique_ptr<pmt::DataBlock>> blocks;
            for (int i=0; i<(int)posList.size(); i++) {
                if (i == 0) {
                    blocks.emplace_back(new UserRequestDataBlock(bodySV.substr(0, posList[i])));
                    continue;
                }
                blocks.emplace_back(new UserRequestDataBlock(bodySV.substr(posList[i-1], posList[i])));
            }
            return pmt::MerkleTree::New(pmtConfig, blocks, wp.get());
        }

    private:
        std::vector<int> posList;
        std::string serializedBody;
        std::unique_ptr<util::thread_pool_light> wp;

    private:
        class UserRequestDataBlock: public pmt::DataBlock {
        public:
            explicit UserRequestDataBlock(std::string_view userRequest) : _userRequest(userRequest) { }

            [[nodiscard]] pmt::ByteString Serialize() const override {
                return _userRequest;
            }

        private:
            std::string_view _userRequest;
        };
    };
}