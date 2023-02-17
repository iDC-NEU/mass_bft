//
// Created by peng on 2/16/23.
//

#include "peer/replicator/block_receiver.h"
#include "tests/block_fragment_generator_utils.h"

#include "gtest/gtest.h"
#include "glog/logging.h"

class BlockReceiverTest : public ::testing::Test {
public:
    using ConfigPtr = peer::SingleRegionBlockReceiver::ConfigPtr;
    using Config = peer::SingleRegionBlockReceiver::Config;

    BlockReceiverTest() {
        dataShardCnt = 4;
        parityShardCnt = 8;
        for(int i=0; i<4; i++) {
            // 4 regions, each region 4+4 shards
            bfgUtils.addCFG(dataShardCnt, parityShardCnt, 1, 2);
        }
        // the sender cfg
        senderPosition = bfgUtils.addCFG(dataShardCnt, parityShardCnt, 1, 2);
        bfgUtils.startBFG();
    }
    static std::vector<ConfigPtr> GenerateNodesConfig(int groupId, int count) {
        std::vector<ConfigPtr> nodesConfig;
        for (int i=0; i<count; i++) {
            auto cfg = std::make_shared<Config>();
            auto nodeCfg = std::make_shared<util::NodeConfig>();
            nodeCfg->groupId = groupId;
            nodeCfg->nodeId = i;
            nodeCfg->ski = std::to_string(i);
            cfg->nodeConfig = std::move(nodeCfg);
            cfg->addr() = "127.0.0.1";
            cfg->port = 51200+i;
            nodesConfig.push_back(std::move(cfg));
        }
        return nodesConfig;
    }

protected:
    void SetUp() override {
    };

    void TearDown() override {
    };

protected:
    tests::BFGUtils bfgUtils;
    int dataShardCnt;
    int parityShardCnt;
    int senderPosition;
};

TEST_F(BlockReceiverTest, IntrgrateTest) {
    // region 0
    int regionId = 0;
    int nodesPerRegion = 4;
    int blockNumber = 10;
    // prepare all fragments
    std::vector<std::unique_ptr<util::ZMQInstance>> servers(nodesPerRegion);
    for(int i=0; i<(int)servers.size(); i++) {
        auto sender = util::ZMQInstance::NewServer<zmq::socket_type::pub>(51200+i);
        ASSERT_TRUE(sender != nullptr) << "Create instance failed";
        servers[i] = std::move(sender);
    }
    int shardPerNode = (parityShardCnt+senderPosition)/nodesPerRegion;    // must be divisible
    // spin up client
    auto nodesCfg = GenerateNodesConfig(regionId, nodesPerRegion);
    auto regionZeroReceiver = peer::SingleRegionBlockReceiver::NewSingleRegionBlockReceiver(bfgUtils.bfg, bfgUtils.cfgList[regionId], nodesCfg);
    ASSERT_TRUE(regionZeroReceiver != nullptr) << "Create instance failed";
    regionZeroReceiver->activeStart(blockNumber);

    // Give the subscribers a chance to connect, so they don't lose any messages
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    auto senderContext = bfgUtils.getContext(senderPosition);
    std::vector<std::string> serializedFragment(nodesPerRegion);
    for(int i=0; i<(int)servers.size(); i++) {
        serializedFragment[i] = bfgUtils.generateMockFragment(senderContext.get(), blockNumber, i*shardPerNode, (i+1)*shardPerNode);
    }
    // send fragment to corresponding peer
    for(int i=0; i<(int)servers.size(); i++) {
        servers[i]->send(std::move(serializedFragment[i]));
    }

    // check the received fragment
    auto ret = regionZeroReceiver->activeGet();
    ASSERT_TRUE(ret != nullptr) << "Can not get block fragments!";
    if (*ret != bfgUtils.message) {
        LOG(INFO) << ret->substr(0, 100);
        LOG(INFO) << bfgUtils.message.substr(0, 100);
    }
    ASSERT_TRUE(*ret == bfgUtils.message) << "Message mismatch!";
}

// test if there is a resource leak
TEST_F(BlockReceiverTest, ContinueousSending) {
    // region 0
    int regionId = 0;
    int nodesPerRegion = 4;
    int startWith = 10;
    int endWith = 1000;
    // prepare all fragments
    std::vector<std::unique_ptr<util::ZMQInstance>> servers(nodesPerRegion);
    for(int i=0; i<(int)servers.size(); i++) {
        auto sender = util::ZMQInstance::NewServer<zmq::socket_type::pub>(51200+i);
        ASSERT_TRUE(sender != nullptr) << "Create instance failed";
        servers[i] = std::move(sender);
    }
    int shardPerNode = (parityShardCnt+senderPosition)/nodesPerRegion;    // must be divisible
    // spin up client
    auto nodesCfg = GenerateNodesConfig(regionId, nodesPerRegion);
    auto regionZeroReceiver = peer::SingleRegionBlockReceiver::NewSingleRegionBlockReceiver(bfgUtils.bfg, bfgUtils.cfgList[regionId], nodesCfg);
    ASSERT_TRUE(regionZeroReceiver != nullptr) << "Create instance failed";
    regionZeroReceiver->passiveStart(startWith);

    // Give the subscribers a chance to connect, so they don't lose any messages
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    for (int blockNumber=startWith; blockNumber<endWith; blockNumber++) {
        LOG(INFO) << "Block number " << blockNumber << " start sending.";
        tests::BFGUtils::FillDummy(bfgUtils.message, 1024*1024*2);
        auto senderContext = bfgUtils.getContext(senderPosition);
        std::vector<std::string> serializedFragment(nodesPerRegion);
        for(int i=0; i<(int)servers.size(); i++) {
            serializedFragment[i] = bfgUtils.generateMockFragment(senderContext.get(), blockNumber, i*shardPerNode, (i+1)*shardPerNode);
        }
        // send fragment to corresponding peer
        for(int i=0; i<(int)servers.size(); i++) {
            servers[i]->send(std::move(serializedFragment[i]));
        }

        // check the received fragment
        auto ret = regionZeroReceiver->passiveGet(blockNumber);
        ASSERT_TRUE(ret != nullptr) << "Can not get block fragments!";
        if (*ret != bfgUtils.message) {
            LOG(INFO) << ret->substr(0, 100);
            LOG(INFO) << bfgUtils.message.substr(0, 100);
        }
        ASSERT_TRUE(*ret == bfgUtils.message) << "Message mismatch!";
    }
}