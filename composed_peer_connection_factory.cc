#include "composed_peer_connection_factory.h"

#include "gangvideocapturer.h"

#include "one_spdlog_console.h"
#include "shared.h"

namespace one {

using gang::GangVideoCapturer;

const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamLabel[] = "stream_label";

ComposedPeerConnectionFactory::ComposedPeerConnectionFactory(
		const string& url,
		Shared* shared) :
				url_(url),
				worker_thread_(new Thread),
				shared_(shared),
				refed_(false),
				video_(NULL) {

	if (!worker_thread_->Start()) {
		console->error() << "worker_thread_ failed to start";
	}
	SPDLOG_TRACE(console);
}

ComposedPeerConnectionFactory::~ComposedPeerConnectionFactory() {
	SPDLOG_TRACE(console);
	delete worker_thread_;
}

scoped_refptr<PeerConnectionInterface> ComposedPeerConnectionFactory::CreatePeerConnection(
		PeerConnectionObserver* observer) {
	SPDLOG_TRACE(console);
	scoped_refptr<PeerConnectionInterface> peer_connection_ =
			factory_->CreatePeerConnection(
					shared_->IceServers,
					&shared_->Constraints,
					NULL,
					NULL,
					observer);

	scoped_refptr<webrtc::MediaStreamInterface> stream =
			factory_->CreateLocalMediaStream(kStreamLabel);

	if (decoder_->IsAudioAvailable()) {
		scoped_refptr<webrtc::AudioTrackInterface> audio_track(
				factory_->CreateAudioTrack(
						kAudioLabel,
						factory_->CreateAudioSource(NULL)));
		if (!stream->AddTrack(audio_track)) {
			console->error("Failed to add audio track ({})", url_);
		}
	}

	if (decoder_->IsVideoAvailable()) {
		rtc::CritScope cs(&lock_);
		scoped_refptr<webrtc::VideoTrackInterface> video_track;
		if (video_.get() || CreateVideoSource()) {
			SPDLOG_TRACE(console, "Got video source.");
			video_track = factory_->CreateVideoTrack(
					kVideoLabel,
					OwnedVideoSourceRef::Create(this, video_));
		}
		if (video_track.get() && !stream->AddTrack(video_track)) {
			// after one delete, exist count 3 -> 2, created count 2 -> 1(delete)
			console->error("Failed to add video track ({})", url_);
		}
	}

	if (!peer_connection_->AddStream(stream)) {
		console->error("Adding stream to PeerConnection failed ({})", url_);
	}

	return peer_connection_;
}

// TODO refactor this
// maybe start gang when peer connection count>0
//       stop gang when peer connection count==0
void ComposedPeerConnectionFactory::SetRef(bool refed) {
	SPDLOG_TRACE(console, "SetRef start.");
	rtc::CritScope cs(&lock_);
	refed_ = refed;
	if (!refed_ && video_.get()) {
		SPDLOG_TRACE(console, "SetRef process.");
		if (video_->AddRef() == 2) {
			SPDLOG_TRACE(console, "SetRef delete.");
			video_->Release();
			video_ = NULL;
			return;
		}
		SPDLOG_TRACE(console, "SetRef pass.");
		video_->Release();
	}
	SPDLOG_TRACE(console, "SetRef end.");
}

bool ComposedPeerConnectionFactory::CreateVideoSource() {
	SPDLOG_TRACE(console, "CreateVideoSource start.");
	// auto delete with SetRef(false)
	video_ = factory_->CreateVideoSource(
			GangVideoCapturer::Create(decoder_.get()),
			NULL);

	SPDLOG_TRACE(console, "VideoSource created.");
	return video_ != NULL;
}

// TODO move Factory relative to signaling thread
bool ComposedPeerConnectionFactory::Init() {
	SPDLOG_TRACE(console);
// 1. Create GangDecoder
	decoder_ = shared_ptr<GangDecoder>(new GangDecoder(url_));
	if (!decoder_->Init()) {
		return false;
	}

// 2. Create GangAudioDevice
	if (decoder_->IsAudioAvailable()) {
		audio_ = GangAudioDevice::Create(decoder_.get(), 1);
		if (!audio_.get()) {
			return false;
		}
		SPDLOG_TRACE(console, "GangAudioDevice Initialized.");
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
	SPDLOG_TRACE(console, "PeerConnectionFactory created.");

// 4. Create VideoSource
	// will be created after peer connection created

// 5. Check if video or audio exist
	if (!audio_.get() && !video_.get()) {
		console->error("Failed to get media from ({})", url_);
		return false;
	}

	SPDLOG_TRACE(console, "OK");

	return true;
}

} // namespace one
