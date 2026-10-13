@echo off
SETLOCAL

cd ..\msvc-full-features
set PATH=%PATH%;%VSAPPIDDIR%\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd
for /F "tokens=*" %%i in ('git rev-parse --short=12 HEAD') do set VERSION=%%i
for /F "tokens=*" %%i in ('git rev-parse HEAD') do set FULLSHA=%%i
if "%VERSION%"=="" set VERSION=unknown
if "%FULLSHA%"=="" set FULLSHA=unknown
if "%RODINA_VERSION%"=="" set RODINA_VERSION=0.1-dev
if "%RODINA_VERSION_RESOURCE%"=="" set RODINA_VERSION_RESOURCE=0,1,0,0
set WRITE_VERSION=
findstr /c:%VERSION% ..\src\version.h > NUL 2> NUL
if %ERRORLEVEL% NEQ 0 set WRITE_VERSION=1
findstr /c:%RODINA_VERSION% ..\src\version.h > NUL 2> NUL
if %ERRORLEVEL% NEQ 0 set WRITE_VERSION=1
findstr /c:"RODINA_VERSION_RESOURCE %RODINA_VERSION_RESOURCE%" ..\src\version.h > NUL 2> NUL
if %ERRORLEVEL% NEQ 0 set WRITE_VERSION=1
if defined WRITE_VERSION (
  (
    echo #define VERSION "%VERSION%"
    echo #define RODINA_VERSION "%RODINA_VERSION%"
    echo #define RODINA_VERSION_RESOURCE %RODINA_VERSION_RESOURCE%
  ) > ..\src\version.h
)
(
  echo product: Cataclysm: RODINA
  echo rodina version: %RODINA_VERSION%
  echo upstream version: CDDA 0.I-1
  echo commit sha: %FULLSHA%
  echo commit url: https://github.com/eugene-lesnov/Cataclysm-RODINA/commit/%FULLSHA%
) > ..\VERSION.txt
