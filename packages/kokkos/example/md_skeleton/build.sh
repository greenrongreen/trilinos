#!/bin/bash

#-----------------------------------------------------------------------------
# Simple build script with options
#-----------------------------------------------------------------------------

# Directory for Kokkos

KOKKOS="../../core"

source ${KOKKOS}/src/build_common.sh

# Process command line options and set compilation variables
#
# INC_PATH     : required include paths
# CXX          : C++ compiler with compiler-specific options
# CXX_SOURCES  : C++ files for host
# NVCC         : Cuda compiler with compiler-specific options
# NVCC_SOURCES : Cuda sources

#-----------------------------------------------------------------------------
# Add local source code:

EXECUTABLE="MD.exe"

INC_PATH="${INC_PATH} -I. "

CXX_SOURCES="${CXX_SOURCES} "
CXX_SOURCES="${CXX_SOURCES} ./*.cpp"

#-----------------------------------------------------------------------------

if [ -n "${NVCC}" ] ;
then
  NVCC_SOURCES="${NVCC_SOURCES} ./*.cu"

  echo ${NVCC} ${INC_PATH} ${NVCC_SOURCES}

  ${NVCC} ${INC_PATH} ${NVCC_SOURCES}
fi

#-----------------------------------------------------------------------------

echo ${CXX} ${INC_PATH} -o ${EXECUTABLE} ${CXX_SOURCES} ${LIB}

${CXX} ${INC_PATH} -o ${EXECUTABLE} ${CXX_SOURCES} ${LIB}

rm -f *.o *.a

#-----------------------------------------------------------------------------

