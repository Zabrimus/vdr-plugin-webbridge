#include <iomanip>
#include <vdr/device.h>
#include <vdr/plugin.h>
#include "log.h"
#include "ffmpeghls.h"

const char *STREAM_DIR = "/tmp/vdr-live-tv-stream";

cFFmpegHLS::cFFmpegHLS(bool isReplay, cString channel, cString recName) {
    cString cmdLineScript;
    cString transcodeCmdLine;

    std::vector<std::string> callStr;
    printf("Channel: %s\n", *channel);

    if (isReplay) {
        callStr.emplace_back(std::string(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N)) + "/stream_recording.sh");
        callStr.emplace_back(*recName);
    } else {
        callStr.emplace_back(std::string(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N)) + "/stream_live.sh");
        callStr.emplace_back(*channel);
    }

    // call the script and read the constructed commandline
    auto cmdLineProcess = new TinyProcessLib::Process(callStr, "",
                                                [&transcodeCmdLine](const char *bytes, size_t n) {
                                                  std::string msg = std::string(bytes, n);
                                                  transcodeCmdLine = transcodeCmdLine.Append(msg.c_str());
                                                },

                                                [](const char *bytes, size_t n) {
                                                  std::string msg = std::string(bytes, n);
                                                  error("Error: %s\n", msg.c_str());
                                                },

                                                true
    );

    cmdLineProcess->get_exit_status();

    debug1("Transcoder command line: %s", *transcodeCmdLine);
    printf("Transcoder command line: %s\n", *transcodeCmdLine);

    if (strlen(*transcodeCmdLine) == 0) {
        error("Transcode commandline is empty");
        return;
    }

    ffmpegProcess = new TinyProcessLib::Process(*transcodeCmdLine, STREAM_DIR, nullptr, nullptr, true);

    int exitStatus;

    if (ffmpegProcess->try_get_exit_status(exitStatus)) {
        printf("ffmpeg error_code: %d\n", exitStatus);
    }
}

cFFmpegHLS::~cFFmpegHLS() {
    if (ffmpegProcess!=nullptr) {
        ffmpegProcess->close_stdin();
        ffmpegProcess->kill(true);
        ffmpegProcess->get_exit_status();

        ffmpegProcess = nullptr;
    }

    system((std::string("rm -rf ") + STREAM_DIR + "/*.ts").c_str());
    system((std::string("rm -rf ") + STREAM_DIR + "/*.m3u8").c_str());
}

void cFFmpegHLS::Receive(const uchar *Data, int Length) {
    int exitStatus;

    if (ffmpegProcess==nullptr) {
        return;
    }

    if (ffmpegProcess->try_get_exit_status(exitStatus)) {
        // process stopped/finished/crashed
        printf("ffmpeg error_code: %d\n", exitStatus);
        return;
    }

    if (!ffmpegProcess->write((const char *) Data, Length)) {
        debug1("Cannot write %d", Length);
    }
}
