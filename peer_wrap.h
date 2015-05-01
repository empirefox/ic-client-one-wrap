// Copyright 2015, empirefox
// Copyright 2014, Salman Aljammaz

#ifndef PEER_WRAP_H_
#define PEER_WRAP_H_
#pragma once

void* Init();
void Release(void* shared);
void AddICE(void* sharedPtr, char *uri, char *name, char *psd);
int RegistryUrl(void* sharedPtr, char *url);
void* CreatePeer(char *url, void* sharedPtr, void* channPtr);
void DeletePeer(void* pc);
void CreateAnswer(void* pc, char* sdp);
void AddCandidate(void* pc, char* sdp, char* mid, int line);

#endif //PEER_WRAP_H_
