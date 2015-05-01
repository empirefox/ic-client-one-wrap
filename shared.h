/*
 * shared.h
 *
 *  Created on: May 1, 2015
 *      Author: savage
 */

#ifndef SHARED_H_
#define SHARED_H_

#include "talk/app/webrtc/peerconnectioninterface.h"
#include "fakeconstraints.h"
#include "composed_peer_connection_factory.h"

namespace one {

using std::map;
using std::string;
using std::shared_ptr;

using webrtc::PeerConnectionInterface;
using rtc::Thread;

class Shared {
public:
	Shared(bool dtls);
	~Shared();

	void AddIceServer(string uri, string name, string psd);

	// Must after AddIceServer
	// Will be used in go
	int AddPeerConnectionFactory(const std::string url);
	shared_ptr<ComposedPeerConnectionFactory> GetPeerConnectionFactory(
			const string& url);

	PeerConnectionInterface::IceServers IceServers;
	webrtc::FakeConstraints Constraints;
	Thread* SignalingThread;

private:
	void InitConstraintsOnce(bool dtls);

	map<string, shared_ptr<ComposedPeerConnectionFactory> > factories_;
};

} // namespace one

#endif /* SHARED_H_ */
