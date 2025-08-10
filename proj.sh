#!/bin/sh
BUILD_PREFIX="build-$(uname)";
if [ $# -ne 1 ]; then
    echo "proj: unknown action specified";
    exit 1;
fi;

. ./project.cfg;

COMPILER_CONFIG="-DFETCH_GTEST=$FETCH_GTEST"

if [[ ! -z "$CMAKE_GENERATOR" ]] && ![[ ! -z "$MAKE_PROGRAM" ]]; then
  COMPILER_CONFIG+=" -G \"$CMAKE_GENERATOR\""
  COMPILER_CONFIG+="-DCMAKE_MAKE_PROGRAM=$MAKE_PROGRAM"
fi

if [[ ! -z "$C_COMPILER" ]] && [[ ! -z "$CXX_COMPILER" ]]; then
  COMPILER_CONFIG+=" -DCMAKE_C_COMPILER=$C_COMPILER"
  COMPILER_CONFIG+="-DCMAKE_CXX_COMPILER=$CXX_COMPILER"
fi

if [ $1 = "init" ]; then
    cmake -B ${BUILD_PREFIX} -S . $COMPILER_CONFIG
elif [ $1 = "build" ]; then
    cmake --build ${BUILD_PREFIX} --config ${CMAKE_BUILD_TYPE} --target ${TARGET_NAME};
elif [ $1 = "test" ]; then
    cmake --build ${BUILD_PREFIX} --config ${CMAKE_BUILD_TYPE} --target unit_tests;
    if [ $? -ne 0 ]; then
        exit $?;
    fi;
    cd ${BUILD_PREFIX} && ctest
elif [ $1 = "install" ]; then
    cmake --install ${BUILD_PREFIX} --config ${CMAKE_BUILD_TYPE} --target ${TARGET_NAME};
elif [ $1 = "memcheck" ]; then
    cmake --build ${BUILD_PREFIX} --config ${CMAKE_BUILD_TYPE} --target unit_tests;
    if [ $? -ne 0 ]; then
        exit $?;
    fi;
    cd ${BUILD_PREFIX} && ctest -T memcheck
elif [ $1 = "clean" ]; then
    rm -rf ${BUILD_PREFIX};
else
    echo "proj: unknown option"
    exit 1;
fi;

exit $?;
