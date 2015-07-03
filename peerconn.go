// debug info use  -DDCHECK_ALWAYS_ON -DWEBRTC_TRACE
// sdplog: -DSPDLOG_DEBUG_ON -DSPDLOG_TRACE_ON -DPEER_INFO_ON -DSPDLOG_NO_DATETIME
// Macro-Logger: -DLOG_LEVEL=1 0:NO_LOGS,1:ERROR,2:INFO,3:DEBUG
// ffmpeg log: -DGANG_AV_LOG quiet:-8,panic:0,fatal:8,error:16
//            warning:24,info:32,verbose:40,debug:48,trace:56
package rtc

// #cgo CXXFLAGS: -DWEBRTC_POSIX
// #cgo CXXFLAGS: -DGANG_AV_LOG=8
// #cgo CXXFLAGS: -DSPDLOG_NO_DATETIME
// #cgo CXXFLAGS: -DPEER_INFO_ON -DLOG_LEVEL=1
//
// #cgo CXXFLAGS: -std=c++11 -fno-rtti
// #cgo CXXFLAGS: -I/home/savage/git/webrtcbuilds
// #cgo CXXFLAGS: -I/usr/include/jsoncpp
// #cgo CXXFLAGS: -I/home/savage/git/ffmpeg-wrap
// #cgo CXXFLAGS: -I/home/savage/git/spdlog/include
//
// #cgo pkg-config: libavcodec libavformat libavfilter libssl nss x11
// #cgo LDFLAGS: -std=gnu++11 -L/home/savage/git/ffmpeg-wrap/Release -lffmpeg-wrap
// #cgo LDFLAGS: -L/home/savage/soft/webrtc/webrtc-linux64/lib/Release -lwebrtc_full
// #cgo LDFLAGS: -lstdc++ -lpthread -lrt -ldl
//
// #include <stdlib.h>
// #include "peer_wrap.h"
import "C"
import (
	"sync"
	"unsafe"

	"github.com/golang/glog"
)

////////////////////////////////////
//            PeerConn
////////////////////////////////////
type PeerConn interface {
	Delete()
	CreateAnswer(sdp string)
	AddCandidate(sdp, mid string, line int)
}

type peerConn struct {
	send chan []byte
	unsafe.Pointer
}

func (pc peerConn) Delete() {
	C.DeletePeer(pc.Pointer)
}

func (pc peerConn) CreateAnswer(sdp string) {
	csdp := C.CString(sdp)
	defer C.free(unsafe.Pointer(csdp))
	C.CreateAnswer(pc.Pointer, csdp)
}

func (pc peerConn) AddCandidate(sdp, mid string, line int) {
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
	Registry(url, recName string, recEnabled bool) bool
	SetRecordEnabled(url string, recEnabled bool)
	CreatePeer(url string, send chan []byte) PeerConn
	DeletePeer(pc PeerConn)
	AddIceServer(uri, name, psd string)
}

type conductor struct {
	shared     unsafe.Pointer
	peers      map[unsafe.Pointer]PeerConn
	peersMutex sync.Mutex
}

func NewConductor() Conductor {
	return &conductor{
		shared: C.Init(),
		peers:  make(map[unsafe.Pointer]PeerConn),
	}
}

// make sure no peer get/set during call
func (conductor conductor) Release() {
	for _, pc := range conductor.peers {
		pc.Delete()
	}
	conductor.peers = nil
	C.Release(conductor.shared)
}

func (conductor conductor) Registry(url, recName string, recEnabled bool) bool {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))
	crecName := C.CString(recName)
	defer C.free(unsafe.Pointer(crecName))
	enabled := C.int(0)
	if recEnabled {
		enabled = C.int(1)
	}

	ok := C.RegistryUrl(conductor.shared, curl, crecName, enabled)
	return int(ok) != 0
}

func (conductor conductor) SetRecordEnabled(url string, recEnabled bool) {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))
	enabled := C.int(0)
	if recEnabled {
		enabled = C.int(1)
	}
	C.SetRecordEnabled(conductor.shared, curl, enabled)
}

func (conductor conductor) CreatePeer(url string, send chan []byte) PeerConn {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))

	pc := &peerConn{send: send}
	pc.Pointer = C.CreatePeer(curl, conductor.shared, unsafe.Pointer(pc))
	conductor.peersMutex.Lock()
	conductor.peers[pc.Pointer] = pc
	conductor.peersMutex.Unlock()
	return pc
}

func (conductor conductor) DeletePeer(pc PeerConn) {
	pcPtr := pc.(*peerConn).Pointer
	conductor.peersMutex.Lock()
	if _, ok := conductor.peers[pcPtr]; ok {
		delete(conductor.peers, pcPtr)
	}
	conductor.peersMutex.Unlock()
	pc.Delete()
}

func (conductor conductor) AddIceServer(uri, name, psd string) {
	curi := C.CString(uri)
	defer C.free(unsafe.Pointer(curi))

	cname := C.CString(name)
	defer C.free(unsafe.Pointer(cname))

	cpsd := C.CString(psd)
	defer C.free(unsafe.Pointer(cpsd))

	C.AddICE(conductor.shared, curi, cname, cpsd)
}

func (pc *peerConn) SendMessage(msg string) {
	pc.send <- []byte(msg)
}

//export go_send_to_peer
func go_send_to_peer(pcPtr unsafe.Pointer, msg *C.char) {
	pc := (*peerConn)(pcPtr)
	pc.SendMessage(C.GoString(msg))
	glog.Infoln("go_send_to_peer ok")
}
