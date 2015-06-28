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
#include "fakeconstraints.h"
#include "composed_peer_connection_factory.h"

namespace one {

class Peer;

using std::map;
using std::string;
using std::unique_ptr;
using std::make_shared;

using webrtc::PeerConnectionInterface;
using rtc::Thread;

enum SignalType {
	RemoteOfferSignal, RemoteCandidateSignal, DeletePeerSignal, DeleteFactoriesSignal
};

class Shared: public rtc::MessageHandler {
public:
	Shared(bool dtls);
	~Shared();

	Peer* CreatePeer(const std::string url, void* goPcPtr);
	void DeletePeer(Peer* pc);

	void AddIceServer(string uri, string name, string psd);

	// Must after AddIceServer
	// Will be used in go
	int AddPeerConnectionFactory(const string& url, const string& rec_name, bool rec_enabled);
	shared_ptr<ComposedPeerConnectionFactory> GetPeerConnectionFactory(const string& url);

	// implements the MessageHandler interface
	void OnMessage(rtc::Message* msg);

	PeerConnectionInterface::IceServers IceServers;
	webrtc::FakeConstraints Constraints;
	Thread* SignalingThread;

private:
	void InitConstraintsOnce(bool dtls);

	map<string, shared_ptr<ComposedPeerConnectionFactory> > factories_;
};

} // namespace one

#endif /* SHARED_H_ */
