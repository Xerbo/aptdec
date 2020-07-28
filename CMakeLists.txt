cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project (aptdec)

file(GLOB_RECURSE C_SOURCE_FILES src/*.c)
add_executable(aptdec ${C_SOURCE_FILES})

# Math
target_link_libraries(aptdec PRIVATE m)

# PNG
if(WIN32 AND NOT MINGW)
    find_package(PNG CONFIG REQUIRED)
else()
    find_package(PNG REQUIRED)
endif()
target_link_libraries(aptdec PRIVATE PNG::PNG)

# SndFile
find_package(LibSndFile REQUIRED)
target_link_libraries(aptdec PRIVATE ${LIBSNDFILE_LIBRARY})
