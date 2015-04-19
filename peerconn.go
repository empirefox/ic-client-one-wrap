package rtc

// #cgo CXXFLAGS: -DPOSIX -DWEBRTC_POSIX
//
// #cgo CXXFLAGS: -std=c++0x
// #cgo CXXFLAGS: -I/home/savage/git/webrtcbuilds
// #cgo CXXFLAGS: -I/usr/include/jsoncpp
// #cgo CXXFLAGS: -I/home/savage/git/ffmpeg-wrap
//
// #cgo pkg-config: libavcodec libavformat
// #cgo LDFLAGS: -std=gnu++0x -L/home/savage/git/ffmpeg-wrap/Debug -lffmpeg-wrap
// #cgo LDFLAGS: -L/home/savage/soft/webrtc/webrtc-linux64/lib/Release -lwebrtc_full
// #cgo LDFLAGS: -lstdc++ -lnss3 -lnssutil3 -lsmime3 -lssl3 -lplds4 -lplc4 -lnspr4 -lX11 -lpthread -lrt -ldl
//
// #include <stdlib.h>
// #include "peerconn.h"
import "C"
import "unsafe"

type PeerConn struct {
	ToPeerChan chan string
	unsafe.Pointer
}

// A map to make looking up the right channel easier for callbacks.
// The alternative would be to send method pointers as callbacks but
// that's messy with cgo and we need a container anyway.
var conns = make(map[unsafe.Pointer]PeerConn)

//export go_send_to_peer
func go_send_to_peer(pc unsafe.Pointer, msg *C.char) {
	if conn, ok := conns[pc]; ok {
		conn.ToPeerChan <- C.GoString(msg)
	}
}

func init() {
	C.init()
}

func CreatePeer() PeerConn {
	pc := PeerConn{
		ToPeerChan: make(chan string, 64),
		Pointer:    C.CreatePeer(),
	}
	conns[pc.Pointer] = pc
	return pc
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

func (pc PeerConn) AddIceServer(uri, name, psd string) {
	curi := C.CString(uri)
	defer C.free(unsafe.Pointer(curi))

	cname := C.CString(name)
	defer C.free(unsafe.Pointer(cname))

	cpsd := C.CString(psd)
	defer C.free(unsafe.Pointer(cpsd))

	C.AddICE(pc.Pointer, curi, cname, cpsd)
}

func (pc PeerConn) AddIceUri(uri string) {
	pc.AddIceServer(uri, "", "")
}
