project('ic-client-one-wrap', 'cpp')
add_global_arguments('-fno-rtti', language : 'cpp')
add_global_arguments('-pthread', language : 'cpp')

inc = include_directories(
  '/home/savage/git/ffmpeg-wrap',
  '/home/savage/git/webrtcbuilds',
  '/home/savage/git/spdlog/include',
  '/usr/include/jsoncpp'
)

src = [
'cgo.cc',
'composed_pc_factory.cc',
'one_spdlog_console.cc',
'peer.cc',
'peer_wrap.cc',
'shared.cc'
]

avcodec = dependency('libavcodec')
avformat = dependency('libavformat')

wraplib = static_library('ic-client-one-wrap',
                        sources: src,
                        include_directories: inc,
                        install: true,
                        cpp_args: ['-DLOG_LEVEL=0',
                                   '-DGANG_AV_LOG=0',
                                   '-DENABLE_THREAD_CHECKER=1',
                                   '-DENABLE_DEBUG=0',
                                   '-DSPDLOG_TRACE_OFF',
                                   '-DWEBRTC_POSIX',
                                   '-DSPDLOG_NO_DATETIME',
                                   '-DNO_CGO',
                                   '-D_GLIBCXX_USE_CXX11_ABI=0'])
