#include "gang_decoder_impl.h"

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include "macrologger.h"

#include "ffmpeg_transcoding.h"
#include "ffmpeg_format.h"
#include "ffmpeg_log.h"

void initialize_gang_decoder_globel() {
  init_gang_av_log();
  avcodec_register_all();
  av_register_all();
  avformat_network_init();
  avfilter_register_all();
}

void cleanup_gang_decoder_globel() {
  avformat_network_deinit();
}

// create gang_decode with given url
gang_decoder* new_gang_decoder(const char* url, const char* rec_name, int rec_on, int audio_off) {
  gang_decoder* dec = (gang_decoder*)malloc(sizeof(gang_decoder));

  if (dec) {
    dec->url              = av_strdup(url);
    dec->rec_name         = av_strdup(rec_name);
    dec->rec_enabled      = rec_on;
    dec->audio_off        = audio_off;
    dec->no_video         = 1;
    dec->no_audio         = 1;
    dec->waitkey          = 1;
    dec->recording        = 0;
    dec->width            = 0;
    dec->height           = 0;
    dec->fps              = 0;
    dec->pix_fmt          = AV_PIX_FMT_NONE;
    dec->channels         = 0;
    dec->sample_rate      = 0;
    dec->bytes_per_sample = 2; // always output s16
    dec->video_buff       = NULL;
    dec->audio_buff       = NULL;
    dec->video_buff_size  = 0;
    dec->audio_buff_size  = 0;
    dec->ifmt_ctx         = NULL;
    dec->ofmt_ctx         = NULL;
    dec->fscs             = NULL;
    dec->fsc_size         = 0;
    dec->i_frame          = NULL;
    dec->o_frame          = NULL;
    av_init_packet(&dec->i_pkt);
    av_init_packet(&dec->o_pkt);
  }
  return dec;
}

void free_gang_decoder(gang_decoder* dec) {
  if (dec) {
    if (dec->url) {
      av_freep(&dec->url);
    }

    if (dec->rec_name) {
      av_freep(&dec->rec_name);
    }
    free(dec);
  }
}

static void init_av_info(gang_decoder* dec) {
  FilterStreamContext fsc;
  int                 i;

  for (i = 0; i < dec->fsc_size; i++) {
    fsc = dec->fscs[i];

    if (fsc.is_video) {
      dec->no_video        = 0;
      dec->pix_fmt         = fsc.os->codec->pix_fmt;
      dec->width           = fsc.os->codec->width;
      dec->height          = fsc.os->codec->height;
      dec->video_buff_size = av_image_get_buffer_size(dec->pix_fmt,
                                                      dec->width,
                                                      dec->height,
                                                      1);
      AVRational rate = fsc.is->r_frame_rate;

      if (rate.den) {
        dec->fps = rate.num / rate.den;
      } else {
        // TODO maybe need calculate it.
        dec->fps = 25;
      }
      LOG_DEBUG("pix_fmt:%s, width:%d, height:%d, fps:%d, buff_size:%d",
                av_get_pix_fmt_name(dec->pix_fmt),
                dec->width,
                dec->height,
                dec->fps,
                dec->video_buff_size);
    } else {
      dec->no_audio        = 0;
      dec->channels        = fsc.os->codec->channels;
      dec->sample_rate     = fsc.os->codec->sample_rate;
      dec->audio_buff_size = dec->bytes_per_sample * dec->channels * dec->sample_rate / 100;
      LOG_DEBUG("channels:%d, sample_rate:%d, buff_size:%d",
                dec->channels,
                dec->sample_rate,
                dec->audio_buff_size);
    }
  }
}

int init_gang_av_info(gang_decoder* dec) {
  int err;

  err = open_input_streams(dec);
  if (!err) err = open_output_streams(dec, 0);
  if (!err) init_av_info(dec);

  close_gang_decoder(dec);
  return err;
}

