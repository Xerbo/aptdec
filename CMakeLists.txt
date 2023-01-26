cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

# Get version
find_package(Git)
if (GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tag WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(VERSION "${GIT_TAG}")
else()
    set(VERSION "Unknown")
endif()

project(aptdec C)
include(GNUInstallDirs)

# aptdec-cli
find_package(PNG)
find_path(SNDFILE_INCLUDE_DIR sndfile.h)
find_library(SNDFILE_LIBRARIES NAMES sndfile libsndfile)

if(PNG_FOUND AND SNDFILE_LIBRARIES AND SNDFILE_INCLUDE_DIR)
    set(APTDEC_CLI_SOURCE_FILES
        aptdec-cli/argparse/argparse.c
        aptdec-cli/main.c
        aptdec-cli/pngio.c
        aptdec-cli/util.c
    )
    add_executable(aptdec-cli ${APTDEC_CLI_SOURCE_FILES})

    target_compile_definitions(aptdec-cli PRIVATE "VERSION=\"${VERSION}\"")
    target_include_directories(aptdec-cli PRIVATE ${PNG_INCLUDE_DIRS})
    target_include_directories(aptdec-cli PRIVATE ${SNDFILE_INCLUDE_DIR})
    target_include_directories(aptdec-cli PRIVATE libaptdec/include/)
    target_link_libraries(aptdec-cli PRIVATE ${PNG_LIBRARIES})
    target_link_libraries(aptdec-cli PRIVATE ${SNDFILE_LIBRARIES})
    target_link_libraries(aptdec-cli PRIVATE aptdec)

    if(MSVC)
        target_compile_options(aptdec-cli PRIVATE /D_CRT_SECURE_NO_WARNINGS=1)
    else()
        target_link_libraries(aptdec-cli PRIVATE m)
        target_compile_options(aptdec-cli PRIVATE -Wall -Wextra -pedantic -Wno-missing-field-initializers)
    endif()

    if (USE_ADDRESS_SANITIZER)
        target_compile_options(aptdec-cli PRIVATE -fsanitize=address)
        target_link_options(aptdec-cli PRIVATE -fsanitize=address)
    endif()

    install(TARGETS aptdec-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    file(GLOB LUTS luts/*.png)
    install(FILES ${LUTS} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/aptdec/luts)
else()
    message(WARNING "Not building aptdec-cli as some/all required dependencies are missing, libaptdec will still be built")
endif()

# libaptdec
set(LIBAPTDEC_HEADER_FILES libaptdec/include/aptdec.h)
set(LIBAPTDEC_SOURCE_FILES
    libaptdec/algebra.c
    libaptdec/calibration.c
    libaptdec/color.c
    libaptdec/dsp.c
    libaptdec/filter.c
    libaptdec/image.c
    libaptdec/util.c
    libaptdec/effects.c
)
add_library(aptdec SHARED ${LIBAPTDEC_SOURCE_FILES})

set_target_properties(aptdec PROPERTIES PUBLIC_HEADER ${LIBAPTDEC_HEADER_FILES})
target_include_directories(aptdec PRIVATE libaptdec/include/)
target_compile_definitions(aptdec PRIVATE "VERSION=\"${VERSION}\"")

if(MSVC)
    target_compile_options(aptdec PRIVATE /D_CRT_SECURE_NO_WARNINGS=1 /DAPTDEC_API_EXPORT)
else()
    target_link_libraries(aptdec PRIVATE m)
    target_compile_options(aptdec PRIVATE -Wall -Wextra -pedantic -Wno-missing-field-initializers)
endif()
if (USE_ADDRESS_SANITIZER)
    target_compile_options(aptdec PRIVATE -fsanitize=address)
    target_link_options(aptdec PRIVATE -fsanitize=address)
endif()

install(TARGETS aptdec PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Packaging
string(REPLACE v "" CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_PACKAGE_NAME "aptdec")
set(CPACK_PACKAGE_CONTACT "Xerbo (xerbo@protonmail.com)")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NOAA APT satellite imagery decoder")
set(CPACK_PACKAGE_DESCRIPTION "Aptdec is a FOSS library/program that decodes images transmitted by the NOAA POES weather satellites. These satellites transmit constantly (among other things) medium resolution (4km/px) images of the earth over a analog mode called APT.")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsndfile1, libpng16-16")

if(WIN32)
    file(GLOB DLLS ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/*.dll)
    install(FILES ${DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR})

    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "DEB;TGZ")
endif()

include(CPack)
