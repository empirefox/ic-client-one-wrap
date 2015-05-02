/*
 * Copyright 2015, empirefox
 * Copyright 2014, Salman Aljammaz
 * Copyright 2012, Google Inc.
 */

#ifndef PEER_H_
#define PEER_H_
#pragma once

#include <memory>
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "shared.h"

namespace one {

class Peer: public webrtc::PeerConnectionObserver,
		public webrtc::CreateSessionDescriptionObserver {
public:
	Peer(const std::string url, Shared* shared, void* goPcPtr);
	~Peer();
	//
	// will export
	//
	void CreateAnswer(std::string offerSDP_sent_from_offerer);
	void AddCandidate(std::string sdp, std::string mid, int line);

	bool connection_active() const;
	// close conn to server
	virtual void Close();

	static rtc::Thread* signaling_thread_;

protected:
	bool CreatePeerConnection();

	//
	// PeerConnectionObserver implementation.
	//
	virtual void OnStateChange(
			webrtc::PeerConnectionObserver::StateType state_changed) {
	}
	virtual void OnAddStream(webrtc::MediaStreamInterface* stream);
	virtual void OnRemoveStream(webrtc::MediaStreamInterface* stream);
	virtual void OnDataChannel(webrtc::DataChannelInterface* channel) {
	}
	virtual void OnRenegotiationNeeded() {
	}
	virtual void OnIceChange() {
	}
	virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);

	//
	// CreateSessionDescriptionObserver implementation.
	//
	virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
	virtual void OnFailure(const std::string& error);

	//
	// TODO idk
	//
	virtual int AddRef() {
		return 0;
	} // We manage own memory.
	virtual int Release() {
		return 0;
	}

	// TODO should select ws
	// use GO FUNC to send a message to the remote peer.
	// or should use callback to send without json.
	virtual void SendMessage(const std::string& json_object);

	// fields
	std::string url_;
	Shared* shared_;
	void* goPcPtr_;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
};

} // namespace one

#endif // PEER_H_
