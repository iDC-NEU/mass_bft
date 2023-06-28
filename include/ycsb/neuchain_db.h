//
// Created by peng on 11/6/22.
//

#pragma once

#include "ycsb/core/db.h"
#include "ycsb/core/status.h"
#include "proto/block.h"
#include "common/zeromq.h"
#include "common/bccsp.h"

namespace brpc {
    class Channel;
}

namespace ycsb::client {
    namespace proto {
        class UserService_Stub;
    }

    struct UserRequestLikeStruct {
        std::string_view _ccNameSV;
        std::string_view _funcNameSV;
        std::string_view _argsSV;
    };

    struct EnvelopLikeStruct {
        std::string_view _payloadSV;
        ::proto::SignatureString _signature;
    };

    class NeuChainDB: public ycsb::core::DB {
    public:
        struct InvokeRequestType {
            constexpr static const auto YCSB = "y";
            constexpr static const auto UPDATE = "u";
            constexpr static const auto INSERT = "i";
            constexpr static const auto READ = "r";
            constexpr static const auto DELETE = "d";
            constexpr static const auto SCAN = "s";
            constexpr static const auto READ_MODIFY_WRITE = "rmw";
        };

        NeuChainDB(util::NodeConfigPtr server, int port, std::shared_ptr<const util::Key> priKey);

        core::Status read(const std::string& table, const std::string& key, const std::vector<std::string>& fields) override;

        core::Status scan(const std::string& table, const std::string& startKey, uint64_t recordCount, const std::vector<std::string>& fields) override;

        core::Status update(const std::string& table, const std::string& key, const utils::ByteIteratorMap& values) override;

        core::Status readModifyWrite(const std::string& table, const std::string& key,
                                     const std::vector<std::string>& readFields, const utils::ByteIteratorMap& writeValues) override;

        core::Status insert(const std::string& table, const std::string& key, const utils::ByteIteratorMap& values) override;

        core::Status remove(const std::string& table, const std::string& key) override;

    protected:
        core::Status sendInvokeRequest(const std::string& funcName, const std::string& args);

    private:
        std::unique_ptr<util::ZMQInstance> _invokeClient;
        std::shared_ptr<const util::Key> _priKey;
        util::NodeConfigPtr _serverConfig;
    };

    class NeuChainStatus: public ycsb::core::DBStatus {
    public:
        NeuChainStatus(util::NodeConfigPtr server, int port, std::shared_ptr<const util::Key> priKey);

        ~NeuChainStatus() override;

        bool connect();

        std::unique_ptr<::proto::Block> getBlock(int blockNumber) override;

    private:
        std::unique_ptr<brpc::Channel> _channel;
        std::unique_ptr<proto::UserService_Stub> _stub;
        std::shared_ptr<const util::Key> _priKey;
        util::NodeConfigPtr _serverConfig;
    };
}
