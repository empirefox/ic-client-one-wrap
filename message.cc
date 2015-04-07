#include "fake_test.h"
#include "message.h"

#include "cgo.h"

void Msg::Error(const std::string& msg) {
	go_msg_error(const_cast<char*>(msg.c_str()));
}

void Msg::Info(const std::string& msg) {
	go_msg_info(const_cast<char*>(msg.c_str()));
}

void Msg::Warning(const std::string& msg) {
	go_msg_warning(const_cast<char*>(msg.c_str()));
}

void Msg::Show(MsgType type_, const std::string& msg) {
	switch (type_) {
	case Msg::ERROR:
		Error(msg);
		break;
	case Msg::INFO:
		Info(msg);
		break;
	case Msg::WARNING:
		Warning(msg);
		break;
	}
}
