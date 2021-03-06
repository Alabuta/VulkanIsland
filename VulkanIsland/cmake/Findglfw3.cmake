##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2016 Sandia Corporation.
##  Copyright 2016 UT-Battelle, LLC.
##  Copyright 2016 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
# Try to find EGL library and include dir.
# Once done this will define
#
# GLFW_FOUND
# GLFW_INCLUDE_DIR
# GLFW_LIBRARY
#

message("44444" ${PROJECT_SOURCE_DIR})

include(FindPackageHandleStandardArgs)

if (WIN32)
    find_path(GLFW_INCLUDE_DIR
        NAMES
            GLFW/glfw3.h
        PATHS
            ${PROJECT_SOURCE_DIR}/../external/glfw/include
    )
    if(ARCH STREQUAL "x86")
      find_library( GLFW_LIBRARY
          NAMES
              glfw3
          PATHS
              ${GLFW_LOCATION}/lib
              $ENV{GLFW_LOCATION}/lib
              $ENV{PROGRAMFILES}/GLFW/lib
              DOC "The GLFW library")
    else()
      find_library( GLFW_LIBRARY
          NAMES
              glfw3
          PATHS
              ${GLFW_LOCATION}/lib
              $ENV{GLFW_LOCATION}/lib
              $ENV{PROGRAMFILES}/GLFW/lib
              DOC "The GLFW library")
    endif()
endif ()

if (${CMAKE_HOST_UNIX})
    find_path(GLFW_INCLUDE_DIR
        NAMES
            GLFW/glfw3.h
        PATHS
            ${PROJECT_SOURCE_DIR}/../external/glfw/include
            NO_DEFAULT_PATH
            DOC "The directory where GLFW/glfw3.h resides"
    )
    find_library(GLFW_LIBRARY
        NAMES
            glfw3 glfw
        PATHS
            ${PROJECT_SOURCE_DIR}/../external/glfw/build/src
            NO_DEFAULT_PATH
            DOC "The GLFW library")
endif ()

find_package_handle_standard_args(GLFW DEFAULT_MSG
    GLFW_INCLUDE_DIR
    GLFW_LIBRARY
)

mark_as_advanced(GLFW_FOUND)

message("44444" ${GLFW_INCLUDE_DIR} ${GLFW_LIBRARY})