// return error
int open_gang_decoder(gang_decoder* dec) {
  int err;

  err = open_input_streams(dec);
  if (!err) err = open_output_streams(dec, dec->rec_enabled);
  if (!err) err = init_filters(dec->fscs, dec->fsc_size);
  if (!err) err = init_frame(&dec->i_frame);
  if (!err) err = init_frame(&dec->o_frame);

  if (err) {
    close_gang_decoder(dec);
  } else {
    init_av_info(dec);
    LOG_DEBUG("All are prepared with: audio:%d video:%d", !dec->no_video, !dec->no_audio);
  }
  return err;
}

void close_gang_decoder(gang_decoder* dec) {
  int i;

  LOG_DEBUG("flushing start...");
  flush_gang_rec_encoder(dec);
  LOG_DEBUG("flushed.");

  av_packet_unref(&dec->i_pkt);
  av_packet_unref(&dec->o_pkt);
  av_init_packet(&dec->i_pkt);
  av_init_packet(&dec->o_pkt);

  if (dec->o_frame) {
    av_frame_unref(dec->o_frame);
    av_frame_free(&dec->o_frame);
  }

  if (dec->i_frame) {
    av_frame_unref(dec->i_frame);
    av_frame_free(&dec->i_frame);
  }

  if (dec->ifmt_ctx) {
    for (i = 0; i < dec->ifmt_ctx->nb_streams; i++) {
      avcodec_close(dec->ifmt_ctx->streams[i]->codec);
    }
    avformat_close_input(&dec->ifmt_ctx);
  }

  if (dec->ofmt_ctx) {
    for (i = 0; i < dec->ofmt_ctx->nb_streams; i++) {
      avcodec_close(dec->ofmt_ctx->streams[i]->codec);
    }

    if (dec->rec_enabled && !(dec->ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&dec->ofmt_ctx->pb);
    avformat_free_context(dec->ofmt_ctx);
    dec->ofmt_ctx = NULL;
  }

  if (dec->fscs) {
    for (i = 0; i < dec->fsc_size; i++) {
      if (dec->fscs[i].filter_graph) {
        if (dec->fscs[i].filter_spec) {
          av_freep(&dec->fscs[i].filter_spec);
        }
        avfilter_graph_free(&dec->fscs[i].filter_graph);
      }
    }
    av_freep(&dec->fscs);
  }
  dec->fsc_size = 0;
  LOG_DEBUG("All are released.");
}

static int copy_send_frame(gang_decoder* dec, FilterStreamContext* fsc) {
  int ret = 0;

  if (fsc->is_video && dec->video_buff) {
    ret = av_image_copy_to_buffer(dec->video_buff,
                                  dec->video_buff_size,
                                  (const uint8_t**)(dec->o_frame->data),
                                  dec->o_frame->linesize,
                                  dec->pix_fmt,
                                  dec->width,
                                  dec->height,
                                  1);
  } else if (!fsc->is_video && dec->audio_buff) {
    memcpy(dec->audio_buff,
           dec->o_frame->extended_data[0],
           dec->audio_buff_size);
  } else {
    LOG_DEBUG("is_video:%d, no video_buff:%d, no audio_buff:%d",
              fsc->is_video,
              !dec->video_buff,
              !dec->audio_buff);
  }
  return ret;
}

// From transcoding.c
// When flushing, i_frame must be set NULL.
// So we do not auto get i_frame from dec.
static int filter_encode_write_frame(gang_decoder* dec, FilterStreamContext* fsc, int not_eof) {
  int ret;

  /* push the decoded frame into the filtergraph */
  ret = av_buffersrc_add_frame_flags(fsc->buffersrc_ctx, not_eof ? dec->i_frame : NULL, 0);
  if (ret < 0) {
    LOG_INFO("Error while feeding the filtergraph");
    return ret;
  }

  /* pull filtered frames from the filtergraph */
  while (1) {
    av_frame_unref(dec->o_frame);

    // av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters");
    ret = av_buffersink_get_frame(fsc->buffersink_ctx, dec->o_frame);
    if (ret < 0) {
      /* if no more frames for output - returns AVERROR(EAGAIN)
       * if flushed and no more frames for output - returns AVERROR_EOF
       * rewrite retcode to 0 to show it as normal procedure completion
       */
      if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF)) ret = 0;
      break;
    }

    dec->o_frame->pict_type = AV_PICTURE_TYPE_NONE;

    // send data to rtc
    // temp use not_eof to tag if sent data.
    if (not_eof) {
      not_eof = 0;
      ret     = copy_send_frame(dec, fsc);
      if (ret < 0) {
        LOG_DEBUG("copy and send frame to rtc error");
        break;
      }
    }

    // write to record file
    if (dec->recording) {
      ret = encode_write_frame(dec, fsc, NULL);
      if (ret < 0) {
        LOG_DEBUG("encode_write_frame error");
        break;
      }
    }
  }

  if (ret > 0) ret = 0;
  return ret;
}

