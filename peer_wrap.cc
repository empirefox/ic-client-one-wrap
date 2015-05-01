/*
 * peer_wrap.cc
 *
 *  Created on: May 1, 2015
 *      Author: savage
 */
#include "peer.h"
#include "webrtc/base/ssladapter.h"

extern "C" {
#include "peer_wrap.h"
}

#define DTLS_ON  true
#define DTLS_OFF false

void* Init() {
	rtc::InitializeSSL();
	return reinterpret_cast<void*>(new one::Shared(DTLS_ON));
}

void Release(void* sharedPtr) {
	one::Shared* shared = reinterpret_cast<one::Shared*>(sharedPtr);
	if (shared) {
		delete shared;
	}
	rtc::CleanupSSL();
}

void AddICE(void* sharedPtr, char *uri, char *name, char *psd) {
	reinterpret_cast<one::Shared*>(sharedPtr)->AddIceServer(
			std::string(uri),
			std::string(name),
			std::string(psd));
}

int RegistryUrl(void* sharedPtr, char *url) {
	one::Shared* shared = reinterpret_cast<one::Shared*>(sharedPtr);
	return shared->AddPeerConnectionFactory(std::string(url));
}

void* CreatePeer(char *url, void* sharedPtr, void* chanPtr) {
	one::Shared* shared = reinterpret_cast<one::Shared*>(sharedPtr);
	Peer *cpc = new Peer(std::string(url), shared, chanPtr);
	return reinterpret_cast<void*>(cpc);
}

void DeletePeer(void* pc) {
	Peer *cpc = reinterpret_cast<Peer*>(pc);
	if (cpc) {
		delete cpc;
	}
}

void CreateAnswer(void* pc, char* sdp) {
	reinterpret_cast<Peer*>(pc)->CreateAnswer(std::string(sdp));
}

void AddCandidate(void* pc, char* sdp, char* mid, int line) {
	reinterpret_cast<Peer*>(pc)->AddCandidate(
			std::string(sdp),
			std::string(mid),
			line);
}

