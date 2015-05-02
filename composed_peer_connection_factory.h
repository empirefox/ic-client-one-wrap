/*
 * composed_peer_connection_factory.h
 *
 *  Created on: Apr 30, 2015
 *      Author: savage
 */
#ifndef COMPOSED_PEER_CONNECTION_FACTORY_H_
#define COMPOSED_PEER_CONNECTION_FACTORY_H_
#pragma once

#include <memory>
#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "gang_decoder.h"
#include "gang_audio_device.h"

namespace one {

using std::map;
using std::string;
using std::shared_ptr;

using webrtc::PeerConnectionFactoryInterface;
using webrtc::PeerConnectionInterface;
using webrtc::PeerConnectionObserver;
using webrtc::VideoSourceInterface;
using rtc::Thread;
using rtc::scoped_refptr;

using gang::GangDecoder;
using gang::GangAudioDevice;

class Shared;

class ComposedPeerConnectionFactory {
public:
	ComposedPeerConnectionFactory(const string& url, Shared* shared);
	~ComposedPeerConnectionFactory();

	scoped_refptr<PeerConnectionInterface> CreatePeerConnection(
			PeerConnectionObserver* observer);

	bool Init();

private:

	string url_;
	Thread* worker_thread_;
	Shared* shared_;

	scoped_refptr<PeerConnectionFactoryInterface> factory_;
	scoped_refptr<VideoSourceInterface> video_;
	scoped_refptr<GangAudioDevice> audio_;
	shared_ptr<GangDecoder> decoder_;
};

} // namespace one

#endif /* COMPOSED_PEER_CONNECTION_FACTORY_H_ */
