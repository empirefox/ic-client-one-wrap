#include "composed_peer_connection_factory.h"

#include "one_spdlog_console.h"
#include "shared.h"

namespace one {

using gang::GangVideoCapturer;

const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamLabel[] = "stream_label";

ComposedPeerConnectionFactory::ComposedPeerConnectionFactory(
		Shared* shared,
		const string& url,
		const string& rec_name,
		bool rec_enabled) :
				url_(url),
				worker_thread_(new Thread),
				shared_(shared),
				decoder_(new GangDecoder(url, rec_name, rec_enabled)),
				video_(NULL),
				peers_(0) {

	if (!worker_thread_->Start()) {
		console->error() << "worker_thread_ failed to start";
	}
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
}

ComposedPeerConnectionFactory::~ComposedPeerConnectionFactory() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	stream_ = NULL;
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "stream_ ok")
	audio_ = NULL;
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "audio_ ok")
	decoder_ = NULL;
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "decoder_ ok")
	factory_ = NULL;
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "factory_ ok")
	// TODO need stop?
	worker_thread_->Stop();
	delete worker_thread_;
	worker_thread_ = NULL;
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
}

scoped_refptr<PeerConnectionInterface> ComposedPeerConnectionFactory::CreatePeerConnection(
		PeerConnectionObserver* observer) {
	DCHECK(shared_->SignalingThread->IsCurrent());
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	auto peer_connection_ = factory_->CreatePeerConnection(
			shared_->IceServers,
			&shared_->Constraints,
			NULL,
			NULL,
			observer);

	rtc::CritScope cs(&lock_);
	if (!stream_.get()) {
		InitStram();
	}
	if (!peer_connection_->AddStream(stream_)) {
		console->error("Adding stream to PeerConnection failed ({})", url_);
	}
	++peers_;
	SPDLOG_TRACE(console, "{} {} {}", __FUNCTION__, "++peers=", peers_)

	return peer_connection_;
}

void ComposedPeerConnectionFactory::RemoveOnePeerConnection() {
	DCHECK(shared_->SignalingThread->IsCurrent());
	rtc::CritScope cs(&lock_);
	--peers_;
	SPDLOG_TRACE(console, "{} {} {}", __FUNCTION__, "--peers=", peers_)
	if (!peers_ && decoder_->IsVideoAvailable()) {
		// TODO change to video_ and add ++peers==1 func
		stream_->FindVideoTrack(kVideoLabel)->GetSource()->Stop();
	}
}

bool ComposedPeerConnectionFactory::Init() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	// 1. Check GangDecoder
	if (!decoder_->Init()) {
		return false;
	}

	// 2. Create GangAudioDevice
	if (decoder_->IsAudioAvailable()) {
		audio_ = GangAudioDevice::Create(decoder_);
		if (!audio_.get()) {
			return false;
		}
		SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "GangAudioDevice Initialized.")
	}

	// 3. Create PeerConnectionFactory
	factory_ = webrtc::CreatePeerConnectionFactory(worker_thread_, shared_->SignalingThread, audio_,
	NULL,
	NULL);
	if (!factory_.get()) {
		return false;
	}
	SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "PeerConnectionFactory created.")

	// 4. Create VideoSource
	if (decoder_->IsVideoAvailable()) {
		video_ = GangVideoCapturer::Create(decoder_);
		if (!video_) {
			return false;
		}
		SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "GangVideoCapturer Initialized.")
	}

	// 5. Check if video or audio exist
	if (!audio_.get() && !video_) {
		console->error("{} Failed to get media from ({})", __FUNCTION__, url_);
		return false;
	}

	return true;
}

void ComposedPeerConnectionFactory::InitStram() {
	DCHECK(shared_->SignalingThread->IsCurrent());
	stream_ = factory_->CreateLocalMediaStream(kStreamLabel);
	if (decoder_->IsAudioAvailable()) {
		if (!stream_->AddTrack(
				factory_->CreateAudioTrack(kAudioLabel, factory_->CreateAudioSource(NULL)))) {
			console->error("{} Failed to add audio track ({})", __FUNCTION__, url_);
		}
	}
	if (decoder_->IsVideoAvailable()) {
		if (!stream_->AddTrack(
				factory_->CreateVideoTrack(kVideoLabel, factory_->CreateVideoSource(video_,
				NULL)))) {
			// after one delete, exist count 3 -> 2, created count 2 -> 1(delete)
			console->error("{} Failed to add video track ({})", __FUNCTION__, url_);
		}
	}

	SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "OK")
}

void ComposedPeerConnectionFactory::SetRecordEnabled(bool enabled) {
	if (decoder_) {
		decoder_->SetRecordEnabled(enabled);
	}
}

} // namespace one
