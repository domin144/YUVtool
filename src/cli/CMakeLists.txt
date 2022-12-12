project(cli)

find_package(Boost REQUIRED)
find_package(PNG REQUIRED)

add_executable(yuvtool_cli)
target_sources(yuvtool_cli PRIVATE main.cpp)
target_link_libraries(yuvtool_cli PRIVATE yuv Boost::boost PNG::PNG)

install(TARGETS yuvtool_cli RUNTIME DESTINATION bin)
