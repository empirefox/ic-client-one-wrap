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

void* Init(void* goConductorPtr) {
	one::InitOneSpdlogConsole();
	gang::InitializeGangDecoderGlobel();
	rtc::InitializeSSL();
	return reinterpret_cast<void*>(new Shared(goConductorPtr, DTLS_ON));
}

void Release(void* sharedPtr) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	if (sharedPtr) {
		delete reinterpret_cast<Shared*>(sharedPtr);
		SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "deleted")
	}
	rtc::CleanupSSL();
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "CleanupSSL ok")
	gang::CleanupGangDecoderGlobel();
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "CleanupGangDecoderGlobel ok")
	one::CleanupOneSpdlog();
	std::cout << "Release ok" << std::endl;
}

void AddICE(void* sharedPtr, char *uri, char *name, char *psd) {
	reinterpret_cast<Shared*>(sharedPtr)->AddIceServer(string(uri), string(name), string(psd));
}

int RegistryCam(
		void* sharedPtr,
		char *id,
		char *url,
		char *rec_name,
		int rec_enabled,
		int audio_off) {
	Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
	string cid = string(id);
	string curl = string(url);
	string crec_name = string(rec_name);
	return shared->AddPeerConnectionFactory(cid, curl, crec_name, rec_enabled, audio_off);
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
	return reinterpret_cast<void*>(shared->CreatePeer(string(url), goPcPtr));
}

void DeletePeer(void* pc) {
	if (pc) {
		Peer *cpc = reinterpret_cast<Peer*>(pc);
		cpc->GetShared()->DeletePeer(cpc);
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

	webrtc::SdpParseError error;
	rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate(
			webrtc::CreateIceCandidate(cmid, line, csdp, &error));
	if (!candidate.get()) {
		one::console->error() << "Can't parse received candidate message. " << "SdpParseError was: "
				<< error.description;
		return;
	}

	cpc->GetShared()->SignalingThread->Post(
			cpc,
			one::RemoteCandidateSignal,
			new one::IceCandidateMsgData(candidate.release()));
}

