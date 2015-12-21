#pragma once

#include <memory>
#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"

#include "gang.h"
#include "gangvideocapturer.h"
#include "gang_audio_device.h"

namespace one {
using std::string;
using std::shared_ptr;

using webrtc::PeerConnectionFactoryInterface;
using webrtc::PeerConnectionInterface;
using webrtc::PeerConnectionObserver;
using webrtc::VideoSourceInterface;
using webrtc::MediaStreamInterface;
using rtc::Thread;
using rtc::scoped_refptr;

using gang::Gang;
using gang::GangDecoderFactoryInterface;
using gang::GangVideoCapturer;
using gang::GangAudioDevice;

class Shared;

class ComposedPCFactory {
public:
  ComposedPCFactory(
    Shared*                                 shared,
    const string&                           id,
    ipcam_info*                             info,
    shared_ptr<GangDecoderFactoryInterface> dec_factory);
  ~ComposedPCFactory();

  scoped_refptr<PeerConnectionInterface> CreatePeerConnection(PeerConnectionObserver* observer);
  void                                   RemoveOnePeerConnection();

  bool                                   Init(ipcam_av_info* info);
  bool                                   CreateFactory();
  void                                   SetRecordEnabled(bool enabled);

private:
  void                                   releaseFactory();

  const string id_;
  Thread*      worker_thread_;
  Shared*      shared_;

  shared_ptr<Gang>                              gang_;
  scoped_refptr<PeerConnectionFactoryInterface> factory_;
  scoped_refptr<MediaStreamInterface>           stream_;
  int                                           peers_;
  mutable rtc::CriticalSection                  lock_;
};
} // namespace one
