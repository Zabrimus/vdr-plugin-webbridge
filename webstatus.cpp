#include "webstatus.h"
#include "webdevice.h"
#include "server.h"
#include "log.h"

void cWebStatus::ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView) {
    debug1("%s: ChannelNumber %d, Live %s", __PRETTY_FUNCTION__, ChannelNumber, LiveView ? "ja" : "nein");
    printf("Channel Switch %d, Live %s\n", ChannelNumber, LiveView ? "ja" : "nein");

    if (!LiveView || ChannelNumber==0) {
        return;
    }

    // recreate the receiver
    webDevice->channelSwitch();
}

void cWebStatus::SetAudioTrack(int Index, const char *const *Tracks) {
    debug1("%s", __PRETTY_FUNCTION__);

    // printf("Set Audio Track to %d\n", Index);
    // printf("Track: %s\n", Tracks[Index]);

    webDevice->changeAudioTrack();
}

void cWebStatus::Replaying(const cControl *Control, const char *Name, const char *FileName, bool On) {
    debug1("%s", __PRETTY_FUNCTION__);

    webDevice->SetRecordingName(Name);
    webDevice->SetRecordingFileName(FileName);
    webDevice->Replaying(On);
}