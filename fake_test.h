#ifndef FAKE_TEST_H_
#define FAKE_TEST_H_
#pragma once

#ifdef NO_CGO

extern void go_msg_error(char* msg);
extern void go_msg_info(char* msg);
extern void go_msg_warning(char* msg);

extern void go_send_to_peer(void* msgChanPtr, char* msg);

#endif //NO_CGO

#endif//FAKE_TEST_H_
