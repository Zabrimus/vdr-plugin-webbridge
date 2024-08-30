#pragma once

#include <vdr/device.h>
#include "ffmpeghls.h"
#include "webosd.h"
#include "webstatus.h"

class cWebDevice : public cDevice {
private:
    int lastPrimaryDevice;

    cFFmpegHLS *ffmpegHls;
    cWebStatus *webStatus;

public:
    cWebDevice();
    ~cWebDevice() override;

    cString DeviceName() const override { return "WebOutDevice"; };

    bool HasDecoder() const override;

    bool SetPlayMode(ePlayMode PlayMode) override;
    int PlayVideo(const uchar *Data, int Length) override;

    int PlayAudio(const uchar *Data, int Length, uchar Id) override;

    int PlayTsVideo(const uchar *Data, int Length) override;
    int PlayTsAudio(const uchar *Data, int Length) override;
    int PlayTsSubtitle(const uchar *Data, int Length) override;

    int PlayPes(const uchar *Data, int Length, bool VideoOnly) override;
    int PlayTs(const uchar *Data, int Length, bool VideoOnly) override;

    bool Poll(cPoller &Poller, int TimeoutMs) override;
    bool Flush(int TimeoutMs) override;

    void Activate(bool On);

    void GetOsdSize(int &Width, int &Height, double &Aspect) override;

    cRect CanScaleVideo(const cRect &Rect, int Alignment);
    void ScaleVideo(const cRect &Rect) override;

    void channelSwitch();
    void changeAudioTrack();
    void Replaying(bool On);

protected:
    void MakePrimaryDevice(bool On) override;
};

extern cWebDevice *webDevice;
