#include <iostream>
#include "CamRTSPServer.h"
#include "INIReader.h"
#include "log.h"

int LogLevel;
int main()
{
    INIReader configs("./configs/config.ini");
    if (configs.ParseError() < 0) {
        printf("read config failed.");
        exit(1);
    } else {
        std::string level = configs.Get("log", "level", "NOTICE");
        if (level == "NOTICE") {
            initLogger(NOTICE);
        } else if (level == "INFO") {
            initLogger(INFO);
        } else if (level == "ERROR") {
            initLogger(ERROR);
        }
    }

    try {
        CamRTSPServer server;
        server.addTranscoder(std::make_shared<TransCoder>());
        server.run();
    } catch (const std::invalid_argument &err) {
        LOG(ERROR, err.what());
    }

    LOG(INFO, "test.");
    return 0;
}
