project(tests)

enable_testing()
find_package(GTest REQUIRED)
find_package(Boost REQUIRED)
find_package(PNG REQUIRED)

add_executable(
    yuv_test
    cache_test.cpp
    gil_test.cpp
    color_transform_test.cpp
    saturable_fixed_test.cpp
    xml_metadata_test.cpp)
target_link_libraries(
    yuv_test
    PRIVATE
        yuv GTest::GTest GTest::Main Boost::boost PNG::PNG)
add_test(yuv_test yuv_test)
