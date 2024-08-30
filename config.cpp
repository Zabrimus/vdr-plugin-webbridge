/*
 * config.cpp: WebBridge plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "config.h"
#include "log.h"

cWebBridgeConfig WebBridgeConfig;

cWebBridgeConfig::cWebBridgeConfig() : traceModeM(eTraceModeNormal), port(0) {
}

void cWebBridgeConfig::SetConfigDirectory(const char *directory) {
    debug1("%s (%s)", __PRETTY_FUNCTION__, directory);

    if (!realpath(directory, configDirectory)) {
        error("Cannot canonicalize configuration directory");
    }
}