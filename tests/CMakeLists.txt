cmake_minimum_required(VERSION 3.16)
project(ffmpegdirect_tests LANGUAGES CXX)

enable_testing()

add_executable(test_realtime_stream TestRealtimeStream.cpp)
target_compile_features(test_realtime_stream PRIVATE cxx_std_17)
add_test(NAME realtime_stream_policy COMMAND test_realtime_stream)
