#pragma once

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

#include "gang_dec.h"

#define GANG_FITAL -1
#define GANG_ERROR_DATA 0
#define GANG_VIDEO_DATA 1
#define GANG_AUDIO_DATA 2

void initialize_gang_decoder_globel();
void cleanup_gang_decoder_globel();


// create gang_decode with given url
gang_decoder* new_gang_decoder(const char* url,
                               const char* rec_name,
                               int         record_enabled,
                               int         audio_off);

// free gang_decoder
void free_gang_decoder(gang_decoder* dec);

int  init_gang_av_info(gang_decoder* dec);

// Init all buffer and data that are needed by dec.
// return error
int  open_gang_decoder(gang_decoder* dec);

// Free all memo that opened by open_gang_decoder.
void close_gang_decoder(gang_decoder* dec);

// Read a single frame.
// If data==NULL, then no copy operate will be exec.
// Support: I420, YUY2, UYVY, ARGB.
// I420 will not be converted again, other format will not be supported
// until changing the api(open_gang_decoder) to output the format to c++.
// For HD avoid YU12 which is a software conversion and has 2 bugs.
//
// output: *data, need allocate and assign to **data.(do not free it)
//         size
// return: -1->EOF, 0->error, 1->video, 2->audio
int gang_decode_next_frame(gang_decoder* dec);

int flush_gang_rec_encoder(gang_decoder* dec);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif // ifdef __cplusplus
