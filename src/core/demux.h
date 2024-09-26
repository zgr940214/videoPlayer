#ifndef _DEMUX_H_
#define _DEMUX_H_
#include "video/video_common.h"
#include "core/core.h"

#include "stream.h"
#define MAX_STREAMS 8

// 解码后的帧
typedef struct demuxer_packet {
    sh_stream_t         *st;    // ref identify video/audio, also let decoder know which source these docoded frame belongs to
    uint8_t             *data;
    uint32_t            len;
} demuxer_packet_t;

typedef struct demuxer_ops 
{
    // create and init priv
    int     (*open)(demuxer_t *demuxer, void **priv);
    void    (*close)(struct demuxer *demuxer);
    bool    (*read_packet)(demuxer_t *demuxer, struct demuxer_frame_t* frame);
    // also clear buffered frames
    void    (*seek)(struct demuxer *demuxer, double rel_seek_secs, int flags);
    
} demuxer_ops_t;

// each demuxer related to an opened url(video file or online streams)  
struct demuxer {
    ao                  *audio_output;
    vo                  *video_output;

    // demuxer implementation specific data
    void                **priv;

    // Number of frames buffered before triggering event to send to decoder
    uint32_t            buffered_frames; 

    sh_stream_t         *streams[MAX_STREAMS];
    const char          *url;
    const char          *url_desc;
};

#endif