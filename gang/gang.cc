#include "gang.h"

#include <memory>
#include "webrtc/base/bind.h"

#include "gang_spdlog_console.h"
#include "gang_decoder_impl.h"

namespace gang {
using rtc::Bind;

// Take from "talk/media/devices/yuvframescapturer.h"
class Gang::GangThread : public Thread, public rtc::MessageHandler {
public:
  explicit GangThread(Gang* gang) :
    gang_(gang),
    waiting_time_ms_(0),
    finished_(false) {}

  virtual ~GangThread() {
    SPDLOG_TRACE(console, "{}", __func__)
    Stop();
    SPDLOG_TRACE(console, "{} {}", __func__, "ok")
  }

  // Override virtual method of parent Thread. Context: Worker Thread.
  virtual void Run() {
    // Read the first frame and start the message pump. The pump runs until
    // Stop() is called externally or Quit() is called by OnMessage().
    if (gang_) {
      Thread::Run();
    }

    rtc::CritScope cs(&crit_);
    finished_ = true;
  }

  // Override virtual method of parent MessageHandler. Context: Worker Thread.
  virtual void OnMessage(rtc::Message* pmsg) {
    if (gang_) {
      switch (pmsg->message_id) {
        case NEXT:
          if (gang_->connected_ && gang_->NextFrameLoop()) {
            PostDelayed(waiting_time_ms_, this, NEXT);
          } else if (gang_->connected_) {
            gang_->Stop(true);
          }
          break;

        case REC_ON:
          SPDLOG_TRACE(console, "{} REC_ON", __func__)
          gang_->SetRecOn(static_cast<RecOnMsgData*>(pmsg->pdata)->data());
          break;

        case VIDEO_START: {
          SPDLOG_TRACE(console, "{} VIDEO_START", __func__)
          rtc::scoped_ptr<ObserverMsgData> data(static_cast<ObserverMsgData*>(pmsg->pdata));
          gang_->StartVideoCapture_g(data->data()->observer, data->data()->buff);
          break;
        }

        case VIDEO_STOP: {
          SPDLOG_TRACE(console, "{} VIDEO_STOP", __func__)
          rtc::scoped_ptr<ObserverMsgData> data(static_cast<ObserverMsgData*>(pmsg->pdata));
          gang_->StopVideoCapture_g(data->data()->observer);
          break;
        }

        case AUDIO_OBSERVER: {
          SPDLOG_TRACE(console, "{} AUDIO_OBSERVER", __func__)
          rtc::scoped_ptr<ObserverMsgData> data(static_cast<ObserverMsgData*>(pmsg->pdata));
          gang_->SetAudioObserver_g(data->data()->observer, data->data()->buff);
          break;
        }

        case START_REC:
          SPDLOG_TRACE(console, "{} START_REC", __func__)
          gang_->Start();
          break;

        case SHUTDOWN:
          SPDLOG_TRACE(console, "{} SHUTDOWN", __func__)
          if (gang_->connected_) gang_->stop();
          Clear(this);
          Quit();
          SPDLOG_TRACE(console, "{} SHUTDOWN ok", __func__)
          break;

        default:
          console->error("{} {}", __func__, "unexpected msg type");
          break;
      }
    } else {
      Quit();
    }
  }

  // Check if Run() is finished.
  bool Finished() const {
    rtc::CritScope cs(&crit_);

    return finished_;
  }

private:
  Gang*                        gang_;
  int                          waiting_time_ms_;
  bool                         finished_;
  mutable rtc::CriticalSection crit_;

