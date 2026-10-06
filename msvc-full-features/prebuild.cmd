@echo off
SETLOCAL

cd ..\msvc-full-features
set PATH=%PATH%;%VSAPPIDDIR%\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd
for /F "tokens=*" %%i in ('git rev-parse --short=12 HEAD') do set VERSION=%%i
for /F "tokens=*" %%i in ('git rev-parse HEAD') do set FULLSHA=%%i
if "%VERSION%"=="" set VERSION=unknown
if "%FULLSHA%"=="" set FULLSHA=unknown
findstr /c:%VERSION% ..\src\version.h > NUL 2> NUL
if %ERRORLEVEL% NEQ 0 (
  >..\src\version.h echo #define VERSION "%VERSION%"
)
(
  echo product: Cataclysm: RODINA
  echo rodina version: 0.1-dev
  echo upstream version: CDDA 0.I-1
  echo commit sha: %FULLSHA%
  echo commit url: https://github.com/eugene-lesnov/Cataclysm-RODINA/commit/%FULLSHA%
) > ..\VERSION.txt
