#pragma once

#include <memory>
#include "webrtc/base/basictypes.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/base/criticalsection.h"
#include "gang.h"

namespace rtc {
class Thread;
} // namespace rtc

using std::shared_ptr;
using webrtc::AudioDeviceModule;
using webrtc::AudioTransport;
using webrtc::AudioDeviceObserver;
using webrtc::kAdmMaxDeviceNameSize;
using webrtc::kAdmMaxFileNameSize;
using webrtc::kAdmMaxGuidSize;

namespace gang {
const uint32_t kMaxBufferSizeBytes = 3840; // 10ms in stereo @ 96kHz

class GangAudioDevice : public AudioDeviceModule, public GangFrameObserver {
public:
  // Creates a GangAudioDevice or returns NULL on failure.
  // |process_thread| is used to push and pull audio frames to and from the
  // returned instance. Note: ownership of |process_thread| is not handed over.
  static rtc::scoped_refptr<GangAudioDevice> Create(shared_ptr<Gang> gang);

  // Following functions are inherited from webrtc::AudioDeviceModule.
  // Only functions called by PeerConnection are implemented, the rest do
  // nothing and return success. If a function is not expected to be called by
  // PeerConnection an assertion is triggered if it is in fact called.
  int64_t                      TimeUntilNextProcess() override;
  int32_t                      Process() override;

  int32_t                      ActiveAudioLayer(AudioDeviceModule::AudioLayer* audio_layer) const override;

  AudioDeviceModule::ErrorCode LastError() const override;
  int32_t                      RegisterEventObserver(webrtc::AudioDeviceObserver* event_callback) override;

  // Note: Calling this method from a callback may result in deadlock.
  int32_t                      RegisterAudioCallback(webrtc::AudioTransport* audio_callback) override;

  int32_t                      Init() override;
  int32_t                      Terminate() override;
  bool                         Initialized() const override;

  int16_t                      PlayoutDevices() override;
  int16_t                      RecordingDevices() override;

  int32_t PlayoutDeviceName(uint16_t index,
                            char name[webrtc::kAdmMaxDeviceNameSize],
                            char guid[webrtc::kAdmMaxGuidSize]) override;

  int32_t RecordingDeviceName(uint16_t index,
                              char name[webrtc::kAdmMaxDeviceNameSize],
                              char guid[webrtc::kAdmMaxGuidSize]) override;

  int32_t SetPlayoutDevice(uint16_t index) override;
  int32_t SetPlayoutDevice(AudioDeviceModule::WindowsDeviceType device) override;
  int32_t SetRecordingDevice(uint16_t index) override;
  int32_t SetRecordingDevice(AudioDeviceModule::WindowsDeviceType device) override;

  int32_t PlayoutIsAvailable(bool* available) override;
  int32_t InitPlayout() override;
  bool    PlayoutIsInitialized() const override;
  int32_t RecordingIsAvailable(bool* available) override;
  int32_t InitRecording() override;
  bool    RecordingIsInitialized() const override;

  int32_t StartPlayout() override;
  int32_t StopPlayout() override;
  bool    Playing() const override;
  int32_t StartRecording() override;
  int32_t StopRecording() override;
  bool    Recording() const override;

  int32_t SetAGC(bool enable) override;
  bool    AGC() const override;

  int32_t SetWaveOutVolume(uint16_t volume_left,
                           uint16_t volume_right) override;
  int32_t WaveOutVolume(uint16_t* volume_left,
                        uint16_t* volume_right) const override;

  int32_t InitSpeaker() override;
  bool    SpeakerIsInitialized() const override;
  int32_t InitMicrophone() override;
  bool    MicrophoneIsInitialized() const override;

  int32_t SpeakerVolumeIsAvailable(bool* available) override;
  int32_t SetSpeakerVolume(uint32_t volume) override;
  int32_t SpeakerVolume(uint32_t* volume) const override;
  int32_t MaxSpeakerVolume(uint32_t* max_volume) const override;
  int32_t MinSpeakerVolume(uint32_t* min_volume) const override;
  int32_t SpeakerVolumeStepSize(uint16_t* step_size) const override;

  int32_t MicrophoneVolumeIsAvailable(bool* available) override;
  int32_t SetMicrophoneVolume(uint32_t volume) override;
  int32_t MicrophoneVolume(uint32_t* volume) const override;
  int32_t MaxMicrophoneVolume(uint32_t* max_volume) const override;

  int32_t MinMicrophoneVolume(uint32_t* min_volume) const override;
  int32_t MicrophoneVolumeStepSize(uint16_t* step_size) const override;

