/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2014 Andrey Utkin
 * Copyright (c) 2015 empirefox@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ffmpeg_transcoding.h"

#include <libavformat/avio.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <macrologger.h>
#include <string.h>
#include <time.h>

#include "ffmpeg_format.h"

static char* timed_name(char* name, const char* short_name) {
  time_t     rawtime;
  struct tm* info;
  char       buffer[24];

  time(&rawtime);
  info = localtime(&rawtime);
  strftime(buffer, 24, "-[%m-%d]-[%H-%M-%S].mkv", info);
  snprintf(name, strlen(short_name) + 24, "%s%s", short_name, buffer);
  return name;
}

static int normalize_opus_rate(int r) {
  return r >= 44100 ?
         48000 : (r >= 24000 ? 24000 : (r >= 16000 ? 16000 : (r >= 12000 ? 12000 : 8000)));
}

int open_input_streams(gang_decoder* dec) {
  FilterStreamContext* fs_ctx      = NULL;
  AVStream*            i_v_s       = NULL;
  AVStream*            i_a_s       = NULL;
  size_t               stream_size = 0;
  unsigned int         i           = 0;
  int                  ret;

  if ((ret = open_input_file(dec->url, &dec->ifmt_ctx, &i_v_s, &i_a_s, dec->audio_off)) < 0) return ret;

  if (i_v_s) stream_size++;
  if (i_a_s) stream_size++;

  if (stream_size) {
    fs_ctx = (FilterStreamContext*)av_malloc_array(stream_size, sizeof(*fs_ctx));

    if (!fs_ctx) {
      LOG_ERROR("Could not malloc fscs array");
      return AVERROR(ENOMEM);
    }
  }
  dec->fsc_size = stream_size;

  if (i_v_s) {
    fs_ctx[i].is_video       = 1;
    fs_ctx[i].buffersrc_ctx  = NULL;
    fs_ctx[i].buffersink_ctx = NULL;
    fs_ctx[i].filter_graph   = NULL;
    fs_ctx[i].os             = NULL;
    fs_ctx[i].is             = i_v_s;
    fs_ctx[i++].enc_id       = AV_CODEC_ID_H264;
  }

  if (i_a_s) {
    fs_ctx[i].is_video       = 0;
    fs_ctx[i].buffersrc_ctx  = NULL;
    fs_ctx[i].buffersink_ctx = NULL;
    fs_ctx[i].filter_graph   = NULL;
    fs_ctx[i].os             = NULL;
    fs_ctx[i].is             = i_a_s;
    fs_ctx[i].enc_id         = AV_CODEC_ID_OPUS;
  }
  dec->fscs = fs_ctx;

  return 0;
}

