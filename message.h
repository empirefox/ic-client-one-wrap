#ifndef MESSAGE_H_
#define MESSAGE_H_
#pragma once

#include <string>

class Msg {
public:
	enum MsgType {
		ERROR, INFO, WARNING
	};

	static void Show(MsgType type_, const std::string& msg);

	static void Error(const std::string& msg);
	static void Info(const std::string& msg);
	static void Warning(const std::string& msg);
};

#endif // MESSAGE_H_
