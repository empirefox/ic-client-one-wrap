#include "shared.h"

#include "peer.h"
#include "one_spdlog_console.h"
#include "cgo.h"

namespace one {
using std::make_shared;

Shared::Shared(void* goConductorPtr, bool dtls) :
  MainThread(rtc::Thread::Current()),
  SignalingThread(new Thread),
  goConductorPtr_(goConductorPtr) {
  SPDLOG_TRACE(console, "{}", __func__)
  if (!SignalingThread->Start()) {
    console->error() << "Failed to start SignalingThread";
  }

  InitConstraintsOnce(dtls);
}

Shared::~Shared() {
  SPDLOG_TRACE(console, "{}", __func__)
  {
    rtc::CritScope cs(&factories_lock_);

    factories_.clear();
  }
  SPDLOG_TRACE(console, "{} {}", __func__, "clear")
  SignalingThread->Stop();
  SPDLOG_TRACE(console, "{} {}", __func__, "SignalingThread Quit")
  delete SignalingThread;
  SignalingThread = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

void Shared::OnStatusChange(const string& id, gang::GangStatus status) {
  if (goConductorPtr_) {
    ::go_on_gang_status(goConductorPtr_, const_cast<char*>(id.c_str()), status);
  }
}

Peer* Shared::CreatePeer(const string& id, void* goPcPtr) {
  rtc::CritScope cs(&peer_lock_);

  return new Peer(id, this, goPcPtr);
}

void Shared::DeletePeer(Peer* pc) {
  rtc::CritScope cs(&peer_lock_);

  delete pc;
}

void Shared::InitConstraintsOnce(bool dtls) {
  SPDLOG_TRACE(console, "{}", __func__)
  Constraints.SetMandatoryReceiveAudio(false);
  Constraints.SetMandatoryReceiveVideo(false);
  if (dtls) {
    Constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "true");
  } else {
    Constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "false");
  }
}

void Shared::AddIceServer(string uri, string name, string psd) {
  SPDLOG_TRACE(console, "{}", __func__)
  PeerConnectionInterface::IceServer server;
  server.uri = uri;
  if (!name.empty()) {
    server.username = name;
  }
  if (!psd.empty()) {
    server.password = psd;
  }
  IceServers.push_back(server);
}

void Shared::SetDecFactory(GangDecoderFactoryInterface* dec_factory) {
  dec_factory_.reset(dec_factory);
}

// For ComposedPeerConnectionFactory
// Will be used in go
bool Shared::AddPeerConnectionFactory(ipcam_av_info* av_info, const string& id, ipcam_info* info) {
  SPDLOG_TRACE(console, "{}", __func__)
  rtc::CritScope cs(&factories_lock_);

  auto factory = make_shared<ComposedPCFactory>(this, id, info, dec_factory_);
  if (!factory.get()) {
    console->error("Failed to create ComposedPeerConnectionFactory with {}", id);
    return false;
  }

  if (!factory->Init(av_info)) {
    console->error("Failed to init ComposedPeerConnectionFactory {}", id);
    return false;
  }

  SPDLOG_TRACE(console, "{} factory use_count:{}", __func__, factory.use_count())
  factories_.insert(make_pair(id, factory));
  SPDLOG_TRACE(console, "{} factory use_count:{}", __func__, factory.use_count())
  return true;
}

Factoty Shared::GetPeerConnectionFactory(const string& id) {
  rtc::CritScope cs(&factories_lock_);

  auto iter = factories_.find(id);

  if (iter != factories_.end()) {
    SPDLOG_TRACE(console, "{} found with {}", __func__, id);
    return iter->second;
  }
  SPDLOG_TRACE(console, "{} not found with {}", __func__, id);
  return NULL;
}
} // namespace one
