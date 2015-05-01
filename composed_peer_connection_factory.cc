#include "gangvideocapturer.h"
#include "composed_peer_connection_factory.h"
#include "defaults.h"
#include "message.h"
#include "shared.h"

namespace one {

using gang::GangVideoCapturer;

ComposedPeerConnectionFactory::ComposedPeerConnectionFactory(
		const string& url,
		Shared* shared) :
				url_(url),
				worker_thread_(new Thread),
				shared_(shared) {

	if (!worker_thread_->Start()) {
		Msg::Error("Failed to start worker_thread_");
	}
}

ComposedPeerConnectionFactory::~ComposedPeerConnectionFactory() {
	delete worker_thread_;
}

scoped_refptr<PeerConnectionInterface> ComposedPeerConnectionFactory::CreatePeerConnection(
		PeerConnectionObserver* observer) {
	scoped_refptr<PeerConnectionInterface> peer_connection_ =
			factory_->CreatePeerConnection(
					shared_->IceServers,
					&shared_->Constraints,
					NULL,
					NULL,
					observer);

	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
			factory_->CreateLocalMediaStream(kStreamLabel);

	if (decoder_->IsAudioAvailable()) {
		rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
				factory_->CreateAudioTrack(
						kAudioLabel,
						factory_->CreateAudioSource(NULL)));
		stream->AddTrack(audio_track);
	}

	if (decoder_->IsVideoAvailable()) {
		rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
				factory_->CreateVideoTrack(kVideoLabel, video_));
		stream->AddTrack(video_track);
	}

	if (!peer_connection_->AddStream(stream)) {
		Msg::Error("Adding stream to PeerConnection failed");
	}

	return peer_connection_;
}

bool ComposedPeerConnectionFactory::Init() {
// 1. Create GangDecoder
	decoder_ = shared_ptr<GangDecoder>(new GangDecoder(url_));
	if (!decoder_->Init()) {
		return false;
	}

// 2. Create GangAudioDevice
	if (decoder_->IsAudioAvailable()) {
		audio_ = gang::GangAudioDevice::Create();
		if (!audio_.get()) {
			return false;
		}
		audio_->Initialize(decoder_.get());
	}

// 3. Create PeerConnectionFactory
	factory_ = webrtc::CreatePeerConnectionFactory(
			worker_thread_,
			shared_->SignalingThread,
			audio_,
			NULL,
			NULL);
	if (!factory_.get()) {
		return false;
	}

// 4. Create VideoSource
	if (decoder_->IsVideoAvailable()) {
		GangVideoCapturer* capturer = new GangVideoCapturer();
		if (!capturer) {
			return false;
		}
		capturer->Initialize(decoder_.get());
		video_ = factory_->CreateVideoSource(capturer, NULL);
		if (!video_.get()) {
			return false;
		}
	}

// 5. Check if video or audio exist
	if (!audio_.get() && !video_.get()) {
		return false;
	}

	return true;
}

} // namespace one
