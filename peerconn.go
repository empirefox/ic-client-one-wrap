// debug info use  -DDCHECK_ALWAYS_ON -DWEBRTC_TRACE -DENABLE_DEBUG=1
// sdplog: -DSPDLOG_DEBUG_ON -DSPDLOG_TRACE_ON -DPEER_INFO_ON -DSPDLOG_NO_DATETIME
// Macro-Logger: -DLOG_LEVEL=1 0:NO_LOGS,1:ERROR,2:INFO,3:DEBUG
// ffmpeg log: -DGANG_AV_LOG quiet:-8,panic:0,fatal:8,error:16
//            warning:24,info:32,verbose:40,debug:48,trace:56
// -O3 -ggdb
package rtc

// #cgo CXXFLAGS: -DWEBRTC_POSIX
// #cgo CXXFLAGS: -DGANG_AV_LOG=0
// #cgo CXXFLAGS: -DSPDLOG_NO_DATETIME
// #cgo CXXFLAGS: -DLOG_LEVEL=0
//
// #cgo CXXFLAGS: -std=gnu++11 -pthread -Wall -O3 -c -fmessage-length=0 -fno-rtti
// #cgo CXXFLAGS: -I/home/savage/git/webrtcbuilds
// #cgo CXXFLAGS: -I/usr/include/jsoncpp
// #cgo CXXFLAGS: -I/home/savage/git/ffmpeg-wrap
// #cgo CXXFLAGS: -I/home/savage/git/spdlog/include
//
// #cgo pkg-config: libavcodec libavformat libavfilter libcrypto openssl
// #cgo LDFLAGS: -std=gnu++11 -L/home/savage/git/ffmpeg-wrap/Release -lffmpeg-wrap
// #cgo LDFLAGS: -L/home/savage/soft/webrtc/webrtc-linux64/lib/Debug -lwebrtc_full
// #cgo LDFLAGS: -pthread -lrt -ldl
//
// #include <stdlib.h>
// #include "peer_wrap.h"
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

////////////////////////////////////
//            PeerConn
////////////////////////////////////
type PeerConn interface {
	Delete()
	CreateAnswer(sdp string)
	AddCandidate(sdp, mid string, line int)
}

type peerConn struct {
	send func([]byte)
	unsafe.Pointer
}

func (pc *peerConn) Delete() {
	C.DeletePeer(pc.Pointer)
}

func (pc *peerConn) CreateAnswer(sdp string) {
	csdp := C.CString(sdp)
	defer C.free(unsafe.Pointer(csdp))
	C.CreateAnswer(pc.Pointer, csdp)
}

func (pc *peerConn) AddCandidate(sdp, mid string, line int) {
	csdp := C.CString(sdp)
	defer C.free(unsafe.Pointer(csdp))

	cmid := C.CString(mid)
	defer C.free(unsafe.Pointer(cmid))

	C.AddCandidate(pc.Pointer, csdp, cmid, C.int(line))
}

////////////////////////////////////
//            Conductor
////////////////////////////////////
type Conductor interface {
	Release()
	Registry(id, url, recName string, recEnabled, isAudioOff bool) bool
	SetRecordEnabled(url string, recEnabled bool)
	CreatePeer(url string, send func([]byte)) PeerConn
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
	c.shared = C.Init(unsafe.Pointer(c))
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
	C.Release(conductor.shared)
	conductor.shared = nil
}

func (conductor *conductor) Registry(id, url, recName string, recEnabled, isAudioOff bool) bool {
	cid := C.CString(id)
	defer C.free(unsafe.Pointer(cid))
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))
	crecName := C.CString(recName)
	defer C.free(unsafe.Pointer(crecName))
	enabled, audioOff := C.int(0), C.int(0)
	if recEnabled {
		enabled = C.int(1)
	}
	if isAudioOff {
		audioOff = C.int(1)
	}

	ok := C.RegistryCam(conductor.shared, cid, curl, crecName, enabled, audioOff)
	return int(ok) != 0
}

func (conductor *conductor) SetRecordEnabled(url string, recEnabled bool) {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))
	enabled := C.int(0)
	if recEnabled {
		enabled = C.int(1)
	}
	C.SetRecordEnabled(conductor.shared, curl, enabled)
}

func (conductor *conductor) CreatePeer(url string, send func([]byte)) PeerConn {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))

	conductor.peersMutex.Lock()
	defer conductor.peersMutex.Unlock()
	pc := &peerConn{send: send}
	pc.Pointer = C.CreatePeer(curl, conductor.shared, unsafe.Pointer(pc))
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

	C.AddICE(conductor.shared, curi, cname, cpsd)
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
