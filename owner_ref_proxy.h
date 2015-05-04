/*
 * owner_ref_proxy.h
 *
 *  Created on: May 4, 2015
 *      Author: savage
 */

#ifndef OWNER_REF_PROXY_H_
#define OWNER_REF_PROXY_H_

#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/refcount.h"
#include "talk/app/webrtc/videosourceinterface.h"

namespace one {

class RefOwner {
public:
	virtual ~RefOwner() {
	}
	virtual void SetRef(bool) = 0;
};

class OwnedVideoSourceRef: webrtc::VideoSourceInterface {
protected:
	typedef rtc::scoped_refptr<webrtc::VideoSourceInterface> C;
	typedef RefOwner* O;
	OwnedVideoSourceRef(O o, C c) :
					o_(o),
					c_(c) {
		if (c_.get()) {
			o_->SetRef(true);
		}
	}
	~OwnedVideoSourceRef() {
		Release_s();
		o_->SetRef(false);
	}

	cricket::VideoCapturer* GetVideoCapturer() {
		return c_->GetVideoCapturer();
	}
	void Stop() {
		c_->Stop();
	}
	void Restart() {
		c_->Restart();
	}
	void AddSink(cricket::VideoRenderer* output) {
		c_->AddSink(output);
	}
	void RemoveSink(cricket::VideoRenderer* output) {
		c_->RemoveSink(output);
	}
	const cricket::VideoOptions* options() const {
		return c_->options();
	}
	cricket::VideoRenderer* FrameInput() {
		return c_->FrameInput();
	}
	void RegisterObserver(webrtc::ObserverInterface* observer) {
		c_->RegisterObserver(observer);
	}
	void UnregisterObserver(webrtc::ObserverInterface* observer) {
		c_->UnregisterObserver(observer);
	}
	SourceState state() const {
		return c_->state();
	}

	O o_;
	C c_;

public:
	static rtc::scoped_refptr<webrtc::VideoSourceInterface> Create(O o, C c) {
		return new rtc::RefCountedObject<OwnedVideoSourceRef>(o, c);
	}

private:
	void Release_s() {
		c_ = NULL;
	}
};

} // namespace one

#endif /* OWNER_REF_PROXY_H_ */
