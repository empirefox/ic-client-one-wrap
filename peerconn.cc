/*
 * Copyright 2015, empirefox
 * Copyright 2012, Google Inc.
 */
#include <iostream>

#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/media/devices/devicemanager.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/json.h"
#include "fakeconstraints.h"

#include "defaults.h"
#include "message.h"
#include "fake_test.h"
#include "gang_audio_device.h"

#include "peerconn.hh"
#include "cgo.h"

extern "C" {
#include "peerconn.h"
}

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";
// TODO test rtsp
const char kTestUrl[] = "rtsp://127.0.0.1:1235/test1.sdp";

#define DTLS_ON  true
#define DTLS_OFF false

class DummySetSessionDescriptionObserver: public webrtc::SetSessionDescriptionObserver {
public:
	static DummySetSessionDescriptionObserver* Create() {
		return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() {
		LOG(INFO) << __func__;
		Msg::Info("SetSessionDescriptionObserver.OnSuccess");
	}
	virtual void OnFailure(const std::string& error) {
		LOG(INFO) << __func__ << " " << error;
		Msg::Error("DummySetSessionDescriptionObserver");
		Msg::Error(error);
	}

protected:
	DummySetSessionDescriptionObserver() {
	}
	~DummySetSessionDescriptionObserver() {
	}
};

Conductor::Conductor() :
		signaling_thread_(new rtc::Thread), worker_thread_(new rtc::Thread) {
	if (!signaling_thread_->Start()) {
		Msg::Error("Failed to start signaling_thread_");
	}
	if (!worker_thread_->Start()) {
		Msg::Error("Failed to start worker_thread_");
	}
}

Conductor::~Conductor() {
	ASSERT(peer_connection_.get() == NULL);
	delete signaling_thread_;
	delete worker_thread_;
}

// public
void Conductor::CreateAnswer(std::string& sdp) {
	if (!InitializePeerConnection()) {
		LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
		return;
	}

	webrtc::SessionDescriptionInterface* session_description(
			webrtc::CreateSessionDescription(
					webrtc::SessionDescriptionInterface::kOffer, sdp));
	if (!session_description) {
		LOG(WARNING) << "Can't parse received session description message.";
		Msg::Error("Can't parse received session description message.");
		return;
	}
	peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(), session_description);

	Msg::Info("SetRemoteDescription");

	peer_connection_->CreateAnswer(this, NULL);
}

void Conductor::AddCandidate(std::string& sdp, std::string& mid, int line) {
	Msg::Info("add a candidate");
	rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate(
			webrtc::CreateIceCandidate(mid, line, sdp));
	if (!candidate.get()) {
		LOG(WARNING) << "Can't parse received candidate message.";
		Msg::Error("Can't parse received candidate message.");
		return;
	}
	if (!peer_connection_->AddIceCandidate(candidate.get())) {
		LOG(WARNING) << "Failed to apply the received candidate";
		Msg::Error("Failed to apply the received candidate");
		return;
	}
	Msg::Info("added a candidate");
	return;
}

void Conductor::AddICE(std::string& uri, std::string& name, std::string& psd) {
	webrtc::PeerConnectionInterface::IceServer server;
	server.uri = uri;
	if (!name.empty()) {
		server.username = name;
	}
	if (!psd.empty()) {
		server.password = psd;
	}
	iceServers_.push_back(server);
}

bool Conductor::connection_active() const {
	return peer_connection_.get() != NULL;
}

void Conductor::Close() {
	// close ws
	DeletePeerConnection();
}
// end of public

bool Conductor::InitializePeerConnection() {
	ASSERT(peer_connection_factory_.get() == NULL);
	ASSERT(peer_connection_.get() == NULL);
	ASSERT(signaling_thread_ != NULL);
	ASSERT(worker_thread_ != NULL);

	rtc::scoped_refptr<gang::GangAudioDevice> adm = gang::GangAudioDevice::Create();

	peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
			worker_thread_, signaling_thread_,
			adm,
			NULL, NULL);

	if (!peer_connection_factory_.get()) {
		Msg::Error("Failed to initialize PeerConnectionFactory");
		DeletePeerConnection();
		return false;
	}

	if (!CreatePeerConnection(DTLS_ON)) {
		Msg::Error("CreatePeerConnection failed");
		DeletePeerConnection();
	}
	Msg::Info("Created PeerConnection");
	sources_.RegistryDecoder(peer_connection_factory_, kTestUrl);
	adm->Initialize(sources_.GetDecoder(kTestUrl).get());
	Msg::Info("RegistryDecoder ok");
	AddStreams();
	Msg::Info("AddStreams ok");
	return peer_connection_.get() != NULL;
}

bool Conductor::CreatePeerConnection(bool dtls) {
	ASSERT(peer_connection_factory_.get() != NULL);
	ASSERT(peer_connection_.get() == NULL);
	ASSERT(g_IceServers.size() != 0);

	webrtc::FakeConstraints constraints;
	constraints.SetMandatoryReceiveAudio(false);
	constraints.SetMandatoryReceiveVideo(false);
	if (dtls) {
		constraints.AddOptional(
				webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "true");
	} else {
		constraints.AddOptional(
				webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "false");
	}

	peer_connection_ = peer_connection_factory_->CreatePeerConnection(
			iceServers_, &constraints,
			NULL,
			NULL, this);
	return peer_connection_.get() != NULL;
}

void Conductor::DeletePeerConnection() {
	peer_connection_ = NULL;
	active_streams_.clear();
	peer_connection_factory_ = NULL;
}

