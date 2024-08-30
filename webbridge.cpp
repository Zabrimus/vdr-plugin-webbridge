/*
 * webbridge.cpp: WebBridge plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/plugin.h>
#include <getopt.h>
#include "log.h"
#include "webbridge.h"
#include "server.h"
#include "webdevice.h"
#include "fpng.h"

static const char *VERSION = "0.1.0";
static const char *DESCRIPTION = "Bridge websockets and vdr: svdrp, osd, video (and for future use ...)";
static const char *MAINMENUENTRY = "Webbridge";

cPluginWebbridge::cPluginWebbridge() {
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginWebbridge::~cPluginWebbridge() {
    // Clean up after yourself!
}

const char *cPluginWebbridge::Description() {
    return DESCRIPTION;
}

const char *cPluginWebbridge::Version() {
    return VERSION;
}

const char *cPluginWebbridge::MainMenuEntry() {
    return MAINMENUENTRY;
}

const char *cPluginWebbridge::CommandLineHelp() {
    debug1("%s", __PRETTY_FUNCTION__);

    // Return a string that describes all known command line options.
    return "  -p <num>,  --port=<number>           port of the internal websocket\n"
           "  -t <mode>, --trace=<mode>            set the tracing mode\n";
}

bool cPluginWebbridge::ProcessArgs(int argc, char *argv[]) {
    debug1("%s", __PRETTY_FUNCTION__);

    // Implement command line argument processing here if applicable.
    static const struct option long_options[] = {
        {"port", required_argument, nullptr, 'p'},
        {"trace", required_argument, nullptr, 't'},
        {nullptr, no_argument, nullptr, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "p:t:", long_options, nullptr))!=-1) {
        switch (c) {
        case 't':
            WebBridgeConfig.SetTraceMode(strtol(optarg, nullptr, 0));
            break;

        case 'p':
            WebBridgeConfig.SetWebsocketPort(strtol(optarg, nullptr, 0));
            break;

        default:
            return false;
        }
    }

    // set default value
    if (WebBridgeConfig.GetWebsocketPort() == 0) {
        WebBridgeConfig.SetWebsocketPort(3000);
    }

    return true;
}

bool cPluginWebbridge::Initialize() {
    // Initialize any background activities the plugin shall perform.
    new cWebBridgeServer(WebBridgeConfig.GetWebsocketPort());

    new cWebDevice();
    fpng::fpng_init();

    return true;
}

bool cPluginWebbridge::Start() {
    // Start any background activities the plugin shall perform.
    WebBridgeServer->Start();

    return true;
}

void cPluginWebbridge::Stop() {
    // Stop any background activities the plugin is performing.
    WebBridgeServer->Cancel();
}

void cPluginWebbridge::Housekeeping() {
    // Perform any cleanup or other regular tasks.
}

void cPluginWebbridge::MainThreadHook() {
    // Perform actions in the context of the main program thread.
    // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginWebbridge::Active() {
    // Return a message string if shutdown should be postponed
    return nullptr;
}

time_t cPluginWebbridge::WakeupTime() {
    // Return custom wakeup time for shutdown script
    return 0;
}

cOsdObject *cPluginWebbridge::MainMenuAction() {
    // Perform the action when selected from the main VDR menu.
    return nullptr;
}

cMenuSetupPage *cPluginWebbridge::SetupMenu() {
    // Return a setup menu in case the plugin supports one.
    return nullptr;
}

bool cPluginWebbridge::SetupParse(const char *Name, const char *Value) {
    // Parse your own setup parameters and store their values.
    return false;
}

bool cPluginWebbridge::Service(const char *Id, void *Data) {
    // Handle custom service requests from other plugins
    return false;
}

const char **cPluginWebbridge::SVDRPHelpPages() {
    // Return help text for SVDRP commands this plugin implements
    return nullptr;
}

cString cPluginWebbridge::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) {
    // Process SVDRP commands this plugin implements
    return nullptr;
}

VDRPLUGINCREATOR(cPluginWebbridge); // Don't touch this!
