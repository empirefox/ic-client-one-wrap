/*
 * shared.cc
 *
 *  Created on: May 1, 2015
 *      Author: savage
 */

#include "shared.h"

#include "one_spdlog_console.h"

namespace one {

Shared::Shared(bool dtls) :
				SignalingThread(new Thread) {

	SPDLOG_TRACE(console);
	if (!SignalingThread->Start()) {
		console->error() << "Failed to start SignalingThread";
	}

	InitConstraintsOnce(dtls);
}

Shared::~Shared() {
	SPDLOG_TRACE(console);
	if (SignalingThread) {
		SignalingThread->Stop();
		delete SignalingThread;
		SignalingThread = NULL;
	}
}

void Shared::InitConstraintsOnce(bool dtls) {
	SPDLOG_TRACE(console);
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
	SPDLOG_TRACE(console);
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
	SPDLOG_TRACE(console);
	shared_ptr<ComposedPeerConnectionFactory> factory(
			new ComposedPeerConnectionFactory(url, this));
	if (!factory.get()) {
		console->error(
				"Failed to create ComposedPeerConnectionFactory with {}",
				url);
		return 0;
	}

	if (!factory->Init()) {
		console->error("Failed to init ComposedPeerConnectionFactory {}", url);
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
		SPDLOG_TRACE(console,"found with {}",url);
		return iter->second;
	}SPDLOG_TRACE(console,"not found with {}",url);
	return NULL;
}

} // namespace one

