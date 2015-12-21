#include "gang_spdlog_console.h"

#include <iostream>

namespace gang {
std::shared_ptr<spdlog::logger> console = NULL;

void InitGangSpdlogConsole() {
  try {
    console = spdlog::stdout_logger_mt("console:gang");
    console->info() << "Log inited ok.";
  } catch (const spdlog::spdlog_ex& ex) {
    std::cout << "Log failed: " << ex.what() << std::endl;
  }
}

void CleanupGangSpdlog() {
  spdlog::drop("console:gang");

  console = NULL;
}
} // namespace gang
