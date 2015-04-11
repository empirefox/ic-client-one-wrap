#include "webrtc/base/bytebuffer.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/logging.h"

#include "gangvideocapturer.hh"
extern "C" {
#include "decode.h"
}

using namespace cricket;

///////////////////////////////////////////////////////////////////////
// Definition of private class FileReadThread that periodically reads
// frames from a file.
///////////////////////////////////////////////////////////////////////

FileReadThread::FileReadThread(FileVideoCapturer* video_capturer) :
		video_capturer_(video_capturer), stopped_(true) {
}

FileReadThread::~FileReadThread() {
	Stop();
}
void FileReadThread::on_audio_callback(uint8_t *data, int size, int64_t pts) {
	// TODO ignored now
	// should move to audio
}

// Override virtual method of parent Thread. Context: Worker Thread.
void FileReadThread::Run() {
	if (video_capturer_) {
		stopped_ = false;
		decode_start_best(reinterpret_cast<void*>(this),
				video_capturer_->GetId().c_str(), on_gang_video_callback,
				on_gang_audio_callback);
	}

	rtc::CritScope cs(&crit_);
	stopped_ = true;
}

// Never call Stop on the current thread.
void FileReadThread::Stop() {
	stop();
	Join();
}

void FileReadThread::stop() {
	rtc::CritScope cs(&crit_);
	stopped_ = true;
}

// Check if Run() is running.
bool FileReadThread::IsRunning() const {
	rtc::CritScope cs(&crit_);
	return !stopped_;
}

int on_gang_video_callback(void* p, uint8_t *data, int size, int64_t pts) {
	FileReadThread* thread_ = reinterpret_cast<FileReadThread*>(p);
	thread_->video_capturer_->on_video_callback(data, size, pts);
	return thread_->stopped_;
}

int on_gang_audio_callback(void* p, uint8_t *data, int size, int64_t pts) {
	FileReadThread* thread_ = reinterpret_cast<FileReadThread*>(p);
	thread_->on_audio_callback(data, size, pts);
	return thread_->stopped_;
}

/////////////////////////////////////////////////////////////////////
// Implementation of class FileVideoCapturer
/////////////////////////////////////////////////////////////////////
static const int64 kNumNanoSecsPerMilliSec = 1000000;
const char* FileVideoCapturer::kVideoFileDevicePrefix = "gang:";

FileVideoCapturer::FileVideoCapturer() :
		frame_buffer_size_(0), file_read_thread_(NULL), start_time_ns_(0) {
}

FileVideoCapturer::~FileVideoCapturer() {
	Stop();
	delete[] static_cast<uint8*>(captured_frame_.data);
}

void FileVideoCapturer::on_video_callback(uint8_t *data_, int size_,
		int64_t pts) {
	// TODO
	SingalGangFrame(reinterpret_cast<uint8*>(data_),
			static_cast<uint32>(size_));
}

void FileVideoCapturer::on_format_callback(int width, int height, int fps) {
	captured_frame_.width = width;
	captured_frame_.height = height;
	captured_frame_.fourcc = cricket::FOURCC_I420;
	// Enumerate the supported formats. We have only one supported format. We set
	// the frame interval to kMinimumInterval here. In Start(), if the capture
	// format's interval is greater than kMinimumInterval, we use the interval;
	// otherwise, we use the timestamp in the file to control the interval.
	VideoFormat format(width, height, cricket::VideoFormat::FpsToInterval(fps),
			cricket::FOURCC_I420);
	std::vector<VideoFormat> supported;
	supported.push_back(format);

	// TODO(thorcarpenter): Report the actual file video format as the supported
	// format. Do not use kMinimumInterval as it conflicts with video adaptation.
	SetSupportedFormats(supported);
}

bool FileVideoCapturer::Init(const Device& device) {
	if (!FileVideoCapturer::IsFileVideoCapturerDevice(device)) {
		return false;
	}
	best_format(reinterpret_cast<void*>(this), GetId().c_str(),
			on_gang_format_callback);

	SetId(device.name);
	// TODO(wuwang): Design an E2E integration test for video adaptation,
	// then remove the below call to disable the video adapter.
	set_enable_video_adapter(false);
	return true;
}

bool FileVideoCapturer::Init(const std::string& filename) {
	return Init(FileVideoCapturer::CreateFileVideoCapturerDevice(filename));
}

CaptureState FileVideoCapturer::Start(const VideoFormat& capture_format) {
	if (IsRunning()) {
		LOG(LS_ERROR) << "The file video capturer is already running";
		return CS_FAILED;
	}

	SetCaptureFormat(&capture_format);
	// Create a thread to read the file.
	file_read_thread_ = new FileReadThread(this);
	start_time_ns_ = kNumNanoSecsPerMilliSec * static_cast<int64>(rtc::Time());
	bool ret = file_read_thread_->Start();
	if (ret) {
		LOG(LS_INFO) << "File video capturer '" << GetId() << "' started";
		return CS_RUNNING;
	} else {
		LOG(LS_ERROR) << "File video capturer '" << GetId()
									<< "' failed to start";
		return CS_FAILED;
	}
}

bool FileVideoCapturer::IsRunning() {
	return file_read_thread_ && file_read_thread_->IsRunning();
}

void FileVideoCapturer::Stop() {
	if (file_read_thread_) {
		file_read_thread_->Stop();
		file_read_thread_ = NULL;
		LOG(LS_INFO) << "File video capturer '" << GetId() << "' stopped";
	}
	SetCaptureFormat(NULL);
}

bool FileVideoCapturer::GetPreferredFourccs(std::vector<uint32>* fourccs) {
	if (!fourccs) {
		return false;
	}

	fourccs->push_back(GetSupportedFormats()->at(0).fourcc);
	return true;
}

void FileVideoCapturer::SingalGangFrame(uint8 *data_, uint32 size_) {
	uint32 start_read_time_ms = rtc::Time();
	// 2.2 Reallocate memory for the frame data if necessary.
	if (frame_buffer_size_ < size_) {
		frame_buffer_size_ = size_;
		delete[] static_cast<uint8*>(captured_frame_.data);
		captured_frame_.data = new uint8[frame_buffer_size_];
	}
	captured_frame_.data_size = size_;

	// 2.3 Read the frame adata.
	memcpy(captured_frame_.data, reinterpret_cast<const uint8*>(data_), size_);

	captured_frame_.time_stamp = kNumNanoSecsPerMilliSec
			* static_cast<int64>(start_read_time_ms);
	captured_frame_.elapsed_time = captured_frame_.time_stamp - start_time_ns_;

	SignalFrameCaptured(this, &captured_frame_);
}

void on_gang_format_callback(void* p, int width, int height, int fps) {
	reinterpret_cast<FileVideoCapturer*>(p)->on_format_callback(width, height,
			fps);
}
