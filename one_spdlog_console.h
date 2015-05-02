/*
 * one_spdlog_console.h
 *
 *  Created on: May 2, 2015
 *      Author: savage
 */

#ifndef ONE_SPDLOG_CONSOLE_H_
#define ONE_SPDLOG_CONSOLE_H_
#pragma once

#include "spdlog/spdlog.h"

namespace one {

extern std::shared_ptr<spdlog::logger> console;

void InitOneSpdlogConsole();

void CleanupOneSpdlog();

} // namespace gang

#endif /* ONE_SPDLOG_CONSOLE_H_ */
