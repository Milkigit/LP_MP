#include(ExternalProject)
#
#set_property (DIRECTORY PROPERTY EP_BASE Dependencies)
#
#set (DEPENDENCIES)
#set (EXTRA_CMAKE_ARGS)

# SIMD library
# note: Ubuntu has package vc-dev, but it is old (version 0.7.4), hence unusable
#list (APPEND DEPENDENCIES Vc_Project)
#ExternalProject_Add(
#   Vc_Project
#   GIT_REPOSITORY "https://github.com/VcDevel/Vc.git"
#   GIT_TAG "1.2.0"
#   CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/Dependencies/Install/Vc_Project"
#   BUILD_COMMAND make Vc
#   INSTALL_COMMAND make install
#   )
#ExternalProject_Get_Property(Vc_Project install_dir)
#add_library(Vc STATIC IMPORTED)
#add_dependencies(Vc Vc_Project)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake"
#   -DVc_ROOT=${CMAKE_CURRENT_BINARY_DIR}/Dependencies/Source/Vc_Project
#   )

# boost hana for compile time metaprogramming. However only use when find_package could not find valid hana installation
#list(APPEND DEPENDENCIES Hana_Project)
#ExternalProject_Add(
#   Hana_Project
#   GIT_REPOSITORY "https://github.com/boostorg/hana.git"
#   GIT_TAG "v1.0.0"
#   #CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/Dependencies/Install/Hana_Project --target install"
#   BUILD_COMMAND ""
#   INSTALL_COMMAND ""
#   #BUILD_COMMAND cmake --build
#   #INSTALL_COMMAND cmake --target install -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/Dependencies/Install/Hana_Project
#  )
#ExternalProject_Get_Property(Hana_Project install_dir)
#include_directories(${install_dir}/Dependencies/Source/Hana_Project/include)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
#set(Hana_INCLUDE_DIR "${install_dir}/Dependencies/Source/Hana_Project/include")
#find_package_handle_standard_args(Hana
#   REQUIRED_VARS Hana_INCLUDE_DIR
#   )

# meta package for compile time metaprogramming
#list(APPEND DEPENDENCIES meta_Project)
#ExternalProject_Add(
#   meta_Project
#   GIT_REPOSITORY "https://github.com/ericniebler/meta.git"
#   GIT_TAG "master"
#   INSTALL_COMMAND ""
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(meta_Project install_dir)
#include_directories(${install_dir}/Dependencies/Source/meta_Project/include)

# unit tests framework
#list(APPEND DEPENDENCIES Catch_Project)
#ExternalProject_Add(
#   Catch_Project
#   GIT_REPOSITORY "https://github.com/philsquared/Catch.git"
#   GIT_TAG "v1.5.6"
#   INSTALL_COMMAND ""
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(Catch_Project install_dir)
#include_directories(${install_dir}/Dependencies/Source/Catch_Project/include)

# sorting routines
#list(APPEND DEPENDENCIES cpp_sort_Project)
#ExternalProject_Add(
#   cpp_sort_Project
#   GIT_REPOSITORY "https://github.com/Morwenn/cpp-sort.git"
#   GIT_TAG "master"
#   INSTALL_COMMAND ""
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(cpp_sort_Project install_dir)
#include_directories(${install_dir}/Dependencies/Source/cpp_sort_Project/include)

# opengm for loading experiments in their hdf5 format
#list(APPEND DEPENDENCIES OpenGM_Project)
#ExternalProject_Add(
#   OpenGM_Project
#   GIT_REPOSITORY "https://github.com/opengm/opengm.git"
#   GIT_TAG "master"
#   INSTALL_COMMAND ""
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(OpenGM_Project install_dir)
#include_directories(${install_dir}/Dependencies/Source/OpenGM_Project/include)

