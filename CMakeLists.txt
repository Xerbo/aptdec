cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(aptdec)

include(GNUInstallDirs)

# libpng
find_package(PNG)

# libsndfile
find_package(LibSndFile)

set(LIB_C_SOURCE_FILES src/color.c src/dsp.c src/filter.c src/image.c src/libs/reg.c src/libs/median.c)
set(EXE_C_SOURCE_FILES src/main.c src/pngio.c src/libs/argparse.c)
set(LIB_C_HEADER_FILES src/apt.h)

# Link with static library for aptdec executable, so we don't need to set the path
add_library(aptstatic STATIC ${LIB_C_SOURCE_FILES})

# Create shared library for 3rd party apps
add_library(apt SHARED ${LIB_C_SOURCE_FILES})
set_target_properties(apt PROPERTIES PUBLIC_HEADER ${LIB_C_HEADER_FILES})

add_compile_definitions(PALETTE_DIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/palettes")

if (PNG_FOUND AND LIBSNDFILE_FOUND)
    add_executable(aptdec ${EXE_C_SOURCE_FILES})
    include_directories(${PNG_PNG_INCLUDE_DIR})
    include_directories(${LIBSNDFILE_INCLUDE_DIR})
    target_link_libraries(aptdec PRIVATE PNG::PNG)
    target_link_libraries(aptdec PRIVATE ${LIBSNDFILE_LIBRARY})
    target_link_libraries(aptdec PRIVATE aptstatic)
    if (MSVC)
        target_compile_options(aptdec PRIVATE /D_CRT_SECURE_NO_WARNINGS=1 /DAPT_API_STATIC)
    else()
        # Math
        target_link_libraries(aptdec PRIVATE m)

        # Throw errors on warnings on release builds
        if(CMAKE_BUILD_TYPE MATCHES "Release")
            target_compile_options(aptdec PRIVATE -Wall -Wextra -pedantic -Wno-missing-field-initializers -Werror)
        else()
            target_compile_options(aptdec PRIVATE -Wall -Wextra -pedantic)
        endif()
    endif()
else()
    MESSAGE(STATUS "WARNING: Only building apt library, as not all of the required libraries were found for aptdec.")
endif()

if (MSVC)
    target_compile_options(apt PRIVATE /D_CRT_SECURE_NO_WARNINGS=1 /DAPT_API_EXPORT)
    target_compile_options(aptstatic PRIVATE /D_CRT_SECURE_NO_WARNINGS=1 /DAPT_API_STATIC)
else()
    # Math
    target_link_libraries(apt PRIVATE m)
    target_link_libraries(aptstatic PRIVATE m)

    if(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(apt PRIVATE -Wall -Wextra -pedantic -Wno-missing-field-initializers -Werror)
    else()
        target_compile_options(apt PRIVATE -Wall -Wextra -pedantic)
    endif()
endif()

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
else()
    #set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

    if (TARGET aptdec)
        file(GLOB_RECURSE DLLS *.dll)
        install(FILES ${DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

if (TARGET aptdec)
    install(TARGETS aptdec)
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/palettes" DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME})
endif()

install(TARGETS apt PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/apt)

include(CPack)
