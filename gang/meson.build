gang_src = [
  'ffmpeg_format.c',
  'ffmpeg_log.c',
  'ffmpeg_transcoding.c',
  'gang_decoder_impl.c',

  'gang_audio_device.cc',
  'gang_decoder.cc',
  'gang_init_deps.cc',
  'gang_spdlog_console.cc',
  'gang.cc',
  'gangvideocapturer.cc'
]

ganglib = static_library(
  'gang',
  sources: gang_src,
  include_directories: inc,
  dependencies: deps,
  cpp_args: main_cpp_args
)

gang = declare_dependency(
  link_with: ganglib,
  link_args: webrtc_link_args,
  include_directories: inc,
  dependencies: deps
)

subdir('test')
