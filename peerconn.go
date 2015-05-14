package rtc

// #cgo CXXFLAGS: -DPOSIX -DWEBRTC_POSIX
// #cgo CXXFLAGS: -DSPDLOG_DEBUG_ON -DSPDLOG_TRACE_ON -DSPDLOG_NO_DATETIME
// #cgo CXXFLAGS: -DPEER_INFO_ON
//
// #cgo CXXFLAGS: -std=c++0x -fno-rtti
// #cgo CXXFLAGS: -I/home/savage/git/webrtcbuilds
// #cgo CXXFLAGS: -I/usr/include/jsoncpp
// #cgo CXXFLAGS: -I/home/savage/git/ffmpeg-wrap
// #cgo CXXFLAGS: -I/home/savage/git/spdlog/include
//
// #cgo pkg-config: libavcodec libavformat
// #cgo LDFLAGS: -std=gnu++0x -L/home/savage/git/ffmpeg-wrap/Debug -lffmpeg-wrap
// #cgo LDFLAGS: -L/home/savage/soft/webrtc/webrtc-linux64/lib/Release -lwebrtc_full
// #cgo LDFLAGS: -lstdc++ -lnss3 -lnssutil3 -lsmime3 -lssl3 -lplds4 -lplc4 -lnspr4 -lX11 -lpthread -lrt -ldl
//
// #include <stdlib.h>
// #include "peer_wrap.h"
import "C"
import "unsafe"

type PeerConn struct {
	ToPeerChan chan []byte
	unsafe.Pointer
}

func (pc PeerConn) Delete() {
	C.DeletePeer(pc.Pointer)
}

func (pc PeerConn) CreateAnswer(sdp string) {
	csdp := C.CString(sdp)
	defer C.free(unsafe.Pointer(csdp))
	C.CreateAnswer(pc.Pointer, csdp)
}

func (pc PeerConn) AddCandidate(sdp, mid string, line int) {
	csdp := C.CString(sdp)
	defer C.free(unsafe.Pointer(csdp))

	cmid := C.CString(mid)
	defer C.free(unsafe.Pointer(cmid))

	C.AddCandidate(pc.Pointer, csdp, cmid, C.int(line))
}

func (pc PeerConn) SendMessage(msg []byte) {
	pc.ToPeerChan <- msg
}

type Conductor struct {
	Shared unsafe.Pointer
	Peers  map[unsafe.Pointer]PeerConn
}

func NewShared() Conductor {
	return Conductor{
		Shared: C.Init(),
		Peers:  make(map[unsafe.Pointer]PeerConn),
	}
}

func (conductor Conductor) Release() {
	for _, pc := range conductor.Peers {
		pc.Delete()
	}
	C.Release(conductor.Shared)
}

func (conductor Conductor) Registry(url string) bool {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))

	ok := C.RegistryUrl(conductor.Shared, curl)
	return int(ok) != 0
}

func (conductor Conductor) CreatePeer(url string) *PeerConn {
	curl := C.CString(url)
	defer C.free(unsafe.Pointer(curl))

	pc := PeerConn{
		ToPeerChan: make(chan []byte, 64),
	}
	pc.Pointer = C.CreatePeer(curl, conductor.Shared, unsafe.Pointer(&pc))
	conductor.Peers[pc.Pointer] = pc
	return &pc
}

func (conductor Conductor) AddIceServer(uri, name, psd string) {
	curi := C.CString(uri)
	defer C.free(unsafe.Pointer(curi))

	cname := C.CString(name)
	defer C.free(unsafe.Pointer(cname))

	cpsd := C.CString(psd)
	defer C.free(unsafe.Pointer(cpsd))

	C.AddICE(conductor.Shared, curi, cname, cpsd)
}

func (conductor Conductor) AddIceUri(uri string) {
	conductor.AddIceServer(uri, "", "")
}

//export go_send_to_peer
func go_send_to_peer(pcPtr unsafe.Pointer, msg *C.char) {
	pc := (*PeerConn)(pcPtr)
	pc.SendMessage([]byte(C.GoString(msg)))
}
