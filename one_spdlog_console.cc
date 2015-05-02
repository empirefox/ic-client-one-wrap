/*
 * one_spdlog_console.cc
 *
 *  Created on: May 2, 2015
 *      Author: savage
 */

#include "one_spdlog_console.h"

#include <iostream>

namespace one {

std::shared_ptr<spdlog::logger> console = NULL;

void InitOneSpdlogConsole() {
	try {
		console = spdlog::stdout_logger_mt("console:one");
#ifdef SPDLOG_TRACE_ON
		spdlog::set_level(spdlog::level::trace);
#elif defined SPDLOG_DEBUG_ON
		spdlog::set_level(spdlog::level::debug);
#elif defined PEER_INFO_ON
		spdlog::set_level(spdlog::level::info);
#endif // SPDLOG_TRACE_ON
		std::cout << "console:one ok" << std::endl;
	} catch (const spdlog::spdlog_ex& ex) {
		std::cout << "Log failed: " << ex.what() << std::endl;
	}
}

void CleanupOneSpdlog() {
	console = NULL;
}

} // namespace gang
