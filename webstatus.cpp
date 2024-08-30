#include "webstatus.h"
#include "webdevice.h"
#include "server.h"

void cWebStatus::ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView) {
    printf("Channel Switch %d, Live %s\n", ChannelNumber, LiveView ? "ja" : "nein");

    if (!LiveView || ChannelNumber==0) {
        return;
    }

    // recreate the receiver
    webDevice->channelSwitch();
}

void cWebStatus::SetAudioTrack(int Index, const char *const *Tracks) {
    printf("Set Audio Track to %d\n", Index);
    printf("Track: %s\n", Tracks[Index]);

    webDevice->changeAudioTrack();
}

void cWebStatus::Replaying(const cControl *Control, const char *Name, const char *FileName, bool On) {
    printf("Replaying: %s -> %s -> %s\n", Name, FileName, On ? " An " : " Aus");
    webDevice->Replaying(On);
}