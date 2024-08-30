#pragma once

#include <vdr/remote.h>

class cWebRemote : cRemote {
public:
    explicit cWebRemote();
    ~cWebRemote() override;

    bool ProcessKey(const char *Code, bool Repeat = false, bool Release = false);
};

extern cWebRemote webRemote;