// Create os
// Require is
static int open_output_stream(FilterStreamContext* fsc, AVFormatContext* o_fmt_ctx) {
  AVCodecContext* enc_ctx   = NULL;
  AVCodecContext* i_dec_ctx = fsc->is->codec;
  AVCodec*        encoder   = NULL;

  AVRational rate;
  int        i_chs         = i_dec_ctx->channels;
  int        i_sample_rate = i_dec_ctx->sample_rate;
  int        o_chs         = 0;
  int        o_sample_rate = 0;
  char       spec[255];

  int ret;

  /* in this example, we choose transcoding to same codec */
  encoder = avcodec_find_encoder(fsc->enc_id);

  if (!encoder) {
    LOG_INFO("Necessary encoder not found");
    return AVERROR_INVALIDDATA;
  }
  fsc->os = avformat_new_stream(o_fmt_ctx, encoder);

  if (!fsc->os) {
    LOG_INFO("Failed allocating output stream");
    return AVERROR_UNKNOWN;
  }
  enc_ctx = fsc->os->codec;

  /* In this example, we transcode to same properties (picture size,
   * sample rate etc.). These properties can be changed for output
   * streams easily using filters */
  if (fsc->is_video) {
    enc_ctx->bit_rate            = i_dec_ctx->bit_rate;
    enc_ctx->height              = i_dec_ctx->height;
    enc_ctx->width               = i_dec_ctx->width;
    enc_ctx->sample_aspect_ratio = i_dec_ctx->sample_aspect_ratio;

    /* take first format from list of supported formats */
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    /* video time_base can be set to whatever is handy and supported by encoder
     */
    fsc->os->time_base = av_make_q(1, 90000);

    //		fsc->os->duration = fsc->is->duration;
    enc_ctx->gop_size        = 10;
    enc_ctx->max_b_frames    = 1;
    enc_ctx->ticks_per_frame = 2;

    rate = i_dec_ctx->framerate;

    if (!rate.num) rate = fsc->is->r_frame_rate;

    if (!rate.num || !rate.den) rate = av_make_q(25, 1);

    // TODO add spec to fit in.
    fsc->filter_spec = av_strdup("null");

    //		if (av_q2d(i_dec_ctx->time_base) * i_dec_ctx->ticks_per_frame >
    // av_q2d(fsc->is->time_base)	&& av_q2d(fsc->is->time_base) < 1.0 / 1000) {
    enc_ctx->time_base = av_inv_q(rate);

    //		enc_ctx->time_base.num *= enc_ctx->ticks_per_frame;
    //		} else {
    //			enc_ctx->time_base = fsc->is->time_base;
    //		}
    //		fsc->os->disposition = fsc->is->disposition;
  } else {
    o_chs         = i_chs > 2 ? 2 : i_chs;
    o_sample_rate = normalize_opus_rate(i_sample_rate);

    enc_ctx->sample_rate    = o_sample_rate;
    enc_ctx->channel_layout = av_get_default_channel_layout(o_chs);
    enc_ctx->channels       = o_chs;

    /* take first format from list of supported formats */
    enc_ctx->sample_fmt    = AV_SAMPLE_FMT_S16;
    enc_ctx->time_base.den = o_sample_rate;
    enc_ctx->time_base.num = 1;

    // Because the default d is 20ms
    snprintf(spec, sizeof(spec), "asetnsamples=n=%d", o_sample_rate / 50);
    fsc->filter_spec = av_strdup(spec);
  }

  /* Third parameter can be used to pass settings to encoder */
  ret = avcodec_open2(enc_ctx, encoder, NULL);

  if (ret < 0) {
    LOG_INFO("Cannot open output stream: %s->%s", i_dec_ctx->codec->name, encoder->name);
    return ret;
  }

  //	o_fmt_ctx->flags |= AVFMT_FLAG_GENPTS;
  //	o_fmt_ctx->flags |= AVFMT_FLAG_IGNDTS;
  if (o_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  return 0;
}

// Create ofmt_ctx fs_ctx[i].os
// Write file header
// Require fs_ctx[i].is
int open_output_streams(gang_decoder* dec, int rec) {
  char         fullname[128];
  unsigned int i;
  int          ret;

  avformat_alloc_output_context2(&dec->ofmt_ctx, NULL, NULL, timed_name(fullname, dec->rec_name));

  if (!dec->ofmt_ctx) {
    LOG_ERROR("Could not create output context");
    return AVERROR_UNKNOWN;
  }

  for (i = 0; i < dec->fsc_size; i++) {
    ret = open_output_stream(&dec->fscs[i], dec->ofmt_ctx);

    if (ret < 0) {
      LOG_ERROR("open_output_stream failed");
      return ret;
    }
  }

  av_dump_format(dec->ofmt_ctx, 0, fullname, 1);

  if (!rec) {
    LOG_DEBUG("no record");
    return 0;
  }

  if (!(dec->ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&dec->ofmt_ctx->pb, fullname, AVIO_FLAG_WRITE);

    if (ret < 0) {
      LOG_INFO("Could not open output file '%s'", fullname);
      return ret;
    }
  }

  /* init muxer, write output file header */
  ret = avformat_write_header(dec->ofmt_ctx, NULL);
  if (ret < 0) {
    LOG_ERROR("Error occurred when opening output file");
    return ret;
  }
  dec->recording = 1;

  return 0;
}

#define SET_SINK_OPT(arg) ret = av_opt_set_bin(        \
    buffersink_ctx, #arg "s", (uint8_t*)&enc_ctx->arg, \
    sizeof(enc_ctx->arg), AV_OPT_SEARCH_CHILDREN);     \
  if (ret < 0) {                                       \
    LOG_INFO("Cannot set output "#arg "");             \
    goto end;                                          \
  }

// Require os is filter_spec
static int init_filter(FilterStreamContext* fsc) {
  char             args[512];
  int              ret            = 0;
  AVCodecContext*  enc_ctx        = NULL;
  AVCodecContext*  dec_ctx        = NULL;
  AVFilter*        buffersrc      = NULL;
  AVFilter*        buffersink     = NULL;
  AVFilterContext* buffersrc_ctx  = NULL;
  AVFilterContext* buffersink_ctx = NULL;
  AVFilterInOut*   outputs        = NULL;
  AVFilterInOut*   inputs         = NULL;
  AVFilterGraph*   filter_graph   = NULL;

  if (!fsc->is) {
    return ret;
  }

  enc_ctx      = fsc->os->codec;
  dec_ctx      = fsc->is->codec;
  outputs      = avfilter_inout_alloc();
  inputs       = avfilter_inout_alloc();
  filter_graph = avfilter_graph_alloc();

  if (!outputs || !inputs || !filter_graph) {
    LOG_ERROR("Cannot alloc outputs/inputs/graph");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (fsc->is_video) {
    buffersrc  = avfilter_get_by_name("buffer");
    buffersink = avfilter_get_by_name("buffersink");

    if (!buffersrc || !buffersink) {
      LOG_INFO("filtering source or sink element not found");
      ret = AVERROR_UNKNOWN;
      goto end;
    }

    snprintf(
      args,
      sizeof(args),
      "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      dec_ctx->width,
      dec_ctx->height,
      dec_ctx->pix_fmt,
      dec_ctx->time_base.num,
      dec_ctx->time_base.den,
      dec_ctx->sample_aspect_ratio.num,
      dec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args,  NULL, filter_graph);
    if (ret < 0) {
      LOG_INFO("Cannot create buffer source");
      goto end;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    if (ret < 0) {
      LOG_INFO("Cannot create buffer sink");
      goto end;
    }
    SET_SINK_OPT(pix_fmt)
  } else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
    buffersrc  = avfilter_get_by_name("abuffer");
    buffersink = avfilter_get_by_name("abuffersink");

    if (!buffersrc || !buffersink) {
      LOG_INFO("filtering source or sink element not found");
      ret = AVERROR_UNKNOWN;
      goto end;
    }

    if (!dec_ctx->channel_layout) dec_ctx->channel_layout = av_get_default_channel_layout(dec_ctx->channels);
    snprintf(
      args,
      sizeof(args),
      "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
      dec_ctx->time_base.num,
      dec_ctx->time_base.den,
      dec_ctx->sample_rate,
      av_get_sample_fmt_name(dec_ctx->sample_fmt),
      dec_ctx->channel_layout);
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);

    if (ret < 0) {
      LOG_INFO("Cannot create audio buffer source");
      goto end;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);

    if (ret < 0) {
      LOG_INFO("Cannot create audio buffer sink");
      goto end;
    }

    SET_SINK_OPT(sample_fmt)
    SET_SINK_OPT(channel_layout)
    SET_SINK_OPT(sample_rate)
  } else {
    ret = AVERROR_UNKNOWN;
    goto end;
  }

  /* Endpoints for the filter graph. */
  outputs->name       = av_strdup("in");
  outputs->filter_ctx = buffersrc_ctx;
  outputs->pad_idx    = 0;
  outputs->next       = NULL;

  inputs->name       = av_strdup("out");
  inputs->filter_ctx = buffersink_ctx;
  inputs->pad_idx    = 0;
  inputs->next       = NULL;

  if (!outputs->name || !inputs->name) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = avfilter_graph_parse_ptr(filter_graph, fsc->filter_spec, &inputs, &outputs, NULL)) < 0) goto end;

  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) goto end;

  /* Fill FilterStreamContext */
  fsc->buffersrc_ctx  = buffersrc_ctx;
  fsc->buffersink_ctx = buffersink_ctx;
  fsc->filter_graph   = filter_graph;

