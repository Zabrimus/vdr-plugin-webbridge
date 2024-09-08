#pragma once

#include <filesystem>
#include <process.hpp>
#include <vdr/tools.h>

extern const char *STREAM_DIR;

class cFFmpegHLS {
private:
    TinyProcessLib::Process *ffmpegProcess;

public:
    explicit cFFmpegHLS(bool isReplay, cString channel, cString recName, cString recFileName);
    ~cFFmpegHLS();

    void Receive(const uchar *Data, int Length);
};
