#include "talk/media/base/videocapturerfactory.h"

class GangVideoCapturerFactory: public cricket::VideoDeviceCapturerFactory {
public:
	GangVideoCapturerFactory() {
	}
	virtual ~GangVideoCapturerFactory() {
	}

	virtual cricket::VideoCapturer* Create(const cricket::Device& device);
};