end: avfilter_inout_free(&inputs);
  avfilter_inout_free(&outputs);

  if (ret > 0) ret = 0;
  return ret;
}

int init_filters(FilterStreamContext* fscs, size_t n) {
  unsigned int i;
  int          ret;

  for (i = 0; i < n; i++) {
    ret = init_filter(&fscs[i]);

    if (ret) return ret;
  }
  return 0;
}

int encode_write_frame(gang_decoder* dec, FilterStreamContext* fsc, int* got_frame) {
  AVStream* os = fsc->os;
  int       ret;
  int       got_frame_local;
  AVFrame*  o_frame = got_frame ? NULL : dec->o_frame;

  int (* enc_func)(AVCodecContext*,
                   AVPacket*,
                   const AVFrame*,
                   int*) =
    fsc->is_video ? avcodec_encode_video2 : avcodec_encode_audio2;

  if (dec->waitkey) {
    if (dec->i_pkt.flags & AV_PKT_FLAG_KEY) {
      dec->waitkey = 0;
    } else  return 0;
  }

  if (!got_frame) got_frame = &got_frame_local;

  /* encode filtered frame */
  av_packet_unref(&dec->o_pkt);
  av_init_packet(&dec->o_pkt);

  ret = enc_func(os->codec, &dec->o_pkt, o_frame, got_frame);
  if (ret < 0) {
    LOG_INFO("enc_func error");
    return ret;
  }

  if (!(*got_frame)) return 0;

  /* prepare packet for muxing */
  dec->o_pkt.stream_index = os->index;

  //	dec->o_pkt.dts = AV_NOPTS_VALUE;
  if (fsc->is_video) {
    //		dec->o_pkt.duration *= 2 * dec->o_pkt.size;
    av_packet_rescale_ts(&dec->o_pkt, fsc->is->codec->time_base, os->time_base);
  } else {
    av_packet_rescale_ts(&dec->o_pkt, os->codec->time_base, os->time_base);
  }

  //	if (fsc->is_video) {
  //		dec->o_pkt.pts /= os->codec->ticks_per_frame;
  //		dec->o_pkt.dts /= os->codec->ticks_per_frame;
  //		dec->o_pkt.duration /= os->codec->ticks_per_frame;
  //	}
  //	if (fsc->is_video) {
  //		dec->o_pkt.pts = dec->pts_video++;
  //		dec->o_pkt.dts = dec->o_pkt.pts;
  //		av_packet_rescale_ts(&dec->o_pkt, (AVRational ) {2, 25},
  // os->time_base);
  //	} else {
  //		dec->o_pkt.pts = dec->pts_audio++ * (dec->o_pkt.size * 1000 /
  // os->codec->sample_rate);
  //	}

  /* mux encoded frame */
  ret = av_interleaved_write_frame(dec->ofmt_ctx, &dec->o_pkt);
  return ret;
}

int flush_encoder(gang_decoder* dec, FilterStreamContext* fsc) {
  int ret;
  int got_frame;

  if (!(fsc->os->codec->codec->capabilities & AV_CODEC_CAP_DELAY)) return 0;

  while (1) {
    // LOG_DEBUG("Flushing stream #%u encoder", fsc->os->index);
    ret = encode_write_frame(dec, fsc, &got_frame);
    if (ret < 0) break;

    if (!got_frame) return 0;
  }
  return ret;
}

int find_fs_index(int* index, FilterStreamContext* fscs, size_t fs_size, int stream_index) {
  int i;

  for (i = 0; i < fs_size; i++) {
    if (fscs[i].is->index == stream_index) {
      *index = i;
      return 0;
    }
  }
  return -1;
}
