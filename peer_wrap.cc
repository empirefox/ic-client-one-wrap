/*
 * peer_wrap.cc
 *
 *  Created on: May 1, 2015
 *      Author: savage
 */
#include "peer.h"

#include "webrtc/base/ssladapter.h"

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

void* Init() {
	one::InitOneSpdlogConsole();
	gang::InitializeGangDecoderGlobel();
	rtc::InitializeSSL();
	return reinterpret_cast<void*>(new Shared(DTLS_ON));
}

void Release(void* sharedPtr) {
	Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
	if (shared) {
		delete shared;
	}
	rtc::CleanupSSL();
	gang::CleanupGangDecoderGlobel();
	one::CleanupOneSpdlog();
}

void AddICE(void* sharedPtr, char *uri, char *name, char *psd) {
	reinterpret_cast<Shared*>(sharedPtr)->AddIceServer(
			string(uri),
			string(name),
			string(psd));
}

int RegistryUrl(void* sharedPtr, char *url) {
	Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
	return shared->AddPeerConnectionFactory(string(url));
}

void* CreatePeer(char *url, void* sharedPtr, void* goPcPtr) {
	Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
	Peer *cpc = new Peer(string(url), shared, goPcPtr);
	return reinterpret_cast<void*>(cpc);
}

void DeletePeer(void* pc) {
	Peer *cpc = reinterpret_cast<Peer*>(pc);
	if (cpc) {
		delete cpc;
	}
}

void CreateAnswer(void* pc, char* sdp) {
	reinterpret_cast<Peer*>(pc)->CreateAnswer(string(sdp));
}

void AddCandidate(void* pc, char* sdp, char* mid, int line) {
	reinterpret_cast<Peer*>(pc)->AddCandidate(string(sdp), string(mid), line);
}

