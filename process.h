#pragma once

#include <iostream>
#include <functional>
#include <thread>
#include "pstream.h"

class cProcess {
private:
    const std::function<void(std::string)> processError;
    const std::function<void(std::string)> readStdout;
    const std::function<void(std::string)> readStderr;

    redi::pstream child;
    std::thread loopThread;
    bool running;

private:
    void processLoop(const std::vector<std::string> &command);

public:
    explicit cProcess(const std::function<void(std::string)> &in,
                      const std::function<void(std::string)> &err,
                      const std::function<void(std::string)> &perr)
        : processError(perr), readStdout(in), readStderr(err), running(false) {
    };

    ~cProcess() {
        stop();
    }

    void start(const std::vector<std::string> &command);
    void stop();
    bool isRunning() const { return running; };

    void writeToStdin(const char *buffer, long bufferSize);
};

class cSimpleProcess {
private:
    std::string errorOutput;
    std::string stdoutOutput;
    std::string stderrOutput;

public:
    cSimpleProcess();
    ~cSimpleProcess() = default;

    void process(const std::vector<std::string> &command);

    std::string getErrorOutput() { return errorOutput; };
    std::string getStdoutOutput() { return stdoutOutput; };
    std::string getStderrOutput() { return stderrOutput; };
};
