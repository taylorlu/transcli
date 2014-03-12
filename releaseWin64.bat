Rem Package transcli for Windows 64 bit platform
md release_win64
cd release_win64
md transcli
cd transcli
md codecs tools preset APPSUBFONTDIR
md codecs\fonts
cd ..
xcopy /y /d ..\bin\x64\*.exe .\transcli\
xcopy /y /d ..\bin\x64\*.dll .\transcli\
xcopy /y /d ..\bin\x64\mcnt.xml .\transcli\
xcopy /e /i /d ..\bin\x64\preset .\transcli\preset
xcopy /e /i /d ..\bin\x64\APPSUBFONTDIR .\transcli\APPSUBFONTDIR

xcopy /y /d ..\bin\x64\codecs\*.exe .\transcli\codecs\
xcopy /y /d ..\bin\x64\codecs\*.dll .\transcli\codecs\
xcopy /e /i /d ..\bin\x64\codecs\fonts .\transcli\codecs\fonts

xcopy /y /d ..\bin\x64\tools\*.exe .\transcli\tools\
xcopy /y /d ..\bin\x64\tools\*.dll .\transcli\tools\

Rem package int 7-zip
..\bin\7z.exe a -t7z Transcli_win64_%date:~0,4%-%date:~5,2%-%date:~8,2%.7z -r -y -x!*.svn .\transcli