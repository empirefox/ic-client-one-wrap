/*
 * Copyright 2015, empirefox
 * Copyright 2012, Google Inc.
 */
#include "peer.h"

#include "webrtc/base/json.h"
#include "fakeconstraints.h"

#include "shared.h"
#include "defaults.h"
#include "message.h"
#include "fake_test.h"

#include "cgo.h"

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";
// TODO test rtsp
const char kTestUrl[] = "rtsp://127.0.0.1:1235/test1.sdp";

class DummySetSessionDescriptionObserver: public webrtc::SetSessionDescriptionObserver {
public:
	static DummySetSessionDescriptionObserver* Create() {
		return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() {
		Msg::Info("SetSessionDescriptionObserver.OnSuccess");
	}
	virtual void OnFailure(const std::string& error) {
		Msg::Error("DummySetSessionDescriptionObserver");
		Msg::Error(error);
	}

protected:
	DummySetSessionDescriptionObserver() {
	}
	~DummySetSessionDescriptionObserver() {
	}
};

Peer::Peer(const std::string url, one::Shared* shared, void* chanPtr) :
				url_(url),
				shared_(shared),
				msgChanPtr_(chanPtr) {
}

Peer::~Peer() {
	Close();
}

// public
void Peer::CreateAnswer(std::string sdp) {
	if (!CreatePeerConnection()) {
		Msg::Error("Failed to initialize our PeerConnection instance");
		return;
	}

	webrtc::SessionDescriptionInterface* session_description(
			webrtc::CreateSessionDescription(
					webrtc::SessionDescriptionInterface::kOffer,
					sdp));
	if (!session_description) {
		Msg::Error("Can't parse received session description message.");
		return;
	}
	peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(),
			session_description);
	Msg::Info("SetRemoteDescription");

	peer_connection_->CreateAnswer(this, NULL);
}

void Peer::AddCandidate(std::string sdp, std::string mid, int line) {
	Msg::Info("add a candidate");
	rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate(
			webrtc::CreateIceCandidate(mid, line, sdp));
	if (!candidate.get()) {
		Msg::Error("Can't parse received candidate message.");
		return;
	}
	if (!peer_connection_->AddIceCandidate(candidate.get())) {
		Msg::Error("Failed to apply the received candidate");
		return;
	}
	Msg::Info("added a candidate");
	return;
}

bool Peer::connection_active() const {
	return peer_connection_.get() != NULL;
}

void Peer::Close() {
	// close ws
	peer_connection_ = NULL;
	msgChanPtr_ = NULL;
}
// end of public

// "rtsp://218.204.223.237:554/live/1/0547424F573B085C/gsfp90ef4k0a6iap.sdp"
bool Peer::CreatePeerConnection() {
	std::shared_ptr<one::ComposedPeerConnectionFactory> factory =
			shared_->GetPeerConnectionFactory(url_);
	// TODO bind to target
	if (!factory || !factory.get()) {
		Msg::Error("Failed to initialize PeerConnectionFactory");
		return false;
	}

	peer_connection_ = factory->CreatePeerConnection(this);
	if (!peer_connection_.get()) {
		Close();
		Msg::Error("CreatePeerConnection failed");
	}
	Msg::Info("Created PeerConnection");

	return peer_connection_.get() != NULL;
}

//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void Peer::OnAddStream(webrtc::MediaStreamInterface* stream) {
	stream->AddRef();
}

void Peer::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
	stream->AddRef();
}

void Peer::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
	Msg::Info("OnIceCandidate ok");

	Json::StyledWriter writer;
	Json::Value jmessage;

	jmessage[kSessionDescriptionTypeName] = "candidate";
	jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
	jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
	std::string sdp;
	if (!candidate->ToString(&sdp)) {
		return;
	}
	jmessage[kCandidateSdpName] = sdp;
	SendMessage(writer.write(jmessage));
}

void Peer::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
	Msg::Info("answer created");
	peer_connection_->SetLocalDescription(
			DummySetSessionDescriptionObserver::Create(),
			desc);
	Msg::Info("SetLocalDescription");

	std::string sdp;
	desc->ToString(&sdp);

	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage[kSessionDescriptionTypeName] = desc->type();
	jmessage[kSessionDescriptionSdpName] = sdp;
	Msg::Info("send desc in Conductor::OnSuccess");
	SendMessage(writer.write(jmessage));
}

void Peer::OnFailure(const std::string& error) {
	Msg::Error("Conductor::OnFailure");
	Msg::Error(error);
}

void Peer::SendMessage(const std::string& json_object) {
	go_send_to_peer(msgChanPtr_, const_cast<char*>(json_object.c_str()));
}

// used by go

