#include <filesystem>
#include <vdr/plugin.h>
#include <vdr/tools.h>
#include <mutex>
#include "log.h"
#include "ffmpeghls.h"

const char *STREAM_DIR = "/tmp/vdr-live-tv-stream";

cFFmpegHLS::cFFmpegHLS(bool isReplay, cString channel, cString recName, cString recFileName) {
    debug1("%s: Channel %s, RecName %s, RecFileName %s", __PRETTY_FUNCTION__, *channel, *recName, *recFileName);

    // delete directory if it exists
    if (!RemoveFileOrDir(STREAM_DIR, false)) {
        logerror("Unable to delete directory %s\n", STREAM_DIR);
    } else {
        debug1("Directory %s removed.", STREAM_DIR);
    }

    // create  the tmp directory
    std::filesystem::create_directory(STREAM_DIR);

    std::string cmdLineScript;
    std::string transcodeCmdLine;

    std::vector<std::string> callStr;

    if (isReplay) {
        callStr.emplace_back(std::string(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N)) + "/stream_recording.sh");
        callStr.emplace_back(*recName);
        callStr.emplace_back(*recFileName);
    } else {
        callStr.emplace_back(std::string(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N)) + "/stream_live.sh");
        callStr.emplace_back(*channel);
    }

    transcodeCmdLine = readCommandLine(callStr);

    if (transcodeCmdLine.empty()) {
        logerror("Transcode commandline is empty");
        return;
    }

    std::vector cmd = {std::string("cd ") + STREAM_DIR + "&&" + transcodeCmdLine};
    ffmpegProcess = new cProcess([this](auto && PH1) { captureStdout(std::forward<decltype(PH1)>(PH1)); },
                                 [this](auto && PH1) { captureStderr(std::forward<decltype(PH1)>(PH1)); },
                                 [this](auto && PH1) { captureError(std::forward<decltype(PH1)>(PH1)); });

    ffmpegProcess->start(cmd);
}

cFFmpegHLS::~cFFmpegHLS() {
    debug1("%s", __PRETTY_FUNCTION__);

    if (ffmpegProcess != nullptr) {
        ffmpegProcess->stop();
        delete ffmpegProcess;
        ffmpegProcess = nullptr;
    }

    // delete directory if it exists
    if (!RemoveFileOrDir(STREAM_DIR, false)) {
        logerror("Unable to delete directory %s\n", STREAM_DIR);
    } else {
        debug1("Directory %s removed.", STREAM_DIR);
    }
}

void cFFmpegHLS::Receive(const uchar *Data, int Length) {
    debug4("%s", __PRETTY_FUNCTION__);

    if (ffmpegProcess != nullptr && ffmpegProcess->isRunning()) {
        ffmpegProcess->writeToStdin(reinterpret_cast<const char *>(Data), Length);
    } else {
        debug9("%s: Receive without an ffmpegProcess", __PRETTY_FUNCTION__);
    }
}

std::string cFFmpegHLS::readCommandLine(std::vector<std::string> callStr) {
    cSimpleProcess p;
    p.process(callStr);

    std::string error = p.getErrorOutput();
    if (!error.empty()) {
        logerror("%s\n", error.c_str());
        return "";
    }

    return p.getStdoutOutput();
}

void cFFmpegHLS::captureStdout(const std::string& out) {
    // TODO: was soll ich hiermit machen?
}

void cFFmpegHLS::captureStderr(const std::string& err) {
    // TODO: was soll ich hiermit machen?
}

void cFFmpegHLS::captureError(const std::string& error) {
    // TODO: was soll ich hiermit machen?
}
