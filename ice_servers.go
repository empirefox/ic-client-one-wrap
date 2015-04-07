package rtc

// #include <stdlib.h>
// #include "ice_servers.h"
import "C"
import "unsafe"

func AddIceServer(uri, name, psd string) {
	curi := C.CString(uri)
	defer C.free(unsafe.Pointer(curi))

	cname := C.CString(name)
	defer C.free(unsafe.Pointer(cname))

	cpsd := C.CString(psd)
	defer C.free(unsafe.Pointer(cpsd))

	C.AddIceServer(curi, cname, cpsd)
}

func AddIceUri(uri string) {
	AddIceServer(uri, "", "")
}
