#pragma once

#include "param_types.h"

void* init_shared(void* goConductorPtr);
void  release_shared(void* shared);

void  add_ICE(void* sharedPtr,
              char* uri,
              char* name,
              char* psd);

bool registry_cam(ipcam_av_info* av_info,
                  void*          sharedPtr,
                  char*          id,
                  ipcam_info*    info);

void unregistry_cam(void* sharedPtr,
                    char* id);

void set_record_on(void* sharedPtr,
                   char* id,
                   bool  rec_on);

void* create_peer(char* id,
                  void* sharedPtr,
                  void* goPcPtr);

void delete_eer(void* pc);

void create_answer(void* pc,
                   char* sdp);

void add_candidate(void* pc,
                   char* sdp,
                   char* mid,
                   int   line);
