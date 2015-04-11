#ifndef TALK_MEDIA_DEVICES_FILEVIDEOCAPTURER_H_
#define TALK_MEDIA_DEVICES_FILEVIDEOCAPTURER_H_

#include <string>
#include <vector>

#include "talk/media/base/videocapturer.h"
#include "webrtc/base/stream.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/base/thread.h"

using namespace cricket;

class FileVideoCapturer;

class FileReadThread: public rtc::Thread {
public:

	friend int on_gang_video_callback(void* p, uint8_t *data, int size,
			int64_t pts);
	friend int on_gang_audio_callback(void* p, uint8_t *data, int size,
			int64_t pts);

	explicit FileReadThread(FileVideoCapturer* video_capturer);
	virtual ~FileReadThread();

	void on_audio_callback(uint8_t *data, int size, int64_t pts);
	virtual void Run();
	virtual void Stop();
	bool IsRunning() const;

private:
	void stop();
	FileVideoCapturer* video_capturer_;
	mutable rtc::CriticalSection crit_;
	bool stopped_;

	DISALLOW_COPY_AND_ASSIGN(FileReadThread);
};

int on_gang_video_callback(void* p, uint8_t *data, int size, int64_t pts);
int on_gang_audio_callback(void* p, uint8_t *data, int size, int64_t pts);

// Simulated video capturer that periodically reads frames from a file.
class FileVideoCapturer: public VideoCapturer {
public:
	static const int kForever = -1;

	FileVideoCapturer();
	virtual ~FileVideoCapturer();

	// Determines if the given device is actually a video file, to be captured
	// with a FileVideoCapturer.
	static bool IsFileVideoCapturerDevice(const Device& device) {
		return rtc::starts_with(device.id.c_str(), kVideoFileDevicePrefix);
	}

	// Creates a fake device for the given filename.
	static Device CreateFileVideoCapturerDevice(const std::string& filename) {
		std::stringstream id;
		id << kVideoFileDevicePrefix << filename;
		return Device(filename, id.str());
	}

	// Initializes the capturer with the given file.
	bool Init(const std::string& filename);

	// Initializes the capturer with the given device. This should only be used
	// if IsFileVideoCapturerDevice returned true for the given device.
	bool Init(const Device& device);

	// Override virtual methods of parent class VideoCapturer.
	virtual CaptureState Start(const VideoFormat& capture_format);
	virtual void Stop();
	virtual bool IsRunning();
	virtual bool IsScreencast() const {
		return false;
	}

	void on_video_callback(uint8_t *data, int size, int64_t pts);
	friend void on_gang_format_callback(void* p, int width, int height,
			int fps);
	void on_format_callback(int width, int height, int fps);

protected:
	// Override virtual methods of parent class VideoCapturer.
	virtual bool GetPreferredFourccs(std::vector<uint32>* fourccs);

	void SingalGangFrame(uint8 *data_, uint32 size_);

	// Return the CapturedFrame - useful for extracting contents after reading
	// a frame. Should be used only while still reading a file (i.e. only while
	// the CapturedFrame object still exists).
	const CapturedFrame* frame() const {
		return &captured_frame_;
	}

private:

	static const char* kVideoFileDevicePrefix;
	CapturedFrame captured_frame_;
	// The number of bytes allocated buffer for captured_frame_.data.
	uint32 frame_buffer_size_;
	FileReadThread* file_read_thread_;
	int64 start_time_ns_;  // Time when the file video capturer starts.

	DISALLOW_COPY_AND_ASSIGN(FileVideoCapturer);
};

void on_gang_format_callback(void* p, int width, int height, int fps);

#endif  // TALK_MEDIA_DEVICES_FILEVIDEOCAPTURER_H_
