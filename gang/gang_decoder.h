#pragma once

#include "gang_dec.h"
#include "gang_decoder_interface.h"

namespace gang {
class GangDecoder : public GangDecoderInterface {
public:
  explicit GangDecoder(gang_decoder* decoder);
  virtual ~GangDecoder();
  virtual bool Init();
  virtual int  NextFrame();
  virtual bool Open();
  virtual void Close();
  virtual bool IsVideoAvailable();
  virtual bool IsAudioAvailable();
  virtual bool IsRecEnabled();
  virtual void SetRecEnabled(bool enabled);
  virtual void SetVideoBuff(uint8_t* buff);
  virtual void SetAudioBuff(uint8_t* buff);
  virtual void GetAvInfo(ipcam_av_info* info);
  virtual void GetVideoInfo(int*    width,
                            int*    height,
                            int*    fps,
                            uint32* buf_size);
  virtual void GetAudioInfo(uint32_t* sample_rate,
                            uint8_t*  channels);

private:
  gang_decoder* decoder_;
};

class GangDecoderFactory : public GangDecoderFactoryInterface {
public:
  virtual std::shared_ptr<GangDecoderInterface> CreateGangDecoder(ipcam_info* info);
};
}
