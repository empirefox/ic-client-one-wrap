package rtc

// #cgo linux,amd64 pkg-config: rtcwrap-linux-amd64.pc
//
// #include <stdlib.h>
// #include "rtc.h"
import "C"
import (
	"sync"
	"unsafe"

	"github.com/golang/glog"
)

// according to wrap
const (
	ALIVE uint = 0
	DEAD  uint = 1
)

type StatusObserver interface {
	OnGangStatus(id string, status uint)
}

type IpcamAvInfo struct {
	Width  int
	Height int
	Video  bool
	Audio  bool
}

////////////////////////////////////
//            PeerConn
////////////////////////////////////
type PeerConn interface {
	IsZero() bool
	Delete()
	CreateAnswer(sdp string)
	AddCandidate(sdp, mid string, line int)
}

type peerConn struct {
	send func([]byte)
	unsafe.Pointer
}

func (pc *peerConn) IsZero() bool {
	return pc == nil
}

func (pc *peerConn) Delete() {
	C.delete_eer(pc.Pointer)
}

func (pc *peerConn) CreateAnswer(sdp string) {
	csdp := C.CString(sdp)
	defer C.free(unsafe.Pointer(csdp))
	C.create_answer(pc.Pointer, csdp)
}

func (pc *peerConn) AddCandidate(sdp, mid string, line int) {
	csdp := C.CString(sdp)
	defer C.free(unsafe.Pointer(csdp))

	cmid := C.CString(mid)
	defer C.free(unsafe.Pointer(cmid))

	C.add_candidate(pc.Pointer, csdp, cmid, C.int(line))
}

////////////////////////////////////
//            Conductor
////////////////////////////////////
type Conductor interface {
	Release()
	Registry(id, url, recName string, isRecOn, isAudioOff bool) (IpcamAvInfo, bool)
	SetRecordEnabled(url string, isRecOn bool)
	CreatePeer(id string, send func([]byte)) PeerConn
	DeletePeer(pc PeerConn)
	AddIceServer(uri, name, psd string)
}

type conductor struct {
	shared         unsafe.Pointer
	peers          map[unsafe.Pointer]PeerConn
	peersMutex     sync.Mutex
	statusObserver StatusObserver
}

func NewConductor(so StatusObserver) Conductor {
	c := &conductor{
		peers:          make(map[unsafe.Pointer]PeerConn),
		statusObserver: so,
	}
	c.shared = C.init_shared(unsafe.Pointer(c))
	return c
}

// make sure no peer get/set during call
func (conductor *conductor) Release() {
	conductor.peersMutex.Lock()
	defer conductor.peersMutex.Unlock()
	for _, pc := range conductor.peers {
		glog.Infoln("Delete pc")
		pc.Delete()
		delete(conductor.peers, pc.(*peerConn).Pointer)
	}
	C.release_shared(conductor.shared)
	conductor.shared = nil
}

func (conductor *conductor) Registry(id, url, recName string, isRecOn, isAudioOff bool) (IpcamAvInfo, bool) {
	cid := C.CString(id)
	defer C.free(unsafe.Pointer(cid))

	info := C.ipcam_info{
		url:       C.CString(url),
		rec_name:  C.CString(recName),
		rec_on:    C._Bool(isRecOn),
		audio_off: C._Bool(isAudioOff),
	}
	avInfo := C.ipcam_av_info{width: 0, height: 0, video: true, audio: true}
	ok := C.registry_cam(&avInfo, conductor.shared, cid, &info)

	return IpcamAvInfo{
		Width: int(avInfo.width), Height: int(avInfo.height),
		Video: bool(avInfo.video), Audio: bool(avInfo.audio),
	}, bool(ok)
}

func (conductor *conductor) SetRecordEnabled(id string, isRecOn bool) {
	cid := C.CString(id)
	defer C.free(unsafe.Pointer(cid))
	C.set_record_on(conductor.shared, cid, C._Bool(isRecOn))
}

func (conductor *conductor) CreatePeer(id string, send func([]byte)) PeerConn {
	cid := C.CString(id)
	defer C.free(unsafe.Pointer(cid))

	conductor.peersMutex.Lock()
	defer conductor.peersMutex.Unlock()
	pc := &peerConn{send: send}
	pc.Pointer = C.create_peer(cid, conductor.shared, unsafe.Pointer(pc))
	conductor.peers[pc.Pointer] = pc
	return pc
}

func (conductor *conductor) DeletePeer(pc PeerConn) {
	pcPtr := pc.(*peerConn).Pointer
	conductor.peersMutex.Lock()
	defer conductor.peersMutex.Unlock()
	if _, ok := conductor.peers[pcPtr]; ok {
		delete(conductor.peers, pcPtr)
	}
	glog.Infoln("Delete pc")
	pc.Delete()
}

func (conductor *conductor) AddIceServer(uri, name, psd string) {
	curi := C.CString(uri)
	defer C.free(unsafe.Pointer(curi))

	cname := C.CString(name)
	defer C.free(unsafe.Pointer(cname))

	cpsd := C.CString(psd)
	defer C.free(unsafe.Pointer(cpsd))

	C.add_ICE(conductor.shared, curi, cname, cpsd)
}

func (pc *peerConn) SendMessage(msg string) {
	pc.send([]byte(msg))
}

//export go_send_to_peer
func go_send_to_peer(pcPtr unsafe.Pointer, msg *C.char) {
	pc := (*peerConn)(pcPtr)
	pc.SendMessage(C.GoString(msg))
	glog.Infoln("go_send_to_peer ok")
}

//export go_on_gang_status
func go_on_gang_status(conductorPtr unsafe.Pointer, id *C.char, status C.uint) {
	(*conductor)(conductorPtr).statusObserver.OnGangStatus(C.GoString(id), uint(status))
}
