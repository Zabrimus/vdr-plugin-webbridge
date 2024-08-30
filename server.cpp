/*
 * server.cpp: WebBridge plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <cstdio>
#include <fstream>
#include <string>
#include <filesystem>
#include <utility>
#include "inetclientstream.hpp"
#include "server.h"
#include "log.h"

using libsocket::inet_stream;

cWebBridgeServer *WebBridgeServer;

cWebBridgeServer::cWebBridgeServer(int portP, const char *Description, bool LowPriority) : cThread(Description, LowPriority) {
    port = portP;
    WebBridgeServer = this;
    svdrpSocket = nullptr;
}

cWebBridgeServer::~cWebBridgeServer() {
    WebBridgeServer = nullptr;
}

void cWebBridgeServer::Action() {
    uWS::App app = uWS::App()
        .ws<PerSocketData>("/svdrp", {
            .compression = uWS::SHARED_COMPRESSOR,
            .maxPayloadLength = 1*1024*1024,
            .idleTimeout = 12,
            .maxBackpressure = 1*1024*1024,
            .upgrade = [](auto *res, auto *req, auto *context) {
              res->template upgrade<PerSocketData>({
                                                       .something = 13
                                                   },
                                                   req->getHeader("sec-websocket-key"),
                                                   req->getHeader("sec-websocket-protocol"),
                                                   req->getHeader("sec-websocket-extensions"),
                                                   context);
            },

            .open = [this](auto *ws) {
              svdrpSocket = ws;
            },

            .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
              debug5("[webbridge] %s, SVDRP message %s", __PRETTY_FUNCTION__, std::string(message).c_str());

              // call svdrp and return output
              int sfd;
              unsigned long ret, bytes;
              char buf[4096];

              ret = sfd = create_inet_stream_socket("127.0.0.1", "6419", LIBSOCKET_IPv4, 0);

              if (ret < 0) {
                  sendError(ws, opCode, ret);
                  return;
              }

              std::string request = std::string(message).append("\n");
              ret = write(sfd, request.c_str(), request.length());

              if (ret < 0) {
                  sendError(ws, opCode, ret);
                  return;
              }

              ret = shutdown_inet_stream_socket(sfd, LIBSOCKET_WRITE);

              if (ret < 0) {
                  sendError(ws, opCode, ret);
                  return;
              }

              std::string result;
              while (0 < (bytes = read(sfd, buf, 1024))) {
                  result.append(std::string_view(buf, bytes));
              }

              ws->send(result, opCode, false);

              ret = destroy_inet_socket(sfd);

              if (ret < 0) {
                  sendError(ws, opCode, ret);
                  return;
              }
            },

            .drain = [](auto *) {
              printf("Drain\n");
            },

            .close = [this](auto *, int, std::string_view) {
              svdrpSocket = nullptr;
            }
        })

        .listen(port, [this](auto *socket) {
          if (socket) {
              std::cout << "Start WebBridgeServer on port " << port << std::endl;
              this->listenSocket = socket;
          }
        });

    globalAppLoop = uWS::Loop::get();
    globalApp = &app;

    app.run();

    globalApp = nullptr;
}

void cWebBridgeServer::sendError(uWS::WebSocket<false, true, PerSocketData> *ws, const uWS::OpCode &opCode, int ret) const {
    const char* errorMsg = strerror(ret);

    debug1("[webbridge] %s, error: %s", __PRETTY_FUNCTION__, errorMsg);

    cString err = cString::sprintf("500 %s\n", errorMsg);
    ws->send(std::string_view(errorMsg, strlen(errorMsg) + 1), opCode, false);
}

void cWebBridgeServer::Cancel(int WaitSeconds) {
    // stop at first the websocket server and then call the super function
    globalAppLoop->defer([this]() {
      us_listen_socket_close(false, listenSocket);
    });

    cThread::Cancel(WaitSeconds);
}