# PEGTL for building grammars
#list(APPEND DEPENDENCIES PEGTL_Project)
#ExternalProject_Add(
#   PEGTL_Project
#   GIT_REPOSITORY "https://github.com/ColinH/PEGTL.git"
#   GIT_TAG "1.3.1"
#   INSTALL_COMMAND ""
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(PEGTL_Project install_dir)
#include_directories(${install_dir}/Dependencies/Source/PEGTL_Project/include)

#IF(BUILD_MULTICUT)
# Bjoern Andres graph package for multicut
#list(APPEND DEPENDENCIES Andres_Project)
#ExternalProject_Add(
#   Andres_Project
#   GIT_REPOSITORY "https://github.com/bjoern-andres/graph.git"
#   GIT_TAG "master"
#   INSTALL_COMMAND "" 
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(Andres_Project install_dir)
#include_directories(${install_dir}/Dependencies/Source/Andres_Project/include)
#ENDIF()

#list(APPEND DEPENDENCIES Cereal_Project)
#ExternalProject_Add(
#   Cereal_Project
#   GIT_REPOSITORY "https://github.com/USCiLab/cereal.git"
#   GIT_TAG "master"
#   INSTALL_COMMAND "" 
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(Cereal_Project install_dir)
#include_directories(${install_dir}/Dependencies/Source/Cereal_Project/include)

# command line library
#list(APPEND DEPENDENCIES TCLAP_Project)
#ExternalProject_ADD(
#   TCLAP_Project
#   URL "http://downloads.sourceforge.net/project/tclap/tclap-1.2.1.tar.gz"
#   # or http://kent.dl.sourceforge.net/project/tclap/tclap-1.2.1.tar.gz
#   INSTALL_COMMAND ""
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(TCLAP_Project install_dir)

# minimum cost flow solvers for e.g. graph matching
#list(APPEND DEPENDENCIES LEMON_Project)
#ExternalProject_ADD(
#   LEMON_Project
#   URL "http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.tar.gz"
#   UPDATE_COMMAND ""
#   INSTALL_COMMAND ""
#   BUILD_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(LEMON_Project install_dir)

# minimum cost flow solver based on Goldberg's implementation
#list(APPEND DEPENDENCIES CS2_CPP_Project)
#ExternalProject_ADD(
#   CS2_CPP_Project
#   GIT_REPOSITORY "https://github.com/pawelswoboda/CS2-CPP.git"
#   GIT_TAG "master"
#   CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/Dependencies/Install/CS2_CPP_Project"
#   BUILD_COMMAND ""
#   INSTALL_COMMAND ""
#   CONFIGURE_COMMAND ""
#   )
#ExternalProject_Get_Property(CS2_CPP_Project install_dir)

#list(APPEND DEPENDENCIES CryptoMiniSat_Project)
#ExternalProject_ADD(
#   CryptoMiniSat_Project
#   GIT_REPOSITORY "https://github.com/msoos/cryptominisat.git"
#   GIT_TAG "master"
#   CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/Dependencies/Install/CryptoMiniSat_Project"
#   BUILD_COMMAND make
#   INSTALL_COMMAND ""
#   CONFIGURE_COMMAND cmake ../../Source/CryptoMiniSat_Project/
#   )
#ExternalProject_Get_Property(CS2_CPP_Project install_dir)

# lingeling SAT solver
#list(APPEND DEPENDENCIES Lingeling_Project)
#ExternalProject_ADD(
#   Lingeling_Project
#   URL "http://fmv.jku.at/lingeling/lingeling-bal-2293bef-151109.tar.gz"
#   INSTALL_COMMAND ""
#   BUILD_IN_SOURCE 1
#   BUILD_COMMAND "make"
#   CONFIGURE_COMMAND "./configure.sh"
#   )
#ExternalProject_Get_Property(TCLAP_Project install_dir)


#ExternalProject_Add (LP_MP
#   DEPENDS ${DEPENDENCIES}
#   SOURCE_DIR ${PROJECT_SOURCE_DIR}
#   CMAKE_ARGS -DDOWNLOAD_DEPENDENCIES=OFF ${EXTRA_CMAKE_ARGS}
#   INSTALL_COMMAND ""
#   BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
