#pragma once

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfiltergraph.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>

typedef struct FilterStreamContext {
  int              is_video;
  char*            filter_spec;
  AVFilterContext* buffersink_ctx;
  AVFilterContext* buffersrc_ctx;
  AVFilterGraph*   filter_graph;
  AVStream*        os;
  AVStream*        is;
  enum AVCodecID   enc_id;
} FilterStreamContext;

typedef struct gang_decoder {
  char* url;
  char* rec_name;
  int   rec_enabled;
  int   audio_off;
  int   no_video;
  int   no_audio;
  int   waitkey;
  int   recording;

  // vidio
  int                width;
  int                height;
  int                fps;
  enum AVPixelFormat pix_fmt;

  // audio
  int sample_rate;
  int channels;
  int bytes_per_sample;

  // video decode buff
  uint8_t* video_buff;
  uint8_t* audio_buff;
  int      video_buff_size;
  int      audio_buff_size;

  AVFormatContext*     ifmt_ctx;
  AVFormatContext*     ofmt_ctx;
  FilterStreamContext* fscs;
  size_t               fsc_size;

  AVPacket i_pkt;
  AVPacket o_pkt;
  AVFrame* i_frame;
  AVFrame* o_frame;
} gang_decoder;

#ifdef __cplusplus
} // closing brace for extern "C"
#endif // ifdef __cplusplus
