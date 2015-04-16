/*
 * Copyright 2015, empirefox
 * Copyright 2014, Salman Aljammaz
 * Copyright 2012, Google Inc.
 */

#ifndef PEERCONN_HH_
#define PEERCONN_HH_
#pragma once

#include "talk/app/webrtc/peerconnectioninterface.h"

namespace webrtc {
class VideoCaptureModule;
} // namespace webrtc

class Conductor: public webrtc::PeerConnectionObserver,
		public webrtc::CreateSessionDescriptionObserver {
public:
	Conductor();
	//
	// will export
	//
	void CreateAnswer(std::string& offerSDP_sent_from_offerer);
	void AddCandidate(std::string& sdp, std::string& mid, int line);
	void AddICE(std::string& uri, std::string& name, std::string& psd);

	bool connection_active() const;
	// close conn to server
	virtual void Close();

protected:
	~Conductor();
	bool InitializePeerConnection();
	bool ReinitializePeerConnectionForLoopback();
	bool CreatePeerConnection(bool dtls);
	void DeletePeerConnection();
	void AddStreams();
	cricket::VideoCapturer* OpenVideoCaptureDevice();

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

	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
	std::map<std::string, rtc::scoped_refptr<webrtc::MediaStreamInterface> > active_streams_;
	webrtc::PeerConnectionInterface::IceServers iceServers_;
	rtc::Thread* signaling_thread_;
	rtc::Thread* worker_thread_;
};

#endif // PEERCONN_HH_
