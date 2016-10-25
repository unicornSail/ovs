# Install script for directory: /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/src

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "Release")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "0")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FOREACH(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libyajl.so.2.1.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libyajl.so.2"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libyajl.so"
      )
    IF(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      FILE(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    ENDIF()
  ENDFOREACH()
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/yajl-2.1.0/lib/libyajl.so.2.1.0"
    "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/yajl-2.1.0/lib/libyajl.so.2"
    "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/yajl-2.1.0/lib/libyajl.so"
    )
  FOREACH(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libyajl.so.2.1.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libyajl.so.2"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libyajl.so"
      )
    IF(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      IF(CMAKE_INSTALL_DO_STRIP)
        EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "${file}")
      ENDIF(CMAKE_INSTALL_DO_STRIP)
    ENDIF()
  ENDFOREACH()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/yajl-2.1.0/lib/libyajl_s.a")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/yajl" TYPE FILE FILES
    "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/src/api/yajl_parse.h"
    "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/src/api/yajl_gen.h"
    "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/src/api/yajl_common.h"
    "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/src/api/yajl_tree.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/yajl" TYPE FILE FILES "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/src/../yajl-2.1.0/include/yajl/yajl_version.h")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pkgconfig" TYPE FILE FILES "/home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/src/../yajl-2.1.0/share/pkgconfig/yajl.pc")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

