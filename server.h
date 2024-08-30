/*
 * server.h: WebBridge plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <vdr/thread.h>
#include <App.h>
#include <WebSocket.h>

struct PerSocketData {
    /* Define your user data */
    int something;
};

class cWebBridgeServer : public cThread {
private:
    int port;

    uWS::App *globalApp;
    uWS::WebSocket<false, true, PerSocketData> *svdrpSocket;
    uWS::Loop *globalAppLoop;
    us_listen_socket_t *listenSocket;

protected:
    void Action() override;
    ///< A derived cThread class must implement the code it wants to
    ///< execute as a separate thread in this function. If this is
    ///< a loop, it must check Running() repeatedly to see whether
    ///< it's time to stop.

public:
    explicit cWebBridgeServer(int portP, const char *Description = nullptr, bool LowPriority = false);
    ///< Creates a new thread.
    ///< If Description is present, a log file entry will be made when
    ///< the thread starts and stops (see SetDescription()).
    ///< The Start() function must be called to actually start the thread.
    ///< LowPriority can be set to true to make this thread run at a lower
    ///< priority.

    ~cWebBridgeServer() override;

    void Cancel(int WaitSeconds = 0);
    ///< Cancels the thread by first setting 'running' to false, so that
    ///< the Action() loop can finish in an orderly fashion and then waiting
    ///< up to WaitSeconds seconds for the thread to actually end. If the
    ///< thread doesn't end by itself, it is killed.
    ///< If WaitSeconds is -1, only 'running' is set to false and Cancel()
    ///< returns immediately, without killing the thread.

    void sendError(uWS::WebSocket<false, true, PerSocketData> *ws, const uWS::OpCode &opCode, int ret) const;
};

extern cWebBridgeServer *WebBridgeServer;
