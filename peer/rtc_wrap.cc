#include "rtc_wrap.h"

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/bind.h"

#include "peer.h"
#include "gang_init_deps.h"
#include "one_spdlog_console.h"

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

bool RegistryCam(
  ipcam_av_info* av_info,
  void*          sharedPtr,
  char*          id,
  ipcam_info*    info) {
  Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
  string  cid    = string(id);

  // free id then cid == ""
  return shared->AddPeerConnectionFactory(av_info, cid, info);
}

void UnregistryCam(void* sharedPtr, char* id) {
  reinterpret_cast<Shared*>(sharedPtr)->DelPeerConnectionFactory(string(id));
}

void SetRecordEnabled(void* sharedPtr, char* id, bool rec_on) {
  Shared* shared  = reinterpret_cast<Shared*>(sharedPtr);
  string  cid     = string(id);
  auto    factory = shared->GetPeerConnectionFactory(cid);

  if (factory) {
    factory->SetRecordEnabled(rec_on);
  }
}

void* CreatePeer(char* id, void* sharedPtr, void* goPcPtr) {
  Shared* shared = reinterpret_cast<Shared*>(sharedPtr);
  string  cid    = string(id);

  return reinterpret_cast<void*>(shared->CreatePeer(cid, goPcPtr));
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
