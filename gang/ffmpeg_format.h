#pragma once

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

#include <libavformat/avformat.h>

/**
 * From transcode_aac.c
 * Convert an error code into a text message.
 * @param error Error code to be converted
 * @return Corresponding error text (not thread-safe)
 */
const char* get_error_text(const int error);

/**
 * From demuxing_decoding.c
 * return error
 */
int         open_codec_context(int*              stream_idx,
                               AVFormatContext** input_format_context,
                               enum AVMediaType  type);

/**
 * From transcode_aac.c
 * Open an input file and the required decoder.
 */
int open_input_file(const char*       filename,
                    AVFormatContext** input_format_context,
                    AVStream**        video_stream,
                    AVStream**        audio_stream,
                    int               audio_off);

/** Initialize one audio frame for reading from the input file */
int init_frame(AVFrame** frame);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif // ifdef __cplusplus
