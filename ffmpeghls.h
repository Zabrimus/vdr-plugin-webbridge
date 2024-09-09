#pragma once

#include <filesystem>
#include <thread>
#include <vdr/tools.h>
#include "process.h"

extern const char *STREAM_DIR;

class cFFmpegHLS {
private:
    std::string commandLine;
    cProcess *ffmpegProcess;

private:
    std::string readCommandLine(std::vector<std::string> callStr);

    void captureStdout(const std::string& out);
    void captureStderr(const std::string& err);
    void captureError(const std::string& error);

public:
    explicit cFFmpegHLS(bool isReplay, cString channel, cString recName, cString recFileName);
    ~cFFmpegHLS();

    void Receive(const uchar *Data, int Length);
};
