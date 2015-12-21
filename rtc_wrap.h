#pragma once

#include "param_types.h"

void* Init(void* goConductorPtr);
void  Release(void* shared);

void  AddICE(void* sharedPtr,
             char* uri,
             char* name,
             char* psd);

bool RegistryCam(ipcam_av_info* av_info,
                 void*          sharedPtr,
                 char*          id,
                 ipcam_info*    info);

void SetRecordEnabled(void* sharedPtr,
                      char* id,
                      bool  rec_on);

void* CreatePeer(char* id,
                 void* sharedPtr,
                 void* goPcPtr);

void DeletePeer(void* pc);

void CreateAnswer(void* pc,
                  char* sdp);

void AddCandidate(void* pc,
                  char* sdp,
                  char* mid,
                  int   line);
