//
// Created by peng on 11/6/22.
//

#include "ycsb/engine.h"

int main(int, char *[]) {
    util::OpenSSLSHA256::initCrypto();
    util::OpenSSLED25519::initCrypto();
    util::Properties::LoadProperties("peer.yaml");
    auto* p = util::Properties::GetProperties();
    ycsb::YCSBEngine engine(*p);
    engine.startTest();
    return 0;
}