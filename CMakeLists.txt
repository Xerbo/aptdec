cmake_minimum_required (VERSION 3.0.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(aptdec)

# libpng
find_package(PNG)

# libsndfile
find_package(LibSndFile)

set(LIB_C_SOURCE_FILES src/color.c src/dsp.c src/filter.c src/image.c src/libs/reg.c src/libs/median.c)
set(EXE_C_SOURCE_FILES src/main.c src/pngio.c src/libs/argparse.c)
set(LIB_C_HEADER_FILES src/apt.h)

add_library(libaptdec SHARED ${LIB_C_SOURCE_FILES})

add_compile_definitions(PALETTE_DIR="../palettes")

if (PNG_FOUND AND LIBSNDFILE_FOUND)
    add_executable(aptdec ${EXE_C_SOURCE_FILES})
    target_link_libraries(aptdec PRIVATE libaptdec)
    target_link_libraries(aptdec PRIVATE PNG::PNG)
    target_link_libraries(aptdec PRIVATE ${LIBSNDFILE_LIBRARY})
    if (CLANG OR GCC)
        # Math
        target_link_libraries(aptdec PRIVATE m)

        # Throw errors on warnings on release builds
        if(CMAKE_BUILD_TYPE MATCHES "Release")
            target_compile_options(aptdec PRIVATE -Wall -Wextra -pedantic -Wno-missing-field-initializers -Werror)
        else()
            target_compile_options(aptdec PRIVATE -Wall -Wextra -pedantic)
        endif()
    elseif (MSVC)
        target_compile_options(aptdec PRIVATE /D_CRT_SECURE_NO_WARNINGS=1)
    endif()
else()
    MESSAGE(STATUS "WARNING: Only building libaptdec, as not all of the required libraries were found.")
endif()

if (CLANG OR CGCC)
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(libaptdec PRIVATE -Wall -Wextra -pedantic -Wno-missing-field-initializers -Werror)
    else()
        target_compile_options(libaptdec PRIVATE -Wall -Wextra -pedantic)
    endif()
elseif (MSVC)
    target_compile_options(libaptdec PRIVATE /D_CRT_SECURE_NO_WARNINGS=1)
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

    if (TARGET aptdec)
        install(TARGETS aptdec DESTINATION "bin/")
        install(DIRECTORY "${PROJECT_SOURCE_DIR}/palettes/" DESTINATION "palettes/")
    endif()

    install(TARGETS libaptdec LIBRARY DESTINATION lib)
    install(FILES ${LIB_C_HEADER_FILES} DESTINATION include/apt )
else()
    #set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

    if (TARGET aptdec)
        install(TARGETS aptdec DESTINATION "bin/")
        file(GLOB_RECURSE DLLS *.dll)
        install(FILES ${DLLS} DESTINATION "bin/")
    endif()
    install(TARGETS libaptdec DESTINATION "bin/")
    install(FILES ${LIB_C_HEADER_FILES} DESTINATION include/apt )
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/palettes/" DESTINATION "palettes/")
endif()

include(CPack)
