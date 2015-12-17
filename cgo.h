#pragma once

#ifdef NO_CGO

extern void go_send_to_peer(void* goPcPtr_,
                            char* msg);

extern void go_on_gang_status(void*        goConductorPtr_,
                              char*        id,
                              unsigned int status);

#else // ifdef NO_CGO
extern "C" {
#include "_cgo_export.h"
}
#endif // NO_CGO
