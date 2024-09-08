#include <iomanip>
#include <filesystem>
#include <vdr/device.h>
#include <vdr/plugin.h>
#include "log.h"
#include "ffmpeghls.h"

const char *STREAM_DIR = "/tmp/vdr-live-tv-stream";

cFFmpegHLS::cFFmpegHLS(bool isReplay, cString channel, cString recName, cString recFileName) {
    debug1("%s: Channel %s, RecName %s, RecFileName %s", __PRETTY_FUNCTION__, *channel, *recName, *recFileName);

    // create at first the tmp directory if it not exists
    if (!std::filesystem::is_directory(STREAM_DIR) || !std::filesystem::exists(STREAM_DIR)) {
        std::filesystem::create_directory(STREAM_DIR);
    }

    for(auto& dir_element : std::filesystem::directory_iterator(STREAM_DIR)) {
        std::filesystem::remove_all(dir_element.path());
    }

    cString cmdLineScript;
    cString transcodeCmdLine;

    std::vector<std::string> callStr;

    // printf("Channel: %s\n", *channel);
    // printf("RecName: %s\n", *recName);
    // printf("RecFileName: %s\n", *recFileName);
    // printf("IsReplay: %s\n", isReplay ? "Ja" : "Nein");

    if (isReplay) {
        callStr.emplace_back(std::string(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N)) + "/stream_recording.sh");
        callStr.emplace_back(*recName);
        callStr.emplace_back(*recFileName);
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

    if (strlen(*transcodeCmdLine) == 0) {
        error("Transcode commandline is empty");
        return;
    }

    ffmpegProcess = new TinyProcessLib::Process(*transcodeCmdLine, STREAM_DIR, nullptr, nullptr, true);

    int exitStatus;

    if (ffmpegProcess->try_get_exit_status(exitStatus)) {
        error("ffmpeg error_code: %d\n", exitStatus);
    }
}

cFFmpegHLS::~cFFmpegHLS() {
    debug1("%s", __PRETTY_FUNCTION__);

    if (ffmpegProcess!=nullptr) {
        ffmpegProcess->close_stdin();
        ffmpegProcess->kill(true);
        ffmpegProcess->get_exit_status();

        ffmpegProcess = nullptr;
    }

    for(auto& dir_element : std::filesystem::directory_iterator(STREAM_DIR)) {
        std::filesystem::remove_all(dir_element.path());
    }
}

void cFFmpegHLS::Receive(const uchar *Data, int Length) {
    debug4("%s", __PRETTY_FUNCTION__);

    int exitStatus;

    if (ffmpegProcess == nullptr) {
        return;
    }

    if (ffmpegProcess->try_get_exit_status(exitStatus)) {
        // process stopped/finished/crashed
        // printf("ffmpeg error_code: %d\n", exitStatus);
        return;
    }

    ssize_t writtenBytes = ffmpegProcess->writeBytes((const char *) Data, Length);
    if (writtenBytes == -1) {
        debug1("Cannot write %d", Length);
    }
}
