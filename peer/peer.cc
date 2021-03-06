/*
 * Copyright 2015, empirefox
 * Copyright 2012, Google Inc.
 */
#include "peer.h"

#include "fakeconstraints.h"

#include "shared.h"
#include "one_spdlog_console.h"
#include "cgo.h"

namespace one {
using std::string;
using webrtc::SessionDescriptionInterface;

const char kCameraId[] = "camera";

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[]        = "id";
const char kCandidateSdpMlineIndexName[] = "label";
const char kCandidateSdpName[]           = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[]  = "sdp";

class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
  static DummySetSessionDescriptionObserver* Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }

  virtual void OnSuccess() {
    SPDLOG_TRACE(console, "{}", __func__)
  }

  virtual void OnFailure(const string& error) {
    console->error("Failed to set sdp, ({})", error);
  }

protected:
  DummySetSessionDescriptionObserver() {}

  ~DummySetSessionDescriptionObserver() {}
};

Peer::Peer(const string& id, Shared* shared, void* goPcPtr) :
  id_(id),
  shared_(shared),
  goPcPtr_(goPcPtr) {
  SPDLOG_TRACE(console, "{}",                      __func__)
  factory_ = shared->GetPeerConnectionFactory(id);
  SPDLOG_TRACE(console, "{} factory use_count:{}", __func__, factory_.use_count())
}

Peer::~Peer() {
  SPDLOG_TRACE(console, "{} factory use_count:{}", __func__, factory_.use_count())
  if (factory_.get()) {
    factory_->RemoveOnePeerConnection();
  }
  Close();
  if (factory_.get()) {
    factory_ = NULL;
  }
  goPcPtr_ = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

// public
void Peer::CreateAnswer(string& sdp) {
  SPDLOG_TRACE(console, "{}", __func__)
  if (!CreatePeerConnection()) {
    console->error() << "Failed to initialize our PeerConnection instance";
    return;
  }

  webrtc::SdpParseError        error;
  SessionDescriptionInterface* session_description(
    webrtc::CreateSessionDescription(SessionDescriptionInterface::kOffer, sdp, &error));
  if (!session_description) {
    console->error() << "Can't parse received session description message. "
                     << "SdpParseError was: " << error.description;
    return;
  }
  peer_connection_->SetRemoteDescription(
    DummySetSessionDescriptionObserver::Create(),
    session_description);

  peer_connection_->CreateAnswer(this, NULL);
}

void Peer::AddCandidate(webrtc::IceCandidateInterface* candidate) {
  SPDLOG_TRACE(console, "{}", __func__)
  if (!peer_connection_->AddIceCandidate(candidate)) {
    console->error() << "Failed to apply the received candidate";
    return;
  }
}

bool Peer::connection_active() const {
  return peer_connection_.get() != NULL;
}

void Peer::Close() {
  SPDLOG_TRACE(console, "{}",    __func__)
  peer_connection_ = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

Shared* Peer::GetShared() {
  return shared_;
}

// end of public

// "rtsp://218.204.223.237:554/live/1/0547424F573B085C/gsfp90ef4k0a6iap.sdp"
bool Peer::CreatePeerConnection() {
  SPDLOG_TRACE(console, "{}", __func__)

  // TODO bind to target
  if (!factory_.get()) {
    console->error() << "Failed to initialize PeerConnectionFactory";
    return false;
  }

  peer_connection_ = factory_->CreatePeerConnection(this);
  if (!peer_connection_.get()) {
    Close();
    console->error() << "CreatePeerConnection failed";
  }

  return peer_connection_.get() != NULL;
}

//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void Peer::OnAddStream(webrtc::MediaStreamInterface* stream) {
  SPDLOG_TRACE(console, "{}", __func__)
}

void Peer::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
  SPDLOG_TRACE(console, "{}", __func__)
}

void Peer::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  SPDLOG_TRACE(console, "{}", __func__)

  Json::StyledWriter writer;
  Json::Value jmessage;

  jmessage[kSessionDescriptionTypeName] = "candidate";
  jmessage[kCandidateSdpMidName]        = candidate->sdp_mid();
  jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
  string sdp;
  if (!candidate->ToString(&sdp)) {
    return;
  }
  jmessage[kCandidateSdpName] = sdp;
  SendMessage(writer, jmessage);
}

void Peer::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  SPDLOG_TRACE(console, "{}", __func__)
  peer_connection_->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);

  string sdp;
  desc->ToString(&sdp);

  Json::StyledWriter writer;
  Json::Value        jmessage;
  jmessage[kSessionDescriptionTypeName] = desc->type();
  jmessage[kSessionDescriptionSdpName]  = sdp;
  SendMessage(writer, jmessage);
}

void Peer::OnFailure(const string& error) {
  console->error("Failed to create sdp ({})", error);
}

void Peer::SendMessage(Json::StyledWriter& writer, Json::Value& jmessage) {
  SPDLOG_TRACE(console, "{}", __func__)
  jmessage[kCameraId] = id_;
  go_send_to_peer(goPcPtr_, const_cast<char*>(writer.write(jmessage).c_str()));
}

// TODO remove
void Peer::OnMessage(rtc::Message* msg) {
  switch (msg->message_id) {
    case RemoteOfferSignal: {
      RemoteOfferMsgData* offer_data = static_cast<RemoteOfferMsgData*>(msg->pdata);
      CreateAnswer(offer_data->data());
      break;
    }

    case RemoteCandidateSignal: {
      rtc::scoped_ptr<IceCandidateMsgData> ice_data(
        static_cast<IceCandidateMsgData*>(msg->pdata));
      AddCandidate(ice_data->data().get());
      break;
    }
  }
}
} // namespace one

// used by go
