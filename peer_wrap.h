// Copyright 2015, empirefox
// Copyright 2014, Salman Aljammaz

#ifndef PEER_WRAP_H_
#define PEER_WRAP_H_
#pragma once

void* Init(void* goConductorPtr);
void Release(void* shared);
void AddICE(void* sharedPtr, char *uri, char *name, char *psd);
int RegistryCam(void* sharedPtr, char *id, char *url, char *rec_name, int rec_enabled);
void SetRecordEnabled(void* sharedPtr, char *url, int rec_enabled);
void* CreatePeer(char *url, void* sharedPtr, void* goPcPtr);
void DeletePeer(void* pc);
void CreateAnswer(void* pc, char* sdp);
void AddCandidate(void* pc, char* sdp, char* mid, int line);

#endif //PEER_WRAP_H_
