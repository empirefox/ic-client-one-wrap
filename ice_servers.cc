#include <cstddef>

#include "webrtc/base/common.h"

extern "C" {
#include "ice_servers.h"
}
#include "ice_servers.hh"

webrtc::PeerConnectionInterface::IceServers g_IceServers;

// used by golang
void AddIceServer(const char *uri, const char *name, const char *psd) {
	ASSERT(uri != NULL);

	std::string suri(uri);
	std::string sname(name);
	std::string spsd(psd);

	webrtc::PeerConnectionInterface::IceServer server;
	server.uri = suri;
	if (!sname.empty()) {
		server.username = sname;
	}
	if (!spsd.empty()) {
		server.password = spsd;
	}
	g_IceServers.push_back(server);
}
