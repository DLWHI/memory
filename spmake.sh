#!/bin/sh
if [ $# -ne 1 ]; then
    echo "spmake: no action specified";
    exit 1;
fi

CXXFLAGS_DEBUG="-g"
LD_FLAGS_DEBUG=""
CXX_FLAGS_RELEASE="-Wall -Werror -Wextra -O3"
LD_FLAGS_RELEASE=""
CXXFLAGS_ASAN="-fsanitize=address"
LD_FLAGS_ASAN="-fsanitize=address"

MEMCHECK=valgrind
MEMCHECK_ARGS="--trace-children=yes --track-fds=yes --track-origins=yes \
    --leak-check=full --show-leak-kinds=all -s"
MEMCHECK_SUPPRESSIONS_FILE=

. ./project.cfg;

if [ $1 = "init" ]; then
    cmake -B ${BUILD_PREFIX} -S . \
          -DCMAKE_MODULE_PATH=${CMAKE_MODULES_DIR} \
          -DCMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATIONS} \
          -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
          -DCMAKE_CXX_FLAGS_DEBUG="${CXXFLAGS_DEBUG}" \
          -DCMAKE_EXE_LINKER_FLAGS_DEBUG="${LD_FLAGS_DEBUG}" \
          -DCMAKE_CXX_FLAGS_RELEASE="${CXX_FLAGS_RELEASE}" \
          -DCMAKE_EXE_LINKER_FLAGS_RELEASE="${LD_FLAGS_RELEASE}" \
          -DCMAKE_CXX_FLAGS_ASAN="${CXXFLAGS_ASAN}" \
          -DCMAKE_EXE_LINKER_FLAGS_ASAN="${LD_FLAGS_ASAN}" \
          -DCTEST_MEMORYCHECK_COMMAND=${MEMCHECK} \
          -DCTEST_MEMORYCHECK_COMMAND_OPTIONS="${MEMCHECK_ARGS}" \
          -DCTEST_MEMORYCHECK_SUPPRESSIONS_FILE="${MEMCHECK_SUPPRESSIONS_FILE}";
elif [ $1 = "build" ]; then
    cmake --build ${BUILD_PREFIX} --config ${BUILD_TYPE} --target ${TARGET_NAME};
elif [ $1 = "test" ]; then
    cmake --build ${BUILD_PREFIX} --config ${BUILD_TYPE} --target unit_tests;
    cd ${BUILD_PREFIX} && ctest
elif [ $1 = "install" ]; then
    cmake --install ${BUILD_PREFIX} --config ${BUILD_TYPE} --target ${TARGET_NAME};
elif [ $1 = "memcheck" ]; then
    cmake --build ${BUILD_PREFIX} --config ${BUILD_TYPE} --target unit_tests;
    cd ${BUILD_PREFIX} && ctest -T memcheck
elif [ $1 = "clean" ]; then
    rm -rf ${BUILD_PREFIX}
fi;
