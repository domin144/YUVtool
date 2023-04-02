eigen_dep = dependency('eigen3')
tinyxml2_dep = dependency('tinyxml2')
yuv_deps = [eigen_dep, tinyxml2_dep]

yuv_includes = include_directories('include')
subdir('include')

yuv_sources = [
    'Errors.cpp',
    'Picture_buffer.cpp',
    'Pixel_format.cpp',
    'saturable_fixed.cpp',
    'xml_metadata.cpp',
    'Yuv_file.cpp'
]

yuv_lib = library(
    'yuv',
    yuv_sources,
    include_directories : yuv_includes,
    dependencies : yuv_deps)

yuv_dep = declare_dependency(
    link_with : yuv_lib,
    include_directories : yuv_includes,
    dependencies : yuv_deps)