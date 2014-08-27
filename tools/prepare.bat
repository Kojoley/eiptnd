@echo off

set BOOST_ROOT=C:\boost_1_56_0
set TOOLSET=gcc
set PREFIX=C:\Boost

call::GetAbsolutePath "BOOST_ROOT" "%BOOST_ROOT%"
echo %BOOST_ROOT%

call:InstallGitToBoost "https://github.com/retf/Boost.Application.git" "boost.application"
call:InstallGitToBoost "https://github.com/apolukhin/Boost.Plugin.git" "boost.plugin"

cd %BOOST_ROOT%
@echo on
if NOT EXIST b2.exe ( call bootstrap.bat )
call b2 toolset=%TOOLSET% -j%NUMBER_OF_PROCESSORS% -q --build-type=complete stage install --prefix="%PREFIX%"

::--------------------------------------------------------
::-- Function section starts below
::--------------------------------------------------------
goto:eof

:InstallGitToBoost
pushd .
setlocal ENABLEEXTENSIONS
set ATTR=%~a2
set DIRATTR=%ATTR:~0,1%
if /I "%DIRATTR%"=="d" (
  echo Pull existing repository %~f2
  call cd %~2
  call git reset --hard HEAD && git pull origin master --depth=1
) else (
  echo Making shallow-copy of repository %~1 to %~2
  call git clone -b master --single-branch --depth=1 %~1 %~2
  call cd %~2
)
for /f %%i IN ('dir /B /A-D include\boost') do (set LIBNAME=%%~ni)
call xcopy /y /s /e include %BOOST_ROOT%\
set LIBPATH=%BOOST_ROOT%\libs\%LIBNAME%
call mkdir %LIBPATH%
call xcopy /y /s /e doc %LIBPATH%\doc\
call xcopy /y /s /e example %LIBPATH%\example\
call xcopy /y /s /e test %LIBPATH%\test\
call xcopy /y index.html %LIBPATH%\
popd
goto:eof

:GetAbsolutePath
pushd .
cd %~dp0
set %~1=%~f2
popd
goto:eof
