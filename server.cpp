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
#include <vdr/plugin.h>
#include <vdr/config.h>
#include "inetclientstream.hpp"
#include "server.h"
#include "log.h"
#include "webremote.h"
#include "ffmpeghls.h"
#include "webdevice.h"
#include "config.h"

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

/**
 * AsyncFileStreamer copied from https://github.com/uNetworking/uWebSockets/discussions/1352
 */
#define AsyncFileStreamer_BLOCK_SIZE (65536)

struct AsyncStreamer {
    std::string root;

    explicit AsyncStreamer() = default;
    explicit AsyncStreamer(std::string root) : root(std::move(root)) {};

    void setRoot(std::string rootP) { root = std::move(rootP); };

    template<bool SSL>
    std::pair<bool, bool>
    sendBlock(uWS::HttpResponse<SSL> *res, FILE *f, long offset, size_t len, size_t &actuallyRead) {
        int blockSize = AsyncFileStreamer_BLOCK_SIZE;
        char block[AsyncFileStreamer_BLOCK_SIZE];

        fseek(f, offset, SEEK_SET);
        actuallyRead = fread(block, 1, blockSize, f);
        actuallyRead = actuallyRead > 0 ? actuallyRead : 0; // truncate anything < 0

        return res->tryEnd(std::string_view(block, actuallyRead), len);
    }

    template<bool SSL>
    void streamFile(uWS::HttpResponse<SSL> *res, std::string_view url) {
        // NOTE: This is very unsafe, people can inject things like ../../bla in the URL and completely bypass the root folder restriction.
        // Make sure to harden this code if it's intended to use it in anything internet facing.
        std::string nview = root + "/" + std::string{url.substr(1)};
        FILE *f = fopen(nview.c_str(), "rb");

        stream(res, f);
    }

    template<bool SSL>
    void streamVideo(uWS::HttpResponse<SSL> *res, std::string_view url) {
        // NOTE: This is very unsafe, people can inject things like ../../bla in the URL and completely bypass the root folder restriction.
        // Make sure to harden this code if it's intended to use it in anything internet facing.
        std::string nview = std::string(STREAM_DIR) + "/" + std::string{url.substr(8)};

        // wait maximum x seconds until file exists
        int sleepTime = 100; // ms
        int sleepCountMax = (15 * 1000) / sleepTime;
        std::filesystem::path desiredFile{nview};
        while (!std::filesystem::exists(desiredFile) && --sleepCountMax > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }

        if (std::filesystem::exists(desiredFile)) {
            FILE *f = fopen(nview.c_str(), "rb");
            stream(res, f);
        } else {
            res->writeStatus("404 Not Found");
            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            res->end("<b>404 Not Found</b>");
        }
    }

    template<bool SSL>
    void streamBuffer(uWS::HttpResponse<SSL> *res, void *buffer, size_t size) {
        FILE *f = fmemopen(buffer, size, "rb");
        stream(res, f);
    }

    template<bool SSL>
    void stream(uWS::HttpResponse<SSL> *res, FILE *file) {
        if (file == nullptr) {
            res->writeStatus("404 Not Found");
            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            res->end("<b>404 Not Found</b>");
        } else {
            res->writeStatus(uWS::HTTP_200_OK);
            fseek(file, 0, SEEK_END);
            long len = ftell(file);
            fseek(file, 0, SEEK_SET);

            res->onAborted([file]() {
              fclose(file);
            });

            auto fillStream = [this, len, res, file]() {
              bool retry;
              bool completed;
              do {
                  size_t actuallyRead;
                  auto tryEndResult = sendBlock(res, file, res->getWriteOffset(), len, actuallyRead);
                  retry = tryEndResult.first /*ok*/ && tryEndResult.second == false /*completed == false*/ ;
                  completed = tryEndResult.second;
              } while (retry);

              if (completed)
                  fclose(file);
            };

            res->onWritable([this, fillStream](uintmax_t offset) {
              fillStream();
              return true;
            });

            fillStream();
        }
    }
};

AsyncStreamer streamer;

