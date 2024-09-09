#include <iomanip>
#include <filesystem>
#include <vdr/device.h>
#include <vdr/plugin.h>
#include <vdr/tools.h>
#include "log.h"
#include "ffmpeghls.h"

const char *STREAM_DIR = "/tmp/vdr-live-tv-stream";

void killPid(int pid, int signal) {
    std::string procf("/proc/");
    procf.append(std::to_string(pid));

    struct stat sts{};
    if (!(stat(procf.c_str(), &sts) == -1 && errno == ENOENT)) {
        kill(-pid, signal);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        kill(pid, signal);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!(stat(procf.c_str(), &sts) == -1 && errno == ENOENT)) {
        // must not happen
        // printf("Prozess %d läuft noch\n", pid);
    }
}

cFFmpegHLS::cFFmpegHLS(bool isReplay, cString channel, cString recName, cString recFileName) {
    debug1("%s: Channel %s, RecName %s, RecFileName %s", __PRETTY_FUNCTION__, *channel, *recName, *recFileName);

    // delete directory if it exists
    if (!RemoveFileOrDir(STREAM_DIR, false)) {
        error("Unable to delete directory %s\n", STREAM_DIR);
    } else {
        debug1("Directory %s removed.", STREAM_DIR);
    }

    // create  the tmp directory
    std::filesystem::create_directory(STREAM_DIR);

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

    int pid = cmdLineProcess->get_id();

    printf("CmdLind Pid: %d\n", pid);

    cmdLineProcess->get_exit_status();

    killPid(pid, SIGKILL);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    killPid(pid, SIGTERM);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    delete cmdLineProcess;

    debug1("Transcoder command line: %s", *transcodeCmdLine);

    if (strlen(*transcodeCmdLine) == 0) {
        error("Transcode commandline is empty");
        return;
    }

    ffmpegProcess = new TinyProcessLib::Process(*transcodeCmdLine, STREAM_DIR, nullptr, nullptr, true);
    printf("ffmpeg pid %d\n", ffmpegProcess->get_id());

    int exitStatus;

    if (ffmpegProcess->try_get_exit_status(exitStatus)) {
        error("ffmpeg error_code: %d\n", exitStatus);
    }
}

cFFmpegHLS::~cFFmpegHLS() {
    debug1("%s", __PRETTY_FUNCTION__);

    if (ffmpegProcess!=nullptr) {
        auto t = ffmpegProcess;
        ffmpegProcess = nullptr;

        t->close_stdin();

        int pid = t->get_id();
        killPid(pid, SIGKILL);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        killPid(pid, SIGTERM);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        delete t;

        // ffmpegProcess->get_exit_status();

        debug1("ffmpegProcess killed");
    }

    // delete directory if it exists
    if (!RemoveFileOrDir(STREAM_DIR, false)) {
        error("Unable to delete directory %s\n", STREAM_DIR);
    } else {
        debug1("Directory %s removed.", STREAM_DIR);
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
