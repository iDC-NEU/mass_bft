//
// Created by peng on 11/6/22.
//

#include "ycsb/core/client.h"
#include "ycsb/core/workload/core_workload.h"
#include "ycsb/core/status_thread.h"
#include "common/thread_pool_light.h"
#include "common/property.h"
#include <yaml-cpp/yaml.h>

int main(int argc, char *argv[]) {
    // auto n = util::Properties::GetProperties()->getYCSBProperties();
    YAML::Node n;

    //get number of threads, target and db
    auto threadCount = n[ycsb::core::Client::THREAD_COUNT_PROPERTY].as<int>(1);
    auto dbName = "site.ycsb.BasicDB";
    auto target = n[ycsb::core::Client::TARGET_PROPERTY].as<int>(0);
    auto label = n[ycsb::core::Client::LABEL_PROPERTY].as<std::string>("");
    //compute the target throughput
    double targetPerThreadPerms = -1;
    if (target > 0) {
        double targetPerThread = ((double) target) / ((double) threadCount);
        targetPerThreadPerms = targetPerThread / 1000.0;
    }

    ycsb::core::workload::CoreWorkload workload;
    workload.init(n);

    LOG(INFO) << "Starting test.";
    auto completeLatch = util::NewSema();
    auto clients = ycsb::core::Client::initDB(dbName, n, threadCount, targetPerThreadPerms, &workload, completeLatch);

    // if measurement == time series, true
    bool standardStatus = false;
    if (n[ycsb::core::Measurements::MEASUREMENT_TYPE_PROPERTY].as<std::string>("") == "timeseries") {
        standardStatus = true;
    }
    int statusIntervalSeconds = n["status.interval"].as<int>(10);
    ycsb::core::StatusThread statusThread(completeLatch, std::move(clients), label, standardStatus, statusIntervalSeconds);
    statusThread.run();
    return 0;
}