/*
 * peer_wrap.cc
 *
 *  Created on: May 1, 2015
 *      Author: savage
 */
#include "peer.h"

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/bind.h"

#include "gang_init_deps.h"
#include "one_spdlog_console.h"

extern "C" {
#include "peer_wrap.h"
}

#define DTLS_ON  true
#define DTLS_OFF false

using std::string;
using one::Peer;
using one::Shared;
using one::console;

void* Init() {
	one::InitOneSpdlogConsole();
	gang::InitializeGangDecoderGlobel();
	rtc::InitializeSSL();
	return reinterpret_cast<void*>(new Shared(DTLS_ON));
}

void Release(void* sharedPtr) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
	if (shared) {
		delete shared;
	}
	rtc::CleanupSSL();
	gang::CleanupGangDecoderGlobel();
	one::CleanupOneSpdlog();
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
}

void AddICE(void* sharedPtr, char *uri, char *name, char *psd) {
	reinterpret_cast<Shared*>(sharedPtr)->AddIceServer(string(uri), string(name), string(psd));
}

int RegistryUrl(void* sharedPtr, char *url, char *rec_name, int rec_enabled) {
	Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
	string curl = string(url);
	string crec_name = string(rec_name);
	return shared->AddPeerConnectionFactory(curl, crec_name, rec_enabled);
}

void SetRecordEnabled(void* sharedPtr, char *url, int rec_enabled) {
	Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
	string curl = string(url);
	auto factory = shared->GetPeerConnectionFactory(curl);
	if (!factory) {
		return;
	}
	factory->SetRecordEnabled(rec_enabled);
}

void* CreatePeer(char *url, void* sharedPtr, void* goPcPtr) {
	Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
	return shared->CreatePeer(string(url), goPcPtr);
}

void DeletePeer(void* pc) {
	Peer *cpc = reinterpret_cast<Peer*>(pc);
	if (cpc) {
		delete cpc;
	}
}

void CreateAnswer(void* pc, char* sdp) {
	Peer *cpc = reinterpret_cast<Peer*>(pc);
	string csdp = string(sdp);
	cpc->GetShared()->SignalingThread->Post(
			cpc,
			one::RemoteOfferSignal,
			new one::RemoteOfferMsgData(csdp));
}

void AddCandidate(void* pc, char* sdp, char* mid, int line) {
	Peer *cpc = reinterpret_cast<Peer*>(pc);
	string csdp = string(sdp);
	string cmid = string(mid);
	rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate(
			webrtc::CreateIceCandidate(cmid, line, csdp));

	if (!candidate.get()) {
		one::console->error() << "Can't parse received candidate message.";
		return;
	}

	cpc->GetShared()->SignalingThread->Post(
			cpc,
			one::RemoteCandidateSignal,
			new one::IceCandidateMsgData(candidate.release()));
}

