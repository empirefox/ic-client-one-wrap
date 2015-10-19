/*
 * composed_peer_connection_factory.h
 *
 *  Created on: Apr 30, 2015
 *      Author: savage
 */
#ifndef COMPOSED_PC_FACTORY_H_
#define COMPOSED_PC_FACTORY_H_
#pragma once

#include <memory>
#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "gang_decoder.h"
#include "gangvideocapturer.h"
#include "gang_audio_device.h"
#include "ipcam_info.h"

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

using gang::GangDecoder;
using gang::GangVideoCapturer;
using gang::GangAudioDevice;

class Shared;

class ComposedPCFactory {
public:
	ComposedPCFactory(
			Shared* shared,
			const string& id,
			const string& url,
			const string& rec_name,
			bool rec_enabled,
			bool audio_off);
	~ComposedPCFactory();

	scoped_refptr<PeerConnectionInterface> CreatePeerConnection(PeerConnectionObserver* observer);
	void RemoveOnePeerConnection();

	bool Init(ipcam_info* info);
	bool CreateFactory();
	void SetRecordEnabled(bool enabled);

private:
	void releaseFactory();

	const string id_;
	Thread* worker_thread_;
	Shared* shared_;

	shared_ptr<GangDecoder> decoder_;
	scoped_refptr<PeerConnectionFactoryInterface> factory_;
	scoped_refptr<MediaStreamInterface> stream_;
	int peers_;
	mutable rtc::CriticalSection lock_;
};

} // namespace one

#endif /* COMPOSED_PC_FACTORY_H_ */
