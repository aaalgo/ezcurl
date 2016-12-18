cmake_minimum_required(VERSION 2.8)
project(ezcurl)

set(PUBLIC_HEADERS ezcurl.h)

foreach(h ${PUBLIC_HEADERS})
    get_filename_component(HEADER_INCLUDE_DIRECTORY include/${h} PATH) # use DIRECTORY instead of PATH once requiring CMake 3.0
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${h} DESTINATION ${HEADER_INCLUDE_DIRECTORY} COMPONENT "headers")
endforeach()