  int32_t SpeakerMuteIsAvailable(bool* available) override;
  int32_t SetSpeakerMute(bool enable) override;
  int32_t SpeakerMute(bool* enabled) const override;

  int32_t MicrophoneMuteIsAvailable(bool* available) override;
  int32_t SetMicrophoneMute(bool enable) override;
  int32_t MicrophoneMute(bool* enabled) const override;

  int32_t MicrophoneBoostIsAvailable(bool* available) override;
  int32_t SetMicrophoneBoost(bool enable) override;
  int32_t MicrophoneBoost(bool* enabled) const override;

  int32_t StereoPlayoutIsAvailable(bool* available) const override;
  int32_t SetStereoPlayout(bool enable) override;
  int32_t StereoPlayout(bool* enabled) const override;
  int32_t StereoRecordingIsAvailable(bool* available) const override;
  int32_t SetStereoRecording(bool enable) override;
  int32_t StereoRecording(bool* enabled) const override;
  int32_t SetRecordingChannel(const AudioDeviceModule::ChannelType channel) override;
  int32_t RecordingChannel(AudioDeviceModule::ChannelType* channel) const override;

  int32_t SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                           uint16_t                            size_ms = 0) override;
  int32_t PlayoutBuffer(AudioDeviceModule::BufferType* type,
                        uint16_t*                      size_ms) const override;
  int32_t PlayoutDelay(uint16_t* delay_ms) const override;
  int32_t RecordingDelay(uint16_t* delay_ms) const override;

  int32_t CPULoad(uint16_t* load) const override;

  int32_t StartRawOutputFileRecording(const char pcm_file_name_utf8[webrtc::kAdmMaxFileNameSize])  override;
  int32_t StopRawOutputFileRecording() override;
  int32_t StartRawInputFileRecording(const char pcm_file_name_utf8[webrtc::kAdmMaxFileNameSize])  override;
  int32_t StopRawInputFileRecording() override;

  int32_t SetRecordingSampleRate(const uint32_t samples_per_sec) override;
  int32_t RecordingSampleRate(uint32_t* samples_per_sec) const override;
  int32_t SetPlayoutSampleRate(const uint32_t samples_per_sec) override;
  int32_t PlayoutSampleRate(uint32_t* samples_per_sec) const override;

  int32_t ResetAudioDevice() override;
  int32_t SetLoudspeakerStatus(bool enable) override;
  int32_t GetLoudspeakerStatus(bool* enabled) const override;


  // AEC
  virtual bool    BuiltInAECIsAvailable() const {return false;}

  virtual int32_t EnableBuiltInAEC(bool enable) {return -1;}

  // End of functions inherited from webrtc::AudioDeviceModule.

  int32_t      DeliverRecordedData();

  virtual void OnGangFrame() override;

  // The destructor is protected because it is reference counted and should not
  // be deleted directly.
  virtual ~GangAudioDevice();

protected:
  // The constructor is protected because the class needs to be created as a
  // reference counted object (for memory managment reasons). It could be
  // exposed in which case the burden of proper instantiation would be put on
  // the creator of a GangAudioDevice instance. To create an instance of
  // this class use the Create(..) API.
  explicit GangAudioDevice(shared_ptr<Gang> gang);

private:
  void Initialize();

  // The time in milliseconds when Process() was last called or 0 if no call
  // has been made.
  uint32 last_process_time_ms_;

  // Callback for playout and recording.
  webrtc::AudioTransport* audio_callback_;

  bool recording_;          // True when audio is being pushed from the
                            // instance.

  bool rec_is_initialized_; // True when the instance is ready to push audio.

  // User provided thread context.
  shared_ptr<Gang> gang_;

  // 10ms in stereo @ 96kHz
  uint8_t rec_buff_[kMaxBufferSizeBytes];
  size_t  len_bytes_per_10ms_;
  size_t  nb_samples_10ms_;

  mutable rtc::CriticalSection lock_;
  mutable rtc::CriticalSection lockCb_;

  uint32_t _recSampleRate;
  uint8_t  _recChannels;

  // selected recording channel (left/right/both)
  AudioDeviceModule::ChannelType _recChannel;

  // 2 or 4 depending on mono or stereo
  // do not equal BytesPerSample in ffmpeg
  size_t _recBytesPerSample;

  uint32_t _currentMicLevel;
  uint32_t _newMicLevel;

  bool _typingStatus;

  uint32_t _totalDelayMS;
  int32_t  _clockDrift;

  uint16_t _record_index;
};
} // namespace gang
