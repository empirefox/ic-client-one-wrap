#pragma once


#include "spdlog/spdlog.h"

namespace gang {
extern std::shared_ptr<spdlog::logger> console;

void InitGangSpdlogConsole();

void CleanupGangSpdlog();
} // namespace gang
