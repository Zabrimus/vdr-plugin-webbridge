#include "webdevice.h"
#include "server.h"
#include "log.h"

const int maxTs = 256;
uint8_t tsBuffer[maxTs*188];
int tsBufferIdx = 0;

cWebDevice *webDevice;

cWebDevice::cWebDevice() {
    debug1("%s", __PRETTY_FUNCTION__);
    webDevice = this;
    lastPrimaryDevice = -1;
}

cWebDevice::~cWebDevice() {
    debug1("%s", __PRETTY_FUNCTION__);
    webDevice = nullptr;

    if (webStatus!=nullptr) {
        delete webStatus;
    }

    if (ffmpegHls!=nullptr) {
        delete ffmpegHls;
        ffmpegHls = nullptr;
    }
}

bool cWebDevice::HasDecoder() const {
    debug9("%s", __PRETTY_FUNCTION__);
    return true;
}

bool cWebDevice::SetPlayMode(ePlayMode PlayMode) {
    debug3("%s", __PRETTY_FUNCTION__);
    return true;
}

int cWebDevice::PlayVideo(const uchar *Data, int Length) {
    debug9("%s", __PRETTY_FUNCTION__);
    return Length;
}

int cWebDevice::PlayAudio(const uchar *Data, int Length, uchar Id) {
    debug9("%s", __PRETTY_FUNCTION__);
    return Length;
}

int cWebDevice::PlayTsVideo(const uchar *Data, int Length) {
    debug9("%s", __PRETTY_FUNCTION__);
    return Length;
}

int cWebDevice::PlayTsAudio(const uchar *Data, int Length) {
    debug9("%s", __PRETTY_FUNCTION__);
    return Length;
}

int cWebDevice::PlayTsSubtitle(const uchar *Data, int Length) {
    debug9("%s", __PRETTY_FUNCTION__);
    return Length;
}

int cWebDevice::PlayPes(const uchar *Data, int Length, bool VideoOnly) {
    debug9("%s", __PRETTY_FUNCTION__);
    return Length;
}

int cWebDevice::PlayTs(const uchar *Data, int Length, bool VideoOnly) {
    debug9("%s", __PRETTY_FUNCTION__);

    if (ffmpegHls!=nullptr) {
        if (Length==188) {
            // buffer this single packet
            if (tsBufferIdx < maxTs - 1) {
                memcpy(tsBuffer + tsBufferIdx*188, Data, Length);
                tsBufferIdx++;
            } else {
                memcpy(tsBuffer + tsBufferIdx*188, Data, Length);
                ffmpegHls->Receive((const uint8_t *) tsBuffer, maxTs*188);
                tsBufferIdx = 0;
            }
        } else {
            // more than one packet (mostly playing a recording)
            ffmpegHls->Receive((const uint8_t *) Data, Length);
            tsBufferIdx = 0;
        }
    }

    return Length;
}

void cWebDevice::Clear() {
    debug1("%s", __PRETTY_FUNCTION__);

    cDevice::Clear();
}

void cWebDevice::Play() {
    debug1("%s", __PRETTY_FUNCTION__);

    cDevice::Play();
}

bool cWebDevice::Flush(int TimeoutMs) {
    debug4("%s", __PRETTY_FUNCTION__);
    return true;
}

bool cWebDevice::Poll(cPoller &Poller, int TimeoutMs) {
    debug4("%s", __PRETTY_FUNCTION__);
    return true;
}

void cWebDevice::MakePrimaryDevice(bool On) {
    debug1("%s", __PRETTY_FUNCTION__);

    if (On) {
        new cWebOsdProvider();
    }
}

void cWebDevice::Activate(bool On) {
    debug1("%s: On %s", __PRETTY_FUNCTION__, On ? "true" : "false");

    if (ffmpegHls!=nullptr) {
        auto t = ffmpegHls;
        ffmpegHls = nullptr;
        delete t;
    }

    if (On) {
        lastPrimaryDevice = cDevice::PrimaryDevice()->DeviceNumber();
        cString channelText;

        {
            LOCK_CHANNELS_READ
            const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());
            channelText = channel->ToText();
        }

        ffmpegHls = new cFFmpegHLS(false, channelText, "", "");

        // switch primary device to webdevice
        Setup.PrimaryDVB = DeviceNumber() + 1;

        webStatus = new cWebStatus();
    } else {
        if (webStatus!=nullptr) {
            DELETENULL(webStatus);
        }

        // switch primary device back to the former one
        if (lastPrimaryDevice!=-1) {
            Setup.PrimaryDVB = lastPrimaryDevice + 1;
            lastPrimaryDevice = -1;
        }
    }
}

void cWebDevice::GetOsdSize(int &Width, int &Height, double &Aspect) {
    Width = 1920;
    Height = 1080;
    Aspect = 16.0/9.0;
}

cRect cWebDevice::CanScaleVideo(const cRect &Rect, int Alignment) {
    debug3("%s", __PRETTY_FUNCTION__);
    return Rect;
}

void cWebDevice::ScaleVideo(const cRect &Rect) {
    debug1("%s, ScaleVideo to Top=%d, Left=%d, Width=%d, Height=%d",
           __PRETTY_FUNCTION__,
           Rect.Top(),
           Rect.Left(),
           Rect.Width(),
           Rect.Height());

    int left = Rect.Left();
    int top = Rect.Top();
    int w = Rect.Width();
    int h = Rect.Height();

    WebBridgeServer->scaleVideo(top, left, w, h);
}

void cWebDevice::channelSwitch() {
    debug1("%s", __PRETTY_FUNCTION__);

    if (ffmpegHls!=nullptr) {
        auto t = ffmpegHls;
        ffmpegHls = nullptr;
        delete t;
    }

    cString channelText;

    {
        LOCK_CHANNELS_READ
        const cChannel *channel = Channels->GetByNumber(cDevice::CurrentChannel());
        channelText = channel->ToText();
    }

    ffmpegHls = new cFFmpegHLS(false, channelText, "", "");

    WebBridgeServer->sendPlayerReset();
}

void cWebDevice::changeAudioTrack() {
    debug1("%s", __PRETTY_FUNCTION__);

    // printf("ChangeAudioTrack from %d to %d\n", currentAudioPid, getCurrentAudioPID());

    // TODO: currently disabled
    return;

    // DelPid(currentAudioPid);
    // AddPid(getCurrentAudioPID());

    // TODO: Der Videoplayer müsste wahrscheinlich über den Wechsel benachrichtigt werden.
}

void cWebDevice::Replaying(bool On) {
    debug1("%s, Replaying: %s", __PRETTY_FUNCTION__, On ? "An" : "Aus");

    auto t = ffmpegHls;
    ffmpegHls = nullptr;
    delete t;

    if (On) {
        ffmpegHls = new cFFmpegHLS(true, "", recName, recFileName);
    }

    WebBridgeServer->sendPlayerReset();
}
