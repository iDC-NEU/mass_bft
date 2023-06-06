//
// Created by user on 23-5-16.
//

#include "ycsb/neuchain_db.h"
#include "proto/user_request.h"
#include "ycsb/core/workload/core_workload.h"
#include "common/property.h"
#include "ycsb/core/request_sender.h"

ycsb::core::Status
ycsb::client::NeuChainDB::db_connection() {
    auto property = utils::YCSBProperties::NewFromProperty();
    rpcClient.emplace_back(property->getLocalBlockServerIP(),
                             util::ZMQInstance::NewClient<zmq::socket_type::pub>(property->getLocalBlockServerIP(), 7003));
    if(property->sendToAllClientProxy()) {
        // load average
        for(const auto& ip: property->getBlockServerIPs()) {
            invokeClient.emplace_back(ip, util::ZMQInstance::NewClient<zmq::socket_type::pub>(ip, 5001));
            // queryClient.emplace_back(ip, std::make_unique<ZMQClient>(ip, "7003"));
        }
        sendInvokeRequest = [this](const proto::UserRequest &request) {
            size_t clientCount = invokeClient.size();
            size_t id = trCount % clientCount;
            std::string&& payloadRaw = Utils::getTransactionPayloadRaw(trCount++, request);
            auto&& invokeRequest = Utils::getUserInvokeRequest(payloadRaw, invokeClient[id].first);
            addPendingTransactionHandle(invokeRequest.digest());
            invokeClient[id].second->send(invokeRequest.SerializeAsString());
        };
    } else {
        // single user
        const auto& ip = property->getLocalBlockServerIP();
        invokeClient.emplace_back(ip, util::ZMQInstance::NewClient<zmq::socket_type::pub>(ip, 5001));
        sendInvokeRequest = [this](const proto::UserRequest &request) {
            std::string&& payloadRaw = Utils::getTransactionPayloadRaw(trCount++, request);
            auto&& invokeRequest = Utils::getUserInvokeRequest(payloadRaw, invokeClient[0].first);
            addPendingTransactionHandle(invokeRequest.digest());
            invokeClient[0].second->send(invokeRequest.SerializeAsString());
        };
    }
}

ycsb::core::Status
ycsb::client::NeuChainDB::read(const std::string& table, const std::string& key, const std::vector<std::string>& fields) {
    proto::UserRequest request;
    request.setFuncName("read");
    request.setTableName(const_cast<std::string &&>(table));
    // TODO:

    request.setArgs(const_cast<std::string &&>(key));

    sendInvokeRequest(request);
    return core::STATUS_OK;
}

ycsb::core::Status
ycsb::client::NeuChainDB::scan(const std::string& table, const std::string& startkey, uint64_t recordcount, const std::vector<std::string>& fields) {
    LOG(WARNING) << "neuChain_db does not support Scan op. ";
    return core::ERROR;
}

ycsb::core::Status
ycsb::client::NeuChainDB::update(const std::string& table, const std::string& key,
                                 const utils::ByteIteratorMap& values) {

    return core::STATUS_OK;
}

ycsb::core::Status
ycsb::client::NeuChainDB::insert(const std::string& table, const std::string& key,
                                 const utils::ByteIteratorMap& values) {
    update(table, key, values);
    return core::STATUS_OK;
}

ycsb::core::Status
ycsb::client::NeuChainDB::remove(const std::string& table, const std::string& key) {
    proto::UserRequest request;
    request.setFuncName("del");
    request.setTableName(const_cast<std::string &&>(table));
    request.setArgs(const_cast<std::string &&>(key));
    sendInvokeRequest(request);
    return core::STATUS_OK;
}

std::unique_ptr<proto::Block> ycsb::client::NeuChainDB::getBlock(int blockNumber) {
    rpcClient[0].second->send("block_query_" + std::to_string(blockNumber));
    auto reply = rpcClient[0].second->receive();
    if (reply == std::nullopt) {
        return nullptr;
    }
    auto block = std::make_unique<proto::Block>();
    auto ret = block->deserializeFromString(reply->to_string());
    if (!ret.valid) {
        return nullptr;
    }
    return block;
}

ycsb::core::Status ycsb::client::NeuChainDB::readModifyWrite(const std::string &table, const std::string &key,
                                                             const std::vector<std::string> &readFields,
                                                             const ycsb::utils::ByteIteratorMap &writeValues) {

    return core::STATUS_OK;
}
