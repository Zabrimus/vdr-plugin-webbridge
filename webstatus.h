#pragma once

#include <vdr/status.h>

class cWebStatus : public cStatus {
protected:
    void SetAudioTrack(int Index, const char *const *Tracks) override;
    // The audio track has been set to the one given by Index, which
    // points into the Tracks array of strings. Tracks is NULL terminated.

    void ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView) override;
    // Indicates a channel switch on the given DVB device.
    // If ChannelNumber is 0, this is before the channel is being switched,
    // otherwise ChannelNumber is the number of the channel that has been switched to.
    // LiveView tells whether this channel switch is for live viewing.

    void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On) override;
    // The given player control has started (On = true) or stopped (On = false) replaying Name.
    // Name is the name of the recording, without any directory path. In case of a player that can't provide
    // a name, Name can be a string that identifies the player type (like, e.g., "DVD").
    // The full file name of the recording is given in FileName, which may be NULL in case there is no
    // actual file involved. If On is false, Name may be NULL.
};