cWebBridgeServer::cWebBridgeServer(int portP, const char *Description, bool LowPriority) : cThread(Description, LowPriority) {
    port = portP;
    WebBridgeServer = this;
    svdrpSocket = nullptr;

    cString staticPath = cString::sprintf("%s/live",cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));
    streamer = AsyncStreamer(*staticPath);
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

                    logerror("Connection denied from %s", denStr);
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
              svdrpSocket = nullptr;
            }
        })

        .ws<PerSocketData>("/tv", {
            .compression = uWS::SHARED_COMPRESSOR,
            .maxPayloadLength = 1 * 1024 * 1024,
            .idleTimeout = 12,
            .maxBackpressure = 1 * 1024 * 1024,
            .upgrade = [](auto *res, auto *req, auto *context) {
              res->template upgrade<PerSocketData>({
                                                       /* We initialize PerSocketData struct here */
                                                       .something = 13
                                                   }, req->getHeader("sec-websocket-key"),
                                                   req->getHeader("sec-websocket-protocol"),
                                                   req->getHeader("sec-websocket-extensions"),
                                                   context);
            },

            .open = [this](auto *ws) {
              osdSocket = ws;
              sendSize();
              webDevice->Activate(true);
            },

            .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
              debug5("Got message: %s:%ld", std::string(message).c_str(), message.length());

              long unsigned int position;
              if ((position = message.find(':')) != std::string::npos) {
                  auto token = message.substr(0, position - 1);
                  auto data = message.substr(position + 1, message.length());

                  if (token.compare("3")) {
                      // got key from browser
                      receiveKeyEvent(data);
                  }
              }
            },

            .drain = [](auto */*ws*/) {
              /* Check ws->getBufferedAmount() here */
            },

            .close = [this](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
              osdSocket = nullptr;
              webDevice->Activate(false);
            }
        })

        .get("/", [this](auto *res, auto *req) {
          // send index.html
          res->writeHeader("Content-Type", "text/html; charset=utf-8");
          streamer.streamFile(res, "/index.html");
        })

        .get("/vdrconfig", [this](auto *res, auto *req) {
          // send parameter
          res->writeStatus(uWS::HTTP_200_OK);
          res->writeHeader("Content-Type", "application/javascript");
          res->write("vdr_host=\""); res->write(*WebBridgeConfig.GetWebsocketHost()); res->write("\";\n");
          res->write("vdr_port=\""); res->write(std::to_string(WebBridgeConfig.GetWebsocketPort()));  res->write("\";\n");
          res->end("");
          WebBridgeConfig.GetWebsocketPort();
        })

        .get("/video.js", [](auto *res, auto *req) {
          res->writeHeader("Content-Type", "application/javascript");
          streamer.streamFile(res, "/video.min.js");
        })

        .get("/video-js.css", [](auto *res, auto *req) {
          res->writeHeader("Content-Type", "text/css");
          streamer.streamFile(res, "/video-js.css");
        })

        .get("/stream/*", [](auto *res, auto *req) {
          // send video stream / m3u8
          streamer.streamVideo(res, req->getUrl());
        })

        .get("/*", [](auto *res, auto *req) {
          debug1("File %s is not configured\n", std::string(req->getUrl()).c_str());

          res->writeStatus("404 Not Found");
          res->writeHeader("Content-Type", "text/html; charset=utf-8");
          res->end("<b>404 Not Found</b>");
        })

        .listen(port, [this](auto *socket) {
          if (socket) {
              info("Start WebBridgeServer on port %d", port);
              printf("Start WebBridgeServer on port %d", port);
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

int cWebBridgeServer::sendPngImage(int x, int y, int w, int h, int bufferSize, uint8_t *buffer) {
    uint8_t sendBuffer[7 * sizeof(uint32_t) + bufferSize];

    // get current OSD size
    int width;
    int height;
    double pa;

    cDevice::PrimaryDevice()->GetOsdSize(width, height, pa);

    // fill buffer
    ((uint32_t *) sendBuffer)[0] = MESSAGE_TYPE_PNG; // type PNG
    ((uint32_t *) sendBuffer)[1] = width;
    ((uint32_t *) sendBuffer)[2] = height;
    ((uint32_t *) sendBuffer)[3] = x;
    ((uint32_t *) sendBuffer)[4] = y;
    ((uint32_t *) sendBuffer)[5] = w;
    ((uint32_t *) sendBuffer)[6] = h;
    memcpy(sendBuffer + 7 * sizeof(uint32_t), reinterpret_cast<const void *>(buffer), bufferSize);

    // sanity check
    if (osdSocket == nullptr) {
        return -1;
    }

    return osdSocket->send(std::string_view((char *) &sendBuffer, 20 + bufferSize), uWS::OpCode::BINARY);
}

int cWebBridgeServer::scaleVideo(int top, int left, int w, int h) {
    int count = 5;

    uint8_t sendBuffer[count * sizeof(uint32_t)];

    // fill buffer
    ((uint32_t *) sendBuffer)[0] = MESSAGE_TYPE_SCALE_VIDEO;
    ((uint32_t *) sendBuffer)[1] = top;
    ((uint32_t *) sendBuffer)[2] = left;
    ((uint32_t *) sendBuffer)[3] = w;
    ((uint32_t *) sendBuffer)[4] = h;

    // sanity check
    if (osdSocket == nullptr) {
        return -1;
    }

    return osdSocket->send(std::string_view((char *) &sendBuffer, count * sizeof(uint32_t)), uWS::OpCode::BINARY);
}

int cWebBridgeServer::sendSize() {
    uint32_t sendBuffer[3];

    int width;
    int height;
    double pa;

    cDevice::PrimaryDevice()->GetOsdSize(width, height, pa);

    // fill buffer
    sendBuffer[0] = MESSAGE_TYPE_SIZE; // type SIZE
    sendBuffer[1] = width;
    sendBuffer[2] = height;

    // sanity check
    if (osdSocket == nullptr) {
        return -1;
    }

    return osdSocket->send(std::string_view((char *) &sendBuffer, sizeof(sendBuffer)), uWS::OpCode::BINARY);
}

int cWebBridgeServer:: sendClearOsd() {
    uint32_t sendBuffer[1];

    // clear OSD
    sendBuffer[0] = MESSAGE_TYPE_CLEAR_OSD; // type CLEAR_OSD

    // sanity check
    if (osdSocket == nullptr) {
        return -1;
    }

    return osdSocket->send(std::string_view((char *) &sendBuffer, sizeof(sendBuffer)), uWS::OpCode::BINARY);
}

int cWebBridgeServer::sendPlayerReset() {
    uint32_t sendBuffer[1];

    // reset
    sendBuffer[0] = MESSAGE_TYPE_RESET; // type RESET

    // sanity check
    if (osdSocket == nullptr) {
        return -1;
    }

    return osdSocket->send(std::string_view((char *) &sendBuffer, sizeof(sendBuffer)), uWS::OpCode::BINARY);
}

void cWebBridgeServer::receiveKeyEvent(std::string_view event) {
    std::string data = std::string(event);
    webRemote.ProcessKey(data.c_str());
}
