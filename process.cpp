#include <mutex>
#include "process.h"
#include "log.h"

std::mutex closeMutex;

void cProcess::start(const std::vector<std::string> &command) {
    debug1("%s", __PRETTY_FUNCTION__);

    running = true;
    loopThread = std::thread(&cProcess::processLoop, this, command);
}

void cProcess::stop() {
    debug1("%s", __PRETTY_FUNCTION__);

    std::lock_guard<std::mutex> guard(closeMutex);

    running = false;

    if (loopThread.joinable()) {
        loopThread.join();
    }
}

void cProcess::writeToStdin(const char *buffer, long bufferSize) {
    debug9("%s", __PRETTY_FUNCTION__);

    std::lock_guard<std::mutex> guard(closeMutex);

    if (!running) {
        return;
    }

    child.write(buffer, bufferSize);
}

void cProcess::processLoop(const std::vector<std::string> &command) {
    debug1("%s", __PRETTY_FUNCTION__);

    if (!running) {
        logerror("%s Not running", __PRETTY_FUNCTION__);
        return;
    }

    const redi::pstreams::pmode mode = redi::pstreams::pstdout | redi::pstreams::pstderr | redi::pstreams::pstdin;

    if (command.size()==1) {
        child = redi::pstream(command.at(0), mode);
    } else {
        child = redi::pstream(command, mode);
    }

    if (!child.is_open()) {
        running = false;
        processError("Unable to start process...");
    }

    char buf[64*1024];
    std::streamsize n;
    bool finishStdin = false, finishStdout = false;

    while (running && (!finishStdin || !finishStdout)) {
        if (!finishStdin) {
            while ((n = child.out().readsome(buf, sizeof(buf))) > 0) {
                std::string inp = std::string(buf, n);
                readStdout(inp);
            }

            if (child.eof()) {
                finishStdin = true;
                if (!finishStdout) {
                    child.clear();
                }
            }
        }

        if (!finishStdout) {
            while ((n = child.err().readsome(buf, sizeof(buf))) > 0) {
                std::string inp = std::string(buf, n);
                readStderr(inp);
            }

            if (child.eof()) {
                finishStdout = true;
                if (!finishStdin) {
                    child.clear();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    redi::pstreambuf *pbuf = child.rdbuf();
    pbuf->peof();
    running = false;
}

// cSimpleProcess

cSimpleProcess::cSimpleProcess() {
    debug1("%s\n", __PRETTY_FUNCTION__);
}

void cSimpleProcess::process(const std::vector<std::string> &command) {
    cProcess process([this](auto &&PH1) { stdoutOutput.append(std::forward<decltype(PH1)>(PH1)); },
                     [this](auto &&PH1) { stderrOutput.append(std::forward<decltype(PH1)>(PH1)); },
                     [this](auto &&PH1) { errorOutput.append(std::forward<decltype(PH1)>(PH1)); } );

    process.start(command);

    while (process.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    process.stop();
}
