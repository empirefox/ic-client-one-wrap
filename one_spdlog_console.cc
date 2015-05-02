/*
 * one_spdlog_console.cc
 *
 *  Created on: May 2, 2015
 *      Author: savage
 */

#include "one_spdlog_console.h"

namespace one {

std::shared_ptr<spdlog::logger> console = NULL;

void InitOneSpdlogConsole() {
	console = spdlog::stdout_logger_mt("console:one");
}

void CleanupOneSpdlog() {
	console = NULL;
}

} // namespace gang
