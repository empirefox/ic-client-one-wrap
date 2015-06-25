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
#include "owner_ref_proxy.h"

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

class ComposedPeerConnectionFactory: public RefOwner {
public:
	ComposedPeerConnectionFactory(
			Shared* shared,
			const string& url,
			const string& rec_name,
			bool rec_enabled);
	~ComposedPeerConnectionFactory();

	scoped_refptr<PeerConnectionInterface> CreatePeerConnection(PeerConnectionObserver* observer);

	virtual void SetRef(bool);

	bool Init();
	void SetRecordEnabled(bool enabled);

private:

	bool CreateVideoSource();

	string url_;
	Thread* worker_thread_;
	Shared* shared_;
	bool refed_;

	mutable rtc::CriticalSection lock_;

	scoped_refptr<PeerConnectionFactoryInterface> factory_;
	scoped_refptr<VideoSourceInterface> video_;
	scoped_refptr<GangAudioDevice> audio_;
	shared_ptr<GangDecoder> decoder_;
};

} // namespace one

#endif /* COMPOSED_PEER_CONNECTION_FACTORY_H_ */
