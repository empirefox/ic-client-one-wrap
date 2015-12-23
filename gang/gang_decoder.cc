#include "gang_decoder.h"

#include "gang_decoder_impl.h"

namespace gang {
GangDecoder::GangDecoder(gang_decoder* decoder) :
  decoder_(decoder) {}

GangDecoder::~GangDecoder() {
  if (decoder_) {
    ::free_gang_decoder(decoder_);
    decoder_ = NULL;
  }
}

bool GangDecoder::Init()                      {return !::init_gang_av_info(decoder_);}

int  GangDecoder::NextFrame()                 {return ::gang_decode_next_frame(decoder_);}

bool GangDecoder::Open()                      {return !::open_gang_decoder(decoder_);}

void GangDecoder::Close()                     {::close_gang_decoder(decoder_);}

bool GangDecoder::IsVideoAvailable()          {return !decoder_->no_video;}

bool GangDecoder::IsAudioAvailable()          {return !decoder_->no_audio;}

bool GangDecoder::IsRecEnabled()              {return decoder_->rec_enabled;}

void GangDecoder::SetRecEnabled(bool enabled) {decoder_->rec_enabled = enabled;}

void GangDecoder::SetVideoBuff(uint8_t* buff) {decoder_->video_buff = buff;}

void GangDecoder::SetAudioBuff(uint8_t* buff) {decoder_->audio_buff = buff;}

void GangDecoder::GetAvInfo(ipcam_av_info* info) {
  info->width  = decoder_->width;
  info->height = decoder_->height;
  info->video  = !decoder_->no_video;
  info->audio  = !decoder_->no_audio;
}

void GangDecoder::GetVideoInfo(int*    width,
                               int*    height,
                               int*    fps,
                               uint32* buf_size) {
  *width    = decoder_->width;
  *height   = decoder_->height;
  *fps      = decoder_->fps;
  *buf_size = static_cast<uint32>(decoder_->video_buff_size);
}

void GangDecoder::GetAudioInfo(uint32_t* sample_rate, uint8_t* channels) {
  *sample_rate = static_cast<uint32_t>(decoder_->sample_rate);
  *channels    = static_cast<uint8_t>(decoder_->channels);
}

// Factory
std::shared_ptr<GangDecoderInterface> GangDecoderFactory::CreateGangDecoder(ipcam_info* info) {
  gang_decoder* decoder = ::new_gang_decoder(info->url, info->rec_name, info->rec_on, info->audio_off);

  if (!decoder) {
    return NULL;
  }
  std::shared_ptr<GangDecoderInterface> gang(new GangDecoder(decoder));
  if (!gang.get() || !gang->Init()) {
    return NULL;
  }
  return gang;
}
}
