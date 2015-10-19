/*
 * shared.h
 *
 *  Created on: May 1, 2015
 *      Author: savage
 */

#ifndef SHARED_H_
#define SHARED_H_

#include <map>
#include "talk/app/webrtc/peerconnectioninterface.h"

#include "composed_pc_factory.h"
#include "fakeconstraints.h"
#include "ipcam_info.h"

namespace one {

class Peer;

using std::map;
using std::string;
using std::shared_ptr;

using webrtc::PeerConnectionInterface;
using rtc::Thread;

typedef shared_ptr<ComposedPCFactory> Factoty;

class Shared: public gang::StatusObserver {
public:
	Shared(void* goConductorPtr, bool dtls);
	virtual ~Shared();

	virtual void OnStatusChange(const std::string& id, gang::GangStatus status);

	Peer* CreatePeer(const std::string url, void* goPcPtr);
	void DeletePeer(Peer* pc);

	void AddIceServer(string uri, string name, string psd);

	// Must after AddIceServer
	// Will be used in go
	int AddPeerConnectionFactory(
			ipcam_info* info,
			const string& id,
			const string& url,
			const string& rec_name,
			bool rec_enabled,
			bool audio_off);
	Factoty GetPeerConnectionFactory(const string& url);

	PeerConnectionInterface::IceServers IceServers;
	webrtc::FakeConstraints Constraints;
	Thread* MainThread;
	Thread* SignalingThread;

	mutable rtc::CriticalSection peer_lock_;
	mutable rtc::CriticalSection factories_lock_;

private:
	void InitConstraintsOnce(bool dtls);

	map<string, Factoty> factories_;
	void* goConductorPtr_;
};

} // namespace one

#endif /* SHARED_H_ */
