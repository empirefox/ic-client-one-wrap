if build_machine.system() == 'windows'
  folder_delim = '\\'
else
  folder_delim = '/'
endif

gang_test_src=[
  'main.cc',
  'gang_decoder_impl_av_info.cc'
]

gang_test_cpp_args = [
  '-D@0@="@1@@2@"'.format('MESON_SRC', meson.current_source_dir(), folder_delim),
  '-D@0@="@1@@2@"'.format('MESON_DST', meson.current_build_dir(), folder_delim)
]

gang_test_exe = executable(
  'gang_test',
  sources: gang_test_src,
  dependencies: gang,
  include_directories: catch_inc,
  cpp_args: main_cpp_args+gang_test_cpp_args
)

test('gang test', gang_test_exe)
