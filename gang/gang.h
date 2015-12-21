#pragma once

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/thread.h"
#include "talk/media/base/videocapturer.h"

#include "gang_decoder.h"

namespace gang {
using std::string;
using rtc::Thread;

typedef enum GangStatus {
  Alive, Dead
} GangStatus;

// Thread safe is the user's responsibility
class StatusObserver {
public:
  virtual void OnStatusChange(const string& id,
                              GangStatus    status) = 0;
  virtual ~StatusObserver() {}
};

// need be shared_ptr
class GangFrameObserver {
public:
  // see "talk/media/webrtc/webrtcvideoframe.h"
  virtual void OnGangFrame() = 0;
  virtual void OnVideoStarted(bool success) {}

  virtual void OnVideoStopped()             {}

  virtual ~GangFrameObserver() {}
};

class Observer {
public:
  Observer(GangFrameObserver* _observer, uint8_t* _buff) :
    observer(_observer),
    buff(_buff) {}

  GangFrameObserver* observer;
  uint8_t*           buff;
};

typedef rtc::ScopedMessageData<Observer> ObserverMsgData;
typedef rtc::TypedMessageData<bool>      RecOnMsgData;

class Gang {
public:
  enum {NEXT, REC_ON, START_REC, SHUTDOWN, VIDEO_START, VIDEO_STOP, AUDIO_OBSERVER};

  explicit Gang(const string&                                id,
                ipcam_info*                                  info,
                Thread*                                      worker_thread,
                StatusObserver*                              status_observer,
                std::shared_ptr<GangDecoderFactoryInterface> dec_factory);

  ~Gang();

  bool Init();

  // only in worker thread
  bool Start();
  void Stop(bool force);

  bool IsVideoAvailable();
  bool IsAudioAvailable();

  void StartVideoCapture(GangFrameObserver* observer,
                         uint8_t*           buff);
  void StopVideoCapture(GangFrameObserver* observer);
  void SetAudioFrameObserver(GangFrameObserver* observer,
                             uint8_t*           buff);

  void GetAvInfo(ipcam_av_info* info);
  void GetVideoInfo(int*    width,
                    int*    height,
                    int*    fps,
                    uint32* buf_size);
  void GetAudioInfo(uint32_t* sample_rate,
                    uint8_t*  channels);

  void SetRecordEnabled(bool enabled);
  void SendStatus(GangStatus status);

  // these can be called outside gang thread.
  // but only at beginning or end.
  void StartRec();
  void Shutdown();

protected:
  void stop();
  bool NextFrameLoop();
  void SetRecOn(bool enabled);

  // only in worker thread
  void StartVideoCapture_g(GangFrameObserver* observer,
                           uint8_t*           buff);
  void StopVideoCapture_g(GangFrameObserver* observer);
  void SetAudioObserver_g(GangFrameObserver* observer,
                          uint8_t*           buff);

  bool connected_;

private:
  class GangThread; // Forward declaration, defined in .cc.

  const string       id_;
  GangThread*        gang_thread_;
  Thread*            worker_thread_;
  GangFrameObserver* video_frame_observer_;
  GangFrameObserver* audio_frame_observer_;
  StatusObserver*    status_observer_;

  std::shared_ptr<GangDecoderFactoryInterface> dec_factory_;
  std::shared_ptr<GangDecoderInterface>        dec_;

  mutable rtc::CriticalSection crit_;

  RTC_DISALLOW_COPY_AND_ASSIGN(Gang);
};
}

// namespace gang
