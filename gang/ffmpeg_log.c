#include "ffmpeg_log.h"

#include <libavutil/log.h>

void init_gang_av_log() {
#ifdef GANG_AV_LOG
  av_log_set_level(GANG_AV_LOG);
#endif /* ifdef GANG_AV_LOG */
}
