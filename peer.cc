/*
 * Copyright 2015, empirefox
 * Copyright 2012, Google Inc.
 */
#include "peer.h"

#include "webrtc/base/json.h"
#include "fakeconstraints.h"

#include "shared.h"
#include "one_spdlog_console.h"
#include "cgo.h"

namespace one {

using std::string;

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

class DummySetSessionDescriptionObserver: public webrtc::SetSessionDescriptionObserver {
public:
	static DummySetSessionDescriptionObserver* Create() {
		return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() {
		SPDLOG_TRACE(console, "{}", __FUNCTION__)
	}
	virtual void OnFailure(const string& error) {
		console->error("Failed to set sdp, ({})", error);
	}

protected:
	DummySetSessionDescriptionObserver() {
	}
	~DummySetSessionDescriptionObserver() {
	}
};

Peer::Peer(const string& url, Shared* shared, void* goPcPtr) :
				url_(url),
				shared_(shared),
				goPcPtr_(goPcPtr) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	factory_ = shared->GetPeerConnectionFactory(url);
	SPDLOG_TRACE(console, "{} factory use_count:{}", __FUNCTION__, factory_.use_count())
}

Peer::~Peer() {
	SPDLOG_TRACE(console, "{} factory use_count:{}", __FUNCTION__, factory_.use_count())
	factory_->RemoveOnePeerConnection();
	factory_ = NULL;
	peer_connection_->Close();
	Close();
	goPcPtr_ = NULL;
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
}

// public
void Peer::CreateAnswer(string& sdp) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	if (!CreatePeerConnection()) {
		console->error() << "Failed to initialize our PeerConnection instance";
		return;
	}

	webrtc::SessionDescriptionInterface* session_description(
			webrtc::CreateSessionDescription(webrtc::SessionDescriptionInterface::kOffer, sdp));
	if (!session_description) {
		console->error() << "Can't parse received session description message.";
		return;
	}
	peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(),
			session_description);

	peer_connection_->CreateAnswer(this, NULL);
}

void Peer::AddCandidate(webrtc::IceCandidateInterface* candidate) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	if (!peer_connection_->AddIceCandidate(candidate)) {
		console->error() << "Failed to apply the received candidate";
		return;
	}
	return;
}

bool Peer::connection_active() const {
	return peer_connection_.get() != NULL;
}

void Peer::Close() {
	// close ws
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	peer_connection_ = NULL;
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "clear")
	shared_->SignalingThread->Clear(this);
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
}

Shared* Peer::GetShared() {
	return shared_;
}
// end of public

// "rtsp://218.204.223.237:554/live/1/0547424F573B085C/gsfp90ef4k0a6iap.sdp"
bool Peer::CreatePeerConnection() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
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
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	stream->AddRef();
}

void Peer::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	stream->AddRef();
}

void Peer::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)

	Json::StyledWriter writer;
	Json::Value jmessage;

	jmessage[kSessionDescriptionTypeName] = "candidate";
	jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
	jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
	string sdp;
	if (!candidate->ToString(&sdp)) {
		return;
	}
	jmessage[kCandidateSdpName] = sdp;
	SendMessage(writer.write(jmessage));
}

void Peer::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	peer_connection_->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);

	string sdp;
	desc->ToString(&sdp);

	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage[kSessionDescriptionTypeName] = desc->type();
	jmessage[kSessionDescriptionSdpName] = sdp;
	SendMessage(writer.write(jmessage));
}

void Peer::OnFailure(const string& error) {
	console->error("Failed to create sdp ({})", error);
}

void Peer::SendMessage(const string& json_object) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	go_send_to_peer(goPcPtr_, const_cast<char*>(json_object.c_str()));
}

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

