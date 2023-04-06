@echo off

rem Set compiler
set CC=cl.exe
set CXX=cl.exe

rem Set variables
set APP_SOURCE=%~dp0\tibra
set APP_BUILD=%APP_SOURCE%\..\build

rem Set basic configuration
set CMAKE_BUILD_TYPE=Release
set BOOST_ROOT=D:\TEMP\boost\boost_1_81_0

rem rem Clean
del /F /Q "%APP_BUILD%\%CMAKE_BUILD_TYPE%\cmake_install.cmake"
del /F /Q "%APP_BUILD%\%CMAKE_BUILD_TYPE%\CMakeCache.txt"
del /F /Q "%APP_BUILD%\%CMAKE_BUILD_TYPE%\CMakeFiles"

rem Configure
@echo on
cmake -G"Visual Studio 17 2022" -A x64 -H"%APP_SOURCE%" -B"%APP_BUILD%\%CMAKE_BUILD_TYPE%"  ^
-DUSE_CGAL=OFF                                                                       ^
-DBOOST_ROOT="D:\TEMP\boost\boost_1_81_0"                                              ^
-DCGAL_DIR="..."

rem Build
cmake --build "%APP_BUILD%\%CMAKE_BUILD_TYPE%" --target install -- /property:configuration=%CMAKE_BUILD_TYPE% /p:Platform=x64
goto:eof