#include "gangvideocapturer.h"

#include "webrtc/base/bind.h"
#include "gang_spdlog_console.h"

namespace gang {
enum {VIDEO_START_OK, VIDEO_START_FAILED, VIDEO_STOPPED};

VideoCapturer* CreateVideoCapturer(shared_ptr<Gang> gang, rtc::Thread* thread) {
  if (!gang.get() || !thread) {
    return NULL;
  }
  rtc::scoped_ptr<GangVideoCapturer> capturer(new GangVideoCapturer(gang, thread));
  if (!capturer.get()) {
    SPDLOG_TRACE(console, "{} {}", __func__, "error")
    return NULL;
  }
  capturer->Initialize();
  return capturer.release();
}

class GangVideoCapturer::ThreadHandler : public rtc::MessageHandler, public sigslot::has_slots<>{
public:
  explicit ThreadHandler(GangVideoCapturer* capture) :
    capture_(capture) {}

  virtual ~ThreadHandler() {}

  virtual void OnMessage(rtc::Message* pmsg) {
    switch (pmsg->message_id) {
      case VIDEO_START_OK:
        capture_->onVideoStarted_s(cricket::CS_RUNNING);
        break;

      case VIDEO_START_FAILED:
        capture_->onVideoStarted_s(cricket::CS_FAILED);
        break;

      case VIDEO_STOPPED:
        capture_->onVideoStopped_s();
        break;

      default:
        ASSERT(false);
        break;
    }
  }

