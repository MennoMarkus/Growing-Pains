@echo off

REM usage: build.bat [config] [languageVersion]
REM config - { debug, release, preprocessor }, default=debug
REM languageVersion - { c, cpp }, default=c

REM 1) Setup configuration
set config=%1
set languageVersion=%2
if "%config%" == "" set config="debug"
if "%languageVersion%" == "" set languageVersion="cpp"
set rootFolder=%cd%
set source=%cd%/main.c
set includes=
set libs=
set libPaths=
set outputName=%cd%/output/GrowingPains_Dev.exe

REM /nologo                 - suppress startup banner
REM /Od                     - disable optimisations
REM /O2                     - full optimisations
REM /Zi                     - generate seprate pdb file
REM /W4                     - warning level 4 
REM /WX                     - threat warnings as errors 
REM /OPT:REF                - remove functions which are never referenced
REM /P                      - output the preprocessor result to a file 
set debugOpts=/nologo /Od /W4 /WX /Zi 
set releaseOpts=/nologo /O2 /W4 /WX /Zi 
set linkOpts=/OPT:REF

set opts=%debugOpts%
if %config% == "preprocessor" set opts=/P %debugOpts%
if %config% == "release" set opts=%releaseOpts%

REM 2) Set the language version
REM /Zc:__cplusplus         - enable usage of __cplusplus macro to reflect the correct value (rather than always C++98)
REM /Za                     - disable language extensions not compatible with C89/C90
REM /Tc                     - enforces compilation with C
REM /std:c++14              - set C++ version to C++14 (as of writing minimum supported version for the current toolset)
REM /Tp                     - enforces compilation with C++
if %languageVersion% == "c" set opts=%opts%/Za /Tc 
if %languageVersion% == "cpp" set opts=%opts%/Zc:__cplusplus /std:c++14 /Tp

REM 3) Create output directories
if not exist output mkdir output
if not exist build mkdir build
cd build

REM 4) Setup msvc cl
setlocal
set VC_OPTS=amd64
set "LIB="

REM Microsoft stupidity. Search system for cl.
set VC_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 14.0
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 13.0
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 11.0
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" %VC_OPTS%))

set VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 10.0
if not defined LIB (if exist "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" %VC_OPTS%))

:compileSetup

REM 5) Compile executable
call cl %opts% %source% %includes% /I"%rootFolder%" /link %linkOpts% %libs% %libPaths% /out:%outputName%
endlocal

cd ..
