package rtc

// #cgo CXXFLAGS: -DWEBRTC_POSIX
// #cgo CXXFLAGS: -DSPDLOG_DEBUG_ON -DSPDLOG_TRACE_ON -DSPDLOG_NO_DATETIME
// #cgo CXXFLAGS: -DPEER_INFO_ON
//
// #cgo CXXFLAGS: -std=c++0x -fno-rtti
// #cgo CXXFLAGS: -I/home/savage/git/webrtcbuilds
// #cgo CXXFLAGS: -I/usr/include/jsoncpp
// #cgo CXXFLAGS: -I/home/savage/git/ffmpeg-wrap
// #cgo CXXFLAGS: -I/home/savage/git/spdlog/include
//
// #cgo pkg-config: libavcodec libavformat libssl
// #cgo LDFLAGS: -std=gnu++0x -L/home/savage/git/ffmpeg-wrap/Debug -lffmpeg-wrap
// #cgo LDFLAGS: -L/home/savage/soft/webrtc/webrtc-linux64/lib/Release -lwebrtc_full
// #cgo LDFLAGS: -lstdc++ -lnss3 -lnssutil3 -lsmime3 -lssl3 -lplds4 -lplc4 -lnspr4 -lX11 -lpthread -lrt -ldl
//
// #include <stdlib.h>
// #include "peer_wrap.h"
import "C"
import (
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
	Registry(url string) bool
	CreatePeer(url string, send chan []byte) PeerConn
	AddIceServer(uri, name, psd string)
}

type conductor struct {
	shared unsafe.Pointer
	peers  map[unsafe.Pointer]PeerConn
}

func NewConductor() Conductor {
	return &conductor{
		shared: C.Init(),
		peers:  make(map[unsafe.Pointer]PeerConn),
	}
}

func (conductor conductor) Release() {
	for _, pc := range conductor.peers {
		pc.Delete()
	}
	C.Release(conductor.shared)
}

func (conductor conductor) Registry(url string) bool {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))

	ok := C.RegistryUrl(conductor.shared, curl)
	return int(ok) != 0
}

func (conductor conductor) CreatePeer(url string, send chan []byte) PeerConn {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))

	pc := &peerConn{send: send}
	pc.Pointer = C.CreatePeer(curl, conductor.shared, unsafe.Pointer(pc))
	conductor.peers[pc.Pointer] = pc
	return pc
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
	glog.Infoln("SendMessage:")
	pc.send <- []byte(msg)
	glog.Infoln("SendMessage ok")
}

//export go_send_to_peer
func go_send_to_peer(pcPtr unsafe.Pointer, msg *C.char) {
	glog.Infoln("go_send_to_peer")
	pc := (*peerConn)(pcPtr)
	pc.SendMessage(C.GoString(msg))
}
