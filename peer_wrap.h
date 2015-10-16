// Copyright 2015, empirefox
// Copyright 2014, Salman Aljammaz

#ifndef PEER_WRAP_H_
#define PEER_WRAP_H_
#pragma once

typedef struct ipcam_info {
	int width;
	int height;
} ipcam_info;

void* Init(void* goConductorPtr);
void Release(void* shared);
void AddICE(void* sharedPtr, char *uri, char *name, char *psd);
int RegistryCam(
		ipcam_info* info,
		void* sharedPtr,
		char *id,
		char *url,
		char *rec_name,
		int rec_enabled,
		int audio_off);
void SetRecordEnabled(void* sharedPtr, char *url, int rec_enabled);
void* CreatePeer(char *id, void* sharedPtr, void* goPcPtr);
void DeletePeer(void* pc);
void CreateAnswer(void* pc, char* sdp);
void AddCandidate(void* pc, char* sdp, char* mid, int line);

#endif //PEER_WRAP_H_
