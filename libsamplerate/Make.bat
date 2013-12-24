@echo off
set ROOT=F:\dev\GPU\libsamplerate
set WIN32=%ROOT%\Win32
set SRC=%ROOT%src
set EXAMPLE=%ROOT%\examples

if "%1"=="check" GOTO CHECK
if "%1"=="clean" GOTO CLEAN

copy /y %WIN32%\config.h %SRC%\config.h
copy /y %WIN32%\unistd.h %EXAMPLE%\unistd.h

nmake -f %WIN32%\Makefile.msvc
goto END


:CHECK
nmake -f %WIN32%\Makefile.msvc check
goto END

:CLEAN
nmake -f %WIN32%\Makefile.msvc clean
goto END

:END

