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
#include <vdr/config.h>
#include "inetclientstream.hpp"
#include "server.h"
#include "log.h"

using libsocket::inet_stream;

std::string_view addressAsText(std::string_view binary) {
    static thread_local char buf[64];
    int ipLength = 0;

    if (!binary.length()) {
        return {};
    }

    unsigned char *b = (unsigned char *) binary.data();

    if (binary.length() == 4) {
        ipLength = snprintf(buf, 64, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
    } else {
        ipLength = snprintf(buf, 64, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                            b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11],
                            b[12], b[13], b[14], b[15]);
    }

    return {buf, (unsigned int) ipLength};
}

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

            .open = [this](uWS::WebSocket<false, true, PerSocketData> *ws) {
                auto ipStr = addressAsText(ws->getRemoteAddress().substr(12));
                struct sockaddr_in sa;
                inet_pton(AF_INET, std::string(ipStr).c_str(), &(sa.sin_addr));

                if (!SVDRPhosts.Acceptable(sa.sin_addr.s_addr)) {
                    char denStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(sa.sin_addr), denStr, INET_ADDRSTRLEN);

                    error("Connection denied from %s", denStr);
                    ws->close();
                }
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