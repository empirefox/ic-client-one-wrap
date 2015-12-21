#include "gang_init_deps.h"

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

#include "gang_decoder_impl.h"
#include "gang_spdlog_console.h"

namespace gang {
void InitializeGangDecoderGlobel() {
  InitGangSpdlogConsole();
  ::initialize_gang_decoder_globel();
  WebRtcSpl_Init();
}

void CleanupGangDecoderGlobel() {
  ::cleanup_gang_decoder_globel();
  CleanupGangSpdlog();
}
} // namespace gang
