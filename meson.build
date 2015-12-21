project('ic-client-one-wrap', 'c', 'cpp')
add_global_arguments('-fno-rtti', language: 'cpp')
add_global_arguments('-pthread', language: 'cpp')
add_global_arguments('-Wno-non-virtual-dtor', language: 'cpp')
#add_global_arguments('-Wno-variadic-macros', language: 'c')

inc = include_directories(
  '..',
  'gang',
  'peer',
  '/home/savage/git/macro-logger',
  '/home/savage/git/spdlog/include',
  '/home/savage/git/webrtcbuilds'
)

catch_inc = include_directories('/home/savage/git/Catch/single_include')

jsoncpp = dependency('jsoncpp')
avcodec = dependency('libavcodec')
avformat = dependency('libavformat')
avfilter = dependency('libavfilter')
crypto = dependency('libcrypto')
openssl = dependency('openssl')

webrtc = find_library('webrtc_full', dirs: '/home/savage/soft/webrtc/webrtc-linux64/lib/Debug')
webrtc_link_args = ['-pthread', '-lrt', '-ldl']

deps = [avcodec, avformat, avfilter, webrtc, jsoncpp, crypto, openssl]

# debug info use  -DDCHECK_ALWAYS_ON -DWEBRTC_TRACE -DENABLE_DEBUG=1
# sdplog: -DSPDLOG_DEBUG_ON -DSPDLOG_TRACE_ON -DPEER_INFO_ON -DSPDLOG_NO_DATETIME
# Macro-Logger: -DLOG_LEVEL=1 0:NO_LOGS,1:ERROR,2:INFO,3:DEBUG
# ffmpeg log: -DGANG_AV_LOG quiet:-8,panic:0,fatal:8,error:16
#            warning:24,info:32,verbose:40,debug:48,trace:56
# -O3 -ggdb

main_cpp_args = [
  '-DLOG_LEVEL=1',
  '-DGANG_AV_LOG=0',
  '-DENABLE_THREAD_CHECKER=1',
  '-DENABLE_DEBUG=0',
  '-DSPDLOG_TRACE_OFF',
  '-DWEBRTC_POSIX',
  '-DSPDLOG_NO_DATETIME',
  '-D_GLIBCXX_USE_CXX11_ABI=0'
]

run_command('go', 'tool', 'cgo', '-exportheader', 'peerconn_cgo.h', 'peerconn.go')

subdir('gang')
subdir('peer')

rtcwraplib = static_library('rtcwrap', objects: [ganglib.extract_all_objects(), peerlib.extract_all_objects()])
