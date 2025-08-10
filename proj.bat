@echo off

setlocal ENABLEDELAYEDEXPANSION

set ARGC=0
for %%x in (%*) do set /A ARGC += 1

if %ARGC% neq 1 (
    echo "proj: unknown no action specified"
    exit /B 1
)

set BUILD_PREFIX="build-Win32"

for /f "delims=" %%L in (project.cfg) do set %%L

set COMPILER_CONFIG=-DFETCH_GTEST=%FETCH_GTEST%

if "%CMAKE_GENERATOR%" neq "" (
  if "%MAKE_PROGRAM%" neq "" (
    set COMPILER_CONFIG=!COMPILER_CONFIG! -G "%CMAKE_GENERATOR%"
    set COMPILER_CONFIG=!COMPILER_CONFIG! -DCMAKE_MAKE_PROGRAM="%MAKE_PROGRAM%"
  )
)

if "%C_COMPILER%" neq "" (
  if "%CXX_COMPILER%" neq "" (
    set COMPILER_CONFIG=!COMPILER_CONFIG! -DCMAKE_C_COMPILER="%C_COMPILER%"
    set COMPILER_CONFIG=!COMPILER_CONFIG! -DCMAKE_CXX_COMPILER="%CXX_COMPILER%"
  )
)

if "%1" equ "init" (
    cmake -B %BUILD_PREFIX% -S . %COMPILER_CONFIG%
    exit /B 0
)

if "%1" equ "build" (
    cmake --build %BUILD_PREFIX% --config %CMAKE_BUILD_TYPE% --target %TARGET_NAME%
    exit /B 0
)

if "%1" equ "test" (
    cmake --build %BUILD_PREFIX% --config %CMAKE_BUILD_TYPE% --target unit_tests
    pushd %BUILD_PREFIX%
        ctest
    popd
    exit /B 0
)

if "%1" equ "install" (
    cmake --install %BUILD_PREFIX% --config %CMAKE_BUILD_TYPE% --target %TARGET_NAME%
    exit /B 0
)

if "%1" equ "memcheck" (
    cmake --build %BUILD_PREFIX% --config %CMAKE_BUILD_TYPE% --target unit_tests
    pushd %BUILD_PREFIX%
        ctest -T memcheck
    popd
    exit /B 0
)

if "%1" equ "clean" (
    rmdir /S /Q %BUILD_PREFIX%
    exit /B 0
) else (
    echo "proj: unknown action"
)

endlocal
