cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(aptdec)
file(GLOB_RECURSE C_SOURCE_FILES src/*.c src/libs/*.c)
add_executable(aptdec ${C_SOURCE_FILES})

add_compile_definitions(PALETTE_DIR="../palettes")

# Math
target_link_libraries(aptdec PRIVATE m)

# Throw errors on warnings on release builds
if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(aptdec PRIVATE -Wall -Wextra -pedantic -Wno-missing-field-initializers -Werror)
else()
    target_compile_options(aptdec PRIVATE -Wall -Wextra -pedantic)
endif()

# libpng
target_link_libraries(aptdec PRIVATE PNG::PNG)
if(WIN32 AND NOT MINGW)
    find_package(PNG CONFIG REQUIRED)
else()
    find_package(PNG REQUIRED)
endif()

# libsndfile
find_package(LibSndFile REQUIRED)
target_link_libraries(aptdec PRIVATE ${LIBSNDFILE_LIBRARY})

# TODO: get this from git
set(PROJECT_VERSION "1.7.0")

# CPack
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_NAME "aptdec")
set(CPACK_PACKAGE_CONTACT "Xerbo (xerbo@protonmail.com)")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NOAA APT satellite imagery decoder")
set(CPACK_PACKAGE_DESCRIPTION "Aptdec is a FOSS program that decodes images transmitted by NOAA weather satellites. These satellites transmit constantly (among other things) medium resolution (4km/px) images of the earth over a analog mode called APT.")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

IF(NOT WIN32)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")

    install(TARGETS aptdec DESTINATION "bin/")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/palettes/" DESTINATION "palettes/")
else()
    #set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

    install(TARGETS aptdec DESTINATION "bin/")
    install(FILES libpng16.dll sndfile.dll libzlib.dll DESTINATION "bin/")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/palettes/" DESTINATION "palettes/")
endif()

include(CPack)
