//
// Created by user on 23-6-28.
//

#pragma once

#include "ycsb/core/status_thread.h"
#include "ycsb/core/client_thread.h"
#include "ycsb/core/workload/core_workload.h"
#include "ycsb/core/common/ycsb_property.h"

namespace ycsb {
    class YCSBEngine {
    public:
        explicit YCSBEngine(const util::Properties &n) :factory(n) {
            ycsbProperties = utils::YCSBProperties::NewFromProperty(n);
            workload = std::make_shared<ycsb::core::workload::CoreWorkload>();
            workload->init(*ycsbProperties);
            measurements = std::make_shared<core::Measurements>();
            workload->setMeasurements(measurements);
            initClients();
            totalBenchmarkTime = static_cast<int>((double)ycsbProperties->getOperationCount() /
                    (ycsbProperties->getTargetTPSPerThread() * ycsbProperties->getThreadCount()));
        }

        void startTest() {
            LOG(INFO) << "Running test.";
            auto status = factory.newDBStatus();
            auto statusThread = std::make_unique<ycsb::core::StatusThread>(measurements, std::move(status));
            LOG(INFO) << "Run worker thread";
            for(auto &client :clients) {
                client->run();
            }
            LOG(INFO) << "Run status thread";
            statusThread->run();
            std::this_thread::sleep_for(std::chrono::seconds(totalBenchmarkTime));
            LOG(INFO) << "Finishing status thread";
            statusThread.reset();
            workload->requestStop();
            LOG(INFO) << "All worker exited";
        }

    protected:
        void initClients() {
            auto operationCount = ycsbProperties->getOperationCount();
            auto threadCount = std::min(ycsbProperties->getThreadCount(), (int)operationCount);
            auto threadOpCount = operationCount / threadCount;
            if (threadOpCount <= 0) {
                threadOpCount += 1;
            }
            auto tpsPerThread = ycsbProperties->getTargetTPSPerThread();
            for (int tid = 0; tid < threadCount; tid++) {   // create a set of clients
                auto db = factory.newDB();  // each client create a connection
                // TODO: optimize zmq connection
                auto t = std::make_unique<core::ClientThread>(std::move(db), workload, tid, (int)threadOpCount, tpsPerThread);
                clients.emplace_back(std::move(t));
            }
        }

    private:
        core::DBFactory factory;
        int totalBenchmarkTime = 0;
        std::unique_ptr<ycsb::utils::YCSBProperties> ycsbProperties;
        std::shared_ptr<ycsb::core::workload::CoreWorkload> workload;
        std::shared_ptr<core::Measurements> measurements;
        std::vector<std::unique_ptr<core::ClientThread>> clients;
    };
}
