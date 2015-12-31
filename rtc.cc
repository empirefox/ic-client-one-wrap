extern "C" {
#include "rtc.h"
}

#include "rtc_wrap.h"

void* init_shared(void* goConductorPtr) {return Init(goConductorPtr);}

void  release_shared(void* shared)      {Release(shared);}

void  add_ICE(void* sharedPtr, char* uri, char* name, char* psd) {
  AddICE(sharedPtr, uri, name, psd);
}

bool registry_cam(ipcam_av_info* av_info, void* sharedPtr, char* id, ipcam_info* info) {
  return RegistryCam(av_info, sharedPtr, id, info);
}

void  unregistry_cam(void* sharedPtr, char* id)               {UnregistryCam(sharedPtr, id);}

void  set_record_on(void* sharedPtr, char* id, bool rec_on)   {SetRecordEnabled(sharedPtr, id, rec_on);}

void* create_peer(char* id, void* sharedPtr, void* goPcPtr)   {return CreatePeer(id, sharedPtr, goPcPtr);}

void  delete_eer(void* pc)                                    {DeletePeer(pc);}

void  create_answer(void* pc, char* sdp)                      {CreateAnswer(pc, sdp);}

void  add_candidate(void* pc, char* sdp, char* mid, int line) {AddCandidate(pc, sdp, mid, line);}
