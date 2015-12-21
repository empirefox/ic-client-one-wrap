#pragma once

#include <memory>
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/json.h"
#include "shared.h"

namespace one {
typedef rtc::TypedMessageData<Peer*>                          DeletePeerMsgData;
typedef rtc::TypedMessageData<string>                         RemoteOfferMsgData;
typedef rtc::ScopedMessageData<webrtc::IceCandidateInterface> IceCandidateMsgData;

enum SignalType {RemoteOfferSignal, RemoteCandidateSignal};

class Peer : public webrtc::PeerConnectionObserver,
             public webrtc::CreateSessionDescriptionObserver,
             public rtc::MessageHandler {
public:
  Peer(const std::string& id,
       Shared*            shared,
       void*              goPcPtr);
  ~Peer();

  //
  // will export
  //
  void         CreateAnswer(std::string& offerSDP_sent_from_offerer);
  void         AddCandidate(webrtc::IceCandidateInterface* candidate);

  bool         connection_active() const;

  // close conn to server
  virtual void Close();

  Shared*      GetShared();

  // implements the MessageHandler interface
  void         OnMessage(rtc::Message* msg);

protected:
  bool         CreatePeerConnection();

  //
  // PeerConnectionObserver implementation.
  //
  virtual void OnStateChange(webrtc::PeerConnectionObserver::StateType state_changed) {}

  virtual void OnAddStream(webrtc::MediaStreamInterface* stream);
  virtual void OnRemoveStream(webrtc::MediaStreamInterface* stream);
  virtual void OnDataChannel(webrtc::DataChannelInterface* channel) {}

  virtual void OnRenegotiationNeeded()                              {}

  virtual void OnIceChange()                                        {}

  virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);

  //
  // CreateSessionDescriptionObserver implementation.
  //
  virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
  virtual void OnFailure(const std::string& error);

  //
  // TODO idk
  //
  virtual int AddRef() {
    return 0;
  } // We manage own memory.

  virtual int Release() {
    return 0;
  }

  // TODO should select ws
  // use GO FUNC to send a message to the remote peer.
  // or should use callback to send without json.
  virtual void SendMessage(Json::StyledWriter& writer,
                           Json::Value&        jmessage);

  // fields
  std::string                                         id_;
  Shared*                                             shared_;
  void*                                               goPcPtr_;
  one::Factoty                                        factory_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
};
} // namespace one
