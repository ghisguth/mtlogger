include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#add_library(logger logger/log.cpp logger/logger.cpp)
#target_link_libraries(logger m boost_thread-mt boost_system-mt)

add_executable(test test/test.cpp)
target_link_libraries(test m boost_thread-mt boost_system-mt)
