Rem Package transcli for Windows 32 bit platform
md release_win32
cd release_win32
md transcli
cd transcli
md codecs tools preset APPSUBFONTDIR
md codecs\fonts
cd ..
xcopy /y /d ..\bin\win32\*.exe .\transcli\
xcopy /y /d ..\bin\win32\*.dll .\transcli\
xcopy /y /d ..\bin\win32\mcnt.xml .\transcli\
xcopy /e /i /d ..\bin\win32\preset .\transcli\preset
xcopy /e /i /d ..\bin\win32\APPSUBFONTDIR .\transcli\APPSUBFONTDIR

xcopy /y /d ..\bin\win32\codecs\*.exe .\transcli\codecs\
xcopy /y /d ..\bin\win32\codecs\*.dll .\transcli\codecs\
xcopy /e /i /d ..\bin\win32\codecs\fonts .\transcli\codecs\fonts

xcopy /y /d ..\bin\win32\tools\*.exe .\transcli\tools\
xcopy /y /d ..\bin\win32\tools\*.dll .\transcli\tools\

Rem package int 7-zip
..\bin\7z.exe a -t7z Transcli_win32_%date:~0,4%-%date:~5,2%-%date:~8,2%.7z -r -y -x!*.svn .\transcli