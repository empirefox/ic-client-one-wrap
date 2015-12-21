#pragma once

#include <map>
#include "talk/app/webrtc/peerconnectioninterface.h"

#include "composed_pc_factory.h"
#include "fakeconstraints.h"

namespace one {
class Peer;

using std::map;
using std::string;
using std::shared_ptr;

using webrtc::PeerConnectionInterface;
using rtc::Thread;

typedef shared_ptr<ComposedPCFactory> Factoty;

class Shared : public gang::StatusObserver {
public:
  Shared(void* goConductorPtr,
         bool  dtls);
  virtual ~Shared();

  virtual void OnStatusChange(const string&    id,
                              gang::GangStatus status);


  Peer* CreatePeer(const string& id,
                   void*         goPcPtr);
  void  DeletePeer(Peer* pc);
  void  AddIceServer(string uri,
                     string name,
                     string psd);
  void  SetDecFactory(GangDecoderFactoryInterface* dec_factory);

  // Must after AddIceServer
  // Will be used in go
  bool AddPeerConnectionFactory(
    ipcam_av_info* av_info,
    const string&  id,
    ipcam_info*    info);

  Factoty GetPeerConnectionFactory(const string& url);

  PeerConnectionInterface::IceServers IceServers;
  webrtc::FakeConstraints             Constraints;
  Thread*                             MainThread;
  Thread*                             SignalingThread;
  mutable rtc::CriticalSection        peer_lock_;
  mutable rtc::CriticalSection        factories_lock_;

private:
  void InitConstraintsOnce(bool dtls);

  map<string, Factoty>                    factories_;
  shared_ptr<GangDecoderFactoryInterface> dec_factory_;
  void*                                   goConductorPtr_;
};
} // namespace one
