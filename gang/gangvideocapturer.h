#pragma once

#include <memory>
#include "webrtc/base/thread_checker.h"
#include "talk/media/base/videocapturer.h"
#include "gang.h"

using std::shared_ptr;
using cricket::VideoCapturer;
using cricket::VideoFormat;
using cricket::CaptureState;
using cricket::CapturedFrame;

namespace gang {
VideoCapturer* CreateVideoCapturer(shared_ptr<Gang> gang,
                                   rtc::Thread*     thread);

// Onwer signaling thread
// Simulated video capturer that reads frames from a url.
class GangVideoCapturer : public VideoCapturer, public GangFrameObserver {
public:
  explicit GangVideoCapturer(shared_ptr<Gang> gang,
                             rtc::Thread*     thread);
  virtual ~GangVideoCapturer();

  void         Initialize();

  // Override virtual methods of parent class VideoCapturer.
  CaptureState Start(const VideoFormat& capture_format) override;
  void         Stop() override;
  bool         IsRunning() override;
  bool         IsScreencast() const override {return false;}

  // Called from gang when the capturer has been started.
  void         OnVideoStarted(bool success) override;
  void         OnVideoStopped() override;

  // Called from gang when a new frame has been captured.
  // Implements VideoFrameObserver
  // data uint8*
  void OnGangFrame() override;

protected:
  // Override virtual methods of parent class VideoCapturer.
  bool GetPreferredFourccs(std::vector<uint32>* fourccs) override;

private:
  class ThreadHandler;
  void release_s();
  void onVideoStarted_s(cricket::CaptureState new_state);
  void onVideoStopped_s();
  void onGangFrame_s();

  rtc::Thread*                 owner_thread_;
  rtc::Thread*                 start_thread_;
  ThreadHandler*               start_thread_handler_;
  shared_ptr<Gang>             gang_;
  int64                        start_time_ns_; // Time when the capturer starts.
  int64                        drop_interval_;
  bool                         running_;
  bool                         accept_;
  cricket::CaptureState        current_state_;
  CapturedFrame                captured_frame_;
  rtc::ThreadChecker           thread_checker_;
  mutable rtc::CriticalSection crit_;

  RTC_DISALLOW_COPY_AND_ASSIGN(GangVideoCapturer);
};
} // namespace gang
