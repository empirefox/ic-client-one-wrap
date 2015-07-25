#ifdef NO_CGO

#include <ostream>
#include "cgo.h"

void go_send_to_peer(void* goPcPtr_, char* msg) {
	printf("go_send_to_peer: %s\n", msg);
}

void go_on_gang_status(void* goConductorPtr_, char* id, unsigned int status) {
	printf("go_on_status: id(%s) status(%d)\n", id, status);
}

#endif // NO_CGO
