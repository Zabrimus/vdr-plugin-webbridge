#include "webremote.h"

cWebRemote webRemote;

cWebRemote::cWebRemote() : cRemote("WebOsd") {
}

cWebRemote::~cWebRemote() = default;

bool cWebRemote::ProcessKey(const char *Code, bool Repeat, bool Release) {
    // extract shift, alt, ctrl, meta (currently not used)
    // bool shift = Code[0] == '1';
    // bool alt = Code[1] == '1';
    // bool ctrl = Code[2] == '1';
    // bool meta = Code[3] == '1';

    return cRemote::Put(Code + 5, Repeat, Release);
}
