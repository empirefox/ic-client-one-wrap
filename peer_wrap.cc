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
  SPDLOG_TRACE(console, "{}", __func__)
  if (sharedPtr) {
    delete reinterpret_cast<Shared*>(sharedPtr);
    SPDLOG_TRACE(console, "{} {}", __func__, "deleted")
  }
  rtc::CleanupSSL();
  SPDLOG_TRACE(console, "{} {}", __func__, "CleanupSSL ok")
  gang::CleanupGangDecoderGlobel();
  SPDLOG_TRACE(console, "{} {}", __func__, "CleanupGangDecoderGlobel ok")
  one::CleanupOneSpdlog();
  std::cout << "Release ok" << std::endl;
}

void AddICE(void* sharedPtr, char* uri, char* name, char* psd) {
  reinterpret_cast<Shared*>(sharedPtr)->AddIceServer(string(uri), string(name), string(psd));
}

int RegistryCam(
  ipcam_info* info,
  void*       sharedPtr,
  char*       id,
  char*       url,
  char*       rec_name,
  int         rec_enabled,
  int         audio_off) {
  Shared* shared    = reinterpret_cast<Shared*>(sharedPtr);
  string  cid       = string(id);
  string  curl      = string(url);
  string  crec_name = string(rec_name);

  return shared->AddPeerConnectionFactory(info, cid, curl, crec_name, rec_enabled, audio_off);
}

void SetRecordEnabled(void* sharedPtr, char* id, int rec_enabled) {
  Shared* shared  = reinterpret_cast<Shared*>(sharedPtr);
  string  cid     = string(id);
  auto    factory = shared->GetPeerConnectionFactory(cid);

  if (!factory) {
    return;
  }
  factory->SetRecordEnabled(rec_enabled);
}

void* CreatePeer(char* id, void* sharedPtr, void* goPcPtr) {
  Shared* shared = reinterpret_cast<Shared*>(sharedPtr);

  return reinterpret_cast<void*>(shared->CreatePeer(string(id), goPcPtr));
}

void DeletePeer(void* pc) {
  if (pc) {
    Peer* cpc = reinterpret_cast<Peer*>(pc);
    cpc->GetShared()->DeletePeer(cpc);
  }
}

void CreateAnswer(void* pc, char* sdp) {
  Peer*  cpc  = reinterpret_cast<Peer*>(pc);
  string csdp = string(sdp);

  cpc->CreateAnswer(csdp);
}

void AddCandidate(void* pc, char* sdp, char* mid, int line) {
  Peer*  cpc  = reinterpret_cast<Peer*>(pc);
  string csdp = string(sdp);
  string cmid = string(mid);

  webrtc::SdpParseError                          error;
  rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate(
    webrtc::CreateIceCandidate(cmid, line, csdp, &error));

  if (!candidate.get()) {
    one::console->error() << "Can't parse received candidate message. " << "SdpParseError was: "
                          << error.description;
    return;
  }
  cpc->AddCandidate(candidate.release());
}
