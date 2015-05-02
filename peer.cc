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
		SPDLOG_TRACE(console);
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

Peer::Peer(const string url, Shared* shared, void* goPcPtr) :
				url_(url),
				shared_(shared),
				goPcPtr_(goPcPtr) {
	SPDLOG_TRACE(console);
}

Peer::~Peer() {
	SPDLOG_TRACE(console);
	Close();
	goPcPtr_ = NULL;
}

// public
void Peer::CreateAnswer(string& sdp) {
	SPDLOG_TRACE(console);
	if (!CreatePeerConnection()) {
		console->error() << "Failed to initialize our PeerConnection instance";
		return;
	}

	webrtc::SessionDescriptionInterface* session_description(
			webrtc::CreateSessionDescription(
					webrtc::SessionDescriptionInterface::kOffer,
					sdp));
	if (!session_description) {
		console->error() << "Can't parse received session description message.";
		return;
	}
	peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(),
			session_description);
	SPDLOG_DEBUG(console,"SetRemoteDescription ok");

	peer_connection_->CreateAnswer(this, NULL);
}

void Peer::AddCandidate(webrtc::IceCandidateInterface* candidate) {
	SPDLOG_TRACE(console);
	if (!peer_connection_->AddIceCandidate(candidate)) {
		console->error() << "Failed to apply the received candidate";
		return;
	}SPDLOG_DEBUG(console,"AddIceCandidate ok");
	return;
}

bool Peer::connection_active() const {
	return peer_connection_.get() != NULL;
}

void Peer::Close() {
	// close ws
	peer_connection_ = NULL;
}

Shared* Peer::GetShared() {
	return shared_;
}
// end of public

// "rtsp://218.204.223.237:554/live/1/0547424F573B085C/gsfp90ef4k0a6iap.sdp"
bool Peer::CreatePeerConnection() {
	SPDLOG_TRACE(console);
	std::shared_ptr<one::ComposedPeerConnectionFactory> factory =
			shared_->GetPeerConnectionFactory(url_);
	// TODO bind to target
	if (!factory || !factory.get()) {
		console->error() << "Failed to initialize PeerConnectionFactory";
		return false;
	}

	peer_connection_ = factory->CreatePeerConnection(this);
	if (!peer_connection_.get()) {
		Close();
		console->error() << "CreatePeerConnection failed";
	}SPDLOG_DEBUG(console,"CreatePeerConnection ok");

	return peer_connection_.get() != NULL;
}

//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void Peer::OnAddStream(webrtc::MediaStreamInterface* stream) {
	SPDLOG_TRACE(console);
	stream->AddRef();
}

void Peer::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
	SPDLOG_TRACE(console);
	stream->AddRef();
}

void Peer::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
	SPDLOG_TRACE(console);

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
	SPDLOG_TRACE(console);
	peer_connection_->SetLocalDescription(
			DummySetSessionDescriptionObserver::Create(),
			desc);
	SPDLOG_DEBUG(console,"SetLocalDescription ok");

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
	SPDLOG_TRACE(console);
	go_send_to_peer(goPcPtr_, const_cast<char*>(json_object.c_str()));
}

void Peer::OnMessage(rtc::Message* msg) {
	switch (msg->message_id) {
	case RemoteOfferSignal: {
		RemoteOfferMsgData* offer_data =
				static_cast<RemoteOfferMsgData*>(msg->pdata);
		CreateAnswer(offer_data->data());
		break;
	}
	case RemoteCandidate: {
		rtc::scoped_ptr<IceCandidateMsgData> ice_data(
				static_cast<IceCandidateMsgData*>(msg->pdata));
		AddCandidate(ice_data->data().get());
		break;
	}
	}
}

} // namespace one

// used by go

