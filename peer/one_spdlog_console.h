#pragma once

#include "spdlog/spdlog.h"

namespace one {
extern std::shared_ptr<spdlog::logger> console;

void InitOneSpdlogConsole();

void CleanupOneSpdlog();
} // namespace gang
