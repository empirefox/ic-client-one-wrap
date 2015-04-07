// Copyright 2015, empirefox
// Copyright 2014, Salman Aljammaz

#ifndef PEERCONN_H_
#define PEERCONN_H_
#pragma once

void init();
void* CreatePeer();
void CreateAnswer(void* pc, char* offerSDP_sent_from_offerer); // add callback?
void AddCandidate(void* pc, char* sdp, char* mid, int line);
void AddICE(void* pc, char *uri, char *name, char *psd);

#endif //PEERCONN_H_
