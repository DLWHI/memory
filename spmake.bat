@echo off

set ARGC=0
for %%x in (%*) do set /A ARGC += 1

if %ARGC% neq 1 (J
    echo "spmake.bat: no action specified"
    exit /B 1
)

set CXXFLAGS_DEBUG=/Zi
set LD_FLAGS_DEBUG=
set CXX_FLAGS_RELEASE=/W4 /O2
set LD_FLAGS_RELEASE=
set CXXFLAGS_ASAN=/fsanitize=address
set LD_FLAGS_ASAN=/fsanitize=address

set MEMCHECK=drmemory
set MEMCHECK_ARGS=
set MEMCHECK_SUPPRESSIONS_FILE=

for /f "delims=" %%L in (project.cfg) do set %%L

if "%1" equ "init" (
    cmake -B %BUILD_PREFIX% -S . ^
          -DCMAKE_MODULE_PATH=%CMAKE_MODULES_DIR% ^
          -DCMAKE_CONFIGURATION_TYPES=%CMAKE_CONFIGURATIONS% ^
          -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
          -DCMAKE_CXX_STANDARD=%STD% ^
          -DCMAKE_CXX_FLAGS_DEBUG="%CXXFLAGS_DEBUG%" ^
          -DCMAKE_EXE_LINKER_FLAGS_DEBUG="%LD_FLAGS_DEBUG%" ^
          -DCMAKE_CXX_FLAGS_RELEASE="%CXX_FLAGS_RELEASE%" ^
          -DCMAKE_EXE_LINKER_FLAGS_RELEASE="%LD_FLAGS_RELEASE%" ^
          -DCMAKE_CXX_FLAGS_ASAN="%CXXFLAGS_ASAN%" ^
          -DCMAKE_EXE_LINKER_FLAGS_ASAN="%LD_FLAGS_ASAN%" ^
          -DCTEST_MEMORYCHECK_COMMAND=%MEMCHECK% ^
          -DCTEST_MEMORYCHECK_COMMAND_OPTIONS="%MEMCHECK_ARGS%" ^
          -DCTEST_MEMORYCHECK_SUPPRESSIONS_FILE="%MEMCHECK_SUPPRESSIONS_FILE%";
    exit /B 0
)

if "%1" equ "build" (
    cmake --build %BUILD_PREFIX% --config %BUILD_TYPE% --target %TARGET_NAME%;
    exit /B 0
)

if "%1" equ "test" (
    cmake --build %BUILD_PREFIX% --config %BUILD_TYPE% --target unit_tests;
    pushd %BUILD_PREFIX%
        ctest
    popd
    exit /B 0
)

if "%1" equ "install" (
    cmake --install %BUILD_PREFIX% --config %BUILD_TYPE% --target %TARGET_NAME%;
    exit /B 0
)

if "%1" equ "memcheck" (
    cmake --build %BUILD_PREFIX% --config %BUILD_TYPE% --target unit_tests;
    pushd %BUILD_PREFIX%
        ctest -T memcheck
    popd
    exit /B 0
)

if "%1" equ "clean" (
    rmdir /S /Q %BUILD_PREFIX%
    exit /B 0
)
