#pragma once

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

#include <stdbool.h>

typedef struct ipcam_info {
  char* url;
  char* rec_name;
  bool  rec_on;
  bool  audio_off;
} ipcam_info;

typedef struct ipcam_av_info {
  int  width;
  int  height;
  bool no_video;
  bool no_audio;
} ipcam_av_info;

#ifdef __cplusplus
} // closing brace for extern "C"
#endif // ifdef __cplusplus
