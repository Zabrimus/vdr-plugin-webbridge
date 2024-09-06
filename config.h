/*
 * config.h: WebBridge plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <vdr/menuitems.h>

#include <utility>

class cWebBridgeConfig {
private:
    unsigned int traceModeM;
    unsigned int port;
    cString host = "";
    char configDirectory[PATH_MAX];

public:
    enum eTraceMode {
        eTraceModeNormal = 0x0000,
        eTraceModeDebug1 = 0x0001,
        eTraceModeDebug2 = 0x0002,
        eTraceModeDebug3 = 0x0004,
        eTraceModeDebug4 = 0x0008,
        eTraceModeDebug5 = 0x0010,
        eTraceModeDebug6 = 0x0020,
        eTraceModeDebug7 = 0x0040,
        eTraceModeDebug8 = 0x0080,
        eTraceModeDebug9 = 0x0100,
        eTraceModeMask = 0xFFFF
    };

    cWebBridgeConfig();

    unsigned int GetTraceMode() const { return traceModeM; };
    bool IsTraceMode(eTraceMode modeP) const { return (traceModeM & modeP); };
    void SetTraceMode(unsigned int modeP) { traceModeM = (modeP & eTraceModeMask); };
    void SetConfigDirectory(const char *directory);

    void SetWebsocketPort(unsigned int portP) { port = portP; };
    unsigned int GetWebsocketPort() const { return port; };
    void SetWebsocketHost(cString hostP) { host = std::move(hostP); };
    cString GetWebsocketHost() const { return host; };

    const char *GetConfigDirectory() const { return configDirectory; };
};

extern cWebBridgeConfig WebBridgeConfig;
