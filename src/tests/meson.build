gtest_dep = dependency('gtest', main : true)

test(
    'yuv test',
    executable('yuv_test', [
        'cache_test.cpp',
        'gil_test.cpp',
        'color_transform_test.cpp',
        'saturable_fixed_test.cpp',
        'xml_metadata_test.cpp'],
        dependencies : [yuv_dep, gtest_dep]))