  RTC_DISALLOW_COPY_AND_ASSIGN(GangThread);
};

Gang::Gang(const string&                                id,
           ipcam_info*                                  info,
           Thread*                                      worker_thread,
           StatusObserver*                              status_observer,
           std::shared_ptr<GangDecoderFactoryInterface> dec_factory) :
  connected_(false),
  id_(id),
  gang_thread_(new GangThread(this)),
  worker_thread_(worker_thread),
  video_frame_observer_(NULL),
  audio_frame_observer_(NULL),
  status_observer_(status_observer),
  dec_factory_(dec_factory) {
  if (!dec_factory_.get()) {
    dec_factory_.reset(new GangDecoderFactory);
  }
  dec_ = dec_factory_->CreateGangDecoder(info);
  gang_thread_->Start();
  SPDLOG_TRACE(console, "{}: ipcam id: {}", __func__, id)
}

Gang::~Gang() {
  SPDLOG_TRACE(console, "{}", __func__)
  if (gang_thread_) {
    gang_thread_->Post(gang_thread_, SHUTDOWN);
    delete gang_thread_;
    gang_thread_ = NULL;
  }
  dec_ = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

void Gang::Shutdown() {
  SPDLOG_TRACE(console, "{}", __func__)
  if (gang_thread_) {
    gang_thread_->Post(gang_thread_, SHUTDOWN);
    gang_thread_->Stop();
  }
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

bool Gang::Init() {
  if (!dec_.get() || !worker_thread_) {
    return false;
  }
  return dec_->Init();
}

bool Gang::IsVideoAvailable()                                       {return dec_->IsVideoAvailable();}

bool Gang::IsAudioAvailable()                                       {return dec_->IsAudioAvailable();}

void Gang::GetAvInfo(ipcam_av_info* info)                           {dec_->GetAvInfo(info);}

void Gang::GetVideoInfo(int* w, int* h, int* fps, uint32* buf_size) {dec_->GetVideoInfo(w, h, fps, buf_size);}

void Gang::GetAudioInfo(uint32_t* sample_rate, uint8_t* channels)   {dec_->GetAudioInfo(sample_rate, channels);}

void Gang::stop() {
  connected_ = false;
  dec_->Close();
  SPDLOG_TRACE(console, "{}: {}", __func__, "shutdown")
}

bool Gang::Start() {
  DCHECK(gang_thread_->IsCurrent());
  SPDLOG_TRACE(console, "{}", __func__)

  if (gang_thread_->Finished()) {
    return false;
  }

  if (!connected_) {
    connected_ = dec_->Open();
    SendStatus(connected_ ? Alive : Dead);
  }

  if (!connected_) {
    return false;
  }
  gang_thread_->Clear(gang_thread_, NEXT);
  gang_thread_->PostDelayed(0, gang_thread_, NEXT);
  return true;
}

void Gang::Stop(bool force) {
  DCHECK(gang_thread_->IsCurrent());
  SPDLOG_TRACE(console, "{}: {}", __func__, force)
  if (!force && dec_->IsRecEnabled()) {
    return;
  }
  stop();
  gang_thread_->Clear(gang_thread_, NEXT);
  SPDLOG_TRACE(console, "{}: {}", __func__, "ok")
}

// return true->continue, false->end
bool Gang::NextFrameLoop() {
  DCHECK(gang_thread_->IsCurrent());

  switch (dec_->NextFrame()) {
    case GANG_VIDEO_DATA:
      if (video_frame_observer_) {
        video_frame_observer_->OnGangFrame();
      }
      break;

    case GANG_AUDIO_DATA:
      if (audio_frame_observer_) {
        worker_thread_->Invoke<void>(Bind(&GangFrameObserver::OnGangFrame, audio_frame_observer_));
      }
      break;

    case GANG_FITAL: // end loop
      SPDLOG_TRACE(console, "{}: {}", __func__, "GANG_FITAL")
      SendStatus(Dead);
      return false;

    case GANG_ERROR_DATA: // ignore and next
      break;

    default:              // unexpected, so end loop
      console->error() << "Unknow ret code from decoder!";
      SendStatus(Dead);
      return false;
  }
  return true;
}

// Called by webrtc worker thread
void Gang::StartVideoCapture(GangFrameObserver* observer, uint8_t* buff) {
  DCHECK(observer);
  DCHECK(buff);
  gang_thread_->Post(gang_thread_, VIDEO_START, new ObserverMsgData(new Observer(observer, buff)));
}

void Gang::StartVideoCapture_g(GangFrameObserver* observer, uint8_t* buff) {
  DCHECK(gang_thread_->IsCurrent());
  video_frame_observer_ = observer;
  dec_->SetVideoBuff(buff);
  observer->OnVideoStarted(Start());
}

// Called by webrtc worker thread
void Gang::StopVideoCapture(GangFrameObserver* observer) {
  DCHECK(observer);
  gang_thread_->Post(gang_thread_, VIDEO_STOP, new ObserverMsgData(new Observer(observer, NULL)));
}

void Gang::StopVideoCapture_g(GangFrameObserver* observer) {
  DCHECK(gang_thread_->IsCurrent());
  video_frame_observer_ = NULL;
  dec_->SetVideoBuff(NULL);
  observer->OnVideoStopped();
  if (!audio_frame_observer_) {
    Stop(false);
  }
}

// Called by webrtc worker thread
void Gang::SetAudioFrameObserver(GangFrameObserver* observer, uint8_t* buff) {
  gang_thread_->Post(gang_thread_, AUDIO_OBSERVER, new ObserverMsgData(new Observer(observer, buff)));
}

void Gang::SetAudioObserver_g(GangFrameObserver* observer, uint8_t* buff) {
  DCHECK(gang_thread_->IsCurrent());
  audio_frame_observer_ = observer;
  dec_->SetAudioBuff(buff);

  if (observer) {
    Start();
    return;
  }
  if (!video_frame_observer_) {
    Stop(false);
  }
}

// TODO fix not to stop thread when toggle rec_enabled status.
// Do not call in the running thread
void Gang::SetRecordEnabled(bool enabled) {
  gang_thread_->Post(gang_thread_, REC_ON, new RecOnMsgData(enabled));
}

void Gang::SetRecOn(bool enabled) {
  DCHECK(gang_thread_->IsCurrent());

  if (dec_->IsRecEnabled() == enabled) {
    return;
  }
  Stop(true);
  dec_->SetRecEnabled(enabled);

  if (enabled || video_frame_observer_ || audio_frame_observer_) {
    SPDLOG_TRACE(console, "{} {}", __func__, "restart")
    Start();
  }
}

void Gang::SendStatus(GangStatus status) {
  if (status_observer_) {
    status_observer_->OnStatusChange(id_, status);
  }
}

void Gang::StartRec() {
  if (dec_->IsRecEnabled()) {
    gang_thread_->Post(gang_thread_, START_REC);
  }
}
} // namespace gang
