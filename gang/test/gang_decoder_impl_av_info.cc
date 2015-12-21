#include "gang_decoder_impl.h"
#include "common.h"

CATCH_SCENARIO("init_gang_av_info", "[gang_decoder]") {
  CATCH_GIVEN("A 256X30pfs mp4") {
    const char kMp4Name[]    = MESON_SRC_FILE("5s-256x256x30fps.mp4");
    const char kMp4RecName[] = MESON_DST_FILE("5s-256x256x30fps-rec.mp4");

    CATCH_WHEN("enable rec and aodio") {
      int record_enabled = 1;
      int audio_off      = 0;

      CATCH_THEN("detected video only") {
        av_register_all();
        gang_decoder* dec = new_gang_decoder(kMp4Name, kMp4RecName, record_enabled, audio_off);

        CATCH_REQUIRE(init_gang_av_info(dec) == 0);
        CATCH_REQUIRE(dec->width == 256);
        CATCH_REQUIRE(dec->height == 256);
        CATCH_REQUIRE(dec->fps == 30);
        CATCH_REQUIRE(dec->no_video == 0);
        CATCH_REQUIRE(dec->no_audio == 1);
        free_gang_decoder(dec);
      }
    }
  }
}
