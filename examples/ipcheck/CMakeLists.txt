include(FetchContent)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        089b9daa1c4b83bf6f2711a21d7e26a156d3a7c4
)
FetchContent_MakeAvailable(httplib)

add_executable(ipcheck main.cpp)
target_link_libraries(ipcheck httplib::httplib)
