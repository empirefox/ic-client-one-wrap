#pragma once

#include "gang_decoder.h"

namespace gang {
class FakeGangDecoder : public GangDecoder {
public:
  explicit FakeGangDecoder(gang_decoder* dec) : GangDecoder(dec) {}

  virtual bool Init() {
    dec_->no_video        = 0;
    dec_->width           = 256;
    dec_->height          = 256;
    dec_->fps             = 30;
    dec_->video_buff_size = av_image_get_buffer_size(dec_->pix_fmt, dec_->width, dec_->height, 1);
    return true;
  }

  virtual int  NextFrame() {return GANG_VIDEO_DATA;}

  virtual bool Open()      {return true;}

  virtual void Close()     {}

private:
  gang_decoder* dec_;
};

class FakeGangDecoderFactory : public GangDecoderFactoryInterface {
public:
  virtual std::shared_ptr<GangDecoderInterface> CreateGangDecoder(ipcam_info* info) {
    gang_decoder* decoder = ::new_gang_decoder(info->url, info->rec_name, info->rec_on, info->audio_off);

    if (!decoder) {
      return NULL;
    }
    std::shared_ptr<GangDecoderInterface> gang(new FakeGangDecoder(decoder));
    if (!gang.get() || !gang->Init()) {
      return NULL;
    }
    return gang;
  }
};
}
