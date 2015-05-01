/*
 * shared.cc
 *
 *  Created on: May 1, 2015
 *      Author: savage
 */

#include "message.h"
#include "shared.h"

namespace one {

Shared::Shared(bool dtls) :
				SignalingThread(new Thread) {

	if (!SignalingThread->Start()) {
		Msg::Error("Failed to start signaling_thread_");
	}

	InitConstraintsOnce(dtls);
}

Shared::~Shared() {
	if (SignalingThread) {
		SignalingThread->Stop();
		delete SignalingThread;
		SignalingThread = NULL;
	}
}

void Shared::InitConstraintsOnce(bool dtls) {
	Constraints.SetMandatoryReceiveAudio(false);
	Constraints.SetMandatoryReceiveVideo(false);
	if (dtls) {
		Constraints.AddOptional(
				webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
				"true");
	} else {
		Constraints.AddOptional(
				webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
				"false");
	}
}

void Shared::AddIceServer(string uri, string name, string psd) {
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

// For ComposedPeerConnectionFactory
// Will be used in go
int Shared::AddPeerConnectionFactory(const std::string url) {
	shared_ptr<ComposedPeerConnectionFactory> factory(
			new ComposedPeerConnectionFactory(url, this));
	if (!factory.get()) {
		Msg::Error("Failed to create ComposedPeerConnectionFactory");
		return 0;
	}

	if (!factory->Init()) {
		Msg::Error("Failed to init ComposedPeerConnectionFactory");
		return 0;
	}

	factories_.insert(make_pair(url, factory));
	return 1;
}

std::shared_ptr<ComposedPeerConnectionFactory> Shared::GetPeerConnectionFactory(
		const string& url) {
	map<string, shared_ptr<ComposedPeerConnectionFactory> >::iterator iter =
			factories_.find(url);

	if (iter != factories_.end()) {
		return iter->second;
	}
	return NULL;
}

} // namespace one

