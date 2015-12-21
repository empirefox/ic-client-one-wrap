peer_src = [
  'cgo.cc',
  'composed_pc_factory.cc',
  'one_spdlog_console.cc',
  'peer.cc',
  'rtc_wrap.cc',
  'shared.cc'
]

peerlib = static_library(
  'peer',
  sources: peer_src,
  dependencies: gang,
  cpp_args: main_cpp_args
)
