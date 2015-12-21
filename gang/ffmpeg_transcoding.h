#pragma once

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

#include "gang_dec.h"

int open_input_streams(gang_decoder* dec);

int open_output_streams(gang_decoder* dec,
                        int           record_enabled);

int init_filters(FilterStreamContext* fscs,
                 size_t               n);

int encode_write_frame(gang_decoder*        dec,
                       FilterStreamContext* fsc,
                       int*                 got_frame);

int flush_encoder(gang_decoder*        dec,
                  FilterStreamContext* fsc);

int find_fs_index(int*                 index,
                  FilterStreamContext* fscs,
                  size_t               fs_size,
                  int                  stream_index);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif // ifdef __cplusplus
