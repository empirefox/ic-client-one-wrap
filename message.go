package rtc

import "C"
import "github.com/golang/glog"

//export go_msg_error
func go_msg_error(msg *C.char) {
	glog.Errorln(C.GoString(msg))
}

//export go_msg_info
func go_msg_info(msg *C.char) {
	glog.Infoln(C.GoString(msg))
}

//export go_msg_warning
func go_msg_warning(msg *C.char) {
	glog.Warningln(C.GoString(msg))
}