//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void Conductor::OnAddStream(webrtc::MediaStreamInterface* stream) {
	LOG(INFO) << __FUNCTION__ << " " << stream->label();
	printf("should never call this func: %s\n", __func__);
	stream->AddRef();
//	stream->Release();
}

void Conductor::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
	LOG(INFO) << __FUNCTION__ << " " << stream->label();
	printf("should never call this func: %s\n", __func__);
	stream->AddRef();
//	stream->Release();
}

void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
	LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();

	Msg::Info("OnIceCandidate ok");

	Json::StyledWriter writer;
	Json::Value jmessage;

	jmessage[kSessionDescriptionTypeName] = "candidate";
	jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
	jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
	std::string sdp;
	if (!candidate->ToString(&sdp)) {
		LOG(LS_ERROR) << "Failed to serialize candidate";
		return;
	}
	jmessage[kCandidateSdpName] = sdp;
	SendMessage(writer.write(jmessage));
}

cricket::VideoCapturer* Conductor::OpenVideoCaptureDevice() {
	Msg::Info("OpenVideoCaptureDevice start");
	rtc::scoped_ptr<cricket::DeviceManagerInterface> dev_manager(
			cricket::DeviceManagerFactory::Create());
	if (!dev_manager->Init()) {
		LOG(LS_ERROR) << "Can't create device manager";
		Msg::Error("Can't create device manager");
		return NULL;
	}
	Msg::Info("dev_manager->Init ok");
	std::vector<cricket::Device> devs;
	if (!dev_manager->GetVideoCaptureDevices(&devs)) {
		LOG(LS_ERROR) << "Can't enumerate video devices";
		Msg::Error("Can't enumerate video devices");
		return NULL;
	}
	Msg::Info("dev_manager->GetVideoCaptureDevices ok");
	std::string dev_size = std::to_string(devs.size());
	Msg::Info(dev_size);
	std::vector<cricket::Device>::iterator dev_it = devs.begin();
	cricket::VideoCapturer* capturer = NULL;
	for (; dev_it != devs.end(); ++dev_it) {
		Msg::Info("loop a device");
		capturer = dev_manager->CreateVideoCapturer(*dev_it);
		Msg::Info(dev_it->id);
		Msg::Info(dev_it->name);
		if (capturer != NULL) {
			Msg::Info("capturer ok");
			break;
		}
	}
	Msg::Info("OpenVideoCaptureDevice end");
	return capturer;
}

// TODO add getUserMedia func after direct test
// "rtsp://218.204.223.237:554/live/1/0547424F573B085C/gsfp90ef4k0a6iap.sdp"
void Conductor::AddStreams() {
	if (active_streams_.find(kStreamLabel) != active_streams_.end()) {
		Msg::Info("stream Already added");
		return;  // Already added.
	}
	Msg::Info("Adding remote stream");
	const char* url = kTestUrl;

	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
			peer_connection_factory_->CreateAudioTrack(kAudioLabel,
			peer_connection_factory_->CreateAudioSource(NULL)));
	Msg::Info("audio_track ok");

	rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
			peer_connection_factory_->CreateVideoTrack(kVideoLabel,
			// TODO gangsources
					sources_.GetVideo(url)));
	Msg::Info("video_track ok");

	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
			peer_connection_factory_->CreateLocalMediaStream(kStreamLabel);

	stream->AddTrack(audio_track);
	stream->AddTrack(video_track);
	if (!peer_connection_->AddStream(stream)) {
		LOG(LS_ERROR) << "Adding stream to PeerConnection failed";
		Msg::Error("Adding stream to PeerConnection failed");
	}
	Msg::Info("stream added");
	typedef std::pair<std::string,
			rtc::scoped_refptr<webrtc::MediaStreamInterface> > MediaStreamPair;
	active_streams_.insert(MediaStreamPair(stream->label(), stream));
}

void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
	Msg::Info("answer created");
	peer_connection_->SetLocalDescription(
			DummySetSessionDescriptionObserver::Create(), desc);
	Msg::Info("SetLocalDescription");

	std::string sdp;
	desc->ToString(&sdp);

	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage[kSessionDescriptionTypeName] = desc->type();
	jmessage[kSessionDescriptionSdpName] = sdp;
	Msg::Info("send desc in Conductor::OnSuccess");
	SendMessage(writer.write(jmessage));
}

void Conductor::OnFailure(const std::string& error) {
	Msg::Error("Conductor::OnFailure");
	Msg::Error(error);
	LOG(LERROR) << error;
}

void Conductor::SendMessage(const std::string& json_object) {
	go_send_to_peer(this, const_cast<char*>(json_object.c_str()));
}

// used by go

void init() {
	rtc::InitializeSSL();
}

void* CreatePeer() {
	Conductor *cpc = new Conductor();
	return (void*) cpc;
}

void CreateAnswer(void* pc, char* sdp) {
	Conductor* cpc = (Conductor*) pc;
	std::string csdp = std::string(sdp);
	cpc->CreateAnswer(csdp);
}

void AddCandidate(void* pc, char* sdp, char* mid, int line) {
	Conductor* cpc = (Conductor*) pc;
	std::string csdp = std::string(sdp);
	std::string cmid = std::string(mid);
	cpc->AddCandidate(csdp, cmid, (int) line);
}

void AddICE(void* pc, char *uri, char *name, char *psd) {
	ASSERT(uri != NULL);

	Conductor* cpc = (Conductor*) pc;
	std::string suri = std::string(uri);
	std::string sname = std::string(name);
	std::string spsd = std::string(psd);

	cpc->AddICE(suri, sname, spsd);
}
