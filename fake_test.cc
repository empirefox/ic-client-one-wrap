#include <ostream>

#include "fake_test.h"

#ifdef NO_CGO

void go_msg_error(char* msg) {
	printf("%s\n", msg);
}
void go_msg_info(char* msg) {
	printf("%s\n", msg);
}
void go_msg_warning(char* msg) {
	printf("%s\n", msg);
}

void go_send_to_peer(void* pc, char* msg) {
	printf("go_send_to_peer: %s\n", msg);
}

#endif //NO_CGO