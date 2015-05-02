#ifndef CGO_H_
#define CGO_H_
#pragma once

#ifdef NO_CGO
#include <ostream>
extern void go_send_to_peer(void* goPcPtr_, char* msg);
void go_send_to_peer(void* goPcPtr_, char* msg) {
	printf("go_send_to_peer: %s\n", msg);
}
#else
extern "C" {
#include "_cgo_export.h"
}
#endif // NO_CGO

#endif // CGO_H_