  void OnCpuAdaptationSignalled() {
    SPDLOG_TRACE(console, "{} {}", __func__, "ok")
  }

private:
  GangVideoCapturer* capture_;
};

GangVideoCapturer::GangVideoCapturer(shared_ptr<Gang> gang, rtc::Thread* thread) :
  VideoCapturer(thread),
  owner_thread_(rtc::Thread::Current()),
  start_thread_(NULL),
  start_thread_handler_(new ThreadHandler(this)),
  gang_(gang),
  start_time_ns_(0),
  drop_interval_(0),
  running_(false),
  accept_(false),
  current_state_(cricket::CS_STOPPED) {
  SPDLOG_TRACE(console, "{}", __func__)
}

GangVideoCapturer::~GangVideoCapturer() {
  SPDLOG_TRACE(console, "{}", __func__)
  CHECK(owner_thread_->IsCurrent());
  SignalFrameCaptured.disconnect_all();
  crit_.Enter();
  crit_.Leave();
  SPDLOG_TRACE(console, "{} waited stopped ok", __func__)
  CHECK(!running_);
  delete start_thread_handler_;
  start_thread_handler_ = NULL;

  if (captured_frame_.data) {
    delete[] static_cast<char*>(captured_frame_.data);
    captured_frame_.data = NULL;
  }

  // TODO Why this can fix RollingAccumulator segfault?
  cricket::VariableInfo<int>    adapt_frame_drops;
  cricket::VariableInfo<double> capturer_frame_time;
  cricket::VideoFormat          format;

  GetStats(&adapt_frame_drops, NULL, &capturer_frame_time, &format);
  SPDLOG_TRACE(console, "{} video_adapter", __func__)

  cricket::CoordinatedVideoAdapter * va = video_adapter();
  sigslot::signal0<>& sg = va->SignalCpuAdaptationUnable;
  sg.disconnect_all();
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

void GangVideoCapturer::Initialize() {
  int width;
  int height;
  int fps;

  gang_->GetVideoInfo(&width, &height, &fps, &captured_frame_.data_size);
  drop_interval_ = cricket::VideoFormat::FpsToInterval(fps) * 2 / 3;

  captured_frame_.fourcc       = cricket::FOURCC_I420;
  captured_frame_.pixel_height = 1;
  captured_frame_.pixel_width  = 1;
  captured_frame_.width        = width;
  captured_frame_.height       = height;

  //	captured_frame_.data_size =
  // static_cast<uint32>(cricket::VideoFrame::SizeOf(width, height));
  captured_frame_.data = new char[captured_frame_.data_size];
  SPDLOG_TRACE(console, "ffmpeg size: {}", captured_frame_.data_size)
  SPDLOG_TRACE(console, "webrtc size: {}", cricket::VideoFrame::SizeOf(width, height))

  // Enumerate the supported formats. We have only one supported format. We set
  // the frame interval to kMinimumInterval here. In Start(), if the capture
  // format's interval is greater than kMinimumInterval, we use the interval;
  // otherwise, we use the timestamp in the file to control the interval.
  // TODO change supper fps to that from stream
  cricket::VideoFormat     format(width, height, cricket::VideoFormat::FpsToInterval(fps), cricket::FOURCC_I420);
  std::vector<VideoFormat> supported;
  supported.push_back(format);
  SetSupportedFormats(supported);
  SetApplyRotation(false);

  // TODO(wuwang): Design an E2E integration test for video adaptation,
  // then remove the below call to disable the video adapter.
  set_enable_video_adapter(false);
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

CaptureState GangVideoCapturer::Start(const VideoFormat& capture_format) {
  thread_checker_.DetachFromThread();
  start_thread_ = rtc::Thread::Current();
  CHECK(!running_);
  SetCaptureFormat(&capture_format);

  accept_  = true;
  running_ = true;
  gang_->StartVideoCapture(this, static_cast<uint8_t*>(captured_frame_.data));
  SPDLOG_TRACE(console, "{}: {}", __func__, "sent")
  current_state_ = cricket::CS_STARTING;
  return current_state_;
}

void GangVideoCapturer::Stop() {
  CHECK(thread_checker_.CalledOnValidThread());
  accept_ = false;
  crit_.Enter();
  CHECK(running_);
  gang_->StopVideoCapture(this);

  //	current_state_ = cricket::CS_STOPPED;
  //	SignalStateChange(this, current_state_);
  SPDLOG_TRACE(console, "{} {}", __func__, "sent")
}

bool GangVideoCapturer::IsRunning() {
  SPDLOG_TRACE(console, "{}", __func__)
  CHECK(thread_checker_.CalledOnValidThread());
  return running_;
}

bool GangVideoCapturer::GetPreferredFourccs(std::vector<uint32>* fourccs) {
  //	CHECK(thread_checker_.CalledOnValidThread());
  fourccs->clear();
  fourccs->push_back(GetSupportedFormats()->at(0).fourcc);
  return true;
}

void GangVideoCapturer::OnVideoStarted(bool success) {
  start_thread_->Post(start_thread_handler_, success ? VIDEO_START_OK : VIDEO_START_FAILED);
}

void GangVideoCapturer::onVideoStarted_s(cricket::CaptureState new_state) {
  SPDLOG_TRACE(console, "{}", __func__)
  CHECK(thread_checker_.CalledOnValidThread());

  if (new_state == current_state_) return;
  current_state_ = new_state;
  SetCaptureState(new_state);
  start_time_ns_ = static_cast<int64>(rtc::TimeNanos());
  SPDLOG_TRACE(console, "{} {}", __func__, "ok")
}

void GangVideoCapturer::OnVideoStopped() {
  start_thread_->Post(start_thread_handler_, VIDEO_STOPPED);
}

void GangVideoCapturer::onVideoStopped_s() {
  CHECK(thread_checker_.CalledOnValidThread());
  running_ = false;
  SetCaptureFormat(NULL);
  onVideoStarted_s(cricket::CS_STOPPED);
  crit_.Leave();
  SPDLOG_TRACE(console, "{} indeed stopped", __func__)
}

void GangVideoCapturer::OnGangFrame() {
  start_thread_->Invoke<void>(Bind(&GangVideoCapturer::onGangFrame_s, this));
}

void GangVideoCapturer::onGangFrame_s() {
  CHECK(thread_checker_.CalledOnValidThread());

  if (!accept_) {
    return;
  }
  int64 n = static_cast<int64>(rtc::TimeNanos());

  if (n - captured_frame_.time_stamp < drop_interval_) {
    return;
  }
  captured_frame_.time_stamp   = n;
  captured_frame_.elapsed_time = captured_frame_.time_stamp - start_time_ns_;

  SignalFrameCaptured(this, &captured_frame_);
}
} // namespace gang
