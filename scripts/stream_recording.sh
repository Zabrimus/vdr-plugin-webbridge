#!/bin/bash

# $1 is the recording name
# $2 is the recording filename
#
# Evaluate the recording and construct your own ffmpeg/vlc command
#

# Set accordingly
ENCODE_VIDEO=false
ENCODE_AUDIO=true

# Video/Audio encode/copy parameter
VIDEO_ENCODE="-crf 23 -c:v libx264 -tune zerolatency -vf format=yuv420p -preset ultrafast -qp 0"
VIDEO_ENCODE_COPY="-c:v copy"
AUDIO_ENCODE="-c:a aac -b:a 384k -channel_layout 5.1"
AUDIO_ENCODE_COPY="-c:a copy"

if [ "${ENCODE_VIDEO}" = "true" ]; then
  VIDEO_COMMAND="${VIDEO_ENCODE}"
else
  VIDEO_COMMAND="${VIDEO_ENCODE_COPY}"
fi

if [ "${ENCODE_AUDIO}" = "true" ]; then
  AUDIO_COMMAND="${AUDIO_ENCODE}"
else
  AUDIO_COMMAND="${AUDIO_ENCODE_COPY}"
fi

cat<<EOF
ffmpeg -re -i - \
      -v panic -hide_banner -ignore_unknown \
      -fflags flush_packets -max_delay 5 -flags -global_header \
      -hls_time 5 -hls_list_size 3 -hls_delete_threshold 3 \
      -hls_segment_filename 'vdr-live-tv-%05d.ts' \
      -hls_segment_type mpegts \
      -hls_flags delete_segments \
      -map 0:v -map 0:a?  \
      ${VIDEO_COMMAND} \
      ${AUDIO_COMMAND} \
      -y vdr-live-tv.m3u8
EOF
