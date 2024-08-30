#include "easywsclient.h"
#include "easywsclient.cpp"
#include <assert.h>
#include <stdio.h>
#include <string>

using easywsclient::WebSocket;
static WebSocket::pointer ws = NULL;

void handle_message(const std::string & message)
{
    printf(">>> %s\n", message.c_str());
    ws->close();
}

int main()
{
    ws = WebSocket::from_url("ws://localhost:3000/svdrp");
    assert(ws);
    ws->send("help");
    while (ws->getReadyState() != WebSocket::CLOSED) {
        ws->poll();
        ws->dispatch(handle_message);
    }
    delete ws;

    return 0;
}