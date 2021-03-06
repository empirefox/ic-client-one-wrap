#include "composed_pc_factory.h"

// #include "webrtc/base/platform_thread.h"
#include "webrtc/base/bind.h"
#include "talk/app/webrtc/peerconnectionfactory.h"
#include "talk/app/webrtc/peerconnectionfactoryproxy.h"
#include "one_spdlog_console.h"
#include "shared.h"

namespace one {
using rtc::Bind;
using gang::GangVideoCapturer;

const char kAudioLabel[]  = "audio_label";
const char kVideoLabel[]  = "video_label";
const char kStreamLabel[] = "stream_label";

ComposedPCFactory::ComposedPCFactory(
  Shared*                                 shared,
  const string&                           id,
  ipcam_info*                             info,
  shared_ptr<GangDecoderFactoryInterface> dec_factory) :
  id_(id),
  worker_thread_(new Thread),
  shared_(shared),
  gang_(NULL),
  peers_(0) {
  if (!worker_thread_->Start()) {
    console->error() << "worker_thread_ failed to start";
  }

  //	rtc::PlatformThreadRef current_thread = rtc::CurrentThreadRef();
  gang_ = std::make_shared<Gang>(id, info, worker_thread_, shared, dec_factory);
  SPDLOG_TRACE(console, "{}", __func__)
}

ComposedPCFactory::~ComposedPCFactory() {
  SPDLOG_TRACE(console, "{}", __func__)
  if (gang_.get()) {
    gang_->Shutdown();
  }
  gang_ = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "gang_=NULL ok")
  releaseFactory();

  // TODO need stop?
  worker_thread_->Stop();
  delete worker_thread_;
  worker_thread_ = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

scoped_refptr<PeerConnectionInterface> ComposedPCFactory::CreatePeerConnection(PeerConnectionObserver* observer) {
  SPDLOG_TRACE(console, "{}", __func__)
  rtc::CritScope cs(&lock_);

  if (!factory_.get() && !CreateFactory()) {
    return NULL;
  }

  auto peer_connection_ = factory_->CreatePeerConnection(
    shared_->IceServers,
    &shared_->Constraints,
    NULL,
    NULL,
    observer);

  if (!peer_connection_->AddStream(stream_)) {
    console->error("Adding stream to PeerConnection failed ({})", id_);
  }
  ++peers_;
  SPDLOG_TRACE(console, "{} {} {}", __func__, "++peers=", peers_)

  return peer_connection_;
}

void ComposedPCFactory::RemoveOnePeerConnection() {
  rtc::CritScope cs(&lock_);

  --peers_;
  if (!peers_) {
    releaseFactory();
  }
  SPDLOG_TRACE(console, "{} {} {}", __func__, "--peers=", peers_)
}

void ComposedPCFactory::releaseFactory() {
  stream_ = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "stream_ ok")
  factory_ = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "factory_ ok")
}

bool ComposedPCFactory::Init(ipcam_av_info* info) {
  SPDLOG_TRACE(console, "{}", __func__)
  if (!gang_->Init()) {
    return false;
  }

  gang_->GetAvInfo(info);
  gang_->StartRec();
  return true;
}

bool ComposedPCFactory::CreateFactory() {
  // 1. Create GangAudioDevice
  scoped_refptr<GangAudioDevice> audio(NULL);
  if (gang_->IsAudioAvailable()) {
    audio = GangAudioDevice::Create(gang_);
    if (!audio.get()) {
      return false;
    }
    SPDLOG_TRACE(console, "{}: {}", __func__, "GangAudioDevice Initialized.")
  }

  // 2. Create PeerConnectionFactory
  factory_ = webrtc::CreatePeerConnectionFactory(worker_thread_, shared_->SignalingThread, audio, NULL, NULL);
  if (!factory_.get()) {
    return false;
  }
  SPDLOG_TRACE(console, "{}: {}", __func__, "PeerConnectionFactory created.")

  // 3. Create VideoSource
  VideoCapturer * video = NULL;
  if (gang_->IsVideoAvailable()) {
    video = shared_->SignalingThread->Invoke<VideoCapturer*>(Bind(&gang::CreateVideoCapturer, gang_, worker_thread_));
    if (!video) {
      return false;
    }
    SPDLOG_TRACE(console, "{}: {}", __func__, "GangVideoCapturer Initialized.")
  }

  // 4. Check if video or audio exist
  if (!audio.get() && !video) {
    console->error("{} Failed to get media from ({})", __func__, id_);
    return false;
  }

  // 4. Create stream
  stream_ = factory_->CreateLocalMediaStream(kStreamLabel);
  if (gang_->IsAudioAvailable()) {
    if (!stream_->AddTrack(factory_->CreateAudioTrack(kAudioLabel, factory_->CreateAudioSource(NULL)))) {
      console->error("{} Failed to add audio track ({})", __func__, id_);
    }
  }
  if (gang_->IsVideoAvailable()) {
    if (!stream_->AddTrack(factory_->CreateVideoTrack(kVideoLabel, factory_->CreateVideoSource(video, NULL)))) {
      // after one delete, exist count 3 -> 2, created count 2 -> 1(delete)
      console->error("{} Failed to add video track ({})", __func__, id_);
    }
  }

  SPDLOG_TRACE(console, "{}: {}", __func__, "OK")
  return true;
}

void ComposedPCFactory::SetRecordEnabled(bool enabled) {
  if (gang_) {
    gang_->SetRecordEnabled(enabled);
  }
}
} // namespace one
