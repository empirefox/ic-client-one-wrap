#pragma once

#include <memory>
#include "webrtc/base/basictypes.h"

#include "param_types.h"
#include "gang_dec.h"

namespace gang {
class GangDecoderInterface {
public:
  virtual ~GangDecoderInterface() {}

  virtual bool Init()                         = 0;
  virtual int  NextFrame()                    = 0;
  virtual bool Open()                         = 0;
  virtual void Close()                        = 0;
  virtual bool IsVideoAvailable()             = 0;
  virtual bool IsAudioAvailable()             = 0;
  virtual bool IsRecEnabled()                 = 0;
  virtual void SetRecEnabled(bool enabled)    = 0;
  virtual void SetVideoBuff(uint8_t* buff)    = 0;
  virtual void SetAudioBuff(uint8_t* buff)    = 0;
  virtual void GetAvInfo(ipcam_av_info* info) = 0;
  virtual void GetVideoInfo(int*    width,
                            int*    height,
                            int*    fps,
                            uint32* buf_size) = 0;
  virtual void GetAudioInfo(uint32_t* sample_rate,
                            uint8_t*  channels) = 0;
};

class GangDecoderFactoryInterface {
public:
  virtual std::shared_ptr<GangDecoderInterface> CreateGangDecoder(ipcam_info* info) = 0;
  virtual ~GangDecoderFactoryInterface() {}
};
}