/**
 * return: -1->FITAL, 0->error, 1->video, 2->audio
 */
int gang_decode_next_frame(gang_decoder* dec) {
  FilterStreamContext fsc;
  AVStream*           is;
  int                 got_frame;
  int                 fs_index;

  int (* dec_func)(AVCodecContext*,
                   AVFrame*,
                   int*,
                   const AVPacket*);
  int err;

  av_packet_unref(&dec->i_pkt);
  av_init_packet(&dec->i_pkt);

  if (!dec->ifmt_ctx || (av_read_frame(dec->ifmt_ctx, &dec->i_pkt) < 0)) {
    LOG_ERROR("av_read_frame error!");

    // TODO AVERROR_EOF?
    return GANG_FITAL;
  }

  err = find_fs_index(&fs_index, dec->fscs, dec->fsc_size, dec->i_pkt.stream_index);
  if (err < 0) {
    return GANG_ERROR_DATA;
  }

  fsc = dec->fscs[fs_index];
  is  = fsc.is;

  av_packet_rescale_ts(&dec->i_pkt, is->time_base, is->codec->time_base);
  av_frame_unref(dec->i_frame);
  dec_func = fsc.is_video ? avcodec_decode_video2 : avcodec_decode_audio4;
  err      = dec_func(is->codec, dec->i_frame, &got_frame, &dec->i_pkt);

  if (err < 0) {
    LOG_INFO("Decode failed");
    return GANG_ERROR_DATA;
  }

  if (got_frame) {
    dec->i_frame->pts = av_frame_get_best_effort_timestamp(dec->i_frame);
    err               = filter_encode_write_frame(dec, &fsc, 1);

    if (err < 0) return GANG_FITAL;
    if (fsc.is_video && dec->video_buff) return GANG_VIDEO_DATA;
    if (!fsc.is_video && dec->audio_buff) return GANG_AUDIO_DATA;
    LOG_DEBUG("Unexpected type");
  }

  return GANG_ERROR_DATA;
}

int flush_gang_rec_encoder(gang_decoder* dec) {
  int i;
  int ret = 0;

  if (!dec->ofmt_ctx || !dec->recording) {
    return ret;
  }

  /* flush filters and encoders */
  for (i = 0; i < dec->fsc_size; i++) {
    /* flush filter */
    if (!dec->fscs[i].filter_graph) {
      continue;
    }

    ret = filter_encode_write_frame(dec, &dec->fscs[i], 0);
    if (ret < 0) {
      LOG_INFO("Flushing filter failed");
      break;
    }

    /* flush encoder */
    ret = flush_encoder(dec, &dec->fscs[i]);
    if (ret < 0) {
      LOG_INFO("Flushing encoder failed");
      break;
    }
  }

  dec->waitkey   = 1;
  ret            = av_write_trailer(dec->ofmt_ctx);
  dec->recording = 0;
  if (ret) LOG_ERROR("Error occurred when trail output file");
  return ret;
}
