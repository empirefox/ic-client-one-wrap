#pragma once

void* Init(void* goConductorPtr);
void  Release(void* shared);

void  AddICE(void* sharedPtr,
             char* uri,
             char* name,
             char* psd);

int RegistryCam(ipcam_info* info,
                void*       sharedPtr,
                char*       id,
                char*       url,
                char*       rec_name,
                int         rec_enabled,
                int         audio_off);

void SetRecordEnabled(void* sharedPtr,
                      char* id,
                      int   rec_enabled);

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
