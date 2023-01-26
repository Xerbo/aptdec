# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR armhf)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf/ ${CMAKE_INSTALL_PREFIX})
# Hack
link_directories(${CMAKE_INSTALL_PREFIX}/lib/arm-linux-gnueabihf)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
