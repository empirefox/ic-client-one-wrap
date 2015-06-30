/*
 * shared.cc
 *
 *  Created on: May 1, 2015
 *      Author: savage
 */

#include "shared.h"

#include "peer.h"
#include "one_spdlog_console.h"

namespace one {

using std::make_shared;

Shared::Shared(bool dtls) :
				SignalingThread(new Thread) {

	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	if (!SignalingThread->Start()) {
		console->error() << "Failed to start SignalingThread";
	}

	InitConstraintsOnce(dtls);
}

Shared::~Shared() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	{
		rtc::CritScope cs(&factories_lock_);
		factories_.clear();
	}
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "clear")
	SignalingThread->Stop();
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "SignalingThread Quit")
	delete SignalingThread;
	SignalingThread = NULL;
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
}

Peer* Shared::CreatePeer(const std::string url, void* goPcPtr) {
	rtc::CritScope cs(&peer_lock_);
	return new Peer(url, this, goPcPtr);
}

void Shared::DeletePeer(Peer* pc) {
	rtc::CritScope cs(&peer_lock_);
	delete pc;
}

void Shared::InitConstraintsOnce(bool dtls) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	Constraints.SetMandatoryReceiveAudio(false);
	Constraints.SetMandatoryReceiveVideo(false);
	if (dtls) {
		Constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "true");
	} else {
		Constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "false");
	}
}

void Shared::AddIceServer(string uri, string name, string psd) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
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
int Shared::AddPeerConnectionFactory(const string& url, const string& rec_name, bool rec_enabled) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	rtc::CritScope cs(&factories_lock_);
	auto factory = make_shared<ComposedPeerConnectionFactory>(this, url, rec_name, rec_enabled);
	if (!factory.get()) {
		console->error("Failed to create ComposedPeerConnectionFactory with {}", url);
		return 0;
	}

	if (!factory->Init()) {
		console->error("Failed to init ComposedPeerConnectionFactory {}", url);
		return 0;
	}

	SPDLOG_TRACE(console, "{} factory use_count:{}", __FUNCTION__, factory.use_count())
	factories_.insert(make_pair(url, factory));
	SPDLOG_TRACE(console, "{} factory use_count:{}", __FUNCTION__, factory.use_count())
	return 1;
}

Factoty Shared::GetPeerConnectionFactory(const string& url) {
	rtc::CritScope cs(&factories_lock_);
	auto iter = factories_.find(url);
	if (iter != factories_.end()) {
		SPDLOG_TRACE(console, "{} found with {}", __FUNCTION__, url);
		return iter->second;
	}
	SPDLOG_TRACE(console, "{} not found with {}", __FUNCTION__, url);
	return NULL;
}

} // namespace one

