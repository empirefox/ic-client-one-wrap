#include "gang_audio_device.h"

#include "webrtc/base/common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/timeutils.h"

#include "gang_spdlog_console.h"

namespace gang {
// Audio sample value that is high enough that it doesn't occur naturally when
// frames are being faked. E.g. NetEq will not generate this large sample value
// unless it has received an audio frame containing a sample of this value.
// Even simpler buffers would likely just contain audio sample values of 0.

// Same value as src/modules/audio_device/main/source/audio_device_config.h in
// https://code.google.com/p/webrtc/
static const uint32 kAdmMaxIdleTimeProcess = 1000;

// Constants here are derived by running VoE using a real ADM.
// The constants correspond to 10ms of mono audio at 44kHz.
static const uint32_t kTotalDelayMs = 0;
static const int32_t  kClockDriftMs = 0;

GangAudioDevice::GangAudioDevice(shared_ptr<Gang> gang) :
  last_process_time_ms_(0),
  audio_callback_(NULL),
  recording_(false),
  rec_is_initialized_(false),
  gang_(gang),
  len_bytes_per_10ms_(0),
  nb_samples_10ms_(0),
  _recSampleRate(0),
  _recChannels(0),
  _recChannel(AudioDeviceModule::kChannelBoth),
  _recBytesPerSample(0),
  _currentMicLevel(0),
  _newMicLevel(0),
  _typingStatus(false),
  _totalDelayMS(kTotalDelayMs),
  _clockDrift(kClockDriftMs),
  _record_index(0) {
  memset(rec_buff_, 0, kMaxBufferSizeBytes);
  SPDLOG_TRACE(console, "{}", __func__)
}

GangAudioDevice::~GangAudioDevice() {
  SPDLOG_TRACE(console, "{}", __func__)
  rtc::CritScope cs(&lock_);

  if (recording_) {
    recording_ = false;
    gang_->SetAudioFrameObserver(NULL, NULL);
  }
  gang_ = NULL;
  SPDLOG_TRACE(console, "{} {}", __func__, "error")
}

rtc::scoped_refptr<GangAudioDevice> GangAudioDevice::Create(shared_ptr<Gang> gang) {
  if (!gang) {
    return NULL;
  }
  rtc::scoped_refptr<GangAudioDevice> capture_module(
    new rtc::RefCountedObject<GangAudioDevice>(gang));
  capture_module->Initialize();
  return capture_module;
}

int64_t GangAudioDevice::TimeUntilNextProcess() {
  //	SPDLOG_TRACE(console, "{}", __func__)
  const uint32 current_time = rtc::Time();

  if (current_time < last_process_time_ms_) {
    // TODO: wraparound could be handled more gracefully.
    return 0;
  }
  const uint32 elapsed_time = current_time - last_process_time_ms_;

  if (kAdmMaxIdleTimeProcess < elapsed_time) {
    return 0;
  }
  return kAdmMaxIdleTimeProcess - elapsed_time;
}

int32_t GangAudioDevice::Process() {
  //	SPDLOG_TRACE(console, "{}", __func__)
  last_process_time_ms_ = rtc::Time();
  return 0;
}

int32_t GangAudioDevice::ActiveAudioLayer(AudioLayer* audio_layer) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

webrtc::AudioDeviceModule::ErrorCode GangAudioDevice::LastError() const {
  SPDLOG_TRACE(console, "{}", __func__)
  return webrtc::AudioDeviceModule::kAdmErrNone;
}

int32_t GangAudioDevice::RegisterEventObserver(webrtc::AudioDeviceObserver* event_callback) {
  // Only used to report warnings and errors. This fake implementation won't
  // generate any so discard this callback.
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::RegisterAudioCallback(webrtc::AudioTransport* audio_callback) {
  rtc::CritScope cs(&lockCb_);

  audio_callback_ = audio_callback;
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::Init() {
  SPDLOG_TRACE(console, "{}", __func__)

  // Initialize is called by the factory method. Safe to ignore this Init call.
  return 0;
}

int32_t GangAudioDevice::Terminate() {
  SPDLOG_TRACE(console, "{}", __func__)

  // Clean up in the destructor. No action here, just success.
  return 0;
}

bool GangAudioDevice::Initialized() const {
  SPDLOG_TRACE(console, "{}", __func__)
  return gang_.get() != NULL;
}

int16_t GangAudioDevice::PlayoutDevices() {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int16_t GangAudioDevice::RecordingDevices() {
  SPDLOG_TRACE(console, "{}", __func__)
  return 1;
}

int32_t GangAudioDevice::PlayoutDeviceName(
  uint16_t index,
  char     name[webrtc::kAdmMaxDeviceNameSize],
  char     guid[webrtc::kAdmMaxGuidSize]) {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::RecordingDeviceName(
  uint16_t index,
  char     name[kAdmMaxDeviceNameSize],
  char     guid[kAdmMaxGuidSize]) {
  SPDLOG_TRACE(console, "{}", __func__)

  // TODO give device a actual name
  const char* kName = "gang_audio_device";
  const char* kGuid = "gang_audio_unique_id";

  if (index < 1) {
    memset(name, 0, kAdmMaxDeviceNameSize);
    memset(guid, 0, kAdmMaxGuidSize);
    memcpy(name, kName, strlen(kName));
    memcpy(guid, kGuid, strlen(guid));
    return 0;
  }
  return -1;
}

int32_t GangAudioDevice::SetPlayoutDevice(uint16_t index) {
  // No playout device, just playing from file. Return success.
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType device) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SetRecordingDevice(uint16_t index) {
  SPDLOG_TRACE(console, "{}", __func__)

  if (index == 0) {
    _record_index = index;
    return _record_index;
  }
  return -1;
}

int32_t GangAudioDevice::SetRecordingDevice(AudioDeviceModule::WindowsDeviceType device) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::PlayoutIsAvailable(bool* available) {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::InitPlayout() {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

bool GangAudioDevice::PlayoutIsInitialized() const {
  SPDLOG_TRACE(console, "{}", __func__)
  return true;
}

int32_t GangAudioDevice::RecordingIsAvailable(bool* available) {
  SPDLOG_TRACE(console, "{}", __func__)

  if (_record_index == 0) {
    *available = true;
    return _record_index;
  }
  *available = false;
  return -1;
}

int32_t GangAudioDevice::InitRecording() {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

bool GangAudioDevice::RecordingIsInitialized() const {
  SPDLOG_TRACE(console, "{}", __func__)
  return true;
}

int32_t GangAudioDevice::StartPlayout() {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::StopPlayout() {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

bool GangAudioDevice::Playing() const {
  SPDLOG_TRACE(console, "{}", __func__)
  return true;
}

// TODO StartRecording
int32_t GangAudioDevice::StartRecording() {
  if (!rec_is_initialized_) {
    return -1;
  }
  SPDLOG_DEBUG(console, "{}", __func__)
  rtc::CritScope cs(&lock_);
  recording_ = true;
  gang_->SetAudioFrameObserver(this, rec_buff_);
  return 0;
}

// TODO StopRecording
int32_t GangAudioDevice::StopRecording() {
  SPDLOG_TRACE(console, "{}", __func__)
  rtc::CritScope cs(&lock_);

  if (recording_) {
    gang_->SetAudioFrameObserver(NULL, NULL);
  }
  recording_ = false;
  return 0;
}

bool GangAudioDevice::Recording() const {
  SPDLOG_TRACE(console, "{}", __func__)
  rtc::CritScope cs(&lock_);
  return recording_;
}

int32_t GangAudioDevice::SetAGC(bool enable) {
  SPDLOG_TRACE(console, "{}", __func__)

  // No Automatic gain control
  return -1;
}

bool GangAudioDevice::AGC() const {
  SPDLOG_TRACE(console, "{}", __func__)
  return false;
}

int32_t GangAudioDevice::SetWaveOutVolume(uint16_t volumeLeft, uint16_t volumeRight) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::WaveOutVolume(uint16_t* volume_left, uint16_t* volume_right) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::InitSpeaker() {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

bool GangAudioDevice::SpeakerIsInitialized() const {
  SPDLOG_TRACE(console, "{}", __func__)
  return false;
}

int32_t GangAudioDevice::InitMicrophone() {
  SPDLOG_TRACE(console, "{}", __func__)

  // No microphone, just playing from file. Return success.
  return 0;
}

bool GangAudioDevice::MicrophoneIsInitialized() const {
  SPDLOG_TRACE(console, "{}", __func__)
  return true;
}

int32_t GangAudioDevice::SpeakerVolumeIsAvailable(bool* available) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SetSpeakerVolume(uint32_t /*volume*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SpeakerVolume(uint32_t* /*volume*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MaxSpeakerVolume(uint32_t* /*max_volume*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MinSpeakerVolume(uint32_t* /*min_volume*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SpeakerVolumeStepSize(uint16_t* /*step_size*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MicrophoneVolumeIsAvailable(bool* available) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SetMicrophoneVolume(uint32_t volume) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MicrophoneVolume(uint32_t* volume) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MaxMicrophoneVolume(uint32_t* max_volume) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MinMicrophoneVolume(uint32_t* /*min_volume*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MicrophoneVolumeStepSize(uint16_t* /*step_size*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SpeakerMuteIsAvailable(bool* /*available*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SetSpeakerMute(bool /*enable*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SpeakerMute(bool* /*enabled*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MicrophoneMuteIsAvailable(bool* /*available*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SetMicrophoneMute(bool /*enable*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MicrophoneMute(bool* /*enabled*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MicrophoneBoostIsAvailable(bool* /*available*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SetMicrophoneBoost(bool /*enable*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::MicrophoneBoost(bool* /*enabled*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::StereoPlayoutIsAvailable(bool* available) const {
  SPDLOG_TRACE(console, "{}", __func__)

  // No recording device, just dropping audio. Stereo can be dropped just
  // as easily as mono.
  * available = true;
  return 0;
}

int32_t GangAudioDevice::SetStereoPlayout(bool /*enable*/) {
  SPDLOG_TRACE(console, "{}", __func__)

  // No recording device, just dropping audio. Stereo can be dropped just
  // as easily as mono.
  return 0;
}

int32_t GangAudioDevice::StereoPlayout(bool* /*enabled*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::StereoRecordingIsAvailable(bool* available) const {
  SPDLOG_TRACE(console, "{}", __func__)

  // Keep thing simple. No stereo recording.
  * available = true;
  return 0;
}

int32_t GangAudioDevice::SetStereoRecording(bool enable) {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::StereoRecording(bool* enabled) const {
  SPDLOG_TRACE(console, "{}", __func__)
  * enabled = true;
  return 0;
}

int32_t GangAudioDevice::SetRecordingChannel(const ChannelType channel) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::RecordingChannel(ChannelType* channel) const {
  SPDLOG_TRACE(console, "{}", __func__)
  * channel = _recChannel;
  return 0;
}

int32_t GangAudioDevice::SetPlayoutBuffer(const BufferType /*type*/, uint16_t /*size_ms*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::PlayoutBuffer(BufferType* /*type*/, uint16_t* /*size_ms*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::PlayoutDelay(uint16_t* delay_ms) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::RecordingDelay(uint16_t* /*delay_ms*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::CPULoad(uint16_t* /*load*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::StartRawOutputFileRecording(
  const char /*pcm_file_name_utf8*/[webrtc::kAdmMaxFileNameSize]) {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::StopRawOutputFileRecording() {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::StartRawInputFileRecording(
  const char /*pcm_file_name_utf8*/[webrtc::kAdmMaxFileNameSize]) {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::StopRawInputFileRecording() {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::SetRecordingSampleRate(const uint32_t /*samples_per_sec*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::RecordingSampleRate(uint32_t* samples_per_sec) const {
  SPDLOG_TRACE(console, "{}", __func__)

  if (_recSampleRate == 0) {
    return -1;
  }
  *samples_per_sec = _recSampleRate;
  return 0;
}

int32_t GangAudioDevice::SetPlayoutSampleRate(const uint32_t /*samples_per_sec*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return 0;
}

int32_t GangAudioDevice::PlayoutSampleRate(uint32_t* /*samples_per_sec*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  ASSERT(false);
  return 0;
}

int32_t GangAudioDevice::ResetAudioDevice() {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::SetLoudspeakerStatus(bool /*enable*/) {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

int32_t GangAudioDevice::GetLoudspeakerStatus(bool* /*enabled*/) const {
  SPDLOG_TRACE(console, "{}", __func__)
  return -1;
}

void GangAudioDevice::Initialize() {
  SPDLOG_TRACE(console, "{}", __func__)
  last_process_time_ms_ = rtc::Time();

  gang_->GetAudioInfo(&_recSampleRate, &_recChannels);

  // 16 bits per sample in mono, 32 bits in stereo
  _recBytesPerSample = 2 * static_cast<size_t>(_recChannels);

  // must fix to rate
  nb_samples_10ms_ = static_cast<size_t>(_recSampleRate) / 100;

  switch (_recChannels) {
    case 2:
      _recChannel = AudioDeviceModule::kChannelBoth;
      break;

    case 1:
      _recChannel = AudioDeviceModule::kChannelRight;
      break;

    default:
      rec_is_initialized_ = false;
      return;
  }

  SPDLOG_DEBUG(
    console,
    "For webrtc, rate: {}, channels: {}, bytesPerSample: {}",
    static_cast<int>(_recSampleRate),
    static_cast<int>(_recChannels),
    static_cast<int>(_recBytesPerSample));

  // init rec_rest_buff_ 10ms container
  len_bytes_per_10ms_ = nb_samples_10ms_ * _recBytesPerSample;

  rec_is_initialized_ = true;
}

void GangAudioDevice::OnGangFrame() {
  if (!audio_callback_) {
    return;
  }
  DeliverRecordedData();
}

// ----------------------------------------------------------------------------
//  DeliverRecordedData
// ----------------------------------------------------------------------------

int32_t GangAudioDevice::DeliverRecordedData() {
  uint32_t newMicLevel(0);
  int32_t  res = audio_callback_->RecordedDataIsAvailable(
    static_cast<void*>(rec_buff_),

    // need interleaved data 10ms samples
    nb_samples_10ms_,
    _recBytesPerSample,
    _recChannels,
    _recSampleRate,
    _totalDelayMS,
    _clockDrift,
    _currentMicLevel,
    _typingStatus,
    newMicLevel);

  if (res != -1) {
    _newMicLevel = newMicLevel;
  }

  return 0;
}
} // namespace gang
